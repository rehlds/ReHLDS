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

#include "maintypes.h"
#include "common.h"
#include "enums.h"
#include "netadr.h"

const int PROTOCOL_VERSION		= 48;

// MAX_CHALLENGES is made large to prevent a denial
//  of service attack that could cycle all of them
//  out before legitimate users connected
#ifdef REHLDS_OPT_PEDANTIC
const int MAX_CHALLENGES		= 64;
#else
const int MAX_CHALLENGES		= 1024;
#endif // REHLDS_OPT_PEDANTIC

// Client connection is initiated by requesting a challenge value
//  the server sends this value back
const char S2C_CHALLENGE		= 'A';	// + challenge value

// Send a userid, client remote address, is this server secure and engine build number
const char S2C_CONNECTION       = 'B';

// HLMaster rejected a server's connection because the server needs to be updated
const char M2S_REQUESTRESTART	= 'O';

// Response details about each player on the server
const char S2A_PLAYERS			= 'D';

// Number of rules + string key and string value pairs
const char S2A_RULES			= 'E';

// info request
const char S2A_INFO				= 'C'; // deprecated goldsrc response

const char S2A_INFO_DETAILED	= 'm'; // New Query protocol, returns dedicated or not, + other performance info.

// send a log event as key value
const char S2A_LOGSTRING		= 'R';

// Send a log string
const char S2A_LOGKEY			= 'S';

// Basic information about the server
const char A2S_INFO				= 'T';

// Details about each player on the server
const char A2S_PLAYER			= 'U';

// The rules the server is using
const char A2S_RULES			= 'V';

// Another user is requesting a challenge value from this machine
const char A2A_GETCHALLENGE		= 'W';	// Request challenge # from another machine

// Generic Ping Request
const char A2A_PING				= 'i';	// respond with an A2A_ACK

// Generic Ack
const char A2A_ACK				= 'j';	// general acknowledgement without info

// Print to client console
const char A2A_PRINT			= 'l'; // print a message on client

// Challenge response from master
const char M2A_CHALLENGE		= 's';	// + challenge value

// 0 == regular, 1 == file stream
enum
{
	FRAG_NORMAL_STREAM = 0,
	FRAG_FILE_STREAM,

	MAX_STREAMS
};

// Flow control bytes per second limits
const float MAX_RATE		= 100000.0f;
const float MIN_RATE		= 1000.0f;

// Default data rate
const float DEFAULT_RATE	= 30000.0f;

// NETWORKING INFO

// Max size of udp packet payload
const int MAX_UDP_PACKET	= 4010; // 9 bytes SPLITHEADER + 4000 payload?

// Max length of a reliable message
const int MAX_MSGLEN		= 3990; // 10 reserved for fragheader?

// Max length of unreliable message
const int MAX_DATAGRAM		= 4000;

// This is the packet payload without any header bytes (which are attached for actual sending)
const int NET_MAX_PAYLOAD	= 65536;

// This is the payload plus any header info (excluding UDP header)

// Packet header is:
//  4 bytes of outgoing seq
//  4 bytes of incoming seq
//  and for each stream
// {
//  byte (on/off)
//  int (fragment id)
//  short (startpos)
//  short (length)
// }
#define HEADER_BYTES (8 + MAX_STREAMS * 9)

// Pad this to next higher 16 byte boundary
// This is the largest packet that can come in/out over the wire, before processing the header
//  bytes will be stripped by the networking channel layer
//#define NET_MAX_MESSAGE PAD_NUMBER( ( MAX_MSGLEN + HEADER_BYTES ), 16 )
// This is currently used value in the engine. TODO: define above gives 4016, check it why.
const int NET_MAX_MESSAGE = 4037;


typedef enum svc_commands_e
{
	svc_bad,
	svc_nop,
	svc_disconnect,
	svc_event,
	svc_version,
	svc_setview,
	svc_sound,
	svc_time,
	svc_print,
	svc_stufftext,
	svc_setangle,
	svc_serverinfo,
	svc_lightstyle,
	svc_updateuserinfo,
	svc_deltadescription,
	svc_clientdata,
	svc_stopsound,
	svc_pings,
	svc_particle,
	svc_damage,
	svc_spawnstatic,
	svc_event_reliable,
	svc_spawnbaseline,
	svc_temp_entity,
	svc_setpause,
	svc_signonnum,
	svc_centerprint,
	svc_killedmonster,
	svc_foundsecret,
	svc_spawnstaticsound,
	svc_intermission,
	svc_finale,
	svc_cdtrack,
	svc_restore,
	svc_cutscene,
	svc_weaponanim,
	svc_decalname,
	svc_roomtype,
	svc_addangle,
	svc_newusermsg,
	svc_packetentities,
	svc_deltapacketentities,
	svc_choke,
	svc_resourcelist,
	svc_newmovevars,
	svc_resourcerequest,
	svc_customization,
	svc_crosshairangle,
	svc_soundfade,
	svc_filetxferfailed,
	svc_hltv,
	svc_director,
	svc_voiceinit,
	svc_voicedata,
	svc_sendextrainfo,
	svc_timescale,
	svc_resourcelocation,
	svc_sendcvarvalue,
	svc_sendcvarvalue2,
	svc_exec,
	svc_reserve60,
	svc_reserve61,
	svc_reserve62,
	svc_reserve63,
// Let's just use an id of the first user message instead of the last svc_*
// This change makes code in `PF_MessageEnd_I` forward-compatible with future svc_* additions
#ifdef REHLDS_FIXES
	svc_startofusermessages = svc_reserve63 + 1,
#else // REHLDS_FIXES
	svc_startofusermessages = svc_exec,
#endif // REHLDS_FIXES
	svc_endoflist = 255,
} svc_commands_t;

#ifdef REHLDS_FIXES
static_assert(svc_startofusermessages == 64, "svc_startofusermessages should be equal to 64 for backward and forward compatibility");
#endif // REHLDS_FIXES

typedef enum clc_commands_e
{
	clc_bad,
	clc_nop,
	clc_move,
	clc_stringcmd,
	clc_delta,
	clc_resourcelist,
	clc_tmove,
	clc_fileconsistency,
	clc_voicedata,
	clc_hltv,
	clc_cvarvalue,
	clc_cvarvalue2,
	clc_endoflist = 255,
} clc_commands_t;

enum
{
	FLOW_OUTGOING = 0,
	FLOW_INCOMING,

	MAX_FLOWS
};

// Message data
typedef struct flowstats_s
{
	// Size of message sent/received
	int size;
	// Time that message was sent/received
	double time;
} flowstats_t;

const int MAX_LATENT = 32;

typedef struct flow_s
{
	// Data for last MAX_LATENT messages
	flowstats_t stats[MAX_LATENT];
	// Current message position
	int current;
	// Time when we should recompute k/sec data
	double nextcompute;
	// Average data
	float kbytespersec;
	float avgkbytespersec;
} flow_t;

const int FRAGMENT_C2S_MIN_SIZE = 16;
const int FRAGMENT_S2C_MIN_SIZE = 256;
const int FRAGMENT_S2C_MAX_SIZE = 1024;

const int CLIENT_FRAGMENT_SIZE_ONCONNECT = 128;
const int CUSTOMIZATION_MAX_SIZE = 20480;

#ifndef REHLDS_FIXES
// Size of fragmentation buffer internal buffers
const int FRAGMENT_MAX_SIZE = 1400;

const int MAX_FRAGMENTS = 25000;
#else
const int FRAGMENT_MAX_SIZE = 1024;

// Client sends normal fragments only while connecting
#define MAX_NORMAL_FRAGMENTS (NET_MAX_PAYLOAD / CLIENT_FRAGMENT_SIZE_ONCONNECT)

// While client is connecting it sending fragments with minimal size, also it transfers sprays with minimal fragments...
// But with sv_delayed_spray_upload it sends with cl_dlmax fragment size
#define MAX_FILE_FRAGMENTS (CUSTOMIZATION_MAX_SIZE / FRAGMENT_C2S_MIN_SIZE)
#endif

const int UDP_HEADER_SIZE = 28;
const int MAX_RELIABLE_PAYLOAD = 1200;

#define MAKE_FRAGID(id,count)	((( id & 0xffff) << 16) | (count & 0xffff))
#define FRAG_GETID(fragid)		((fragid >> 16) & 0xffff)
#define FRAG_GETCOUNT(fragid)	(fragid & 0xffff)

// Generic fragment structure
typedef struct fragbuf_s
{
	// Next buffer in chain
	fragbuf_s *next;
	// Id of this buffer
	int bufferid;
	// Message buffer where raw data is stored
	sizebuf_t frag_message;
	// The actual data sits here
	byte frag_message_buf[FRAGMENT_MAX_SIZE];
	// Is this a file buffer?
	qboolean isfile;
	// Is this file buffer from memory ( custom decal, etc. ).
	qboolean isbuffer;
	qboolean iscompressed;
	// Name of the file to save out on remote host
	char filename[MAX_PATH];
	// Offset in file from which to read data
	int foffset;
	// Size of data to read at that offset
	int size;
} fragbuf_t;

// Waiting list of fragbuf chains
typedef struct fragbufwaiting_s
{
	// Next chain in waiting list
	fragbufwaiting_s *next;
	// Number of buffers in this chain
	int fragbufcount;
	// The actual buffers
	fragbuf_t *fragbufs;
} fragbufwaiting_t;

// Network Connection Channel
typedef struct netchan_s
{
	// NS_SERVER or NS_CLIENT, depending on channel.
	netsrc_t sock;

	// Address this channel is talking to.
	netadr_t remote_address;

	int player_slot;
	// For timeouts.  Time last message was received.
	float last_received;
	// Time when channel was connected.
	float connect_time;

	// Bandwidth choke
	// Bytes per second
	double rate;
	// If realtime > cleartime, free to send next packet
	double cleartime;

	// Sequencing variables
	//
	// Increasing count of sequence numbers
	int incoming_sequence;
	// # of last outgoing message that has been ack'd.
	int incoming_acknowledged;
	// Toggles T/F as reliable messages are received.
	int incoming_reliable_acknowledged;
	// single bit, maintained local
	int incoming_reliable_sequence;
	// Message we are sending to remote
	int outgoing_sequence;
	// Whether the message contains reliable payload, single bit
	int reliable_sequence;
	// Outgoing sequence number of last send that had reliable data
	int last_reliable_sequence;

	void *connection_status;
	int (*pfnNetchan_Blocksize)(void *);

	// Staging and holding areas
	sizebuf_t message;
	byte message_buf[MAX_MSGLEN];

	// Reliable message buffer. We keep adding to it until reliable is acknowledged. Then we clear it.
	int reliable_length;
	byte reliable_buf[MAX_MSGLEN];

	// Waiting list of buffered fragments to go onto queue. Multiple outgoing buffers can be queued in succession.
	fragbufwaiting_t *waitlist[MAX_STREAMS];

	// Is reliable waiting buf a fragment?
	int reliable_fragment[MAX_STREAMS];
	// Buffer id for each waiting fragment
	unsigned int reliable_fragid[MAX_STREAMS];

	// The current fragment being set
	fragbuf_t *fragbufs[MAX_STREAMS];
	// The total number of fragments in this stream
	int fragbufcount[MAX_STREAMS];

	// Position in outgoing buffer where frag data starts
	short int frag_startpos[MAX_STREAMS];
	// Length of frag data in the buffer
	short int frag_length[MAX_STREAMS];

	// Incoming fragments are stored here
	fragbuf_t *incomingbufs[MAX_STREAMS];
	// Set to true when incoming data is ready
	qboolean incomingready[MAX_STREAMS];

	// Only referenced by the FRAG_FILE_STREAM component
	// Name of file being downloaded
	char incomingfilename[MAX_PATH];

	void *tempbuffer;
	int tempbuffersize;

	// Incoming and outgoing flow metrics
	flow_t flow[MAX_FLOWS];

} netchan_t;

#ifdef REHLDS_FIXES
#define Con_NetPrintf Con_DPrintf
#else // REHLDS_FIXES
#define Con_NetPrintf Con_Printf
#endif // REHLDS_FIXES
