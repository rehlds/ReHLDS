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

#include "archtypes.h"

class IGameClient;

// Protocol 48 extensions a client and the server agree on per connection.
// Numbers are wire values: they are assigned once and never reused.
enum ProtocolExtension
{
	PROTOCOL_EXT_NETMESSAGE       = 0, // svc_netmessage / clc_netmessage families
	PROTOCOL_EXT_USERMESSAGE      = 1, // extended user messages in both directions
	PROTOCOL_EXT_REPLICATEDCVARS  = 2, // FCVAR_REPLICATED cvar values sent to the client
	PROTOCOL_EXT_RATE             = 3, // reserved
	PROTOCOL_EXT_PACKETSIZE       = 4, // reserved
	PROTOCOL_EXT_USERINFO         = 5, // reserved
	PROTOCOL_EXT_PACKETENTITIES   = 6, // reserved

	PROTOCOL_EXT_COUNT
};

// Decides which extensions one connect request gets. Lives only for the duration of the
// SV_ProtocolExtensions hook chain, before the client has a slot.
class IProtocolNegotiation
{
public:
	virtual ~IProtocolNegotiation() { }

	// Extensions the client declared in the connect request
	virtual bool IsRequested(ProtocolExtension extension) const = 0;

	// Extensions the server agrees to if requested. Starts from the sv_rehlds_ext_* cvars.
	// An extension the engine does not implement is never agreed, whatever is allowed.
	virtual bool IsAllowed(ProtocolExtension extension) const = 0;
	virtual void SetAllowed(ProtocolExtension extension, bool allowed) = 0;

	// Adds a key to the info string of the connection reply. The key starts with the module
	// prefix and an underscore ("gcl_v"); a key another module already added is refused.
	virtual bool AddReplyValue(const char *key, const char *value) = 0;
};

// RegUserMsgEx flag: the message is sent as svc_usermessage and only to clients that agreed
// on PROTOCOL_EXT_USERMESSAGE, without the 192 byte limit of a user message
#define USERMSG_EXTENDED (1<<0)

// Receives one clc_usermessage. pData points into the incoming packet and is valid only during the call.
typedef void (*ClientUserMsgHandler_t)(IGameClient *pClient, const uint8 *pData, int nSize);
