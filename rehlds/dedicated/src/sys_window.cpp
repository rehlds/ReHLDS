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

class CSys: public ISys {
public:
	CSys();
	virtual ~CSys();

	void Sleep(int msec) override;
	bool GetExecutableName(char *out) override;
	void ErrorMessage(int level, const char *msg) override;

	void WriteStatusText(const char *szText) override;
	void UpdateStatus(int force) override;

	long LoadLibrary(const char *lib) override;
	void FreeLibrary(long library) override;

	bool CreateConsoleWindow() override;
	void DestroyConsoleWindow() override;

	void ConsoleOutput(const char *string) override;
	const char *ConsoleInput() override;
	void Printf(const char *fmt, ...) override;
};

CSys g_Sys;
ISys *sys = &g_Sys;

CSys::CSys()
{
	// Startup winsock
	WORD version = MAKEWORD(2, 0);
	WSADATA wsaData;
	WSAStartup(version, &wsaData);

	// Request 1ms resolution for windows timer
	timeBeginPeriod(1);
}

CSys::~CSys()
{
	// Clear previously set timer resolution
	timeEndPeriod(1);

	WSACleanup();
	sys = nullptr;
}

void CSys::Sleep(int msec)
{
	::Sleep(msec);
}

bool CSys::GetExecutableName(char *out)
{
	if (!::GetModuleFileName((HINSTANCE)GetModuleHandle(nullptr), out, 256))
		return false;

	return true;
}

void CSys::ErrorMessage(int level, const char *msg)
{
	MessageBox(nullptr, msg, "Half-Life Dedicated Server Error", MB_OK);
	PostQuitMessage(0);
}

void CSys::WriteStatusText(const char *szText)
{
	SetConsoleTitle(szText);
}

void CSys::UpdateStatus(int force)
{
	static double tLast = 0.0;
	char szStatus[256];
	int n, nMax;
	char szMap[32];
	float fps;

	if (!engineAPI)
		return;

	double tCurrent = timeGetTime() * 0.001;
	engineAPI->UpdateStatus(&fps, &n, &nMax, szMap);

	if (!force)
	{
		if ((tCurrent - tLast) < 0.5f)
			return;
	}

	tLast = tCurrent;
	Q_snprintf(szStatus, sizeof(szStatus), "%.1f fps %2i/%2i on %16s", fps, n, nMax, szMap);

	console.SetStatusLine(szStatus);
	console.UpdateStatus();
}

long CSys::LoadLibrary(const char *lib)
{
	void *hDll = ::LoadLibrary(lib);
	return (long)hDll;
}

void CSys::FreeLibrary(long library)
{
	if (!library)
		return;

	::FreeLibrary((HMODULE)library);
}

bool CSys::CreateConsoleWindow()
{
#if 0
	if (!AllocConsole())
	{
		return false;
	}
#endif

	InitConProc();
	return true;
}

void CSys::DestroyConsoleWindow()
{
	FreeConsole();

	// shut down QHOST hooks if necessary
	DeinitConProc();
}

void CSys::ConsoleOutput(const char *string)
{
	if (g_bVGui)
	{
		VGUIPrintf(string);
	}
	else
	{
		console.Print(string);
	}
}

const char *CSys::ConsoleInput()
{
	return console.GetLine();
}

void CSys::Printf(const char *fmt, ...)
{
	// Dump text to debugging console.
	va_list argptr;
	char szText[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(szText, sizeof(szText), fmt, argptr);
	va_end(argptr);

	// Get Current text and append it.
	ConsoleOutput(szText);
}

BOOL WINAPI ConsoleCtrlHandler(DWORD CtrlType)
{
	switch (CtrlType) {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		g_bAppHasBeenTerminated = true;
		return TRUE;
	default:
		break;
	}

	return FALSE;
}

bool Sys_SetupConsole()
{
	console.SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);

	if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE)) {
		sys->ErrorMessage(0, "Unable to set control handler.");
		return false;
	}

	return true;
}

void Sys_PrepareConsoleInput()
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
		if (!GetMessage(&msg, nullptr, 0, 0)) {
			break;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void Sys_InitPingboost()
{
	;
}

void Sys_WriteProcessIdFile()
{
	;
}

// Load steamclient.dll from the server folder before the engine initializes Steam.
// Without this, steam_api may take the steamclient.dll of an installed Steam client from
// HKCU\Software\Valve\Steam\ActiveProcess\SteamClientDll (the value stays there after Steam exits).
// A current Steam client's 32-bit steamclient.dll needs SDL3.dll, which is not next to hlds.exe,
// so the server fails with "Unable to initialize Steam". Disable with -nolocalsteamclient.
void Sys_PreloadSteamClient()
{
	if (CommandLine()->CheckParm("-nolocalsteamclient"))
		return;

	char path[MAX_PATH];
	DWORD len = GetModuleFileNameA(nullptr, path, sizeof(path));
	if (len == 0 || len >= sizeof(path))
		return;

	char *slash = strrchr(path, '\\');
	if (!slash)
		return;

	const char steamClient[] = "steamclient.dll";
	if ((size_t)(slash + 1 - path) + sizeof(steamClient) > sizeof(path))
		return;

	Q_strcpy(slash + 1, steamClient);
	if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
		return; // no local steamclient.dll, leave the choice to steam_api

	if (!LoadLibraryExA(path, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH))
		printf("Warning: unable to load %s (error %lu)\n", path, GetLastError());
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#ifdef LAUNCHER_FIXES
	return CatchAndWriteMiniDump([=]()
	{
#endif
		if (ShouldLaunchAppViaSteam(lpCmdLine, STDIO_FILESYSTEM_LIB, STDIO_FILESYSTEM_LIB))
			return 0;

		//auto command = CommandLineToArgvW(GetCommandLineW(), (int *)&lpCmdLine);
		auto ret = StartServer(lpCmdLine);
		//LocalFree(command);
		return ret;
#ifdef LAUNCHER_FIXES
	}, lpCmdLine);
#endif
}
