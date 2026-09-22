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

#include <vector>

#include "archtypes.h"
#include "rehlds_api.h"
#include "rehlds_interfaces_impl.h"

//Steam_NotifyClientConnect
typedef IHookChainImpl<qboolean, IGameClient*, const void*, unsigned int> CRehldsHook_Steam_NotifyClientConnect;
typedef IHookChainRegistryImpl<qboolean, IGameClient*, const void*, unsigned int> CRehldsHookRegistry_Steam_NotifyClientConnect;

//SV_ConnectClient hook
typedef IVoidHookChainImpl<> CRehldsHook_SV_ConnectClient;
typedef IVoidHookChainRegistryImpl<> CRehldsHookRegistry_SV_ConnectClient;

//SV_GetIDString hook
typedef IHookChainImpl<char*, USERID_t*> CRehldsHook_SV_GetIDString;
typedef IHookChainRegistryImpl<char*, USERID_t*> CRehldsHookRegistry_SV_GetIDString;

//SV_SendServerinfo hook
typedef IVoidHookChainImpl<sizebuf_t*, IGameClient*> CRehldsHook_SV_SendServerinfo;
typedef IVoidHookChainRegistryImpl<sizebuf_t*, IGameClient*> CRehldsHookRegistry_SV_SendServerinfo;

//SV_CheckProtocol hook
typedef IHookChainImpl<int, netadr_t*, int> CRehldsHook_SV_CheckProtocol;
typedef IHookChainRegistryImpl<int, netadr_t*, int> CRehldsHookRegistry_SV_CheckProtocol;

//SVC_GetChallenge_mod hook
typedef IVoidHookChainImpl<char*, int> CRehldsHook_SVC_GetChallenge_mod;
typedef IVoidHookChainRegistryImpl<char*, int> CRehldsHookRegistry_SVC_GetChallenge_mod;

//SV_CheckKeyInfo hook
typedef IHookChainImpl<int, netadr_t*, char*, uint16*, int*, char*, char*> CRehldsHook_SV_CheckKeyInfo;
typedef IHookChainRegistryImpl<int, netadr_t*, char*, uint16*, int*, char*, char*> CRehldsHookRegistry_SV_CheckKeyInfo;

//SV_CheckIPRestrictions hook
typedef IHookChainImpl<int, netadr_t*, int> CRehldsHook_SV_CheckIPRestrictions;
typedef IHookChainRegistryImpl<int, netadr_t*, int> CRehldsHookRegistry_SV_CheckIPRestrictions;

//SV_FinishCertificateCheck hook
typedef IHookChainImpl<int, netadr_t*, int, char*, char*> CRehldsHook_SV_FinishCertificateCheck;
typedef IHookChainRegistryImpl<int, netadr_t*, int, char*, char*> CRehldsHookRegistry_SV_FinishCertificateCheck;

//Steam_NotifyBotConnect hook
typedef IHookChainImpl<qboolean, IGameClient*> CRehldsHook_Steam_NotifyBotConnect;
typedef IHookChainRegistryImpl<qboolean, IGameClient*> CRehldsHookRegistry_Steam_NotifyBotConnect;

//SerializeSteamId
typedef IVoidHookChainImpl<USERID_t*, USERID_t*> CRehldsHook_SerializeSteamId;
typedef IVoidHookChainRegistryImpl<USERID_t*, USERID_t*> CRehldsHookRegistry_SerializeSteamId;

//SV_CompareUserID hook
typedef IHookChainImpl<qboolean, USERID_t*, USERID_t*> CRehldsHook_SV_CompareUserID;
typedef IHookChainRegistryImpl<qboolean, USERID_t*, USERID_t*> CRehldsHookRegistry_SV_CompareUserID;

//Steam_NotifyClientDisconnect
typedef IVoidHookChainImpl<IGameClient*> CRehldsHook_Steam_NotifyClientDisconnect;
typedef IVoidHookChainRegistryImpl<IGameClient*> CRehldsHookRegistry_Steam_NotifyClientDisconnect;

//PreProcessPacket
typedef IHookChainImpl<bool, uint8*, unsigned int, const netadr_t&> CRehldsHook_PreprocessPacket;
typedef IHookChainRegistryImpl<bool, uint8*, unsigned int, const netadr_t&> CRehldsHookRegistry_PreprocessPacket;

//ValidateCommand
typedef IHookChainImpl<bool, const char*, cmd_source_t, IGameClient*> CRehldsHook_ValidateCommand;
typedef IHookChainRegistryImpl<bool, const char*, cmd_source_t, IGameClient*> CRehldsHookRegistry_ValidateCommand;

//ExecuteServerStringCmd
typedef IVoidHookChainImpl<const char*, cmd_source_t, IGameClient*> CRehldsHook_ExecuteServerStringCmd;
typedef IVoidHookChainRegistryImpl<const char*, cmd_source_t, IGameClient*> CRehldsHookRegistry_ExecuteServerStringCmd;

//ClientConnected
typedef IVoidHookChainImpl<IGameClient*> CRehldsHook_ClientConnected;
typedef IVoidHookChainRegistryImpl<IGameClient*> CRehldsHookRegistry_ClientConnected;

//HandleNetCommand
typedef IVoidHookChainImpl<IGameClient*, uint8> CRehldsHook_HandleNetCommand;
typedef IVoidHookChainRegistryImpl<IGameClient*, uint8> CRehldsHookRegistry_HandleNetCommand;

//Mod_LoadBrushModel
typedef IVoidHookChainImpl<model_t*, void*> CRehldsHook_Mod_LoadBrushModel;
typedef IVoidHookChainRegistryImpl<model_t*, void*> CRehldsHookRegistry_Mod_LoadBrushModel;

//Mod_LoadStudioModel
typedef IVoidHookChainImpl<model_t*, void*> CRehldsHook_Mod_LoadStudioModel;
typedef IVoidHookChainRegistryImpl<model_t*, void*> CRehldsHookRegistry_Mod_LoadStudioModel;

//SV_EmitEvents hook
typedef IVoidHookChainImpl<IGameClient *, packet_entities_t *, sizebuf_t *> CRehldsHook_SV_EmitEvents;
typedef IVoidHookChainRegistryImpl<IGameClient *, packet_entities_t *, sizebuf_t *> CRehldsHookRegistry_SV_EmitEvents;

//EV_PlayReliableEvent hook
typedef IVoidHookChainImpl<IGameClient *, int, unsigned short, float, event_args_t *> CRehldsHook_EV_PlayReliableEvent;
typedef IVoidHookChainRegistryImpl<IGameClient *, int, unsigned short, float, event_args_t *> CRehldsHookRegistry_EV_PlayReliableEvent;

//SV_StartSound hook
typedef IVoidHookChainImpl<int , edict_t *, int, const char *, int, float, int, int> CRehldsHook_SV_StartSound;
typedef IVoidHookChainRegistryImpl<int , edict_t *, int, const char *, int, float, int, int> CRehldsHookRegistry_SV_StartSound;

//PF_Remove_I hook
typedef IVoidHookChainImpl<edict_t *> CRehldsHook_PF_Remove_I;
typedef IVoidHookChainRegistryImpl<edict_t *> CRehldsHookRegistry_PF_Remove_I;

//PF_BuildSoundMsg_I hook
typedef IVoidHookChainImpl<edict_t *, int, const char *, float, float, int, int, int, int, const float *, edict_t *> CRehldsHook_PF_BuildSoundMsg_I;
typedef IVoidHookChainRegistryImpl<edict_t *, int, const char *, float, float, int, int, int, int, const float *, edict_t *> CRehldsHookRegistry_PF_BuildSoundMsg_I;

//SV_WriteFullClientUpdate hook
typedef IVoidHookChainImpl<IGameClient *, char *, size_t, sizebuf_t *, IGameClient *> CRehldsHook_SV_WriteFullClientUpdate;
typedef IVoidHookChainRegistryImpl<IGameClient *, char *, size_t, sizebuf_t *, IGameClient *> CRehldsHookRegistry_SV_WriteFullClientUpdate;

//SV_CheckConsistencyResponse hook
typedef IHookChainImpl<bool, IGameClient *, resource_t *, uint32> CRehldsHook_SV_CheckConsistencyResponse;
typedef IHookChainRegistryImpl<bool, IGameClient *, resource_t *, uint32> CRehldsHookRegistry_SV_CheckConsistencyResponse;

//SV_DropClient hook
typedef IVoidHookChainImpl<IGameClient*, bool, const char*> CRehldsHook_SV_DropClient;
typedef IVoidHookChainRegistryImpl<IGameClient*, bool, const char*> CRehldsHookRegistry_SV_DropClient;

//SV_ActivateServer hook
typedef IVoidHookChainImpl<int> CRehldsHook_SV_ActivateServer;
typedef IVoidHookChainRegistryImpl<int> CRehldsHookRegistry_SV_ActivateServer;

//SV_WriteVoiceCodec hook
typedef IVoidHookChainImpl<sizebuf_t *> CRehldsHook_SV_WriteVoiceCodec;
typedef IVoidHookChainRegistryImpl<sizebuf_t *> CRehldsHookRegistry_SV_WriteVoiceCodec;

//Steam_GSGetSteamID hook
typedef IHookChainImpl<uint64> CRehldsHook_Steam_GSGetSteamID;
typedef IHookChainRegistryImpl<uint64> CRehldsHookRegistry_Steam_GSGetSteamID;

//SV_TransferConsistencyInfo hook
typedef IHookChainImpl<int> CRehldsHook_SV_TransferConsistencyInfo;
typedef IHookChainRegistryImpl<int> CRehldsHookRegistry_SV_TransferConsistencyInfo;

//Steam_GSBUpdateUserData hook
typedef IHookChainImpl<bool, uint64, const char *, uint32> CRehldsHook_Steam_GSBUpdateUserData;
typedef IHookChainRegistryImpl<bool, uint64, const char *, uint32> CRehldsHookRegistry_Steam_GSBUpdateUserData;

//Cvar_DirectSet hook
typedef IVoidHookChainImpl<struct cvar_s *, const char *> CRehldsHook_Cvar_DirectSet;
typedef IVoidHookChainRegistryImpl<struct cvar_s *, const char *> CRehldsHookRegistry_Cvar_DirectSet;

//SV_EstablishTimeBase hook
typedef IVoidHookChainImpl<IGameClient *, usercmd_t *, int, int, int> CRehldsHook_SV_EstablishTimeBase;
typedef IVoidHookChainRegistryImpl<IGameClient *, usercmd_t *, int, int, int> CRehldsHookRegistry_SV_EstablishTimeBase;

//SV_Spawn_f hook
typedef IVoidHookChainImpl<> CRehldsHook_SV_Spawn_f;
typedef IVoidHookChainRegistryImpl<> CRehldsHookRegistry_SV_Spawn_f;

//SV_CreatePacketEntities hook
typedef IHookChainImpl<int, sv_delta_t, IGameClient *, packet_entities_t *, sizebuf_t *> CRehldsHook_SV_CreatePacketEntities;
typedef IHookChainRegistryImpl<int, sv_delta_t, IGameClient *, packet_entities_t *, sizebuf_t *> CRehldsHookRegistry_SV_CreatePacketEntities;

//SV_EmitSound2 hook
typedef IHookChainImpl<bool, edict_t *, IGameClient *, int, const char*, float, float, int, int, int, const float*> CRehldsHook_SV_EmitSound2;
typedef IHookChainRegistryImpl<bool, edict_t *, IGameClient *, int, const char*, float, float, int, int, int, const float*> CRehldsHookRegistry_SV_EmitSound2;

//CreateFakeClient hook
typedef IHookChainImpl<edict_t *, const char *> CRehldsHook_CreateFakeClient;
typedef IHookChainRegistryImpl<edict_t *, const char *> CRehldsHookRegistry_CreateFakeClient;

//SV_CheckConnectionLessRateLimits hook
typedef IHookChainImpl<bool, netadr_t &, const uint8_t *, int> CRehldsHook_SV_CheckConnectionLessRateLimits;
typedef IHookChainRegistryImpl<bool, netadr_t &, const uint8_t *, int> CRehldsHookRegistry_SV_CheckConnectionLessRateLimits;

//SV_Frame hook
typedef IVoidHookChainImpl<> CRehldsHook_SV_Frame;
typedef IVoidHookChainRegistryImpl<> CRehldsHookRegistry_SV_Frame;

//SV_ShouldSendConsistencyList hook
typedef IHookChainImpl<bool, IGameClient *, bool> CRehldsHook_SV_ShouldSendConsistencyList;
typedef IHookChainRegistryImpl<bool, IGameClient *, bool> CRehldsHookRegistry_SV_ShouldSendConsistencyList;

//GetEntityInit hook
typedef IHookChainImpl<ENTITYINIT, char *> CRehldsHook_GetEntityInit;
typedef IHookChainRegistryImpl<ENTITYINIT, char *> CRehldsHookRegistry_GetEntityInit;

//SV_EmitPings hook
typedef IHookChainImpl<void, IGameClient *, sizebuf_t *> CRehldsHook_SV_EmitPings;
typedef IHookChainRegistryImpl<void, IGameClient *, sizebuf_t *> CRehldsHookRegistry_SV_EmitPings;

//ED_Alloc hook
typedef IHookChainImpl<edict_t *> CRehldsHook_ED_Alloc;
typedef IHookChainRegistryImpl<edict_t *> CRehldsHookRegistry_ED_Alloc;

//ED_Free hook
typedef IVoidHookChainImpl<edict_t *> CRehldsHook_ED_Free;
typedef IVoidHookChainRegistryImpl<edict_t *> CRehldsHookRegistry_ED_Free;

//Con_Printf hook
typedef IHookChainImpl<void, const char *> CRehldsHook_Con_Printf;
typedef IHookChainRegistryImpl<void, const char *> CRehldsHookRegistry_Con_Printf;

//SV_CheckUserInfo hook
typedef IHookChainImpl<int, netadr_t*, char*, qboolean, int, char*> CRehldsHook_SV_CheckUserInfo;
typedef IHookChainRegistryImpl<int, netadr_t*, char*, qboolean, int, char*> CRehldsHookRegistry_SV_CheckUserInfo;

//PF_precache_generic_I hook
typedef IHookChainImpl<int, const char*> CRehldsHook_PF_precache_generic_I;
typedef IHookChainRegistryImpl<int, const char*> CRehldsHookRegistry_PF_precache_generic_I;

//PF_precache_model_I hook
typedef IHookChainImpl<int, const char*> CRehldsHook_PF_precache_model_I;
typedef IHookChainRegistryImpl<int, const char*> CRehldsHookRegistry_PF_precache_model_I;

//PF_precache_sound_I hook
typedef IHookChainImpl<int, const char*> CRehldsHook_PF_precache_sound_I;
typedef IHookChainRegistryImpl<int, const char*> CRehldsHookRegistry_PF_precache_sound_I;

//EV_Precache hook
typedef IHookChainImpl<unsigned short, int, const char*> CRehldsHook_EV_Precache;
typedef IHookChainRegistryImpl<unsigned short, int, const char*> CRehldsHookRegistry_EV_Precache;

//SV_AddResource hook
typedef IVoidHookChainImpl<resourcetype_t, const char*, int, unsigned char, int> CRehldsHook_SV_AddResource;
typedef IVoidHookChainRegistryImpl<resourcetype_t, const char*, int, unsigned char, int> CRehldsHookRegistry_SV_AddResource;

//SV_ClientPrintf hook
typedef IVoidHookChainImpl<const char*> CRehldsHook_SV_ClientPrintf;
typedef IVoidHookChainRegistryImpl<const char*> CRehldsHookRegistry_SV_ClientPrintf;

//SV_AllowPhysent hook
typedef IHookChainImpl<bool, edict_t*, edict_t*> CRehldsHook_SV_AllowPhysent;
typedef IHookChainRegistryImpl<bool, edict_t*, edict_t*> CRehldsHookRegistry_SV_AllowPhysent;

// SV_TestEntityPosition hook
typedef IHookChainImpl<edict_t*, edict_t*> CRehldsHook_SV_TestEntityPosition;
typedef IHookChainRegistryImpl<edict_t*, edict_t*> CRehldsHookRegistry_SV_TestEntityPosition;

//SV_SendResources hook
typedef IVoidHookChainImpl<sizebuf_t *> CRehldsHook_SV_SendResources;
typedef IVoidHookChainRegistryImpl<sizebuf_t *> CRehldsHookRegistry_SV_SendResources;

class CRehldsHookchains : public IRehldsHookchains {
public:
	CRehldsHookRegistry_Steam_NotifyClientConnect m_Steam_NotifyClientConnect;
	CRehldsHookRegistry_SV_ConnectClient m_SV_ConnectClient;
	CRehldsHookRegistry_SV_GetIDString m_SV_GetIDString;
	CRehldsHookRegistry_SV_SendServerinfo m_SV_SendServerinfo;
	CRehldsHookRegistry_SV_CheckProtocol m_SV_CheckProtocol;
	CRehldsHookRegistry_SVC_GetChallenge_mod m_SVC_GetChallenge_mod;
	CRehldsHookRegistry_SV_CheckKeyInfo m_SV_CheckKeyInfo;
	CRehldsHookRegistry_SV_CheckIPRestrictions m_SV_CheckIPRestrictions;
	CRehldsHookRegistry_SV_FinishCertificateCheck m_SV_FinishCertificateCheck;
	CRehldsHookRegistry_Steam_NotifyBotConnect m_Steam_NotifyBotConnect;
	CRehldsHookRegistry_SerializeSteamId m_SerializeSteamId;
	CRehldsHookRegistry_SV_CompareUserID m_SV_CompareUserID;
	CRehldsHookRegistry_Steam_NotifyClientDisconnect m_Steam_NotifyClientDisconnect;
	CRehldsHookRegistry_PreprocessPacket m_PreprocessPacket;
	CRehldsHookRegistry_ValidateCommand m_ValidateCommand;
	CRehldsHookRegistry_ClientConnected m_ClientConnected;
	CRehldsHookRegistry_HandleNetCommand m_HandleNetCommand;
	CRehldsHookRegistry_Mod_LoadBrushModel m_Mod_LoadBrushModel;
	CRehldsHookRegistry_Mod_LoadStudioModel m_Mod_LoadStudioModel;
	CRehldsHookRegistry_ExecuteServerStringCmd m_ExecuteServerStringCmd;
	CRehldsHookRegistry_SV_EmitEvents m_SV_EmitEvents;
	CRehldsHookRegistry_EV_PlayReliableEvent m_EV_PlayReliableEvent;
	CRehldsHookRegistry_SV_StartSound m_SV_StartSound;
	CRehldsHookRegistry_PF_Remove_I m_PF_Remove_I;
	CRehldsHookRegistry_PF_BuildSoundMsg_I m_PF_BuildSoundMsg_I;
	CRehldsHookRegistry_SV_WriteFullClientUpdate m_SV_WriteFullClientUpdate;
	CRehldsHookRegistry_SV_CheckConsistencyResponse m_SV_CheckConsistencyResponse;
	CRehldsHookRegistry_SV_DropClient m_SV_DropClient;
	CRehldsHookRegistry_SV_ActivateServer m_SV_ActivateServer;
	CRehldsHookRegistry_SV_WriteVoiceCodec m_SV_WriteVoiceCodec;
	CRehldsHookRegistry_Steam_GSGetSteamID m_Steam_GSGetSteamID;
	CRehldsHookRegistry_SV_TransferConsistencyInfo m_SV_TransferConsistencyInfo;
	CRehldsHookRegistry_Steam_GSBUpdateUserData m_Steam_GSBUpdateUserData;
	CRehldsHookRegistry_Cvar_DirectSet m_Cvar_DirectSet;
	CRehldsHookRegistry_SV_EstablishTimeBase m_SV_EstablishTimeBase;
	CRehldsHookRegistry_SV_Spawn_f m_SV_Spawn_f;
	CRehldsHookRegistry_SV_CreatePacketEntities m_SV_CreatePacketEntities;
	CRehldsHookRegistry_SV_EmitSound2 m_SV_EmitSound2;
	CRehldsHookRegistry_CreateFakeClient m_CreateFakeClient;
	CRehldsHookRegistry_SV_CheckConnectionLessRateLimits m_SV_CheckConnectionLessRateLimits;
	CRehldsHookRegistry_SV_Frame m_SV_Frame;
	CRehldsHookRegistry_SV_ShouldSendConsistencyList m_SV_ShouldSendConsistencyList;
	CRehldsHookRegistry_GetEntityInit m_GetEntityInit;
	CRehldsHookRegistry_SV_EmitPings m_SV_EmitPings;
	CRehldsHookRegistry_ED_Alloc m_ED_Alloc;
	CRehldsHookRegistry_ED_Free m_ED_Free;
	CRehldsHookRegistry_Con_Printf m_Con_Printf;
	CRehldsHookRegistry_SV_CheckUserInfo m_SV_CheckUserInfo;
	CRehldsHookRegistry_PF_precache_generic_I m_PF_precache_generic_I;
	CRehldsHookRegistry_PF_precache_model_I m_PF_precache_model_I;
	CRehldsHookRegistry_PF_precache_sound_I m_PF_precache_sound_I;
	CRehldsHookRegistry_EV_Precache m_EV_Precache;
	CRehldsHookRegistry_SV_AddResource m_SV_AddResource;
	CRehldsHookRegistry_SV_ClientPrintf m_SV_ClientPrintf;
	CRehldsHookRegistry_SV_AllowPhysent m_SV_AllowPhysent;
	CRehldsHookRegistry_SV_SendResources m_SV_SendResources;
	CRehldsHookRegistry_SV_TestEntityPosition m_SV_TestEntityPosition;

public:
	EXT_FUNC virtual IRehldsHookRegistry_Steam_NotifyClientConnect* Steam_NotifyClientConnect();
	EXT_FUNC virtual IRehldsHookRegistry_SV_ConnectClient* SV_ConnectClient();
	EXT_FUNC virtual IRehldsHookRegistry_SV_GetIDString* SV_GetIDString();
	EXT_FUNC virtual IRehldsHookRegistry_SV_SendServerinfo* SV_SendServerinfo();
	EXT_FUNC virtual IRehldsHookRegistry_SV_CheckProtocol* SV_CheckProtocol();
	EXT_FUNC virtual IRehldsHookRegistry_SVC_GetChallenge_mod* SVC_GetChallenge_mod();
	EXT_FUNC virtual IRehldsHookRegistry_SV_CheckKeyInfo* SV_CheckKeyInfo();
	EXT_FUNC virtual IRehldsHookRegistry_SV_CheckIPRestrictions* SV_CheckIPRestrictions();
	EXT_FUNC virtual IRehldsHookRegistry_SV_FinishCertificateCheck* SV_FinishCertificateCheck();
	EXT_FUNC virtual IRehldsHookRegistry_Steam_NotifyBotConnect* Steam_NotifyBotConnect();
	EXT_FUNC virtual IRehldsHookRegistry_SerializeSteamId* SerializeSteamId();
	EXT_FUNC virtual IRehldsHookRegistry_SV_CompareUserID* SV_CompareUserID();
	EXT_FUNC virtual IRehldsHookRegistry_Steam_NotifyClientDisconnect* Steam_NotifyClientDisconnect();
	EXT_FUNC virtual IRehldsHookRegistry_PreprocessPacket* PreprocessPacket();
	EXT_FUNC virtual IRehldsHookRegistry_ValidateCommand* ValidateCommand();
	EXT_FUNC virtual IRehldsHookRegistry_ClientConnected* ClientConnected();
	EXT_FUNC virtual IRehldsHookRegistry_HandleNetCommand* HandleNetCommand();
	EXT_FUNC virtual IRehldsHookRegistry_Mod_LoadBrushModel* Mod_LoadBrushModel();
	EXT_FUNC virtual IRehldsHookRegistry_Mod_LoadStudioModel* Mod_LoadStudioModel();
	EXT_FUNC virtual IRehldsHookRegistry_ExecuteServerStringCmd* ExecuteServerStringCmd();
	EXT_FUNC virtual IRehldsHookRegistry_SV_EmitEvents* SV_EmitEvents();
	EXT_FUNC virtual IRehldsHookRegistry_EV_PlayReliableEvent* EV_PlayReliableEvent();
	EXT_FUNC virtual IRehldsHookRegistry_SV_StartSound* SV_StartSound();
	EXT_FUNC virtual IRehldsHookRegistry_PF_Remove_I* PF_Remove_I();
	EXT_FUNC virtual IRehldsHookRegistry_PF_BuildSoundMsg_I* PF_BuildSoundMsg_I();
	EXT_FUNC virtual IRehldsHookRegistry_SV_WriteFullClientUpdate* SV_WriteFullClientUpdate();
	EXT_FUNC virtual IRehldsHookRegistry_SV_CheckConsistencyResponse* SV_CheckConsistencyResponse();
	EXT_FUNC virtual IRehldsHookRegistry_SV_DropClient* SV_DropClient();
	EXT_FUNC virtual IRehldsHookRegistry_SV_ActivateServer* SV_ActivateServer();
	EXT_FUNC virtual IRehldsHookRegistry_SV_WriteVoiceCodec* SV_WriteVoiceCodec();
	EXT_FUNC virtual IRehldsHookRegistry_Steam_GSGetSteamID* Steam_GSGetSteamID();
	EXT_FUNC virtual IRehldsHookRegistry_SV_TransferConsistencyInfo* SV_TransferConsistencyInfo();
	EXT_FUNC virtual IRehldsHookRegistry_Steam_GSBUpdateUserData* Steam_GSBUpdateUserData();
	EXT_FUNC virtual IRehldsHookRegistry_Cvar_DirectSet* Cvar_DirectSet();
	EXT_FUNC virtual IRehldsHookRegistry_SV_EstablishTimeBase* SV_EstablishTimeBase();
	EXT_FUNC virtual IRehldsHookRegistry_SV_Spawn_f* SV_Spawn_f();
	EXT_FUNC virtual IRehldsHookRegistry_SV_CreatePacketEntities* SV_CreatePacketEntities();
	EXT_FUNC virtual IRehldsHookRegistry_SV_EmitSound2* SV_EmitSound2();
	EXT_FUNC virtual IRehldsHookRegistry_CreateFakeClient* CreateFakeClient();
	EXT_FUNC virtual IRehldsHookRegistry_SV_CheckConnectionLessRateLimits* SV_CheckConnectionLessRateLimits();
	EXT_FUNC virtual IRehldsHookRegistry_SV_Frame* SV_Frame();
	EXT_FUNC virtual IRehldsHookRegistry_SV_ShouldSendConsistencyList* SV_ShouldSendConsistencyList();
	EXT_FUNC virtual IRehldsHookRegistry_GetEntityInit* GetEntityInit();
	EXT_FUNC virtual IRehldsHookRegistry_SV_EmitPings* SV_EmitPings();
	EXT_FUNC virtual IRehldsHookRegistry_ED_Alloc* ED_Alloc();
	EXT_FUNC virtual IRehldsHookRegistry_ED_Free* ED_Free();
	EXT_FUNC virtual IRehldsHookRegistry_Con_Printf* Con_Printf();
	EXT_FUNC virtual IRehldsHookRegistry_SV_CheckUserInfo* SV_CheckUserInfo();
	EXT_FUNC virtual IRehldsHookRegistry_PF_precache_generic_I* PF_precache_generic_I();
	EXT_FUNC virtual IRehldsHookRegistry_PF_precache_model_I* PF_precache_model_I();
	EXT_FUNC virtual IRehldsHookRegistry_PF_precache_sound_I* PF_precache_sound_I();
	EXT_FUNC virtual IRehldsHookRegistry_EV_Precache* EV_Precache();
	EXT_FUNC virtual IRehldsHookRegistry_SV_AddResource* SV_AddResource();
	EXT_FUNC virtual IRehldsHookRegistry_SV_ClientPrintf* SV_ClientPrintf();
	EXT_FUNC virtual IRehldsHookRegistry_SV_AllowPhysent* SV_AllowPhysent();
	EXT_FUNC virtual IRehldsHookRegistry_SV_SendResources* SV_SendResources();
	EXT_FUNC virtual IRehldsHookRegistry_SV_TestEntityPosition* SV_TestEntityPosition();
};

extern CRehldsHookchains g_RehldsHookchains;
extern RehldsFuncs_t g_RehldsApiFuncs;
extern CRehldsServerStatic g_RehldsServerStatic;
extern CRehldsServerData g_RehldsServerData;


class CRehldsApi : public IRehldsApi {
public:
	virtual int GetMajorVersion();
	virtual int GetMinorVersion();
	virtual const RehldsFuncs_t* GetFuncs();
	virtual IRehldsHookchains* GetHookchains();
	virtual IRehldsServerStatic* GetServerStatic();
	virtual IRehldsServerData* GetServerData();
	virtual IRehldsFlightRecorder* GetFlightRecorder();
	virtual IMessageManager* GetMessageManager();
};

extern sizebuf_t* GetNetMessage_api();
extern IGameClient* GetHostClient_api();
extern int* GetMsgReadCount_api();
extern std::vector<cvar_listener_t *> g_CvarsListeners;

bool SV_EmitSound2_api(edict_t *entity, IGameClient *receiver, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch, int emitFlags, const float *pOrigin);
