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
#include "const.h"
#include "eiface.h"
#include "model.h"
#include "common.h"
#include "server.h"

#define MAX_RANDOM_RANGE 0x7FFFFFFFUL

// Ambient sound flags
enum
{
	AMBIENT_SOUND_STATIC		= 0,	// medium radius attenuation
	AMBIENT_SOUND_EVERYWHERE	= BIT(0),
	AMBIENT_SOUND_SMALLRADIUS	= BIT(1),
	AMBIENT_SOUND_MEDIUMRADIUS	= BIT(2),
	AMBIENT_SOUND_LARGERADIUS	= BIT(3),
	AMBIENT_SOUND_START_SILENT	= BIT(4),
	AMBIENT_SOUND_NOT_LOOPING	= BIT(5)
};

// The most a message written between MessageBegin and MessageEnd holds
const int MAX_MSG_DATA = 512;

extern unsigned char gMsgData[MAX_MSG_DATA];
extern sizebuf_t gMsgBuffer;
extern edict_t *gMsgEntity;
extern int gMsgDest;
extern int gMsgType;
extern qboolean gMsgStarted;
extern vec3_t gMsgOrigin;
extern int32 idum;
extern int g_groupop;
extern int g_groupmask;
extern unsigned char checkpvs[1024];
extern int c_invis;
extern int c_notvis;
extern vec3_t vec_origin;
extern int r_visframecount;

enum
{
	GROUP_OP_AND = 0,
	GROUP_OP_NAND
};

void PF_makevectors_I(const float *rgflVector);
float PF_Time(void);
void PF_setorigin_I(edict_t *e, const float *org);
void SetMinMaxSize(edict_t *e, const float *min, const float *max, qboolean rotate);
void PF_setsize_I(edict_t *e, const float *rgflMin, const float *rgflMax);
void PF_setmodel_I(edict_t *e, const char *m);
int PF_modelindex(const char *pstr);
int ModelFrames(int modelIndex);
int SV_LookupModelIndex(const char *name);
void PF_bprint(char *s);
void PF_sprint(char *s, int entnum);
void ServerPrint(const char *szMsg);
void ClientPrintf(edict_t *pEdict, PRINT_TYPE ptype, const char *szMsg);
float PF_vectoyaw_I(const float *rgflVector);
void PF_vectoangles_I(const float *rgflVectorIn, float *rgflVectorOut);
void PF_particle_I(const float *org, const float *dir, float color, float count);
void PF_ambientsound_I(edict_t *entity, float *pos, const char *samp, float vol, float attenuation, int fFlags, int pitch);
void PF_sound_I(edict_t *entity, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch);
void PF_traceline_Shared(const float *v1, const float *v2, int nomonsters, edict_t *ent);
void PF_traceline_DLL(const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr);
void TraceHull(const float *v1, const float *v2, int fNoMonsters, int hullNumber, edict_t *pentToSkip, TraceResult *ptr);
void TraceSphere(const float *v1, const float *v2, int fNoMonsters, float radius, edict_t *pentToSkip, TraceResult *ptr);
void TraceModel(const float *v1, const float *v2, int hullNumber, edict_t *pent, TraceResult *ptr);
msurface_t *SurfaceAtPoint(model_t *pModel, mnode_t *node, vec_t *start, vec_t *end);
const char *TraceTexture(edict_t *pTextureEntity, const float *v1, const float *v2);
void PF_TraceToss_Shared(edict_t *ent, edict_t *ignore);
void SV_SetGlobalTrace(trace_t *ptrace);
void PF_TraceToss_DLL(edict_t *pent, edict_t *pentToIgnore, TraceResult *ptr);
int TraceMonsterHull(edict_t *pEdict, const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr);
int PF_newcheckclient(int check);
edict_t *PF_checkclient_I(edict_t *pEdict);
mnode_t *PVSNode(mnode_t *node, vec_t *emins, vec_t *emaxs);
void PVSMark(model_t *pmodel, unsigned char *ppvs);
edict_t *PVSFindEntities(edict_t *pplayer);
qboolean ValidCmd(const char *pCmd);
void PF_stuffcmd_I(edict_t *pEdict, const char *szFmt, ...);
void PF_localcmd_I(const char *str);
void PF_localexec_I(void);
edict_t *FindEntityInSphere(edict_t *pEdictStartSearchAfter, const float *org, float rad);
edict_t *PF_Spawn_I(void);
edict_t *CreateNamedEntity(int className);
void PF_Remove_I(edict_t *ed);
void PF_Remove_I_internal(edict_t *ed);
edict_t *PF_find_Shared(int eStartSearchAfter, int iFieldToMatch, const char *szValueToFind);
int iGetIndex(const char *pszField);
edict_t *FindEntityByString(edict_t *pEdictStartSearchAfter, const char *pszField, const char *pszValue);
int GetEntityIllum(edict_t *pEnt);
qboolean PR_IsEmptyString(const char *s);
int PF_precache_sound_I(const char *s);
int PF_precache_sound_I_internal(const char *s);
unsigned short EV_Precache(int type, const char *psz);
unsigned short EV_Precache_internal(int type, const char *psz);
void EV_PlayReliableEvent_api(IGameClient *cl, int entindex, unsigned short eventindex, float delay, event_args_t *pargs);
void EV_PlayReliableEvent(client_t *cl, int entindex, unsigned short eventindex, float delay, event_args_t *pargs);
void EV_PlayReliableEvent_internal(client_t *cl, int entindex, unsigned short eventindex, float delay, event_args_t *pargs);
void EV_Playback(int flags, const edict_t *pInvoker, unsigned short eventindex, float delay, float *origin, float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);
void EV_SV_Playback(int flags, int clientindex, unsigned short eventindex, float delay, float *origin, float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);
int PF_precache_model_I(const char *s);
int PF_precache_model_I_internal(const char *s);
int PF_precache_generic_I(const char *s);
int PF_precache_generic_I_internal(const char *s);
int PF_IsMapValid_I(const char *mapname);
int PF_NumberOfEntities_I(void);
char *PF_GetInfoKeyBuffer_I(edict_t *e);
char *PF_InfoKeyValue_I(char *infobuffer, const char *key);
void PF_SetKeyValue_I(char *infobuffer, const char *key, const char *value);
void PF_RemoveKey_I(char *s, const char *key);
void PF_SetClientKeyValue_I(int clientIndex, char *infobuffer, const char *key, const char *value);
int PF_walkmove_I(edict_t *ent, float yaw, float dist, int iMode);
int PF_droptofloor_I(edict_t *ent);
int PF_DecalIndex(const char *name);
void PF_lightstyle_I(int style, const char *val);
int PF_checkbottom_I(edict_t *pEdict);
int PF_pointcontents_I(const float *rgflVector);
void PF_aim_I(edict_t *ent, float speed, float *rgflReturn);
void PF_changeyaw_I(edict_t *ent);
void PF_changepitch_I(edict_t *ent);
void PF_setview_I(const edict_t *clientent, const edict_t *viewent);
void PF_crosshairangle_I(const edict_t *clientent, float pitch, float yaw);
edict_t *PF_CreateFakeClient_I(const char *netname);
edict_t *CreateFakeClient_internal(const char *netname);
void PF_RunPlayerMove_I(edict_t *fakeclient, const float *viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, unsigned char impulse, unsigned char msec);
sizebuf_t *WriteDest_Parm(int dest);
void PF_MessageBegin_I(int msg_dest, int msg_type, const float *pOrigin, edict_t *ed);
void PF_MessageEnd_I(void);
void PF_WriteByte_I(int iValue);
void PF_WriteChar_I(int iValue);
void PF_WriteShort_I(int iValue);
void PF_WriteLong_I(int iValue);
void PF_WriteAngle_I(float flValue);
void PF_WriteCoord_I(float flValue);
void PF_WriteString_I(const char *sz);
void PF_WriteEntity_I(int iValue);
void PF_makestatic_I(edict_t *ent);
void PF_StaticDecal(const float *origin, int decalIndex, int entityIndex, int modelIndex);
void PF_setspawnparms_I(edict_t *ent);
void PF_changelevel_I(const char *s1, const char *s2);
void SeedRandomNumberGenerator(void);
int32 ran1(void);
float fran1(void);
float RandomFloat(float flLow, float flHigh);
int32 RandomLong(int32 lLow, int32 lHigh);
void PF_FadeVolume(const edict_t *clientent, int fadePercent, int fadeOutSeconds, int holdTime, int fadeInSeconds);
void PF_SetClientMaxspeed(edict_t *clientent, float fNewMaxspeed);
int PF_GetPlayerUserId(edict_t *e);
unsigned int PF_GetPlayerWONId(edict_t *e);
const char *PF_GetPlayerAuthId(edict_t *e);
void PF_BuildSoundMsg_I(edict_t *entity, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch, int msg_dest, int msg_type, const float *pOrigin, edict_t *ed);
void PF_BuildSoundMsg_I_internal(edict_t *entity, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch, int msg_dest, int msg_type, const float *pOrigin, edict_t *ed);
int PF_IsDedicatedServer(void);
const char *PF_GetPhysicsInfoString(const edict_t *pClient);
const char *PF_GetPhysicsKeyValue(const edict_t *pClient, const char *key);
void PF_SetPhysicsKeyValue(const edict_t *pClient, const char *key, const char *value);
int PF_GetCurrentPlayer(void);
int PF_CanSkipPlayer(const edict_t *pClient);
void PF_SetGroupMask(int mask, int op);
int PF_CreateInstancedBaseline(int classname, struct entity_state_s *baseline);
void PF_Cvar_DirectSet(struct cvar_s *var, const char *value);
void PF_ForceUnmodified(FORCE_TYPE type, float *mins, float *maxs, const char *filename);
void PF_GetPlayerStats(const edict_t *pClient, int *ping, int *packet_loss);
NOXREF void QueryClientCvarValueCmd(void);
NOXREF void QueryClientCvarValueCmd2(void);
void QueryClientCvarValue(const edict_t *player, const char *cvarName);
void QueryClientCvarValue2(const edict_t *player, const char *cvarName, int requestID);
int hudCheckParm(char *parm, char **ppnext);
int EngCheckParm(const char *pchCmdLineToken, char **pchNextVal);
