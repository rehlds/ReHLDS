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

bool g_bVGui = false;
char *gpszCvars = nullptr;
bool g_bAppHasBeenTerminated = false;

CSysModule *g_pEngineModule = nullptr;
IDedicatedServerAPI *engineAPI = nullptr;

IFileSystem *g_pFileSystemInterface = nullptr;
CSysModule *g_pFileSystemModule = nullptr;

CreateInterfaceFn g_FilesystemFactoryFn;

void Sys_Printf_Safe(const char *text)
{
	if (sys)
	{
		sys->Printf("%s", text);
	}
}

void Sys_Printf(const char *fmt, ...)
{
	// Dump text to debugging console.
	va_list argptr;
	char szText[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(szText, sizeof(szText), fmt, argptr);
	va_end(argptr);

	// Get Current text and append it.
#ifdef VGUI
	if (g_bVGui)
	{
		VGUIPrintf(szText);
	}
	else
#endif
	{
		console.Print(szText);
	}
}

void ProcessConsoleInput()
{
	if (!engineAPI)
		return;

	const char *inputLine = console.GetLine();
	if (inputLine)
	{
		char szBuf[256];
		Q_snprintf(szBuf, sizeof(szBuf), "%s\n", inputLine);
		engineAPI->AddConsoleText(szBuf);
	}
}

const char *UTIL_GetBaseDir()
{
	return ".";
}

int RunEngine()
{
#ifdef VGUI
	if (g_bVGui)
	{
		// TODO: finish VGUI
		//vgui::ivgui()->SetSleep(0);
	}
#endif

	CreateInterfaceFn engineFactory = Sys_GetFactory(g_pEngineModule);
	RunVGUIFrame();

	if (engineFactory) {
		engineAPI = (IDedicatedServerAPI *)engineFactory(VENGINE_HLDS_API_VERSION, nullptr);
	}

	RunVGUIFrame();
	if (!engineAPI || !engineAPI->Init(UTIL_GetBaseDir(), (char *)CommandLine()->GetCmdLine(), Sys_GetFactoryThis(), g_FilesystemFactoryFn)) {
		return LAUNCHER_ERROR;
	}

#ifdef LAUNCHER_FIXES
	if (engineFactory)
	{
		ISystemModule *pSystemWrapper = (ISystemModule *)engineFactory(BASESYSTEM_INTERFACE_VERSION, nullptr);
		if (pSystemWrapper) {
			console.InitSystem(pSystemWrapper->GetSystem());
		}
	}
#endif // LANUCHER_FIXES

	RunVGUIFrame();

#ifdef VGUI
	// TODO: finish me!
	/*if (g_bVGui) {
		g_pFileSystemInterface->AddSearchPath("platform", "PLATFORM");

		// find our configuration directory
		char szConfigDir[512];
		const char *steamPath = getenv("SteamInstallPath");
		if (steamPath) {
			// put the config dir directly under steam
			Q_snprintf(szConfigDir, sizeof(szConfigDir), "%s/config", steamPath);
		}
		else {
			// we're not running steam, so just put the config dir under the platform
			Q_strlcpy(szConfigDir, "platform/config");
		}
	}*/
#endif // VGUI

	RunVGUIFrame();

	if (gpszCvars) {
		engineAPI->AddConsoleText(gpszCvars);
	}

	VGUIFinishedConfig();
	RunVGUIFrame();

	bool bDone = false;
	while (!bDone)
	{
		// Running really fast, yield some time to other apps
		sys->Sleep(1);

		Sys_PrepareConsoleInput();

		if (g_bAppHasBeenTerminated)
			break;

#ifdef VGUI
		if (g_bVGui) {
			RunVGUIFrame();
		}
		else
#endif
		{
			ProcessConsoleInput();
		}

		bDone = !engineAPI->RunFrame();
		sys->UpdateStatus(FALSE /* don't force */);
	}

#ifdef VGUI
	if (g_bVGui) {
		RunVGUIFrame();
		StopVGUI();
	}
	else
#endif
	{
		sys->DestroyConsoleWindow();
		console.ShutDown();
	}

	int iret = engineAPI->Shutdown();
	VGUIFinishedConfig();

	if (iret == DLL_CLOSE)
		g_bAppHasBeenTerminated = true;

	return LAUNCHER_OK;
}

int StartServer(char* cmdline)
{
	g_bAppHasBeenTerminated = false;

	do {
		CommandLine()->CreateCmdLine(cmdline);
		CommandLine()->AppendParm("-steam", nullptr);

#ifdef LAUNCHER_FIXES
		// Prefer the steamclient next to the server over the one of an installed Steam client
		Sys_PreloadSteamClient();
#endif

		// Load engine
		g_pEngineModule = Sys_LoadModule(ENGINE_LIB);
		if (!g_pEngineModule) {
			sys->ErrorMessage(0, "Unable to load engine, image is corrupt.");
			return LAUNCHER_ERROR;
		}

		Sys_InitPingboost();
		Sys_WriteProcessIdFile();

		// Load filesystem
		g_pFileSystemModule = Sys_LoadModule(STDIO_FILESYSTEM_LIB);
		if (!g_pFileSystemModule) {
			sys->ErrorMessage(0, "Unable to load filesystem, image is corrupt.");
			return LAUNCHER_ERROR;
		}

		// Get filesystem factory
		g_FilesystemFactoryFn = Sys_GetFactory(g_pFileSystemModule);
		if (!g_FilesystemFactoryFn) {
			sys->ErrorMessage(0, "Unable to get filesystem factory.");
			return LAUNCHER_ERROR;
		}

		// Retrieve filesystem interface
		IFileSystem *pFullFileSystem = (IFileSystem *)g_FilesystemFactoryFn(FILESYSTEM_INTERFACE_VERSION, nullptr);
		if (!pFullFileSystem) {
			sys->ErrorMessage(0, "Can not retrive filesystem interface version '" FILESYSTEM_INTERFACE_VERSION "'.\n");
			return LAUNCHER_ERROR;
		}

		// Mount filesystem
		pFullFileSystem->Mount();

		// Init VGUI or Console mode
#ifdef VGUI
		if (!CommandLine()->CheckParm("-console")) {
			g_bVGui = true;
			StartVGUI();
		}
		else
#endif
		{
			if (!console.Init()) {
				sys->ErrorMessage(0, "Failed to initialize console."); // TODO: add more details
				return LAUNCHER_ERROR;
			}

			if (!sys->CreateConsoleWindow()) {
				sys->ErrorMessage(0, "Failed to create console window.");
				return LAUNCHER_ERROR;
			}

			if (!Sys_SetupConsole()) {
				return LAUNCHER_ERROR;
			}
		}

#ifdef VGUI
		// TODO: finish me!
		/*// run vgui
		if (g_bVGui)
		{
			while (VGUIIsInConfig()	&& VGUIIsRunning()) {
				RunVGUIFrame();
			}
		}
		else*/
#endif
		{
			int ret = RunEngine();
			if (ret == LAUNCHER_ERROR) {
				sys->ErrorMessage(0, "Failed to launch engine.\n");
				return LAUNCHER_ERROR;
			}
		}

		if (gpszCvars)
			free(gpszCvars);

		// Unmount filesystem
		pFullFileSystem->Unmount();

		// Unload modules
		Sys_UnloadModule(g_pFileSystemModule);
		Sys_UnloadModule(g_pEngineModule);

	} while (!g_bAppHasBeenTerminated);

	return LAUNCHER_OK;
}
