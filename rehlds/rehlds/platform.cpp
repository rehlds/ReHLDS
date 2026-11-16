#include "precompiled.h"

IReHLDSPlatform* CRehldsPlatformHolder::m_Platform;

IReHLDSPlatform* CRehldsPlatformHolder::get() {
	if (m_Platform == NULL) {
		m_Platform = new CSimplePlatform();
	}

	return m_Platform;
}

void CRehldsPlatformHolder::set(IReHLDSPlatform* p) {
	m_Platform = p;
}

// HLDS ships two steam_api builds:
//   legacy (Steamworks SDK 1.22 on Windows, 1.23a on Linux; HLDS builds up to 8684) and
//   HL25   (SDK 1.53a on Windows, 1.57 on Linux; HLDS builds 9890 and later).
// HL25 builds dropped the flat SteamGameServer_Init(), SteamGameServer() and SteamApps() exports in favour of
// the SteamInternal_* entry points. These three are resolved at run time, so one engine binary loads with
// either steam_api. The interfaces stay the versions the engine is compiled against (SteamGameServer011,
// STEAMAPPS_INTERFACE_VERSION005); steamclient still serves them by name.
namespace
{
	typedef bool (*SteamGameServer_Init_t)(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString);
	typedef ISteamGameServer *(*SteamGameServer_t)();
	typedef ISteamApps *(*SteamApps_t)();
	typedef HSteamUser (*GetHSteamUser_t)();
	typedef void *(*FindOrCreateInterface_t)(HSteamUser hSteamUser, const char *pszVersion);

	struct SteamApiEntries
	{
		bool resolved;
		SteamGameServer_Init_t legacyGameServerInit;   // legacy steam_api
		SteamGameServer_t legacyGameServer;
		SteamApps_t legacyApps;
		SteamGameServer_Init_t internalGameServerInit; // HL25 steam_api
		GetHSteamUser_t gameServerUser;
		GetHSteamUser_t apiUser;
		FindOrCreateInterface_t gameServerInterface;
		FindOrCreateInterface_t userInterface;
	};

	SteamApiEntries g_SteamApi;
	ISteamGameServer *g_pSteamGameServer;

	void *SteamApiProc(const char *name)
	{
#ifdef _WIN32
		HMODULE hSteamApi = GetModuleHandleA("steam_api.dll");
		return hSteamApi ? (void *)GetProcAddress(hSteamApi, name) : nullptr;
#else
		return dlsym(RTLD_DEFAULT, name);
#endif
	}

	SteamApiEntries &SteamApi()
	{
		if (!g_SteamApi.resolved)
		{
			g_SteamApi.legacyGameServerInit = (SteamGameServer_Init_t)SteamApiProc("SteamGameServer_Init");
			g_SteamApi.legacyGameServer = (SteamGameServer_t)SteamApiProc("SteamGameServer");
			g_SteamApi.legacyApps = (SteamApps_t)SteamApiProc("SteamApps");
			g_SteamApi.internalGameServerInit = (SteamGameServer_Init_t)SteamApiProc("SteamInternal_GameServer_Init");
			g_SteamApi.gameServerUser = (GetHSteamUser_t)SteamApiProc("SteamGameServer_GetHSteamUser");
			g_SteamApi.apiUser = (GetHSteamUser_t)SteamApiProc("SteamAPI_GetHSteamUser");
			g_SteamApi.gameServerInterface = (FindOrCreateInterface_t)SteamApiProc("SteamInternal_FindOrCreateGameServerInterface");
			g_SteamApi.userInterface = (FindOrCreateInterface_t)SteamApiProc("SteamInternal_FindOrCreateUserInterface");
			g_SteamApi.resolved = true;
		}

		return g_SteamApi;
	}
}

CSimplePlatform::CSimplePlatform() {
#ifdef _WIN32
	wsock = LoadLibraryA("wsock32.dll");
	setsockopt_v11 = (setsockopt_proto)GetProcAddress(wsock, "setsockopt");
	if (setsockopt_v11 == NULL)
		rehlds_syserror("%s: setsockopt_v11 not found", __func__);
#endif
}

CSimplePlatform::~CSimplePlatform()
{
#ifdef _WIN32
	FreeLibrary(wsock);
#endif
}

uint32 CSimplePlatform::time(uint32* pTime)
{
	time_t res = ::time((time_t*)NULL);
	if (pTime != NULL) *pTime = (uint32)res;

	return (uint32) res;
}

struct tm* CSimplePlatform::localtime(uint32 time)
{
	time_t theTime = (time_t)time;
	return ::localtime(&theTime);
}

void CSimplePlatform::srand(uint32 seed)
{
	return ::srand(seed);
}

int CSimplePlatform::rand()
{
	return ::rand();
}

#ifdef _WIN32
void CSimplePlatform::Sleep(DWORD msec) {
	::Sleep(msec);
}

BOOL CSimplePlatform::QueryPerfCounter(LARGE_INTEGER* counter) {
	return ::QueryPerformanceCounter(counter);
}

BOOL CSimplePlatform::QueryPerfFreq(LARGE_INTEGER* freq) {
	return ::QueryPerformanceFrequency(freq);
}

DWORD CSimplePlatform::GetTickCount() {
	return ::GetTickCount();
}

void CSimplePlatform::GetLocalTime(LPSYSTEMTIME time) {
	return ::GetLocalTime(time);
}

void CSimplePlatform::GetSystemTime(LPSYSTEMTIME time) {
	return ::GetSystemTime(time);
}

void CSimplePlatform::GetTimeZoneInfo(LPTIME_ZONE_INFORMATION zinfo) {
	::GetTimeZoneInformation(zinfo);
}

BOOL CSimplePlatform::GetProcessTimes(HANDLE hProcess, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime)
{
	return ::GetProcessTimes(hProcess, lpCreationTime, lpExitTime, lpKernelTime, lpUserTime);
}

void CSimplePlatform::GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
	::GetSystemTimeAsFileTime(lpSystemTimeAsFileTime);
}
#endif //WIN32

SOCKET CSimplePlatform::socket(int af, int type, int protocol) {
	return ::socket(af, type, protocol);
}

int CSimplePlatform::setsockopt(SOCKET s, int level, int optname, const char* optval, int optlen) {
#ifdef _WIN32
	return setsockopt_v11(s, level, optname, optval, optlen);
#else
	return ::setsockopt(s, level, optname, optval, optlen);
#endif
}

int CSimplePlatform::closesocket(SOCKET s) {
#ifdef _WIN32
	return ::closesocket(s);
#else
	return ::close(s);
#endif
}

int CSimplePlatform::recvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, socklen_t *fromlen) {
	return ::recvfrom(s, buf, len, flags, from, fromlen);
}

int CSimplePlatform::sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen) {
	return ::sendto(s, buf, len, flags, to, tolen);
}

int CSimplePlatform::bind(SOCKET s, const struct sockaddr* addr, int namelen) {
	return ::bind(s, addr, namelen);
}

int CSimplePlatform::getsockname(SOCKET s, struct sockaddr* name, socklen_t* namelen) {
	return ::getsockname(s, name, namelen);
}

struct hostent* CSimplePlatform::gethostbyname(const char *name) {
	return ::gethostbyname(name);
}

int CSimplePlatform::gethostname(char *name, int namelen) {
	return ::gethostname(name, namelen);
}

#ifdef _WIN32

int CSimplePlatform::ioctlsocket(SOCKET s, long cmd, u_long *argp) {
	return ::ioctlsocket(s, cmd, argp);
}

int CSimplePlatform::WSAGetLastError() {
	return ::WSAGetLastError();
}

#endif //WIN32
void CSimplePlatform::SteamAPI_SetBreakpadAppID(uint32 unAppID) {
	return ::SteamAPI_SetBreakpadAppID(unAppID);
}

void CSimplePlatform::SteamAPI_UseBreakpadCrashHandler(char const *pchVersion, char const *pchDate, char const *pchTime, bool bFullMemoryDumps, void *pvContext, PFNPreMinidumpCallback m_pfnPreMinidumpCallback) {
	::SteamAPI_UseBreakpadCrashHandler(pchVersion, pchDate, pchTime, bFullMemoryDumps, pvContext, m_pfnPreMinidumpCallback);
}

void CSimplePlatform::SteamAPI_RegisterCallback(CCallbackBase *pCallback, int iCallback) {
	::SteamAPI_RegisterCallback(pCallback, iCallback);
}

bool CSimplePlatform::SteamAPI_Init() {
	return ::SteamAPI_Init();
}

void CSimplePlatform::SteamAPI_UnregisterCallResult(class CCallbackBase *pCallback, SteamAPICall_t hAPICall) {
	return ::SteamAPI_UnregisterCallResult(pCallback, hAPICall);
}

ISteamApps* CSimplePlatform::SteamApps() {
	SteamApiEntries &api = SteamApi();
	if (api.legacyApps)
		return api.legacyApps();

	if (api.userInterface && api.apiUser)
	{
		HSteamUser hUser = api.apiUser();
		if (hUser)
			return (ISteamApps *)api.userInterface(hUser, STEAMAPPS_INTERFACE_VERSION);
	}

	return nullptr;
}

bool CSimplePlatform::SteamGameServer_Init(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString) {
	SteamApiEntries &api = SteamApi();
	g_pSteamGameServer = nullptr;

	if (api.legacyGameServerInit)
		return api.legacyGameServerInit(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, pchVersionString);

	if (api.internalGameServerInit)
	{
		// HL25 steam_api (like the SDK 1.53a/1.57 SteamGameServer_Init wrapper) expects 0 for the legacy Steam port
		// and a plain "x.x.x.x" version: cut a suffix like "1.1.2.2/Stdio" at the slash.
		char szVersion[32];
		Q_strncpy(szVersion, pchVersionString ? pchVersionString : "", sizeof(szVersion) - 1);
		szVersion[sizeof(szVersion) - 1] = '\0';
		char *pSlash = Q_strchr(szVersion, '/');
		if (pSlash)
			*pSlash = '\0';

		return api.internalGameServerInit(unIP, 0, usGamePort, usQueryPort, eServerMode, szVersion);
	}

	return false;
}

ISteamGameServer* CSimplePlatform::SteamGameServer() {
	SteamApiEntries &api = SteamApi();
	if (api.legacyGameServer)
		return api.legacyGameServer();

	if (!g_pSteamGameServer && api.gameServerInterface && api.gameServerUser)
	{
		HSteamUser hUser = api.gameServerUser();
		if (hUser)
			g_pSteamGameServer = (ISteamGameServer *)api.gameServerInterface(hUser, STEAMGAMESERVER_INTERFACE_VERSION);
	}

	return g_pSteamGameServer;
}

void CSimplePlatform::SteamGameServer_RunCallbacks() {
	::SteamGameServer_RunCallbacks();
}

void CSimplePlatform::SteamAPI_RunCallbacks() {
	::SteamAPI_RunCallbacks();
}

void CSimplePlatform::SteamGameServer_Shutdown() {
	g_pSteamGameServer = nullptr;
	::SteamGameServer_Shutdown();
}

void CSimplePlatform::SteamAPI_UnregisterCallback(CCallbackBase *pCallback)
{
	::SteamAPI_UnregisterCallback(pCallback);
}

void NORETURN rehlds_syserror(const char* fmt, ...) {
	va_list			argptr;
	static char		string[8192];

	va_start(argptr, fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	printf("%s\n", string);

	FILE* fl = fopen("rehlds_error.txt", "w");
	fprintf(fl, "%s\n", string);
	fclose(fl);

	//TerminateProcess(GetCurrentProcess(), 1);
	volatile int *null = 0;
	*null = 0;

	while (true);
}
