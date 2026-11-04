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

#include "bitvec.h"
#include "protocol_extensions.h"

const int PROTOCOL_EXT_MAX_BITS = 64;
static_assert(PROTOCOL_EXT_COUNT <= PROTOCOL_EXT_MAX_BITS, "protocol extension numbers exceed the bit set");

typedef CBitVec<PROTOCOL_EXT_MAX_BITS> CProtocolExtensionBits;

// Size of the info string appended to S2C_CONNECTION for a client that sent "pext"
const int PROTOCOL_EXT_REPLY_SIZE = 512;

// Module keys of the reply; the rest holds "\pext\<agreed set>" and the agreed parameters
const int PROTOCOL_EXT_MODULE_KEYS_SIZE = PROTOCOL_EXT_REPLY_SIZE - 64;

// The most entities in a frame of a client that agreed on PROTOCOL_EXT_PACKETENTITIES;
// the client puts every entity of a frame into the physent lists of its playermove_t
const int MAX_EXT_PACKET_ENTITIES = 512;

extern cvar_t sv_rehlds_ext_usermessage;
extern cvar_t sv_rehlds_ext_replicatedcvars;
extern cvar_t sv_rehlds_ext_rate;
extern cvar_t sv_rehlds_ext_packetsize;
extern cvar_t sv_rehlds_ext_packetentities;
extern cvar_t sv_rehlds_maxpacketsize;
extern cvar_t sv_rehlds_maxpacketentities;
extern cvar_t sv_rehlds_maxrate;
extern cvar_t sv_rehlds_minrate;

// Extensions a connection agreed on, with the values of their parameters
class CProtocolAgreement
{
public:
	CProtocolAgreement() { Clear(); }

	bool Has(ProtocolExtension extension) const;

	// The agreed value of the extension parameter, 0 when the extension has none or is not agreed
	int GetParam(ProtocolExtension extension) const;

	void Set(ProtocolExtension extension, int param = 0);
	void Clear();

	const CProtocolExtensionBits &GetBits() const { return m_Bits; }

	// PROTOCOL_EXT_PACKETSIZE: UDP payload of one part, 0 when the extension is not agreed
	int GetMaxRoutable() const { return m_MaxRoutable; }
	void SetMaxRoutable(int maxRoutable) { m_MaxRoutable = maxRoutable; }

private:
	CProtocolExtensionBits m_Bits;
	int m_Params[PROTOCOL_EXT_COUNT];
	int m_MaxRoutable;
};

// Agreement on protocol extensions for one connect request
class CProtocolNegotiation: public IProtocolNegotiation
{
public:
	CProtocolNegotiation();

	// Reads "pext" and the extension parameters from the protinfo of the connect request.
	// Returns false when "pext" is not a hex bit set.
	bool ParseRequest(const char *protinfo);

	// The client sent "pext", so the reply carries the agreed set even when it is empty
	bool HasRequest() const { return m_bHasRequest; }

	virtual bool IsRequested(ProtocolExtension extension) const;
	virtual bool IsAllowed(ProtocolExtension extension) const;
	virtual void SetAllowed(ProtocolExtension extension, bool allowed);
	virtual bool AddReplyValue(const char *key, const char *value);

	// Requested and allowed extensions the engine implements, without those whose dependency was not agreed
	// and those whose parameter does not go past the default limit; the part size of packetsize is the
	// client's "proute" within sv_rehlds_maxroutable, the server bound alone without it
	void Agree();

	// Name of an extension the server requires (sv_rehlds_ext_<name> 2) that was not agreed, or NULL
	const char *FindMissingRequired() const;

	const CProtocolAgreement &GetAgreed() const { return m_Agreed; }

	// "\pext\<agreed set>", the agreed parameters, then the module keys
	void BuildReply(char *reply, int size) const;

	static bool ParseBits(const char *hex, CProtocolExtensionBits *bits);
	static void FormatBits(const CProtocolExtensionBits &bits, char *hex, int size);

private:
	CProtocolExtensionBits m_Requested;
	int m_RequestedParams[PROTOCOL_EXT_COUNT];
	int m_RequestedMaxRoutable;
	CProtocolExtensionBits m_Allowed;
	CProtocolAgreement m_Agreed;
	bool m_bHasRequest;
	char m_ModuleKeys[PROTOCOL_EXT_MODULE_KEYS_SIZE];
};

// Original function of the SV_ProtocolExtensions hook chain: allows what the sv_rehlds_ext_* cvars allow
void SV_ProtocolExtensions_internal(netadr_t *adr, IProtocolNegotiation *negotiation);

extern void Rehlds_ProtocolExt_Init();
extern void Rehlds_ProtocolExt_Shutdown();
