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
#include "archtypes.h"
#include "cmd_rehlds.h"
#include "rehlds_interfaces.h"
#include "hookchains.h"
#include "FlightRecorder.h"
#include "IMessageManager.h"
#include "interface.h"
#include "model.h"
#include "ObjectList.h"
#include "pr_dlls.h"

#define REHLDS_API_VERSION_MAJOR 3
#define REHLDS_API_VERSION_MINOR 15

//Steam_NotifyClientConnect hook
typedef IHookChain<qboolean, IGameClient*, const void*, unsigned int> IRehldsHook_Steam_NotifyClientConnect;
typedef IHookChainRegistry<qboolean, IGameClient*, const void*, unsigned int> IRehldsHookRegistry_Steam_NotifyClientConnect;

//SV_ConnectClient hook
typedef IVoidHookChain<> IRehldsHook_SV_ConnectClient;
typedef IVoidHookChainRegistry<> IRehldsHookRegistry_SV_ConnectClient;

//SV_GetIDString hook
typedef IHookChain<char*, USERID_t*> IRehldsHook_SV_GetIDString;
typedef IHookChainRegistry<char*, USERID_t*> IRehldsHookRegistry_SV_GetIDString;

//SV_SendServerinfo hook
typedef IVoidHookChain<sizebuf_t*, IGameClient*> IRehldsHook_SV_SendServerinfo;
typedef IVoidHookChainRegistry<sizebuf_t*, IGameClient*> IRehldsHookRegistry_SV_SendServerinfo;

//SV_CheckProtocol hook
typedef IHookChain<int, netadr_t*, int> IRehldsHook_SV_CheckProtocol;
typedef IHookChainRegistry<int, netadr_t*, int> IRehldsHookRegistry_SV_CheckProtocol;

//SVC_GetChallenge_mod hook
typedef IVoidHookChain<char*, int> IRehldsHook_SVC_GetChallenge_mod;
typedef IVoidHookChainRegistry<char*, int> IRehldsHookRegistry_SVC_GetChallenge_mod;

//SV_CheckKeyInfo hook
typedef IHookChain<int, netadr_t*, char*, uint16*, int*, char*, char*> IRehldsHook_SV_CheckKeyInfo;
typedef IHookChainRegistry<int, netadr_t*, char*, uint16*, int*, char*, char*> IRehldsHookRegistry_SV_CheckKeyInfo;

//SV_CheckIPRestrictions hook
typedef IHookChain<int, netadr_t*, int> IRehldsHook_SV_CheckIPRestrictions;
typedef IHookChainRegistry<int, netadr_t*, int> IRehldsHookRegistry_SV_CheckIPRestrictions;

//SV_FinishCertificateCheck hook
typedef IHookChain<int, netadr_t*, int, char*, char*> IRehldsHook_SV_FinishCertificateCheck;
typedef IHookChainRegistry<int, netadr_t*, int, char*, char*> IRehldsHookRegistry_SV_FinishCertificateCheck;

//Steam_NotifyBotConnect hook
typedef IHookChain<qboolean, IGameClient*> IRehldsHook_Steam_NotifyBotConnect;
typedef IHookChainRegistry<qboolean, IGameClient*> IRehldsHookRegistry_Steam_NotifyBotConnect;

//SerializeSteamId
typedef IVoidHookChain<USERID_t*, USERID_t*> IRehldsHook_SerializeSteamId;
typedef IVoidHookChainRegistry<USERID_t*, USERID_t*> IRehldsHookRegistry_SerializeSteamId;

//SV_CompareUserID hook
typedef IHookChain<qboolean, USERID_t*, USERID_t*> IRehldsHook_SV_CompareUserID;
typedef IHookChainRegistry<qboolean, USERID_t*, USERID_t*> IRehldsHookRegistry_SV_CompareUserID;

//Steam_NotifyClientDisconnect
typedef IVoidHookChain<IGameClient*> IRehldsHook_Steam_NotifyClientDisconnect;
typedef IVoidHookChainRegistry<IGameClient*> IRehldsHookRegistry_Steam_NotifyClientDisconnect;

//PreProcessPacket
typedef IHookChain<bool, uint8*, unsigned int, const netadr_t&> IRehldsHook_PreprocessPacket;
typedef IHookChainRegistry<bool, uint8*, unsigned int, const netadr_t&> IRehldsHookRegistry_PreprocessPacket;

//ValidateCommand
typedef IHookChain<bool, const char*, cmd_source_t, IGameClient*> IRehldsHook_ValidateCommand;
typedef IHookChainRegistry<bool, const char*, cmd_source_t, IGameClient*> IRehldsHookRegistry_ValidateCommand;

//ExecuteServerStringCmd
typedef IVoidHookChain<const char*, cmd_source_t, IGameClient*> IRehldsHook_ExecuteServerStringCmd;
typedef IVoidHookChainRegistry<const char*, cmd_source_t, IGameClient*> IRehldsHookRegistry_ExecuteServerStringCmd;

//ClientConnected
typedef IVoidHookChain<IGameClient*> IRehldsHook_ClientConnected;
typedef IVoidHookChainRegistry<IGameClient*> IRehldsHookRegistry_ClientConnected;

//HandleNetCommand
typedef IVoidHookChain<IGameClient*, uint8> IRehldsHook_HandleNetCommand;
typedef IVoidHookChainRegistry<IGameClient*, uint8> IRehldsHookRegistry_HandleNetCommand;

//Mod_LoadBrushModel
typedef IVoidHookChain<model_t*, void*> IRehldsHook_Mod_LoadBrushModel;
typedef IVoidHookChainRegistry<model_t*, void*> IRehldsHookRegistry_Mod_LoadBrushModel;

//Mod_LoadStudioModel
typedef IVoidHookChain<model_t*, void*> IRehldsHook_Mod_LoadStudioModel;
typedef IVoidHookChainRegistry<model_t*, void*> IRehldsHookRegistry_Mod_LoadStudioModel;

//SV_EmitEvents hook
typedef IVoidHookChain<IGameClient *, struct packet_entities_s *, sizebuf_t *> IRehldsHook_SV_EmitEvents;
typedef IVoidHookChainRegistry<IGameClient *, struct packet_entities_s *, sizebuf_t *> IRehldsHookRegistry_SV_EmitEvents;

//EV_PlayReliableEvent hook
typedef IVoidHookChain<IGameClient *, int, unsigned short, float, struct event_args_s *> IRehldsHook_EV_PlayReliableEvent;
typedef IVoidHookChainRegistry<IGameClient *, int, unsigned short, float, struct event_args_s *> IRehldsHookRegistry_EV_PlayReliableEvent;

//SV_StartSound hook
typedef IVoidHookChain<int , edict_t *, int, const char *, int, float, int, int> IRehldsHook_SV_StartSound;
typedef IVoidHookChainRegistry<int , edict_t *, int, const char *, int, float, int, int> IRehldsHookRegistry_SV_StartSound;

//PF_Remove_I hook
typedef IVoidHookChain<edict_t *> IRehldsHook_PF_Remove_I;
typedef IVoidHookChainRegistry<edict_t *> IRehldsHookRegistry_PF_Remove_I;

//PF_BuildSoundMsg_I hook
typedef IVoidHookChain<edict_t *, int, const char *, float, float, int, int, int, int, const float *, edict_t *> IRehldsHook_PF_BuildSoundMsg_I;
typedef IVoidHookChainRegistry<edict_t *, int, const char *, float, float, int, int, int, int, const float *, edict_t *> IRehldsHookRegistry_PF_BuildSoundMsg_I;

//SV_WriteFullClientUpdate hook
typedef IVoidHookChain<IGameClient *, char *, size_t, sizebuf_t *, IGameClient *> IRehldsHook_SV_WriteFullClientUpdate;
typedef IVoidHookChainRegistry<IGameClient *, char *, size_t, sizebuf_t *, IGameClient *> IRehldsHookRegistry_SV_WriteFullClientUpdate;

//SV_CheckConsistencyResponse hook
typedef IHookChain<bool, IGameClient *, resource_t *, uint32> IRehldsHook_SV_CheckConsistencyResponse;
typedef IHookChainRegistry<bool, IGameClient *, resource_t *, uint32> IRehldsHookRegistry_SV_CheckConsistencyResponse;

//SV_DropClient hook
typedef IVoidHookChain<IGameClient*, bool, const char*> IRehldsHook_SV_DropClient;
typedef IVoidHookChainRegistry<IGameClient*, bool, const char*> IRehldsHookRegistry_SV_DropClient;

//SV_ActivateServer hook
typedef IVoidHookChain<int> IRehldsHook_SV_ActivateServer;
typedef IVoidHookChainRegistry<int> IRehldsHookRegistry_SV_ActivateServer;

//SV_WriteVoiceCodec hook
typedef IVoidHookChain<sizebuf_t *> IRehldsHook_SV_WriteVoiceCodec;
typedef IVoidHookChainRegistry<sizebuf_t *> IRehldsHookRegistry_SV_WriteVoiceCodec;

//Steam_GSGetSteamID hook
typedef IHookChain<uint64> IRehldsHook_Steam_GSGetSteamID;
typedef IHookChainRegistry<uint64> IRehldsHookRegistry_Steam_GSGetSteamID;

//SV_TransferConsistencyInfo hook
typedef IHookChain<int> IRehldsHook_SV_TransferConsistencyInfo;
typedef IHookChainRegistry<int> IRehldsHookRegistry_SV_TransferConsistencyInfo;

//Steam_GSBUpdateUserData hook
typedef IHookChain<bool, uint64, const char *, uint32> IRehldsHook_Steam_GSBUpdateUserData;
typedef IHookChainRegistry<bool, uint64, const char *, uint32> IRehldsHookRegistry_Steam_GSBUpdateUserData;

//Cvar_DirectSet hook
typedef IVoidHookChain<struct cvar_s *, const char *> IRehldsHook_Cvar_DirectSet;
typedef IVoidHookChainRegistry<struct cvar_s *, const char *> IRehldsHookRegistry_Cvar_DirectSet;

//SV_EstablishTimeBase hook
typedef IVoidHookChain<IGameClient *, struct usercmd_s *, int, int, int> IRehldsHook_SV_EstablishTimeBase;
typedef IVoidHookChainRegistry<IGameClient *, struct usercmd_s *, int, int, int> IRehldsHookRegistry_SV_EstablishTimeBase;

//SV_Spawn_f hook
typedef IVoidHookChain<> IRehldsHook_SV_Spawn_f;
typedef IVoidHookChainRegistry<> IRehldsHookRegistry_SV_Spawn_f;

//SV_CreatePacketEntities hook
typedef IHookChain<int, enum sv_delta_s, IGameClient *, struct packet_entities_s *, struct sizebuf_s *> IRehldsHook_SV_CreatePacketEntities;
typedef IHookChainRegistry<int, enum sv_delta_s, IGameClient *, struct packet_entities_s *, struct sizebuf_s *> IRehldsHookRegistry_SV_CreatePacketEntities;

//SV_EmitSound2 hook
typedef IHookChain<bool, edict_t *, IGameClient *, int, const char*, float, float, int, int, int, const float*> IRehldsHook_SV_EmitSound2;
typedef IHookChainRegistry<bool, edict_t *, IGameClient *, int, const char*, float, float, int, int, int, const float*> IRehldsHookRegistry_SV_EmitSound2;

//CreateFakeClient hook
typedef IHookChain<edict_t *, const char *> IRehldsHook_CreateFakeClient;
typedef IHookChainRegistry<edict_t *, const char *> IRehldsHookRegistry_CreateFakeClient;

//SV_CheckConnectionLessRateLimits
typedef IHookChain<bool, netadr_t &, const uint8_t *, int> IRehldsHook_SV_CheckConnectionLessRateLimits;
typedef IHookChainRegistry<bool, netadr_t &, const uint8_t *, int> IRehldsHookRegistry_SV_CheckConnectionLessRateLimits;

//SV_Frame hook
typedef IVoidHookChain<> IRehldsHook_SV_Frame;
typedef IVoidHookChainRegistry<> IRehldsHookRegistry_SV_Frame;

//SV_ShouldSendConsistencyList hook
typedef IHookChain<bool, IGameClient *, bool> IRehldsHook_SV_ShouldSendConsistencyList;
typedef IHookChainRegistry<bool, IGameClient *, bool> IRehldsHookRegistry_SV_ShouldSendConsistencyList;

//GetEntityInit hook
typedef IHookChain<ENTITYINIT, char *> IRehldsHook_GetEntityInit;
typedef IHookChainRegistry<ENTITYINIT, char *> IRehldsHookRegistry_GetEntityInit;

//SV_EmitPings hook
typedef IHookChain<void, IGameClient *, sizebuf_t *> IRehldsHook_SV_EmitPings;
typedef IHookChainRegistry<void, IGameClient *, sizebuf_t *> IRehldsHookRegistry_SV_EmitPings;

//ED_Alloc hook
typedef IHookChain<edict_t *> IRehldsHook_ED_Alloc;
typedef IHookChainRegistry<edict_t *> IRehldsHookRegistry_ED_Alloc;

//ED_Free hook
typedef IVoidHookChain<edict_t *> IRehldsHook_ED_Free;
typedef IVoidHookChainRegistry<edict_t *> IRehldsHookRegistry_ED_Free;

//Con_Printf hook
typedef IHookChain<void, const char *> IRehldsHook_Con_Printf;
typedef IHookChainRegistry<void, const char *> IRehldsHookRegistry_Con_Printf;

//SV_CheckUserInfo hook
typedef IHookChain<int, netadr_t *, char *, qboolean, int, char *> IRehldsHook_SV_CheckUserInfo;
typedef IHookChainRegistry<int, netadr_t *, char *, qboolean, int, char *> IRehldsHookRegistry_SV_CheckUserInfo;

//PF_precache_generic_I hook
typedef IHookChain<int, const char *> IRehldsHook_PF_precache_generic_I;
typedef IHookChainRegistry<int, const char *> IRehldsHookRegistry_PF_precache_generic_I;

//PF_precache_model_I hook
typedef IHookChain<int, const char *> IRehldsHook_PF_precache_model_I;
typedef IHookChainRegistry<int, const char *> IRehldsHookRegistry_PF_precache_model_I;

//PF_precache_sound_I hook
typedef IHookChain<int, const char *> IRehldsHook_PF_precache_sound_I;
typedef IHookChainRegistry<int, const char *> IRehldsHookRegistry_PF_precache_sound_I;

//EV_Precache hook
typedef IHookChain<unsigned short, int, const char *> IRehldsHook_EV_Precache;
typedef IHookChainRegistry<unsigned short, int, const char *> IRehldsHookRegistry_EV_Precache;

//SV_AddResource hook
typedef IVoidHookChain<resourcetype_t, const char *, int, unsigned char, int> IRehldsHook_SV_AddResource;
typedef IVoidHookChainRegistry<resourcetype_t, const char *, int, unsigned char, int> IRehldsHookRegistry_SV_AddResource;

//SV_ClientPrintf hook
typedef IVoidHookChain<const char *> IRehldsHook_SV_ClientPrintf;
typedef IVoidHookChainRegistry<const char *> IRehldsHookRegistry_SV_ClientPrintf;

//SV_AllowPhysent hook
typedef IHookChain<bool, edict_t*, edict_t*> IRehldsHook_SV_AllowPhysent;
typedef IHookChainRegistry<bool, edict_t*, edict_t*> IRehldsHookRegistry_SV_AllowPhysent;

//SV_SendResources hook
typedef IVoidHookChain<sizebuf_t *> IRehldsHook_SV_SendResources;
typedef IVoidHookChainRegistry<sizebuf_t *> IRehldsHookRegistry_SV_SendResources;

class IRehldsHookchains {
public:
	virtual ~IRehldsHookchains() { }

	virtual IRehldsHookRegistry_Steam_NotifyClientConnect* Steam_NotifyClientConnect() = 0;
	virtual IRehldsHookRegistry_SV_ConnectClient* SV_ConnectClient() = 0;
	virtual IRehldsHookRegistry_SV_GetIDString* SV_GetIDString() = 0;
	virtual IRehldsHookRegistry_SV_SendServerinfo* SV_SendServerinfo() = 0;
	virtual IRehldsHookRegistry_SV_CheckProtocol* SV_CheckProtocol() = 0;
	virtual IRehldsHookRegistry_SVC_GetChallenge_mod* SVC_GetChallenge_mod() = 0;
	virtual IRehldsHookRegistry_SV_CheckKeyInfo* SV_CheckKeyInfo() = 0;
	virtual IRehldsHookRegistry_SV_CheckIPRestrictions* SV_CheckIPRestrictions() = 0;
	virtual IRehldsHookRegistry_SV_FinishCertificateCheck* SV_FinishCertificateCheck() = 0;
	virtual IRehldsHookRegistry_Steam_NotifyBotConnect* Steam_NotifyBotConnect() = 0;
	virtual IRehldsHookRegistry_SerializeSteamId* SerializeSteamId() = 0;
	virtual IRehldsHookRegistry_SV_CompareUserID* SV_CompareUserID() = 0;
	virtual IRehldsHookRegistry_Steam_NotifyClientDisconnect* Steam_NotifyClientDisconnect() = 0;
	virtual IRehldsHookRegistry_PreprocessPacket* PreprocessPacket() = 0;
	virtual IRehldsHookRegistry_ValidateCommand* ValidateCommand() = 0;
	virtual IRehldsHookRegistry_ClientConnected* ClientConnected() = 0;
	virtual IRehldsHookRegistry_HandleNetCommand* HandleNetCommand() = 0;
	virtual IRehldsHookRegistry_Mod_LoadBrushModel* Mod_LoadBrushModel() = 0;
	virtual IRehldsHookRegistry_Mod_LoadStudioModel* Mod_LoadStudioModel() = 0;
	virtual IRehldsHookRegistry_ExecuteServerStringCmd* ExecuteServerStringCmd() = 0;
	virtual IRehldsHookRegistry_SV_EmitEvents* SV_EmitEvents() = 0;
	virtual IRehldsHookRegistry_EV_PlayReliableEvent* EV_PlayReliableEvent() = 0;
	virtual IRehldsHookRegistry_SV_StartSound* SV_StartSound() = 0;
	virtual IRehldsHookRegistry_PF_Remove_I* PF_Remove_I() = 0;
	virtual IRehldsHookRegistry_PF_BuildSoundMsg_I* PF_BuildSoundMsg_I() = 0;
	virtual IRehldsHookRegistry_SV_WriteFullClientUpdate* SV_WriteFullClientUpdate() = 0;
	virtual IRehldsHookRegistry_SV_CheckConsistencyResponse* SV_CheckConsistencyResponse() = 0;
	virtual IRehldsHookRegistry_SV_DropClient* SV_DropClient() = 0;
	virtual IRehldsHookRegistry_SV_ActivateServer* SV_ActivateServer() = 0;
	virtual IRehldsHookRegistry_SV_WriteVoiceCodec* SV_WriteVoiceCodec() = 0;
	virtual IRehldsHookRegistry_Steam_GSGetSteamID* Steam_GSGetSteamID() = 0;
	virtual IRehldsHookRegistry_SV_TransferConsistencyInfo* SV_TransferConsistencyInfo() = 0;
	virtual IRehldsHookRegistry_Steam_GSBUpdateUserData* Steam_GSBUpdateUserData() = 0;
	virtual IRehldsHookRegistry_Cvar_DirectSet* Cvar_DirectSet() = 0;
	virtual IRehldsHookRegistry_SV_EstablishTimeBase* SV_EstablishTimeBase() = 0;
	virtual IRehldsHookRegistry_SV_Spawn_f* SV_Spawn_f() = 0;
	virtual IRehldsHookRegistry_SV_CreatePacketEntities* SV_CreatePacketEntities() = 0;
	virtual IRehldsHookRegistry_SV_EmitSound2* SV_EmitSound2() = 0;
	virtual IRehldsHookRegistry_CreateFakeClient* CreateFakeClient() = 0;
	virtual IRehldsHookRegistry_SV_CheckConnectionLessRateLimits* SV_CheckConnectionLessRateLimits() = 0;
	virtual IRehldsHookRegistry_SV_Frame* SV_Frame() = 0;
	virtual IRehldsHookRegistry_SV_ShouldSendConsistencyList* SV_ShouldSendConsistencyList() = 0;
	virtual IRehldsHookRegistry_GetEntityInit* GetEntityInit() = 0;
	virtual IRehldsHookRegistry_SV_EmitPings* SV_EmitPings() = 0;
	virtual IRehldsHookRegistry_ED_Alloc* ED_Alloc() = 0;
	virtual IRehldsHookRegistry_ED_Free* ED_Free() = 0;
	virtual IRehldsHookRegistry_Con_Printf* Con_Printf() = 0;
	virtual IRehldsHookRegistry_SV_CheckUserInfo* SV_CheckUserInfo() = 0;
	virtual IRehldsHookRegistry_PF_precache_generic_I* PF_precache_generic_I() = 0;
	virtual IRehldsHookRegistry_PF_precache_model_I* PF_precache_model_I() = 0;
	virtual IRehldsHookRegistry_PF_precache_sound_I* PF_precache_sound_I() = 0;
	virtual IRehldsHookRegistry_EV_Precache* EV_Precache() = 0;
	virtual IRehldsHookRegistry_SV_AddResource* SV_AddResource() = 0;
	virtual IRehldsHookRegistry_SV_ClientPrintf* SV_ClientPrintf() = 0;
	virtual IRehldsHookRegistry_SV_AllowPhysent* SV_AllowPhysent() = 0;
	virtual IRehldsHookRegistry_SV_SendResources* SV_SendResources() = 0;
};

struct RehldsFuncs_t {
	void(*DropClient)(IGameClient* cl, bool crash, const char* fmt, ...);
	void(*RejectConnection)(netadr_t *adr, char *fmt, ...);
	qboolean(*SteamNotifyBotConnect)(IGameClient* cl);
	sizebuf_t*(*GetNetMessage)();
	IGameClient*(*GetHostClient)();
	int*(*GetMsgReadCount)();
	qboolean(*FilterUser)(USERID_t*);
	void(*NET_SendPacket)(unsigned int length, void *data, const netadr_t &to);
	void(*TokenizeString)(char* s);
	bool(*CheckChallenge)(const netadr_t& adr, int challenge);
	void(*SendUserReg)(sizebuf_t* msg);
	void(*WriteDeltaDescriptionsToClient)(sizebuf_t* msg);
	void(*SetMoveVars)();
	void(*WriteMovevarsToClient)(sizebuf_t* msg);
	char*(*GetClientFallback)();
	int*(*GetAllowCheats)();
	bool(*GSBSecure)();
	int(*GetBuildNumber)();
	double(*GetRealTime)();
	int*(*GetMsgBadRead)();
	cmd_source_t*(*GetCmdSource)();
	void(*Log)(const char* prefix, const char* msg);
	DLL_FUNCTIONS *(*GetEntityInterface)();
	void(*EV_PlayReliableEvent)(IGameClient *cl, int entindex, unsigned short eventindex, float delay, struct event_args_s *pargs);
	int(*SV_LookupSoundIndex)(const char *sample);
	void(*MSG_StartBitWriting)(sizebuf_t *buf);
	void(*MSG_WriteBits)(uint32 data, int numbits);
	void(*MSG_WriteBitVec3Coord)(const float *fa);
	void(*MSG_EndBitWriting)(sizebuf_t *buf);
	void*(*SZ_GetSpace)(sizebuf_t *buf, int length);
	cvar_t*(*GetCvarVars)();
	int (*SV_GetChallenge)(const netadr_t& adr);
	void (*SV_AddResource)(resourcetype_t type, const char *name, int size, unsigned char flags, int index);
	int(*MSG_ReadShort)();
	int(*MSG_ReadBuf)(int iSize, void *pbuf);
	void(*MSG_WriteBuf)(sizebuf_t *sb, int iSize, void *buf);
	void(*MSG_WriteByte)(sizebuf_t *sb, int c);
	void(*MSG_WriteShort)(sizebuf_t *sb, int c);
	void(*MSG_WriteString)(sizebuf_t *sb, const char *s);
	void*(*GetPluginApi)(const char *name);
	void(*RegisterPluginApi)(const char *name, void *impl);
	qboolean(*SV_FileInConsistencyList)(const char *filename, struct consistency_s **ppconsist);
	qboolean(*Steam_NotifyClientConnect)(IGameClient *cl, const void *pvSteam2Key, unsigned int ucbSteam2Key);
	void(*Steam_NotifyClientDisconnect)(IGameClient* cl);
	void(*SV_StartSound)(int recipients, edict_t *entity, int channel, const char *sample, int volume, float attenuation, int flags, int pitch);
	bool(*SV_EmitSound2)(edict_t *entity, IGameClient *receiver, int channel, const char *sample, float volume, float attenuation, int flags, int pitch, int emitFlags, const float *pOrigin);
	void(*SV_UpdateUserInfo)(IGameClient *pGameClient);
	bool(*StripUnprintableAndSpace)(char *pch);
	void(*Cmd_RemoveCmd)(const char *cmd_name);
	void(*GetCommandMatches)(const char *string, ObjectList *pMatchList);
	bool(*AddExtDll)(void *hModule);
	void(*AddCvarListener)(const char *var_name, cvar_callback_t func);
	void(*RemoveExtDll)(void *hModule);
	void(*RemoveCvarListener)(const char *var_name, cvar_callback_t func);
	ENTITYINIT(*GetEntityInit)(char *pszClassName);

	// Read functions
	int(*MSG_ReadChar)();
	int(*MSG_ReadByte)();
	int(*MSG_ReadLong)();
	float(*MSG_ReadFloat)();
	char*(*MSG_ReadString)();
	char*(*MSG_ReadStringLine)();
	float(*MSG_ReadAngle)();
	float(*MSG_ReadHiresAngle)();
	void(*MSG_ReadUsercmd)(struct usercmd_s *to, struct usercmd_s *from);
	float(*MSG_ReadCoord)();
	void(*MSG_ReadVec3Coord)(sizebuf_t *sb, vec3_t fa);

	// Read bit functions
	bool(*MSG_IsBitReading)();
	void(*MSG_StartBitReading)(sizebuf_t *buf);
	void(*MSG_EndBitReading)(sizebuf_t *buf);
	uint32(*MSG_PeekBits)(int numbits);
	int(*MSG_ReadOneBit)();
	uint32(*MSG_ReadBits)(int numbits);
	int(*MSG_ReadSBits)(int numbits);
	float(*MSG_ReadBitCoord)();
	void(*MSG_ReadBitVec3Coord)(vec_t *fa);
	float(*MSG_ReadBitAngle)(int numbits);
	int(*MSG_ReadBitData)(void *dest, int length);
	char*(*MSG_ReadBitString)();
	int(*MSG_CurrentBit)();

	// Write functions
	void(*MSG_WriteLong)(sizebuf_t *sb, int c);
	void(*MSG_WriteFloat)(sizebuf_t *sb, float f);
	void(*MSG_WriteAngle)(sizebuf_t *sb, float f);
	void(*MSG_WriteHiresAngle)(sizebuf_t *sb, float f);
	void(*MSG_WriteUsercmd)(sizebuf_t *sb, struct usercmd_s *to, struct usercmd_s *from);
	void(*MSG_WriteCoord)(sizebuf_t *sb, float f);
	void(*MSG_WriteVec3Coord)(sizebuf_t *sb, const vec3_t fa);

	// Write bit functions
	bool(*MSG_IsBitWriting)();
	void(*MSG_WriteOneBit)(int nValue);
	void(*MSG_WriteSBits)(uint32 data, int numbits);
	void(*MSG_WriteBitCoord)(float f);
	void(*MSG_WriteBitAngle)(float fAngle, int numbits);
	void(*MSG_WriteBitData)(void *src, int length);
	void(*MSG_WriteBitString)(const char *p);
	void(*SZ_Write)(sizebuf_t *buf, const void *data, int length);
	void(*SZ_Print)(sizebuf_t *buf, const char *data);
	void(*SZ_Clear)(sizebuf_t *buf);
	void(*MSG_BeginReading)();
	double(*GetHostFrameTime)();
	struct cmd_function_s *(*GetFirstCmdFunctionHandle)();

	// Pause
	void(*SetServerPause)(bool status);
};

class IRehldsApi {
public:
	virtual ~IRehldsApi() {	}

	virtual int GetMajorVersion() = 0;
	virtual int GetMinorVersion() = 0;
	virtual const RehldsFuncs_t* GetFuncs() = 0;
	virtual IRehldsHookchains* GetHookchains() = 0;
	virtual IRehldsServerStatic* GetServerStatic() = 0;
	virtual IRehldsServerData* GetServerData() = 0;
	virtual IRehldsFlightRecorder* GetFlightRecorder() = 0;
	virtual IMessageManager *GetMessageManager() = 0;
};

#define VREHLDS_HLDS_API_VERSION "VREHLDS_HLDS_API_VERSION001"
