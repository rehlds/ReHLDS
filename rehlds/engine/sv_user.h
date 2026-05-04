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
#include "server.h"
#include "world.h"
#include "pm_defs.h"

const int CMD_MAXBACKUP = 64;

typedef struct sv_adjusted_positions_s
{
	int active;
	int needrelink;
	vec3_t neworg;
	vec3_t oldorg;
	vec3_t initial_correction_org;
	vec3_t oldabsmin;
	vec3_t oldabsmax;
	int deadflag;
	vec3_t temp_org;
	int temp_org_setflag;
} sv_adjusted_positions_t;

typedef struct clc_func_s
{
	unsigned char opcode;
	char *pszname;
	void(*pfnParse)(client_t *);
} clc_func_t;

extern edict_t *sv_player;
extern sv_adjusted_positions_t truepositions[MAX_CLIENTS];
extern qboolean g_balreadymoved;

extern float s_LastFullUpdate[33];
extern cvar_t sv_edgefriction;
extern cvar_t sv_maxspeed;
extern cvar_t sv_accelerate;
extern cvar_t sv_footsteps;
extern cvar_t sv_rollspeed;
extern cvar_t sv_rollangle;
extern cvar_t sv_unlag;
extern cvar_t sv_maxunlag;
extern cvar_t sv_unlagpush;
extern cvar_t sv_unlagsamples;
extern cvar_t mp_consistency;
extern cvar_t sv_voiceenable;

extern qboolean nofind;

void SV_ParseConsistencyResponse(client_t *pSenderClient);
qboolean SV_FileInConsistencyList(const char *filename, consistency_t **ppconsist);
int SV_TransferConsistencyInfo(void);
int SV_TransferConsistencyInfo_internal(void);
void SV_SendConsistencyList(sizebuf_t *msg);
void SV_PreRunCmd(void);
void SV_CopyEdictToPhysent(physent_t *pe, int e, edict_t *check);
void SV_AddLinksToPM_(areanode_t *node, float *pmove_mins, float *pmove_maxs);
void SV_AddLinksToPM(areanode_t *node, vec_t *origin);
void SV_PlayerRunPreThink(edict_t *player, float time);
qboolean SV_PlayerRunThink(edict_t *ent, float frametime, double clienttimebase);
void SV_CheckMovingGround(edict_t *player, float frametime);
void SV_ConvertPMTrace(trace_t *dest, pmtrace_t *src, edict_t *ent);
void SV_ForceFullClientsUpdate(void);
void SV_RunCmd(usercmd_t* ucmd, int random_seed, qboolean fNetCmd = FALSE, qboolean fChopped = FALSE);
int SV_ValidateClientCommand(char *pszCommand);
float SV_CalcClientTime(client_t *cl);
void SV_ComputeLatency(client_t *cl);
int SV_UnlagCheckTeleport(vec_t *v1, vec_t *v2);
void SV_GetTrueOrigin(int player, vec_t *origin);
void SV_GetTrueMinMax(int player, float **fmin, float **fmax);
entity_state_t *SV_FindEntInPack(int index, packet_entities_t *pack);
void SV_SetupMove(client_t *_host_client);
void SV_RestoreMove(client_t *_host_client);
void SV_ParseStringCommand(client_t *pSenderClient);
void SV_ParseDelta(client_t *pSenderClient);
void SV_EstablishTimeBase(client_t *cl, usercmd_t *cmds, int dropped, int numbackup, int numcmds);
void SV_EstablishTimeBase_internal(client_t *cl, usercmd_t *cmds, int dropped, int numbackup, int numcmds);
void SV_ParseMove(client_t *pSenderClient);
void SV_ParseVoiceData(client_t *cl);
void SV_IgnoreHLTV(client_t *cl);
void SV_ParseCvarValue(client_t *cl);
void SV_ParseCvarValue2(client_t *cl);
void SV_ExecuteClientMessage(client_t *cl);
qboolean SV_SetPlayer(int idnum);
void SV_ShowServerinfo_f(void);
void SV_SendEnts_f(void);
void SV_FullUpdate_f(void);
