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

void(*Launcher_ConsolePrintf)(char *, ...);
char *(*Launcher_GetLocalizedString)(unsigned int);
int(*Launcher_MP3subsys_Suspend_Audio)(void);
void(*Launcher_MP3subsys_Resume_Audio)(void);
void(*VID_FlipScreen)(void);

//double curtime;
//double lastcurtime;
//qboolean sc_return_on_enter;

//char findbase[260];
//char findname[260];
//char findpath[260];
//char findpattern[260];

//const char g_szExtensionDllSubDir;
//const char g_szHalfLifeDllName;
//const char g_szBaseDllName;

char gszDisconnectReason[MAX_DISCONNECT_REASON];
char gszExtendedDisconnectReason[MAX_DISCONNECT_REASON];

qboolean gfExtendedError;
qboolean g_bIsDedicatedServer;

int giSubState;
int giActive;
int giStateInfo;
DLL_FUNCTIONS gEntityInterface;
NEW_DLL_FUNCTIONS gNewDLLFunctions;

extensiondll_t g_rgextdll[MAX_EXTENSION_DLL];

int g_iextdllMac;
modinfo_t gmodinfo;
qboolean gfBackground;
//int starttime;
//qboolean Win32AtLeastV4;
//int lowshift;
//double pfreq;
qboolean g_bPrintingKeepAliveDots;
//DWORD gProcessorSpeed;
#ifndef _WIN32
qboolean gHasMMXTechnology;
#endif // _WIN32
//volatile int sys_checksum;
//char *argv[MAX_NUM_ARGVS];
qboolean con_debuglog;

#ifdef REHLDS_FIXES
char g_szFindFirstFileName[MAX_PATH];
#endif

#ifdef _WIN32
int g_PerfCounterInitialized;
CRITICAL_SECTION g_PerfCounterMutex;

int g_PerfCounterShiftRightAmount;
double g_PerfCounterSlice;
double g_CurrentTime;
double g_StartTime;

int g_WinNTOrHigher;
#endif // _WIN32

int g_FPUCW_Mask_Prec_64Bit = 0;
int g_FPUCW_Mask_Prec_64Bit_2 = 0;
int g_FPUCW_Mask_Round_Trunc = 0;
int g_FPUCW_Mask_Round_Up = 0;

FileFindHandle_t g_hfind = FILESYSTEM_INVALID_FIND_HANDLE;

enginefuncs_t g_engfuncsExportedToDlls = {
	PF_precache_model_I, PF_precache_sound_I,
	PF_setmodel_I, PF_modelindex,
	ModelFrames, PF_setsize_I,
	PF_changelevel_I, PF_setspawnparms_I,
	SaveSpawnParms, PF_vectoyaw_I,
	PF_vectoangles_I, SV_MoveToOrigin_I,
	PF_changeyaw_I, PF_changepitch_I,
	FindEntityByString, GetEntityIllum,
	FindEntityInSphere, PF_checkclient_I,
	PVSFindEntities, PF_makevectors_I,
	AngleVectors_ext, PF_Spawn_I,
	PF_Remove_I, CreateNamedEntity,
	PF_makestatic_I, PF_checkbottom_I,
	PF_droptofloor_I, PF_walkmove_I,
	PF_setorigin_I, PF_sound_I,
	PF_ambientsound_I, PF_traceline_DLL,
	PF_TraceToss_DLL, TraceMonsterHull,
	TraceHull, TraceModel,
	TraceTexture, TraceSphere,
	PF_aim_I, PF_localcmd_I,
	PF_localexec_I, PF_stuffcmd_I,
	PF_particle_I, PF_lightstyle_I,
	PF_DecalIndex, PF_pointcontents_I,
	PF_MessageBegin_I, PF_MessageEnd_I,
	PF_WriteByte_I, PF_WriteChar_I,
	PF_WriteShort_I, PF_WriteLong_I,
	PF_WriteAngle_I, PF_WriteCoord_I,
	PF_WriteString_I, PF_WriteEntity_I,
	CVarRegister, CVarGetFloat,
	CVarGetString, CVarSetFloat,
	CVarSetString, AlertMessage,
	EngineFprintf, PvAllocEntPrivateData,
	PvEntPrivateData, FreeEntPrivateData,
	SzFromIndex, AllocEngineString,
	GetVarsOfEnt, PEntityOfEntOffset,
	EntOffsetOfPEntity, IndexOfEdict,
	PEntityOfEntIndex, FindEntityByVars,
	GetModelPtr, SV_RegUserMsg,
	AnimationAutomove, GetBonePosition,
	FunctionFromName, NameForFunction,
	ClientPrintf, ServerPrint,
	Cmd_Args, Cmd_Argv, Cmd_Argc,
	GetAttachment, CRC32_Init,
	CRC32_ProcessBuffer, CRC32_ProcessByte,
	CRC32_Final, RandomLong,
	RandomFloat, PF_setview_I,
	PF_Time, PF_crosshairangle_I,
	COM_LoadFileForMe, COM_FreeFile,
	Host_EndSection, COM_CompareFileTime,
	COM_GetGameDir, Cvar_RegisterVariable,
	PF_FadeVolume, PF_SetClientMaxspeed,
	PF_CreateFakeClient_I,
	PF_RunPlayerMove_I,
	PF_NumberOfEntities_I,
	PF_GetInfoKeyBuffer_I, PF_InfoKeyValue_I,
	PF_SetKeyValue_I, PF_SetClientKeyValue_I,
	PF_IsMapValid_I, PF_StaticDecal,
	PF_precache_generic_I,
	PF_GetPlayerUserId, PF_BuildSoundMsg_I,
	PF_IsDedicatedServer, CVarGetPointer,
	PF_GetPlayerWONId, PF_RemoveKey_I,
	PF_GetPhysicsKeyValue,
	PF_SetPhysicsKeyValue,
	PF_GetPhysicsInfoString, EV_Precache,
	EV_Playback, SV_FatPVS, SV_FatPAS,
	SV_CheckVisibility, DELTA_SetField,
	DELTA_UnsetField, DELTA_AddEncoder,
	PF_GetCurrentPlayer, PF_CanSkipPlayer,
	DELTA_FindFieldIndex,
	DELTA_SetFieldByIndex,
	DELTA_UnsetFieldByIndex, PF_SetGroupMask,
	PF_CreateInstancedBaseline,
	PF_Cvar_DirectSet, PF_ForceUnmodified,
	PF_GetPlayerStats, Cmd_AddGameCommand,
	Voice_GetClientListening,
	Voice_SetClientListening,
	PF_GetPlayerAuthId, NULL,
	NULL, COM_FileSize,
	COM_GetApproxWavePlayLength,
	VGuiWrap2_IsInCareerMatch,
	VGuiWrap2_GetLocalizedStringLength,
	RegisterTutorMessageShown,
	GetTimesTutorMessageShown,
	ProcessTutorMessageDecayBuffer,
	ConstructTutorMessageDecayBuffer,
	ResetTutorMessageDecayData,
	QueryClientCvarValue, QueryClientCvarValue2,
	EngCheckParm
};

#ifdef _WIN32
void Sys_SetupFPUOptions()
{
#ifndef __SSE__
	static uint8 fpuOpts[32];

	__asm { fnstenv byte ptr fpuOpts }
	fpuOpts[0] |= 0x3Fu;
	__asm { fldenv  byte ptr fpuOpts }
#endif
}

void Sys_InitFPUControlWords()
{
#ifndef __SSE__
	int fpucw = 0;
	__asm { fnstcw fpucw }

	g_FPUCW_Mask_Prec_64Bit = (fpucw & 0xF0FF) | 0x300;
	g_FPUCW_Mask_Prec_64Bit_2 = (fpucw & 0xF0FF) | 0x300;
	g_FPUCW_Mask_Round_Trunc = (fpucw & 0xF0FF) | 0xC00;
	g_FPUCW_Mask_Round_Up = (fpucw & 0xF0FF) | 0x800;
#endif
}

void Sys_SetStartTime()
{
	int startTimeArg;

	Sys_FloatTime();
	startTimeArg = COM_CheckParm("-starttime");
	if (startTimeArg)
		g_CurrentTime = Q_atof(com_argv[startTimeArg + 1]);
	else
		g_CurrentTime = 0;

	g_StartTime = g_CurrentTime;
}

void __cdecl Sys_InitHardwareTimer()
{
	unsigned int perfHighPart;
	unsigned int perfLowPart;
	LARGE_INTEGER perfFreq;

	if (!g_PerfCounterInitialized)
	{
		InitializeCriticalSection(&g_PerfCounterMutex);
		g_PerfCounterInitialized = 1;
	}
	Sys_SetupFPUOptions();
	Sys_InitFPUControlWords();

	if (!CRehldsPlatformHolder::get()->QueryPerfFreq(&perfFreq))
		Sys_Error("%s: No hardware timer available", __func__);

	perfHighPart = perfFreq.HighPart;
	perfLowPart = perfFreq.LowPart;
	g_PerfCounterShiftRightAmount = 0;
	while (perfHighPart || perfLowPart > 2000000.0)
	{
		g_PerfCounterShiftRightAmount++;
		perfLowPart = (perfHighPart << 31) | (perfLowPart >> 1);
		perfHighPart >>= 1;
	}
	g_PerfCounterSlice = 1.0 / (double)perfLowPart;

	Sys_CheckOSVersion();
	Sys_SetStartTime();
}

int g_SavedFPUCW1 = 0;
void Sys_FPUCW_Push_Prec64() {
#ifndef __SSE__
	uint16 tmp = g_FPUCW_Mask_Prec_64Bit;
	__asm { fnstcw  g_SavedFPUCW1 }
	__asm { fldcw tmp }
#endif
}

void Sys_FPUCW_Pop_Prec64() {
#ifndef __SSE__
	uint16 tmp = g_SavedFPUCW1;
	__asm { fldcw tmp }
#endif
}

#endif // _WIN32

NOXREF void Sys_PageIn(void *ptr, int size)
{
	NOXREFCHECK;
}

// TODO: investigate filesystem_stdio problem (multiple enumeration of files).
const char *Sys_FindFirst(const char *path, char *basename)
{
	if (g_hfind != -1)
	{
		Sys_Error("%s: called without close", __func__);
	}

	const char *psz = FS_FindFirst(path, &g_hfind, 0);

#ifdef REHLDS_FIXES
	// Hack: store first file name to fix multiple enumeration of files in the filesystem module
	if (psz != NULL)
	{
		Q_strncpy(g_szFindFirstFileName, psz, MAX_PATH - 1);
	}
#endif // REHLDS_FIXES

	if (basename && psz)
	{
		COM_FileBase((char *)psz, basename);
	}

	return psz;
}

const char *Sys_FindFirstPathID(const char *path, char *pathid)
{
	//const char *psz;//unused?
	if (g_hfind != -1)
	{
		Sys_Error("%s: called without close", __func__);
	}

	return FS_FindFirst(path, &g_hfind, pathid);
}

// TODO: investigate filesystem_stdio problem (multiple enumeration of files).
const char *Sys_FindNext(char *basename)
{
	const char *psz = FS_FindNext(g_hfind);

#ifdef REHLDS_FIXES
	// Hack: stop if we are starting over again
	if (psz && !Q_strcmp(g_szFindFirstFileName, psz))
	{
		return NULL;
	}
#endif // REHLDS_FIXES

	if (basename && psz)
	{
		COM_FileBase((char *)psz, basename);
	}

	return psz;
}

void Sys_FindClose(void)
{
	if (g_hfind != -1)
	{
		FS_FindClose(g_hfind);
		g_hfind = -1;
	}

#ifdef REHLDS_FIXES
	g_szFindFirstFileName[0] = 0;
#endif // REHLDS_FIXES
}

NOBODY int glob_match_after_star(char *pattern, char *text);
//{
//	char *p;                                                     //   282
//	char *t;                                                     //   282
//	char c;                                                       //   283
//	char c1;                                                      //   283
//}

NOBODY int glob_match(char *pattern, char *text);
//{
//	char *p;                                                     //   325
//	char *t;                                                     //   325
//	char c;                                                       //   326
//
//match :                                                                //   386;
//	{
//		char c1;                                              //   347
//		int invert;                                           //   348
//		{
//			char cstart;                                  //   359
//			char cend;                                    //   359
//		}
//	}
//	glob_match_after_star(char *pattern,
//				char *text);   //   343
//}

NOXREF void Sys_MakeCodeWriteable(uint32 startaddr, uint32 length)
{
	NOXREFCHECK;
#ifdef _WIN32
	if (!VirtualProtect((LPVOID)startaddr, length, PAGE_EXECUTE_READWRITE, (PDWORD)&length))
		Sys_Error("%s: Protection change failed.", __func__);
#endif // _WIN32
}

NOBODY void Sys_SetFPCW(void);
//{
//}

NOBODY void Sys_PushFPCW_SetHigh(void);
//{
//}

NOBODY void Sys_PopFPCW(void);
//{
//}

NOBODY void MaskExceptions(void);
//{
//}

NOBODY void Sys_Init(void);

void Sys_Sleep(int msec)
{
#ifdef _WIN32
	Sleep(msec);
#else
	usleep(1000 * msec);
#endif // _WIN32
}

NOBODY void Sys_DebugOutStraight(const char *pStr);
//{
//}

void NORETURN Sys_Error(const char *error, ...)
{
	va_list argptr;
	char text[1024];
	static qboolean bReentry;

	va_start(argptr, error);
	Q_vsnprintf(text, ARRAYSIZE(text), error, argptr);
	va_end(argptr);

#ifdef _WIN32
	MessageBox(GetForegroundWindow(), text, "Fatal error - Dedicated server", MB_ICONERROR | MB_OK);
#endif // _WIN32

	if (bReentry)
	{
		fprintf(stderr, "%s\n", text);
		longjmp(host_abortserver, 2);
	}
	bReentry = true;

	if (g_psvs.dll_initialized && gEntityInterface.pfnSys_Error)
		gEntityInterface.pfnSys_Error(text);

	Log_Printf("FATAL ERROR (shutting down): %s\n", text);

#ifdef REHLDS_FIXES
	if (syserror_logfile.string[0] != '\0')
	{
		auto pFile = FS_Open(syserror_logfile.string, "a");
		if (pFile)
		{
			tm *today;
			time_t ltime;
			char szDate[32];

			time(&ltime);
			today = localtime(&ltime);
			strftime(szDate, ARRAYSIZE(szDate) - 1, "L %d/%m/%Y - %H:%M:%S:", today);

			FS_FPrintf(pFile, "%s (map \"%s\") %s\n", szDate, &pr_strings[gGlobalVariables.mapname], text);
			FS_Close(pFile);
		}
	}
#endif // REHLDS_FIXES

	if (g_bIsDedicatedServer)
	{
		if (Launcher_ConsolePrintf)
			Launcher_ConsolePrintf("FATAL ERROR (shutting down): %s\n", text);
		else
			printf("FATAL ERROR (shutting down): %s\n", text);
	}
#ifndef SWDS
	else
	{
		HWND hWnd = 0;
		if (pmainwindow)
			hWnd = *pmainwindow;

		Sys_Printf(text);
		SDL_ShowSimpleMessageBox(MB_ICONERROR | MB_OK, "Fatal Error", text, hWnd);
		VideoMode_IsWindowed();
	}
#endif // SWDS

	//Allahu akbar!
	volatile int *null = 0;
	*null = 0;
	exit(-1);
}

NOXREF void Sys_Warning(const char *pszWarning, ...)
{
	NOXREFCHECK;
	va_list argptr;
	char text[1024];

	va_start(argptr, pszWarning);
	Q_vsnprintf(text, sizeof(text), pszWarning, argptr);
	va_end(argptr);

	Sys_Printf(text);
}

void Sys_Printf(const char *fmt, ...)
{
	char Dest[1024];
	va_list va;

	va_start(va, fmt);
	Q_vsnprintf(Dest, sizeof(Dest), fmt, va);
	va_end(va);

	if (g_bIsDedicatedServer && Launcher_ConsolePrintf)
		Launcher_ConsolePrintf("%s", Dest);

#ifdef _WIN32
	OutputDebugStringA(Dest);
#else
	if (!g_bIsDedicatedServer)
		fprintf(stderr, "%s\n", Dest);
#endif // _WIN32

}

void Sys_Quit(void)
{
	giActive = DLL_CLOSE;
}

#ifdef _WIN32

double EXT_FUNC Sys_FloatTime(void)
{
	unsigned int currentTime;
	int savedOldTime;
	LARGE_INTEGER PerformanceCount;

	static bool s_NeedInit = true;
	static unsigned int s_oldTime = 0;
	static int s_timeNotChangedCounter = 0;

	if (!g_PerfCounterInitialized)
		return 1.0;

	EnterCriticalSection(&g_PerfCounterMutex);
	Sys_FPUCW_Push_Prec64();

	CRehldsPlatformHolder::get()->QueryPerfCounter(&PerformanceCount);
	if (g_PerfCounterShiftRightAmount)
		currentTime = (PerformanceCount.LowPart >> g_PerfCounterShiftRightAmount) | (PerformanceCount.HighPart << (32 - g_PerfCounterShiftRightAmount));
	else
		currentTime = PerformanceCount.LowPart;

	if (!s_NeedInit)
	{
		savedOldTime = s_oldTime;
		if (currentTime <= s_oldTime && s_oldTime - currentTime < 0x10000000)
		{
			s_oldTime = currentTime;
		}
		else
		{
			s_oldTime = currentTime;
			g_CurrentTime = g_CurrentTime + (double)(currentTime - savedOldTime) * g_PerfCounterSlice;
			if (g_CurrentTime == g_StartTime)
			{
				if (s_timeNotChangedCounter >= 100000)
				{
					g_CurrentTime = g_CurrentTime + 1.0;
					s_timeNotChangedCounter = 0;
				}
			}
			else
			{
				s_timeNotChangedCounter = 0;
			}
			g_StartTime = g_CurrentTime;
		}
	}
	else
	{
		s_oldTime = currentTime;
		s_NeedInit = false;
	}

	Sys_FPUCW_Pop_Prec64();
	LeaveCriticalSection(&g_PerfCounterMutex);
	return g_CurrentTime;
}

#else // not _WIN32

double Sys_FloatTime(void)
{
	static struct timespec start_time;
	static bool bInitialized = false;
	struct timespec now;

	if ( !bInitialized )
	{
		bInitialized = true;
		clock_gettime(CLOCK_MONOTONIC, &start_time);
	}
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (now.tv_sec - start_time.tv_sec) + now.tv_nsec * 0.000000001;
}

#endif //_WIN32

void Dispatch_Substate(int iSubState)
{
	giSubState = iSubState;
}

void GameSetSubState(int iSubState)
{
	if (iSubState & 2)
	{
		Dispatch_Substate(1);
	}
	else if (iSubState != 1)
	{
		Dispatch_Substate(iSubState);
	}
}

void GameSetState(int iState)
{
	giActive = iState;
}

NOBODY void GameSetBackground(qboolean bNewSetting);
//{
//}

qboolean EXT_FUNC Voice_GetClientListening(int iReceiver, int iSender)
{
	--iReceiver;
	--iSender;

	if (iReceiver < 0 || iSender < 0 || iReceiver >= g_psvs.maxclients || iSender >= g_psvs.maxclients)
		return 0;

#ifdef REHLDS_FIXES
	return (g_psvs.clients[iSender].m_VoiceStreams[iReceiver >> 5] & (1 << iReceiver)) != 0;
#else // REHLDS_FIXES
	return (1 << iReceiver) & (g_psvs.clients[iSender].m_VoiceStreams[iReceiver >> 5] != 0);
#endif // REHLDS_FIXES
}

qboolean EXT_FUNC Voice_SetClientListening(int iReceiver, int iSender, qboolean bListen)
{
	--iReceiver;
	--iSender;

	if (iReceiver < 0 || iSender < 0 || iReceiver >= g_psvs.maxclients || iSender >= g_psvs.maxclients)
		return 0;

	uint32 *pDest = g_psvs.clients[iSender].m_VoiceStreams;
	if (bListen)
	{
		pDest[iReceiver >> 5] |= 1 << iReceiver;
	}
	else
	{
		pDest[iReceiver >> 5] &= ~(1 << iReceiver);
	}

	return 1;
}

DISPATCHFUNCTION GetDispatch(char *pname)
{
	int i;
	DISPATCHFUNCTION pDispatch;

	for (i = 0; i < g_iextdllMac; i++)
	{
		pDispatch = (DISPATCHFUNCTION)GetProcAddress((HMODULE)g_rgextdll[i].lDLLHandle, pname);
		if (pDispatch)
		{
			return pDispatch;
		}
	}

	return NULL;
}

const char *FindAddressInTable(extensiondll_t *pDll, uint32 function)
{
#ifdef _WIN32
	for (int i = 0; i < pDll->functionCount; i++)
	{
		if (pDll->functionTable[i].pFunction == function)
		{
			return pDll->functionTable[i].pFunctionName;
		}
	}
#else // _WIN32
	Dl_info addrInfo;
	if (dladdr((void *)function, &addrInfo))
	{
		return addrInfo.dli_sname;
	}
#endif // _WIN32

	return NULL;
}

uint32 FindNameInTable(extensiondll_t *pDll, const char *pName)
{
#ifdef _WIN32
	for (int i = 0; i < pDll->functionCount; i++)
	{
		if (!Q_strcmp(pName, pDll->functionTable[i].pFunctionName))
		{
			return pDll->functionTable[i].pFunction;
		}
	}
	return NULL;
#else
	return (uint32)dlsym(pDll->lDLLHandle, pName);
#endif // _WIN32
}

NOBODY const char *ConvertNameToLocalPlatform(const char *pchInName);
//{
//	char s_szNewName;                                             //  1409
//	{
//		char szTempName;                                      //  1463
//		char *pchAt;                                         //  1464
//		char *pchClassName;                                  //  1465
//		char *pchFunctionName;                               //  1466
//		FindNameInTable(extensiondll_t *pDll,
//				const char *pName);  / //  1483
//		FindNameInTable(extensiondll_t *pDll,
//				const char *pName);   //  1487
//		FindNameInTable(extensiondll_t *pDll,
//				const char *pName);   //  1491
//	}
//}

uint32 EXT_FUNC FunctionFromName(const char *pName)
{
	return 0; //TODO: do we really need to reverse it?
}

const char* EXT_FUNC NameForFunction(uint32 function)
{
	int i;
	const char *pName;

	for (i = 0; i < g_iextdllMac; i++)
	{
		pName = FindAddressInTable(&g_rgextdll[i], function);
		if (pName)
		{
			return pName;
		}
	}

	Con_Printf("Can't find address: %08lx\n", function);
	return NULL;
}

ENTITYINIT GetEntityInit_internal(char *pClassName)
{
	return (ENTITYINIT)GetDispatch(pClassName);
}

ENTITYINIT EXT_FUNC GetEntityInit_api(char *pClassName)
{
	return g_RehldsHookchains.m_GetEntityInit.callChain(GetEntityInit_internal, pClassName);
}

ENTITYINIT GetEntityInit(char *pClassName)
{
	return GetEntityInit_api(pClassName);
}

FIELDIOFUNCTION GetIOFunction(char *pName)
{
	return (FIELDIOFUNCTION)GetDispatch(pName);
}

void DLL_SetModKey(modinfo_t *pinfo, char *pkey, char *pvalue)
{
	if (!Q_stricmp(pkey, "url_info"))
	{
		pinfo->bIsMod = 1;
		Q_strncpy(pinfo->szInfo, pvalue, sizeof(pinfo->szInfo) - 1);
		pinfo->szInfo[sizeof(pinfo->szInfo) - 1] = 0;
	}
	else if (!Q_stricmp(pkey, "url_dl"))
	{
		pinfo->bIsMod = 1;
		Q_strncpy(pinfo->szDL, pvalue, sizeof(pinfo->szDL) - 1);
		pinfo->szDL[sizeof(pinfo->szDL) - 1] = 0;
	}
	else if (!Q_stricmp(pkey, "version"))
	{
		pinfo->bIsMod = 1;
		pinfo->version = Q_atoi(pvalue);
	}
	else if (!Q_stricmp(pkey, "size"))
	{
		pinfo->bIsMod = 1;
		pinfo->size = Q_atoi(pvalue);
	}
	else if (!Q_stricmp(pkey, "svonly"))
	{
		pinfo->bIsMod = 1;
		pinfo->svonly = Q_atoi(pvalue) != 0;
	}
	else if (!Q_stricmp(pkey, "cldll"))
	{
		pinfo->bIsMod = 1;
		pinfo->cldll = Q_atoi(pvalue) != 0;
	}
	else if (!Q_stricmp(pkey, "secure"))
	{
		pinfo->bIsMod = 1;
		pinfo->secure = Q_atoi(pvalue) != 0;
	}
	else if (!Q_stricmp(pkey, "hlversion"))
	{
		Q_strncpy(pinfo->szHLVersion, pvalue, sizeof(pinfo->szHLVersion) - 1);
		pinfo->szHLVersion[sizeof(pinfo->szHLVersion) - 1] = 0;
	}
	else if (!Q_stricmp(pkey, "edicts"))
	{
		pinfo->num_edicts = Q_atoi(pvalue);
		if (pinfo->num_edicts < NUM_EDICTS)
			pinfo->num_edicts = NUM_EDICTS;
	}
	else if (!Q_stricmp(pkey, "crcclientdll"))
	{
		pinfo->bIsMod = 1;
		pinfo->clientcrccheck = Q_atoi(pvalue) != 0;
	}
	else if (!Q_stricmp(pkey, "type"))
	{
		if (!Q_stricmp(pvalue, "singleplayer_only"))
			pinfo->type = SINGLEPLAYER_ONLY;
		else if (!Q_stricmp(pvalue, "multiplayer_only"))
			pinfo->type = MULTIPLAYER_ONLY;
		else
			pinfo->type = BOTH;
	}
	else if (!Q_stricmp(pkey, "fallback_dir"))
	{
		COM_AddDefaultDir(pvalue);
	}

}

void LoadEntityDLLs(const char *szBaseDir)
{
	FileHandle_t hLibListFile;
	unsigned int nFileSize;
	unsigned int nFileSize2;
	char *pszInputStream;
	int nBytesRead;
	char *pStreamPos;
	const char *findfn;
	NEW_DLL_FUNCTIONS_FN pNewAPI;
	APIFUNCTION2 pfnGetAPI2;
	APIFUNCTION pfnGetAPI;
	char szDllFilename[8192];
	char szDllWildcard[260];
	char szDllListFile[260];
	char szValue[256];
	char szKey[64];
	char szGameDir[64];
	int interface_version;

	SV_ResetModInfo();
	g_iextdllMac = 0;
	Q_memset(g_rgextdll, 0, sizeof(g_rgextdll));

	Q_strncpy(szGameDir, com_gamedir, sizeof(szGameDir) - 1);
	if (Q_stricmp(szGameDir, "valve"))
		gmodinfo.bIsMod = 1;

	Q_snprintf(szDllListFile, sizeof(szDllListFile), "%s", "liblist.gam");
	hLibListFile = FS_Open(szDllListFile, "rb");
	if (hLibListFile)
	{
		nFileSize = FS_Size(hLibListFile);
		nFileSize2 = nFileSize;
		if (!nFileSize || (signed int)nFileSize > 262144)
			Sys_Error("%s: Game listing file size is bogus [%s: size %i]", __func__, "liblist.gam", nFileSize);

		pszInputStream = (char *)Mem_Malloc(nFileSize + 1);
		if (!pszInputStream)
			Sys_Error("%s: Could not allocate space for game listing file of %i bytes", __func__, nFileSize2 + 1);

		nBytesRead = FS_Read(pszInputStream, nFileSize2, 1, hLibListFile);
		if (nBytesRead != nFileSize2)
			Sys_Error("%s: Error reading in game listing file, expected %i bytes, read %i", __func__, nFileSize2, nBytesRead);

		pszInputStream[nFileSize2] = 0;
		pStreamPos = pszInputStream;
		com_ignorecolons = 1;
		while (1)
		{
			pStreamPos = COM_Parse(pStreamPos);
			if (Q_strlen(com_token) <= 0)
				break;

			Q_strncpy(szKey, com_token, sizeof(szKey) - 1);
			szKey[sizeof(szKey) - 1] = 0;
			pStreamPos = COM_Parse(pStreamPos);
			Q_strncpy(szValue, com_token, sizeof(szValue) - 1);
			szValue[sizeof(szValue) - 1] = 0;
#ifdef _WIN32
			if (Q_stricmp(szKey, "gamedll"))
#else // _WIN32
			if (Q_stricmp(szKey, "gamedll_linux"))
#endif // _WIN32
			{
				DLL_SetModKey(&gmodinfo, szKey, szValue);
			}
			else
			{
				int index = COM_CheckParm("-dll");
				if (index && index < com_argc - 1)
				{
					Q_strncpy(szValue, com_argv[index + 1], sizeof(szValue) - 1);
					szValue[sizeof(szValue) - 1] = 0;
				}
#ifdef REHLDS_FIXES
				char *value_extension = Q_strrchr(szValue, '.');
#ifdef _WIN32
				if (value_extension && Q_strcmp(value_extension, ".dll") == 0)
#else // _WIN32
				if (value_extension && Q_strcmp(value_extension, ".so") == 0)
#endif // _WIN32

#else // REHLDS_FIXES
#ifdef _WIN32
				if (Q_strstr(szValue, ".dll"))
#else // _WIN32
				if (Q_strstr(szValue, ".so"))
#endif // _WIN32

#endif // REHLDS_FIXES
				{
					FS_GetLocalPath(szValue, szDllFilename, sizeof(szDllFilename));
					Con_DPrintf("\nAdding:  %s/%s\n", szGameDir, szValue);
					LoadThisDll(szDllFilename);
				}
				else
				{
					Con_DPrintf("Skipping non-dll:  %s\n", szValue);
				}
			}
		}
		com_ignorecolons = 0;
		Mem_Free(pszInputStream);
		FS_Close(hLibListFile);
	}
	else
	{
#ifdef _WIN32
		Q_snprintf(szDllWildcard, sizeof(szDllWildcard), "%s\\*.dll", "valve\\dlls");
#else // _WIN32
		Q_snprintf(szDllWildcard, sizeof(szDllWildcard), "%s\\*.so", "valve\\dlls");
#endif // _WIN32
		for (findfn = Sys_FindFirst(szDllWildcard, 0); findfn; findfn = Sys_FindNext(0))
		{
			Q_snprintf(szDllFilename, sizeof(szDllWildcard), "%s/%s/%s", szBaseDir, "valve\\dlls", findfn);
			LoadThisDll(szDllFilename);
		}
		Sys_FindClose();
	}
	gNewDLLFunctions.pfnOnFreeEntPrivateData = NULL;
	gNewDLLFunctions.pfnGameShutdown = NULL;
	gNewDLLFunctions.pfnShouldCollide = NULL;
	gNewDLLFunctions.pfnCvarValue = NULL;
	gNewDLLFunctions.pfnCvarValue2 = NULL;
	pNewAPI = (NEW_DLL_FUNCTIONS_FN)GetDispatch("GetNewDLLFunctions");
	if (pNewAPI)
	{
		interface_version = NEW_DLL_FUNCTIONS_VERSION;
		pNewAPI(&gNewDLLFunctions, &interface_version);
	}

	pfnGetAPI2 = (APIFUNCTION2)GetDispatch("GetEntityAPI2");
	if (pfnGetAPI2)
	{
		interface_version = INTERFACE_VERSION;
		if (!pfnGetAPI2(&gEntityInterface, &interface_version))
		{
			Con_Printf("==================\n");
			Con_Printf("Game DLL version mismatch\n");
			Con_Printf("DLL version is %i, engine version is %i\n", interface_version, INTERFACE_VERSION);
			if (interface_version <= INTERFACE_VERSION)
				Con_Printf("The game DLL for %s appears to be outdated, check for updates\n", szGameDir);
			else
				Con_Printf("Engine appears to be outdated, check for updates\n");
			Con_Printf("==================\n");
			Host_Error("\n");
		}
	}
	else
	{
		pfnGetAPI = (APIFUNCTION)GetDispatch("GetEntityAPI");
		if (!pfnGetAPI)
			Host_Error("Couldn't get DLL API from %s!", szDllFilename);
		interface_version = INTERFACE_VERSION;
		if (!pfnGetAPI(&gEntityInterface, interface_version))
		{
			Con_Printf("==================\n");
			Con_Printf("Game DLL version mismatch\n");
			Con_Printf("The game DLL for %s appears to be outdated, check for updates\n", szGameDir);
			Con_Printf("==================\n");
			Host_Error("\n");
		}
	}

	Con_DPrintf("Dll loaded for %s %s\n", gmodinfo.bIsMod ? "mod" : "game", gEntityInterface.pfnGetGameDescription());
}

#ifdef _WIN32
HMODULE LoadWindowsDLL(LPCSTR lpLibFileName)
{
	if (!lpLibFileName)
		return NULL;

	FS_GetLocalCopy(lpLibFileName);
	return LoadLibraryA(lpLibFileName);
}
#endif //_WIN32

void LoadThisDll(const char *szDllFilename)
{
#ifdef _WIN32
	typedef void(__stdcall *PFN_GiveFnptrsToDll)(enginefuncs_t *, globalvars_t *);
#else
	typedef void(__cdecl *PFN_GiveFnptrsToDll)(enginefuncs_t *, globalvars_t *);
#endif // _WIN32
	PFN_GiveFnptrsToDll pfnGiveFnptrsToDll;
	extensiondll_t *pextdll;

#ifdef _WIN32
	HMODULE hDLL = LoadWindowsDLL(szDllFilename);
	if (!hDLL)
	{
		Con_Printf("LoadLibrary failed on %s (%d)\n", szDllFilename, GetLastError());
		goto IgnoreThisDLL;
	}
#else // _WIN32
#ifdef REHLDS_FIXES
	void *hDLL = dlopen(szDllFilename, RTLD_NOW | RTLD_DEEPBIND | RTLD_LOCAL);
#else // REHLDS_FIXES
	void *hDLL = dlopen(szDllFilename, RTLD_NOW);
#endif // REHLDS_FIXES
	if (!hDLL)
	{
		Con_Printf("LoadLibrary failed on %s: %s\n", szDllFilename, dlerror());
		goto IgnoreThisDLL;
	}
#endif


#ifdef _WIN32
	pfnGiveFnptrsToDll = (PFN_GiveFnptrsToDll)GetProcAddress(hDLL, "GiveFnptrsToDll");
#else
	pfnGiveFnptrsToDll = (PFN_GiveFnptrsToDll) dlsym(hDLL, "GiveFnptrsToDll");
#endif // _WIN32

	if (pfnGiveFnptrsToDll == NULL)
	{
		Con_Printf("Couldn't get GiveFnptrsToDll in %s\n", szDllFilename);
		goto IgnoreThisDLL;
	}

#ifdef REHLDS_API
	MessageManager().Init();
#endif

	pfnGiveFnptrsToDll(&g_engfuncsExportedToDlls, &gGlobalVariables);
	if (g_iextdllMac == MAX_EXTENSION_DLL)
	{
		Con_Printf("Too many DLLs, ignoring remainder\n");
		goto IgnoreThisDLL;
	}

	pextdll = &g_rgextdll[g_iextdllMac++];
	Q_memset(pextdll, 0, sizeof(*pextdll));
	pextdll->lDLLHandle = hDLL;
	return;

IgnoreThisDLL:
	if (hDLL)
	{
#ifdef _WIN32
		FreeLibrary(hDLL);
#else
		dlclose(hDLL);
#endif // _WIN32
	}
}

void ReleaseEntityDlls(void)
{
	extensiondll_t *pextdll;
	extensiondll_t *pextdllMac;

	if (!g_psvs.dll_initialized)
		return;

	FreeAllEntPrivateData();

	if (gNewDLLFunctions.pfnGameShutdown)
		gNewDLLFunctions.pfnGameShutdown();

	Cvar_UnlinkExternals();

	pextdll = &g_rgextdll[0];
	pextdllMac = &g_rgextdll[g_iextdllMac];

	while (pextdll < pextdllMac)
	{
#ifdef _WIN32
		FreeLibrary((HMODULE)pextdll->lDLLHandle);
#else
		dlclose(pextdll->lDLLHandle);
#endif // _WIN32

		pextdll->lDLLHandle = NULL;
		if (pextdll->functionTable)
		{
			Mem_Free((void *)pextdll->functionTable);
			pextdll->functionTable = NULL;
		}
		pextdll++;
	}
	g_psvs.dll_initialized = FALSE;
}

void EXT_FUNC EngineFprintf(void *pfile, const char *szFmt, ...)
{
	AlertMessage(at_console, "EngineFprintf:  Obsolete API\n");
}

void EXT_FUNC AlertMessage(ALERT_TYPE atype, const char *szFmt, ...)
{
	char szOut[2048];
	va_list argptr;

	if (atype == at_logged && g_psvs.maxclients > 1)
	{
		va_start(argptr, szFmt);
		Q_vsnprintf(szOut, sizeof(szOut), szFmt, argptr);
		va_end(argptr);

		Log_Printf("%s", szOut);
		return;
	}

	if (!developer.value)
		return;

	if (atype == at_aiconsole && developer.value < 2)
		return;

	va_start(argptr, szFmt);
	Q_vsnprintf(szOut, sizeof(szOut), szFmt, argptr);
	va_end(argptr);

	switch (atype)
	{
	case at_notice:
		Con_Printf("NOTE:  %s", szOut);
		break;
	case at_console:
	case at_aiconsole:
		Con_Printf("%s", szOut);
		break;
	case at_warning:
		Con_Printf("WARNING:  %s", szOut);
		break;
	case at_error:
		Con_Printf("ERROR:  %s", szOut);
		break;
	default:
		break;
	}
}

NOXREF void Sys_SplitPath(const char *path, char *drive, char *dir, char *fname, char *ext)
{
	NOXREFCHECK;
#ifdef _WIN32
	_splitpath(path, drive, dir, fname, ext);
#else // _WIN32

	char *p;
	char *last_slash = NULL;
	char *dot = NULL;
	unsigned int len;

	if (path[0] && path[1] == ':')
	{
		if (drive)
		{
			Q_strncpy(drive, path, 2);
			drive[2] = 0;
		}
		path += 2;
	}
	else if (drive)
		drive[0] = 0;

	for (p = (char *)path; *p; p++)
	{
		if (*p == '\\' || *p == '/')
			last_slash = ++p;
	}

	if (last_slash)
	{
		if (dir)
		{
			len = (unsigned int)(last_slash - path);
			if (len > 0xFF)
				len = 0xFF;

			Q_strncpy(dir, path, len);
			dir[len] = 0;
		}
		path = last_slash;
	}
	else if (dir)
		dir[0] = 0;

	for (p = (char *)path; *p; p++)
	{
		if (*p == '.')
			dot = p;
	}

	if (!dot)
		dot = p;

	if (fname)
	{
		len = (unsigned int)(dot - path);
		if (len > 0xFF)
			len = 0xFF;

		Q_strncpy(fname, path, len);
		fname[len] = 0;
	}

	if (ext)
	{
		len = (unsigned int)(dot - path);
		if (len > 0xFF)
			len = 0xFF;

		Q_strncpy(ext, dot, len);
		ext[len] = 0;
	}

#endif // _WIN32
}

void Con_Debug_f(void)
{
	if (con_debuglog)
	{
		Con_Printf("condebug disabled\n");
		con_debuglog = FALSE;
	}
	else
	{
		con_debuglog = TRUE;
		Con_Printf("condebug enabled\n");
	}
}

void Con_Init(void)
{
	con_debuglog = COM_CheckParm("-condebug");
	Con_DPrintf("Console initialized.\n");
	Cmd_AddCommand("condebug", Con_Debug_f);
}

void Con_DebugLog(const char *file, const char *fmt, ...)
{
	va_list argptr;
	static char data[8192];

	va_start(argptr, fmt);
	Q_vsnprintf(data, sizeof(data), fmt, argptr);
	va_end(argptr);

	data[sizeof(data) - 1] = 0;

#ifdef _WIN32

	int fd = _open(file, _O_WRONLY | _O_APPEND | _O_CREAT, _S_IREAD | _S_IWRITE);
	int len = Q_strlen(data);
	_write(fd, data, len);
	_close(fd);

#else // _WIN32

	FILE *fd = FS_Open(file, "at");
	FS_FPrintf(fd, "%s", data);
	FS_Close(fd);

#endif // _WIN32
}

void Con_Printf(const char *fmt, ...)
{
	char Dest[4096];
	va_list va;

	va_start(va, fmt);
	Q_vsnprintf(Dest, sizeof(Dest), fmt, va);
	va_end(va);

	g_RehldsHookchains.m_Con_Printf.callChain(Con_Printf_internal, Dest);
}

void EXT_FUNC Con_Printf_internal(const char *Dest)
{
#ifdef REHLDS_FLIGHT_REC
	FR_Log("REHLDS_CON", Dest);
#endif

#ifdef REHLDS_FIXES
	if (sv_redirected == RD_NONE || sv_rcon_condebug.value > 0.0f)
#endif
	{
		Sys_Printf("%s", Dest);
	}

	if (sv_redirected)
	{
		if ((Q_strlen(outputbuf) + Q_strlen(Dest)) > sizeof(outputbuf) - 1)
			SV_FlushRedirect();
		Q_strncat(outputbuf, Dest, sizeof(outputbuf) - 1);
	}
	else
	{
		if (con_debuglog)
			Con_DebugLog("qconsole.log", "%s", Dest);
#ifndef SWDS
		if (host_initialized && con_initialized && g_pcls.state)
		{
			if (developer.value != 0.0f)
			{
				Q_strncpy(g_szNotifyAreaString, msg, 255);
				g_szNotifyAreaString[255] = 0;
				*con_times = realtime;
			}
			VGuiWrap2_ConPrintf(msg);
		}
#endif // SWDS
	}
}

void Con_SafePrintf(const char *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);

#ifdef _WIN32
	char Dest[1024];
	Q_vsnprintf(Dest, sizeof(Dest), fmt, argptr);
	va_end(argptr);
	Con_Printf("%s", Dest);
#else
	vfprintf(stdout, fmt, argptr);
	va_end(argptr);
	fflush(stdout);
#endif // _WIN32
}

#if defined(REHLDS_FIXES) && defined(REHLDS_FLIGHT_REC)
// Always print debug logs to the flight recorder
void EXT_FUNC Con_DPrintf(const char *fmt, ...)
{
	char Dest[4096];
	va_list argptr;
	va_start(argptr, fmt);
	Q_vsnprintf(Dest, sizeof(Dest), fmt, argptr);
	va_end(argptr);

	FR_Log("REHLDS_CONDBG", Dest);

	if (developer.value != 0.0f)
	{
#ifdef _WIN32
		OutputDebugStringA(Dest);
		if (con_debuglog)
			Con_DebugLog("qconsole.log", "%s", Dest);
#else
		vfprintf(stdout, fmt, argptr);
		fflush(stdout);
#endif // _WIN32
	}
}

#else // defined(REHLDS_FIXES) and defined(REHLDS_FLIGHT_REC)

void EXT_FUNC Con_DPrintf(const char *fmt, ...)
{
	va_list argptr;

	va_start(argptr, fmt);
	if (developer.value != 0.0f)
	{
#ifdef _WIN32
		char Dest[4096];
		Q_vsnprintf(Dest, sizeof(Dest), fmt, argptr);

		OutputDebugStringA(Dest);
		if (con_debuglog)
			Con_DebugLog("qconsole.log", "%s", Dest);
#else
		vfprintf(stdout, fmt, argptr);
		fflush(stdout);
#endif // _WIN32
	}
	va_end(argptr);
}

#endif // defined(REHLDS_FIXES) and defined(REHLDS_FLIGHT_REC)

