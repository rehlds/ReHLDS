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

// 0 - off, 1 - allowed, 2 - required: a client without the extension is rejected
cvar_t sv_rehlds_ext_usermessage = { "sv_rehlds_ext_usermessage", "1", 0, 1.0f, NULL };
cvar_t sv_rehlds_ext_replicatedcvars = { "sv_rehlds_ext_replicatedcvars", "1", 0, 1.0f, NULL };

enum
{
	PROTOCOL_EXT_SETTING_OFF      = 0,
	PROTOCOL_EXT_SETTING_ALLOWED  = 1,
	PROTOCOL_EXT_SETTING_REQUIRED = 2,
};

struct protocol_ext_desc_t
{
	ProtocolExtension extension;
	const char *name;
	cvar_t *setting; // sv_rehlds_ext_<name>; NULL when the extension has no switch and is always allowed
	int dependency;  // extension that has to be agreed as well, -1 for none
};

// Extensions this engine implements, every dependency before its dependents
static const protocol_ext_desc_t g_ProtocolExtensions[] =
{
	{ PROTOCOL_EXT_NETMESSAGE,      "netmessage",      NULL,                           -1                      },
	{ PROTOCOL_EXT_USERMESSAGE,     "usermessage",     &sv_rehlds_ext_usermessage,     -1                      },
	{ PROTOCOL_EXT_REPLICATEDCVARS, "replicatedcvars", &sv_rehlds_ext_replicatedcvars, PROTOCOL_EXT_NETMESSAGE },
};

static_assert(PROTOCOL_EXT_MAX_BITS % 4 == 0, "a hex digit carries four extension numbers");
static_assert(sizeof("\\pext\\") + PROTOCOL_EXT_MAX_BITS / 4 <= PROTOCOL_EXT_REPLY_SIZE - PROTOCOL_EXT_MODULE_KEYS_SIZE, "no room for pext in the reply");

static int GetSetting(const protocol_ext_desc_t &desc)
{
	return desc.setting ? (int)desc.setting->value : PROTOCOL_EXT_SETTING_ALLOWED;
}

static bool IsValidExtension(ProtocolExtension extension)
{
	return extension >= 0 && extension < PROTOCOL_EXT_COUNT;
}

// "<prefix>_<name>": the prefix keeps keys of different modules apart and
// can't match "pext" or the keys of extension parameters
static bool IsModuleKey(const char *key)
{
	const char *c = key;
	while (isalnum((unsigned char)*c))
		c++;

	return c != key && c[0] == '_' && c[1] != '\0';
}

CProtocolNegotiation::CProtocolNegotiation()
{
	m_bHasRequest = false;
	m_ModuleKeys[0] = '\0';
}

bool CProtocolNegotiation::ParseRequest(const char *protinfo)
{
	const char *hex = Info_ValueForKey(protinfo, "pext");
	if (!hex[0])
		return true;

	m_bHasRequest = true;
	return ParseBits(hex, &m_Requested);
}

bool EXT_FUNC CProtocolNegotiation::IsRequested(ProtocolExtension extension) const
{
	return IsValidExtension(extension) && m_Requested.IsBitSet(extension);
}

bool EXT_FUNC CProtocolNegotiation::IsAllowed(ProtocolExtension extension) const
{
	return IsValidExtension(extension) && m_Allowed.IsBitSet(extension);
}

void EXT_FUNC CProtocolNegotiation::SetAllowed(ProtocolExtension extension, bool allowed)
{
	if (IsValidExtension(extension))
		m_Allowed.Set(extension, allowed);
}

bool EXT_FUNC CProtocolNegotiation::AddReplyValue(const char *key, const char *value)
{
	if (!key || !IsModuleKey(key))
	{
		Con_Printf("%s: \"%s\" is not a module key, expected <prefix>_<name>\n", __func__, key ? key : "");
		return false;
	}

	if (!value || !value[0])
	{
		Con_Printf("%s: empty value for \"%s\"\n", __func__, key);
		return false;
	}

	if (Info_ValueForKey(m_ModuleKeys, key)[0])
	{
		Con_Printf("%s: \"%s\" is already in the reply, added by another module\n", __func__, key);
		return false;
	}

	return Info_SetValueForStarKey(m_ModuleKeys, key, value, sizeof(m_ModuleKeys)) != FALSE;
}

void CProtocolNegotiation::Agree()
{
	m_Agreed.ClearAll();

	for (const protocol_ext_desc_t &desc : g_ProtocolExtensions)
	{
		if (!m_Requested.IsBitSet(desc.extension) || !m_Allowed.IsBitSet(desc.extension))
			continue;

		if (desc.dependency != -1 && !m_Agreed.IsBitSet(desc.dependency))
			continue;

		m_Agreed.Set(desc.extension);
	}
}

const char *CProtocolNegotiation::FindMissingRequired() const
{
	for (const protocol_ext_desc_t &desc : g_ProtocolExtensions)
	{
		if (GetSetting(desc) >= PROTOCOL_EXT_SETTING_REQUIRED && !m_Agreed.IsBitSet(desc.extension))
			return desc.name;
	}

	return NULL;
}

void CProtocolNegotiation::BuildReply(char *reply, int size) const
{
	char hex[PROTOCOL_EXT_MAX_BITS / 4 + 1];
	FormatBits(m_Agreed, hex, sizeof(hex));

	Q_snprintf(reply, size, "\\pext\\%s%s", hex, m_ModuleKeys);
}

// Hex digit k, counting from the left, carries extensions 4k..4k+3; 4k is its lowest bit.
// Numbers past PROTOCOL_EXT_MAX_BITS are ignored: they belong to newer protocol revisions.
bool CProtocolNegotiation::ParseBits(const char *hex, CProtocolExtensionBits *bits)
{
	bits->ClearAll();

	for (int digit = 0; hex[digit]; digit++)
	{
		int value;
		char c = hex[digit];

		if (c >= '0' && c <= '9')
			value = c - '0';
		else if (c >= 'a' && c <= 'f')
			value = c - 'a' + 10;
		else if (c >= 'A' && c <= 'F')
			value = c - 'A' + 10;
		else
			return false;

		for (int bit = 0; bit < 4; bit++)
		{
			int extension = digit * 4 + bit;
			if ((value & (1 << bit)) && extension < PROTOCOL_EXT_MAX_BITS)
				bits->Set(extension);
		}
	}

	return true;
}

// Trailing zero digits are dropped; an empty set is "0" because an info value can't be empty
void CProtocolNegotiation::FormatBits(const CProtocolExtensionBits &bits, char *hex, int size)
{
	int digits = 1;
	for (int extension = 0; extension < PROTOCOL_EXT_MAX_BITS; extension++)
	{
		if (bits.IsBitSet(extension))
			digits = extension / 4 + 1;
	}

	if (digits > size - 1)
		digits = size - 1;

	for (int digit = 0; digit < digits; digit++)
	{
		int value = 0;
		for (int bit = 0; bit < 4; bit++)
		{
			if (bits.IsBitSet(digit * 4 + bit))
				value |= 1 << bit;
		}

		hex[digit] = "0123456789abcdef"[value];
	}

	hex[digits] = '\0';
}

void EXT_FUNC SV_ProtocolExtensions_internal(netadr_t *adr, IProtocolNegotiation *negotiation)
{
	for (const protocol_ext_desc_t &desc : g_ProtocolExtensions)
		negotiation->SetAllowed(desc.extension, GetSetting(desc) >= PROTOCOL_EXT_SETTING_ALLOWED);
}

void Rehlds_ProtocolExt_Init()
{
#ifdef REHLDS_FIXES
	Cvar_RegisterVariable(&sv_rehlds_ext_usermessage);
	Cvar_RegisterVariable(&sv_rehlds_ext_replicatedcvars);
#endif
}
