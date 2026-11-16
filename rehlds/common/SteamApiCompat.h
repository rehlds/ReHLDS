/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the MIT License; see the LICENSE file in the root of
*    this repository.
*
*/

#pragma once

// HLDS ships two steam_api builds:
//   legacy (Steamworks SDK 1.22 on Windows, 1.23a on Linux; HLDS builds up to 8684) and
//   HL25   (SDK 1.53a on Windows, 1.57 on Linux; HLDS builds 9890 and later).
// HL25 builds dropped the flat SteamGameServer_Init(), SteamGameServer() and SteamApps() exports in favour of
// the SteamInternal_* entry points. They are resolved here at run time, so one binary loads with either
// steam_api. The interfaces stay the versions the code is compiled against (SteamGameServer011,
// STEAMAPPS_INTERFACE_VERSION005); steamclient still serves them by name.
// Do not call ::SteamGameServer_Init(), ::SteamGameServer() or ::SteamApps() directly: that brings the
// imports back and the module will not load with HL25's steam_api.

#include <steam/steam_api.h>
#include <steam/steam_gameserver.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <string.h>

namespace SteamApiCompat
{
	typedef bool (*GameServerInit_t)(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString);
	typedef ISteamGameServer *(*GameServer_t)();
	typedef ISteamApps *(*Apps_t)();
	typedef HSteamUser (*GetHSteamUser_t)();
	typedef void *(*FindOrCreateInterface_t)(HSteamUser hSteamUser, const char *pszVersion);

	struct Entries
	{
		GameServerInit_t legacyGameServerInit;   // legacy steam_api
		GameServer_t legacyGameServer;
		Apps_t legacyApps;
		GameServerInit_t internalGameServerInit; // HL25 steam_api
		GetHSteamUser_t gameServerUser;
		GetHSteamUser_t apiUser;
		FindOrCreateInterface_t gameServerInterface;
		FindOrCreateInterface_t userInterface;
	};

	inline void *Proc(const char *name)
	{
#ifdef _WIN32
		HMODULE hSteamApi = GetModuleHandleA("steam_api.dll");
		return hSteamApi ? (void *)GetProcAddress(hSteamApi, name) : nullptr;
#else
		return dlsym(RTLD_DEFAULT, name);
#endif
	}

	inline Entries &Get()
	{
		static Entries entries;
		static bool resolved = false;

		if (!resolved)
		{
			entries.legacyGameServerInit = (GameServerInit_t)Proc("SteamGameServer_Init");
			entries.legacyGameServer = (GameServer_t)Proc("SteamGameServer");
			entries.legacyApps = (Apps_t)Proc("SteamApps");
			entries.internalGameServerInit = (GameServerInit_t)Proc("SteamInternal_GameServer_Init");
			entries.gameServerUser = (GetHSteamUser_t)Proc("SteamGameServer_GetHSteamUser");
			entries.apiUser = (GetHSteamUser_t)Proc("SteamAPI_GetHSteamUser");
			entries.gameServerInterface = (FindOrCreateInterface_t)Proc("SteamInternal_FindOrCreateGameServerInterface");
			entries.userInterface = (FindOrCreateInterface_t)Proc("SteamInternal_FindOrCreateUserInterface");
			resolved = true;
		}

		return entries;
	}

	inline ISteamGameServer *&CachedGameServer()
	{
		static ISteamGameServer *pGameServer = nullptr;
		return pGameServer;
	}

	// Copies the engine version string into pszOut in the plain "x.x.x.x" form.
	// The engine builds it as "<PatchVersion>/<suffix>", e.g. "1.1.2.2/Stdio" (see Host_Version in host.cpp).
	// Steamworks documents this argument of SteamGameServer_Init as "x.x.x.x" (the master server compares it
	// to detect out-of-date servers), so the "/..." suffix is cut off for the HL25 steam_api.
	inline void PlainVersionString(const char *pchVersionString, char *pszOut, size_t outSize)
	{
		if (!outSize)
			return;

		strncpy(pszOut, pchVersionString ? pchVersionString : "", outSize - 1);
		pszOut[outSize - 1] = '\0';

		char *pSlash = strchr(pszOut, '/');
		if (pSlash)
			*pSlash = '\0';
	}

	// HL25 steam_api (SDK 1.53a/1.57): its SteamGameServer_Init is only an inline wrapper in the SDK headers that calls
	// SteamInternal_GameServer_Init(unIP, 0, usGamePort, usQueryPort, eServerMode, pchVersionString).
	// Do the same thing here: 0 for the legacy Steam port (unused since the SDK dropped it) and a plain version.
	inline bool GameServer_InitHL25(Entries &api, uint32 unIP, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString)
	{
		char szVersion[32];
		PlainVersionString(pchVersionString, szVersion, sizeof(szVersion));

		return api.internalGameServerInit(unIP, 0, usGamePort, usQueryPort, eServerMode, szVersion);
	}

	inline bool GameServer_Init(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString)
	{
		Entries &api = Get();
		CachedGameServer() = nullptr;

		// legacy steam_api (SDK 1.22/1.23a): exported SteamGameServer_Init, arguments passed through unchanged
		if (api.legacyGameServerInit)
			return api.legacyGameServerInit(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, pchVersionString);

		if (api.internalGameServerInit)
			return GameServer_InitHL25(api, unIP, usGamePort, usQueryPort, eServerMode, pchVersionString);

		return false;
	}

	inline ISteamGameServer *GameServer()
	{
		Entries &api = Get();
		if (api.legacyGameServer)
			return api.legacyGameServer();

		ISteamGameServer *&pGameServer = CachedGameServer();
		if (!pGameServer && api.gameServerInterface && api.gameServerUser)
		{
			HSteamUser hUser = api.gameServerUser();
			if (hUser)
				pGameServer = (ISteamGameServer *)api.gameServerInterface(hUser, STEAMGAMESERVER_INTERFACE_VERSION);
		}

		return pGameServer;
	}

	inline ISteamApps *Apps()
	{
		Entries &api = Get();
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

	inline void GameServer_Shutdown()
	{
		CachedGameServer() = nullptr;
		::SteamGameServer_Shutdown();
	}
}
