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

struct plugin_api_t
{
	char name[32];
	void *impl;
};

std::vector<plugin_api_t *> g_PluginApis;
std::vector<cvar_listener_t *> g_CvarsListeners;

plugin_api_t* FindPluginApiByName(const char *name) {
	for (auto pl : g_PluginApis) {
		if (!strcmp(pl->name, name)) {
			return pl;
		}
	}

	return NULL;
}

char* EXT_FUNC GetClientFallback_api() {
	return com_clientfallback;
}

int* EXT_FUNC GetAllowCheats_api() {
	static int sv_cheats_stub = 0;
	Con_Printf("WARNING! allow_cheats marked as deprecated! Use sv_cheats cvar directly!\n");
	return &sv_cheats_stub;
}

bool EXT_FUNC GSBSecure_api() {
	return Steam_GSBSecure() != FALSE;
}

int EXT_FUNC GetBuildNumber_api() {
	return build_number();
}

double EXT_FUNC GetRealTime_api() {
	return realtime;
}

double EXT_FUNC GetHostFrameTime_api() {
	return host_frametime;
}

cmd_function_t* EXT_FUNC GetFirstCmdFunctionHandle_api() {
	return Cmd_GetFirstCmd();
}

int* EXT_FUNC GetMsgBadRead_api() {
	return &msg_badread;
}

cmd_source_t* EXT_FUNC GetCmdSource_api() {
	return &cmd_source;
}

void EXT_FUNC Log_api(const char* prefix, const char* msg) {
#ifdef REHLDS_FLIGHT_REC
	FR_Log(prefix, msg);
#endif
}

DLL_FUNCTIONS* EXT_FUNC GetEntityInterface_api() {
	return &gEntityInterface;
}


//
// Read functions API
//

int EXT_FUNC MSG_ReadChar_api() {
	return MSG_ReadChar();
}

int EXT_FUNC MSG_ReadByte_api() {
	return MSG_ReadByte();
}

int EXT_FUNC MSG_ReadShort_api() {
	return MSG_ReadShort();
}

int EXT_FUNC MSG_ReadLong_api() {
	return MSG_ReadLong();
}

float EXT_FUNC MSG_ReadFloat_api() {
	return MSG_ReadFloat();
}

char* EXT_FUNC MSG_ReadString_api() {
	return MSG_ReadString();
}

char* EXT_FUNC MSG_ReadStringLine_api() {
	return MSG_ReadStringLine();
}

int EXT_FUNC MSG_ReadBuf_api(int iSize, void *pbuf) {
	return MSG_ReadBuf(iSize, pbuf);
}

float EXT_FUNC MSG_ReadAngle_api() {
	return MSG_ReadAngle();
}

float EXT_FUNC MSG_ReadHiresAngle_api() {
	return MSG_ReadHiresAngle();
}

void EXT_FUNC MSG_ReadUsercmd_api(usercmd_t *to, usercmd_t *from) {
	MSG_ReadUsercmd(to, from);
}

float EXT_FUNC MSG_ReadCoord_api() {
	return MSG_ReadCoord();
}

void EXT_FUNC MSG_ReadVec3Coord_api(sizebuf_t *sb, vec3_t fa) {
	MSG_ReadVec3Coord(sb, fa);
}


//
// Read bit functions API
//

bool EXT_FUNC MSG_IsBitReading_api() {
	return MSG_IsBitReading() ? true : false;
}

void EXT_FUNC MSG_StartBitReading_api(sizebuf_t *buf) {
	MSG_StartBitReading(buf);
}

void EXT_FUNC MSG_EndBitReading_api(sizebuf_t *buf) {
	MSG_EndBitReading(buf);
}

uint32 EXT_FUNC MSG_PeekBits_api(int numbits) {
	return MSG_PeekBits(numbits);
}

int EXT_FUNC MSG_ReadOneBit_api() {
	return MSG_ReadOneBit();
}

uint32 EXT_FUNC MSG_ReadBits_api(int numbits) {
	return MSG_ReadBits(numbits);
}

int EXT_FUNC MSG_ReadSBits_api(int numbits) {
	return MSG_ReadSBits(numbits);
}

float EXT_FUNC MSG_ReadBitCoord_api() {
	return MSG_ReadBitCoord();
}

void EXT_FUNC MSG_ReadBitVec3Coord_api(vec_t *fa) {
	MSG_ReadBitVec3Coord(fa);
}

float EXT_FUNC MSG_ReadBitAngle_api(int numbits) {
	return MSG_ReadBitAngle(numbits);
}

int EXT_FUNC MSG_ReadBitData_api(void *dest, int length) {
	return MSG_ReadBitData(dest, length);
}

char* EXT_FUNC MSG_ReadBitString_api() {
	return MSG_ReadBitString();
}

int EXT_FUNC MSG_CurrentBit_api() {
	return MSG_CurrentBit();
}

//
// Write functions API
//

void EXT_FUNC MSG_WriteChar_api(sizebuf_t *sb, int c) {
	MSG_WriteChar(sb, c);
}

void EXT_FUNC MSG_WriteByte_api(sizebuf_t *sb, int c) {
	MSG_WriteByte(sb, c);
}

void EXT_FUNC MSG_WriteShort_api(sizebuf_t *sb, int c) {
	MSG_WriteShort(sb, c);
}

void EXT_FUNC MSG_WriteLong_api(sizebuf_t *sb, int c) {
	MSG_WriteLong(sb, c);
}

void EXT_FUNC MSG_WriteFloat_api(sizebuf_t *sb, float f) {
	MSG_WriteFloat(sb, f);
}

void EXT_FUNC MSG_WriteString_api(sizebuf_t *sb, const char *s) {
	MSG_WriteString(sb, s);
}

void EXT_FUNC MSG_WriteBuf_api(sizebuf_t *sb, int iSize, void *buf) {
	MSG_WriteBuf(sb, iSize, buf);
}

void EXT_FUNC MSG_WriteAngle_api(sizebuf_t *sb, float f) {
	MSG_WriteAngle(sb, f);
}

void EXT_FUNC MSG_WriteHiresAngle_api(sizebuf_t *sb, float f) {
	MSG_WriteHiresAngle(sb, f);
}

void EXT_FUNC MSG_WriteUsercmd_api(sizebuf_t *sb, usercmd_t *to, usercmd_t *from) {
	MSG_WriteUsercmd(sb, to, from);
}

void EXT_FUNC MSG_WriteCoord_api(sizebuf_t *sb, float f) {
	MSG_WriteCoord(sb, f);
}

void EXT_FUNC MSG_WriteVec3Coord_api(sizebuf_t *sb, const vec3_t fa) {
	MSG_WriteVec3Coord(sb, fa);
}


//
// Write bit functions API
//

bool EXT_FUNC MSG_IsBitWriting_api() {
	return MSG_IsBitWriting() ? true : false;
}

void EXT_FUNC MSG_StartBitWriting_api(sizebuf_t *buf) {
	MSG_StartBitWriting(buf);
}

void EXT_FUNC MSG_EndBitWriting_api(sizebuf_t *buf) {
	MSG_EndBitWriting(buf);
}

void EXT_FUNC MSG_WriteOneBit_api(int nValue) {
	MSG_WriteOneBit(nValue);
}

void EXT_FUNC MSG_WriteBits_api(uint32 data, int numbits) {
	MSG_WriteBits(data, numbits);
}

void EXT_FUNC MSG_WriteSBits_api(uint32 data, int numbits) {
	MSG_WriteSBits(data, numbits);
}

void EXT_FUNC MSG_WriteBitCoord_api(float f) {
	MSG_WriteBitCoord(f);
}

void EXT_FUNC MSG_WriteBitVec3Coord_api(const vec3_t fa) {
	MSG_WriteBitVec3Coord(fa);
}

void EXT_FUNC MSG_WriteBitAngle_api(float fAngle, int numbits) {
	MSG_WriteBitAngle(fAngle, numbits);
}

void EXT_FUNC MSG_WriteBitData_api(void *src, int length) {
	MSG_WriteBitData(src, length);
}

void EXT_FUNC MSG_WriteBitString_api(const char *p) {
	MSG_WriteBitString(p);
}

void EXT_FUNC SZ_Write_api(sizebuf_t *buf, const void *data, int length) {
	SZ_Write(buf, data, length);
}

void EXT_FUNC SZ_Print_api(sizebuf_t *buf, const char *data) {
	SZ_Print(buf, data);
}

void EXT_FUNC SZ_Clear_api(sizebuf_t *buf) {
	SZ_Clear(buf);
}

void EXT_FUNC MSG_BeginReading_api() {
	MSG_BeginReading();
}

cvar_t* EXT_FUNC GetCvarVars_api() {
	return cvar_vars;
}

void EXT_FUNC SV_StartSound_api(int recipients, edict_t *entity, int channel, const char *sample, int volume, float attenuation, int flags, int pitch) {
	SV_StartSound(recipients, entity, channel, sample, volume, attenuation, flags, pitch);
}

void EXT_FUNC SV_UpdateUserInfo_api(IGameClient *pGameClient) {
	SV_UpdateUserInfo(pGameClient->GetClient());
}

void* EXT_FUNC Rehlds_GetPluginApi(const char *name) {
	auto api = FindPluginApiByName(name);
	return api ? api->impl : NULL;
}

bool EXT_FUNC StripUnprintableAndSpace_api(char *pch) {
	return Q_StripUnprintableAndSpace(pch) != FALSE;
}

void EXT_FUNC Rehlds_RegisterPluginApi(const char *name, void *impl) {
	auto api = FindPluginApiByName(name);

	if (!api) {
		api = new plugin_api_t;
		Q_strncpy(api->name, name, sizeof api->name - 1);
		api->name[sizeof api->name - 1] = '\0';

		g_PluginApis.push_back(api);
	}

	api->impl = impl;
}

void EXT_FUNC GetCommandMatches_api(const char *string, ObjectList *pMatchList)
{
	if (!string || !string[0])
	{
		return;
	}
	auto TextLen = Q_strlen(string);
	for (auto EngCmd = Cmd_GetFirstCmd(); EngCmd; EngCmd = EngCmd->next)
	{
		if (!Q_strnicmp(EngCmd->name, string, TextLen))
		{
			pMatchList->Add((void*)(EngCmd->name));
		}
	}
	for (auto EngCvar = GetCvarVars_api(); EngCvar; EngCvar = EngCvar->next)
	{
		if (!Q_strnicmp(EngCvar->name, string, TextLen))
		{
			pMatchList->Add((void*)(EngCvar->name));
		}
	}
}

bool EXT_FUNC AddExtDll_api(void *hModule)
{
	if (!hModule) {
		return false;
	}

	if (g_iextdllMac >= MAX_EXTENSION_DLL) {
		Con_Printf("Too many DLLs, ignoring remainder\n");
		return false;
	}

	auto pextdll = &g_rgextdll[g_iextdllMac++];
	Q_memset(pextdll, 0, sizeof(*pextdll));
	pextdll->lDLLHandle = hModule;
	return true;
}

void EXT_FUNC RemoveExtDll_api(void *hModule)
{
	if (!hModule) {
		return;
	}

	for (auto i = 0; i < g_iextdllMac; i++)
	{
		if (g_rgextdll[i].lDLLHandle == hModule)
		{
			g_iextdllMac--;
			if (g_iextdllMac != i)
			{
				Q_memmove(&g_rgextdll[i], &g_rgextdll[i + 1], (g_iextdllMac - i) * sizeof(g_rgextdll[0]));
				i = g_iextdllMac;
			}

			Q_memset(&g_rgextdll[i], 0, sizeof(g_rgextdll[0]));
			break;
		}
	}
}

void EXT_FUNC AddCvarListener_api(const char *var_name, cvar_callback_t func)
{
	cvar_t *var = Cvar_FindVar(var_name);
	if (!var) {
		return;
	}

	for (auto cvars : g_CvarsListeners) {
		if (!Q_strcmp(cvars->name, var_name)) {
			if (cvars->func == func) {
				return;
			}
		}
	}

	cvar_listener_t *list = new cvar_listener_t(var_name, func);
	g_CvarsListeners.push_back(list);
}

void EXT_FUNC RemoveCvarListener_api(const char *var_name, cvar_callback_t func)
{
	auto iter = g_CvarsListeners.begin();
	while (iter != g_CvarsListeners.end())
	{
		if (Q_strcmp((*iter)->name, var_name) != 0 || (*iter)->func != func) {
			iter++;
			continue;
		}

		delete (*iter);
		iter = g_CvarsListeners.erase(iter);
	}
}

void EXT_FUNC SV_SetMoveVars_api()
{
	SV_SetMoveVars(&sv_movevars);
}

void EXT_FUNC SV_WriteMovevarsToClient_api(sizebuf_t *message)
{
	SV_WriteMovevarsToClient(message, &sv_movevars);
}

void EXT_FUNC SetServerPause(bool setPause)
{
	g_psv.paused = setPause;
#ifdef REHLDS_FIXES
	for (int i = 0; i < g_psvs.maxclients; i++)
	{
		if (g_psvs.clients[i].fakeclient)
			continue;
		if (!g_psvs.clients[i].connected)
			continue;

		MSG_WriteByte(&g_psvs.clients[i].netchan.message, svc_setpause);
		MSG_WriteByte(&g_psvs.clients[i].netchan.message, g_psv.paused);
	}
#else // REHLDS_FIXES
	MSG_WriteByte(&g_psv.reliable_datagram, svc_setpause);
	MSG_WriteByte(&g_psv.reliable_datagram, g_psv.paused);
#endif // REHLDS_FIXES
}

CRehldsServerStatic g_RehldsServerStatic;
CRehldsServerData g_RehldsServerData;
CRehldsHookchains g_RehldsHookchains;
RehldsFuncs_t g_RehldsApiFuncs =
{
	&SV_DropClient_api,
	&SV_RejectConnection,
	&Steam_NotifyBotConnect_api,
	&GetNetMessage_api,
	&GetHostClient_api,
	&GetMsgReadCount_api,
	&SV_FilterUser,
	&NET_SendPacket_api,
	&Cmd_TokenizeString,
	&SV_CheckChallenge_api,
	&SV_SendUserReg,
	&SV_WriteDeltaDescriptionsToClient,
	&SV_SetMoveVars_api,
	&SV_WriteMovevarsToClient_api,
	&GetClientFallback_api,
	&GetAllowCheats_api,
	&GSBSecure_api,
	&GetBuildNumber_api,
	&GetRealTime_api,
	&GetMsgBadRead_api,
	&GetCmdSource_api,
	&Log_api,
	&GetEntityInterface_api,
	&EV_PlayReliableEvent_api,
	&SV_LookupSoundIndex,
	&MSG_StartBitWriting_api,
	&MSG_WriteBits_api,
	&MSG_WriteBitVec3Coord_api,
	&MSG_EndBitWriting_api,
	&SZ_GetSpace,
	&GetCvarVars_api,
	&SV_GetChallenge,
	&SV_AddResource,
	&MSG_ReadShort_api,
	&MSG_ReadBuf_api,
	&MSG_WriteBuf_api,
	&MSG_WriteByte_api,
	&MSG_WriteShort_api,
	&MSG_WriteString_api,
	&Rehlds_GetPluginApi,
	&Rehlds_RegisterPluginApi,
	&SV_FileInConsistencyList,
	&Steam_NotifyClientConnect_api,
	&Steam_NotifyClientDisconnect_api,
	&SV_StartSound_api,
	&SV_EmitSound2_api,
	&SV_UpdateUserInfo_api,
	&StripUnprintableAndSpace_api,
	&Cmd_RemoveCmd,
	&GetCommandMatches_api,
	&AddExtDll_api,
	&AddCvarListener_api,
	&RemoveExtDll_api,
	&RemoveCvarListener_api,
	&GetEntityInit_api,
	&MSG_ReadChar_api,
	&MSG_ReadByte_api,
	&MSG_ReadLong_api,
	&MSG_ReadFloat_api,
	&MSG_ReadString_api,
	&MSG_ReadStringLine_api,
	&MSG_ReadAngle_api,
	&MSG_ReadHiresAngle_api,
	&MSG_ReadUsercmd_api,
	&MSG_ReadCoord_api,
	&MSG_ReadVec3Coord_api,
	&MSG_IsBitReading_api,
	&MSG_StartBitReading_api,
	&MSG_EndBitReading_api,
	&MSG_PeekBits_api,
	&MSG_ReadOneBit_api,
	&MSG_ReadBits_api,
	&MSG_ReadSBits_api,
	&MSG_ReadBitCoord_api,
	&MSG_ReadBitVec3Coord_api,
	&MSG_ReadBitAngle_api,
	&MSG_ReadBitData_api,
	&MSG_ReadBitString_api,
	&MSG_CurrentBit_api,
	&MSG_WriteLong_api,
	&MSG_WriteFloat_api,
	&MSG_WriteAngle_api,
	&MSG_WriteHiresAngle_api,
	&MSG_WriteUsercmd_api,
	&MSG_WriteCoord_api,
	&MSG_WriteVec3Coord_api,
	&MSG_IsBitWriting_api,
	&MSG_WriteOneBit_api,
	&MSG_WriteSBits_api,
	&MSG_WriteBitCoord_api,
	&MSG_WriteBitAngle_api,
	&MSG_WriteBitData_api,
	&MSG_WriteBitString_api,
	&SZ_Write_api,
	&SZ_Print_api,
	&SZ_Clear_api,
	&MSG_BeginReading_api,
	&GetHostFrameTime_api,
	&GetFirstCmdFunctionHandle_api,
	&SetServerPause,
};

bool EXT_FUNC SV_EmitSound2_internal(edict_t *entity, IGameClient *pReceiver, int channel, const char *sample, float volume, float attenuation, int flags, int pitch, int emitFlags, const float *pOrigin)
{
	bool bSendPAS = (channel != CHAN_STATIC && !(flags & SND_FL_STOP) && !(emitFlags & SND_EMIT2_NOPAS));
	vec3_t origin = {0, 0, 0};

	if ((!pOrigin || !(emitFlags & SND_EMIT2_USE_ORIGIN)) && entity && entity != g_psv.edicts)
	{
		for (int i = 0; i < 3; ++i)
			origin[i] = (entity->v.maxs[i] + entity->v.mins[i]) * 0.5f + entity->v.origin[i];
	}
	// only worldspawn
	else if (pOrigin != nullptr)
	{
		for (int i = 0; i < 3; ++i)
			origin[i] = pOrigin[i];
	}

	// spawn sound of the entity for everyone clients
	if (!pReceiver)
	{
		if (!SV_BuildSoundMsg(entity, channel, sample, (int)(volume * 255.0f), attenuation, flags, pitch, origin, &g_psv.multicast))
			return false;

		int msg_destination = MSG_FL_NONE;
		if (emitFlags & SND_EMIT2_INVOKER) {
			msg_destination = MSG_FL_ONE;
		}

		if (bSendPAS)
		{
			SV_Multicast(entity, origin, msg_destination | MSG_FL_PAS, FALSE);
		}
		else
		{
			SV_Multicast(entity, origin, msg_destination | MSG_FL_BROADCAST, TRUE);
		}

		return true;
	}

	sizebuf_t *pBuffer;
	client_t *receiver = pReceiver->GetClient();

	if (receiver->fakeclient || (!receiver->connected && !receiver->active && !receiver->spawned))
		return false;

	if (bSendPAS && receiver->edict != entity) // if the sound for myself, then do not check PAS.
	{
		int leafnum = SV_PointLeafnum(origin);
		if (!SV_ValidClientMulticast(receiver, leafnum, MSG_FL_PAS))
			return false;

		pBuffer = &receiver->datagram;
	}
	else
	{
		// reliable
		pBuffer = &receiver->netchan.message;
	}

	// send to one client
	return SV_BuildSoundMsg(entity, channel, sample, (int)(volume * 255.0f), attenuation, flags, pitch, origin, pBuffer) != FALSE;
}

bool EXT_FUNC SV_EmitSound2_api(edict_t *entity, IGameClient *receiver, int channel, const char *sample, float volume, float attenuation, int flags, int pitch, int emitFlags, const float *pOrigin)
{
	return g_RehldsHookchains.m_SV_EmitSound2.callChain(SV_EmitSound2_internal, entity, receiver, channel, sample, volume, attenuation, flags, pitch, emitFlags, pOrigin);
}

sizebuf_t* EXT_FUNC GetNetMessage_api()
{
	return &net_message;
}

IGameClient* EXT_FUNC GetHostClient_api()
{
	if (host_client == NULL)
		return NULL;

	return GetRehldsApiClient(host_client);
}

extern int* EXT_FUNC GetMsgReadCount_api()
{
	return &msg_readcount;
}

IRehldsHookRegistry_Steam_NotifyClientConnect* CRehldsHookchains::Steam_NotifyClientConnect()
{
	return &m_Steam_NotifyClientConnect;
}

IRehldsHookRegistry_SV_ConnectClient* CRehldsHookchains::SV_ConnectClient()
{
	return &m_SV_ConnectClient;
}

IRehldsHookRegistry_SV_GetIDString* CRehldsHookchains::SV_GetIDString()
{
	return &m_SV_GetIDString;
}

IRehldsHookRegistry_SV_SendServerinfo* CRehldsHookchains::SV_SendServerinfo()
{
	return &m_SV_SendServerinfo;
}

IRehldsHookRegistry_SV_CheckProtocol* CRehldsHookchains::SV_CheckProtocol()
{
	return &m_SV_CheckProtocol;
}

IRehldsHookRegistry_SVC_GetChallenge_mod* CRehldsHookchains::SVC_GetChallenge_mod()
{
	return &m_SVC_GetChallenge_mod;
}

IRehldsHookRegistry_SV_CheckKeyInfo* CRehldsHookchains::SV_CheckKeyInfo()
{
	return &m_SV_CheckKeyInfo;
}

IRehldsHookRegistry_SV_CheckIPRestrictions* CRehldsHookchains::SV_CheckIPRestrictions()
{
	return &m_SV_CheckIPRestrictions;
}

IRehldsHookRegistry_SV_FinishCertificateCheck* CRehldsHookchains::SV_FinishCertificateCheck()
{
	return &m_SV_FinishCertificateCheck;
}

IRehldsHookRegistry_Steam_NotifyBotConnect* CRehldsHookchains::Steam_NotifyBotConnect()
{
	return &m_Steam_NotifyBotConnect;
}

IRehldsHookRegistry_SerializeSteamId* CRehldsHookchains::SerializeSteamId()
{
	return &m_SerializeSteamId;
}

IRehldsHookRegistry_SV_CompareUserID* CRehldsHookchains::SV_CompareUserID()
{
	return &m_SV_CompareUserID;
}

IRehldsHookRegistry_Steam_NotifyClientDisconnect* CRehldsHookchains::Steam_NotifyClientDisconnect()
{
	return &m_Steam_NotifyClientDisconnect;
}

IRehldsHookRegistry_PreprocessPacket* CRehldsHookchains::PreprocessPacket() {
	return &m_PreprocessPacket;
}

IRehldsHookRegistry_ValidateCommand* CRehldsHookchains::ValidateCommand() {
	return &m_ValidateCommand;
}

IRehldsHookRegistry_ClientConnected* CRehldsHookchains::ClientConnected() {
	return &m_ClientConnected;
}

IRehldsHookRegistry_HandleNetCommand* CRehldsHookchains::HandleNetCommand() {
	return &m_HandleNetCommand;
}

IRehldsHookRegistry_Mod_LoadBrushModel* CRehldsHookchains::Mod_LoadBrushModel() {
	return &m_Mod_LoadBrushModel;
}

IRehldsHookRegistry_Mod_LoadStudioModel* CRehldsHookchains::Mod_LoadStudioModel() {
	return &m_Mod_LoadStudioModel;
}

IRehldsHookRegistry_ExecuteServerStringCmd* CRehldsHookchains::ExecuteServerStringCmd() {
	return &m_ExecuteServerStringCmd;
}

IRehldsHookRegistry_SV_EmitEvents* CRehldsHookchains::SV_EmitEvents() {
	return &m_SV_EmitEvents;
}

IRehldsHookRegistry_EV_PlayReliableEvent* CRehldsHookchains::EV_PlayReliableEvent() {
	return &m_EV_PlayReliableEvent;
}

IRehldsHookRegistry_SV_StartSound* CRehldsHookchains::SV_StartSound() {
	return &m_SV_StartSound;
}

IRehldsHookRegistry_PF_Remove_I* CRehldsHookchains::PF_Remove_I() {
	return &m_PF_Remove_I;
}

IRehldsHookRegistry_PF_BuildSoundMsg_I* CRehldsHookchains::PF_BuildSoundMsg_I() {
	return &m_PF_BuildSoundMsg_I;
}

IRehldsHookRegistry_SV_WriteFullClientUpdate* CRehldsHookchains::SV_WriteFullClientUpdate() {
	return &m_SV_WriteFullClientUpdate;
}

IRehldsHookRegistry_SV_CheckConsistencyResponse* CRehldsHookchains::SV_CheckConsistencyResponse() {
	return &m_SV_CheckConsistencyResponse;
}

IRehldsHookRegistry_SV_DropClient* CRehldsHookchains::SV_DropClient() {
	return &m_SV_DropClient;
}

IRehldsHookRegistry_SV_ActivateServer* CRehldsHookchains::SV_ActivateServer() {
	return &m_SV_ActivateServer;
}

IRehldsHookRegistry_SV_WriteVoiceCodec* CRehldsHookchains::SV_WriteVoiceCodec() {
	return &m_SV_WriteVoiceCodec;
}

IRehldsHookRegistry_Steam_GSGetSteamID* CRehldsHookchains::Steam_GSGetSteamID() {
	return &m_Steam_GSGetSteamID;
}

IRehldsHookRegistry_SV_TransferConsistencyInfo* CRehldsHookchains::SV_TransferConsistencyInfo() {
	return &m_SV_TransferConsistencyInfo;
}

IRehldsHookRegistry_Steam_GSBUpdateUserData* CRehldsHookchains::Steam_GSBUpdateUserData() {
	return &m_Steam_GSBUpdateUserData;
}

IRehldsHookRegistry_Cvar_DirectSet* CRehldsHookchains::Cvar_DirectSet() {
	return &m_Cvar_DirectSet;
}

IRehldsHookRegistry_SV_EstablishTimeBase* CRehldsHookchains::SV_EstablishTimeBase() {
	return &m_SV_EstablishTimeBase;
}

IRehldsHookRegistry_SV_Spawn_f* CRehldsHookchains::SV_Spawn_f() {
	return &m_SV_Spawn_f;
}

IRehldsHookRegistry_SV_CreatePacketEntities* CRehldsHookchains::SV_CreatePacketEntities() {
	return &m_SV_CreatePacketEntities;
}

IRehldsHookRegistry_SV_EmitSound2* CRehldsHookchains::SV_EmitSound2() {
	return &m_SV_EmitSound2;
}

IRehldsHookRegistry_CreateFakeClient* CRehldsHookchains::CreateFakeClient() {
	return &m_CreateFakeClient;
}

IRehldsHookRegistry_SV_CheckConnectionLessRateLimits* CRehldsHookchains::SV_CheckConnectionLessRateLimits()
{
	return &m_SV_CheckConnectionLessRateLimits;
}

IRehldsHookRegistry_SV_Frame* CRehldsHookchains::SV_Frame() {
	return &m_SV_Frame;
}

IRehldsHookRegistry_SV_ShouldSendConsistencyList* CRehldsHookchains::SV_ShouldSendConsistencyList() {
	return &m_SV_ShouldSendConsistencyList;
}

IRehldsHookRegistry_GetEntityInit* CRehldsHookchains::GetEntityInit() {
	return &m_GetEntityInit;
}

IRehldsHookRegistry_SV_EmitPings* CRehldsHookchains::SV_EmitPings() {
	return &m_SV_EmitPings;
}

IRehldsHookRegistry_ED_Alloc* CRehldsHookchains::ED_Alloc() {
	return &m_ED_Alloc;
}

IRehldsHookRegistry_ED_Free* CRehldsHookchains::ED_Free() {
	return &m_ED_Free;
}

IRehldsHookRegistry_Con_Printf* CRehldsHookchains::Con_Printf() {
	return &m_Con_Printf;
}

IRehldsHookRegistry_SV_CheckUserInfo* CRehldsHookchains::SV_CheckUserInfo() {
	return &m_SV_CheckUserInfo;
}

IRehldsHookRegistry_PF_precache_generic_I* CRehldsHookchains::PF_precache_generic_I() {
	return &m_PF_precache_generic_I;
}

IRehldsHookRegistry_PF_precache_model_I* CRehldsHookchains::PF_precache_model_I() {
	return &m_PF_precache_model_I;
}

IRehldsHookRegistry_PF_precache_sound_I* CRehldsHookchains::PF_precache_sound_I() {
	return &m_PF_precache_sound_I;
}

IRehldsHookRegistry_EV_Precache* CRehldsHookchains::EV_Precache() {
	return &m_EV_Precache;
}

IRehldsHookRegistry_SV_AddResource* CRehldsHookchains::SV_AddResource(){
	return &m_SV_AddResource;
}

IRehldsHookRegistry_SV_ClientPrintf* CRehldsHookchains::SV_ClientPrintf(){
	return &m_SV_ClientPrintf;
}

IRehldsHookRegistry_SV_AllowPhysent* CRehldsHookchains::SV_AllowPhysent() {
	return &m_SV_AllowPhysent;
}

IRehldsHookRegistry_SV_SendResources* CRehldsHookchains::SV_SendResources() {
	return &m_SV_SendResources;
}

int EXT_FUNC CRehldsApi::GetMajorVersion()
{
	return REHLDS_API_VERSION_MAJOR;
}

int EXT_FUNC CRehldsApi::GetMinorVersion()
{
	return REHLDS_API_VERSION_MINOR;
}

const RehldsFuncs_t* EXT_FUNC CRehldsApi::GetFuncs()
{
	return &g_RehldsApiFuncs;
}

IRehldsHookchains* EXT_FUNC CRehldsApi::GetHookchains()
{
	return &g_RehldsHookchains;
}

IRehldsServerStatic* EXT_FUNC CRehldsApi::GetServerStatic() {
	return &g_RehldsServerStatic;
}

IRehldsServerData* EXT_FUNC CRehldsApi::GetServerData() {
	return &g_RehldsServerData;
}

IMessageManager* EXT_FUNC CRehldsApi::GetMessageManager() {
	return &MessageManager();
}

IRehldsFlightRecorder* EXT_FUNC CRehldsApi::GetFlightRecorder() {
	return g_FlightRecorder;
}

EXPOSE_SINGLE_INTERFACE(CRehldsApi, IRehldsApi, VREHLDS_HLDS_API_VERSION);
