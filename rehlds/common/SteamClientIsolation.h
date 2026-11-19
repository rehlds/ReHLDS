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

#ifdef _WIN32

#include <windows.h>
#include <string.h>

// While a Steam client is running, steam_api.dll loads the client's steamclient.dll by the full path from
// HKCU\Software\Valve\Steam\ActiveProcess\SteamClientDll, even when the server folder has its own steamclient.dll
// (both end up in the process). The client's build may not match the server's tier0_s.dll / vstdlib_s.dll
// ("entry point not found") or may need DLLs the server does not have (SDL3), and the server fails to start.
//
// CSteamClientIsolation hides HKCU from steam_api for the duration of SteamGameServer_Init: HKCU is mapped
// (for this process only, RegOverridePredefKey) to an empty volatile key, so steam_api sees no running client and
// takes the steamclient.dll next to the executable. Nothing is changed for other processes or for the Steam client.
// Active only when the server folder has steamclient.dll; -nolocalsteamclient restores the vanilla behaviour.
class CSteamClientIsolation
{
public:
	explicit CSteamClientIsolation(bool enabled) : m_hKey(nullptr)
	{
		if (!enabled || !HasLocalSteamClient())
			return;

		HKEY hKey = nullptr;
		DWORD disposition = 0;
		if (RegCreateKeyExA(HKEY_CURRENT_USER, ISOLATION_KEY, 0, nullptr, REG_OPTION_VOLATILE, KEY_READ, nullptr, &hKey, &disposition) != ERROR_SUCCESS)
			return;

		if (RegOverridePredefKey(HKEY_CURRENT_USER, hKey) != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			return;
		}

		m_hKey = hKey;
	}

	~CSteamClientIsolation()
	{
		if (!m_hKey)
			return;

		RegOverridePredefKey(HKEY_CURRENT_USER, nullptr);
		RegCloseKey(m_hKey);
		RegDeleteKeyA(HKEY_CURRENT_USER, ISOLATION_KEY);
	}

	bool IsActive() const { return m_hKey != nullptr; }

private:
	CSteamClientIsolation(const CSteamClientIsolation &) = delete;
	CSteamClientIsolation &operator=(const CSteamClientIsolation &) = delete;

	static bool HasLocalSteamClient()
	{
		char path[MAX_PATH];
		DWORD len = GetModuleFileNameA(nullptr, path, sizeof(path));
		if (len == 0 || len >= sizeof(path))
			return false;

		char *slash = strrchr(path, '\\');
		if (!slash)
			return false;

		const char steamClient[] = "steamclient.dll";
		if ((size_t)(slash + 1 - path) + sizeof(steamClient) > sizeof(path))
			return false;

		strcpy(slash + 1, steamClient);
		return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
	}

	static constexpr const char *ISOLATION_KEY = "Software\\ReHLDS\\SteamClientIsolation";

	HKEY m_hKey;
};

#endif // _WIN32
