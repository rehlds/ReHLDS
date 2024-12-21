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
	NORETURN void ErrorMessage(int level, const char *msg) override;

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
char g_szEXEName[MAX_PATH];

SleepType Sys_Sleep;
NET_Sleep_t NET_Sleep_Timeout = nullptr;

CSys::CSys()
{
}

CSys::~CSys()
{
	sys = nullptr;
}

// this checks if pause has run yet,
// tell the compiler it can change at any time
volatile bool g_bPaused = false;

void CSys::Sleep(int msec)
{
	Sys_Sleep(msec);
}

void Sleep_Old(int msec)
{
	usleep(msec * 1000);
}

void Sleep_Select(int msec)
{
	struct timeval tv;

	// Assumes msec < 1000
	tv.tv_sec = 0;
	tv.tv_usec = 1000 * msec;

	select(1, nullptr, nullptr, nullptr, &tv);
}

void Sleep_Net(int msec)
{
	NET_Sleep_Timeout();
}

// linux runs on a 100Hz scheduling clock, so the minimum latency from
// usleep is 10msec. However, people want lower latency than this..
//
// There are a few solutions, one is to use the realtime scheduler in the
// kernel BUT this needs root privelleges to start. It also can play
// unfriendly with other programs.
//
// Another solution is to use software timers, they use the RTC of the
// system and are accurate to microseconds (or so).
//
// timers, via setitimer() are used here
void Sleep_Timer(int msec)
{
	struct itimerval tm;

	tm.it_value.tv_sec = msec / 1000;		// convert msec to seconds
	tm.it_value.tv_usec = (msec % 1000) * 1E3;	// get the number of msecs and change to micros
	tm.it_interval.tv_sec = 0;
	tm.it_interval.tv_usec = 0;

	g_bPaused = false;

	// set the timer to trigger
	if (!setitimer(ITIMER_REAL, &tm, nullptr)) {
		// wait for the signal
		pause();
	}

	g_bPaused = true;
}

void alarmFunc(int num)
{
	// reset the signal handler
	signal(SIGALRM, alarmFunc);

	// paused is 0, the timer has fired before the pause was called... Lets queue it again
	if (!g_bPaused)
	{
		struct itimerval itim;
		itim.it_interval.tv_sec = 0;
		itim.it_interval.tv_usec = 0;
		itim.it_value.tv_sec = 0;
		itim.it_value.tv_usec = 1000; // get it to run again real soon
		setitimer(ITIMER_REAL, &itim, 0);
	}

}

void Sleep_Pingboost4(int msec);

void Sys_InitPingboost()
{
    Sys_Sleep = Sleep_Old;

    char* pPingType;
    if (CommandLine()->CheckParm("-pingboost", &pPingType) && pPingType) {
        int type = Q_atoi(pPingType);
        switch (type) {
        case 1:
            signal(SIGALRM, alarmFunc);
            Sys_Sleep = Sleep_Timer;
            break;
        case 2:
            Sys_Sleep = Sleep_Select;
            break;
        case 3:
            Sys_Sleep = Sleep_Net;
            NET_Sleep_Timeout = (NET_Sleep_t)Sys_GetProcAddress(g_pEngineModule, "NET_Sleep_Timeout");
            break;
        case 4: // New case for -pingboost 4
            Sys_Sleep = Sleep_Pingboost4;
            break;
        default:
            Sys_Sleep = Sleep_Old;
            break;
        }
    }
}

void Sleep_Pingboost4(int msec)
{
    // Do nothing, just return without sleep
}

void Sys_WriteProcessIdFile()
{
	char *fname;
	if (!CommandLine()->CheckParm("-pidfile", &fname) || !fname) {
		return;
	}

	FILE *pidFile = fopen(fname, "w");
	if (!pidFile) {
		printf("Warning: unable to open pidfile (%s)\n", fname);
		return;
	}

	fprintf(pidFile, "%i\n", getpid());
	fclose(pidFile);
}

bool CSys::GetExecutableName(char *out)
{
	Q_strcpy(out, g_szEXEName);
	return true;
}

// Engine is erroring out, display error in message box
void CSys::ErrorMessage(int level, const char *msg)
{
	puts(msg);
	exit(-1);
}

void CSys::WriteStatusText(const char *szText)
{
}

void CSys::UpdateStatus(int force)
{
}

long CSys::LoadLibrary(const char *lib)
{
	char cwd[1024];
	char absolute_lib[1024];

	if (!getcwd(cwd, sizeof(cwd)))
		ErrorMessage(1, "Sys_LoadLibrary: Couldn't determine current directory.");

	if (cwd[Q_strlen(cwd) - 1] == '/')
		cwd[Q_strlen(cwd) - 1] = '\0';

	Q_snprintf(absolute_lib, sizeof(absolute_lib), "%s/%s", cwd, lib);

#ifdef LAUNCHER_FIXES
	void *hDll = dlopen(absolute_lib, RTLD_NOW | RTLD_DEEPBIND | RTLD_LOCAL);
#else // LAUNCHER_FIXES
	void *hDll = dlopen(absolute_lib, RTLD_NOW);
#endif // LAUNCHER_FIXES
	if (!hDll)
	{
		ErrorMessage(1, dlerror());
	}

	return (long)hDll;
}

void CSys::FreeLibrary(long library)
{
	if (!library)
		return;

	dlclose((void *)library);
}

bool CSys::CreateConsoleWindow()
{
	return true;
}

void CSys::DestroyConsoleWindow()
{
}

// Print text to the dedicated console
void CSys::ConsoleOutput(const char *string)
{
	printf("%s", string);
	fflush(stdout);
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

const int MAX_LINUX_CMDLINE = 2048;
static char linuxCmdline[MAX_LINUX_CMDLINE];

char* BuildCmdLine(int argc, char **argv)
{
	int len = 0;

	for (int i = 1; i < argc; i++)
	{
		len += Q_strlen(argv[i]) + 1;
	}

	if (len > MAX_LINUX_CMDLINE)
	{
		printf("command line too long, %i max\n", MAX_LINUX_CMDLINE);
		exit(-1);
	}

	linuxCmdline[0] = '\0';
	for (int i = 1; i < argc; i++)
	{
		if (i > 1) {
			Q_strlcat(linuxCmdline, " ");
		}

		Q_strlcat(linuxCmdline, argv[i]);
	}

	return linuxCmdline;
}

static void ConsoleCtrlHandler(int signal)
{
	if (signal == SIGINT || signal == SIGTERM)
	{
		g_bAppHasBeenTerminated = true;
	}
}

bool Sys_SetupConsole()
{
	struct sigaction action;
	action.sa_handler = ConsoleCtrlHandler;
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);

	return true;
}

void Sys_PrepareConsoleInput()
{
}

int main(int argc, char *argv[])
{
	Q_snprintf(g_szEXEName, sizeof(g_szEXEName), "%s", argv[0]);
	char* cmdline = BuildCmdLine(argc, argv);

	return StartServer(cmdline) == LAUNCHER_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
