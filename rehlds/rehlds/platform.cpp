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

CSimplePlatform::CSimplePlatform() {
#ifdef _WIN32
	wsock = LoadLibraryA("wsock32.dll");
	setsockopt_v11 = (setsockopt_proto)GetProcAddress(wsock, "setsockopt");
	if (setsockopt_v11 == NULL)
		rehlds_syserror("%s: setsockopt_v11 not found", __func__);
#endif

#ifdef VERSION_SAFE_STEAM_API_INTERFACES
	m_pGameServerContext = new CSteamGameServerAPIContext();
	m_pClientContext = new CSteamAPIContext();
#endif
}

CSimplePlatform::~CSimplePlatform()
{
#ifdef _WIN32
	FreeLibrary(wsock);
#endif

#ifdef VERSION_SAFE_STEAM_API_INTERFACES
	delete static_cast<CSteamGameServerAPIContext*>(m_pGameServerContext);
	delete static_cast<CSteamAPIContext*>(m_pClientContext);
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
	#ifdef VERSION_SAFE_STEAM_API_INTERFACES
	if (!::SteamAPI_InitSafe())
		return false;
	return static_cast<CSteamAPIContext*>(m_pClientContext)->Init();
	#else
	return ::SteamAPI_Init();
	#endif
}

void CSimplePlatform::SteamAPI_UnregisterCallResult(class CCallbackBase *pCallback, SteamAPICall_t hAPICall) {
	return ::SteamAPI_UnregisterCallResult(pCallback, hAPICall);
}

ISteamApps* CSimplePlatform::SteamApps() {
	#ifdef VERSION_SAFE_STEAM_API_INTERFACES
	return static_cast<CSteamAPIContext*>(m_pClientContext)->SteamApps();
	#else
	return ::SteamApps();
	#endif
}

#ifdef VERSION_SAFE_STEAM_API_INTERFACES
bool CSimplePlatform::SteamGameServer_InitSafe(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString) {
	if (!::SteamGameServer_InitSafe(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, pchVersionString))
		return false;
	return static_cast<CSteamGameServerAPIContext*>(m_pGameServerContext)->Init();
}
#else 
bool CSimplePlatform::SteamGameServer_Init(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString) {
	return ::SteamGameServer_Init(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, pchVersionString);
}
#endif

ISteamGameServer* CSimplePlatform::SteamGameServer() {
	#ifdef VERSION_SAFE_STEAM_API_INTERFACES
	return static_cast<CSteamGameServerAPIContext*>(m_pGameServerContext)->SteamGameServer();
	#else
	return ::SteamGameServer();
	#endif
}

void CSimplePlatform::SteamGameServer_RunCallbacks() {
	::SteamGameServer_RunCallbacks();
}

void CSimplePlatform::SteamAPI_RunCallbacks() {
	::SteamAPI_RunCallbacks();
}

void CSimplePlatform::SteamGameServer_Shutdown() {
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
