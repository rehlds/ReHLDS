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

Master::GameToAppIDMapItem_t Master::m_GameToAppIDMap[] =
{
	{ GAME_APPID_CSTRIKE,      "cstrike" },
	{ GAME_APPID_TFC,          "tfc" },
	{ GAME_APPID_DOD,          "dod" },
	{ GAME_APPID_DMC,          "dmc" },
	{ GAME_APPID_GEARBOX,      "gearbox" },
	{ GAME_APPID_RICOCHET,     "ricochet" },
	{ GAME_APPID_VALVE,        "valve" },
	{ GAME_APPID_CZERO,        "czero" },
	{ GAME_APPID_CZEROR,       "czeror" },
	{ GAME_APPID_BSHIFT,       "bshift" },
	{ GAME_APPID_CSTRIKE_BETA, "cstrike_beta" }
};

Master::Master()
{
	m_Proxy = nullptr;
	m_MasterSocket = nullptr;

	m_NoMaster = false;
	m_bMasterLoaded = false;
	m_bSteamInitialized = false;

	m_flMasterUpdateTime = 0;
}

int Master::GetGameAppID(const char *gamedir) const
{
	for (auto& game : m_GameToAppIDMap) {
		if (!Q_stricmp(game.dir, gamedir)) {
			return game.appID;
		}
	}

	return GAME_APPID_VALVE;
}

bool Master::Init(IBaseSystem *system, int serial, char *name)
{
	BaseSystemModule::Init(system, serial, name);

	m_MasterSocket = m_Proxy->GetSocket();

	m_System->RegisterCommand("heartbeat",  this, CMD_ID_HEARTBEAT);
	m_System->RegisterCommand("nomaster",   this, CMD_ID_NOMASTER);
	m_System->RegisterCommand("listmaster", this, CMD_ID_LISTMASTER);

	m_State = MODULE_RUNNING;
	m_NoMaster = false;
	m_bMasterLoaded = false;

	INetwork *network = m_MasterSocket->GetNetwork();
	if (!network->GetLocalAddress()) {
		m_System->Printf("Master module failed to initialize (no net).\n");
		return false;
	}

	m_bSteamInitialized = false;
	m_System->Printf("Master module initialized.\n");

	return true;
}

void Master::SetProxy(IProxy *proxy)
{
	m_Proxy = proxy;
}

void Master::RunFrame(double time)
{
	BaseSystemModule::RunFrame(time);

	static double s_flLastRunCallbacks = 0.0f;
	if ((time - s_flLastRunCallbacks) > 0.1)
	{
		SteamGameServer_RunCallbacks();
		s_flLastRunCallbacks = time;
	}

	if (m_NoMaster)
		return;

	if (m_Proxy->IsActive())
	{
		if (!m_bSteamInitialized)
		{
			IWorld *world = m_Proxy->GetWorld();
			if (world)
			{
				int nAppID = GetGameAppID(world->GetGameDir());
				if (nAppID > 0)
				{
					FILE *f = fopen("steam_appid.txt", "w+");
					if (f)
					{
						fprintf(f, "%d\n", nAppID);
						fclose(f);
					}
				}

				INetwork *network = m_MasterSocket->GetNetwork();
				NetAddress *netAdr = network->GetLocalAddress();

				if (SteamApiCompat::GameServer_Init(ntohl(*(u_long *)&netAdr->m_IP[0]), 0, 0, 0xFFFFu, eServerModeNoAuthentication, MASTER_VERSION))
				{
					char szSteamInfo[2560];
					SteamApiCompat::Describe(szSteamInfo, sizeof(szSteamInfo));
					m_System->Printf("%s", szSteamInfo);

					SteamApiCompat::GameServer()->SetProduct("hltv");
					SteamApiCompat::GameServer()->SetGameDescription("hltv");
					SteamApiCompat::GameServer()->SetModDir(world->GetGameDir());
					SteamApiCompat::GameServer()->SetDedicatedServer(true);
					SteamApiCompat::GameServer()->SetSpectatorPort(htons(netAdr->m_Port));
					SteamApiCompat::GameServer()->LogOnAnonymous();

					SteamApiCompat::GameServer()->EnableHeartbeats(true);
				}
				else
				{
					// NOTE: Actually here call via Printf, but we will be use DPrintf to avoid useless print into console
					// or at least make only print once.
					m_System->DPrintf("Master module failed to initialize. (init failed)\n");
				}

				m_bSteamInitialized = SteamApiCompat::GameServer() ? true : false;
			}
		}

		if (m_bSteamInitialized)
		{
			IWorld *world = m_Proxy->GetWorld();
			if (m_flMasterUpdateTime < m_System->GetTime())
			{
				char mapName[MAX_PATH];
				COM_FileBase(world->GetLevelName(), mapName);

				char szHostName[MAX_PATH];
				Q_strlcpy(szHostName, world->GetHostName());

				int slots, proxies, spectators;
				m_Proxy->GetStatistics(proxies, slots, spectators);

				SteamApiCompat::GameServer()->SetMaxPlayerCount(Q_min(slots, 127));	// max slots
				SteamApiCompat::GameServer()->SetServerName(szHostName);
				SteamApiCompat::GameServer()->SetMapName(mapName);
				SteamApiCompat::GameServer()->SetPasswordProtected(m_Proxy->IsPasswordProtected());
				SteamApiCompat::GameServer()->SetModDir(world->GetGameDir());

				m_flMasterUpdateTime = m_System->GetTime() + 5.0f;
			}
		}
	}

	if (m_bSteamInitialized)
	{
		uint32 ip;
		uint16 port;
		char szOutBuf[4096];

		int iLen = SteamApiCompat::GameServer()->GetNextOutgoingPacket(szOutBuf, sizeof(szOutBuf), &ip, &port);
		while (iLen > 0)
		{
			NetAddress netAdr;
			*((uint32 *)&netAdr.m_IP[0]) = htonl(ip);
			netAdr.m_Port = htons(port);

			m_MasterSocket->SendPacket(&netAdr, szOutBuf, iLen);

			iLen = SteamApiCompat::GameServer()->GetNextOutgoingPacket(szOutBuf, sizeof(szOutBuf), &ip, &port);
		}
	}
}

void Master::ShutDown()
{
	if (m_State == MODULE_DISCONNECTED) {
		return;
	}

	if (SteamApiCompat::GameServer()) {
		SteamApiCompat::GameServer()->LogOff();
	}

	BaseSystemModule::ShutDown();
	SteamApiCompat::GameServer_Shutdown();
	m_System->Printf("Master module shutdown.\n");
}

void Master::CMD_Heartbeat(char *cmdLine)
{
	if (m_State == MODULE_DISCONNECTED) {
		return;
	}

	if (SteamApiCompat::GameServer()) {
		SteamApiCompat::GameServer()->ForceHeartbeat();
	}
}

void Master::CMD_NoMaster(char *cmdLine)
{
	if (m_State == MODULE_DISCONNECTED || !SteamApiCompat::GameServer()) {
		return;
	}

	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		m_System->Printf("Syntax: nomaster <0|1>\n");
		m_System->Printf("Master server notification is %s.\n", m_NoMaster ? "disabled" : "enabled");
		return;
	}

	bool bOldMasterState = m_NoMaster;
	m_NoMaster = Q_atoi(params.GetToken(1)) ? true : false;

	if (bOldMasterState != m_NoMaster) {
		SteamApiCompat::GameServer()->EnableHeartbeats(m_NoMaster);
	}
}

void Master::CMD_ListMaster(char *pchCmdLine)
{
	if (m_State == MODULE_DISCONNECTED) {
		return;
	}

	m_System->Printf("No longer used\n");
}

void Master::ExecuteCommand(int commandID, char *commandLine)
{
	switch (commandID)
	{
	case CMD_ID_HEARTBEAT:
		CMD_Heartbeat(commandLine);
		break;
	case CMD_ID_NOMASTER:
		CMD_NoMaster(commandLine);
		break;
	case CMD_ID_LISTMASTER:
		CMD_ListMaster(commandLine);
		break;
	default:
		m_System->Printf("ERROR! Master::ExecuteCommand: unknown command ID %i.\n", commandID);
		break;
	}
}

char *Master::GetType()
{
	return MASTER_INTERFACE_VERSION;
}

void Master::SendShutdown()
{
	if (m_State == MODULE_DISCONNECTED) {
		return;
	}

	if (SteamApiCompat::GameServer()) {
		SteamApiCompat::GameServer()->EnableHeartbeats(false);
	}
}

char *Master::GetStatusLine()
{
	static char string[256];
	Q_snprintf(string, sizeof(string), "Master servers: 0\n");
	return string;
}
