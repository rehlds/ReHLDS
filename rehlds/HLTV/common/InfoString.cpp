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

InfoString::InfoString(char *string, unsigned int maxSize) :
	m_MaxSize(0),
	m_String(nullptr)
{
	unsigned int len = Q_strlen(string) + 1;
	if (len < maxSize) {
		len = maxSize;
	}

	SetMaxSize(len);
	SetString(string);
}

InfoString::InfoString() :
	m_MaxSize(0),
	m_String(nullptr)
{
}

InfoString::InfoString(unsigned int maxSize) :
	m_MaxSize(0),
	m_String(nullptr)
{
	SetMaxSize(maxSize);
}

InfoString::InfoString(char *string) :
	m_MaxSize(0),
	m_String(nullptr)
{
	unsigned int len = Q_strlen(string) + 1;
	if (len < MAX_INFO_LEN) {
		len = MAX_INFO_LEN;
	}

	SetMaxSize(len);
	SetString(string);
}

InfoString::~InfoString()
{
	if (m_String) {
		Mem_Free(m_String);
		m_String = nullptr;
	}
}

bool InfoString::SetString(char *string)
{
	if (!m_String || !string) {
		return false;
	}

	if (Q_strlen(string) >= m_MaxSize) {
		return false;
	}

	Q_strlcpy(m_String, string, m_MaxSize);
	return true;
}

void InfoString::SetMaxSize(unsigned int maxSize)
{
	char *newBuffer = (char *)Mem_ZeroMalloc(maxSize);
	if (!newBuffer) {
		return;
	}

	if (m_String)
	{
		if (maxSize > Q_strlen(m_String)) {
			Q_strlcpy(newBuffer, m_String, maxSize);
		}

		Mem_Free(m_String);
	}

	m_MaxSize = maxSize;
	m_String = newBuffer;
}

int InfoString::GetMaxSize()
{
	return m_MaxSize;
}

int InfoString::GetCurrentSize()
{
	return Q_strlen(m_String);
}

void InfoString::Clear()
{
	if (m_String) {
		Q_memset(m_String, 0, m_MaxSize);
	}
}

char *InfoString::GetString()
{
	return m_String;
}

// Searches the string for the given key and returns the associated value, or an empty string.
char *InfoString::ValueForKey(const char *key)
{
	// use two buffers so compares work without stomping on each other
	static char value[MAX_INFO_VALUES][MAX_INFO_VALUE];
	static int valueindex;
	char pkey[MAX_INFO_KEY];
	char *c;
	char *s = m_String;
	int nCount;

	while (*s)
	{
		if (*s == '\\') {
			// skip the slash
			s++;
		}

		// Copy a key
		nCount = 0;
		c = pkey;

		while (*s != '\\')
		{
			if (!*s)
			{
				// key should end with a \, not a nullptr, but suppose its value as absent
				return "";
			}

			if (nCount >= MAX_INFO_KEY - 1)
			{
				s++;
				// skip oversized key chars till the slash or EOL
				continue;
			}

			*c++ = *s++;
			nCount++;
		}

		*c = '\0';

		// skip the slash
		s++;

		// Copy a value
		nCount = 0;
		c = value[valueindex];

		while (*s != '\\')
		{
			if (!*s)
			{
				// allow value to be ended with nullptr
				break;
			}

			if (nCount >= MAX_INFO_VALUE - 1)
			{
				s++;
				// skip oversized value chars till the slash or EOL
				continue;
			}

			*c++ = *s++;
			nCount++;
		}

		*c = '\0';

		if (!Q_strcmp(key, pkey))
		{
			c = value[valueindex];
			valueindex = (valueindex + 1) % MAX_INFO_VALUES;
			return c;
		}
	}

	return "";
}

bool InfoString::RemoveKey(const char *key)
{
	char *start;
	char pkey[MAX_INFO_KEY];
	char value[MAX_INFO_VALUE];
	char *c;
	char *s;
	int cmpsize;
	int nCount;

	s = m_String;

	if (Q_strchr(key, '\\')) {
		return false;
	}

	cmpsize = Q_strlen(key);
	if (cmpsize > MAX_INFO_LEN - 1) {
		cmpsize = MAX_INFO_LEN - 1;
	}

	bool found = false;
	while (*s)
	{
		start = s;
		if (*s == '\\') {
			// skip the slash
			s++;
		}

		// Copy a key
		nCount = 0;
		c = pkey;
		while (*s != '\\')
		{
			if (!*s) {
				// key should end with a \, not a nullptr, but allow to remove it
				break;
			}

			if (nCount >= MAX_INFO_KEY - 1)
			{
				s++;
				// skip oversized key chars till the slash or EOL
				continue;
			}

			*c++ = *s++;
			nCount++;
		}

		*c = '\0';
		if (*s) {
			// skip the slash
			s++;
		}

		// Copy a value
		nCount = 0;
		c = value;
		while (*s != '\\')
		{
			if (!*s) {
				// allow value to be ended with nullptr
				break;
			}

			if (nCount >= MAX_INFO_VALUE - 1)
			{
				s++;
				// skip oversized value chars till the slash or EOL
				continue;
			}

			*c++ = *s++;
			nCount++;
		}

		*c = '\0';

		// Compare keys
		if (!Q_strncmp(key, pkey, cmpsize) )
		{
			found = true;
			Q_strcpy_s(start, s);	// remove this part
			s = start;				// continue searching
		}
	}

	return found;
}

void InfoString::RemovePrefixedKeys(char prefix)
{
	char pkey[MAX_INFO_KEY];
	char value[MAX_INFO_VALUE];
	char *start;
	char *c;
	char *s;
	int nCount;

	s = m_String;
	while (*s)
	{
		start = s;

		if (*s == '\\') {
			// skip the slash
			s++;
		}

		// Copy a key
		nCount = 0;
		c = pkey;
		while (*s != '\\')
		{
			if (!*s) {
				// key should end with a \, not a nullptr, but allow to remove it
				break;
			}

			if (nCount >= MAX_INFO_KEY - 1)
			{
				s++;
				// skip oversized key chars till the slash or EOL
				continue;
			}

			*c++ = *s++;
			nCount++;
		}

		*c = '\0';
		if (*s) {
			// skip the slash
			s++;
		}

		// Copy a value
		nCount = 0;
		c = value;
		while (*s != '\\')
		{
			if (!*s) {
				// allow value to be ended with nullptr
				break;
			}

			if (nCount >= MAX_INFO_VALUE - 1) {
				s++;
				// skip oversized value chars till the slash or EOL
				continue;
			}

			*c++ = *s++;
			nCount++;
		}

		*c = '\0';

		// Compare prefix
		if (pkey[0] == prefix)
		{
			Q_strcpy_s(start, s);	// remove this part
			s = start;				// continue searching
		}
	}
}

bool InfoString::SetValueForStarKey(const char *key, const char *value)
{
	char newtoken[MAX_INFO_LEN + 4];
	if (Q_strstr(key, "\\") || Q_strstr(value, "\\")) {
		return false;
	}

	if (Q_strstr(key, "\"") || Q_strstr(value, "\"")) {
		return false;
	}

	if (Q_strlen(key) > MAX_INFO_KEY || Q_strlen(value) > MAX_INFO_VALUE) {
		return false;
	}

	// Remove current key/value and return if we doesn't specified to set a value
	RemoveKey(key);

	if (!Q_strlen(value)) {
		return true;
	}

	// Create key/value pair
	Q_snprintf(newtoken, sizeof(newtoken), "\\%s\\%s", key, value);

	int length = Q_strlen(m_String);
	if ((length + Q_strlen(newtoken)) < m_MaxSize)
	{
		char *v = newtoken;
		char *s = m_String + length;
		unsigned char c;

		while (*v)
		{
			c = (unsigned char)*v++;
			*s++ = c;
		}

		*s = '\0';
		return true;
	}

	return false;
}

bool InfoString::SetValueForKey(const char *key, const char *value)
{
	return SetValueForStarKey(key, value);
}
