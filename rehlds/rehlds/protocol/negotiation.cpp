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
cvar_t sv_rehlds_ext_packetsize = { "sv_rehlds_ext_packetsize", "1", 0, 1.0f, NULL };
cvar_t sv_rehlds_ext_packetentities = { "sv_rehlds_ext_packetentities", "1", 0, 1.0f, NULL };

// Server limits of the extension parameters; the engine clamps them to what it handles
cvar_t sv_rehlds_maxpacketsize = { "sv_rehlds_maxpacketsize", "131072", 0, 131072.0f, NULL };
cvar_t sv_rehlds_maxpacketentities = { "sv_rehlds_maxpacketentities", "512", 0, 512.0f, NULL };

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
	cvar_t *setting;      // sv_rehlds_ext_<name>; NULL when the extension has no switch and is always allowed
	int dependency;       // extension that has to be agreed as well, -1 for none
	const char *paramKey; // key of the parameter in protinfo and in the reply, NULL when there is none
	cvar_t *paramLimit;   // server limit of the parameter
	int defaultLimit;     // the limit without the extension; an agreed value has to go past it
	int maxValue;         // the most the engine handles
};

// Extensions this engine implements, every dependency before its dependents
static const protocol_ext_desc_t g_ProtocolExtensions[] =
{
	{ PROTOCOL_EXT_NETMESSAGE,      "netmessage",      NULL,                           -1,                      NULL,    NULL,                         0,                   0                       },
	{ PROTOCOL_EXT_USERMESSAGE,     "usermessage",     &sv_rehlds_ext_usermessage,     -1,                      NULL,    NULL,                         0,                   0                       },
	{ PROTOCOL_EXT_REPLICATEDCVARS, "replicatedcvars", &sv_rehlds_ext_replicatedcvars, PROTOCOL_EXT_NETMESSAGE, NULL,    NULL,                         0,                   0                       },
	{ PROTOCOL_EXT_PACKETSIZE,      "packetsize",      &sv_rehlds_ext_packetsize,      -1,                      "psize", &sv_rehlds_maxpacketsize,     NET_MAX_PACKET,      NET_MAX_EXT_PACKET      },
	{ PROTOCOL_EXT_PACKETENTITIES,  "packetentities",  &sv_rehlds_ext_packetentities,  PROTOCOL_EXT_PACKETSIZE, "pents", &sv_rehlds_maxpacketentities, MAX_PACKET_ENTITIES, MAX_EXT_PACKET_ENTITIES },
};

static_assert(PROTOCOL_EXT_MAX_BITS % 4 == 0, "a hex digit carries four extension numbers");
static_assert(sizeof("\\pext\\") + PROTOCOL_EXT_MAX_BITS / 4 <= PROTOCOL_EXT_REPLY_SIZE - PROTOCOL_EXT_MODULE_KEYS_SIZE, "no room for pext in the reply");
static_assert(1 + MAX_EXT_PACKET_ENTITIES + MAX_CLIENTS <= MAX_PHYSENTS, "the entities of a frame overflow the physent lists of the client");

static int GetSetting(const protocol_ext_desc_t &desc)
{
	return desc.setting ? (int)desc.setting->value : PROTOCOL_EXT_SETTING_ALLOWED;
}

static bool IsValidExtension(ProtocolExtension extension)
{
	return extension >= 0 && extension < PROTOCOL_EXT_COUNT;
}

// The smaller of the client and the server limits, 0 when it does not go past the default limit
static int AgreeParam(const protocol_ext_desc_t &desc, int requested)
{
	int value = Q_min(requested, (int)desc.paramLimit->value);
	value = Q_min(value, desc.maxValue);

	return (value > desc.defaultLimit) ? value : 0;
}

bool CProtocolAgreement::Has(ProtocolExtension extension) const
{
	return IsValidExtension(extension) && m_Bits.IsBitSet(extension);
}

int CProtocolAgreement::GetParam(ProtocolExtension extension) const
{
	return Has(extension) ? m_Params[extension] : 0;
}

void CProtocolAgreement::Set(ProtocolExtension extension, int param)
{
	if (!IsValidExtension(extension))
		return;

	m_Bits.Set(extension);
	m_Params[extension] = param;
}

void CProtocolAgreement::Clear()
{
	m_Bits.ClearAll();
	Q_memset(m_Params, 0, sizeof(m_Params));
	m_MaxRoutable = 0;
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
	Q_memset(m_RequestedParams, 0, sizeof(m_RequestedParams));
	m_RequestedMaxRoutable = 0;
	m_bHasRequest = false;
	m_ModuleKeys[0] = '\0';
}

bool CProtocolNegotiation::ParseRequest(const char *protinfo)
{
	const char *hex = Info_ValueForKey(protinfo, "pext");
	if (!hex[0])
		return true;

	m_bHasRequest = true;

	for (const protocol_ext_desc_t &desc : g_ProtocolExtensions)
	{
		if (desc.paramKey)
			m_RequestedParams[desc.extension] = Q_atoi(Info_ValueForKey(protinfo, desc.paramKey));
	}

	// Part size the client takes with packetsize, net_maxroutable of Source; optional
	m_RequestedMaxRoutable = Q_atoi(Info_ValueForKey(protinfo, "proute"));

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
	m_Agreed.Clear();

	for (const protocol_ext_desc_t &desc : g_ProtocolExtensions)
	{
		if (!m_Requested.IsBitSet(desc.extension) || !m_Allowed.IsBitSet(desc.extension))
			continue;

		if (desc.dependency != -1 && !m_Agreed.Has((ProtocolExtension)desc.dependency))
			continue;

		int param = 0;
		if (desc.paramKey)
		{
			param = AgreeParam(desc, m_RequestedParams[desc.extension]);
			if (!param)
				continue;
		}

		m_Agreed.Set(desc.extension, param);
	}

	if (m_Agreed.Has(PROTOCOL_EXT_PACKETSIZE))
	{
		int maxRoutable = (int)sv_rehlds_maxroutable.value;
		if (m_RequestedMaxRoutable > 0)
			maxRoutable = Q_min(m_RequestedMaxRoutable, maxRoutable);

		m_Agreed.SetMaxRoutable(clamp(maxRoutable, MIN_USER_MAXROUTABLE_SIZE, MAX_USER_MAXROUTABLE_SIZE));
	}
}

const char *CProtocolNegotiation::FindMissingRequired() const
{
	for (const protocol_ext_desc_t &desc : g_ProtocolExtensions)
	{
		if (GetSetting(desc) >= PROTOCOL_EXT_SETTING_REQUIRED && !m_Agreed.Has(desc.extension))
			return desc.name;
	}

	return NULL;
}

void CProtocolNegotiation::BuildReply(char *reply, int size) const
{
	char hex[PROTOCOL_EXT_MAX_BITS / 4 + 1];
	FormatBits(m_Agreed.GetBits(), hex, sizeof(hex));

	char params[PROTOCOL_EXT_REPLY_SIZE - PROTOCOL_EXT_MODULE_KEYS_SIZE] = "";
	for (const protocol_ext_desc_t &desc : g_ProtocolExtensions)
	{
		if (!desc.paramKey || !m_Agreed.Has(desc.extension))
			continue;

		char param[32];
		Q_snprintf(param, sizeof(param), "\\%s\\%d", desc.paramKey, m_Agreed.GetParam(desc.extension));
		Q_strlcat(params, param);
	}

	Q_snprintf(reply, size, "\\pext\\%s%s%s", hex, params, m_ModuleKeys);
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
	Cvar_RegisterVariable(&sv_rehlds_ext_packetsize);
	Cvar_RegisterVariable(&sv_rehlds_ext_packetentities);
	Cvar_RegisterVariable(&sv_rehlds_maxpacketsize);
	Cvar_RegisterVariable(&sv_rehlds_maxpacketentities);
	Cvar_RegisterVariable(&sv_rehlds_splitrate);
	Cvar_RegisterVariable(&sv_rehlds_maxroutable);
#endif
}

// The thread of queued parts stops before the engine library goes away
void Rehlds_ProtocolExt_Shutdown()
{
	g_QueuedPacketSender.Shutdown();
}
