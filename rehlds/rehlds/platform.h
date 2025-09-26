#pragma once

#include "osconfig.h"
#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"

#ifdef _WIN32
typedef int(__stdcall *setsockopt_proto)(SOCKET s, int level, int optname, const char *optval, int optlen);
#endif

class IReHLDSPlatform {
public:
	virtual uint32 time(uint32* pTime) = 0;
	virtual struct tm* localtime(uint32 time) = 0;
	virtual void srand(uint32 seed) = 0;
	virtual int rand() = 0;

#ifdef _WIN32
	virtual void Sleep(DWORD msec) = 0;
	virtual BOOL QueryPerfCounter(LARGE_INTEGER* counter) = 0;
	virtual BOOL QueryPerfFreq(LARGE_INTEGER* freq) = 0;
	virtual DWORD GetTickCount() = 0;
	virtual void GetLocalTime(LPSYSTEMTIME time) = 0;
	virtual void GetSystemTime(LPSYSTEMTIME time) = 0;
	virtual void GetTimeZoneInfo(LPTIME_ZONE_INFORMATION zinfo) = 0;
	virtual BOOL GetProcessTimes(HANDLE hProcess, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime) = 0;
	virtual void GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime) = 0;
#endif

	virtual SOCKET socket(int af, int type, int protocol) = 0;
	virtual int setsockopt(SOCKET s, int level, int optname, const char* optval, int optlen) = 0;
	virtual int closesocket(SOCKET s) = 0;
	virtual int recvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, socklen_t *fromlen) = 0;
	virtual int sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen) = 0;
	virtual int bind(SOCKET s, const struct sockaddr* addr, int namelen) = 0;
	virtual int getsockname(SOCKET s, struct sockaddr* name, socklen_t* namelen) = 0;
	virtual struct hostent* gethostbyname(const char *name) = 0;
	virtual int gethostname(char *name, int namelen) = 0;

#ifdef _WIN32
	virtual int ioctlsocket(SOCKET s, long cmd, u_long *argp) = 0;
	virtual int WSAGetLastError() = 0;
#endif

	virtual void SteamAPI_SetBreakpadAppID(uint32 unAppID) = 0;
	virtual void SteamAPI_UseBreakpadCrashHandler(char const *pchVersion, char const *pchDate, char const *pchTime, bool bFullMemoryDumps, void *pvContext, PFNPreMinidumpCallback m_pfnPreMinidumpCallback) = 0;
	virtual void SteamAPI_RegisterCallback(CCallbackBase *pCallback, int iCallback) = 0;
	virtual bool SteamAPI_Init() = 0;
	virtual void SteamAPI_UnregisterCallResult(class CCallbackBase *pCallback, SteamAPICall_t hAPICall) = 0;
	virtual ISteamApps* SteamApps() = 0;
	#ifdef VERSION_SAFE_STEAM_API_INTERFACES
	virtual bool SteamGameServer_InitSafe(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString) = 0;
	#else
	virtual bool SteamGameServer_Init(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString) = 0;
	#endif
	virtual ISteamGameServer* SteamGameServer() = 0;
	virtual void SteamGameServer_RunCallbacks() = 0;
	virtual void SteamAPI_RunCallbacks() = 0;
	virtual void SteamGameServer_Shutdown() = 0;
	virtual void SteamAPI_UnregisterCallback(CCallbackBase *pCallback) = 0;
};

class CSimplePlatform : public IReHLDSPlatform {
private:
#ifdef _WIN32
	setsockopt_proto setsockopt_v11;
	HMODULE wsock;
#endif

#ifdef VERSION_SAFE_STEAM_API_INTERFACES
	void* m_pGameServerContext;    // Actually CSteamGameServerAPIContext*
	void* m_pClientContext;        // Actually CSteamAPIContext*
#endif

public:
	CSimplePlatform();
	virtual ~CSimplePlatform();

	virtual uint32 time(uint32* pTime);
	virtual struct tm* localtime(uint32 time);
	virtual void srand(uint32 seed);
	virtual int rand();

#ifdef _WIN32
	virtual void Sleep(DWORD msec);
	virtual BOOL QueryPerfCounter(LARGE_INTEGER* counter);
	virtual BOOL QueryPerfFreq(LARGE_INTEGER* freq);
	virtual DWORD GetTickCount();
	virtual void GetLocalTime(LPSYSTEMTIME time);
	virtual void GetSystemTime(LPSYSTEMTIME time);
	virtual void GetTimeZoneInfo(LPTIME_ZONE_INFORMATION zinfo);
	virtual BOOL GetProcessTimes(HANDLE hProcess, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime);
	virtual void GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime);
#endif

	virtual SOCKET socket(int af, int type, int protocol);
	virtual int setsockopt(SOCKET s, int level, int optname, const char* optval, int optlen);
	virtual int closesocket(SOCKET s);
	virtual int recvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, socklen_t *fromlen);
	virtual int sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);
	virtual int bind(SOCKET s, const struct sockaddr* addr, int namelen);
	virtual int getsockname(SOCKET s, struct sockaddr* name, socklen_t* namelen);
	virtual struct hostent* gethostbyname(const char *name);
	virtual int gethostname(char *name, int namelen);

#ifdef _WIN32
	virtual int ioctlsocket(SOCKET s, long cmd, u_long *argp);
	virtual int WSAGetLastError();
#endif

	virtual void SteamAPI_SetBreakpadAppID(uint32 unAppID);
	virtual void SteamAPI_UseBreakpadCrashHandler(char const *pchVersion, char const *pchDate, char const *pchTime, bool bFullMemoryDumps, void *pvContext, PFNPreMinidumpCallback m_pfnPreMinidumpCallback);
	virtual void SteamAPI_RegisterCallback(CCallbackBase *pCallback, int iCallback);
	virtual bool SteamAPI_Init();
	virtual void SteamAPI_UnregisterCallResult(class CCallbackBase *pCallback, SteamAPICall_t hAPICall);
	virtual ISteamApps* SteamApps();
	#ifdef VERSION_SAFE_STEAM_API_INTERFACES
	virtual bool SteamGameServer_InitSafe(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString);
	#else
	virtual bool SteamGameServer_Init(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString);
	#endif
	virtual ISteamGameServer* SteamGameServer();
	virtual void SteamGameServer_RunCallbacks();
	virtual void SteamAPI_RunCallbacks();
	virtual void SteamGameServer_Shutdown();
	virtual void SteamAPI_UnregisterCallback(CCallbackBase *pCallback);
};

class CRehldsPlatformHolder {
private:
	static IReHLDSPlatform* m_Platform;

public:
	static IReHLDSPlatform* get();
	static void set(IReHLDSPlatform* p);
};

extern void NORETURN rehlds_syserror(const char* fmt, ...);
