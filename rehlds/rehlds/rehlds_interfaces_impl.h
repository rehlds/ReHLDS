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
#include "rehlds_interfaces.h"
#include "server.h"

const int NET_DECOMPRESS_MAX_TIMES = 10;

struct FragStats_t
{
	float decompress_failure_times[NET_DECOMPRESS_MAX_TIMES];

	// Count of abnormal fragment decompressions in a time window
	int num_decompress_failures;
};

class CNetChan : public INetChan
{
private:
	netchan_t* m_pNetChan;

#ifdef REHLDS_FIXES
	uint8_t m_messageBuffer[NET_MAX_PAYLOAD];
#endif

	// Stats for decompression of incoming fragments
	FragStats_t m_FragStats[MAX_STREAMS];

public:
	CNetChan(netchan_t* chan);

	virtual const netadr_t* GetRemoteAdr();
	virtual sizebuf_t* GetMessageBuf();

	virtual netchan_t* GetChan();

	void Clear();

public:
#ifdef REHLDS_FIXES
	uint8_t* GetExtendedMessageBuffer() { return m_messageBuffer; };
#endif
	FragStats_t &GetFragStats(int stream) { return m_FragStats[stream]; };
};


class CGameClient : public IGameClient
{
private:
	int m_Id;
	client_t* m_pClient;
	CNetChan m_NetChan;

	// added this field to handle a bug with hanging clients in scoreboard after a map change.
	// we would also use the field client_t:connected, but actually can't because there is a bug in CS client when server sends TeamScore
	// and client is not yet initialized ScoreBoard which leads to memory corruption in the client.
	bool m_bSpawnedOnce;

#ifdef REHLDS_FIXES
	double m_localGameTimeBase;
#endif
public:
	CGameClient(int id, client_t* cl);

	virtual int GetId();

	virtual bool IsActive();
	virtual void SetActive(bool active);

	virtual bool IsSpawned();
	virtual void SetSpawned(bool spawned);

	virtual INetChan* GetNetChan();

	virtual sizebuf_t* GetDatagram();

	virtual edict_t* GetEdict();

	virtual USERID_t* GetNetworkUserID();

	virtual const char* GetName();

	virtual bool IsConnected();
	virtual void SetConnected(bool connected);

	virtual uint32 GetVoiceStream(int stream_id);
	virtual void SetLastVoiceTime(double time);
	virtual double GetLastVoiceTime();
	virtual bool GetLoopback();
	virtual struct usercmd_s *GetLastCmd();

	virtual bool IsProxy();
	virtual void SetProxy(bool proxy);

	virtual client_t* GetClient();

	// This client is a fake player controlled by the game DLL
	virtual bool IsFakeClient();
	virtual void SetFakeClient(bool state);

	// On server, getting data
	virtual bool IsFullyConnected();
	virtual void SetFullyConnected(bool state);

	virtual bool IsUploading();
	virtual void SetUploading(bool state);

	virtual bool IsHasUserMsgs();
	virtual void SetHasUserMsgs(bool value);

	virtual bool HasForceUnmodified();
	virtual void SetHasForceUnmodified(bool value);

	// Number of packets choked at the server because the client - server network channel
	// is backlogged with too much data
	virtual int GetChokeCount();
	virtual void SetChokeCount(int count);

	// -1 = no compression. This is where the server is creating the compressed info from
	virtual int GetDeltaSequence();
	virtual void SetDeltaSequence(int value);

	// For filling in big drops
	virtual void SetLastCmd(struct usercmd_s *ucmd);

	virtual double GetConnectTime();
	virtual void SetConnectTime(double time);

	virtual double GetCmdTime();
	virtual void SetCmdTime(double time);

	virtual double GetIgnoreCmdTime();
	virtual void SetIgnoreCmdTime(double time);

	virtual float GetLatency();
	virtual void SetLatency(float latency);

	virtual float GetPacketLoss();
	virtual void SetPacketLoss(float packetLoss);

	virtual double GetLocalTime();
	virtual void SetLocalTime(double time);

	virtual double GetSvTimeBase();
	virtual void SetSvTimeBase(double time);

	// Or time of disconnect for zombies
	virtual double GetConnectionStartedTime();
	virtual void SetConnectionStartedTime(double time);

	// Time when we should send next world state update (datagram)
	virtual double GetNextMessageTime();
	virtual void SetNextMessageTime(double time);

	// Default time to wait for next message
	virtual double GetNextMessageIntervalTime();
	virtual void SetNextMessageIntervalTime(double time_interval);

	// false - only send messages if the client has sent one and the bandwidth is not choked
	virtual bool GetSendMessageState();
	virtual void SetSendMessageState(bool state);

	virtual bool GetSkipMessageState();
	virtual void SetSkipMessageState(bool state);

	virtual bool GetSendInfoState();
	virtual void SetSendInfoState(bool state);

	virtual float GetSendInfoTime();
	virtual void SetSendInfoTime(float time);

	// updates can be deltad from here
	virtual struct client_frame_s *GetFrames();

	// Per edict events
	virtual struct event_state_s *GetEvents();

	// View Entity (camera or the client itself) svc_setview
	virtual const edict_t *GetViewEntity();
	virtual void SetViewEntity(const edict_t *entity);

	// Identifying number on server
	virtual int GetUserID();
	virtual void SetUserID(int iUserID);

	// name, etc (received from client)
	virtual char *GetUserInfo();

	// MD5 hash is 32 hex #'s, plus trailing 0
	// Hashed CD Key (32 hex alphabetic chars + 0 terminator)
	virtual char *GetHashedCDKey();

	virtual int GetTopColor();
	virtual void SetTopColor(int color);

	virtual int GetBottomColor();
	virtual void SetBottomColor(int color);

	virtual resource_t *GetResourcesOnHand();
	virtual resource_t *GetResourcesNeeded();

	virtual FileHandle_t GetUploadFileHandle();
	virtual void SetUploadFileHandle(FileHandle_t fhFile);

	virtual bool IsUploadDoneRegistering();
	virtual void SetUploadDoneRegistering(bool state);

	virtual customization_t *GetCustomizationData();

	virtual int GetCRC32MapValue();
	virtual void SetCRC32MapValue(int crcMapValue);

	// Perform client side prediction of weapon effects
	// Determines that the client enabled prediction weapons and will be handled pfnGetWeaponData
	virtual bool IsClientPredictingWeapons();
	virtual void SetClientPredictingWeapons(bool state);

	// Perform server side lag compensation of player movement
	// Determines that the client is requesting lag compensation
	virtual bool IsClientLagCompensation();
	virtual void SetClientLagCompensation(bool state);

	// Set on server (transmit to client)
	virtual char *GetPhysInfo();

	virtual void SetVoiceStream(int stream_id, int value);

	virtual int GetSendResourceCount();
	virtual void SetSendResourceCount(int count);

	virtual bool IsSentNewResponse();
	virtual void SetSentNewResponse(bool state);

public:
	bool GetSpawnedOnce() const { return m_bSpawnedOnce; }
	void SetSpawnedOnce(bool spawned) { m_bSpawnedOnce = spawned; }
#ifdef REHLDS_FIXES
	uint8_t* GetExtendedMessageBuffer() { return m_NetChan.GetExtendedMessageBuffer(); };
#endif

	void NetchanClear() { m_NetChan.Clear(); }
	FragStats_t &GetFragStats(int stream) { return m_NetChan.GetFragStats(stream); };

#ifdef REHLDS_FIXES
	void SetupLocalGameTime() { m_localGameTimeBase = g_psv.time; }
	double GetLocalGameTime() const { return g_psv.time - m_localGameTimeBase; }
	double GetLocalGameTimeBase() const { return m_localGameTimeBase; }
#endif
};

class CRehldsServerStatic : public IRehldsServerStatic {
public:
	virtual int GetMaxClients();
	virtual bool IsLogActive();
	virtual IGameClient* GetClient(int id);
	virtual client_t* GetClient_t(int id);
	virtual int GetIndexOfClient_t(client_t* client);
	virtual int GetMaxClientsLimit();
	virtual client_t *GetNextClient_t(client_t *client);
	virtual int GetSpawnCount();
	virtual void SetSpawnCount(int count);
	virtual struct server_log_s *GetLog();
	virtual bool IsSecure();
	virtual void SetSecure(bool value);
};

class CRehldsServerData : public IRehldsServerData {
public:
	virtual const char* GetModelName();
	virtual const char* GetName();
	virtual uint32 GetWorldmapCrc();
	virtual uint8* GetClientDllMd5();
	virtual sizebuf_t* GetDatagram();
	virtual sizebuf_t* GetReliableDatagram();

	virtual void SetModelName(const char* modelname);
	virtual void SetConsistencyNum(int num);
	virtual int GetConsistencyNum();
	virtual int GetResourcesNum();
	virtual int GetDecalNameNum();

	virtual double GetTime();
	virtual void SetResourcesNum(int num);
	virtual struct resource_s *GetResource(int index);
	virtual void SetName(const char* name);
	virtual ISteamGameServer *GetSteamGameServer();
	virtual netadr_t *GetNetFrom();
	virtual double GetOldTime();

	virtual void SetNetFrom(struct netadr_s *from);
	virtual void SetWorldmapCrc(uint32 crcValue);
	virtual void SetDecalNameNum(int num);

	virtual bool IsActive();
	virtual void SetActive(bool state);
	virtual bool IsPaused();
	virtual void SetPaused(bool state);
	virtual int GetLastIndexCheckInPVS();
	virtual void SetLastIndexCheckInPVS(int id);
	virtual double GetLastIndexCheckTimeInPVS();
	virtual void SetLastIndexCheckTimeInPVS(double time);
	virtual const char *GetOldName();
	virtual void SetOldName(const char *name);
	virtual const char *GetStartSpotName();
	virtual void SetStartSpotName(const char *startspot);
	virtual struct model_s *GetWorldModel();
	virtual void SetWorldModel(struct model_s *model);
	virtual struct consistency_s *GetConsistency(int index);
	virtual struct model_s *GetModel(int index);
	virtual struct event_s *GetEventPrecache(int index);
	virtual struct entity_state_s *GetEntityBaseline(int index);
	virtual struct extra_baselines_s *GetEntityInstanceBaselines();
	virtual int GetNumGenericNames();
	virtual void SetNumGenericNames(int num);
	virtual int GetNumEdicts();
	virtual void SetNumEdicts(int num_edicts);
	virtual int GetMaxEdicts();
	virtual void SetMaxEdicts(int max_edicts);
	virtual edict_t *GetEdict(int index);
	virtual server_state_t GetState();
	virtual void SetState(server_state_t st);
	virtual sizebuf_t *GetMulticastBuf();
	virtual sizebuf_t *GetSpectatorBuf();
	virtual sizebuf_t *GetSignonBuf();
};

extern CGameClient** g_GameClients;

extern void Rehlds_Interfaces_InitClients();
extern void Rehlds_Interfaces_FreeClients();

extern IGameClient* GetRehldsApiClient(client_t* cl);
