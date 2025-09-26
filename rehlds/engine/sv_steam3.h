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

#pragma once

#include "maintypes.h"
#include "common.h"
#include "common/SteamCommon.h"
#include "public/steam/steam_api.h"
#include "public/steam/steam_gameserver.h"
#include "public/steam/steamclientpublic.h"
#include "server.h"

class CSteamID;
class CSteam3
{
protected:
	bool m_bLoggedOn;
	bool m_bLogOnResult;
	HSteamPipe m_hSteamPipe;

protected:
	CSteam3() : m_bLoggedOn(false), m_bLogOnResult(false), m_hSteamPipe(0) {}

	virtual ~CSteam3() {}
	virtual void Shutdown() = 0;

	void GSSendUserStatusResponse(CSteamID &steamID, int nSecondsConnected, int nSecondsSinceLast);
	bool InitModule();
};

#define MAX_STEAM_TAGS_LENGTH 128 // Steam doesn't send tags string more than 128 bytes

class CSteam3Server: public CSteam3
{
public:

	STEAM_GAMESERVER_CALLBACK(CSteam3Server, OnGSClientApprove, GSClientApprove_t, m_CallbackGSClientApprove);
	STEAM_GAMESERVER_CALLBACK(CSteam3Server, OnGSClientDeny, GSClientDeny_t, m_CallbackGSClientDeny);
	STEAM_GAMESERVER_CALLBACK(CSteam3Server, OnGSClientKick, GSClientKick_t, m_CallbackGSClientKick);
	STEAM_GAMESERVER_CALLBACK(CSteam3Server, OnGSPolicyResponse, GSPolicyResponse_t, m_CallbackGSPolicyResponse);
	STEAM_GAMESERVER_CALLBACK(CSteam3Server, OnLogonSuccess, SteamServersConnected_t, m_CallbackLogonSuccess);
	STEAM_GAMESERVER_CALLBACK(CSteam3Server, OnLogonFailure, SteamServerConnectFailure_t, m_CallbackLogonFailure);

protected:
	bool m_bHasActivePlayers;
	bool m_bWantToBeSecure;
	bool m_bLanOnly;
	CSteamID m_SteamIDGS;

#ifdef REHLDS_FIXES
	char m_GameTagsData[MAX_STEAM_TAGS_LENGTH];
#endif

#ifdef VERSION_SAFE_STEAM_API_INTERFACES
	CSteamGameServerAPIContext m_SteamGameServerAPIContext;
#endif

public:

	NOBODY void SetServerType();
	NOBODY void SetSpawnCount(int count);

	bool BSecure()      const { return m_bWantToBeSecure; }
	bool BLanOnly()     const { return m_bLanOnly; };
	bool BWantsSecure() const { return m_bWantToBeSecure; }
	bool BLoggedOn()    const { return m_bLoggedOn; }

	uint64 GetSteamID();

	void OnGSClientDenyHelper(client_t *cl, EDenyReason eDenyReason, const char *pchOptionalText);
	client_t *ClientFindFromSteamID(class CSteamID &steamIDFind);

	CSteam3Server();

	void Activate();
	virtual void Shutdown();
	bool NotifyClientConnect(client_t *client, const void *pvSteam2Key, uint32 ucbSteam2Key);
	bool NotifyBotConnect(client_t *client);
	void NotifyClientDisconnect(client_t *cl);
	void NotifyOfLevelChange(bool bForce);
	void RunFrame();
	void SendUpdatedServerDetails();
	void UpdateGameTags();

#ifdef VERSION_SAFE_STEAM_API_INTERFACES
	CSteamGameServerAPIContext* GetSteamGameServerAPIContext();
#endif
};

class CSteam3Client: public CSteam3
{
public:
	STEAM_CALLBACK(CSteam3Client, OnClientGameServerDeny, ClientGameServerDeny_t, m_CallbackClientGameServerDeny);
	STEAM_CALLBACK(CSteam3Client, OnGameServerChangeRequested, GameServerChangeRequested_t, m_CallbackGameServerChangeRequested);
	STEAM_CALLBACK(CSteam3Client, OnGameOverlayActivated, GameOverlayActivated_t, m_CallbackGameOverlayActivated);

	CSteam3Client() :
		m_CallbackClientGameServerDeny(this, &CSteam3Client::OnClientGameServerDeny),
		m_CallbackGameServerChangeRequested(this, &CSteam3Client::OnGameServerChangeRequested),
		m_CallbackGameOverlayActivated(this, &CSteam3Client::OnGameOverlayActivated)
	{}

	virtual void Shutdown();

	int InitiateGameConnection(void *pData, int cbMaxData, uint64 steamID, uint32 unIPServer, uint16 usPortServer, bool bSecure);
	void TerminateConnection(uint32, uint16);
	void InitClient();
	void RunFrame();

#ifdef VERSION_SAFE_STEAM_API_INTERFACES
	CSteamAPIContext* GetSteamAPIContext();
#endif

private:
#ifdef VERSION_SAFE_STEAM_API_INTERFACES
	CSteamAPIContext m_SteamAPIContext;
#endif
};

extern CSteam3Server *s_Steam3Server;
extern CSteam3Client s_Steam3Client;

uint64 ISteamGameServer_CreateUnauthenticatedUserConnection();
bool ISteamGameServer_BUpdateUserData(uint64 steamid, const char *netname, uint32 score);
bool ISteamApps_BIsSubscribedApp(uint32 appid);
const char *Steam_GetCommunityName();
qboolean Steam_NotifyClientConnect(client_t *cl, const void *pvSteam2Key, unsigned int ucbSteam2Key);
qboolean Steam_NotifyClientConnect_internal(client_t *cl, const void *pvSteam2Key, unsigned int ucbSteam2Key);
qboolean Steam_NotifyClientConnect_api(IGameClient *cl, const void *pvSteam2Key, unsigned int ucbSteam2Key);
qboolean Steam_NotifyBotConnect(client_t *cl);
qboolean Steam_NotifyBotConnect_internal(client_t *cl);
qboolean Steam_NotifyBotConnect_api(IGameClient* cl);
void Steam_NotifyClientDisconnect(client_t *cl);
void Steam_NotifyClientDisconnect_internal(IGameClient *cl);
void Steam_NotifyClientDisconnect_api(IGameClient* cl);
void Steam_NotifyOfLevelChange();
void Steam_Shutdown();
void Steam_Activate();
void Steam_RunFrame();
void Steam_SetCVar(const char *pchKey, const char *pchValue);
void Steam_ClientRunFrame();
void Steam_InitClient();
int Steam_GSInitiateGameConnection(void *pData, int cbMaxData, uint64 steamID, uint32 unIPServer, uint16 usPortServer, qboolean bSecure);
void Steam_GSTerminateGameConnection(uint32 unIPServer, uint16 usPortServer);
void Steam_ShutdownClient();
uint64 Steam_GSGetSteamID();
qboolean Steam_GSBSecure();
qboolean Steam_GSBLoggedOn();
qboolean Steam_GSBSecurePreference();
TSteamGlobalUserID Steam_Steam3IDtoSteam2(uint64 unSteamID);
uint64 Steam_StringToSteamID(const char *pStr);
const char *Steam_GetGSUniverse();
CSteam3Server *Steam3Server();
CSteam3Client *Steam3Client();
void Master_SetMaster_f();
void Steam_HandleIncomingPacket(byte *data, int len, int fromip, uint16 port);
