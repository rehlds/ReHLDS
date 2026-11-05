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

Server::svc_func_s Server::m_ClientFuncs[]
{
	{ svc_bad,                 "svc_bad",                 &Server::ParseBad },
	{ svc_nop,                 "svc_nop",                 &Server::ParseNop },
	{ svc_disconnect,          "svc_disconnect",          &Server::ParseDisconnect },
	{ svc_event,               "svc_event",               &Server::ParseEvent },
	{ svc_version,             "svc_version",             &Server::ParseVersion },
	{ svc_setview,             "svc_setview",             &Server::ParseSetView },
	{ svc_sound,               "svc_sound",               &Server::ParseSound },
	{ svc_time,                "svc_time",                &Server::ParseTime },
	{ svc_print,               "svc_print",               &Server::ParsePrint },
	{ svc_stufftext,           "svc_stufftext",           &Server::ParseStuffText },
	{ svc_setangle,            "svc_setangle",            &Server::ParseSetAngle },
	{ svc_serverinfo,          "svc_serverinfo",          &Server::ParseServerinfo },
	{ svc_lightstyle,          "svc_lightstyle",          &Server::ParseLightStyle },
	{ svc_updateuserinfo,      "svc_updateuserinfo",      &Server::ParseUpdateUserInfo },
	{ svc_deltadescription,    "svc_deltadescription",    &Server::ParseDeltaDescription },
	{ svc_clientdata,          "svc_clientdata",          &Server::ParseClientData },
	{ svc_stopsound,           "svc_stopsound",           &Server::ParseStopSound },
	{ svc_pings,               "svc_pings",               &Server::ParsePings },
	{ svc_particle,            "svc_particle",            &Server::ParseParticle },
	{ svc_damage,              "svc_damage",              &Server::ParseNop },
	{ svc_spawnstatic,         "svc_spawnstatic",         &Server::ParseNop },
	{ svc_event_reliable,      "svc_event_reliable",      &Server::ParseEventReliable },
	{ svc_spawnbaseline,       "svc_spawnbaseline",       &Server::ParseBaseline },
	{ svc_temp_entity,         "svc_temp_entity",         &Server::ParseTempEntity },
	{ svc_setpause,            "svc_setpause",            &Server::ParseSetPause },
	{ svc_signonnum,           "svc_signonnum",           &Server::ParseSignonNum },
	{ svc_centerprint,         "svc_centerprint",         &Server::ParseCenterPrint },
	{ svc_killedmonster,       "svc_killedmonster",       &Server::ParseNop },
	{ svc_foundsecret,         "svc_foundsecret",         &Server::ParseNop },
	{ svc_spawnstaticsound,    "svc_spawnstaticsound",    &Server::ParseSpawnStaticSound },
	{ svc_intermission,        "svc_intermission",        &Server::ParseIntermission },
	{ svc_finale,              "svc_finale",              &Server::ParseFinale },
	{ svc_cdtrack,             "svc_cdtrack",             &Server::ParseCDTrack },
	{ svc_restore,             "svc_restore",             &Server::ParseRestore },
	{ svc_cutscene,            "svc_cutscene",            &Server::ParseCutscene },
	{ svc_weaponanim,          "svc_weaponanim",          &Server::ParseWeaponAnim },
	{ svc_decalname,           "svc_decalname",           &Server::ParseDecalName },
	{ svc_roomtype,            "svc_roomtype",            &Server::ParseRoomType },
	{ svc_addangle,            "svc_addangle",            &Server::ParseAddAngle },
	{ svc_newusermsg,          "svc_newusermsg",          &Server::ParseNewUserMsg },
	{ svc_packetentities,      "svc_packetentities",      &Server::ParsePacketEntities },
	{ svc_deltapacketentities, "svc_deltapacketentities", &Server::ParseDeltaPacketEntities },
	{ svc_choke,               "svc_choke",               &Server::ParseChoke },
	{ svc_resourcelist,        "svc_resourcelist",        &Server::ParseResourceList },
	{ svc_newmovevars,         "svc_newmovevars",         &Server::ParseMoveVars },
	{ svc_resourcerequest,     "svc_resourcerequest",     &Server::ParseResourceRequest },
	{ svc_customization,       "svc_customization",       &Server::ParseCustomization },
	{ svc_crosshairangle,      "svc_crosshairangle",      &Server::ParseCrosshairAngle },
	{ svc_soundfade,           "svc_soundfade",           &Server::ParseSoundFade },
	{ svc_filetxferfailed,     "svc_filetxferfailed",     &Server::ParseFileTransferFailed },
	{ svc_hltv,                "svc_hltv",                &Server::ParseHLTV },
	{ svc_director,            "svc_director",            &Server::ParseDirector },
	{ svc_voiceinit,           "svc_voiceinit",           &Server::ParseVoiceInit },
	{ svc_voicedata,           "svc_voicedata",           &Server::ParseVoiceData },
	{ svc_sendextrainfo,       "svc_sendextrainfo",       &Server::ParseSendExtraInfo },
	{ svc_timescale,           "svc_timescale",           &Server::ParseTimeScale },
	{ svc_resourcelocation,    "svc_resourcelocation",    &Server::ParseResourceLocation },
	{ svc_sendcvarvalue,       "svc_sendcvarvalue",       &Server::ParseSendCvarValue },
	{ svc_sendcvarvalue2,      "svc_sendcvarvalue2",      &Server::ParseSendCvarValue2 },
	{ svc_exec,                "svc_exec",                &Server::ParseExec },
	{ svc_endoflist,           "End of List",             nullptr }
};

bool Server::Init(IBaseSystem *system, int serial, char *name)
{
	SetState(SERVER_INITIALIZING);
	BaseSystemModule::Init(system, serial, name);

	if (!name) {
		SetName(SERVER_INTERFACE_VERSION);
	}

	m_FileSystem = m_System->GetFileSystem();
	m_Rate = 10000;
	m_UpdateRate = 20;
	m_NextAutoRetry = 0;
	m_DelayReconnect = true;
	m_Protocol = PROTOCOL_VERSION;

	m_UserInfo.SetMaxSize(MAX_INFO_STRING);
	m_UserInfo.SetValueForKey("name", "HLTV Proxy");
	m_UserInfo.SetValueForKey("cl_lw", "1");
	m_UserInfo.SetValueForKey("cl_lc", "1");
	m_UserInfo.SetValueForKey("*hltv", "1");
	m_UserInfo.SetValueForKey("rate", COM_VarArgs("%i", m_Rate));
	m_UserInfo.SetValueForKey("cl_updaterate", COM_VarArgs("%i", m_UpdateRate));

	m_IsGameServer = false;
	m_IsVoiceBlocking = false;
	m_ServerSocket = nullptr;

#ifdef HLTV_FIXES
	m_PendingIntermission = false;
#endif

	m_ServerChannel.Create(system);
	m_ServerInfo.SetMaxSize(512);

	SetState(SERVER_DISCONNECTED);

	m_ReliableData.Resize(MAX_UDP_PACKET);
	m_UnreliableData.Resize(MAX_UDP_PACKET);
	m_VoiceData.Resize(MAX_UDP_PACKET);
	m_UserMessages.Resize(MAX_UDP_PACKET);
	m_ClientData.Resize(MAX_UDP_PACKET);
	m_DemoData.Resize(MAX_UDP_PACKET);

	Q_memset(&m_Frame, 0, sizeof(m_Frame));

	m_Frame.reliableData   = m_ReliableData.GetData();
	m_Frame.unreliableData = m_UnreliableData.GetData();
	m_Frame.voiceData      = m_VoiceData.GetData();
	m_Frame.userMessages   = m_UserMessages.GetData();
	m_Frame.clientData     = m_ClientData.GetData();
	m_Frame.demoData       = m_DemoData.GetData();

	m_UnreliableData.Clear();
	m_ReliableData.Clear();
	m_VoiceData.Clear();
	m_UserMessages.Clear();
	m_ClientData.Clear();
	m_DemoData.Clear();

	Q_strlcpy(m_CDKey, "2123437429222");
	Q_strlcpy(m_HostName, "Unkown Host");

	Q_memset(m_SeqNrMap, 0, sizeof(m_SeqNrMap));
	m_validSequence = 0;
	m_AutoRetry = true;
	m_IsHLTV = true;
	m_ForceHLTV = false;
	m_IsPaused = false;

	m_World = nullptr;
	m_Proxy = nullptr;
	m_Director = nullptr;

	m_DemoFile.Reset();
	m_State = MODULE_RUNNING;
	m_System->Printf("Server module initialized.\n");

	return true;
}

void Server::SetGameDirectory(const char *defaultDir, const char *gameDir)
{
	char temp[MAX_PATH];
	m_FileSystem->RemoveAllSearchPaths();

	if (gameDir && Q_stricmp(gameDir, defaultDir) != 0)
	{
		Q_sprintf(temp, "%s/%s", m_System->GetBaseDir(), gameDir);
		m_FileSystem->AddSearchPath(temp, "GAME");
	}

	Q_sprintf(temp, "%s/%s", m_System->GetBaseDir(), defaultDir);
	m_FileSystem->AddSearchPath(temp, "DEFAULTGAME");
	m_FileSystem->AddSearchPath(m_System->GetBaseDir(), "ROOT");
}

void Server::ExecuteCommand(int commandID, char *commandLine)
{
	m_System->Printf("ERROR! Server::ExecuteCommand: unknown command ID %i.\n", commandID);
}

void Server::Challenge()
{
	const int MAX_CONNECT_RETRIES = 3;
	const float CONNECT_RETRY_INTERVAL = 4.0;

	static float nextRetry = 0;
	if (!m_CurrentRetry)
	{
		nextRetry = 0;
		m_CurrentRetry++;
	}

	if (nextRetry >= m_SystemTime) {
		return;
	}

	if (m_CurrentRetry > MAX_CONNECT_RETRIES)
	{
		m_System->Printf("WARNING! Server::Challenge: Timeout after %i retries\n", MAX_CONNECT_RETRIES);
		SetState(SERVER_DISCONNECTED);
		ScheduleAutoRetry();
		return;
	}

	m_ServerChannel.OutOfBandPrintf("%s\n", "getchallenge");
	m_System->Printf("Challenging %s (%i/%i).\n", m_ServerChannel.GetTargetAddress()->ToString(), m_CurrentRetry, MAX_CONNECT_RETRIES);
	nextRetry = m_SystemTime + CONNECT_RETRY_INTERVAL;
	m_CurrentRetry++;
}

void Server::ShutDown()
{
	if (m_State == MODULE_DISCONNECTED) {
		return;
	}

	Disconnect();
	m_ServerChannel.Close();

	m_ReliableData.Free();
	m_UnreliableData.Free();
	m_VoiceData.Free();
	m_UserMessages.Free();
	m_ClientData.Free();
	m_DemoData.Free();
	m_Listener.Clear();
	m_System->Printf("Server module shutdown.\n");

	BaseSystemModule::ShutDown();
}

void Server::RunFrame(double time)
{
	BaseSystemModule::RunFrame(time);
	CheckConnection();

	if (IsDemoFile()) {
		m_DemoFile.ReadDemoPacket(&m_DemoData, &m_DemoInfo);
	}

	int packets = 0;
	NetPacket *packet;
	while ((packet = m_ServerChannel.GetPacket()))
	{
		if (packet->connectionless)
		{
			if (!ProcessConnectionlessMessage(&packet->address, &packet->data) && m_Proxy)
			{
				packet->data.Reset();
				m_Proxy->ProcessConnectionlessMessage(&packet->address, &packet->data);
			}
		}
		else
		{
			m_Instream = &packet->data;
			ProcessMessage(packet->seqnr);
		}

		m_ServerChannel.FreePacket(packet);

		if (++packets > 32) {
			break;
		}

		if (IsDemoFile()) {
			m_DemoFile.ReadDemoPacket(&m_DemoData, &m_DemoInfo);
		}
	}

	switch (m_ServerState)
	{
	case SERVER_CONNECTED:
		break;
	case SERVER_DISCONNECTED:
		CheckAutoRetry();
		break;
	case SERVER_CHALLENGING:
		Challenge();
		break;
	case SERVER_CONNECTING:
		SendConnectPacket();
		break;
	case SERVER_RUNNING:
	case SERVER_INTERMISSION:
		if (m_ServerChannel.IsReadyToSend()) {
			SendServerCommands();
		}
		break;
	default:
		m_System->Errorf("Server::RunFrame: not valid state.\n");
		break;
	}
}

bool Server::ProcessConnectionlessMessage(NetAddress *from, BitBuffer *stream)
{
	TokenLine params;

	char *firstToken = stream->ReadString();
	if (!params.SetLine(firstToken)) {
		m_System->Printf("WARNING! Server::ProcessConnectionlessMessage: message too long.\n");
		return true;
	}

	if (!params.CountToken()) {
		m_System->Printf("WARNING! Invalid packet from %s.\n", from->ToString());
		return true;
	}

	char *c = params.GetToken(0);
	switch (c[0])
	{
	case S2C_CONNECTION:
		AcceptConnection();
		break;
	case S2C_CHALLENGE:
		AcceptChallenge(params.GetLine());
		break;
	case A2A_PRINT:
		m_System->Printf(">%s\n", params.GetLine() + 1);
		break;
	case S2C_REJECT_BADPASSWORD:
		AcceptBadPassword();
		break;
	case S2C_REJECT:
		AcceptRejection(params.GetLine() + 1);
		break;
	case S2A_PROXY_LISTEN:
		AcceptChallenge(params.GetLine());
		break;
	case S2A_PROXY_REDIRECT:
		AcceptRedirect(params.GetLine() + 1);
		break;
	case S2A_INFO:
		ParseInfo(stream, false);
		break;
	case S2A_INFO_DETAILED:
		ParseInfo(stream, true);
		break;
	default:
		m_System->DPrintf("Server::ProcessConnectionlessServerMessage: unknown command \"%c\"\n", *c);
		return false;
	}

	return true;
}

void Server::ProcessMessage(unsigned int seqNr)
{
	if (!IsConnected()) {
		return;
	}

	m_Frame.seqnr = seqNr;

	while (m_ServerState != SERVER_DISCONNECTED)
	{
		if (m_Instream->IsOverflowed()) {
			m_System->Printf("WARNING! Server::ProcessMessage: packet read overflow.\n");
			break;
		}

		int cmd = m_Instream->ReadByte();
		if (cmd == -1) {
			break;
		}

// With `HLTV_FIXES` enabled meaning of `svc_startofusermessages` changed a bit: now it is an id of the first user message
#ifdef HLTV_FIXES
		if (cmd >= svc_startofusermessages)
#else // HLTV_FIXES
		if (cmd > svc_startofusermessages)
#endif // HLTV_FIXES
		{
			if (!ParseUserMessage(cmd)) {
				break;
			}

			continue;
		}

		if (!m_ClientFuncs[cmd].func) {
			m_System->Printf("TODO! Server::ProcessMessage: missing parsing routine for %s.\n", m_ClientFuncs[cmd].pszname);
			return;
		}

		(this->*m_ClientFuncs[cmd].func)();
	}

	if (m_Frame.entitynum)
	{
		if (!m_ReliableData.IsOverflowed()) {
			m_Frame.reliableDataSize = m_ReliableData.CurrentSize();
		}

		if (!m_UnreliableData.IsOverflowed()) {
			m_Frame.unreliableDataSize = m_UnreliableData.CurrentSize();
		}

		if (!m_VoiceData.IsOverflowed()) {
			m_Frame.voiceDataSize = m_VoiceData.CurrentSize();
		}

		if (!m_ClientData.IsOverflowed()) {
			m_Frame.clientDataSize = m_ClientData.CurrentSize();
		}

		if (!m_UserMessages.IsOverflowed()) {
			m_Frame.userMessagesSize = m_UserMessages.CurrentSize();
		}

		if (!m_DemoData.IsOverflowed() && !m_ForceHLTV) {
			m_Frame.demoDataSize = m_DemoData.CurrentSize();
		}

		if (!m_IsHLTV && !m_ForceHLTV) {
			m_Frame.demoInfo = &m_DemoInfo;
		}

		if (m_ForceHLTV) {
			ProcessEntityUpdate();
		}

		m_SeqNrMap[seqNr & 0xFF] = m_World->AddFrame(&m_Frame);
		m_validSequence = seqNr;
		ClearFrame(true);
	}
	else
	{
		ClearFrame(m_World->IsActive() ? false : true);
	}
}

void Server::AcceptChallenge(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (m_ServerState != SERVER_CHALLENGING) {
		m_System->Printf("WARNING! Server::AcceptChallenge: ignoring unwanted challenge return.\n");
		return;
	}

	if (params.CountToken() < 3) {
		m_System->Printf("WARNING! Server::AcceptChallenge: invalid challenge format.\n");
		return;
	}

	m_ChallengeNumber = Q_atoi(params.GetToken(1));
	m_AuthProtocol = Q_atoi(params.GetToken(2));
	m_AuthProtocol = 2;

	if (m_AuthProtocol == 2)
	{
		m_System->Printf("Get challenge (HASHEDCDKEY)\n");
		SetState(SERVER_CONNECTING);
	}
	else
	{
		m_System->Printf("Invalid auth type\n");
	}

	m_CurrentRetry = 0;
}

bool Server::IsVoiceBlocking()
{
	return m_IsVoiceBlocking;
}

void Server::SetVoiceBlocking(bool state)
{
	m_IsVoiceBlocking = state;
}

void Server::SetRate(int rate)
{
	m_Rate = clamp(rate, MIN_SERVER_RATE, MAX_SERVER_RATE);
	SetUserInfo("rate", COM_VarArgs("%i", m_Rate));
}

void Server::SetUpdateRate(int updaterate)
{
	m_UpdateRate = clamp(updaterate, MIN_SERVER_UPDATERATE, MAX_SERVER_UPDATERATE);
	SetUserInfo("cl_updaterate", COM_VarArgs("%i", m_UpdateRate));
}

void Server::SetState(ServerState newState)
{
	if (newState == m_ServerState)
		return;

	bool stateError = false;
	switch (newState)
	{
	case SERVER_INITIALIZING:
		break;
	case SERVER_DISCONNECTED:
	{
		if (m_ServerState != SERVER_INITIALIZING) {
			m_ServerChannel.Close();
		}

		break;
	}
	case SERVER_CHALLENGING:
	{
		if (m_ServerState != SERVER_DISCONNECTED) {
			stateError = true;
		}
		m_ServerChannel.SetUpdateRate(1);
		break;
	}
	case SERVER_AUTHENTICATING:
	{
		if (m_ServerState != SERVER_CHALLENGING) {
			stateError = true;
		}
		break;
	}
	case SERVER_CONNECTING:
	{
		if (m_ServerState != SERVER_AUTHENTICATING
			&& m_ServerState != SERVER_CHALLENGING) {
			stateError = true;
		}
		break;
	}
	case SERVER_CONNECTED:
	{
		if (m_ServerState != SERVER_CONNECTING
			&& m_ServerState != SERVER_RUNNING
			&& m_ServerState != SERVER_INTERMISSION && !IsDemoFile()) {
			stateError = true;
		}

		m_ServerChannel.SetTimeOut(60);
		m_ServerChannel.SetUpdateRate(30);
		m_ServerChannel.SetKeepAlive(true);
		break;
	}
	case SERVER_RUNNING:
	{
		if (m_ServerState != SERVER_CONNECTED
			&& m_ServerState != SERVER_INTERMISSION) {
			stateError = true;
		}

		m_ServerChannel.SetKeepAlive(false);
		m_ServerChannel.SetTimeOut(30);
		break;
	}
	case SERVER_INTERMISSION:
	{
		if (m_ServerState != SERVER_RUNNING) {
			stateError = true;
		}
		break;
	}
	default:
		stateError = true;
		break;
	}

	if (stateError)
	{
		m_System->Errorf("Server::SetState: not valid m_ServerState (%i -> %i).\n", m_ServerState, newState);
		return;
	}

	m_ServerState = newState;
}

void Server::SendConnectPacket()
{
	const int MAX_CONNECT_RETRIES = 3;
	const float CONNECT_RETRY_INTERVAL = 4.0;

	char data[2048];
	char buffer[1024];
	InfoString protinfo(1024);

	static float nextRetry = 0;
	if (!m_CurrentRetry) {
		nextRetry = 0;
		m_CurrentRetry++;
	}

	if (nextRetry >= m_SystemTime) {
		return;
	}

	if (m_CurrentRetry > MAX_CONNECT_RETRIES)
	{
		m_System->Printf("WARNING! Server::SendConnectPacket: Timeout \n");
		SetState(SERVER_DISCONNECTED);
		ScheduleAutoRetry();
		return;
	}

	if (m_AuthProtocol == 2)
	{
		if (!m_CDKey[0])
		{
			m_System->Printf("Invalid CD Key\n");
			SetState(SERVER_DISCONNECTED);
			return;
		}

		Q_strlcpy(buffer, MD5_GetCDKeyHash(m_CDKey));
	}

	protinfo.SetValueForKey("prot", COM_VarArgs("%i", m_AuthProtocol));
	protinfo.SetValueForKey("unique", COM_VarArgs("%i", -1));
	protinfo.SetValueForKey("raw", buffer);

	if (m_AuthProtocol != 2) {
		protinfo.SetValueForKey("cdkey", MD5_GetCDKeyHash(m_CDKey));
	}

	Q_snprintf(data, sizeof(data), "%c%c%c%cconnect %i %i \"%s\" \"%s\"\n", 0xFF, 0xFF, 0xFF, 0xFF, m_Protocol, m_ChallengeNumber, protinfo.GetString(), m_UserInfo.GetString());
	m_ServerSocket->SendPacket(m_ServerChannel.GetTargetAddress(), data, Q_strlen(data));

	m_System->Printf("Connecting to %s (%i/%i).\n", m_ServerChannel.GetTargetAddress()->ToString(), m_CurrentRetry, MAX_CONNECT_RETRIES);
	nextRetry = m_SystemTime + CONNECT_RETRY_INTERVAL;
	m_CurrentRetry++;
}

void Server::AcceptBadPassword()
{
	if (m_ServerState != SERVER_CONNECTING) {
		m_System->Printf("WARNING! Server::AcceptBadPassword: ignoring unwanted bad password return.\n");
		return;
	}

	m_System->Printf("Bad server password.\n");
	SetState(SERVER_DISCONNECTED);
}

void Server::AcceptRejection(char *reason)
{
	if (m_ServerState != SERVER_CONNECTING) {
		m_System->Printf("WARNING! Server::AcceptRejection: ignoring unwanted rejection message.\n");
		return;
	}

	m_System->Printf("Connection rejected: %s\n", reason);
	SetState(SERVER_DISCONNECTED);
	ScheduleAutoRetry();
}

void Server::AcceptConnection()
{
	if (m_ServerState != SERVER_CONNECTING) {
		m_System->Printf("WARNING! Server::AcceptConnection: ignoring unwanted connection return.\n");
		return;
	}

	SetState(SERVER_CONNECTED);
	m_ServerChannel.SetConnected(true);

	m_System->DPrintf("Connection accepted by %s\n", m_ServerAddress.ToString());
	SendStringCommand("new");

	BaseSystemModule::FireSignal(1, &m_ServerAddress);
}

void Server::AcceptRedirect(char *toAddress)
{
	if (m_ServerState != SERVER_CONNECTING) {
		m_System->Printf("WARNING! Server::AcceptRedirect: ignoring unwanted redirect message.\n");
		return;
	}

	INetwork *newtwork = m_ServerSocket->GetNetwork();
	newtwork->ResolveAddress(toAddress, &m_ServerAddress);
	m_System->Printf("Redirected to %s\n", m_ServerAddress.ToString());

	SetState(SERVER_DISCONNECTED);
	BaseSystemModule::FireSignal(3, &m_ServerAddress);
	Retry();
}

void Server::ParsePrint()
{
	char *string = m_Instream->ReadString();
	if (*string < 32) {
		string++;
	}

	m_System->Printf(">%s\n", string);
}

void Server::ParseVoiceInit()
{
	unsigned char *start = m_Instream->CurrentByte();
	char *codec = m_Instream->ReadString();
	unsigned char quality = m_Instream->ReadByte();

	int length = Q_strlen(codec);
	if (m_ServerState == SERVER_CONNECTED) {
		m_World->AddSignonData(svc_voiceinit, start, length + 2);
	}
	else if (m_ServerState == SERVER_RUNNING || m_ServerState == SERVER_INTERMISSION)
	{
		m_ReliableData.WriteByte(svc_voiceinit);
		m_ReliableData.WriteString(codec);
		m_ReliableData.WriteByte(quality);
	}
	else {
		m_System->Errorf("Server::ParseVoiceInit: unexpected server state.\n");
	}

	m_World->SetVoiceEnabled(codec[0] ? true : false);
}

void Server::ParseVoiceData()
{
	int index = m_Instream->ReadByte();
	int dataLength = m_Instream->ReadShort();

	if (!m_IsVoiceBlocking && m_ServerState >= SERVER_RUNNING)
	{
		m_VoiceData.WriteByte(svc_voicedata);
		m_VoiceData.WriteByte(index);
		m_VoiceData.WriteShort(dataLength);
		m_VoiceData.WriteBuf(m_Instream->CurrentByte(), dataLength);
	}

	m_Instream->SkipBytes(dataLength);
}

void Server::ParseNop()
{
}

void Server::ParseBad()
{
	m_System->Printf("WARNING! Server::ParseBad: illegal server message.\n");
	m_Instream->m_Overflowed = true;
}

void Server::ParseServerinfo()
{
	if (IsDemoFile() && m_ServerState == SERVER_RUNNING) {
		Disconnect();
		return;
	}

	m_System->DPrintf("Serverinfo packet received.\n");

	int protocol = m_Instream->ReadLong();
	if (protocol != m_Protocol)
	{
		m_System->Printf("WARNING! Server returned protocol version %i, not %i\n", protocol, m_Protocol);
		Disconnect();
		return;
	}

	m_ServerCount = m_Instream->ReadLong();
	m_World->NewGame(m_ServerCount);
	m_ServerCRC = m_Instream->ReadLong();

	unsigned char clientdllmd5[16];
	m_Instream->ReadBuf(sizeof(clientdllmd5), clientdllmd5);

	int maxclients = m_Instream->ReadByte();
	int playernum = m_Instream->ReadByte();
	m_ClientPlayerNum = playernum;

	COM_UnMunge3((byte *)&m_ServerCRC, sizeof(m_ServerCRC), (-1 - playernum) & 0xFF);
	if (m_ForceHLTV)
	{
		if (maxclients == 1) {
			maxclients = 2;
		}

		playernum = maxclients - 1;
	}

	char gamedir[MAX_PATH];
	char levelname[MAX_PATH];
	int gametype = m_Instream->ReadByte();

	Q_strlcpy(gamedir, m_Instream->ReadString());
	Q_strlcpy(m_HostName, m_Instream->ReadString());
	Q_strlcpy(levelname, m_Instream->ReadString());

	m_Instream->SkipString();

	if (m_Instream->ReadByte())
	{
		int length = m_Instream->ReadByte();
		m_Instream->SkipBytes(length);
		m_Instream->SkipBytes(16);
	}

	m_World->SetServerInfo(protocol, m_ServerCRC, clientdllmd5, maxclients, playernum, gametype, gamedir, m_HostName, levelname);

	if (!IsDemoFile()) {
		SetGameDirectory("valve", gamedir);
	}

	m_World->SetHLTV((m_IsHLTV || m_ForceHLTV) ? true : false);

	SetState(SERVER_CONNECTED);
	SendStringCommand("sendres");
}

void Server::ParseDeltaDescription()
{
	m_World->ParseDeltaDescription(m_Instream);
}

void Server::ParseDecalName()
{
	m_ReliableData.WriteByte(svc_decalname);
	m_ReliableData.WriteByte(m_Instream->ReadByte());
	m_ReliableData.WriteString(m_Instream->ReadString());
}

void Server::ParseMoveVars()
{
	movevars_t movevars;
	movevars.gravity           = m_Instream->ReadFloat();
	movevars.stopspeed         = m_Instream->ReadFloat();
	movevars.maxspeed          = m_Instream->ReadFloat();
	movevars.spectatormaxspeed = m_Instream->ReadFloat();
	movevars.accelerate        = m_Instream->ReadFloat();
	movevars.airaccelerate     = m_Instream->ReadFloat();
	movevars.wateraccelerate   = m_Instream->ReadFloat();
	movevars.friction          = m_Instream->ReadFloat();
	movevars.edgefriction      = m_Instream->ReadFloat();
	movevars.waterfriction     = m_Instream->ReadFloat();
	movevars.entgravity        = m_Instream->ReadFloat();
	movevars.bounce            = m_Instream->ReadFloat();
	movevars.stepsize          = m_Instream->ReadFloat();
	movevars.maxvelocity       = m_Instream->ReadFloat();
	movevars.zmax              = m_Instream->ReadFloat();
	movevars.waveHeight        = m_Instream->ReadFloat();
	movevars.footsteps         = m_Instream->ReadByte ();
	movevars.rollangle         = m_Instream->ReadFloat();
	movevars.rollspeed         = m_Instream->ReadFloat();
	movevars.skycolor_r        = m_Instream->ReadFloat();
	movevars.skycolor_g        = m_Instream->ReadFloat();
	movevars.skycolor_b        = m_Instream->ReadFloat();
	movevars.skyvec_x          = m_Instream->ReadFloat();
	movevars.skyvec_y          = m_Instream->ReadFloat();
	movevars.skyvec_z          = m_Instream->ReadFloat();

	Q_strlcpy(movevars.skyName, m_Instream->ReadString());
	m_World->SetMoveVars(&movevars);
}

void Server::ParseCDTrack()
{
	int cdtrack = m_Instream->ReadByte();
	int looptrack = m_Instream->ReadByte();

	m_World->SetCDInfo(cdtrack, looptrack);
}

void Server::ParseRestore()
{
	m_Instream->SkipString();

	int mapCount = m_Instream->ReadByte();
	for (int i = 0; i < mapCount; i++) {
		m_Instream->SkipString();
	}
}

void Server::ParseSetView()
{
	if (m_ServerState == SERVER_CONNECTED)
	{
		m_World->SetViewEntity(m_Instream->ReadShort());
	}
	else if (m_ServerState == SERVER_RUNNING || m_ServerState == SERVER_INTERMISSION)
	{
		m_ReliableData.WriteByte(svc_setview);
		m_ReliableData.WriteBuf(m_Instream->CurrentByte(), 2);
		m_Instream->SkipBytes(2);
	}
	else
	{
		m_System->Errorf("Server::ParseSetView: unexpected server state.\n");
	}
}

void Server::ParseResourceRequest()
{
	if (m_ServerState != SERVER_CONNECTED) {
		m_System->Printf("WARNING! Server::ParseResourceRequest: custom resource request not valid - not connected\n");
		return;
	}

	int arg = m_Instream->ReadLong();
	if (arg != m_ServerCount) {
		m_System->Printf("WARNING! Server::ParseResourceRequest: from old level\n");
		return;
	}

	int nStartIndex = m_Instream->ReadLong();
	if (nStartIndex) {
		m_System->Printf("WARNING! Server::ParseResourceRequest: custom resource list request out of range\n");
		return;
	}

	m_ServerChannel.m_reliableStream.WriteByte(clc_resourcelist);
	m_ServerChannel.m_reliableStream.WriteShort(0);
}

void Server::ParseCrosshairAngle()
{
	m_ReliableData.WriteByte(svc_crosshairangle);
	m_ReliableData.WriteChar(m_Instream->ReadChar());
	m_ReliableData.WriteChar(m_Instream->ReadChar());
}

void Server::ParseSoundFade()
{
	m_ReliableData.WriteByte(svc_soundfade);
	m_ReliableData.WriteByte(m_Instream->ReadByte());
	m_ReliableData.WriteByte(m_Instream->ReadByte());
	m_ReliableData.WriteByte(m_Instream->ReadByte());
	m_ReliableData.WriteByte(m_Instream->ReadByte());
}

void Server::ParseNewUserMsg()
{
	int iMsg = m_Instream->ReadByte();
	int iSize = m_Instream->ReadByte();
	if (iSize == 255) {
		iSize = -1;
	}

	char name[17];
	m_Instream->ReadBuf(sizeof(name) - 1, name);
	name[sizeof(name) - 1] = '\0';

	m_System->DPrintf("Adding user message:%s(%i).\n", name, iMsg);
	m_World->AddUserMessage(iMsg, iSize, name);
}

void Server::ParseWeaponAnim()
{
	m_Instream->SkipBytes(2);
}

void Server::ParseStuffText()
{
	TokenLine cmdLine;
	if (!cmdLine.SetLine(m_Instream->ReadString())) {
		m_System->Printf("WARNING! Server::ParseStuffText: command too long.\n");
		return;
	}

	char *cmd = cmdLine.GetToken(0);
	if (!cmd) {
		m_System->Printf("WARNING! Server::ParseStuffText: command is NULL.\n");
		return;
	}

	if (!Q_stricmp(cmd, "fullserverinfo"))
	{
		char *infostring = cmdLine.GetToken(1);
		m_ServerInfo.SetString(infostring);
		m_World->SetServerInfoString(infostring);

		if (IsDemoFile())
		{
			m_IsGameServer = false;
		}
		else
		{
			NetAddress gameAddr;
			m_ServerSocket->GetNetwork()->ResolveAddress(m_ServerInfo.ValueForKey("proxy"), &gameAddr);

			if (gameAddr.IsValid())
			{
				m_IsGameServer = false;
				m_World->SetGameServerAddress(&gameAddr);
			}
			else
			{
				m_IsGameServer = true;
				m_World->SetGameServerAddress(&m_ServerAddress);
				m_World->GetServerInfoString()->SetValueForKey("proxy", m_ServerAddress.ToString());
			}
		}

		BaseSystemModule::FireSignal(4);
		return;
	}
	else if (!Q_stricmp(cmd, "reconnect"))
	{
		if (IsDemoFile())
		{
			Disconnect();
		}
		else if (m_DelayReconnect)
		{
			BaseSystemModule::FireSignal(5);
			m_System->DPrintf("Delaying reconnect to broadcast complete game.\n");
		}
		else
		{
			Reconnect();
		}

		m_IsPaused = false;
		return;
	}
	else if (!Q_stricmp(cmd, "connect"))
	{
		if (m_ServerSocket && m_World)
		{
			NetAddress to;
			m_ServerSocket->GetNetwork()->ResolveAddress(cmdLine.GetToken(1), &to);

			Connect(m_World, &to, m_ServerSocket);
			BaseSystemModule::FireSignal(3);
		}
		else
		{
			m_System->Printf("ERROR! Server::ParseStuffText: received redirect while offline.\n");
			Disconnect();
		}

		return;
	}
	else if (!Q_stricmp(cmd, "rate")
		|| !Q_stricmp(cmd, "cl_updaterate")
		|| !Q_stricmp(cmd, "ex_interp")
		|| !Q_stricmp(cmd, "cl_cmdrate")
		|| !Q_stricmp(cmd, "cl_cmdbackup"))
	{
		return;
	}

	if (m_ServerState == SERVER_CONNECTED)
	{
		m_World->AddSignonData(svc_stufftext, (unsigned char *)cmdLine.GetLine(), Q_strlen(cmdLine.GetLine()) + 1);
	}
	else if (m_ServerState == SERVER_RUNNING || m_ServerState == SERVER_INTERMISSION)
	{
		m_UnreliableData.WriteByte(svc_stufftext);
		m_UnreliableData.WriteString(cmdLine.GetLine());
	}
	else
	{
		m_System->Errorf("Server::ParseStuffText: unexpected state.\n");
	}
}

void Server::ParseUpdateUserInfo()
{
	unsigned char *start = m_Instream->CurrentByte();
	int playernumber = m_Instream->ReadByte();
	int userID = m_Instream->ReadLong();
	char *infostring = m_Instream->ReadString();

	char hashedcdkey[16];
	m_Instream->ReadBuf(sizeof(hashedcdkey), hashedcdkey);
	m_World->UpdatePlayer(playernumber, userID, infostring, hashedcdkey);

	if (m_ServerState == SERVER_CONNECTED)
	{
		/* do nothing */
	}
	else if (m_ServerState == SERVER_RUNNING || m_ServerState == SERVER_INTERMISSION)
	{
		m_ReliableData.WriteByte(svc_updateuserinfo);
		m_ReliableData.WriteBuf(start, m_Instream->CurrentByte() - start);
	}
	else
	{
		m_System->Errorf("Server::ParseUpdateUserInfo: unexpected state.\n");
	}
}

void Server::ParseResourceList()
{
	m_Instream->StartBitMode();
	int total = m_Instream->ReadBits(12);
	for (int i = 0; i < total; i++)
	{
		resource_t resource;
		Q_memset(&resource, 0, sizeof(resource));

		resource.type = (resourcetype_t)m_Instream->ReadBits(4);

		Q_strlcpy(resource.szFileName, m_Instream->ReadBitString());

		resource.nIndex = m_Instream->ReadBits(12);
		resource.nDownloadSize = m_Instream->ReadBits(24);
		resource.ucFlags = m_Instream->ReadBits(3);

		if (resource.ucFlags & RES_CUSTOM) {
			m_Instream->ReadBitData(resource.rgucMD5_hash, sizeof(resource.rgucMD5_hash));
		}

		if (m_Instream->ReadBit()) {
			m_Instream->ReadBitData(resource.rguc_reserved, sizeof(resource.rguc_reserved));
		}

		if (Q_strncmp(resource.szFileName, "gfx/temp/", 9) != 0) {
			m_World->AddResource(&resource);
		}
	}

	m_System->Printf("Added %i resources.\n", total);

	if (m_Instream->ReadBit())
	{
		if (!IsDemoFile()) {
			m_System->Printf("WARNING: HLTV proxy doesn't support file consistency check.\nWARNING: Disable mp_consistency on server.");
		}

		while (m_Instream->ReadBit())
		{
			if (m_Instream->ReadBit())
				m_Instream->ReadBits(5);
			else
				m_Instream->ReadBits(10);
		}
	}

	m_Instream->EndBitMode();

	CRC32_t mungebuffer = m_ServerCRC;
	COM_Munge2((byte *)&mungebuffer, sizeof(mungebuffer), (-1 - m_ServerCount) & 0xFF);
	SendStringCommand(COM_VarArgs("spawn %i %i", m_ServerCount, mungebuffer));
}

static const int TE_LENGTH[TE_MAX] =
{
	24, 20,  6, 11,  6, 10, 12, 17, 16,  6,  6,  6,  8, -1,  9, 19,
	-2, 10, 16, 24, 24, 24, 10, 11, 16, 19, -2, 12, 16, -1, 19, 17,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2,  2, 10, 14, 12, 14,  9,  5, 17, 13, 24,  9, 17,  7,
	10, 19, 19, 12,  7,  7,  9, 16, 18,  6, 10, 13,  7,  1, 18, 15,
};

void Server::ParseTempEntity()
{
	unsigned char *start = m_Instream->CurrentByte();
	int type = m_Instream->ReadByte();

	if (type >= TE_MAX) {
		m_System->Errorf("Server::ParseTempEntity:type >= TE_MAX.\n");
		return;
	}

	int length = TE_LENGTH[type];
	if (length == -2) {
		m_System->Errorf("Server::ParseTempEntity:invalid type %i.\n", type);
		return;
	}

	if (length == -1)
	{
		if (type == TE_BSPDECAL)
		{
			m_Instream->SkipBytes(8);
			if (m_Instream->ReadWord())
			{
				m_Instream->SkipBytes(2);
				length = 12;
			}
			else
				length = 10;
		}
		else if (type == TE_TEXTMESSAGE)
		{
			m_Instream->SkipBytes(5);
			if (m_Instream->ReadByte() == 2)
			{
				m_Instream->SkipBytes(2);
				length = 8;
			}
			else
				length = 6;

			m_Instream->SkipBytes(14);
			length += m_Instream->SkipString() + 14;
		}
		else
		{
			m_System->Printf("ERROR! Server::ParseTempEntity: unknown type with dynamic length.\n");
		}
	}
	else
	{
		m_Instream->SkipBytes(length);
	}

	if (m_ServerState == SERVER_CONNECTED)
	{
		m_World->AddSignonData(svc_temp_entity, start, length + 1);
	}
	else if (m_ServerState == SERVER_RUNNING || m_ServerState == SERVER_INTERMISSION)
	{
		m_UnreliableData.WriteByte(svc_temp_entity);
		m_UnreliableData.WriteBuf(start, length + 1);
	}
	else {
		m_System->Errorf("Server::ParseTempEntity: unexpected server state.\n");
	}
}

void Server::ParseBaseline()
{
	m_World->ParseBaseline(m_Instream);
}

void Server::ParseVersion()
{
	int protocol = m_Instream->ReadLong();
	if (protocol != m_Protocol) {
		m_System->Printf("Server::ParseVersion: Server is protocol %i instead of %i\n", protocol, PROTOCOL_VERSION);
	}
}

void Server::ParseTime()
{
	m_Time = m_Instream->ReadFloat();
	m_Frame.time = m_Time;
}

void Server::ParseLightStyle()
{
	int index = m_Instream->ReadByte();
	char *style = m_Instream->ReadString();

	if (m_ServerState == SERVER_CONNECTED)
	{
		m_World->AddLightStyle(index, style);
	}
	else if (m_ServerState == SERVER_RUNNING || m_ServerState == SERVER_INTERMISSION)
	{
		m_UnreliableData.WriteByte(svc_lightstyle);
		m_UnreliableData.WriteByte(index);
		m_UnreliableData.WriteString(style);
	}
	else
	{
		m_System->Errorf("Server::ParseLightStyle: unexpected server state.\n");
	}
}

void Server::ParseSetAngle()
{
	if (m_ServerState == SERVER_CONNECTED)
	{
		m_World->AddSignonData(svc_setangle, m_Instream->CurrentByte(), 6);
	}
	else if (m_ServerState == SERVER_RUNNING || m_ServerState == SERVER_INTERMISSION)
	{
		m_UnreliableData.WriteByte(svc_setangle);
		m_UnreliableData.WriteBuf(m_Instream->CurrentByte(), 6);
	}
	else
	{
		m_System->Errorf("Server::ParseSetAngle: unexpected server state.\n");
	}

	m_Instream->SkipBytes(6);
}

void Server::ParseAddAngle()
{

	if (m_ServerState == SERVER_CONNECTED)
	{
		m_World->AddSignonData(svc_addangle, m_Instream->CurrentByte(), 2);
	}
	else if (m_ServerState == SERVER_RUNNING || m_ServerState == SERVER_INTERMISSION)
	{
		m_UnreliableData.WriteByte(svc_addangle);
		m_UnreliableData.WriteBuf(m_Instream->CurrentByte(), 2);
	}
	else
	{
		m_System->Errorf("Server::ParseAddAngle: unexpected server state.\n");
	}

	m_Instream->SkipBytes(2);
}

void Server::ParseChoke()
{
}

void Server::ParseDisconnect()
{
	char *msg = m_Instream->ReadString();
	if (msg && *msg) {
		m_System->Printf("Dropped from %s (%s).\n", m_ServerChannel.GetTargetAddress()->ToString(), msg);
	}
	else {
		m_System->Printf("Dropped from %s.\n", m_ServerChannel.GetTargetAddress()->ToString());
	}

	if (IsConnected()) {
		ScheduleAutoRetry();
	}

	Disconnect();
}

void Server::ParseFileTransferFailed()
{
	char *name = m_Instream->ReadString();
	if (!name || !name[0]) {
		m_System->Printf("WARNING! Server::ParseFileTransferFailed: empty filename.\n");
		return;
	}

	m_System->Printf("WARNING! Server::ParseFileTransferFailed: Downloading \"%s\" failed.\n", name);
}

void Server::ParseSignonNum()
{
	int num = m_Instream->ReadByte();
	m_System->DPrintf("Received signon (%i).\n", num);

	if (IsDemoFile())
	{
		m_World->SetServerInfo(&m_DemoFile.m_ServerInfo);
	}
	else
	{
		m_ServerChannel.OutOfBandPrintf("details");

		m_World->UpdateServerInfo();
		SendStringCommand("sendents");
		SendStringCommand("spectate");
		SendStringCommand(COM_VarArgs("VModEnable %d", m_IsVoiceBlocking ? 0 : 1));

		char string[128];
		Q_strlcpy(string, "vban");

		for (int i = 0; i < MAX_CLIENTS; i++) {
			Q_strlcat(string, " 0");
		}

		SendStringCommand(string);
	}

	SetState(SERVER_RUNNING);

#ifdef HLTV_FIXES
	if (m_PendingIntermission && m_ServerState == SERVER_RUNNING)
	{
		m_PendingIntermission = false;
		ParseIntermission();
	}
#endif
}

void Server::ParseCustomization()
{
	int index = m_Instream->ReadByte();
	if (index < 0 || index >= MAX_CLIENTS)
	{
		m_System->DPrintf("Bogus player index (%i) during customization parsing.\n", index);
		m_Instream->m_Overflowed = true;
		return;
	}

	resource_t *resource = (resource_t *)Mem_ZeroMalloc(sizeof(resource_t));
	resource->type = (resourcetype_t)m_Instream->ReadByte();

	Q_strlcpy(resource->szFileName, m_Instream->ReadString());

	resource->nIndex = m_Instream->ReadShort();
	resource->nDownloadSize = m_Instream->ReadLong();
	resource->ucFlags = m_Instream->ReadByte() & (~RES_WASMISSING);
	resource->pNext = nullptr;

	if (resource->ucFlags & RES_CUSTOM) {
		m_Instream->ReadBuf(sizeof(resource->rgucMD5_hash), resource->rgucMD5_hash);
	}

	resource->playernum = index;
	m_System->DPrintf("Ignoring player customization (%s).\n", resource->szFileName);
	Mem_Free(resource);
}

void Server::ClearFrame(bool completely)
{
	if (completely)
	{
		m_UserMessages.FastClear();
		m_ReliableData.FastClear();

		m_Frame.reliableDataSize = 0;
		m_Frame.userMessagesSize = 0;
	}

	m_UnreliableData.FastClear();
	m_VoiceData.FastClear();
	m_ClientData.FastClear();
	m_DemoData.FastClear();

	m_Frame.time = 0;
	m_Frame.entities = 0;
	m_Frame.entitynum = 0;
	m_Frame.entitiesSize = 0;
	m_Frame.events = 0;
	m_Frame.eventnum = 0;
	m_Frame.eventsSize = 0;
	m_Frame.demoInfo = 0;

	Q_memset(&m_DemoInfo, 0, sizeof(m_DemoInfo));
	m_Frame.unreliableDataSize = 0;
	m_Frame.voiceDataSize = 0;
	m_Frame.clientDataSize = 0;
	m_Frame.demoDataSize = 0;
}

bool Server::ParseUserMessage(int cmd)
{
	UserMsg *usermsg = m_World->GetUserMsg(cmd);
	if (!usermsg) {
		m_System->Printf("WARNING! Server::ParseUserMessage: unknown user message (%i).\n", cmd);
		return false;
	}

	unsigned char *start = m_Instream->CurrentByte();

	int length = 0;
	int msgSize = usermsg->iSize;

	if (msgSize == -1) {
		msgSize = m_Instream->ReadByte();
		length = msgSize + 1;
	}
	else
		length = msgSize;

	if (msgSize > MAX_USER_MSG_DATA) {
		m_System->Printf("WARNING! Server::ParseUserMessage: User Msg %d sent too much data (%i bytes)\n", cmd, msgSize);
		return false;
	}

	m_Instream->SkipBytes(msgSize);

	if (m_ServerState == SERVER_CONNECTED)
	{
		m_World->AddSignonData(cmd, start, length);
	}
	else if (m_ServerState == SERVER_RUNNING || m_ServerState == SERVER_INTERMISSION)
	{
		m_UserMessages.WriteByte(cmd);
		m_UserMessages.WriteBuf(start, length);
	}
	else
	{
		m_System->Printf("WARNING! Server::ParseUserMessage: unexpected server state.\n");
	}

	if (!Q_strcmp(usermsg->szName, "SayText"))
	{
		m_System->Printf("%s\n", start + 2);
	}
	else if (!Q_strcmp(usermsg->szName, "TextMsg"))
	{
		m_System->DPrintf("%s\n", start + 2);
	}
	else if (!Q_strcmp(usermsg->szName, "ReqState"))
	{
		char cmdString[32];
		Q_snprintf(cmdString, sizeof(cmdString), "VModEnable %d", m_IsVoiceBlocking == 0);
		SendStringCommand(cmdString);

		char string[128];
		Q_strlcpy(string, "vban");

		for (int i = 0; i < MAX_CLIENTS; i++) {
			Q_strlcat(string, " 0");
		}

		SendStringCommand(string);
	}

	return true;
}

void Server::ParsePacketEntities()
{
	int entnum = m_Instream->ReadShort();
	if (entnum > MAX_PACKET_ENTITIES) {
		m_System->Errorf("ERROR! Server::ParsePacketEntities: entnum > MAX_PACKET_ENTITIES.\n");
		m_Instream->m_Overflowed = true;
		return;
	}

	m_Frame.delta = 0;
	m_Frame.entitynum = entnum;
	m_Frame.entitiesSize = sizeof(entity_state_t) * entnum;
	m_Frame.entities = m_EntityBuffer;

	Q_memset(m_EntityBuffer, 0, m_Frame.entitiesSize);
	m_World->UncompressEntitiesFromStream(&m_Frame, m_Instream);
}

void Server::ParseDeltaPacketEntities()
{
	int entnum = m_Instream->ReadShort();
	if (entnum > MAX_PACKET_ENTITIES) {
		m_System->Errorf("Server::ParseDeltaPacketEntities: entnum > MAX_PACKET_ENTITIES.\n");
	}

	m_Frame.delta = 0;
	m_Frame.entitynum = entnum;
	m_Frame.entitiesSize = sizeof(entity_state_t) * entnum;
	m_Frame.entities = m_EntityBuffer;
	Q_memset(m_EntityBuffer, 0, m_Frame.entitiesSize);

	int from = m_Instream->ReadByte();
	if (!m_World->UncompressEntitiesFromStream(&m_Frame, m_Instream, m_SeqNrMap[from])) {
		m_Instream->m_Overflowed = true;
	}
}

void Server::SendServerCommands()
{
	if (m_validSequence && m_ServerState >= SERVER_RUNNING)
	{
		m_ServerChannel.m_unreliableStream.WriteByte(clc_delta);
		m_ServerChannel.m_unreliableStream.WriteByte(m_validSequence & 0xFF);
	}

	m_ServerChannel.TransmitOutgoing();
}

void Server::ParseSound()
{
	m_Instream->StartBitMode();

	vec3_t pos;
	unsigned char *start = this->m_Instream->CurrentByte();
	int field_mask = m_Instream->ReadBits(9);

	if (field_mask & SND_FL_VOLUME) {
		m_Instream->ReadBits(8);
	}

	if (field_mask & SND_FL_ATTENUATION) {
		m_Instream->ReadBits(8);
	}

	m_Instream->ReadBits(22);

	if (field_mask & SND_FL_LARGE_INDEX) {
		m_Instream->ReadBits(8);
	}

	m_Instream->ReadBitVec3Coord(pos);

	if (field_mask & SND_FL_PITCH) {
		m_Instream->ReadBits(8);
	}

	m_Instream->EndBitMode();

	m_UnreliableData.WriteByte(svc_sound);
	m_UnreliableData.WriteBuf(start, m_Instream->CurrentByte() - start);
}

void Server::ParseEvent()
{
	m_Instream->StartBitMode();

	m_Frame.events = m_Instream->CurrentByte();
	m_Frame.eventnum = m_Instream->ReadBits(5);

	for (unsigned int i = 0; i < m_Frame.eventnum; i++)
	{
		m_Instream->SkipBits(10);

		if (m_Instream->ReadBit())
			m_Instream->SkipBits(11);

		if (m_Instream->ReadBit())
			m_World->ParseEvent(m_Instream);

		if (m_Instream->ReadBit())
			m_Instream->SkipBits(16);
	}

	m_Instream->EndBitMode();
	m_Frame.eventsSize = m_Instream->CurrentByte() - m_Frame.events;
}

void Server::ParseStopSound()
{
	int entityIndex = m_Instream->ReadShort();

	m_UnreliableData.WriteByte(svc_stopsound);
	m_UnreliableData.WriteShort(entityIndex);
}

void Server::ParsePings()
{
	m_Instream->StartBitMode();

	unsigned char *start = m_Instream->CurrentByte();
	while (m_Instream->ReadBit()) {
		m_Instream->ReadBits(24);
	}

	m_Instream->EndBitMode();

	m_UnreliableData.WriteByte(svc_pings);
	m_UnreliableData.WriteBuf(start, m_Instream->CurrentByte() - start);
}

void Server::ParseEventReliable()
{
	m_Instream->StartBitMode();

	unsigned char *start = m_Instream->CurrentByte();

	m_Instream->SkipBits(10);
	m_World->ParseEvent(m_Instream);

	if (m_Instream->ReadBit()) {
		m_Instream->SkipBits(16);
	}

	m_Instream->EndBitMode();

	m_ReliableData.WriteByte(svc_event_reliable);
	m_ReliableData.WriteBuf(start, m_Instream->CurrentByte() - start);
}

void Server::ParseSpawnStaticSound()
{
	if (m_ServerState == SERVER_CONNECTED) {
		m_World->AddSignonData(svc_spawnstaticsound, m_Instream->CurrentByte(), 14);
	}
	else if (m_ServerState == SERVER_RUNNING || m_ServerState == SERVER_INTERMISSION)
	{
		m_UnreliableData.WriteByte(svc_spawnstaticsound);
		m_UnreliableData.WriteBuf(m_Instream->CurrentByte(), 14);
	}
	else {
		m_System->Errorf("Server::ParseSpawnStaticSound: unexpected server state.\n");
	}

	m_Instream->SkipBytes(14);
}

void Server::ParseRoomType()
{
	m_ReliableData.WriteByte(svc_roomtype);
	m_ReliableData.WriteShort(m_Instream->ReadShort());
}

void Server::ParseParticle()
{
	m_UnreliableData.WriteByte(svc_particle);
	m_UnreliableData.WriteBuf(m_Instream->CurrentByte(), 11);

	m_Instream->SkipBytes(11);
}

InfoString *Server::GetServerInfoString()
{
	return &m_ServerInfo;
}

int Server::GetRate()
{
	return m_Rate;
}

int Server::GetUpdateRate()
{
	return m_UpdateRate;
}

char *Server::GetHostName()
{
	return m_HostName;
}

int Server::GetProtocol()
{
	return m_Protocol;
}

bool Server::SetProtocol(int version)
{
	if (version != 46
		&& version != 47
		&& version != PROTOCOL_VERSION) {
		return false;
	}

	m_Protocol = version;
	return true;
}

void Server::SetPlayerName(char *newName)
{
	COM_RemoveEvilChars(newName);
	SetUserInfo("name", newName);
}

char *Server::GetPlayerName()
{
	return m_UserInfo.ValueForKey("name");
}

char *Server::GetType()
{
	return SERVER_INTERFACE_VERSION;
}

IWorld *Server::GetWorld()
{
	return m_World;
}

NetAddress *Server::GetAddress()
{
	if (IsDemoFile()) {
		return nullptr;
	}

	return &m_ServerAddress;
}

char *Server::GetDemoFileName()
{
	if (!IsDemoFile()) {
		return nullptr;
	}

	return m_DemoFile.GetFileName();
}

bool Server::GetAutoRetry()
{
	return m_AutoRetry;
}

float Server::GetPacketLoss()
{
	return m_ServerChannel.GetLoss();
}

char *Server::GetStatusLine()
{
	float in, out;
	static char string[256];

	switch (m_ServerState)
	{
	case SERVER_INITIALIZING:
		Q_snprintf(string, sizeof(string), "Initializing.\n");
		break;
	case SERVER_DISCONNECTED:
		Q_snprintf(string, sizeof(string), "Disconnected.\n");
		break;
	case SERVER_CHALLENGING:
		Q_snprintf(string, sizeof(string), "challenging %s.\n", m_ServerChannel.GetTargetAddress()->ToString());
		break;
	case SERVER_AUTHENTICATING:
		Q_snprintf(string, sizeof(string), "Authenticating.\n");
		break;
	case SERVER_CONNECTING:
		Q_snprintf(string, sizeof(string), "Connecting to %s.\n", m_ServerChannel.GetTargetAddress()->ToString());
		break;
	case SERVER_CONNECTED:
	case SERVER_RUNNING:
		m_ServerChannel.GetFlowStats(&in, &out);
		Q_snprintf(string, sizeof(string), "Connected to %s, Time %.0f, In %.2f, Out %.2f.\n", m_ServerChannel.GetTargetAddress()->ToString(), m_SystemTime - m_ServerChannel.m_connect_time, in, out);
		break;
	case SERVER_INTERMISSION:
		Q_snprintf(string, sizeof(string), "Intermission (%s).\n", m_ServerChannel.GetTargetAddress()->ToString());
		break;
	default:
		m_System->Errorf("Server::GetStatusLine: not valid state.\n");
		break;
	}

	return string;
}

void Server::ParseHLTV()
{
	unsigned char *start = m_Instream->CurrentByte();
	unsigned char cmd = m_Instream->ReadByte();

	switch (cmd)
	{
	case HLTV_ACTIVE:
		m_IsHLTV = true;
		m_ForceHLTV = false;
		break;
	case HLTV_STATUS:
	{
		if (m_Proxy) {
			m_Proxy->ParseStatusMsg(m_Instream);
			return;
		}

		m_Instream->SkipBytes(10);
		break;
	}
	case HLTV_LISTEN:
		m_System->Printf("WARNING! Server::ParseHLTV: unexpected \"listen\" command.\n");
		break;
	default:
		m_System->Printf("WARNING! Server::ParseHLTV: unknown director command.\n");
		m_Instream->m_Overflowed = true;
		break;
	}
}

void Server::SetProxy(IProxy *proxy)
{
	m_Proxy = proxy;
	m_Director = proxy->GetDirector();
}

void Server::SetDirector(IDirector *director)
{
	m_Director = director;
}

void Server::ParseDirector()
{
	unsigned char *start = m_Instream->CurrentByte();
	int length = m_Instream->ReadByte();

	if (m_Director)
	{
		DirectorCmd cmd;
		cmd.ReadFromStream(m_Instream);
		cmd.SetTime(m_Time);

		m_Director->AddCommand(&cmd);
		return;
	}

	m_Instream->SkipBytes(length);

	if (m_ServerState == SERVER_CONNECTED) {
		m_World->AddSignonData(svc_director, start, length + 1);
	}
	else if (m_ServerState == SERVER_RUNNING || m_ServerState == SERVER_INTERMISSION)
	{
		m_ReliableData.WriteByte(svc_director);
		m_ReliableData.WriteBuf(start, length + 1);
	}
	else {
		m_System->Errorf("Server::ParseDirector: unexpected server state.\n");
	}
}

void Server::ParseInfo(BitBuffer *stream, bool detailed)
{
	serverinfo_t si;

	Q_strlcpy(si.address, m_ServerChannel.GetTargetAddress()->ToBaseString());
	Q_strlcpy(si.name, stream->ReadString());
	Q_strlcpy(si.map, stream->ReadString());
	Q_strlcpy(si.gamedir, stream->ReadString());
	Q_strlcpy(si.description, stream->ReadString());

	si.activePlayers = stream->ReadByte();
	si.maxPlayers = stream->ReadByte();
	si.protocol = stream->ReadByte();

	if (detailed)
	{
		si.type = stream->ReadByte();
		si.os = stream->ReadByte();
		si.pw = stream->ReadByte();
		si.mod = stream->ReadByte() != 0;

		if (si.mod)
		{
			Q_strlcpy(si.url_info, stream->ReadString());
			Q_strlcpy(si.url_dl, stream->ReadString());
			Q_strlcpy(si.hlversion, stream->ReadString());

			si.ver = stream->ReadLong();
			si.size = stream->ReadLong();
			si.svonly = stream->ReadByte() != 0;
			si.cldll = stream->ReadByte() != 0;
		}
		else
		{
			si.url_info[0] = '\0';
			si.url_dl[0] = '\0';

			si.ver = 0;
			si.size = 0;
		}

		stream->ReadByte();
	}
	else
	{
		si.type = '?';
		si.os = '?';
		si.pw = '?';
		si.mod = 0;
	}

	if (si.activePlayers >= MAX_PROXY_CLIENTS || si.maxPlayers >= MAX_PROXY_CLIENTS)
	{
		si.activePlayers = stream->ReadLong();
		si.maxPlayers = stream->ReadLong();
	}

	if (m_World) {
		m_World->SetServerInfo(&si);
	}
}

void Server::Reconnect()
{
	if (IsConnected())
	{
		m_System->DPrintf("Reconnecting...\n");
		SetState(SERVER_CONNECTED);

		m_ServerChannel.Clear();
		m_validSequence = 0;

		BaseSystemModule::FireSignal(2);
		SendStringCommand("new");
	}
}

void Server::Disconnect()
{
	if (m_ServerState == SERVER_DISCONNECTED) {
		return;
	}

	BaseSystemModule::FireSignal(6);
	m_ServerChannel.ClearFragments();

	if (IsDemoFile())
	{
		m_DemoFile.StopPlayBack();
		m_System->Printf("Demo playback stopped.\n");
	}
	else
	{
		for (int i = 0; i < 3; i++)
		{
			SendStringCommand("dropclient\n");
			m_ServerChannel.TransmitOutgoing();
		}

		m_System->Printf("Disconnected.\n");
	}

	m_World->FinishGame();
	Reset();
	ScheduleAutoRetry();
	m_ServerChannel.Close();
}

void Server::ParseClientData()
{
	if (m_IsHLTV) {
		return;
	}

	unsigned int fromSeqNr = 0;
	m_Instream->StartBitMode();

	if (m_Instream->ReadBit()) {
		fromSeqNr = m_SeqNrMap[m_Instream->ReadByte()];
	}

	m_World->ParseClientData(m_Instream, fromSeqNr, &m_ClientData, &m_ClientDataStruct);
	m_Instream->EndBitMode();
}

void Server::ParseIntermission()
{
#ifdef HLTV_FIXES
	if (m_ServerState < SERVER_RUNNING)
	{
		m_System->DPrintf("Received intermission too early; deferring until RUNNING.\n");
		m_PendingIntermission = true;
		return;
	}
#endif

	SetState(SERVER_INTERMISSION);

	if (!IsDemoFile()) {
		m_ReliableData.WriteByte(svc_intermission);
	}
}

void Server::ParseFinale()
{
	m_ReliableData.WriteByte(svc_finale);
	m_ReliableData.WriteString(m_Instream->ReadString());
}

void Server::ParseCutscene()
{
	m_ReliableData.WriteByte(svc_cutscene);
	m_ReliableData.WriteString(m_Instream->ReadString());
}

void Server::SendUserVar(char *key, char *value)
{
	char cmdString[1024];

	if (!IsConnected()) {
		return;
	}

	if (*value)
	{
		Q_snprintf(cmdString, sizeof(cmdString), "setinfo \"%s\" \"%s\"\n", key, value);
		SendStringCommand(cmdString);
	}
}

bool Server::IsConnected()
{
	if (m_ServerState >= SERVER_CONNECTED) {
		return true;
	}

	return false;
}

bool Server::IsDemoFile()
{
	return m_DemoFile.IsPlaying();
}

bool Server::IsGameServer()
{
	return m_IsGameServer;
}

bool Server::IsRelayProxy()
{
	return !m_IsGameServer;
}

void Server::ParseCenterPrint()
{
	char *string = m_Instream->ReadString();
	if (m_ServerState == SERVER_CONNECTED)
	{
		m_World->AddSignonData(svc_centerprint, (unsigned char *)string, Q_strlen(string) + 1);
	}
	else if (m_ServerState == SERVER_RUNNING || m_ServerState == SERVER_INTERMISSION)
	{
		m_UnreliableData.WriteByte(svc_centerprint);
		m_UnreliableData.WriteString(string);
	}
	else {
		m_System->Errorf("Server::ParseCenterPrint: unexpected state.\n");
	}

	if (string[0]) {
		m_System->DPrintf(">>%s\n", string);
	}
}

void Server::CheckAutoRetry()
{
	if (m_NextAutoRetry > 0 && m_NextAutoRetry < m_SystemTime)
	{
		m_NextAutoRetry = 0;

		Retry();
		m_System->Printf("Automatic connection retry...\n");
	}
}

void Server::ScheduleAutoRetry()
{
	if (m_AutoRetry) {
		m_NextAutoRetry = m_SystemTime + 4;
		return;
	}

	m_NextAutoRetry = 0;
}

bool Server::Connect(IWorld *world, NetAddress *address, INetSocket *socket)
{
	if (!socket || !world || !address) {
		m_System->Printf("ERROR! Server::Connect: invalid parameters.\n");
		return false;
	}

	INetwork *network = socket->GetNetwork();
	if (address->Equal(network->GetLocalAddress())) {
		m_System->Printf("ERROR! Server::Connect: can't connect to myself.\n");
		return false;
	}

	Reset();
	m_ServerAddress.FromNetAddress(address);

	m_ServerSocket = socket;
	m_ServerChannel.Create(m_System, m_ServerSocket, &m_ServerAddress);
	m_World = world;

	SetState(SERVER_CHALLENGING);
	return true;
}

bool Server::LoadDemo(IWorld *world, char *filename, bool forceHLTV, bool continuous)
{
	Reset();
	m_World = world;
	m_ServerSocket = nullptr;

	m_ServerChannel.Create(m_System);
	m_DemoFile.Init(m_World, this, &m_ServerChannel);

	if (!m_DemoFile.LoadDemo(filename)) {
		return false;
	}

	m_DemoFile.SetContinuous(continuous);

	m_IsHLTV = false;
	m_ForceHLTV = forceHLTV;

	SetState(SERVER_CONNECTED);
	m_ServerChannel.SetConnected(true);

	return true;
}

void Server::SetUserInfo(char *key, char *value)
{
	if (!key || !value) {
		return;
	}

	m_UserInfo.SetValueForKey(key, value);
	SendUserVar(key, value);
}

void Server::ParseSetPause()
{
	m_IsPaused = m_Instream->ReadByte() != 0;
	if (!m_IsPaused) {
		m_validSequence = 0;
	}

	m_World->SetPaused(m_IsPaused);
}

void Server::SendStringCommand(char *command)
{
	if (!m_ServerChannel.IsFakeChannel())
	{
		m_ServerChannel.m_reliableStream.WriteByte(clc_stringcmd);
		m_ServerChannel.m_reliableStream.WriteString(command);
	}
}

void Server::SendHLTVCommand(BitBuffer *msg)
{
	if (!m_ServerChannel.IsFakeChannel())
	{
		m_ServerChannel.m_reliableStream.WriteByte(clc_hltv);
		m_ServerChannel.m_reliableStream.WriteBuf(msg->GetData(), msg->CurrentSize());
	}
}

void Server::ParseSendExtraInfo()
{
	char *clientfallback = m_Instream->ReadString();
	int allowCheats = m_Instream->ReadByte();

	m_World->SetExtraInfo(clientfallback, allowCheats);
}

void Server::ParseTimeScale()
{
	float timescale = m_Instream->ReadFloat();
	if (m_Director) {
		return;
	}

	if (m_ServerState == SERVER_CONNECTED)
	{
		m_System->Printf("Server::ParseTimeScale: invalid during signon.\n");
	}
	else if (m_ServerState == SERVER_RUNNING || m_ServerState == SERVER_INTERMISSION)
	{
		m_ReliableData.WriteByte(svc_timescale);
		m_ReliableData.WriteFloat(timescale);
	}
	else {
		m_System->Errorf("Server::ParseTimeScale: unexpected server state.\n");
	}
}

void Server::ParseResourceLocation()
{
	char *url = m_Instream->ReadString();
	if (url) {
		Q_strlcpy(g_DownloadURL, url);
	}
}

float Server::GetTime()
{
	return m_Time;
}

void Server::CheckConnection()
{
	if (m_ServerState == SERVER_DISCONNECTED) {
		return;
	}

	if (!m_ServerChannel.IsTimedOut() && !m_ServerChannel.IsCrashed()) {
		return;
	}

	m_System->Printf("Server connection lost (%s).\n", m_ServerChannel.IsCrashed() ? "data mismatch" : "time out");

	Disconnect();
	ScheduleAutoRetry();
}

void Server::StopRetry()
{
	m_NextAutoRetry = 0;
}

void Server::Retry()
{
	if (m_ServerAddress.IsValid() && m_ServerSocket && m_World)
	{
		Connect(m_World, &m_ServerAddress, m_ServerSocket);
		BaseSystemModule::FireSignal(7);
	}
	else
	{
		m_System->DPrintf("HLTV proxy wasn't connected before.\n");
	}
}

void Server::SetAutoRetry(bool state)
{
	if (!state) {
		m_NextAutoRetry = 0;
	}

	m_AutoRetry = state;
}

void Server::SetDelayReconnect(bool state)
{
	m_DelayReconnect = state;
}

void Server::Reset()
{
	m_ServerChannel.ClearFragments();
	m_ServerChannel.Close();

	ClearFrame(true);
	Q_memset(m_SeqNrMap, 0, sizeof(m_SeqNrMap));

	m_validSequence = 0;
	m_CurrentRetry = 0;
	m_IsHLTV = true;
	m_ForceHLTV = false;
	m_IsPaused = false;
	m_IsGameServer = false;
	m_Time = 0;

#ifdef HLTV_FIXES
	m_PendingIntermission = false;
#endif

	m_DemoFile.Reset();
	SetState(SERVER_DISCONNECTED);
}

char *Server::GetCmdName(int cmd)
{
	static char description[64];
// With `HLTV_FIXES` enabled meaning of `svc_startofusermessages` changed a bit: now it is an id of the first user message
#ifdef HLTV_FIXES
	if (cmd >= svc_startofusermessages && m_World)
#else // HLTV_FIXES
	if (cmd > svc_startofusermessages && m_World)
#endif // HLTV_FIXES
	{
		UserMsg *usermsg = m_World->GetUserMsg(cmd);
		if (usermsg)
		{
			Q_snprintf(description, sizeof(description), "UserMsg:%s", usermsg->szName);
		}
		else
		{
			Q_snprintf(description, sizeof(description), "Invalid UserMsg");
		}
	}
	else
	{
		Q_snprintf(description, sizeof(description), "EngMsg:%s", m_ClientFuncs[cmd].pszname);
	}

	return description;
}

void Server::SetName(char *newName)
{
	Q_strlcpy(m_Name, newName);
}

void Server::ProcessEntityUpdate()
{
	if (!m_Frame.entities) {
		return;
	}

	for (unsigned int num = 0; num < m_Frame.entitynum; num++)
	{
		entity_state_t *state = &((entity_state_t *)m_Frame.entities)[num];
		if (m_ClientPlayerNum == state->number - 1)
		{
			state->origin[0] = m_ClientDataStruct.origin[0];
			state->origin[1] = m_ClientDataStruct.origin[1];
			state->origin[2] = m_ClientDataStruct.origin[2];
		}
	}
}

void Server::ParseSendCvarValue()
{
	char *name = m_Instream->ReadString();
}

void Server::ParseSendCvarValue2()
{
	int requestID = m_Instream->ReadLong();
	char *name = m_Instream->ReadString();
}

void Server::ParseExec()
{
	bool execClassScript = m_Instream->ReadByte() != 0;
	if (execClassScript)
	{
		int scriptId = m_Instream->ReadByte();
	}
}

void Server::ReceiveSignal(ISystemModule *module, unsigned int signal, void *data)
{
	BaseSystemModule::ReceiveSignal(module, signal, data);
}

void Server::RegisterListener(ISystemModule *module)
{
	BaseSystemModule::RegisterListener(module);
}

void Server::RemoveListener(ISystemModule *module)
{
	BaseSystemModule::RemoveListener(module);
}

IBaseSystem *Server::GetSystem()
{
	return BaseSystemModule::GetSystem();
}

int Server::GetSerial()
{
	return BaseSystemModule::GetSerial();
}

char *Server::GetName()
{
	return BaseSystemModule::GetName();
}

int Server::GetState()
{
	return BaseSystemModule::GetState();
}

int Server::GetVersion()
{
	return BaseSystemModule::GetVersion();
}

IBaseInterface *CreateServer()
{
	IServer *pServer = new Server;
	return (IBaseInterface *)pServer;
}

EXPOSE_INTERFACE_FN(CreateServer, Server, SERVER_INTERFACE_VERSION);
