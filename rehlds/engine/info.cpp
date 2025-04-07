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

// NOTE: This file contains a lot of fixes that are not covered by REHLDS_FIXES define.
// TODO: Most of the Info_ functions can be speedup via removing unneded copy of key and values.

struct info_field_t
{
	char*	name;
	bool	integer;
};

info_field_t g_info_important_fields[] =
{
	// name				integer
	{ "name",			false },
	{ "model",			false },

	// model colors
	{ "topcolor",		true },
	{ "bottomcolor",	true },

	// network
	{ "rate",			true },
	{ "cl_updaterate",	true },
	{ "cl_lw",			true },
	{ "cl_lc",			true },

	// hltv flag
	{ "*hltv",			true },

	// avatars
	{ "*sid",			false }, // transmit as string because it's int64

	// gui/text menus
	{ "_vgui_menus",	true }
};

std::vector<info_field_t *> g_info_transmitted_fields;

// Searches the string for the given
// key and returns the associated value, or an empty string.
const char* EXT_FUNC Info_ValueForKey(const char *s, const char *lookup)
{
#ifdef REHLDS_FIXES
	static char valueBuf[INFO_MAX_BUFFER_VALUES][MAX_KV_LEN];
	static int valueIndex;

	size_t lookupLen = Q_strlen(lookup);
	while (*s == '\\')
	{
		// skip starting slash
		const char* key = ++s;

		// skip key
		while (*s != '\\') {
			// Add some sanity checks because it's external function
			if (*s == '\0')
				return "";

			s++;
		}

		size_t keyLen = s - key;
		const char* value = ++s; // skip separating slash

		// skip value
		while (*s != '\\' && *s != '\0')
			s++;

		size_t valueLen = Q_min(s - value, MAX_KV_LEN - 1);

		if (keyLen == lookupLen && !Q_strncmp(key, lookup, lookupLen))
		{
			char* dest = valueBuf[valueIndex];
			Q_memcpy(dest, value, valueLen);
			dest[valueLen] = '\0';

			valueIndex = (valueIndex + 1) % INFO_MAX_BUFFER_VALUES;
			return dest;
		}
	}

	return "";
#else
	// use few (two?) buffers so compares work without stomping on each other
	static char value[INFO_MAX_BUFFER_VALUES][MAX_KV_LEN];
	static int valueindex;
	char pkey[MAX_KV_LEN];
	char *c;
	int nCount;

	while (*s)
	{
		if (*s == '\\')
		{
			s++;	// skip the slash
		}

		// Copy a key
		nCount = 0;
		c = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				return "";		// key should end with a \, not a NULL, but suppose its value as absent
			}
			if (nCount >= MAX_KV_LEN)
			{
				s++;
				continue;	// skip oversized key chars till the slash or EOL
			}
			*c++ = *s++;
			nCount++;
		}
		*c = 0;
		s++;	// skip the slash

				// Copy a value
		nCount = 0;
		c = value[valueindex];
		while (*s != '\\')
		{
			if (!*s)
			{
				break;		// allow value to be ended with NULL
			}
			if (nCount >= MAX_KV_LEN)
			{
				s++;
				continue;	// skip oversized value chars till the slash or EOL
			}
			*c++ = *s++;
			nCount++;
		}
		*c = 0;

		if (!Q_strcmp(lookup, pkey))
		{
			c = value[valueindex];
			valueindex = (valueindex + 1) % INFO_MAX_BUFFER_VALUES;
			return c;
		}
	}

	return "";
#endif
}

void Info_RemoveKey(char *s, const char *lookup)
{
#ifdef REHLDS_FIXES
	size_t lookupLen = Q_strlen(lookup);

	while (*s == '\\')
	{
		char* start = s;

		// skip starting slash
		const char* key = ++s;

		// skip key
		while (*s != '\\') {
			if (*s == '\0')
				return;

			s++;
		}

		size_t keyLen = s - key;
		++s; // skip separating slash

		// skip value
		while (*s != '\\' && *s != '\0')
			s++;

		if (keyLen != lookupLen)
			continue;

		if (!Q_memcmp(key, lookup, lookupLen))
		{
			// cut key and value
			Q_memmove(start, s, Q_strlen(s) + 1);
			break;
		}
	}
#else
	char pkey[MAX_KV_LEN];
	char value[MAX_KV_LEN];
	char *start;
	char *c;
	int cmpsize;
	int nCount;

	if (Q_strstr(lookup, "\\"))
	{
		Con_Printf("Can't use a key with a \\\n");
		return;
	}

	cmpsize = Q_strlen(lookup);
	if (cmpsize > MAX_KV_LEN - 1)
		cmpsize = MAX_KV_LEN - 1;

	while (*s)
	{
		start = s;

		if (*s == '\\')
		{
			s++;	// skip the slash
		}

		// Copy a key
		nCount = 0;
		c = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				break;		// key should end with a \, not a NULL, but allow to remove it
			}
			if (nCount >= MAX_KV_LEN)
			{
				s++;
				continue;	// skip oversized key chars till the slash or EOL
			}
			*c++ = *s++;
			nCount++;
		}
		*c = 0;
		if (*s)
			s++;	// skip the slash

		// Copy a value
		nCount = 0;
		c = value;
		while (*s != '\\')
		{
			if (!*s)
			{
				break;		// allow value to be ended with NULL
			}
			if (nCount >= MAX_KV_LEN)
			{
				s++;
				continue;	// skip oversized value chars till the slash or EOL
			}
			*c++ = *s++;
			nCount++;
		}
		*c = 0;

		// Compare keys
		if (!Q_strncmp(lookup, pkey, cmpsize))
		{
			Q_strcpy_s(start, s);	// remove this part
			s = start;	// continue searching
		}
	}
#endif
}

void Info_RemovePrefixedKeys(char *s, const char prefix)
{
#ifdef REHLDS_FIXES
	while (*s == '\\')
	{
		char* start = s;

		// skip starting slash
		const char* key = ++s;

		// skip key
		while (*s != '\\') {
			if (*s == '\0')
				return;

			s++;
		}

		// skip separating slash
		++s;

		// skip value
		while (*s != '\\' && *s != '\0')
			s++;

		if (key[0] == prefix)
		{
			Q_memmove(start, s, Q_strlen(s) + 1);
			s = start;
		}
	}
#else
	char pkey[MAX_KV_LEN];
	char value[MAX_KV_LEN];
	char *start;
	char *c;
	int nCount;

	while (*s)
	{
		start = s;

		if (*s == '\\')
		{
			s++;	// skip the slash
		}

		// Copy a key
		nCount = 0;
		c = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				break;		// key should end with a \, not a NULL, but allow to remove it
			}
			if (nCount >= MAX_KV_LEN)
			{
				s++;
				continue;	// skip oversized key chars till the slash or EOL
			}
			*c++ = *s++;
			nCount++;
		}
		*c = 0;
		if (*s)
			s++;	// skip the slash

		// Copy a value
		nCount = 0;
		c = value;
		while (*s != '\\')
		{
			if (!*s)
			{
				break;		// allow value to be ended with NULL
			}
			if (nCount >= MAX_KV_LEN)
			{
				s++;
				continue;	// skip oversized value chars till the slash or EOL
			}
			*c++ = *s++;
			nCount++;
		}
		*c = 0;

		if (pkey[0] == prefix)
		{
			Q_strcpy_s(start, s);	// remove this part
			s = start;	// continue searching
		}
	}
#endif
}

#ifdef REHLDS_FIXES
qboolean Info_IsKeyImportant(const char *key)
{
	if (key[0] == '*')
		return true;

	for (auto& field : g_info_important_fields) {
		if (!Q_strcmp(key, field.name))
			return true;
	}

	return false;
}

qboolean Info_IsKeyImportant(const char *key, size_t keyLen)
{
	char copy[MAX_KV_LEN];
	keyLen = min(keyLen, sizeof(copy) - 1);
	Q_memcpy(copy, key, keyLen);
	copy[keyLen] = '\0';
	return Info_IsKeyImportant(copy);
}
#else
qboolean Info_IsKeyImportant(const char *key)
{
	if (key[0] == '*')
		return true;

	if (!Q_strcmp(key, "name"))
		return true;
	if (!Q_strcmp(key, "model"))
		return true;
	if (!Q_strcmp(key, "rate"))
		return true;
	if (!Q_strcmp(key, "topcolor"))
		return true;
	if (!Q_strcmp(key, "bottomcolor"))
		return true;
	if (!Q_strcmp(key, "cl_updaterate"))
		return true;
	if (!Q_strcmp(key, "cl_lw"))
		return true;
	if (!Q_strcmp(key, "cl_lc"))
		return true;
	if (!Q_strcmp(key, "*hltv"))
		return true;
	if (!Q_strcmp(key, "*sid"))
		return true;

	return false;
}
#endif

const char *Info_FindLargestKey(const char *s, int maxsize)
{
#ifdef REHLDS_FIXES
	static char largestKey[MAX_KV_LEN];
	size_t largestLen = 0;

	while (*s == '\\')
	{
		// skip starting slash
		const char* key = ++s;

		// skip key
		while (*s != '\\') {
			if (*s == '\0')
				return "";

			s++;
		}

		size_t keyLen = s - key;
		const char* value = ++s; // skip separating slash

			 // skip value
		while (*s != '\\' && *s != '\0')
			s++;

		size_t valueLen = s - value;
		size_t totalLen = keyLen + valueLen;

		if (totalLen > largestLen && !Info_IsKeyImportant(key, keyLen)) {
			largestLen = totalLen;
			Q_memcpy(largestKey, key, keyLen);
			largestKey[keyLen] = '\0';
		}
	}

	return largestLen ? largestKey : "";
#else
	static char largest_key[MAX_KV_LEN];
	char key[MAX_KV_LEN];
	char value[MAX_KV_LEN];
	char *c;
	int nCount;
	int largest_size = 0;

	largest_key[0] = 0;

	while (*s)
	{
		if (*s == '\\')
		{
			s++;	// skip the slash
		}

		// Copy a key
		nCount = 0;
		c = key;
		while (*s != '\\')
		{
			if (!*s)		// key should end with a \, not a NULL, return this key, so it will be deleted as wrong
			{
				*c = 0;
				Q_strcpy(largest_key, key);
				return largest_key;
			}
			if (nCount >= MAX_KV_LEN)	// oversized key, return this key, so it will be deleted as wrong
			{
				*c = 0;
				Q_strcpy(largest_key, key);
				return largest_key;
			}
			*c++ = *s++;
			nCount++;
		}
		*c = 0;
		s++;	// skip the slash

		// Get length
		int size = c - key;

		// Copy a value
		nCount = 0;
		c = value;
		while (*s != '\\')
		{
			if (!*s)
			{
				break;				// allow value to be ended with NULL
			}
			if (nCount >= MAX_KV_LEN)	// oversized value, return this key, so it will be deleted as wrong
			{
				*c = 0;
				Q_strcpy(largest_key, key);
				return largest_key;
			}
			*c++ = *s++;
			nCount++;
		}
		*c = 0;

		// Add length
		size += c - value;

		if (size > largest_size && !Info_IsKeyImportant(key))
		{
			largest_size = size;
			Q_strcpy(largest_key, key);
		}
	}

	return largest_key;
#endif
}

#ifdef REHLDS_FIXES
qboolean Info_SetValueForStarKey(char *s, const char *key, const char *value, size_t maxsize)
{
	char newArray[MAX_INFO_STRING], valueBuf[MAX_KV_LEN];

	if (!key || !value)
	{
		Con_Printf("Keys and values can't be null\n");
		return FALSE;
	}

	if (key[0] == '\0')
	{
		Con_Printf("Keys can't be an empty string\n");
		return FALSE;
	}

	if (Q_strchr(key, '\\') || Q_strchr(value, '\\'))
	{
		Con_Printf("Can't use keys or values with a \\\n");
		return FALSE;
	}

	if (Q_strchr(key, '\"') || Q_strchr(value, '\"'))
	{
		Con_Printf("Can't use keys or values with a \"\n");
		return FALSE;
	}

	if (Q_strstr(key, "..") || Q_strstr(value, ".."))
	{
		Con_Printf("Can't use keys or values with a ..\n");
		return FALSE;
	}

	int keyLen = Q_strlen(key);
	int valueLen = Q_strlen(value);

	if (keyLen >= MAX_KV_LEN || valueLen >= MAX_KV_LEN)
	{
		Con_Printf("Keys and values must be < %i characters\n", MAX_KV_LEN);
		return FALSE;
	}

	if (!Q_UnicodeValidate(key) || !Q_UnicodeValidate(value))
	{
		Con_Printf("Keys and values must be valid utf8 text\n");
		return FALSE;
	}

	// Remove current key/value and return if we doesn't specified to set a value
	Info_RemoveKey(s, key);
	if (value[0] == '\0')
	{
		return TRUE;
	}

	// auto lowercase team
	if (!Q_strcmp(key, "team")) {
		value = Q_strcpy(valueBuf, value);
		Q_strlwr(valueBuf);
	}

	// Create key/value pair
	size_t neededLength = Q_snprintf(newArray, sizeof newArray, "\\%s\\%s", key, value);

	if (Q_strlen(s) + neededLength >= maxsize)
	{
		// no more room in the buffer to add key/value
		if (!Info_IsKeyImportant(key))
		{
			// no room to add setting
			Con_Printf("Info string length exceeded\n");
			return FALSE;
		}

		// keep removing the largest key/values until we have a room
		do
		{
			const char* largekey = Info_FindLargestKey(s, maxsize);
			if (largekey[0] == '\0')
			{
				// no room to add setting
				Con_Printf("Info string length exceeded\n");
				return FALSE;
			}
			Info_RemoveKey(s, largekey);
		} while ((int)Q_strlen(s) + neededLength >= maxsize);
	}

	Q_strcat(s, newArray);

	return TRUE;
}
#else
void Info_SetValueForStarKey(char *s, const char *key, const char *value, int maxsize)
{
	char newArray[MAX_INFO_STRING];
	char *v;
	int c;

	if (!key || !value)
	{
		Con_Printf("Keys and values can't be null\n");
		return;
	}

	if (key[0] == 0)
	{
		Con_Printf("Keys can't be an empty string\n");
		return;
	}

	if (Q_strstr(key, "\\") || Q_strstr(value, "\\"))
	{
		Con_Printf("Can't use keys or values with a \\\n");
		return;
	}

	if (Q_strstr(key, "..") || Q_strstr(value, ".."))
	{
		// TODO: Why silently return?
		//Con_Printf("Can't use keys or values with a ..\n");
		return;
	}

	if (Q_strstr(key, "\"") || Q_strstr(value, "\""))
	{
		Con_Printf("Can't use keys or values with a \"\n");
		return;
	}

	int keyLen = Q_strlen(key);
	int valueLan = Q_strlen(value);

	if (keyLen >= MAX_KV_LEN || valueLan >= MAX_KV_LEN)
	{
		Con_Printf("Keys and values must be < %i characters\n", MAX_KV_LEN);
		return;
	}

	if (!Q_UnicodeValidate(key) || !Q_UnicodeValidate(value))
	{
		Con_Printf("Keys and values must be valid utf8 text\n");
		return;
	}

	// Remove current key/value and return if we doesn't specified to set a value
	Info_RemoveKey(s, key);
	if (value[0] == 0)
	{
		return;
	}

	// Create key/value pair
	Q_snprintf(newArray, MAX_INFO_STRING - 1, "\\%s\\%s", key, value);
	newArray[MAX_INFO_STRING - 1] = 0;

	int neededLength = Q_strlen(newArray);
	if ((int)Q_strlen(s) + neededLength >= maxsize)
	{
		// no more room in the buffer to add key/value
		if (!Info_IsKeyImportant(key))
		{
			// no room to add setting
			Con_Printf("Info string length exceeded\n");
			return;
		}

		// keep removing the largest key/values until we have a room
		const char *largekey;
		do
		{
			largekey = Info_FindLargestKey(s, maxsize);
			if (largekey[0] == 0)
			{
				// no room to add setting
				Con_Printf("Info string length exceeded\n");
				return;
			}
			Info_RemoveKey(s, largekey);
		} while ((int)Q_strlen(s) + neededLength >= maxsize);
	}

	// auto lowercase team
	bool lowerCaseValue = Q_stricmp(key, "team") == 0;
	s += Q_strlen(s);
	v = newArray;
	while (*v)
	{
		c = (unsigned char)*v++;
		if (lowerCaseValue)
		{
			c = tolower(c);
		}
		*s++ = c;
	}
	*s = 0;
}
#endif

void Info_SetValueForKey(char *s, const char *key, const char *value, int maxsize)
{
	if (key[0] == '*')
	{
		Con_Printf("Can't set * keys\n");
		return;
	}

	Info_SetValueForStarKey(s, key, value, maxsize);
}

void Info_Print(const char *s)
{
	char key[MAX_KV_LEN];
	char value[MAX_KV_LEN];
	char *c;
	int l;
	int nCount;

	while (*s)
	{
		if (*s == '\\')
		{
			s++;	// skip the slash
		}

		// Copy a key
		nCount = 0;
		c = key;
		while (*s != '\\')
		{
			if (!*s)
			{
				break;		// key should end with a \, not a NULL, but allow to print it
			}
			if (nCount >= MAX_KV_LEN)
			{
				s++;
				continue;	// skip oversized key chars till the slash or EOL
			}
			*c++ = *s++;
			nCount++;
		}
		*c = 0;
		if (*s)
			s++;	// skip the slash

		// Pad and print a key
		l = c - key;
		if (l < 20)
		{
			Q_memset(c, ' ', 20 - l);
			key[20] = 0;
		}
		Con_Printf("%s", key);

		if (!*s)
		{
			Con_Printf("MISSING VALUE\n");
			return;
		}

		// Copy a value
		nCount = 0;
		c = value;
		while (*s != '\\')
		{
			if (!*s)
			{
				break;		// allow value to be ended with NULL
			}
			if (nCount >= MAX_KV_LEN)
			{
				s++;
				continue;	// skip oversized value chars till the slash or EOL
			}
			*c++ = *s++;
			nCount++;
		}
		*c = 0;

		Con_Printf("%s\n", value);
	}
}

qboolean Info_IsValid(const char *s)
{
#ifdef REHLDS_FIXES
	struct {
		const char*	start;
		size_t		len;
	} existingKeys[MAX_INFO_STRING * 2 / 4];
	size_t existingKeysNum = 0;

	auto isAlreadyExists = [&](const char* key, size_t len)
	{
		for (size_t i = 0; i < existingKeysNum; i++) {
			if (len == existingKeys[i].len && !Q_memcmp(key, existingKeys[i].start, existingKeys[i].len))
				return true;
		}
		return false;
	};

	// invalid utf8 chars are deprecated
	if (!Q_UnicodeValidate(s))
	{
		return FALSE;
	}

	while (*s == '\\')
	{
		const char* key = ++s;

		// keys and values are separated by another slash
		while (*s != '\\')
		{
			// key should end with a '\', not a NULL
			if (*s == '\0')
				return FALSE;

			// quotes are deprecated
			if (*s == '"')
				return FALSE;

			// ".." deprecated. don't know why. model path?
			if (*s == '.' && *(s + 1) == '.')
				return FALSE;

			s++;
		}

		size_t keyLen = s - key;
		if (keyLen == 0 || keyLen >= MAX_KV_LEN)
			return FALSE;

		if (isAlreadyExists(key, keyLen))
			return FALSE;

		const char* value = ++s; // skip the slash

		// values should be ended by eos or slash
		while (*s != '\\' && *s != '\0')
		{
			// quotes are deprecated
			if (*s == '"')
				return FALSE;

			// ".." deprecated
			if (*s == '.' && *(s + 1) == '.')
				return FALSE;

			s++;
		}

		size_t valueLen = s - value;
		if (valueLen == 0 || valueLen >= MAX_KV_LEN)
			return FALSE;

		if (*s == '\0')
			return TRUE;

		if (existingKeysNum == ARRAYSIZE(existingKeys))
			return FALSE;

		existingKeys[existingKeysNum].start = key;
		existingKeys[existingKeysNum].len = keyLen;
		existingKeysNum++;
	}

	return FALSE;
#else
	char key[MAX_KV_LEN];
	char value[MAX_KV_LEN];
	char *c;
	int nCount;

	while (*s)
	{
		if (*s == '\\')
		{
			s++;	// skip the slash
		}

		// Copy a key
		nCount = 0;
		c = key;
		while (*s != '\\')
		{
			if (!*s)
			{
				return FALSE;		// key should end with a \, not a NULL
			}
			if (nCount >= MAX_KV_LEN)
			{
				return FALSE;		// key length should be less then MAX_KV_LEN
			}
			*c++ = *s++;
			nCount++;
		}
		*c = 0;
		s++;	// skip the slash

				// Copy a value
		nCount = 0;
		c = value;
		while (*s != '\\')
		{
			if (!*s)
			{
				break;				// allow value to be ended with NULL
			}
			if (nCount >= MAX_KV_LEN)
			{
				return FALSE;		// value length should be less then MAX_KV_LEN
			}
			*c++ = *s++;
			nCount++;
		}
		*c = 0;

		if (value[0] == 0)
		{
			return FALSE;	// empty values are not valid
		}

		if (!*s)
		{
			return TRUE;	// EOL, info is valid
		}
	}

	return FALSE;
#endif
}

#ifdef REHLDS_FIXES
void Info_SetFieldsToTransmit()
{
	// clean all
	for (auto field : g_info_transmitted_fields) {
		free(field->name);
		delete field;
	}
	g_info_transmitted_fields.clear();

	char keys[512];
	Q_strlcpy(keys, sv_rehlds_userinfo_transmitted_fields.string);

	auto isIntegerField = [](const char* key)
	{
		for (auto& x : g_info_important_fields) {
			if (!Q_strcmp(key, x.name))
				return x.integer;
		}
		return false;
	};

	for (char *key = Q_strtok(keys, "\\"); key; key = Q_strtok(nullptr, "\\"))
	{
		if (key[0] == '_') {
			Con_Printf("%s: private key '%s' couldn't be transmitted.\n", __FUNCTION__, key);
			continue;
		}

		if (Q_strlen(key) >= MAX_KV_LEN) {
			Con_Printf("%s: key '%s' is too long (should be < %i characters)\n", __FUNCTION__, key, MAX_KV_LEN);
			continue;
		}

		if (std::find_if(g_info_transmitted_fields.begin(), g_info_transmitted_fields.end(), [key](info_field_t* field) { return !Q_strcmp(key, field->name); }) == g_info_transmitted_fields.end()) {
			auto field = new info_field_t;
			field->name = Q_strdup(key);
			field->integer = isIntegerField(key);
			g_info_transmitted_fields.push_back(field);
		}
	}
}

void Info_CollectFields(char *destInfo, const char *srcInfo, size_t maxsize)
{
	if (g_info_transmitted_fields.empty()) {
		Q_strlcpy(destInfo, srcInfo, maxsize);
		Info_RemovePrefixedKeys(destInfo, '_');
		return;
	}

	char add[512], valueBuf[32];
	size_t userInfoLength = 0;

	for (auto field : g_info_transmitted_fields)
	{
		const char *value = Info_ValueForKey(srcInfo, field->name);
		if (value[0] == '\0')
			continue;

		// clean garbage from integer fields
		if (field->integer)
		{
			// don't send zero fields
			int intValue = Q_atoi(value);
			if (!intValue)
				continue;

			Q_sprintf(valueBuf, "%i", intValue);
			value = valueBuf;
		}

		// don't write truncated keys/values
		size_t len = Q_sprintf(add, "\\%s\\%s", field->name, value);
		if (userInfoLength + len < maxsize) {
			Q_strcpy(destInfo + userInfoLength, add);
			userInfoLength += len;
		}
	}

	destInfo[userInfoLength] = '\0';
}
#endif // REHLDS_FIXES
