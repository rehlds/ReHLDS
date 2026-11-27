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
#include "filesystem_internal.h"
#include "sys_dll.h"
#include "server.h"
#include "rehlds_api.h"

const int MAX_COMMAND_LINE_PARAMS = 50;

typedef struct quakeparms_s
{
	char *basedir;
	char *cachedir;
	int argc;
	char **argv;
	void *membase;
	int memsize;
} quakeparms_t;

extern cvar_t host_name;
extern cvar_t host_speeds;
extern cvar_t host_profile;
extern cvar_t developer;
extern cvar_t host_limitlocal;
extern cvar_t skill;
extern cvar_t deathmatch;
extern cvar_t coop;
extern cvar_t sys_ticrate;
extern cvar_t sys_timescale;
extern cvar_t fps_max;
extern cvar_t host_killtime;
extern cvar_t sv_stats;
extern cvar_t fps_override;
extern cvar_t host_framerate;
extern cvar_t pausable;
extern cvar_t suitvolume;

extern double realtime;
extern double rolling_fps;
extern quakeparms_t host_parms;
extern qboolean host_initialized;
extern double host_frametime;
extern int host_framecount;
extern client_t *host_client;
extern qboolean gfNoMasterServer;
extern double oldrealtime;
extern int host_hunklevel;
extern jmp_buf host_abortserver;
extern jmp_buf host_enddemo;
extern unsigned short *host_basepal;

NOXREF void Host_EndGame(const char *message, ...);
void NORETURN Host_Error(const char *error, ...);
void Host_InitLocal(void);
NOXREF void Info_WriteVars(FileHandle_t fp);
void Host_WriteConfiguration(void);
void Host_WriteCustomConfig(void);
void SV_ClientPrintf(const char *fmt, ...);
void SV_ClientPrintf_internal(const char *Dest);
void SV_BroadcastPrintf(const char *fmt, ...);
void Host_ClientCommands(const char *fmt, ...);
void SV_DropClient_api(IGameClient* cl, bool crash, const char* fmt, ...);
void SV_DropClient(client_t *cl, qboolean crash, const char *fmt, ...);
void SV_DropClient_internal(client_t *cl, qboolean crash, const char *string);
void Host_ClearClients(qboolean bFramesOnly);
void Host_ShutdownServer(qboolean crash);
void SV_ClearClientStates(void);
void Host_CheckDyanmicStructures(void);
void Host_ClearMemory(qboolean bQuiet);
float Host_GetDedicatedTicrate();
#ifdef REHLDS_FIXES
double Host_GetDedicatedFrameWaitTime();
#endif
DLL_EXPORT int Host_GetFrameSleepTime();
qboolean Host_FilterTime(float time);
qboolean Master_IsLanGame(void);
void Master_Heartbeat_f(void);
void Host_ComputeFPS(double frametime);
void Host_GetHostInfo(float *fps, int *nActive, int *unused, int *nMaxPlayers, char *pszMap);
void Host_Speeds(double *time);
void Host_UpdateScreen(void);
void Host_UpdateSounds(void);
void Host_CheckConnectionFailure(void);
void _Host_Frame(float time);
int Host_Frame(float time, int iState, int *stateInfo);
void CheckGore(void);
qboolean Host_IsSinglePlayerGame(void);
qboolean Host_IsServerActive(void);
void Host_Version(void);
int Host_Init(quakeparms_t *parms);
void Host_Shutdown(void);
