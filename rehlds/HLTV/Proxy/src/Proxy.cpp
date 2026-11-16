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

Proxy::LocalCommandID_s Proxy::m_LocalCmdReg[] = {
	{ "rcon",              CMD_ID_RCON,               &Proxy::CMD_Rcon },
	{ "rconpassword",      CMD_ID_RCONPASSWORD,       &Proxy::CMD_RconPassword },
	{ "rconaddress",       CMD_ID_RCONADDRESS,        &Proxy::CMD_RconAddress },
	{ "say",               CMD_ID_SAY,                &Proxy::CMD_Say },
	{ "msg",               CMD_ID_MSG,                &Proxy::CMD_Msg },
	{ "clients",           CMD_ID_CLIENTS,            &Proxy::CMD_Clients },
	{ "kick",              CMD_ID_KICK,               &Proxy::CMD_Kick },
	{ "chatmode",          CMD_ID_CHATMODE,           &Proxy::CMD_ChatMode },
	{ "publicgame",        CMD_ID_PUBLICGAME,         &Proxy::CMD_PublicGame },
	{ "offlinetext",       CMD_ID_OFFLINETEXT,        &Proxy::CMD_OffLineText },
	{ "adminpassword",     CMD_ID_ADMINPASSWORD,      &Proxy::CMD_AdminPassword },
	{ "signoncommands",    CMD_ID_SIGNONCOMMANDS,     &Proxy::CMD_SignOnCommands },
	{ "spectatorpassword", CMD_ID_SPECTATORPASSWORD,  &Proxy::CMD_SpectatorPassword },
	{ "dispatchmode",      CMD_ID_DISPATCHMODE,       &Proxy::CMD_DispatchMode },
	{ "cheeringthreshold", CMD_ID_CHEERINGTHRESHOLD,  &Proxy::CMD_CheeringThreshold },
//	{ "informplayers",     CMD_ID_INFORMPLAYERS,      nullptr },
	{ "ping",              CMD_ID_PING,               &Proxy::CMD_Ping },
	{ "proxypassword",     CMD_ID_PROXYPASSWORD,      &Proxy::CMD_ProxyPassword },
	{ "maxrate",           CMD_ID_MAXRATE,            &Proxy::CMD_MaxRate },
	{ "loopcmd",           CMD_ID_LOOPCMD,            &Proxy::CMD_LoopCmd },
	{ "maxclients",        CMD_ID_MAXCLIENTS,         &Proxy::CMD_MaxClients },
	{ "disconnect",        CMD_ID_DISCONNECT,         &Proxy::CMD_Disconnect },
	{ "localmsg",          CMD_ID_LOCALMSG,           &Proxy::CMD_LocalMsg },
	{ "connect",           CMD_ID_CONNECT,            &Proxy::CMD_Connect },
	{ "playdemo",          CMD_ID_PLAYDEMO,           &Proxy::CMD_PlayDemo },
	{ "delay",             CMD_ID_DELAY,              &Proxy::CMD_Delay },
	{ "stop",              CMD_ID_STOP,               &Proxy::CMD_Stop },
	{ "record",            CMD_ID_RECORD,             &Proxy::CMD_Record },
	{ "stoprecording",     CMD_ID_STOPRECORDING,      &Proxy::CMD_StopRecording },
	{ "servercmd",         CMD_ID_SERVERCMD,          &Proxy::CMD_ServerCmd },
	{ "clientcmd",         CMD_ID_CLIENTCMD,          &Proxy::CMD_ClientCmd },
	{ "blockvoice",        CMD_ID_BLOCKVOICE,         &Proxy::CMD_BlockVoice },
	{ "name",              CMD_ID_NAME,               &Proxy::CMD_Name },
	{ "updaterate",        CMD_ID_UPDATERATE,         &Proxy::CMD_Updaterate },
	{ "rate",              CMD_ID_RATE,               &Proxy::CMD_Rate },
	{ "addresource",       CMD_ID_ADDRESOURCE,        &Proxy::CMD_AddResource },
	{ "resources",         CMD_ID_RESOURCES,          &Proxy::CMD_Resources },
	{ "bannerfile",        CMD_ID_BANNERFILE,         &Proxy::CMD_BannerFile },
	{ "bann",              CMD_ID_BANN,               &Proxy::CMD_Bann },
	{ "addfakeclients",    CMD_ID_ADDFAKECLIENTS,     &Proxy::CMD_AddFakeClients },
	{ "retry",             CMD_ID_RETRY,              &Proxy::CMD_Retry },
	{ "players",           CMD_ID_PLAYERS,            &Proxy::CMD_Players },
	{ "autoretry",         CMD_ID_AUTORETRY,          &Proxy::CMD_AutoRetry },
	{ "serverpassword",    CMD_ID_SERVERPASSWORD,     &Proxy::CMD_ServerPassword },
	{ "status",            CMD_ID_STATUS,             &Proxy::CMD_Status },
	{ "hostname",          CMD_ID_HOSTNAME,           &Proxy::CMD_HostName },
	{ "maxqueries",        CMD_ID_MAXQUERIES,         &Proxy::CMD_MaxQueries },
	{ "clearbanns",        CMD_ID_CLEARBANNS,         &Proxy::CMD_ClearBanns },
	{ "maxloss",           CMD_ID_MAXLOSS,            &Proxy::CMD_MaxLoss },
	{ "protocol",          CMD_ID_PROTOCOL,           &Proxy::CMD_Protocol },
	{ "region",            CMD_ID_REGION,             &Proxy::CMD_Region },
	{ "chatdelay",         CMD_ID_CHATDELAY,          &Proxy::CMD_ChatDelay },
};

EXPOSE_SINGLE_INTERFACE(Proxy, IProxy, PROXY_INTERFACE_VERSION);

bool Proxy::Init(IBaseSystem *system, int serial, char *name)
{
	BaseSystemModule::Init(system, serial, name);

	if (!name) {
		SetName(PROXY_INTERFACE_VERSION);
	}

	m_MaxRate = 20000;
	m_MaxUpdateRate = 20;
	m_IsMaster = false;

	for (auto& cmd : m_LocalCmdReg) {
		m_System->RegisterCommand(cmd.name, this, cmd.id);
	}

	m_Network = (INetwork *)m_System->GetModule(NETWORK_INTERFACE_VERSION, "core");
	if (!m_Network)
	{
		m_System->Errorf("Proxy::Init: couldn't load network module.\n");
		return false;
	}

	char *portparam = m_System->CheckParam("-port");
	int proxyport = Q_atoi(portparam ? portparam : PROXY_DEFAULT_PORT);

	if (!(m_Socket = (INetSocket *)m_Network->CreateSocket(proxyport)))
	{
		m_System->Errorf("Proxy::Init: Could not create proxy port %i.\n", proxyport);
		return false;
	}

	m_Status.SetProxy(this);
	if (!m_System->AddModule(&m_Status, "status"))
	{
		m_System->Errorf("Proxy::Init: add status module.\n");
		return false;
	}

	m_Master.SetProxy(this);
	if (!m_System->AddModule(&m_Master, "master"))
	{
		m_System->Errorf("Proxy::Init: add master module.\n");
		return false;
	}

	m_Server = (IServer *)m_System->GetModule(SERVER_INTERFACE_VERSION, "core");
	if (!m_Server)
	{
		m_System->Errorf("Proxy::Init: couldn't load server module.\n");
		return false;
	}

	m_Server->RegisterListener(this);
	m_Server->SetProxy(this);
	m_Server->SetUserInfo("hspecs", "0");
	m_Server->SetUserInfo("hslots", "0");
	m_Server->SetGameDirectory("valve");

	m_World = (IWorld *)m_System->GetModule(WORLD_INTERFACE_VERSION, "core");
	if (!m_World)
	{
		m_System->Errorf("Proxy::Init: couldn't load world module.\n");
		return false;
	}

	m_World->RegisterListener(this);

	if (!m_System->AddModule(&m_DemoClient, "demo")) {
		m_System->Printf("Proxy::Init: Couldn't create demo client.\n");
	}

	m_DemoClient.SetProxy(this);
	m_DemoClient.SetWorld(m_World);

	SetDelay(30);
	SetClientTimeScale(1);

	m_Director = nullptr;
	m_DispatchMode = DISPATCH_BALANCE;
	m_ChatMode = CHAT_OFF;

	m_LastClockUpdateTime = 0;
	m_NextStatusUpdateTime = 0;

	m_CheeringPlayers = 0;
	m_BannerTGA = nullptr;

	m_PublicGame = true;
	m_IsReconnectRequested = false;
	m_IsFinishingBroadcast = false;

	m_LastCheeringUpdate = 0;
	m_CheeringThreshold = 0.25f;
	m_MaxLoss = 0.05f;
	m_CurrentLoss = 0;
	m_FPS = 1;

	m_MaxSeenClients = 0;
	m_MaxClients = 128;
	m_MaxQueries = 100;
	m_Region = 255;
	m_ChatDelay = 6;

	const int maxRouteAblePacketSize = 1400;
	m_InfoInfo.Resize(maxRouteAblePacketSize);
	m_InfoRules.Resize(maxRouteAblePacketSize);
	m_InfoPlayers.Resize(maxRouteAblePacketSize);
	m_InfoDetails.Resize(maxRouteAblePacketSize);

	m_InfoString.Resize(2080);
	m_NextInfoMessagesUpdate = 0;

	m_RconAddress.Clear();

	// Clear buffers
	Q_memset(m_RconPassword, 0, sizeof(m_RconPassword));
	Q_memset(m_AdminPassword, 0, sizeof(m_AdminPassword));
	Q_memset(m_ProxyPassword, 0, sizeof(m_ProxyPassword));
	Q_memset(m_SpectatorPassword, 0, sizeof(m_SpectatorPassword));
	Q_memset(m_LastRconCommand, 0, sizeof(m_LastRconCommand));
	Q_memset(m_OffLineText, 0, sizeof(m_OffLineText));
	Q_memset(m_SignonCommands, 0, sizeof(m_SignonCommands));

	for (int i = 0; i < MAX_CHALLENGES; i++)
	{
		m_Challenges[i].adr.Clear();
		m_Challenges[i].challenge = 0;
		m_Challenges[i].time = 0.0f;
	}

	m_LoopCommands.Init();
	m_BannList.Init();
	m_Resources.Init();

	Q_memset(&m_LocalMessage, 0, sizeof(m_LocalMessage));
	m_LocalMessage = {
		0,						// effect
		255, 160, 0, 255,		// r1, g1, b1, a1
		255, 255, 255, 255,		// r2, g2, b2, a2
		-1.f, -1.f,				// x, y
		0.5f, 2.f,				// fadein, fadeout
		5.f, 0.f,				// holdtime, fxtime
		{}						// text
	};

	Q_memset(&m_CommentatorMessage, 0, sizeof(m_CommentatorMessage));
	m_CommentatorMessage = {
		0,						// effect
		255, 160, 0, 255,		// r1, g1, b1, a1
		255, 160, 0, 255,		// r2, g2, b2, a2
		-1.f, -1.f,				// x, y
		0.3f, 1.f,				// fadein, fadeout
		5.f, 0.f,				// holdtime, fxtime
		{}						// text
	};

	Q_strlcpy(m_OffLineText, "Game is delayed. Please try again later.");

	m_System->SetTitle("HLTV - offline");
	m_System->ExecuteFile("hltv.cfg");
	m_System->Printf("Proxy module initialized.\n");
	m_State = MODULE_RUNNING;

	return true;
}

void Proxy::ExecuteCommand(int commandID, char *commandLine)
{
	for (auto& cmd : m_LocalCmdReg)
	{
		if (cmd.pfnCmd && cmd.id == commandID) {
			(this->*cmd.pfnCmd)(commandLine);
			return;
		}
	}

	m_System->Printf("ERROR! Proxy::ExecuteCommand: unknown command ID %i.\n", commandID);
}

void Proxy::RunFrame(double time)
{
	float frameTime = float(time - m_SystemTime);
	BaseSystemModule::RunFrame(time);

	if (m_MaxQueries > 0)
	{
		m_MaxFrameQueries = int(m_MaxQueries * frameTime);
		if (m_MaxFrameQueries <= 0) {
			m_MaxFrameQueries = 1;
		}
	}
	else
		m_MaxFrameQueries = 0;

	if (frameTime > 0) {
		m_FPS = 0.99f * m_FPS + 0.01f / frameTime;
	}

	if (m_SystemTime > m_NextStatusUpdateTime) {
		UpdateStatusLine();
	}

	if (m_SystemTime > m_NextInfoMessagesUpdate) {
		UpdateInfoMessages();
	}

	int numPackets = 0;
	NetPacket *packet;
	while (numPackets < 32 && (packet = m_Socket->ReceivePacket()))
	{
		if (packet->connectionless && !packet->data.IsOverflowed())
		{
			if (ProcessConnectionlessMessage(&packet->address, &packet->data)) {
				numPackets++;
			}
		}
		else
		{
			m_System->DPrintf("WARNING! Packet from %s with invalid sequence number.\n", packet->address.ToString());
		}

		m_Socket->FreePacket(packet);
	}

	ExecuteLoopCommands();

	if (m_World->IsActive())
	{
		if (GetDelay() > 0)
		{
			RunClocks();
			if (m_IsFinishingBroadcast && m_ClientWorldTime > m_World->GetTime() && !m_IsReconnectRequested)
			{
				if (m_Server->IsConnected()) {
					m_Server->Reconnect();
				}
				m_IsReconnectRequested = true;
			}
		}

		float lastTime = m_LastCheeringUpdate + 8;
		if (lastTime < m_SystemTime)
		{
			m_CheeringPlayers /= 2;
			m_LastCheeringUpdate = float(m_SystemTime);
		}
	}
}

void Proxy::ShutDown()
{
	if (m_State == MODULE_DISCONNECTED) {
		return;
	}

	StopBroadcast("HLTV Shutdown.");

	m_Master.ShutDown();
	m_Status.ShutDown();
	m_DemoClient.ShutDown();

	// Director module shutdown
	if (m_Director) {
		m_Director->ShutDown();
	}

	// World module shutdown
	if (m_World) {
		m_World->ShutDown();
	}

	// Server module shutdown
	if (m_Server) {
		m_Server->ShutDown();
	}

	// Network module shutdown
	if (m_Network) {
		m_Network->ShutDown();
	}

	m_LoopCommands.Clear(true);
	m_BannList.Clear(true);

	m_InfoRules.Free();
	m_InfoPlayers.Free();
	m_InfoDetails.Free();
	m_InfoInfo.Free();
	m_InfoString.Free();

	ClearResources();
	BaseSystemModule::ShutDown();
	m_System->Printf("Proxy module shutdown.\n");
}

void Proxy::ReplyPing(NetAddress *to)
{
	m_Socket->OutOfBandPrintf(to, "%c", A2A_ACK);
}

void Proxy::CMD_Ping(char *cmdLine)
{
	NetAddress to;
	TokenLine params(cmdLine);
	if (!m_Network->ResolveAddress(params.GetToken(1), &to)) {
		m_System->Printf("Error! HLTV Proxy::CMD_Ping: IP address not valid.\n");
		return;
	}

	if (!to.m_Port) {
		to.SetPort(Q_atoi("27015"));
	}

	m_Socket->OutOfBandPrintf(&to, "ping");
}

void Proxy::ReplyChallenge(NetAddress *to)
{
	unsigned int challengenr = GetChallengeNumber(to);

	m_Socket->OutOfBandPrintf(to, "%c00000000 %u %i\n", S2C_CHALLENGE, challengenr, 2);
	m_System->DPrintf("Received challenge from %s.\n", to->ToString());
}

void Proxy::ReplyInfoString(NetAddress *to)
{
	m_System->DPrintf("Info string request from %s.\n", to->ToString());
	m_Socket->SendPacket(to, m_InfoString.GetData(), m_InfoString.CurrentSize());
}

void Proxy::ReplyInfo(NetAddress *to, bool detailed)
{
	m_System->DPrintf("General information%srequest from %s.\n", detailed ? " (detail) " : " ", to->ToString());

	if (m_World->IsActive())
	{
		BitBuffer *buf = &(detailed ? m_InfoDetails : m_InfoInfo);
		m_Socket->SendPacket(to, buf->GetData(), buf->CurrentSize());
	}
}

void Proxy::ReplyPlayers(NetAddress *to)
{
	m_System->DPrintf("Player information request from %s.\n", to->ToString());

	if (m_World->IsActive()) {
		m_Socket->SendPacket(to, m_InfoPlayers.GetData(), m_InfoPlayers.CurrentSize());
	}
}

void Proxy::ReplyRules(NetAddress *to)
{
	m_System->DPrintf("Rules information request from %s.\n", to->ToString());

	if (m_World->IsActive()) {
		m_Socket->SendPacket(to, m_InfoRules.GetData(), m_InfoRules.CurrentSize());
	}
}

void Proxy::ReplyConnect(NetAddress *to, int protocol, int challenge, char *protinfo, char *userinfo)
{
	InfoString info(userinfo);
	NetAddress relayProxy;

	int type = Q_atoi(info.ValueForKey("*hltv"));
	char *name = info.ValueForKey("name");

	if (protocol != PROTOCOL_VERSION) {
		RejectConnection(to, false, "This HLTV proxy is using protocol %i which is incompatible with yours (%i).\n", PROTOCOL_VERSION, protocol);
		return;
	}

	if (!CheckChallenge(to, challenge)) {
		RejectConnection(to, false, "Challenge number invalid.\n");
		return;
	}

	if (!m_World->IsActive()) {
		RejectConnection(to, false, "HLTV not started. %s\n", m_OffLineText);
		return;
	}

	if (m_MaxClients <= 0) {
		RejectConnection(to, false, "HLTV proxy disabled.\n");
		return;
	}

	if (type == TYPE_CLIENT && m_DispatchMode != DISPATCH_OFF)
	{
		float ratio = m_Status.GetBestRelayProxy(&relayProxy);
		float myRatio = float(m_Clients.CountElements()) / float(m_MaxClients) * 1.25f;
		if (myRatio > 1) {
			myRatio = 1;
		}

		if (ratio < 0)
		{
			if (m_DispatchMode == DISPATCH_ALL) {
				RejectConnection(to, false, "HLTV network is full.\n");
				return;
			}
		}
		else if (myRatio >= ratio || m_DispatchMode == DISPATCH_ALL)
		{
			DispatchClient(to, &relayProxy);
			return;
		}
	}

	if (!IsValidPassword(type, info.ValueForKey("password"))) {
		RejectConnection(to, true, "Bad password");
		return;
	}

	if (IsStressed() && type == TYPE_CLIENT) {
		RejectConnection(to, false, "Workload limit exceeded.");
		return;
	}

	char *clientTypeString[] = { "Spectator", "Relay Proxy", "Director", "Commentator", "Fake Client" };
	IClient *client = (IClient *)m_Clients.GetFirst();
	while (client)
	{
		if (to->Equal(client->GetAddress())) {
			m_System->Printf("%s reconnected (%s at %s).\n", clientTypeString[type], name, to->ToString());
			break;
		}

		client = (IClient *)m_Clients.GetNext();
	}

	// Duplicate client IP is not found.
	if (!client)
	{
		if (m_Clients.CountElements() >= m_MaxClients) {
			RejectConnection(to, false, (m_MaxClients > 0) ? "HLTV proxy is full.\n" : "HLTV proxy is disabled.\n");
			return;
		}

		client = new ProxyClient(this);

		if (!m_System->AddModule(client, to->ToString()))
		{
			RejectConnection(to, false, "HLTV proxy is overloaded.\n");
			delete client;
			return;
		}

		client->SetWorld(m_World);
		m_Clients.AddHead(client);
		m_System->Printf("%s connected (%s at %s).\n", clientTypeString[type], name, to->ToString());
	}

	if (!client->Connect(m_Socket, to, userinfo)) {
		RejectConnection(to, false, "Connection rejected.\n");
		m_System->RemoveModule(client);
		m_System->Printf("Refused director connection for %s.\n", to->ToString());
		return;
	}

	int specs, proxies;
	CountLocalClients(specs, proxies);
	if (m_MaxSeenClients < specs) {
		m_MaxSeenClients = specs;
	}
}

void Proxy::CMD_Clients(char *cmdLine)
{
	int count = 0;
	TokenLine params(cmdLine);
	IClient *client = (IClient *)m_Clients.GetFirst();
	while (client)
	{
		count++;
		m_System->Printf("%s", client->GetStatusLine());
		client = (IClient *)m_Clients.GetNext();
	}

	m_System->Printf("--- Total %i Clients ---\n", count);
}

void Proxy::RejectConnection(NetAddress *adr, bool badPassword, const char *fmt, ...)
{
	va_list argptr;
	char text[1024] = "";

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	if (badPassword) {
		m_Socket->OutOfBandPrintf(adr, "%cBADPASSWORD", S2C_REJECT_BADPASSWORD);
	}
	else {
		m_Socket->OutOfBandPrintf(adr, "%c%s", S2C_REJECT, text);
	}

	m_System->DPrintf("Rejected connection: %s (%s)\n", text, adr->ToString());
}

char *Proxy::GetStatusLine()
{
	static char string[256];
	Q_snprintf(string, sizeof(string), "Proxy name \"%s\", %s, Port %i, Clients %i/%i.\n", m_World->GetName(), m_IsMaster ? "Master" : "Relay", m_Socket->GetPort(), m_Clients.CountElements(), m_MaxClients);
	return string;
}

IObjectContainer *Proxy::GetClients()
{
	return &m_Clients;
}

IWorld *Proxy::GetWorld()
{
	return m_World;
}

IServer *Proxy::GetServer()
{
	return m_Server;
}

IDirector *Proxy::GetDirector()
{
	return m_Director;
}

INetSocket *Proxy::GetSocket()
{
	return m_Socket;
}

ChatMode_e Proxy::GetChatMode()
{
	return m_ChatMode;
}

void Proxy::UpdateStatusLine()
{
	float in, out;
	char text[128];
	char activeTime[32];

	Q_strlcpy(activeTime, COM_FormatTime((float)m_World->GetTime()));

	m_Network->GetFlowStats(&in, &out);
	m_CurrentLoss = m_Server->GetPacketLoss();

	Q_snprintf(text, sizeof(text), "%s, Time %s, Delay %.0f, FPS %.0f, Clients %i, In %.1f, Out %.1f, Loss %.2f",
		IsMaster() ? "Master" : "Relay", activeTime, m_ClientDelay, m_FPS, m_Clients.CountElements(), in, out, m_CurrentLoss);

	m_System->SetStatusLine(text);
	m_NextStatusUpdateTime = m_SystemTime + 0.25;
}

void Proxy::CMD_Status(char *cmdLine)
{
	float in, out, loss;
	m_Network->GetFlowStats(&in, &out);
	loss = m_Server->GetPacketLoss();

	m_System->Printf("--- HLTV Status ---\n");
	m_System->Printf("Online %s, FPS %.1f, Version %i (%s)\n", COM_FormatTime((float)m_System->GetTime()), m_FPS, COM_BuildNumber(), __isWindows ? "Win32" : "Linux");
	m_System->Printf("Local IP %s, Network In %.1f, Out %.1f, Loss %.2f\n", m_Network->GetLocalAddress()->ToString(), in, out, loss);

	int spectators, proxies;
	CountLocalClients(spectators, proxies);
	m_System->Printf("Local Slots %i, Spectators %i (max %i), Proxies %i\n", GetMaxClients(), spectators, m_MaxSeenClients, proxies);

	if (m_Server->IsConnected())
	{
		int slots, maxspecs;
		m_Status.GetGlobalStats(proxies, slots, spectators);
		maxspecs = m_Status.GetMaxSpectatorNumber();
		m_System->Printf("Total Slots %i, Spectators %i (max %i), Proxies %i\n", slots, spectators, maxspecs, proxies);

		if (m_Server->IsDemoFile())
		{
			m_System->Printf("Playing Demo File \"%s\"\n", m_Server->GetDemoFileName());
		}
		else if (m_Server->IsGameServer())
		{
			m_System->Printf("Connected to Game Server %s, Delay %.0f\n", m_Server->GetAddress()->ToString(), m_ClientDelay);
			m_System->Printf("Server Name \"%s\"\n", m_Server->GetHostName());
		}
		else if (m_Server->IsRelayProxy())
		{
			m_System->Printf("Connected to HLTV Proxy %s\n", m_Server->GetAddress()->ToString());
			m_System->Printf("Proxy Name \"%s\"\n", m_Server->GetHostName());
		}
		else
		{
			m_System->Printf("Not connected.\n");
		}
	}
	else
	{
		m_System->Printf("Not connected.\n");
	}

	if (m_World->IsActive())
	{
		char activeTime[32];
		Q_strlcpy(activeTime, COM_FormatTime((float)m_World->GetTime()));

		const char *mapname = m_World->GetLevelName() + sizeof("maps/") - 1; // skip 'maps/'
		m_System->Printf("Game Time %s, Mod \"%s\", Map \"%s\", Players %i\n", activeTime, m_World->GetGameDir(), mapname, m_World->GetNumPlayers());
	}

	if (m_DemoClient.IsActive())
	{
		DemoFile *df = m_DemoClient.GetDemoFile();
		m_System->Printf("Recording to %s, Length %.1f sec.\n", df->GetFileName(), df->GetDemoTime());
	}
}

void Proxy::CountLocalClients(int &spectators, int &proxies)
{
	spectators = 0;
	proxies = 0;

	IClient *client = (IClient *)m_Clients.GetFirst();
	while (client)
	{
		if (client->GetClientType() == TYPE_PROXY)
			proxies++;
		else
			spectators++;

		client = (IClient *)m_Clients.GetNext();
	}
}

void Proxy::GetStatistics(int &proxies, int &slots, int &spectators)
{
	m_Status.GetGlobalStats(proxies, slots, spectators);
}

int Proxy::GetMaxUpdateRate()
{
	return m_MaxUpdateRate;
}

int Proxy::GetMaxRate()
{
	return m_MaxRate;
}

bool Proxy::WriteSignonData(int type, BitBuffer *stream)
{
	if (type != TYPE_CLIENT && type != TYPE_DEMO) {
		return false;
	}

	if (m_SignonCommands[0]) {
		stream->WriteByte(svc_stufftext);
		stream->WriteString(COM_VarArgs("%s\n", m_SignonCommands));
	}

	float ex_interp = (1.0f / GetMaxUpdateRate()) + 0.05f;
	stream->WriteByte(svc_stufftext);
	stream->WriteString(COM_VarArgs("ex_interp %.2f\n", ex_interp));

	stream->WriteByte(svc_timescale);
	stream->WriteFloat(1);

	if (m_BannerTGA) {
		DirectorCmd cmd;
		cmd.SetBannerData(m_BannerTGA->szFileName);
		cmd.WriteToStream(stream);
	}

	return true;
}

bool Proxy::IsPublicGame()
{
	return m_PublicGame;
}

bool Proxy::IsPasswordProtected()
{
	return m_SpectatorPassword[0] ? true : false;
}

bool Proxy::IsStressed()
{
	if (m_CurrentLoss > m_MaxLoss || m_FPS < 30) {
		return true;
	}

	return false;
}

int Proxy::GetDispatchMode()
{
	return m_DispatchMode;
}

char *Proxy::GetType()
{
	return PROXY_INTERFACE_VERSION;
}

bool Proxy::IsLanOnly()
{
	return m_Master.m_NoMaster;
}

bool Proxy::IsActive()
{
	if (!m_World) {
		return false;
	}

	return m_World->IsActive();
}

unsigned char Proxy::GetRegion()
{
	return m_Region;
}

void Proxy::ReplyListen(NetAddress *to)
{
	m_Socket->OutOfBandPrintf(to, "%c 0.0.0.0\n", S2A_PROXY_LISTEN);
}

void Proxy::Reset()
{
	m_Status.Reset();

	m_MaxSeenClients = 0;
	m_LastCheeringUpdate = 0;
	m_CheeringPlayers = 0;
}

void Proxy::ParseStatusMsg(BitBuffer *stream)
{
	m_Status.ParseStatusMsg(stream);
}

void Proxy::ParseStatusReport(NetAddress *from, BitBuffer *stream)
{
	m_Status.ParseStatusReport(from, stream);
}

void Proxy::Broadcast(byte *data, int length, int groupType, bool isReliable)
{
	IClient *client = (IClient *)m_Clients.GetFirst();
	while (client)
	{
		if (client->IsActive()
			&& (((groupType & GROUP_CLIENT) && client->GetClientType() == TYPE_CLIENT)
			||  ((groupType & GROUP_PROXY)  && client->GetClientType() == TYPE_PROXY)
			||  ((groupType & GROUP_VOICE)  && client->IsHearingVoices())
			||  ((groupType & GROUP_CHAT)   && client->HasChatEnabled())))
		{
			client->Send(data, length, isReliable);
		}

		client = (IClient *)m_Clients.GetNext();
	}

	if (m_DemoClient.IsActive())
	{
		if (groupType & GROUP_DEMO) {
			m_DemoClient.Send(data, length, isReliable);
		}
	}
}

void Proxy::CMD_Say(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() < 2)
	{
		m_System->Printf("Syntax: say <text>\n");
		return;
	}

	if (m_Server->IsConnected())
	{
		char string[1024];
		Q_snprintf(string, sizeof(string), "say \"%s\"", params.GetRestOfLine(1));
		m_Server->SendStringCommand(string);
	}
}

void Proxy::CMD_MaxClients(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: maxclients <number>\n");
		m_System->Printf("Current number of maximal clients is %i.\n", GetMaxClients());
		return;
	}

	if (!SetMaxClients(Q_atoi(params.GetToken(1)))) {
		m_System->Printf("Allowed maximum number of local clients is %i.\n", MAX_PROXY_CLIENTS);
		return;
	}
}

bool Proxy::SetMaxClients(int number)
{
	if (number < 0) {
		m_MaxClients = 0;
		return false;
	}

	if (number > MAX_PROXY_CLIENTS) {
		m_MaxClients = MAX_PROXY_CLIENTS;
		return false;
	}

	m_MaxClients = number;
	return true;
}

void Proxy::SetMaxLoss(float maxloss)
{
	m_MaxLoss = clamp(maxloss, 0.0f, 1.0f);
}

int Proxy::GetMaxClients()
{
	return m_MaxClients;
}

void Proxy::SetRegion(unsigned char region)
{
	m_Region = region;
}

void Proxy::CMD_Delay(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: delay <seconds>\n");
		m_System->Printf("Current spectator delay is %.1f seconds.\n", m_ClientDelay);
		return;
	}

	SetDelay(float(Q_atof(params.GetToken(1))));
}

void Proxy::CMD_Stop(char *cmdLine)
{
	TokenLine params(cmdLine);
	StopBroadcast(params.CountToken() > 1 ? params.GetRestOfLine(1) : "HLTV stopped.");
	m_System->Printf("Disconnected and stoppted.\n");
}

void Proxy::CMD_Connect(char *cmdLine)
{
	NetAddress address;
	TokenLine params(cmdLine);
	if (!m_Network->ResolveAddress(params.GetToken(1), &address)) {
		m_System->Printf("Error! HLTV Proxy::ConnectToServer: couldn't resolve server address.\n");
		return;
	}

	if (!address.m_Port) {
		address.SetPort(Q_atoi("27015"));
	}

	Reset();
	m_Server->Connect(m_World, &address, m_Socket);
}

void Proxy::CMD_Name(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() < 2)
	{
		m_System->Printf("Current name is \"%s\".\n", m_Server->GetPlayerName());
		return;
	}

	char name[MAX_NAME];
	unsigned int len = Q_strlen(params.GetToken(1));
	if (len > sizeof(name) - 1) {
		m_System->Printf("Invalid name length.\n");
		return;
	}

	Q_strlcpy(name, params.GetToken(1));
	m_Server->SetPlayerName(name);
}

void Proxy::CMD_Msg(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() < 2)
	{
		m_System->Printf("Syntax: msg <text> [<duration> <pos x> <pos y> <color hex rgba>]\n", m_Server->GetPlayerName());
		return;
	}

	Q_strlcpy(m_LocalMessage.text, params.GetToken(1));

	if (params.CountToken() == 6)
	{
		m_LocalMessage.holdtime = float(Q_atof(params.GetToken(2)));
		m_LocalMessage.x = float(Q_atof(params.GetToken(3)));
		m_LocalMessage.y = float(Q_atof(params.GetToken(4)));

		sscanf(params.GetToken(5), "%2hhx%2hhx%2hhx%2hhx", &m_LocalMessage.r1, &m_LocalMessage.g1, &m_LocalMessage.b1, &m_LocalMessage.a1);
	}

	BitBuffer buffer(144);
	WriteHUDMsg(&m_LocalMessage, &buffer);

	Broadcast(buffer.GetData(), buffer.CurrentSize(), GROUP_CLIENT_ALL, false);
}

void Proxy::CMD_Protocol(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: protocol <46|47>\n");
		m_System->Printf("Current protcol version is %i\n", m_Server->GetProtocol());
		return;
	}

	if (!m_Server->SetProtocol(Q_atoi(params.GetToken(1)))) {
		m_System->Printf("Protocol version not supported!\n");
		return;
	}
}

void Proxy::CMD_MaxRate(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: maxrate <n>\n");
		m_System->Printf("Current maximal client rate is %i bytes/sec.\n", m_MaxRate);
		return;
	}

	SetMaxRate(Q_atoi(params.GetToken(1)));
}

void Proxy::CMD_ServerCmd(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() < 2)
	{
		m_System->Printf("Syntax: servercmd <command>\n");
		return;
	}

	if (m_Server->IsConnected()) {
		m_Server->SendStringCommand(params.GetRestOfLine(1));
	}
}

void Proxy::CMD_ClientCmd(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() < 3)
	{
		m_System->Printf("Syntax: clientcmd <group> <command>\n");
		m_System->Printf("groups: 1 = spectators only, 2 = proxies only, 3 = all\n");
		return;
	}

	int group = Q_atoi(params.GetToken(1));
	char *cmdstring = params.GetRestOfLine(2);
	if (Q_strlen(cmdstring) > 100) {
		m_System->Printf("ERROR! Command string too long.\n");
		return;
	}

	switch (group)
	{
	case 1: // spectators
		group = GROUP_CLIENT | GROUP_DEMO | GROUP_MULTICAST;
		break;
	case 2: // proxies
		group = GROUP_PROXY;
		break;
	default: // all
		group = GROUP_CLIENT_ALL;
		break;
	}

	BitBuffer cmdbuf(128);
	cmdbuf.WriteByte(svc_stufftext);
	cmdbuf.WriteString(COM_VarArgs("%s\n", cmdstring));

	Broadcast(cmdbuf.GetData(), cmdbuf.CurrentSize(), group, true);
}

void Proxy::CMD_Rate(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: rate <bytes per second>\n");
		m_System->Printf("Current maximal server to HLTV proxy rate is %i bytes/sec.\n", m_Server->GetRate());
		return;
	}

	m_Server->SetRate(Q_atoi(params.GetToken(1)));
}

void Proxy::CMD_Players(char *cmdLine)
{
	int count = 0;
	InfoString playerInfo(MAX_INFO_STRING);
	for (int i = 0; i < m_World->GetMaxClients(); i++)
	{
		if (m_World->GetPlayerInfoString(i, &playerInfo)) {
			m_System->Printf("#%2i \"%s\" %s\n", i + 1, playerInfo.ValueForKey("name"), playerInfo.ValueForKey("model"));
			count++;
		}
	}

	m_System->Printf("--- Total %i Players ---\n", count);
}

void Proxy::CMD_HostName(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Host name is \"%s\"\n", m_World->GetHostName());
		return;
	}

	if (!Q_stricmp(params.GetToken(1), "none")) {
		m_World->SetHostName(nullptr);
		return;
	}

	m_World->SetHostName(params.GetToken(1));
}

void Proxy::CMD_Updaterate(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: updaterate <n>\n");
		m_System->Printf("Current update rate is %i packets/sec.\n", m_Server->GetUpdateRate());
		return;
	}

	SetMaxUpdateRate(Q_atoi(params.GetToken(1)));
	m_Server->SetUpdateRate(m_MaxUpdateRate);
}

void Proxy::CMD_BlockVoice(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: blockvoice <0|1>\n");
		m_System->Printf("Currently voice data is %s.\n", m_Server->IsVoiceBlocking() ? "blocked" : "relayed");
		return;
	}

	m_Server->SetVoiceBlocking(Q_atoi(params.GetToken(1)) ? true : false);
}

void Proxy::CMD_Record(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: record <filename>\n");
		m_System->Printf("Output format is \"filename-date-map.dem\".\n");
		return;
	}

	if (m_DemoClient.Connect())
	{
		m_DemoClient.SetFileName(params.GetToken(1));

#ifdef HLTV_FIXES
		// Increased rate for recording demofile
		m_DemoClient.SetUpdateRate(m_MaxUpdateRate);
		m_DemoClient.SetRate(m_MaxRate);
#endif
	}
}

void Proxy::CMD_StopRecording(char *cmdLine)
{
	if (m_DemoClient.IsActive()) {
		m_DemoClient.Disconnect("End of Record");
		return;
	}

	m_System->Printf("Not recording.\n");
}

void Proxy::CMD_LoopCmd(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() < 4)
	{
		m_System->Printf("Syntax: loopcmd <id> <seconds> <commands>\n");
		m_System->Printf("Currently %i looping commands in list.\n", m_LoopCommands.CountElements());
		return;
	}

	int id = Q_atoi(params.GetToken(1));
	float seconds = float(Q_atof(params.GetToken(2)));
	char *cmds = params.GetRestOfLine(3);

	loopcmd_t *lcmd = (loopcmd_t *)m_LoopCommands.GetFirst();
	while (lcmd)
	{
		if (lcmd->id == id) {
			break;
		}

		lcmd = (loopcmd_t *)m_LoopCommands.GetNext();
	}

	if (!Q_stricmp(cmds, "none") || !seconds)
	{
		if (lcmd) {
			m_LoopCommands.Remove(lcmd);
			return;
		}

		m_System->Printf("Couldn't remove loop command %i\n", id);
		return;
	}

	if (!lcmd)
	{
		lcmd = (loopcmd_t *)Mem_ZeroMalloc(sizeof(loopcmd_t));
		lcmd->id = id;

		m_LoopCommands.Add(lcmd);
	}

	lcmd->interval = seconds;
	lcmd->lastTime = 0;

	Q_strlcpy(lcmd->command, cmds);
}

void Proxy::CMD_RconAddress(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: rconaddress <IP:Port>\n");
		m_System->Printf("Current remote console address: %s\n", m_RconAddress.ToString());
		return;
	}

	m_Network->ResolveAddress(params.GetToken(1), &m_RconAddress);
	if (!m_RconAddress.m_Port) {
		m_RconAddress.SetPort(Q_atoi("27015"));
	}
}

void Proxy::CMD_RconPassword(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: rconpassword <string>\n");
		return;
	}

	Q_strlcpy(m_RconPassword, params.GetToken(1));
}

void Proxy::CMD_Rcon(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() < 2)
	{
		m_System->Printf("Syntax: rcon <string>\n");
		return;
	}

	if (!m_RconPassword[0]) {
		m_System->Printf("Set 'rconpassword' before issuing a rcon command.\n");
		return;
	}

	if (!m_RconAddress.IsValid() && m_Server->IsConnected()) {
		m_RconAddress.FromNetAddress(m_Server->GetAddress());
	}

	if (!m_RconAddress.IsValid()) {
		m_System->Printf("rconaddress not valid.\n");
		return;
	}

	Q_strlcpy(m_LastRconCommand, params.GetRestOfLine(1));
	m_Socket->OutOfBandPrintf(&m_RconAddress, "challenge rcon\n");
}

void Proxy::SendRcon(NetAddress *to, unsigned int challenge)
{
	if (!m_LastRconCommand[0]) {
		m_System->Printf("HLTV Proxy::SendRcon: Empty rcon string\n");
		return;
	}

	if (!to->Equal(&m_RconAddress)) {
		m_System->Printf("Unwanted rcon challenge reply from %s\n", to->ToString());
		return;
	}

	m_Socket->OutOfBandPrintf(to, "rcon %u \"%s\" %s", challenge, m_RconPassword, m_LastRconCommand);
}

unsigned int Proxy::GetChallengeNumber(NetAddress *host)
{
	int i;
	int oldest = 0;
	float oldestTime = 9.9999997e37f;

	for (i = 0; i < MAX_CHALLENGES; i++)
	{
		if (m_Challenges[i].adr.EqualBase(host)) {
			break;
		}

		if (m_Challenges[i].time < oldestTime)
		{
			oldestTime = m_Challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		m_Challenges[oldest].adr.FromNetAddress(host);

		// generate new challenge number
		m_Challenges[oldest].challenge = (RandomLong(0, 0xFFFF) | RandomLong(0, 0xFFFF) << 16);
		m_Challenges[oldest].time = float(m_SystemTime);

		i = oldest;
	}

	if (m_SystemTime > m_Challenges[i].time + PROXY_CHALLENGE_LIFE)
	{
		// generate new challenge number
		m_Challenges[i].challenge = (RandomLong(0, 0xFFFF) | RandomLong(0, 0xFFFF) << 16);
		m_Challenges[i].time = float(m_SystemTime);
	}

	return m_Challenges[i].challenge;
}

bool Proxy::CheckChallenge(NetAddress *from, unsigned int challengeNumber)
{
	for (auto& it : m_Challenges)
	{
		if (from->EqualBase(&it.adr)) {
			if (it.challenge == challengeNumber) {
				return (m_SystemTime - it.time <= PROXY_CHALLENGE_LIFE);
			}

			break;
		}
	}

	return false;
}

void Proxy::ReplyServiceChallenge(NetAddress *to, char *type)
{
	unsigned int challengeNumber = GetChallengeNumber(to);
	m_Socket->OutOfBandPrintf(to, "challenge %s %u\n", type, challengeNumber);
}

void Proxy::CMD_ProxyPassword(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: proxypassword <password>\n");
		return;
	}

	if (!Q_stricmp(params.GetToken(1), "none")) {
		m_ProxyPassword[0] = '\0';
		return;
	}

	Q_strlcpy(m_ProxyPassword, params.GetToken(1));
}

void Proxy::NewServerConnection()
{
	m_IsMaster = m_Server->IsGameServer();

	if (m_IsMaster && m_ClientDelay > 0)
	{
		m_World->SetBufferSize(m_ClientDelay + m_ClientDelay);
		m_Server->SetDelayReconnect(true);
	}
	else
	{
		m_World->SetBufferSize(10);
		m_Server->SetDelayReconnect(false);
	}

	resource_t *resource = (resource_t *)m_Resources.GetFirst();
	while (resource)
	{
		m_World->AddResource(resource);
		resource = (resource_t *)m_Resources.GetNext();
	}

	m_IsReconnectRequested = false;
}

void Proxy::ReceiveSignal(ISystemModule *module, unsigned int signal, void *data)
{
	int from = module->GetSerial();

	// Server module
	if (from == m_Server->GetSerial())
	{
		switch (signal)
		{
		case 4:
			NewServerConnection();
			break;
		case 5:
		case 6:
			m_IsFinishingBroadcast = true;
			break;
		case 7:
			BroadcastRetryMessage();
			break;
		default:
			break;
		}

		return;
	}

	// World module
	if (from == m_World->GetSerial())
	{
		switch (signal)
		{
		case 2:
			NewGameStarted();
			ReconnectClients();
			break;
		case 5:
		case 6:
			BroadcastPaused(signal == 5 ? true : false);
			break;
		case 8:
			StopBroadcast("HLTV shutddown.");
			break;
		default:
			break;
		}

		return;
	}
}

char *Proxy::GetModVersion(char *gamedir)
{
	static char version[] = PROXY_VERSION;
	return version;
}

void Proxy::BroadcastRetryMessage()
{
	BitBuffer msg(32);
	msg.WriteByte(svc_centerprint);
	msg.WriteString("Retrying HLTV connection ...");

	Broadcast(msg.GetData(), msg.CurrentSize(), GROUP_CLIENT | GROUP_PROXY | GROUP_MULTICAST, false);
}

void Proxy::StopBroadcast(const char *message)
{
	DisconnectClients(message ? COM_VarArgs("%s\n", message) : "HLTV stopped.\n");

	m_Server->Disconnect();
	m_Server->StopRetry();
	m_World->StopGame();

	m_Master.SendShutdown();
	m_System->SetTitle("HLTV - offline");
}

void Proxy::ExecuteRcon(NetAddress *from, char *command)
{
	char outputbuf[1024];
	outputbuf[0] = '\0';
	m_System->Printf("Executing rcon \"%s\" from %s.\n", command, from->ToString());

	m_System->RedirectOutput(outputbuf + 1, sizeof(outputbuf) - 1);
	m_System->ExecuteString(command);
	m_System->RedirectOutput();

	if (outputbuf[1]) {
		m_Socket->OutOfBandPrintf(from, "%c%s", A2A_PRINT, outputbuf);
	}
}

bool Proxy::ProcessConnectionlessMessage(NetAddress *from, BitBuffer *stream)
{
	if (IsBanned(from)) {
		return false;
	}

	TokenLine cmdLine;
	if (!cmdLine.SetLine(stream->ReadStringLine())) {
		m_System->DPrintf("WARNING! HLTV Proxy::ProcessConnectionlessMessage: message too long.\n");
		return false;
	}

	if (!cmdLine.CountToken()) {
		m_System->DPrintf("WARNING! Invalid packet from %s.\n", from->ToString());
		return false;
	}

	if (SteamGameServer())
	{
		const int maxBuffer = 4096;
		unsigned char data[maxBuffer + 4];
		int maxSize = Q_min(stream->m_MaxSize, maxBuffer);

		Q_memcpy((char *)&data[4], stream->m_Data, maxSize);
		*(uint32 *)data = CONNECTIONLESS_HEADER; // connectionless packet

		SteamGameServer()->HandleIncomingPacket(data, maxSize + 4, ntohl(*(u_long *)&from->m_IP[0]), htons(from->m_Port));
	}

	char *c = cmdLine.GetToken(0);
	if (c[0] == S2C_CHALLENGE)
	{
		m_System->DPrintf("Unwanted challenge response from %s.\n", from->ToString());
		return false;
	}

	if (!Q_strcmp(c, "challenge"))
	{
		char *type = cmdLine.GetToken(1);
		switch (cmdLine.CountToken())
		{
		case 3:
		{
			if (type && type[0] && !Q_stricmp(type, "rcon"))
			{
				unsigned int challengeNr = strtoul(cmdLine.GetToken(2), 0, 10);
				SendRcon(from, challengeNr);
				break;
			}

			m_System->Printf("Invalid challenge type (%s) from: %s\n", type ? type : "NULL", from->ToString());
			return false;
		}
		case 2:
			ReplyServiceChallenge(from, type);
			break;
		default:
			m_System->Printf("Invalid challenge request from: %s\n", from->ToString());
			return false;
		}
	}
	else if (!Q_strcmp(c, "rcon"))
	{
		if (cmdLine.CountToken() < 4) {
			return false;
		}

		unsigned int challenge = strtoul(cmdLine.GetToken(1), 0, 10);
		char *password = cmdLine.GetToken(2);
		char *command = cmdLine.GetRestOfLine(3);

		if (!CheckChallenge(from, challenge))
		{
			m_System->Printf("Invalid rcon challenge from: %s\n", from->ToString());
			return false;
		}

		if (!m_AdminPassword[0] || Q_strcmp(m_AdminPassword, password) != 0)
		{
			m_System->Printf("Invalid rcon password from: %s\n", from->ToString());
			return false;
		}

		ExecuteRcon(from, command);
	}
	else if (!Q_strcmp(c, "getchallenge"))
	{
		ReplyChallenge(from);
	}
	else if (!Q_strcmp(c, "connect"))
	{
		if (cmdLine.CountToken() == 5)
		{
			int protocol = Q_atoi(cmdLine.GetToken(1));
			int challenge = Q_atoi(cmdLine.GetToken(2));

			char *protinfo = cmdLine.GetToken(3);
			char *userinfo = cmdLine.GetToken(4);

			ReplyConnect(from, protocol, challenge, protinfo, userinfo);
		}
		else
		{
			RejectConnection(from, false, "Insufficient connection info\n");
		}
	}
	else if (c[0] == A2A_ACK && (c[1] == 0 || c[1] == '\n'))
	{
		m_System->DPrintf("Acknowledgment from %s.\n", from->ToString());
	}
	else if (c[0] == A2S_INFO)
	{
		ReplyInfo(from, true);
	}
	else if (c[0] == A2S_PLAYER)
	{
		ReplyPlayers(from);
	}
	else if (c[0] == A2S_RULES)
	{
		ReplyRules(from);
	}
	else if (!Q_strcmp(c, "listen"))
	{
		ReplyListen(from);
	}
	// guard to frequency queries
	else if (m_MaxFrameQueries > 0)
	{
		if (!Q_strcmp(c, "ping") || (c[0] == A2A_PING && (c[1] == 0 || c[1] == '\n')))
		{
			ReplyPing(from);
		}
		else if (!Q_stricmp(c, "infostring"))
		{
			ReplyInfoString(from);
		}
		else if (!Q_stricmp(c, "info"))
		{
			ReplyInfo(from, false);
		}
		else if (!Q_stricmp(c, "details"))
		{
			ReplyInfo(from, true);
		}
		else if (!Q_stricmp(c, "players"))
		{
			ReplyPlayers(from);
		}
		else if (!Q_stricmp(c, "rules"))
		{
			ReplyRules(from);
		}
		else if (!Q_stricmp(c, "log"))
		{
			m_System->Printf("Ignoring log from %s.\n", from->ToString());
			return false;
		}
		else if (c[0] == A2A_PRINT)
		{
			stream->Reset();
			stream->SkipBytes(1);

			m_System->Printf(">%s\n", stream->ReadString());
		}
		else
		{
			m_System->DPrintf("Proxy::ProcessConnectionlessMessage: unknown \"%s\" from %s \n", c, from->ToString());
			return false;
		}

		m_MaxFrameQueries--;
	}

	return true;
}

void Proxy::CMD_ChatMode(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		static const char *chatModeString[] = { "OFF", "LOCAL", "GLOBAL" };

		m_System->Printf("Syntax: chatmode <0|1|2>\n");
		m_System->Printf("Currently chat mode is %s.\n", chatModeString[ m_ChatMode ]);
		return;
	}

	m_ChatMode = clamp((ChatMode_e)Q_atoi(params.GetToken(1)), CHAT_OFF, CHAT_GLOBAL);
}

void Proxy::CMD_MaxQueries(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: maxqueries <n>\n");
		m_System->Printf("Currently maximum %.0f queries per seconds accepted.\n", m_MaxQueries);
		return;
	}

	m_MaxQueries = float(Q_atof(params.GetToken(1)));
}

void Proxy::CMD_Kick(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: kick <Client ID>\n");
		return;
	}

	int id = Q_atoi(params.GetToken(1));

	IClient *client = (IClient *)m_Clients.GetFirst();
	while (client)
	{
		if (client->GetSerial() == id) {
			client->Disconnect("You have been kicked.\n");
			return;
		}

		client = (IClient *)m_Clients.GetNext();
	}

	m_System->Printf("%i is not a valid client ID.\n", id);
}

void Proxy::CMD_Disconnect(char *cmdLine)
{
	m_Server->Disconnect();
}

void Proxy::ChatSpectator(char *nick, char *text)
{
	if (!text || m_ChatMode == CHAT_OFF) {
		return;
	}

	int sayMsgNum = m_World->FindUserMsgByName("SayText");
	if (!sayMsgNum) {
		return;
	}

	char string[512] = "";
	COM_RemoveEvilChars(text);

	// TODO: too harsh, let more? maxlen: 190
	const int maxLengthOfString = 64;
	int len = Q_strlen(text);
	if (len <= 0) {
		return;
	}

	if (len > maxLengthOfString) {
		text[maxLengthOfString] = '\0';
	}

	if (m_ChatMode != CHAT_LOCAL && !m_IsMaster)
	{
		Q_snprintf(string, sizeof(string), "say \"%s\"", text);
		if (m_Server->IsConnected()) {
			m_Server->SendStringCommand(string);
		}

		return;
	}

	const int extraBytesOfEnd = 2;				// extra of ends bytes \n + \0
	const int headerBytes = 3;					// reserve a header of 3 bytes.
	const int maxSizeOfMessage = MAX_USER_MSG_DATA - headerBytes;	// user message size limit is 192 bytes

	Q_snprintf(&string[ headerBytes ], sizeof(string) - headerBytes, "<%s> %s", nick, text);

	int curLen = Q_strlen(&string[ headerBytes ]);
	if (curLen > maxSizeOfMessage) {
		curLen = maxSizeOfMessage;
	}

	// fill up to 3+ remaining bytes
	string[0] = sayMsgNum;				// unique id usermsg SayText.
	string[1] = curLen + headerBytes;	// the length of message.
	string[2] = '\0';					// terminal null

	string[curLen + 3] = '\n';
	string[curLen + 4] = '\0';

	int groupType;
	if (m_ChatMode == CHAT_LOCAL) {
		groupType = GROUP_CHAT;
	} else {
		groupType = GROUP_CHAT | GROUP_PROXY;
	}

	Broadcast((byte *)string, curLen + headerBytes + extraBytesOfEnd, groupType, false);
}

bool Proxy::IsMaster()
{
	return m_IsMaster;
}

bool Proxy::CheckDirectorModule()
{
	char szAbsoluteLibFilename[MAX_PATH];
	if (m_Director && !Q_strcmp(m_Director->GetModName(), m_World->GetGameDir())) {
		return true;
	}

	Q_snprintf(szAbsoluteLibFilename, sizeof(szAbsoluteLibFilename), "%s/dlls/director", m_World->GetGameDir());
	if (m_Director) {
		m_System->RemoveModule(m_Director);
	}

	m_Director = dynamic_cast<IDirector *>(m_System->GetModule(DIRECTOR_INTERFACE_VERSION, szAbsoluteLibFilename, "director"));
	if (m_Director)
	{
		m_System->DPrintf("Using extern director module (%s).\n", szAbsoluteLibFilename);
		return true;
	}

	// If we don't have director module,
	// to set up on internal default director.
	if (!m_System->AddModule(&m_DefaultDirector, "director")) {
		m_System->Errorf("Proxy::CheckDirectorModule: failed to add internal director module.\n");
		return false;
	}

	m_Director = &m_DefaultDirector;
	m_System->DPrintf("Using internal default director.\n");

	return true;
}

void Proxy::ReconnectClients()
{
	IClient *client = (IClient *)m_Clients.GetFirst();
	while (client)
	{
		client->Reconnect();
		client = (IClient *)m_Clients.GetNext();
	}

	m_DemoClient.Reconnect();
}

void Proxy::CMD_OffLineText(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: offlinetext <text>\n");
		m_System->Printf("Offline info text is \"%s\"\n", m_OffLineText);
		return;
	}

	Q_strlcpy(m_OffLineText, params.GetToken(1));
	COM_RemoveEvilChars(m_OffLineText);
}

void Proxy::CMD_Resources(char *cmdLine)
{
	resource_t *resource = (resource_t *)m_Resources.GetFirst();
	while (resource)
	{
		m_System->Printf("File: \"%s\", Size: %i bytes.\n", resource->szFileName, resource->nDownloadSize);
		resource = (resource_t *)m_Resources.GetNext();
	}

	m_System->Printf("--- Total %i Resources ---\n", m_Resources.CountElements());
}

void Proxy::CMD_AddResource(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() < 3)
	{
		m_System->Printf("Syntax: addresource <filename> <type> [<alias>]\n");
		return;
	}

	if (!AddResource(params.GetToken(1), (resourcetype_t)Q_atoi(params.GetToken(2)), params.GetToken(3)))
	{
		m_System->Printf("Error! Failed to load resource %s.\n", params.GetToken(1));
		return;
	}
}

void Proxy::CMD_PublicGame(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: publicgame <0|1>\n");
		m_System->Printf("Joingame is %s.\n", m_PublicGame ? "enabled" : "disabled");
		return;
	}

	m_PublicGame = Q_atoi(params.GetToken(1)) ? true : false;
}

void Proxy::WriteHUDMsg(textmessage_t *msg, BitBuffer *stream)
{
	DirectorCmd cmd;

	vec3_t position;
	position[0] = msg->x;
	position[1] = msg->y;
	position[2] = 0;

	cmd.SetMessageData(
		msg->effect,
		COM_PackRGB(msg->r1, msg->g1, msg->b1),
		position,
		msg->fadein,
		msg->fadeout,
		msg->holdtime,
		msg->fxtime,
		msg->text
	);

	cmd.WriteToStream(stream);
}

void Proxy::CMD_SpectatorPassword(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: spectatorpassword <password>\n");
		return;
	}

	if (!Q_stricmp(params.GetToken(1), "none")) {
		m_SpectatorPassword[0] = '\0';
		return;
	}

	Q_strlcpy(m_SpectatorPassword, params.GetToken(1));
}

void Proxy::CMD_DispatchMode(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		static const char *dispatchModeString[] = { "OFF", "AUTO", "ALWAYS" };

		m_System->Printf("Syntax: dispatchmode <0|1|2>\n");
		m_System->Printf("Spectator dispatch mode is: %s.\n", dispatchModeString[ m_DispatchMode ]);
		return;
	}

	m_DispatchMode = clamp((DispatchMode_e)Q_atoi(params.GetToken(1)), DISPATCH_OFF, DISPATCH_ALL);
}

bool Proxy::IsValidPassword(int type, char *pw)
{
	switch (type)
	{
	// spec password
	case TYPE_CLIENT:
	{
		if (m_SpectatorPassword[0]) {
			return Q_strcmp(m_SpectatorPassword, pw) == 0;
		}

		return true;
	}
	// proxy password
	case TYPE_PROXY:
	{
		if (m_ProxyPassword[0]) {
			return Q_strcmp(m_ProxyPassword, pw) == 0;
		}

		// password is not set
		return true;
	}
	// admin password
	case TYPE_COMMENTATOR:
	{
		if (m_AdminPassword[0]) {
			return Q_strcmp(m_AdminPassword, pw) == 0;
		}

		break;
	}
	default:
		break;
	}

	return false;
}

void Proxy::CMD_PlayDemo(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() < 2)
	{
		m_System->Printf("Sytax: playdemo <filename>\n");
		return;
	}

	m_Server->LoadDemo(m_World, params.GetToken(1), true, true);
	m_World->SetBufferSize(10);
}

void Proxy::DispatchClient(NetAddress *client, NetAddress *proxy)
{
	BitBuffer buf(128);

	buf.WriteLong(CONNECTIONLESS_HEADER);	// connectionless header
	buf.WriteByte(S2A_PROXY_REDIRECT);		// data header
	buf.WriteString(proxy->ToString());		// ip proxy destination

	m_Socket->SendPacket(client, buf.GetData(), buf.CurrentSize());
	m_System->DPrintf("Dispatched client %s to proxy %s.\n", client->ToString(), proxy->ToString());
}

resource_t *Proxy::LoadResourceFromFile(char *fileName, resourcetype_t type)
{
	resource_t *newresource = (resource_t *)Mem_ZeroMalloc(sizeof(resource_t));

	Q_strlcpy(newresource->szFileName, fileName);

	newresource->type = type;
	newresource->data = m_System->LoadFile(newresource->szFileName, &newresource->nDownloadSize);
	if (newresource->data && newresource->nDownloadSize >= 0) {
		MD5_Hash_Mem(newresource->rgucMD5_hash, newresource->data, newresource->nDownloadSize);
		return newresource;
	}

	m_System->Printf("WARNING! Failed to load resource file %s.\n", fileName);
	Mem_Free(newresource);
	return nullptr;
}

void Proxy::FreeResource(resource_t *resource)
{
	if (!resource) {
		return;
	}

	if (resource->data) {
		m_System->FreeFile(resource->data);
	}

	Mem_Free(resource);
}

void Proxy::ClearResources()
{
	resource_t *resource;
	while ((resource = (resource_t *)m_Resources.RemoveHead())) {
		FreeResource(resource);
	}
}

resource_t *Proxy::AddResource(char *fileName, resourcetype_t type, char *asFileName)
{
	resource_t *resource = GetResource(fileName);
	if (resource) {
		FreeResource(resource);
		m_Resources.Remove(resource);
	}

	resource = LoadResourceFromFile(fileName, t_generic /* type */); // TODO: why we use only t_generic?
	if (resource)
	{
		if (asFileName) {
			Q_strlcpy(resource->szFileName, asFileName);
		}

		m_Resources.Add(resource);
		return resource;
	}

	return nullptr;
}

resource_t *Proxy::GetResource(char *fileName)
{
	resource_t *resource = (resource_t *)m_Resources.GetFirst();
	while (resource)
	{
		if (!_strnicmp(fileName, resource->szFileName, sizeof(resource->szFileName))) {
			return resource;
		}

		resource = (resource_t *)m_Resources.GetNext();
	}

	return nullptr;
}

void Proxy::CMD_SignOnCommands(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: signoncommands <string>\n");
		m_System->Printf("Current sign on commands: \"%s\"\n", m_SignonCommands);
		return;
	}

	if (Q_strlen(params.GetToken(1)) > 250)
	{
		m_System->Printf("Error! String too long (>250)!\n");
		return;
	}

	Q_strlcpy(m_SignonCommands, params.GetToken(1));
}

void Proxy::CMD_AdminPassword(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: adminpassword <password>\n");
		return;
	}

	if (!Q_stricmp(params.GetToken(1), "none")) {
		m_AdminPassword[0] = '\0';
		return;
	}

	Q_strlcpy(m_AdminPassword, params.GetToken(1));
}

void Proxy::CMD_LocalMsg(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() < 2)
	{
		m_System->Printf("Syntax: localmsg <text> [<duration> <pos x> <pos y> <color hex rgba>]\n");
		return;
	}

	Q_strlcpy(m_LocalMessage.text, params.GetToken(1));

	if (params.CountToken() == 6)
	{
		m_LocalMessage.holdtime = float(Q_atof(params.GetToken(2)));
		m_LocalMessage.x = float(Q_atof(params.GetToken(3)));
		m_LocalMessage.y = float(Q_atof(params.GetToken(4)));

		sscanf(params.GetToken(5), "%2hhx%2hhx%2hhx%2hhx", &m_LocalMessage.r1, &m_LocalMessage.g1, &m_LocalMessage.b1, &m_LocalMessage.a1);
	}

	BitBuffer buffer(144);
	WriteHUDMsg(&m_LocalMessage, &buffer);

	Broadcast(buffer.GetData(), buffer.CurrentSize(), GROUP_CLIENT | GROUP_DEMO, false);
}

void Proxy::ChatCommentator(char *nick, char *text)
{
	BitBuffer buffer(144);

	Q_strlcpy(m_CommentatorMessage.text, text);

	COM_RemoveEvilChars(m_CommentatorMessage.text);
	WriteHUDMsg(&m_CommentatorMessage, &buffer);

	Broadcast(buffer.GetData(), buffer.CurrentSize(), GROUP_CLIENT_ALL, true);
}

bool Proxy::IsBanned(NetAddress *adr)
{
	NetAddress *bannedAdr = (NetAddress *)m_BannList.GetFirst();
	while (bannedAdr)
	{
		if (adr->EqualBase(bannedAdr)) {
			return true;
		}

		bannedAdr = (NetAddress *)m_BannList.GetNext();
	}

	return false;
}

void Proxy::CMD_AddFakeClients(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 5)
	{
		m_System->Printf("Syntax: addfakeclients <number> <IP:port> <rate> <activity>\n");
		return;
	}

	int number = Q_atoi(params.GetToken(1));

	NetAddress adr;
	m_Network->ResolveAddress(params.GetToken(2), &adr);

	int rate = Q_atoi(params.GetToken(3));
	float activity = float(Q_atof(params.GetToken(4)));

	for (int i = 0; i < number; i++)
	{
		FakeClient *fakeclient = new FakeClient;
		if (!m_System->AddModule(fakeclient, "")) {
			m_System->Errorf("AddFakeClients: failed to add module.\n");
			delete fakeclient;
			break;
		}

		fakeclient->SetRate(rate);
		fakeclient->Connect(&adr);
	}
}

void Proxy::CMD_MaxLoss(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: maxloss <f>\n");
		m_System->Printf("Current server packet loss limit is %.2f.\n", m_MaxLoss);
		return;
	}

	SetMaxLoss(float(Q_atof(params.GetToken(1))));
}

void Proxy::CMD_Region(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: region <f>\n");
		m_System->Printf("Current server region is %i.\n", m_Region);
		return;
	}

	SetRegion(Q_atoi(params.GetToken(1)));
}

void Proxy::CMD_Bann(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: bann <IP>\n");
		m_System->Printf("Currently %i IPs banned\n", m_BannList.CountElements());
		return;
	}

	NetAddress *adr = (NetAddress *)Mem_ZeroMalloc(sizeof(NetAddress));
	if (!(m_Network->ResolveAddress(params.GetToken(1), adr)))
	{
		m_System->Printf("Couldn't resolve IP \x02%s\"\n", params.GetToken(1));
		Mem_Free(adr);
		return;
	}

	if (IsBanned(adr)) {
		m_System->Printf("IP already banned.\n");
		Mem_Free(adr);
		return;
	}

	m_BannList.Add(adr);
}

void Proxy::CMD_ClearBanns(char *cmdLine)
{
	m_System->Printf("Clearing IP bann list (%i entries).\n", m_BannList.CountElements());
	m_BannList.Clear(true);
}

void Proxy::CMD_ServerPassword(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() < 2)
	{
		m_System->Printf("Sytax: serverpassword <string>\n");
		return;
	}

	m_Server->SetUserInfo("password", params.GetToken(1));
}

void Proxy::CMD_Retry(char *cmdLine)
{
	m_System->Printf("Retrying connection...\n");
	m_Server->Retry();
}

void Proxy::CMD_AutoRetry(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: autoretry <0|1>\n");
		m_System->Printf("Automatic connection retry is %s.\n", m_Server->GetAutoRetry() ? "enabled" : "disabled");
		return;
	}

	m_Server->SetAutoRetry(Q_atoi(params.GetToken(1)) ? true : false);
}

void Proxy::CMD_BannerFile(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: bannerfile <file.tga>\n");
		if (m_BannerTGA) {
			m_System->Printf("Current HLTV banner file is %s\n", m_BannerTGA->szFileName);
			return;
		}

		m_System->Printf("No HLTV banner file specified.\n");
		return;
	}

	if (!Q_stricmp(params.GetToken(1), "none")) {
		m_BannerTGA = nullptr;
		return;
	}

	if (!(m_BannerTGA = AddResource(params.GetToken(1), t_generic))) {
		m_System->Printf("ERROR! Couldn't load banner file.\n");
		return;
	}

	Q_snprintf(m_BannerTGA->szFileName, sizeof(m_BannerTGA->szFileName), "gfx/temp/%s.tga", COM_BinPrintf(m_BannerTGA->rgucMD5_hash, sizeof(m_BannerTGA->rgucMD5_hash)));
}

void Proxy::CMD_CheeringThreshold(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: cheeringthreshold <n>\n");
		m_System->Printf("Cheering threshold is %.2f.\n", m_CheeringThreshold);
		return;
	}

	m_CheeringThreshold = float(Q_atof(params.GetToken(1)));
}

float Proxy::GetDelay()
{
	if (m_IsMaster) {
		return m_ClientDelay;
	}

	return 0;
}

double Proxy::GetSpectatorTime()
{
	return m_ClientWorldTime;
}

double Proxy::GetProxyTime()
{
	return m_ClientProxyTime;
}

void Proxy::IncreaseCheering(int votes)
{
	m_CheeringPlayers += votes;
	float fraction = float(m_CheeringPlayers) / float(m_Clients.CountElements());
	if (fraction > 1) {
		fraction = 1;
	}

	if (fraction > m_CheeringThreshold)
	{
		DirectorCmd cmd;
		BitBuffer buf(64);

		cmd.SetSoundData("ambience/goal_1.wav", fraction);
		cmd.WriteToStream(&buf);

		Broadcast(buf.GetData(), buf.CurrentSize(), GROUP_CLIENT | GROUP_DEMO, true);
		m_CheeringPlayers = 0;
	}
}

void Proxy::ExecuteLoopCommands()
{
	static float nextCheck = 0.0f;
	if (nextCheck > m_SystemTime) {
		return;
	}

	loopcmd_t *lcmd = (loopcmd_t *)m_LoopCommands.GetFirst();
	while (lcmd)
	{
		if (m_SystemTime > (lcmd->lastTime + lcmd->interval)) {
			m_System->ExecuteString(lcmd->command);
			lcmd->lastTime = float(m_SystemTime);
		}

		lcmd = (loopcmd_t *)m_LoopCommands.GetNext();
	}

	nextCheck = float(m_SystemTime + 0.5f);
}

void Proxy::CreateServerInfoString(InfoString *info)
{
	int proxies, slots, spectators;
	m_Status.GetLocalStats(proxies, slots, spectators);

	char address[256];
	Q_snprintf(address, sizeof(address), "%s", m_Network->GetLocalAddress()->ToString());

	info->SetValueForKey("protocol", COM_VarArgs("%i", PROTOCOL_VERSION));
	info->SetValueForKey("address", address);
	info->SetValueForKey("players", COM_VarArgs("%i", spectators));
	info->SetValueForKey("proxy", COM_VarArgs("%i", IsMaster() ? 1 : 2));

	if (IsPublicGame()) {
		info->SetValueForKey("proxyaddress", m_World->GetGameServerAddress()->ToString());
	}

	info->SetValueForKey("lan", COM_VarArgs("%i", IsLanOnly()));
	info->SetValueForKey("max", COM_VarArgs("%i", slots));
	if (m_World->IsActive())
	{
		char gameDir[MAX_PATH], mapName[MAX_PATH];
		COM_FileBase(m_World->GetLevelName(), mapName);
		COM_FileBase(m_World->GetGameDir(), gameDir);

		serverinfo_t *serverInfo = m_World->GetServerInfo();
		info->SetValueForKey("gamedir", gameDir);
		info->SetValueForKey("description", serverInfo->description);
		info->SetValueForKey("hostname", m_World->GetHostName());
		info->SetValueForKey("map", mapName);

		if (serverInfo->mod)
		{
			info->SetValueForKey("mod", "1");
			info->SetValueForKey("modversion", COM_VarArgs("%i", serverInfo->ver));
		}
	}
	else
	{
		info->SetValueForKey("gamedir", "valve");
		info->SetValueForKey("description", "HLTV");
		info->SetValueForKey("hostname", m_OffLineText);
		info->SetValueForKey("map", "none");
	}

	info->SetValueForKey("type", GetServerType(HLST_TV));
	info->SetValueForKey("password", COM_VarArgs("%i", IsPasswordProtected()));
	info->SetValueForKey("os", GetServerOS());
	info->SetValueForKey("secure", "0");
}

void Proxy::SetMaxRate(int rate)
{
	m_MaxRate = clamp(rate, MIN_PROXY_RATE, MAX_PROXY_RATE);
}

void Proxy::SetMaxUpdateRate(int updaterate)
{
	m_MaxUpdateRate = clamp(updaterate, MIN_PROXY_UPDATERATE, MAX_PROXY_UPDATERATE);
}

void Proxy::SetDelay(float seconds)
{
	m_ClientDelay = seconds;

	if (seconds < 10)
	{
		m_ClientDelay = 0;
		m_World->SetBufferSize(10);
	}
	else
	{
		m_World->SetBufferSize(seconds + seconds);
		m_ClientWorldTime = m_World->GetTime() - m_ClientDelay;
	}

	m_Server->SetUserInfo("hdelay", COM_VarArgs("%u", (int)m_ClientDelay));
}

void Proxy::SetClientTime(double time, bool relative)
{
	if (relative) {
		m_ClientWorldTime += time;
		return;
	}

	m_ClientWorldTime = time;
}

void Proxy::SetClientTimeScale(float scale)
{
	BitBuffer buf(32);
	m_ClientTimeScale = clamp(scale, 0.5f, 4.0f);

	buf.WriteByte(svc_timescale);
	buf.WriteFloat(m_ClientTimeScale);

	Broadcast(buf.GetData(), buf.CurrentSize(), GROUP_CLIENT_ALL, true);
}

void Proxy::BroadcastPaused(bool paused)
{
	BitBuffer buf(32);
	buf.WriteByte(svc_centerprint);
	buf.WriteString(paused ? "Game was paused" : "Game was unpaused");

	buf.WriteByte(svc_setpause);
	buf.WriteByte(paused ? 1 : 0);

	Broadcast(buf.GetData(), buf.CurrentSize(), GROUP_CLIENT_ALL, true);
}

void Proxy::NewGameStarted()
{
	m_LastClockUpdateTime = m_SystemTime;
	m_IsFinishingBroadcast = false;

	m_ClientWorldTime = 0;
	m_ClientProxyTime = 0;
	m_ClientTimeScale = 1;

	m_CurrentLoss = 0;

	if (m_Server->IsDemoFile())
	{
		if (m_Director) {
			m_System->RemoveModule(m_Director);
			m_Director = nullptr;
		}
	}
	else if (GetDelay() > 0)
	{
		if (CheckDirectorModule()) {
			m_Director->NewGame(m_World, this);
		}

		m_ClientWorldTime = m_World->GetTime() - m_ClientDelay;
	}
	else
	{
		if (m_Director) {
			m_System->RemoveModule(m_Director);
			m_Director = nullptr;
		}

		if (m_IsMaster)
		{
			if (m_System->AddModule(&m_NullDirector, "nulldirector")) {
				m_Director = &m_NullDirector;
				m_NullDirector.NewGame(m_World, this);
				m_System->DPrintf("Using zero delay director.\n");
			}
			else {
				m_System->Errorf("Proxy::NewGameStarted: failed to add null director module.\n");
			}
		}
	}

	m_Server->SetDirector(m_Director);

	char newTitle[4096];
	Q_snprintf(newTitle, sizeof(newTitle), "HLTV - %s %s",
		m_Server->IsDemoFile() ? m_Server->GetDemoFileName() : m_Server->GetAddress()->ToString(),
		m_World->GetLevelName());

	m_System->SetTitle(newTitle);
}

void Proxy::RunClocks()
{
	double lastClockTime = m_LastClockUpdateTime;
	m_LastClockUpdateTime = m_SystemTime;

	if (m_World->IsPaused()) {
		return;
	}

	double realTimeDiff = (m_SystemTime - lastClockTime) * m_ClientTimeScale;
	m_ClientProxyTime += realTimeDiff;
	m_ClientWorldTime += realTimeDiff;

	if (!m_IsFinishingBroadcast)
	{
		frame_t *start = m_World->GetFirstFrame();
		if (m_ClientWorldTime > m_World->GetTime()) {
			m_ClientWorldTime = m_World->GetTime() - m_ClientDelay;
			return;
		}

		if (start->time > (m_ClientWorldTime + m_ClientDelay))
		{
			m_System->DPrintf("Proxy::RunClocks: forcing client delay (1).\n");
			m_ClientWorldTime = start->time - m_ClientDelay;
			return;
		}

		if (m_World->GetTime() - m_ClientDelay > (m_ClientWorldTime + 10)) {
			m_System->DPrintf("Proxy::RunClocks: forcing client delay (2).\n");
			m_ClientWorldTime = m_World->GetTime() - m_ClientDelay;
			return;
		}
	}
}

void Proxy::DisconnectClients(const char *reason)
{
	IClient *client = (IClient *)m_Clients.GetFirst();
	while (client)
	{
		client->Disconnect(reason);
		client = (IClient *)m_Clients.GetNext();
	}

	m_DemoClient.Disconnect(reason);
}

void Proxy::UpdateInfoMessages()
{
	char ipport[32];
	int proxies, slots, spectators;
	serverinfo_t *serverInfo = m_World->GetServerInfo();

	m_Status.GetLocalStats(proxies, slots, spectators);
	Q_snprintf(ipport, sizeof(ipport), "%s:%i", m_Network->GetLocalAddress()->ToBaseString(), m_Socket->GetPort());

	m_InfoInfo.Clear();
	if (m_World->IsActive())
	{
		// deprecated goldsrc response
		m_InfoInfo.WriteLong(CONNECTIONLESS_HEADER);				// connectionless header
		m_InfoInfo.WriteByte(S2A_INFO);								// data header
		m_InfoInfo.WriteString(ipport);								// server address
		m_InfoInfo.WriteString(m_World->GetHostName());				// hostname
		m_InfoInfo.WriteString(serverInfo->map);					// mapname
		m_InfoInfo.WriteString(serverInfo->gamedir);				// gamedir
		m_InfoInfo.WriteString(serverInfo->description);			// gamename
		m_InfoInfo.WriteByte(Q_min(spectators, MAX_PROXY_CLIENTS));	// players
		m_InfoInfo.WriteByte(Q_min(slots, MAX_PROXY_CLIENTS));		// maxplayers
		m_InfoInfo.WriteByte(PROTOCOL_VERSION);						// protocol

		if (spectators >= MAX_PROXY_CLIENTS || slots >= MAX_PROXY_CLIENTS)
		{
			m_InfoInfo.WriteLong(spectators);
			m_InfoInfo.WriteLong(slots);
		}
	}

	m_InfoDetails.Clear();
	if (m_World->IsActive())
	{
		m_InfoDetails.WriteLong(CONNECTIONLESS_HEADER);					// connectionless header
		m_InfoDetails.WriteByte(S2A_INFO_DETAILED);						// data header (detailed mode)
		m_InfoDetails.WriteString(ipport);								// server address
		m_InfoDetails.WriteString(m_World->GetHostName());				// hostname
		m_InfoDetails.WriteString(serverInfo->map);						// mapname
		m_InfoDetails.WriteString(serverInfo->gamedir);					// gamedir
		m_InfoDetails.WriteString(serverInfo->description);				// gamename
		m_InfoDetails.WriteByte(Q_min(spectators, MAX_PROXY_CLIENTS));	// players
		m_InfoDetails.WriteByte(Q_min(slots, MAX_PROXY_CLIENTS));		// maxplayers
		m_InfoDetails.WriteByte(PROTOCOL_VERSION);						// protocol

		m_InfoDetails.WriteByte(GetServerType(HLST_TV)[0]);				// server type (Indicates the type of server: HLTV Server)
		m_InfoDetails.WriteByte(GetServerOS()[0]);						// server os   (Indicates the operating system of the server)
		m_InfoDetails.WriteByte(IsPasswordProtected());					// password

		// whether the game is a mod
		// 0: Half-Life
		// 1: Half-Life mod
		m_InfoDetails.WriteByte(serverInfo->mod);				// game is mod

		if (serverInfo->mod)
		{
			m_InfoDetails.WriteString(serverInfo->url_info);	// url to mod website
			m_InfoDetails.WriteString(serverInfo->url_dl);		// url to download the mod
			m_InfoDetails.WriteString(serverInfo->hlversion);
			m_InfoDetails.WriteLong(serverInfo->ver);			// version of mod installed on server
			m_InfoDetails.WriteLong(serverInfo->size);			// space (in bytes) the mod takes up
			m_InfoDetails.WriteByte(serverInfo->svonly);		// the type of mod
			m_InfoDetails.WriteByte(serverInfo->cldll);			// whether mod uses its own DLL
		}

		m_InfoDetails.WriteByte(0);								// specifies whether the server uses VAC

		if (spectators >= MAX_PROXY_CLIENTS || slots >= MAX_PROXY_CLIENTS)
		{
			m_InfoDetails.WriteLong(spectators);
			m_InfoDetails.WriteLong(slots);
		}
	}

	m_InfoRules.Clear();
	if (m_World->IsActive())
	{
		m_InfoRules.WriteLong(CONNECTIONLESS_HEADER);			// connectionless header
		m_InfoRules.WriteByte(S2A_RULES);						// rules query are multi-packeted
		m_InfoRules.WriteShort(5);								// number of rules in the response

		// HLTV Rules
		m_InfoRules.WriteString("GameServer");
		m_InfoRules.WriteString(GetDescription());

		m_InfoRules.WriteString("HLTVProxy");
		m_InfoRules.WriteString(COM_VarArgs("%i", m_IsMaster ? 1 : 2));

		m_InfoRules.WriteString("HLTVDelay");
		m_InfoRules.WriteString(COM_VarArgs("%.1f", m_ClientDelay));

		m_InfoRules.WriteString("HLTVChat");
		m_InfoRules.WriteString(COM_VarArgs("%i", m_ChatMode));

		m_InfoRules.WriteString("HLTVDemo");
		m_InfoRules.WriteString(m_DemoClient.IsActive() ? m_DemoClient.GetDemoFile()->GetFileName() : "none");
	}

	m_InfoPlayers.Clear();

	InfoString info(2048);
	if (m_World->GetNumPlayers() > 0)
	{
		m_InfoPlayers.WriteLong(CONNECTIONLESS_HEADER);			// connectionless header
		m_InfoPlayers.WriteByte(S2A_PLAYERS);					// data header (players info)
		m_InfoPlayers.WriteByte(0);								// number of players whose information was gathered

		int count = 0;
		for (int i = 0; i < m_World->GetMaxClients(); i++)
		{
			if (!m_World->GetPlayerInfoString(i, &info)) {
				continue;
			}

			m_InfoPlayers.WriteByte(++count);						// index of player chunk starting from 0
			m_InfoPlayers.WriteString(info.ValueForKey("name"));	// name of the player.
			m_InfoPlayers.WriteLong(0);								// player's score (usually "frags" or "kills")
			m_InfoPlayers.WriteFloat(1);							// time (in seconds) player has been connected to the server
		}

		m_InfoPlayers.m_Data[5] = count;
	}

	CreateServerInfoString(&info);

	m_InfoString.Clear();
	m_InfoString.WriteLong(CONNECTIONLESS_HEADER);		// connectionless header
	m_InfoString.WriteString("infostringresponse");		// data header (response on infostring)
	m_InfoString.WriteString(info.GetString());			// info

	m_NextInfoMessagesUpdate = m_SystemTime + 1;
}

void Proxy::SetName(char *newName)
{
	Q_strlcpy(m_Name, newName);
}

const char *Proxy::GetDescription()
{
	if (m_Server->IsDemoFile()) {
		return m_Server->GetDemoFileName();
	}

	if (IsPublicGame()) {
		return m_World->GetGameServerAddress()->ToString();
	}

	return "Private Server";
}

void Proxy::CMD_ChatDelay(char *cmdLine)
{
	enum { param_ChatDelay = 1 };

	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: chatdelay <number>\n");
		m_System->Printf("Current clients chat delay is %i.\n", GetChatDelay());
		return;
	}

	SetChatDelay(Q_atoi(params.GetToken(param_ChatDelay)));
}

void Proxy::SetChatDelay(int delay)
{
	m_ChatDelay = delay;
}

int Proxy::GetChatDelay() const
{
	return m_ChatDelay;
}
