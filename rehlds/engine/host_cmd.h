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
#include "eiface.h"
#include "FileSystem.h"

#define FILETIME_TO_QWORD(ft) \
		((((uint64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime)

#define FILETIME_TO_PAIR(f,h)\
		(((uint64)f << 32) | h)

typedef void (*SV_SAVEGAMECOMMENT_FUNC)(char *, int);

typedef struct GAME_HEADER_s
{
	char mapName[32];
	char comment[80];
	int mapCount;
} GAME_HEADER;

typedef struct SAVE_HEADER_s
{
	int saveId;
	int version;
	int skillLevel;
	int entityCount;
	int connectionCount;
	int lightStyleCount;
	float time;
	char mapName[32];
	char skyName[32];
	int skyColor_r;
	int skyColor_g;
	int skyColor_b;
	float skyVec_x;
	float skyVec_y;
	float skyVec_z;
} SAVE_HEADER;

typedef struct SAVELIGHTSTYLE_s
{
	int index;
	char style[MAX_LIGHTSTYLE_SIZE];
} SAVELIGHTSTYLE;

typedef struct TITLECOMMENT_s
{
	char *pBSPName;
	char *pTitleName;
} TITLECOMMENT;

extern int r_dointerp;
extern vec3_t r_origin;
extern double cpuPercent;
extern int32 startTime;
extern int current_skill;
extern int gHostSpawnCount;
extern CareerStateType g_careerState;
extern SV_SAVEGAMECOMMENT_FUNC g_pSaveGameCommentFunc;
extern qboolean g_bMajorMapChange;
extern cvar_t voice_recordtofile;
extern cvar_t voice_inputfromfile;
extern TITLECOMMENT gTitleComments[66];
extern TYPEDESCRIPTION gGameHeaderDescription[3];
extern TYPEDESCRIPTION gSaveHeaderDescription[13];
extern TYPEDESCRIPTION gAdjacencyDescription[4];
extern TYPEDESCRIPTION gEntityTableDescription[5];
extern TYPEDESCRIPTION gLightstyleDescription[2];
extern cvar_t gHostMap;
extern int g_iQuitCommandIssued;
extern char *g_pPostRestartCmdLineArgs;

void SV_GetPlayerHulls(void);
void Host_InitializeGameDLL(void);
void Host_Motd_f(void);
void Host_Motd_Write_f(void);
int Host_GetStartTime(void);
void Host_UpdateStats(void);
void GetStatsString(char *buf, int bufSize);
void Host_Stats_f(void);
void Host_Quit_f(void);
void Host_Quit_Restart_f(void);
void Host_Status_Printf(qboolean conprint, qboolean log, char *fmt, ...);
void Host_Status_f(void);
void Host_Status_Formatted_f(void);
void Host_Ping_f(void);
void Host_Map(qboolean bIsDemo, char *mapstring, char *mapName, qboolean loadGame);
void Host_Map_f(void);
void Host_Career_f(void);
void Host_Maps_f(void);
void Host_Changelevel_f(void);
const char *Host_FindRecentSave(char *pNameBuf);
void Host_Restart_f(void);
void Host_Reload_f(void);
void Host_Reconnect_f(void);
char *Host_SaveGameDirectory(void);
void Host_SavegameComment(char *pszBuffer, int iSizeBuffer);
void Host_SaveAgeList(const char *pName, int count);
int Host_ValidSave(void);
SAVERESTOREDATA *SaveInit(int size);
void SaveExit(SAVERESTOREDATA *save);
qboolean SaveGameSlot(const char *pSaveName, const char *pSaveComment);
void Host_Savegame_f(void);
void Host_AutoSave_f(void);
qboolean SaveGame(const char *pszSlot, const char *pszComment);
int SaveReadHeader(FileHandle_t pFile, GAME_HEADER *pHeader, int readGlobalState);
void SaveReadComment(FileHandle_t f, char *name);
void Host_Loadgame_f(void);
int LoadGame(const char *pName);
int Host_Load(const char *pName);
SAVERESTOREDATA *SaveGamestate(void);
void EntityInit(edict_t *pEdict, int className);
SAVERESTOREDATA *LoadSaveData(const char *level);
void ParseSaveTables(SAVERESTOREDATA *pSaveData, SAVE_HEADER *pHeader, int updateGlobals);
void EntityPatchWrite(SAVERESTOREDATA *pSaveData, const char *level);
void EntityPatchRead(SAVERESTOREDATA *pSaveData, const char *level);
int LoadGamestate(char *level, int createPlayers);
int EntryInTable(SAVERESTOREDATA *pSaveData, const char *pMapName, int index);
void LandmarkOrigin(SAVERESTOREDATA *pSaveData, vec_t *output, const char *pLandmarkName);
int EntityInSolid(edict_t *pent);
int CreateEntityList(SAVERESTOREDATA *pSaveData, int levelMask);
void LoadAdjacentEntities(const char *pOldLevel, const char *pLandmarkName);
int FileSize(FileHandle_t pFile);
void FileCopy(FileHandle_t pOutput, FileHandle_t pInput, int fileSize);
void DirectoryCopy(const char *pPath, FileHandle_t pFile);
void DirectoryExtract(FileHandle_t pFile, int fileCount);
int DirectoryCount(const char *pPath);
void Host_ClearSaveDirectory(void);
void Host_ClearGameState(void);
void Host_Changelevel2_f(void);
void Host_Version_f(void);
void Host_FullInfo_f(void);
NOXREF void Host_KillVoice_f(void);
void Host_SetInfo_f(void);
void Host_Say(qboolean teamonly);
void Host_Say_f(void);
void Host_Say_Team_f(void);
void Host_Tell_f(void);
void Host_Kill_f(void);
void Host_TogglePause_f(void);
void Host_Pause_f(void);
void Host_Unpause_f(void);
void Host_Pause_api(bool setPause);
void Host_Interp_f(void);
void Host_NextDemo(void);
void Host_Startdemos_f(void);
void Host_Demos_f(void);
void Host_Stopdemo_f(void);
NOXREF void Host_EndSection(const char *pszSection);
void Host_Soundfade_f(void);
void Host_KillServer_f(void);
void Host_VoiceRecordStart_f(void);
void Host_VoiceRecordStop_f(void);
void Host_Crash_f(void);

#ifdef REHLDS_FIXES
void Host_ResourcesList_f();
void Host_ResourcesCount_f();
#endif

void Host_InitCommands(void);
void SV_CheckBlendingInterface(void);
void SV_CheckSaveGameCommentInterface(void);
