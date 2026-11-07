#include "precompiled.h"

#include <vector>

#include "netinspector/recorder.h"

namespace
{

cvar_t net_record_anonymize = { "net_record_anonymize", "1", 0, 1.0f, nullptr };

// Entries waiting for the writer thread: a full server moves a few megabytes a second
const uint32 NET_RECORD_QUEUE_BYTES = 64 * 1024 * 1024;

// Shorter parsing of a client's messages is ordinary and is not recorded
const double NET_RECORD_SLOW_CLIENT_MESSAGES = 0.001;

class CServerNetRecord
{
public:
	void Init();
	void Shutdown();
	void Command();

	void OnDatagramReceived(netsrc_t sock, const struct sockaddr &from, const void *data, int length);
	void OnDatagramSent(netsrc_t sock, const struct sockaddr &to, const void *data, int length);
	void OnFrame(int frame);

	void OnClientConnected(const client_t *client);
	void OnClientDropped(const client_t *client, const char *reason);

	void OnServerActivated();
	void OnUserMessagesChanged();
	void OnBufferOverflowed(const sizebuf_t *buf, int length);
	void OnClientMessagesStarted(const client_t *client);
	void OnClientMessagesFinished();

private:
	// What a recording keeps of the traffic
	struct Filter
	{
		enum Mode
		{
			ALL,
			SUBNET,		// ip and prefix bits, any port
			ENDPOINT,	// one client's address and port
		};

		Mode mode;
		uint32 ip;		// host order
		uint32 mask;
		uint16 port;	// host order
	};

	bool Start(const char *target);
	void Stop();
	void PrintStatus() const;

	// The cvars that decide how much a server sends to whom and how often
	void RecordSettings();

	// The session tables of the running server
	void RecordServerTables();
	void RecordUserMessages();
	void RecordDeltaDescriptions();
	void RecordPrecache();

	bool ParseFilter(const char *target, Filter &filter) const;
	bool Matches(const NetInspector::Endpoint &endpoint) const;

	static NetInspector::Endpoint ToEndpoint(const struct sockaddr &address);
	static NetInspector::Endpoint ToEndpoint(const netadr_t &address);

	NetInspector::CRecorder m_Recorder;
	Filter m_Filter;
	double m_ClientMessagesStart;
	NetInspector::Endpoint m_ClientMessagesEndpoint;
	char m_Path[MAX_PATH];
	char m_Target[64];
};

CServerNetRecord s_NetRecord;

void NetRecord_f()
{
	s_NetRecord.Command();
}

void CServerNetRecord::Init()
{
	Q_memset(&m_Filter, 0, sizeof(m_Filter));
	m_ClientMessagesStart = 0.0;
	Q_memset(&m_ClientMessagesEndpoint, 0, sizeof(m_ClientMessagesEndpoint));
	m_Path[0] = '\0';
	m_Target[0] = '\0';

	Cvar_RegisterVariable(&net_record_anonymize);
	Cmd_AddCommand("net_record", NetRecord_f);
}

void CServerNetRecord::Shutdown()
{
	Stop();
}

void CServerNetRecord::Command()
{
	const char *action = Cmd_Argc() >= 2 ? Cmd_Argv(1) : "status";

	if (!Q_stricmp(action, "start"))
		Start(Cmd_Argc() >= 3 ? Cmd_Argv(2) : "all");
	else if (!Q_stricmp(action, "stop"))
		Stop();
	else if (!Q_stricmp(action, "status"))
		PrintStatus();
	else
		Con_Printf("usage: net_record start [all | <ip> | <ip>/<bits> | #<userid>] | stop | status\n");
}

bool CServerNetRecord::Start(const char *target)
{
	if (m_Recorder.IsRecording())
	{
		Con_Printf("net_record: already recording to %s\n", m_Path);
		return false;
	}

	Filter filter;
	if (!ParseFilter(target, filter))
	{
		Con_Printf("net_record: \"%s\" is not all, an address, a subnet or #<userid> of a connected client\n", target);
		return false;
	}

	time_t now = time(nullptr);
	const struct tm *local = localtime(&now);

	FS_CreateDirHierarchy("captures", "GAMECONFIG");
	Q_snprintf(m_Path, sizeof(m_Path), "%s/captures/%04d-%02d-%02d_%02d-%02d-%02d.gcap", com_gamedir,
		local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
	COM_FixSlashes(m_Path);

	NetInspector::CRecorder::Settings settings;
	settings.role = NetInspector::Role::Server;
	settings.application = "ReHLDS " APP_VERSION "-" APP_COMMIT_SHA;
	settings.protocol = PROTOCOL_VERSION;
	settings.anonymizeAddresses = net_record_anonymize.value != 0.0f;
	settings.queueBytes = NET_RECORD_QUEUE_BYTES;
	settings.privateCommands.push_back("g-auth");	// the account token a GoldClient player sends on connect

	if (!m_Recorder.Start(m_Path, settings))
	{
		Con_Printf("net_record: could not write %s\n", m_Path);
		return false;
	}

	m_Filter = filter;
	Q_strlcpy(m_Target, target);

	RecordSettings();
	RecordDeltaDescriptions();
	if (g_psv.active)
		RecordServerTables();
	else
		RecordUserMessages();

	Con_Printf("net_record: recording %s to %s%s\n", m_Target, m_Path, settings.anonymizeAddresses ? "" : ", real addresses kept");
	return true;
}

void CServerNetRecord::Stop()
{
	if (!m_Recorder.IsRecording())
		return;

	m_Recorder.Stop();
	Con_Printf("net_record: stopped, %s\n", m_Path);
}

void CServerNetRecord::PrintStatus() const
{
	if (!m_Recorder.IsRecording())
	{
		Con_Printf("net_record: not recording\n");
		return;
	}

	Con_Printf("net_record: recording %s to %s, %llu bytes written%s\n", m_Target, m_Path,
		(unsigned long long)m_Recorder.GetBytesWritten(), m_Recorder.HasFailed() ? ", WRITE FAILED" : "");
}

void CServerNetRecord::RecordSettings()
{
	static const char *const SETTINGS[] =
	{
		"sv_maxrate", "sv_minrate", "sv_maxupdaterate", "sv_minupdaterate", "sys_ticrate", "sv_timeout",
		"sv_failuretime", "sv_rehlds_movecmdrate_max_avg", "sv_rehlds_stringcmdrate_max_avg",
	};

	char text[1024] = "";
	for (const char *name : SETTINGS)
	{
		const cvar_t *var = Cvar_FindVar(name);
		if (var)
			Q_strlcat(text, va("%s%s=%s", text[0] ? " " : "", name, var->string));
	}

	m_Recorder.RecordEvent(NetInspector::Event::Settings, nullptr, text);
}

void CServerNetRecord::RecordServerTables()
{
	char gameDirectory[MAX_PATH];
	COM_FileBase(com_gamedir, gameDirectory);

	const int instancedBaselines = g_psv.instance_baselines ? g_psv.instance_baselines->number : 0;
	m_Recorder.RecordServerInfo(g_psvs.maxclients, instancedBaselines, g_psv.name, gameDirectory);
	RecordUserMessages();
	RecordPrecache();
}

void CServerNetRecord::RecordUserMessages()
{
	std::vector<NetInspector::CRecorder::UserMessage> messages;

	UserMsg *const lists[] = { sv_gpUserMsgs, sv_gpNewUserMsgs };
	for (UserMsg *list : lists)
	{
		for (const UserMsg *message = list; message; message = message->next)
			messages.push_back({ message->iMsg, message->iSize, message->szName });
	}

	m_Recorder.RecordUserMessages(messages.data(), messages.size());
}

void CServerNetRecord::RecordDeltaDescriptions()
{
	std::vector<NetInspector::CRecorder::DeltaField> fields;

	for (const delta_info_t *info = g_sv_delta; info; info = info->next)
	{
		const delta_t *delta = info->delta;
		if (!delta)
			continue;

		fields.clear();
		for (int i = 0; i < delta->fieldCount; i++)
		{
			const delta_description_t &field = delta->pdd[i];
			fields.push_back({ field.fieldName, uint32(field.fieldType), field.significant_bits, field.premultiply, field.postmultiply });
		}

		m_Recorder.RecordDeltaDescription(info->name, fields.data(), fields.size());
	}
}

void CServerNetRecord::RecordPrecache()
{
	std::vector<NetInspector::CRecorder::PrecacheItem> items;

	for (int i = 1; i < MAX_MODELS && g_psv.model_precache[i]; i++)
		items.push_back({ i, g_psv.model_precache[i] });
	m_Recorder.RecordPrecache(NetInspector::PrecacheKind::Model, items.data(), items.size());

	items.clear();
	for (int i = 1; i < MAX_SOUNDS && g_psv.sound_precache[i]; i++)
		items.push_back({ i, g_psv.sound_precache[i] });
	m_Recorder.RecordPrecache(NetInspector::PrecacheKind::Sound, items.data(), items.size());

	items.clear();
	for (int i = 1; i < MAX_EVENTS && g_psv.event_precache[i].filename; i++)
		items.push_back({ i, g_psv.event_precache[i].filename });
	m_Recorder.RecordPrecache(NetInspector::PrecacheKind::Event, items.data(), items.size());

	items.clear();
	for (int i = 1; i < MAX_GENERIC && g_psv.generic_precache[i]; i++)
		items.push_back({ i, g_psv.generic_precache[i] });
	m_Recorder.RecordPrecache(NetInspector::PrecacheKind::Generic, items.data(), items.size());
}

bool CServerNetRecord::ParseFilter(const char *target, Filter &filter) const
{
	Q_memset(&filter, 0, sizeof(filter));

	if (!Q_stricmp(target, "all"))
	{
		filter.mode = Filter::ALL;
		return true;
	}

	// One connected client, by the address it connected from
	if (target[0] == '#')
	{
		const int userid = Q_atoi(target + 1);
		for (int i = 0; i < g_psvs.maxclients; i++)
		{
			const client_t *client = &g_psvs.clients[i];
			if (!client->connected || client->fakeclient || client->userid != userid)
				continue;

			const NetInspector::Endpoint endpoint = ToEndpoint(client->netchan.remote_address);
			filter.mode = Filter::ENDPOINT;
			filter.ip = (uint32(endpoint.ip[0]) << 24) | (uint32(endpoint.ip[1]) << 16) | (uint32(endpoint.ip[2]) << 8) | endpoint.ip[3];
			filter.mask = 0xFFFFFFFF;
			filter.port = endpoint.port;
			return true;
		}

		return false;
	}

	char address[64];
	Q_strlcpy(address, target);

	int bits = 32;
	char *slash = Q_strchr(address, '/');
	if (slash)
	{
		*slash = '\0';
		bits = Q_atoi(slash + 1);
		if (bits < 0 || bits > 32)
			return false;
	}

	netadr_t adr;
	if (!NET_StringToAdr(address, &adr) || adr.type != NA_IP)
		return false;

	filter.mode = Filter::SUBNET;
	filter.ip = (uint32(adr.ip[0]) << 24) | (uint32(adr.ip[1]) << 16) | (uint32(adr.ip[2]) << 8) | adr.ip[3];
	filter.mask = bits ? 0xFFFFFFFFu << (32 - bits) : 0;
	filter.ip &= filter.mask;
	return true;
}

bool CServerNetRecord::Matches(const NetInspector::Endpoint &endpoint) const
{
	if (m_Filter.mode == Filter::ALL)
		return true;

	const uint32 ip = (uint32(endpoint.ip[0]) << 24) | (uint32(endpoint.ip[1]) << 16) | (uint32(endpoint.ip[2]) << 8) | endpoint.ip[3];
	if ((ip & m_Filter.mask) != m_Filter.ip)
		return false;

	return m_Filter.mode != Filter::ENDPOINT || endpoint.port == m_Filter.port;
}

NetInspector::Endpoint CServerNetRecord::ToEndpoint(const struct sockaddr &address)
{
	netadr_t adr;
	SockadrToNetadr(&address, &adr);
	return ToEndpoint(adr);
}

NetInspector::Endpoint CServerNetRecord::ToEndpoint(const netadr_t &address)
{
	NetInspector::Endpoint endpoint;
	Q_memcpy(endpoint.ip, address.ip, sizeof(endpoint.ip));
	endpoint.port = ntohs(address.port);
	return endpoint;
}

void CServerNetRecord::OnDatagramReceived(netsrc_t sock, const struct sockaddr &from, const void *data, int length)
{
	if (sock != NS_SERVER || !m_Recorder.IsRecording() || length <= 0)
		return;

	const NetInspector::Endpoint endpoint = ToEndpoint(from);
	if (Matches(endpoint))
		m_Recorder.RecordDatagram(NetInspector::Direction::In, endpoint, data, length);
}

void CServerNetRecord::OnDatagramSent(netsrc_t sock, const struct sockaddr &to, const void *data, int length)
{
	if (sock != NS_SERVER || !m_Recorder.IsRecording() || length <= 0)
		return;

	const NetInspector::Endpoint endpoint = ToEndpoint(to);
	if (Matches(endpoint))
		m_Recorder.RecordDatagram(NetInspector::Direction::Out, endpoint, data, length);
}

void CServerNetRecord::OnFrame(int frame)
{
	m_Recorder.RecordFrame(frame);
}

void CServerNetRecord::OnClientConnected(const client_t *client)
{
	if (!m_Recorder.IsRecording() || client->fakeclient)
		return;

	const NetInspector::Endpoint endpoint = ToEndpoint(client->netchan.remote_address);
	if (Matches(endpoint))
		m_Recorder.RecordEvent(NetInspector::Event::Connect, &endpoint, va("userid=%d", client->userid));
}

void CServerNetRecord::OnClientDropped(const client_t *client, const char *reason)
{
	if (!m_Recorder.IsRecording() || client->fakeclient)
		return;

	const NetInspector::Endpoint endpoint = ToEndpoint(client->netchan.remote_address);
	if (Matches(endpoint))
		m_Recorder.RecordEvent(NetInspector::Event::ClientDropped, &endpoint, va("userid=%d reason=\"%s\"", client->userid, reason));
}

void CServerNetRecord::OnServerActivated()
{
	if (m_Recorder.IsRecording())
		RecordServerTables();
}

// Messages registered while the map loads reach the recording with the server's tables
void CServerNetRecord::OnUserMessagesChanged()
{
	if (m_Recorder.IsRecording() && g_psv.active)
		RecordUserMessages();
}

// The address is taken now: a client dropped by its own messages has lost its channel by the end
void CServerNetRecord::OnClientMessagesStarted(const client_t *client)
{
	m_ClientMessagesStart = 0.0;
	if (!m_Recorder.IsRecording() || client->fakeclient)
		return;

	m_ClientMessagesEndpoint = ToEndpoint(client->netchan.remote_address);
	if (Matches(m_ClientMessagesEndpoint))
		m_ClientMessagesStart = Sys_FloatTime();
}

void CServerNetRecord::OnClientMessagesFinished()
{
	if (m_ClientMessagesStart == 0.0 || !m_Recorder.IsRecording())
		return;

	const double elapsed = Sys_FloatTime() - m_ClientMessagesStart;
	m_ClientMessagesStart = 0.0;
	if (elapsed >= NET_RECORD_SLOW_CLIENT_MESSAGES)
		m_Recorder.RecordProcessTime(m_ClientMessagesEndpoint, uint32(elapsed * 1000000.0));
}

void CServerNetRecord::OnBufferOverflowed(const sizebuf_t *buf, int length)
{
	if (!m_Recorder.IsRecording())
		return;

	for (int i = 0; i < g_psvs.maxclients; i++)
	{
		const client_t *client = &g_psvs.clients[i];
		if (&client->netchan.message != buf)
			continue;

		const NetInspector::Endpoint endpoint = ToEndpoint(client->netchan.remote_address);
		if (client->fakeclient || !Matches(endpoint))
			return;

		const netchan_t &chan = client->netchan;
		NetInspector::OverflowCaptureEntry state = {};
		state.bufferBytes = buf->maxsize;
		state.requestedBytes = length;
		state.queuedBytes = buf->cursize;
		state.reliableInFlightBytes = chan.reliable_length;
		state.unacknowledgedPackets = chan.outgoing_sequence - chan.incoming_acknowledged;
		state.silentMilliseconds = uint32((realtime - chan.last_received) * 1000.0);

		for (int stream = 0; stream < MAX_STREAMS; stream++)
		{
			state.queuedFragmentBuffers += chan.fragbufcount[stream];
			for (const fragbufwaiting_t *waiting = chan.waitlist[stream]; waiting; waiting = waiting->next)
				state.queuedFragmentBuffers += waiting->fragbufcount;
		}

		m_Recorder.RecordOverflow(endpoint, state, buf->data, chan.reliable_buf);
		return;
	}
}

} // namespace

void NetRecord_Init()
{
	s_NetRecord.Init();
}

void NetRecord_Shutdown()
{
	s_NetRecord.Shutdown();
}

void NetRecord_DatagramReceived(netsrc_t sock, const struct sockaddr &from, const void *data, int length)
{
	s_NetRecord.OnDatagramReceived(sock, from, data, length);
}

void NetRecord_DatagramSent(netsrc_t sock, const struct sockaddr &to, const void *data, int length)
{
	s_NetRecord.OnDatagramSent(sock, to, data, length);
}

void NetRecord_Frame(int frame)
{
	s_NetRecord.OnFrame(frame);
}

void NetRecord_ClientConnected(const client_t *client)
{
	s_NetRecord.OnClientConnected(client);
}

void NetRecord_ClientDropped(const client_t *client, const char *reason)
{
	s_NetRecord.OnClientDropped(client, reason);
}

void NetRecord_ServerActivated()
{
	s_NetRecord.OnServerActivated();
}

void NetRecord_UserMessagesChanged()
{
	s_NetRecord.OnUserMessagesChanged();
}

void NetRecord_ClientMessagesStarted(const client_t *client)
{
	s_NetRecord.OnClientMessagesStarted(client);
}

void NetRecord_ClientMessagesFinished()
{
	s_NetRecord.OnClientMessagesFinished();
}

void NetRecord_BufferOverflowed(const sizebuf_t *buf, int length)
{
	s_NetRecord.OnBufferOverflowed(buf, length);
}
