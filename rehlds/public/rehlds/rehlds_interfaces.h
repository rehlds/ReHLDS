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

class INetChan;
class IGameClient;

#include "archtypes.h"
#include "const.h"
#include "netadr.h"

#include "common_rehlds.h"
#include "userid_rehlds.h"
#include "FileSystem.h"
#include "protocol_extensions.h"

#ifdef REHLDS_SELF
#include "server.h"
#endif

class INetChan;
class IGameClient;

class IGameClient {
public:
	virtual int GetId() = 0;

	// false = client is free
	virtual bool IsActive() = 0;
	virtual void SetActive(bool active) = 0;

	// false = don't send datagrams
	virtual bool IsSpawned() = 0;
	virtual void SetSpawned(bool spawned) = 0;

	// The client's net connection
	virtual INetChan* GetNetChan() = 0;

	// The datagram is written to after every frame, but only cleared
	// when it is sent out to the client. It can be harmlessly overflowed
	virtual sizebuf_t* GetDatagram() = 0;

	// EDICT_NUM(clientnum + 1)
	virtual edict_t* GetEdict() = 0;

	virtual USERID_t* GetNetworkUserID() = 0;

	virtual const char* GetName() = 0;

	virtual bool IsConnected() = 0;
	virtual void SetConnected(bool connected) = 0;

	// Which other clients does this guy's voice stream go to?
	virtual uint32 GetVoiceStream(int stream_id) = 0;
	virtual void SetLastVoiceTime(double time) = 0;
	virtual double GetLastVoiceTime() = 0;

	// Does this client want to hear his own voice?
	virtual bool GetLoopback() = 0;
	virtual struct usercmd_s *GetLastCmd() = 0;

	// This is spectator proxy (hltv)
	virtual bool IsProxy() = 0;
	virtual void SetProxy(bool proxy) = 0;

	// This client is a fake player controlled by the game DLL
	virtual bool IsFakeClient() = 0;
	virtual void SetFakeClient(bool state) = 0;

	// On server, getting data
	virtual bool IsFullyConnected() = 0;
	virtual void SetFullyConnected(bool state) = 0;

	virtual bool IsUploading() = 0;
	virtual void SetUploading(bool state) = 0;

	virtual bool IsHasUserMsgs() = 0;
	virtual void SetHasUserMsgs(bool value) = 0;

	virtual bool HasForceUnmodified() = 0;
	virtual void SetHasForceUnmodified(bool value) = 0;

	// Number of packets choked at the server because the client - server network channel
	// is backlogged with too much data
	virtual int GetChokeCount() = 0;
	virtual void SetChokeCount(int count) = 0;

	// -1 = no compression. This is where the server is creating the compressed info from
	virtual int GetDeltaSequence() = 0;
	virtual void SetDeltaSequence(int value) = 0;

	// For filling in big drops
	virtual void SetLastCmd(struct usercmd_s *ucmd) = 0;

	virtual double GetConnectTime() = 0;
	virtual void SetConnectTime(double time) = 0;

	virtual double GetCmdTime() = 0;
	virtual void SetCmdTime(double time) = 0;

	virtual double GetIgnoreCmdTime() = 0;
	virtual void SetIgnoreCmdTime(double time) = 0;

	virtual float GetLatency() = 0;
	virtual void SetLatency(float latency) = 0;

	virtual float GetPacketLoss() = 0;
	virtual void SetPacketLoss(float packetLoss) = 0;

	virtual double GetLocalTime() = 0;
	virtual void SetLocalTime(double time) = 0;

	virtual double GetSvTimeBase() = 0;
	virtual void SetSvTimeBase(double time) = 0;

	// Or time of disconnect for zombies
	virtual double GetConnectionStartedTime() = 0;
	virtual void SetConnectionStartedTime(double time) = 0;

	// Time when we should send next world state update (datagram)
	virtual double GetNextMessageTime() = 0;
	virtual void SetNextMessageTime(double time) = 0;

	// Default time to wait for next message
	virtual double GetNextMessageIntervalTime() = 0;
	virtual void SetNextMessageIntervalTime(double time_interval) = 0;

	// false - only send messages if the client has sent one and the bandwidth is not choked
	virtual bool GetSendMessageState() = 0;
	virtual void SetSendMessageState(bool state) = 0;

	virtual bool GetSkipMessageState() = 0;
	virtual void SetSkipMessageState(bool state) = 0;

	virtual bool GetSendInfoState() = 0;
	virtual void SetSendInfoState(bool state) = 0;

	virtual float GetSendInfoTime() = 0;
	virtual void SetSendInfoTime(float time) = 0;

	// updates can be deltad from here
	virtual struct client_frame_s *GetFrames() = 0;

	// Per edict events
	virtual struct event_state_s *GetEvents() = 0;

	// View Entity (camera or the client itself) svc_setview
	virtual const edict_t *GetViewEntity() = 0;
	virtual void SetViewEntity(const edict_t *entity) = 0;

	// Identifying number on server
	virtual int GetUserID() = 0;
	virtual void SetUserID(int iUserID) = 0;

	// name, etc (received from client)
	virtual char *GetUserInfo() = 0;

	// MD5 hash is 32 hex #'s, plus trailing 0
	// Hashed CD Key (32 hex alphabetic chars + 0 terminator)
	virtual char *GetHashedCDKey() = 0;

	virtual int GetTopColor() = 0;
	virtual void SetTopColor(int color) = 0;

	virtual int GetBottomColor() = 0;
	virtual void SetBottomColor(int color) = 0;

	virtual resource_t *GetResourcesOnHand() = 0;
	virtual resource_t *GetResourcesNeeded() = 0;

	virtual FileHandle_t GetUploadFileHandle() = 0;
	virtual void SetUploadFileHandle(FileHandle_t fhFile) = 0;

	virtual bool IsUploadDoneRegistering() = 0;
	virtual void SetUploadDoneRegistering(bool state) = 0;

	virtual customization_t *GetCustomizationData() = 0;

	virtual int GetCRC32MapValue() = 0;
	virtual void SetCRC32MapValue(int crcMapValue) = 0;

	// Perform client side prediction of weapon effects
	// Determines that the client enabled prediction weapons and will be handled pfnGetWeaponData
	virtual bool IsClientPredictingWeapons() = 0;
	virtual void SetClientPredictingWeapons(bool state) = 0;

	// Perform server side lag compensation of player movement
	// Determines that the client is requesting lag compensation
	virtual bool IsClientLagCompensation() = 0;
	virtual void SetClientLagCompensation(bool state) = 0;

	// Set on server (transmit to client)
	virtual char *GetPhysInfo() = 0;

	virtual void SetVoiceStream(int stream_id, int value) = 0;

	virtual int GetSendResourceCount() = 0;
	virtual void SetSendResourceCount(int count) = 0;

	virtual bool IsSentNewResponse() = 0;
	virtual void SetSentNewResponse(bool state) = 0;

	// Agreed with the client in its connect request, kept until it disconnects
	virtual bool HasProtocolExtension(ProtocolExtension extension) = 0;

	// The agreed value of the extension parameter (PROTOCOL_EXT_PACKETSIZE: bytes, PROTOCOL_EXT_PACKETENTITIES: entities),
	// 0 when the extension has no parameter or is not agreed
	virtual int GetProtocolExtensionParam(ProtocolExtension extension) = 0;

	// this must be the last virtual function in class
#ifdef REHLDS_SELF
	virtual client_t* GetClient() = 0;
#endif
};

class INetChan {
public:
	virtual const netadr_t* GetRemoteAdr() = 0;
	virtual sizebuf_t* GetMessageBuf() = 0;


	// this must be the last virtual function in class
#ifdef REHLDS_SELF
	virtual netchan_t* GetChan() = 0;
#endif
};

#ifndef REHLDS_SELF
struct client_t;
#endif

class IRehldsServerStatic {
public:
	virtual ~IRehldsServerStatic() { }

	virtual int GetMaxClients() = 0;
	virtual bool IsLogActive() = 0;
	virtual IGameClient* GetClient(int id) = 0;
	virtual client_t* GetClient_t(int id) = 0;
	virtual int GetIndexOfClient_t(client_t* client) = 0;
	virtual int GetMaxClientsLimit() = 0;
	virtual client_t *GetNextClient_t(client_t *client) = 0;
	virtual int GetSpawnCount() = 0;
	virtual void SetSpawnCount(int count) = 0;
	virtual struct server_log_s *GetLog() = 0;
	virtual bool IsSecure() = 0;
	virtual void SetSecure(bool value) = 0;
};

class IRehldsServerData {
public:
	virtual ~IRehldsServerData() { }

	virtual const char* GetModelName() = 0;
	virtual const char* GetName() = 0;
	virtual uint32 GetWorldmapCrc() = 0;
	virtual uint8* GetClientDllMd5() = 0;
	virtual sizebuf_t* GetDatagram() = 0;
	virtual sizebuf_t* GetReliableDatagram() = 0;

	virtual void SetModelName(const char* modelname) = 0;
	virtual void SetConsistencyNum(int num) = 0;
	virtual int GetConsistencyNum() = 0;
	virtual int GetResourcesNum() = 0;
	virtual int GetDecalNameNum() = 0;

	virtual double GetTime() = 0;
	virtual void SetResourcesNum(int num) = 0;
	virtual struct resource_s *GetResource(int index) = 0;
	virtual void SetName(const char* name) = 0;
	virtual class ISteamGameServer *GetSteamGameServer() = 0;
	virtual struct netadr_s *GetNetFrom() = 0;
	virtual double GetOldTime() = 0;

	virtual void SetNetFrom(struct netadr_s *from) = 0;
	virtual void SetWorldmapCrc(uint32 crcValue) = 0;
	virtual void SetDecalNameNum(int num) = 0;

	virtual bool IsActive() = 0;
	virtual void SetActive(bool state) = 0;
	virtual bool IsPaused() = 0;
	virtual void SetPaused(bool state) = 0;
	virtual int GetLastIndexCheckInPVS() = 0;
	virtual void SetLastIndexCheckInPVS(int id) = 0;
	virtual double GetLastIndexCheckTimeInPVS() = 0;
	virtual void SetLastIndexCheckTimeInPVS(double time) = 0;
	virtual const char *GetOldName() = 0;
	virtual void SetOldName(const char *name) = 0;
	virtual const char *GetStartSpotName() = 0;
	virtual void SetStartSpotName(const char *startspot) = 0;
	virtual struct model_s *GetWorldModel() = 0;
	virtual void SetWorldModel(struct model_s *model) = 0;
	virtual struct consistency_s *GetConsistency(int index) = 0;
	virtual struct model_s *GetModel(int index) = 0;
	virtual struct event_s *GetEventPrecache(int index) = 0;
	virtual struct entity_state_s *GetEntityBaseline(int index) = 0;
	virtual struct extra_baselines_s *GetEntityInstanceBaselines() = 0;
	virtual int GetNumGenericNames() = 0;
	virtual void SetNumGenericNames(int num) = 0;
	virtual int GetNumEdicts() = 0;
	virtual void SetNumEdicts(int num_edicts) = 0;
	virtual int GetMaxEdicts() = 0;
	virtual void SetMaxEdicts(int max_edicts) = 0;
	virtual edict_t *GetEdict(int index) = 0;
	virtual server_state_t GetState() = 0;
	virtual void SetState(server_state_t st) = 0;
	virtual sizebuf_t *GetMulticastBuf() = 0;
	virtual sizebuf_t *GetSpectatorBuf() = 0;
	virtual sizebuf_t *GetSignonBuf() = 0;
};
