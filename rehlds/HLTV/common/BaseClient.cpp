/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#include "precompiled.h"

bool BaseClient::Init(IBaseSystem *system, int serial, char *name)
{
	if (!BaseSystemModule::Init(system, serial, name)) {
		return false;
	}

	if (!name) {
		Q_strlcpy(m_Name, CLIENT_INTERFACE_VERSION);
	}

	SetState(CLIENT_INITIALIZING);

	m_ClientType = TYPE_CLIENT;
	m_World = nullptr;
	m_Socket = nullptr;

	m_VoiceEnabled = false;
	m_VoiceQuery = false;
	m_Userinfo.SetMaxSize(MAX_USER_INFO);

	return false;
}

void BaseClient::RunFrame(double time)
{
	BaseSystemModule::RunFrame(time);

	if (m_ClientState != CLIENT_DISCONNECTED && m_ClientChannel.IsTimedOut()) {
			m_System->Printf("Client %s timed out.\n", m_ClientChannel.GetTargetAddress()->ToString());
			Disconnect("Timed out.\n");
	}

	if (m_ClientState != CLIENT_DISCONNECTED && m_ClientChannel.IsCrashed()) {
		m_System->Printf("Client %s netchannel crashed.\n", m_ClientChannel.GetTargetAddress()->ToString());
		Disconnect("Netchannel crashed.\n");
	}

	int numPackets = 0;
	NetPacket *packet;
	while ((packet = m_ClientChannel.GetPacket()))
	{
		if (packet->connectionless)
		{
			m_Socket->AddPacket(packet);
		}
		else if (numPackets < 32)
		{
			numPackets++;
			ProcessMessage(packet);
		}

		m_ClientChannel.FreePacket(packet);
	}

	// process state
	switch (m_ClientState)
	{
	case CLIENT_RUNNING:
	{
		if (m_World->IsActive() && m_ClientChannel.IsReadyToSend()) {
			SendDatagram();
		}
		break;
	}
	case CLIENT_DISCONNECTED:
		ShutDown();
		break;
	case CLIENT_INITIALIZING:
	case CLIENT_CONNECTING:
		/* do nothing */
		break;
	default:
	case CLIENT_UNDEFINED:
		m_System->Errorf("Client::RunFrame: not valid state.\n");
		break;
	}
}

void BaseClient::ShutDown()
{
	if (m_State == MODULE_DISCONNECTED) {
		return;
	}

	if (m_ClientState != CLIENT_DISCONNECTED) {
		Disconnect("Connection dropped (hard).\n");
	}

	m_ClientChannel.Close();
	BaseSystemModule::ShutDown();
}

bool BaseClient::Connect(INetSocket *socket, NetAddress *adr, char *userinfo)
{
	if (!userinfo || !adr) {
		return false;
	}

	m_ClientType = TYPE_CLIENT;
	m_Socket = socket;

	if (!adr->Equal(m_ClientChannel.GetTargetAddress())) {
		m_ClientChannel.Create(m_System, m_Socket, adr);
	} else {
		m_ClientChannel.Clear();
		m_ClientChannel.Reset();
	}

	SetState(CLIENT_CONNECTING);
	m_ClientChannel.SetUpdateRate(20);
	m_ClientChannel.SetRate(20000);
	m_ClientChannel.SetConnected(1);

	UpdateUserInfo(userinfo);
	m_ClientChannel.OutOfBandPrintf("%c0000000000000000", S2C_CONNECTION);

	return true;
}

BaseClient::clc_func_s BaseClient::m_ClientFuncs[] = {
	{ clc_bad,             "clc_bad",             &BaseClient::ParseBad },
	{ clc_nop,             "clc_nop",             &BaseClient::ParseNop },
	{ clc_move,            "clc_move",            &BaseClient::ParseMove },
	{ clc_stringcmd,       "clc_stringcmd",       &BaseClient::ParseStringCmd },
	{ clc_delta,           "clc_delta",           &BaseClient::ParseDelta },
	{ clc_resourcelist,    "clc_resourcelist",    nullptr },
	{ clc_tmove,           "clc_tmove",           nullptr },
	{ clc_fileconsistency, "clc_fileconsistency", nullptr },
	{ clc_voicedata,       "clc_voicedata",       &BaseClient::ParseVoiceData },
	{ clc_hltv,            "clc_hltv",            &BaseClient::ParseHLTV },
	{ clc_cvarvalue,       "clc_cvarvalue",       &BaseClient::ParseCvarValue },
	{ clc_cvarvalue2,      "clc_cvarvalue2",      &BaseClient::ParseCvarValue2 },
	{ clc_endoflist,       "End of List",         nullptr },
};

void BaseClient::ProcessMessage(NetPacket *packet)
{
	m_ClientDelta = 0;
	m_DeltaFrameSeqNr = 0;

	while (true)
	{
		if (packet->data.IsOverflowed()) {
			m_System->Printf("Client::ProcessMessage: packet read overflow\n");
			Disconnect("Dropped due to bad message format!\n");
			break;
		}

		int cmd = packet->data.ReadByte();
		if (cmd == -1)
			break;

		if (cmd < clc_bad || cmd > clc_cvarvalue2)
		{
			m_System->Printf("Client::ProcessMessage: unknown command char\n");
			Disconnect("Bad command character in client command");
			return;
		}

		if (!m_ClientFuncs[cmd].func) {
			m_System->Printf("TODO! Client::ProcessMessage: missing parsing routine for %s.\n", m_ClientFuncs[cmd].pszname);
			return;
		}

		(this->*m_ClientFuncs[cmd].func)(packet);
	}
}

char *BaseClient::GetClientName()
{
	return m_ClientName;
}

InfoString *BaseClient::GetUserInfo()
{
	return &m_Userinfo;
}

void BaseClient::Disconnect(const char *reason)
{
	SetState(CLIENT_DISCONNECTED);

	m_ClientChannel.m_reliableStream.WriteByte(svc_disconnect);
	m_ClientChannel.m_reliableStream.WriteString(reason ? reason : "");

	m_ClientChannel.m_unreliableStream.WriteByte(svc_disconnect);
	m_ClientChannel.m_unreliableStream.WriteString(reason ? reason : "");
	m_ClientChannel.TransmitOutgoing();
}

void BaseClient::ParseNop(NetPacket *packet)
{
	;
}

void BaseClient::ParseBad(NetPacket *packet)
{
	m_System->Printf("Client::ParseBad: bad command char\n");
	Disconnect("Bad command character in client command");
}

void BaseClient::ParseStringCmd(NetPacket *packet)
{
	char *string = packet->data.ReadString();
	ProcessStringCmd(string);
}

BaseClient::LocalCommandID_s BaseClient::m_LocalCmdReg[] = {
	{ "name",       CMD_ID_NAME,       &BaseClient::CMD_Name },
	{ "spawn",      CMD_ID_SPAWN,      &BaseClient::CMD_Spawn },
	{ "new",        CMD_ID_NEW,        &BaseClient::CMD_New },
	{ "fullupdate", CMD_ID_FULLUPDATE, &BaseClient::CMD_FullUpdate },
	{ "dropclient", CMD_ID_DROPCLIENT, &BaseClient::CMD_DropClient },
	{ "dlfile",     CMD_ID_DLFILE,     &BaseClient::CMD_DownloadFile },
	{ "setinfo",    CMD_ID_SETINFO,    &BaseClient::CMD_SetInfo },
	{ "showinfo",   CMD_ID_SHOWINFO,   &BaseClient::CMD_ShowInfo },
	{ "sendents",   CMD_ID_SENDENTS,   &BaseClient::CMD_SendEntities },
	{ "VModEnable", CMD_ID_VMODENABLE, &BaseClient::CMD_VoiceModEnable },
	{ "vban",       CMD_ID_VBAN,       &BaseClient::CMD_VoiceBan },

	// TODO: Pure function's, finish them.
	{ "sendres",    CMD_ID_SENDERS,    &BaseClient::CMD_SendResources },
	{ "ping",       CMD_ID_PING,       &BaseClient::CMD_RequestPing },
	{ "spectate",   CMD_ID_SPECTATE,   &BaseClient::CMD_Spectate },
	{ "spk",        CMD_ID_SPK,        &BaseClient::CMD_Spk },
	{ "pause",      CMD_ID_PAUSE,      &BaseClient::CMD_Pause },
	{ "unpause",    CMD_ID_UNPAUSE,    &BaseClient::CMD_UnPause },
	{ "setpause",   CMD_ID_SETPAUSE,   &BaseClient::CMD_SetPause }
};

bool BaseClient::ProcessStringCmd(char *string)
{
	TokenLine cmdLine;
	if (!cmdLine.SetLine(string)) {
		m_System->Printf("WARNING! BaseClient::ProcessStringCmd: string command too long.\n");
		return false;
	}

	if (!cmdLine.CountToken()) {
		m_System->DPrintf("WARNING! BaseClient::ProcessStringCmd: invalid command string.\n");
		return false;
	}

	char *cmd = cmdLine.GetToken(0);
	for (auto& local_cmd : m_LocalCmdReg)
	{
		if (!Q_stricmp(local_cmd.name, cmd)) {
			(this->*local_cmd.pfnCmd)(&cmdLine);
			return true;
		}
	}

	return false;
}

void BaseClient::CMD_Name(TokenLine *cmd)
{
	SetName(cmd->GetRestOfLine(1));
}

void BaseClient::CMD_Spawn(TokenLine *cmd)
{
	if (cmd->CountToken() != 3) {
		m_System->Printf("Client::ParseStringCmd: spawn is not valid\n");
		Disconnect("Spawn is not valid.");
		return;
	}

	ReplySpawn(Q_atoi(cmd->GetToken(1)), Q_atoi(cmd->GetToken(2)));
}

void BaseClient::CMD_New(TokenLine *cmd)
{
	ReplyNew();
}

void BaseClient::CMD_FullUpdate(TokenLine *cmd)
{
	ReplyFullUpdate();
}

void BaseClient::CMD_DropClient(TokenLine *cmd)
{
	Disconnect();
}

void BaseClient::CMD_DownloadFile(TokenLine *cmd)
{
	// GetRestOfLine(1) starts at token 1, not after it
	char *rest = cmd->GetRestOfLine(1);
	if (!rest)
		return;

	char fileName[MAX_PATH];

	size_t len = 0;
	while (rest[len])
	{
		if (++len >= sizeof(fileName))
			return;	// no legitimate resource path is this long
	}

	// The full line keeps a quoted token's closing quote
	while (len > 0 && ((unsigned char)rest[len - 1] <= ' ' || rest[len - 1] == '\"'))
		len--;

	if (len == 0)
		return;

	Q_memcpy(fileName, rest, len);
	fileName[len] = '\0';

	DownloadFile(fileName);
}

void BaseClient::CMD_SetInfo(TokenLine *cmd)
{
	if (cmd->CountToken() != 3) {
		return;
	}

	if (cmd->GetToken(1)[0] ==  '*') {
		return;
	}

	m_Userinfo.SetValueForKey(cmd->GetToken(1), cmd->GetToken(2));
	UpdateUserInfo();
}

void BaseClient::CMD_ShowInfo(TokenLine *cmd)
{
	InfoString info(MAX_INFO_STRING);

	info.SetString(m_World->GetServerInfoString()->GetString());
	info.RemoveKey("proxy"); // remove proxy ip
	PrintfToClient("%s\n", info.GetString());
}

void BaseClient::CMD_SendEntities(TokenLine *cmd)
{
	m_System->DPrintf("Client fully connected.\n");
	SetState(CLIENT_RUNNING);
}

void BaseClient::CMD_VoiceModEnable(TokenLine *cmd)
{
	if (cmd->CountToken() != 2) {
		return;
	}

	m_VoiceQuery = false;
	m_VoiceEnabled = Q_atoi(cmd->GetToken(1)) ? true : false;
	UpdateVoiceMask(&m_ClientChannel.m_reliableStream);
}

void BaseClient::CMD_VoiceBan(TokenLine *cmd)
{
	if (cmd->CountToken() < 2) {
		return;
	}

	for (int i = 1; i < cmd->CountToken(); i++)
	{
		uint32 mask = 0;
		sscanf(cmd->GetToken(i), "%x", &mask);

		if (i <= VOICE_MAX_PLAYERS_DW) {
			m_SentBanMask.SetDWord(i - 1, mask);
		}
	}

	UpdateVoiceMask(&m_ClientChannel.m_reliableStream);
}

void BaseClient::CMD_SendResources(TokenLine *cmd) { /* do nothing  */ }
void BaseClient::CMD_RequestPing(TokenLine *cmd)   { /* do nothing  */ }
void BaseClient::CMD_Spectate(TokenLine *cmd)      { /* do nothing  */ }
void BaseClient::CMD_Spk(TokenLine *cmd)           { /* do nothing  */ }
void BaseClient::CMD_Pause(TokenLine *cmd)         { /* do nothing  */ }
void BaseClient::CMD_UnPause(TokenLine *cmd)       { /* do nothing  */ }
void BaseClient::CMD_SetPause(TokenLine *cmd)      { /* do nothing  */ }

void BaseClient::UpdateUserInfo(char *userinfostring)
{
	char buffer[1024];
	char *string;

	if (userinfostring) {
		m_Userinfo.SetString(userinfostring);
	}

	Q_strlcpy(buffer, m_Userinfo.ValueForKey("name"));

	SetName(buffer);
	m_ClientType = Q_atoi(m_Userinfo.ValueForKey("*hltv"));

	if (m_ClientType < TYPE_CLIENT) {
		m_System->DPrintf("WARNING! BaseClient::UpdateUserInfo: invalid client ype %i\n", m_ClientType);
		m_ClientType = TYPE_CLIENT;
	}

	string = m_Userinfo.ValueForKey("rate");
	if (*string) {
		m_ClientChannel.SetRate(Q_atoi(string));
	}

	string = m_Userinfo.ValueForKey("cl_updaterate");
	if (*string) {
		m_ClientChannel.SetUpdateRate(Q_atoi(string));
	}
}

void BaseClient::PrintfToClient(char *fmt, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	m_ClientChannel.m_reliableStream.WriteByte(svc_print);
	m_ClientChannel.m_reliableStream.WriteString(string);
}

void BaseClient::ReplySpawn(int spawncount, int crcMap)
{
	BitBuffer msg(MAX_POSSIBLE_MSG);

	m_CRC_Value = crcMap;
	COM_UnMunge2((unsigned char *)&m_CRC_Value, 4, (-1 - m_World->GetServerCount()) & 0xFF);

	if (m_World->GetServerCount() != spawncount) {
		ReplyNew();
		return;
	}

	m_World->WriteSigonData(&msg);
	WriteSpawn(&msg);

	m_ClientChannel.CreateFragmentsFromBuffer(msg.GetData(), msg.CurrentSize(), FRAG_NORMAL_STREAM, nullptr);
	m_ClientChannel.FragSend();
	m_LastFrameSeqNr = 0;
}

void BaseClient::ReplyNew()
{
	BitBuffer msg(MAX_POSSIBLE_MSG);

	Reset();
	m_World->WriteNewData(&msg);

	m_ClientChannel.CreateFragmentsFromBuffer(msg.GetData(), msg.CurrentSize(), FRAG_NORMAL_STREAM, nullptr);
	m_ClientChannel.FragSend();
	msg.Free();
}

void BaseClient::ReplyFullUpdate()
{
	BitBuffer msg(1024 * 9);
	for (int i = 0; i < m_World->GetMaxClients(); i++) {
		m_World->WriteClientUpdate(&msg, i);
	}

	m_ClientChannel.CreateFragmentsFromBuffer(msg.GetData(), msg.CurrentSize(), FRAG_NORMAL_STREAM, nullptr);
	m_ClientChannel.FragSend();
}

void BaseClient::SetState(ClientState newState)
{
	if (newState == m_ClientState)
		return;

	bool stateError = false;
	switch (newState)
	{
	case CLIENT_INITIALIZING:
		break;
	case CLIENT_CONNECTING:
	{
		if (m_ClientState != CLIENT_INITIALIZING
			&& m_ClientState != CLIENT_RUNNING) {
			stateError = true;
		}

		m_ClientChannel.SetKeepAlive(true);
		m_ClientChannel.SetTimeOut(60);
		break;
	}
	case CLIENT_RUNNING:
	{
		if (m_ClientState != CLIENT_CONNECTING) {
			stateError = true;
		}

		m_ClientChannel.SetKeepAlive(false);
		m_ClientChannel.SetTimeOut(30);
		break;
	}
	case CLIENT_DISCONNECTED:
	{
		m_ClientChannel.SetKeepAlive(true);
		break;
	}
	default:
		stateError = true;
		break;
	}

	if (stateError)
	{
		m_System->Errorf("Client::SetState: not valid m_ClientState (%i -> %i).\n", m_ClientState, newState);
		return;
	}

	m_ClientState = newState;
}

void BaseClient::WriteSpawn(BitBuffer *stream)
{
	stream->WriteByte(svc_time);
	stream->WriteFloat(1);

	for (int i = 0; i < m_World->GetMaxClients(); i++) {
		m_World->WriteClientUpdate(stream, i);
	}

	m_World->WriteLightStyles(stream);

	stream->WriteByte(svc_signonnum);
	stream->WriteByte(1);
}

void BaseClient::WriteDatagram(double time, frame_t *frame)
{
	if (!frame) {
		return;
	}

	if (!m_LastFrameSeqNr || m_LastFrameSeqNr > frame->seqnr) {
		m_LastFrameSeqNr = frame->seqnr - 1;
		m_ClientDelta = 0;
		m_DeltaFrameSeqNr = 0;
	}

	if (m_LastFrameSeqNr >= frame->seqnr && m_ClientChannel.GetIdleTime() <= 2) {
		return;
	}

	m_ClientChannel.m_unreliableStream.WriteByte(svc_time);
	m_ClientChannel.m_unreliableStream.WriteFloat(float(time));
	m_World->WriteFrame(frame, m_LastFrameSeqNr, &m_ClientChannel.m_reliableStream, &m_ClientChannel.m_unreliableStream, m_DeltaFrameSeqNr, m_ClientDelta, IsHearingVoices());

	if (m_VoiceQuery) {
		QueryVoiceEnabled(&m_ClientChannel.m_unreliableStream);
	}

	if (m_ClientChannel.m_reliableStream.IsOverflowed()) {
		Disconnect("Reliable data stream overflow.\n");
		return;
	}

	if (m_ClientChannel.m_unreliableStream.IsOverflowed()) {
		m_System->DPrintf("Unreliable data stream overflow.\n");
		m_ClientChannel.m_unreliableStream.Clear();

		// FIXME: V519 The 'm_LastFrameSeqNr' variable is assigned values twice successively.
		// m_LastFrameSeqNr = 0;
	}

	m_LastFrameSeqNr = frame->seqnr;
	m_SeqNrMap[m_ClientChannel.m_outgoing_sequence & 0xFF] = frame->seqnr;
	m_ClientChannel.TransmitOutgoing();
}

void BaseClient::ParseDelta(NetPacket *packet)
{
	if (m_ClientState != CLIENT_RUNNING) {
		packet->data.SkipBytes(1);
		return;
	}

	m_ClientDelta = packet->data.ReadByte();
	m_DeltaFrameSeqNr = m_SeqNrMap[m_ClientDelta];
}

char *BaseClient::GetStatusLine()
{
	float in, out;
	static char string[256];

	m_ClientChannel.GetFlowStats(&in, &out);
	Q_snprintf(string, sizeof(string),
		"ID: %i, Name \"%s\", Time %s, IP %s, In %.2f, Out %.2f.\n",
		GetSerial(),
		m_ClientName,
		COM_FormatTime(float(m_SystemTime - m_ClientChannel.m_connect_time)),
		m_ClientChannel.GetTargetAddress()->ToString(),
		in,
		out
	);

	return string;
}

void BaseClient::SetWorld(IWorld *world)
{
	if (m_World && m_World != world) {
		Reconnect();
	}

	m_World = world;
}

void BaseClient::Reconnect()
{
	Reset();
	m_ClientChannel.m_reliableStream.WriteByte(svc_stufftext);
	m_ClientChannel.m_reliableStream.WriteString("reconnect\n");
}

bool BaseClient::IsActive()
{
	return (m_ClientState == CLIENT_RUNNING);
}

void BaseClient::ParseVoiceData(NetPacket *packet)
{
	int nDataLength = packet->data.ReadShort();
	packet->data.SkipBytes(nDataLength);
}

void BaseClient::ParseMove(NetPacket *packet)
{
	int nDataLength = packet->data.ReadByte();
	packet->data.SkipBytes(nDataLength + 1);
}

void BaseClient::SetName(char *newName)
{
	char temp[1024];
	COM_RemoveEvilChars(newName);
	COM_TrimSpace(newName, temp);

	const int len = sizeof(m_ClientName);
	if (Q_strlen(temp) >= len) {
		temp[len] = '\0';
	}

	if (!temp[0] || !Q_stricmp(temp, "console")) {
		Q_strcpy(temp, "unnamed");
	}

	Q_strlcpy(m_ClientName, temp);
	m_Userinfo.SetValueForKey("name", m_ClientName);
}

void BaseClient::ParseHLTV(NetPacket *packet)
{
	Disconnect("TODO Client::ParseHLTV: Invalid client command.\n");
}

void BaseClient::ParseCvarValue(NetPacket *packet)
{
	packet->data.ReadString();
}

void BaseClient::ParseCvarValue2(NetPacket *packet)
{
	packet->data.ReadWord();
	packet->data.ReadString();
	packet->data.ReadString();
}

void BaseClient::QueryVoiceEnabled(BitBuffer *stream)
{
	int reqState = m_World->FindUserMsgByName("ReqState");
	if (!reqState || !m_World->IsVoiceEnabled())
		return;

	stream->WriteByte(reqState);
}

void BaseClient::UpdateVoiceMask(BitBuffer *stream)
{
	int voiceMask = m_World->FindUserMsgByName("VoiceMask");
	if (!voiceMask) {
		m_System->DPrintf("WARNING! Client::UpdateVoiceMask(): could find User Msg VoiceMaks.\n");
		return;
	}

	if (!m_World->IsVoiceEnabled()) {
		return;
	}

	CPlayerBitVec gameRulesMask;
	gameRulesMask[m_World->GetSlotNumber()] = !!m_VoiceEnabled;

	if (gameRulesMask != m_SentGameRulesMask || m_BanMask != m_SentBanMask)
	{
		m_SentGameRulesMask = gameRulesMask;
		m_SentBanMask = m_BanMask;

		stream->WriteByte(voiceMask);
		for (int dw = 0; dw < VOICE_MAX_PLAYERS_DW; dw++)
		{
			stream->WriteLong(gameRulesMask.GetDWord(dw));
			stream->WriteLong(m_BanMask.GetDWord(dw));
		}
	}
}

bool BaseClient::IsHearingVoices()
{
	return m_BanMask[m_World->GetSlotNumber()] ? true : false;
}

bool BaseClient::HasChatEnabled()
{
	return false;
}

void BaseClient::DownloadFile(char *fileName)
{
	if (!fileName || !fileName[0])
		return;

	DownloadFailed(fileName);
}

void BaseClient::DownloadFailed(char *fileName)
{
	if (!fileName || !fileName[0])
		return;

	m_ClientChannel.m_reliableStream.WriteByte(svc_filetxferfailed);
	m_ClientChannel.m_reliableStream.WriteString(fileName);
}

void BaseClient::Send(unsigned char *data, int length, bool isReliable)
{
	if (isReliable)
		m_ClientChannel.m_reliableStream.WriteBuf(data, length);
	else
		m_ClientChannel.m_unreliableStream.WriteBuf(data, length);
}

NetAddress *BaseClient::GetAddress()
{
	return m_ClientChannel.GetTargetAddress();
}

int BaseClient::GetClientType()
{
	return m_ClientType;
}

char *BaseClient::GetType()
{
	return CLIENT_INTERFACE_VERSION;
}

void BaseClient::SendDatagram()
{
	frame_t *frame = m_World->GetLastFrame();
	if (frame) {
		WriteDatagram(frame->time, frame);
	}
}

void BaseClient::Reset()
{
	SetState(CLIENT_CONNECTING);

	m_LastFrameSeqNr = 0;
	m_DeltaFrameSeqNr = 0;
	m_ClientDelta = 0;

	Q_memset(m_SeqNrMap, 0, sizeof(m_SeqNrMap));
	m_VoiceQuery = true;
	m_ClientChannel.Clear();
}
