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

qboolean net_thread_initialized;

loopback_t loopbacks[2];
packetlag_t g_pLagData[NS_MAX];	// List of lag structures, if fakelag is set.
float gFakeLag;
int net_configured;
#ifdef _WIN32
netadr_t net_local_ipx_adr;
#endif
netadr_t net_local_adr;
netadr_t net_from;
sizebuf_t net_message;
qboolean noip;
qboolean noipx;
qboolean use_thread;

unsigned char net_message_buffer[NET_MAX_PAYLOAD];
unsigned char in_message_buf[NET_MAX_PAYLOAD];
sizebuf_t in_message;
netadr_t in_from;

#ifdef REHLDS_FIXES
// Define default to INVALID_SOCKET
#define INV_SOCK INVALID_SOCKET
#else
// Original engine behavior
#define INV_SOCK 0
#endif

SOCKET ip_sockets[NS_MAX] = { INV_SOCK, INV_SOCK, INV_SOCK };

#ifdef _WIN32
SOCKET ipx_sockets[NS_MAX] = { INV_SOCK, INV_SOCK, INV_SOCK };
#endif

LONGPACKET gNetSplit;
net_messages_t *messages[NS_MAX];
net_messages_t *normalqueue;

#ifdef _WIN32
HANDLE hNetThread;
DWORD dwNetThreadId;
CRITICAL_SECTION net_cs;
#endif

cvar_t net_address = { "net_address", "", 0, 0.0f, NULL };
cvar_t ipname = { "ip", "localhost", 0, 0.0f, NULL };
cvar_t defport = { "port", "27015", 0, 0.0f, NULL };
cvar_t ip_clientport = { "ip_clientport", "0", 0, 0.0f, NULL };
cvar_t clientport = { "clientport", "27005", 0, 0.0f, NULL };
int net_sleepforever = 1;

cvar_t clockwindow = { "clockwindow", "0.5", 0, 0.0f, NULL };

cvar_t iphostport = { "ip_hostport", "0", 0, 0.0f, NULL };
cvar_t hostport = { "hostport", "0", 0, 0.0f, NULL };
cvar_t multicastport = { "multicastport", "27025", 0, 0.0f, NULL };

#ifdef _WIN32
cvar_t ipx_hostport = { "ipx_hostport", "0", 0, 0.0f, NULL };
cvar_t ipx_clientport = { "ipx_clientport", "0", 0, 0.0f, NULL };
#endif

cvar_t fakelag = { "fakelag", "0.0", 0, 0.0f, NULL };
cvar_t fakeloss = { "fakeloss", "0.0", 0, 0.0f, NULL };
cvar_t net_graph = { "net_graph", "0", FCVAR_ARCHIVE, 0.0f, NULL };
cvar_t net_graphwidth = { "net_graphwidth", "150", 0, 0.0f, NULL };
cvar_t net_scale = { "net_scale", "5", FCVAR_ARCHIVE, 0.0f, NULL };
cvar_t net_graphpos = { "net_graphpos", "1", FCVAR_ARCHIVE, 0.0f, NULL };

void NET_ThreadLock()
{
#ifdef _WIN32
	if (use_thread && net_thread_initialized)
	{
		EnterCriticalSection(&net_cs);
	}
#endif // _WIN32
}

void NET_ThreadUnlock()
{
#ifdef _WIN32
	if (use_thread && net_thread_initialized)
	{
		LeaveCriticalSection(&net_cs);
	}
#endif // _WIN32
}

unsigned short Q_ntohs(unsigned short netshort)
{
	return ntohs(netshort);
}

void NetadrToSockadr(const netadr_t *a, struct sockaddr *s)
{
	Q_memset(s, 0, sizeof(*s));

	auto s_in = (sockaddr_in *)s;

	switch (a->type)
	{
	case NA_BROADCAST:
		s_in->sin_family = AF_INET;
		s_in->sin_addr.s_addr = INADDR_BROADCAST;
		s_in->sin_port = a->port;
		break;
	case NA_IP:
		s_in->sin_family = AF_INET;
		s_in->sin_addr.s_addr = *(int *)&a->ip;
		s_in->sin_port = a->port;
		break;
#ifdef _WIN32
	case NA_IPX:
		s->sa_family = AF_IPX;
		Q_memcpy(s->sa_data, a->ipx, 10);
		*(unsigned short *)&s->sa_data[10] = a->port;
		break;
	case NA_BROADCAST_IPX:
		s->sa_family = AF_IPX;
		Q_memset(&s->sa_data, 0, 4);
		Q_memset(&s->sa_data[4], 255, 6);
		*(unsigned short *)&s->sa_data[10] = a->port;
		break;
#endif // _WIN32
	default:
		break;
	}
}

void SockadrToNetadr(const struct sockaddr *s, netadr_t *a)
{
	if (s->sa_family == AF_INET)
	{
		a->type = NA_IP;
		*(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
		a->port = ((struct sockaddr_in *)s)->sin_port;
	}
#ifdef _WIN32
	else if (s->sa_family == AF_IPX)
	{
		a->type = NA_IPX;
		Q_memcpy(a->ipx, s->sa_data, sizeof(a->ipx));
		a->port = *(unsigned short *)&s->sa_data[10];
	}
#endif // _WIN32
}

NOXREF unsigned short NET_HostToNetShort(unsigned short us_in)
{
	NOXREFCHECK;

	return htons(us_in);
}

qboolean NET_CompareAdr(netadr_t& a, netadr_t& b)
{
	if (a.type != b.type)
	{
		return FALSE;
	}
	if (a.type == NA_LOOPBACK)
	{
		return TRUE;
	}

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] &&
			a.ip[1] == b.ip[1] &&
			a.ip[2] == b.ip[2] &&
			a.ip[3] == b.ip[3] &&
			a.port == b.port)
			return TRUE;
	}
#ifdef _WIN32
	else if (a.type == NA_IPX)
	{
		if (Q_memcmp(a.ipx, b.ipx, 10) == 0 &&
			a.port == b.port)
			return TRUE;
	}
#endif // _WIN32

	return FALSE;
}

qboolean NET_CompareClassBAdr(netadr_t& a, netadr_t& b)
{
	if (a.type != b.type)
	{
		return FALSE;
	}
	if (a.type == NA_LOOPBACK)
	{
		return TRUE;
	}

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] &&
			a.ip[1] == b.ip[1])
			return TRUE;
	}
#ifdef _WIN32
	else if (a.type == NA_IPX)
	{
		return TRUE;
	}
#endif // _WIN32

	return FALSE;
}

qboolean NET_IsReservedAdr(netadr_t& a)
{
	if (a.type == NA_LOOPBACK)
	{
		return TRUE;
	}
	if (a.type == NA_IP)
	{
		if (a.ip[0] == 10 || a.ip[0] == 127)
		{
			return TRUE;
		}
		if (a.ip[0] == 172 && a.ip[1] >= 16)
		{
			if (a.ip[1] >= 32)
			{
				return FALSE;
			}
			return TRUE;
		}
		if (a.ip[0] == 192 && a.ip[1] >= 168)
		{
			return TRUE;
		}
		return FALSE;
	}
#ifdef _WIN32
	else if (a.type == NA_IPX)
	{
		return TRUE;
	}
#endif // _WIN32

	return FALSE;
}

qboolean NET_CompareBaseAdr(const netadr_t& a, const netadr_t& b)
{
	if (a.type != b.type)
	{
		return FALSE;
	}
	if (a.type == NA_LOOPBACK)
	{
		return TRUE;
	}

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] &&
			a.ip[1] == b.ip[1] &&
			a.ip[2] == b.ip[2] &&
			a.ip[3] == b.ip[3])
			return TRUE;
	}
#ifdef _WIN32
	else if (a.type == NA_IPX)
	{
		if (Q_memcmp(a.ipx, b.ipx, 10) == 0)
			return TRUE;
	}
#endif // _WIN32

	return FALSE;
}

char *NET_AdrToString(const netadr_t& a)
{
	static char s[64];

	Q_memset(s, 0, sizeof(s));

	if (a.type == NA_LOOPBACK)
		Q_snprintf(s, sizeof(s), "loopback");
	else if (a.type == NA_IP)
		Q_snprintf(s, sizeof(s), "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));
#ifdef _WIN32
	else // NA_IPX
		Q_snprintf(s, sizeof(s), "%02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x:%i", a.ipx[0], a.ipx[1], a.ipx[2], a.ipx[3], a.ipx[4], a.ipx[5], a.ipx[6], a.ipx[7], a.ipx[8], a.ipx[9], ntohs(a.port));
#endif // _WIN32

	return s;
}

char *NET_BaseAdrToString(netadr_t& a)
{
	static char s[64];

	if (a.type == NA_LOOPBACK)
	{
		Q_snprintf(s, sizeof(s), "loopback");
	}
	else if (a.type == NA_IP)
	{
		Q_snprintf(s, sizeof(s), "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3]);
	}
#ifdef _WIN32
	else // NA_IPX
	{
		Q_snprintf(s, sizeof(s), "%02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x", a.ipx[0], a.ipx[1], a.ipx[2], a.ipx[3], a.ipx[4], a.ipx[5], a.ipx[6], a.ipx[7], a.ipx[8], a.ipx[9]);
	}
#endif // _WIN32

	return s;
}

qboolean NET_StringToSockaddr(const char *s, struct sockaddr *sadr)
{
	Q_memset(sadr, 0, sizeof(*sadr));

#ifdef _WIN32
	// IPX support.
	if (Q_strlen(s) >= 24 && s[8] == ':' && s[21] == ':')
	{
		sadr->sa_family = AF_IPX;
		int val = 0;
		for(int i = 0; i < 20; i+=2)
		{
			if (s[i] == ':')
			{
				--i; // Skip one char on next iteration.
				continue;
			}
			// Convert from hexademical represantation to sockaddr
			char temp[3] = { s[i], s[i + 1], '\0' };
			sscanf(temp, "%x", &val);
			sadr->sa_data[i / 2] = (char)val;
		}

		sscanf(s + 22, "%u", &val);
		*(uint16 *)&sadr->sa_data[10] = htons(val);

		return TRUE;
	}
#endif // _WIN32

	auto *sadr_in = (sockaddr_in *)sadr;

	sadr_in->sin_family = AF_INET;

	char copy[128];
	Q_strncpy(copy, s, sizeof(copy) - 1);
	copy[sizeof(copy) - 1] = 0;

	// Parse port
	char *colon = Q_strchr(copy, ':');
	if(colon != nullptr)
	{
		*colon = '\0';
		sadr_in->sin_port = htons(Q_atoi(colon + 1));
	}
	else
	{
		sadr_in->sin_port = 0;
	}

	// Parse address
	sadr_in->sin_addr.s_addr = inet_addr(copy);
	if (sadr_in->sin_addr.s_addr == INADDR_NONE)
	{
		struct hostent *host = CRehldsPlatformHolder::get()->gethostbyname(copy);

		if (host == nullptr || host->h_addr == nullptr)
		{
			return FALSE;
		}

		sadr_in->sin_addr.s_addr = *(uint32 *)host->h_addr;
	}
	return TRUE;
}

qboolean NET_StringToAdr(const char *s, netadr_t *a)
{
	struct sockaddr sadr;

	if (Q_strcmp(s, "localhost"))
	{
		if (!NET_StringToSockaddr(s, &sadr))
		{
			return FALSE;
		}
		SockadrToNetadr(&sadr, a);
	}
	else
	{
		Q_memset(a, 0, sizeof(netadr_t));
		a->type = NA_LOOPBACK;
	}
	return TRUE;
}

qboolean NET_IsLocalAddress(const netadr_t& adr)
{
	return adr.type == NA_LOOPBACK ? TRUE : FALSE;
}

int NET_GetLastError()
{
#ifdef _WIN32
	return CRehldsPlatformHolder::get()->WSAGetLastError();
#else
	return errno;
#endif // _WIN32
}

char *NET_ErrorString(int code)
{
#ifdef _WIN32
	switch (code)
	{
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEFAULT: return "WSAEFAULT";

	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";

	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";

	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";

	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAEDISCON: return "WSAEDISCON";

	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";

	default: return "NO ERROR";
	}
#else // _WIN32
	return strerror(code);
#endif // _WIN32
}

void NET_TransferRawData(sizebuf_t *msg, unsigned char *pStart, int nSize)
{
#ifdef REHLDS_CHECKS
	if (msg->maxsize < nSize)
	{
		Sys_Error("%s: data size is bigger than sizebuf maxsize", __func__);
	}
#endif // REHLDS_CHECKS
	Q_memcpy(msg->data, pStart, nSize);
	msg->cursize = nSize;
}

qboolean NET_GetLoopPacket(netsrc_t sock, netadr_t *in_from_, sizebuf_t *msg)
{
	loopback_t *loop = &loopbacks[sock];

	if (loop->send - loop->get > 4)
	{
		loop->get = loop->send - 4;
	}

	if (loop->get < loop->send)
	{
		int i = loop->get & (MAX_LOOPBACK - 1);
		loop->get++;

		NET_TransferRawData(msg, loop->msgs[i].data, loop->msgs[i].datalen);

		Q_memset(in_from_, 0, sizeof(netadr_t));
		in_from_->type = NA_LOOPBACK;

		return TRUE;
	}
	return FALSE;
}

void NET_SendLoopPacket(netsrc_t sock, int length, void *data, const netadr_t& to)
{
	NET_ThreadLock();

	loopback_t *loop = &loopbacks[sock ^ 1];

	int i = loop->send & (MAX_LOOPBACK - 1);
	loop->send++;

#ifdef REHLDS_CHECKS
	if (sizeof(loop->msgs[i].data) < length)
	{
		Sys_Error("%s: data size is bigger than message storage size", __func__);
	}
#endif // REHLDS_CHECKS

	Q_memcpy(loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;

	NET_ThreadUnlock();
}

void NET_RemoveFromPacketList(packetlag_t *pPacket)
{
	pPacket->pPrev->pNext = pPacket->pNext;
	pPacket->pNext->pPrev = pPacket->pPrev;
	pPacket->pPrev = 0;
	pPacket->pNext = 0;
}

NOXREF int NET_CountLaggedList(packetlag_t *pList)
{
	NOXREFCHECK;

	int c = 0;
	for (packetlag_t* p = pList->pNext; p && p != pList; p = p->pNext)
	{
		c++;
	}

	return c;
}

void NET_ClearLaggedList(packetlag_t *pList)
{
	packetlag_t *p = pList->pNext;
	while (p && p != pList)
	{
		packetlag_t *n = p->pNext;
		NET_RemoveFromPacketList(p);
		if (p->pPacketData)
		{
			Mem_Free(p->pPacketData);
			p->pPacketData = NULL;
		}
		Mem_Free(p);
		p = n;
	}

	pList->pPrev = pList;
	pList->pNext = pList;
}

void NET_AddToLagged(netsrc_t sock, packetlag_t *pList, packetlag_t *pPacket, netadr_t *net_from_, sizebuf_t messagedata, float timestamp)
{
	unsigned char *pStart;

	if (pPacket->pPrev || pPacket->pNext)
	{
		Con_Printf("Packet already linked\n");
		return;
	}

	pPacket->pPrev = pList->pPrev;
	pList->pPrev->pNext = pPacket;
	pList->pPrev = pPacket;
	pPacket->pNext = pList;

	pStart = (unsigned char *)Mem_Malloc(messagedata.cursize);
	Q_memcpy(pStart, messagedata.data, messagedata.cursize);
	pPacket->pPacketData = pStart;
	pPacket->nSize = messagedata.cursize;
	pPacket->receivedTime = timestamp;
	Q_memcpy(&pPacket->net_from_, net_from_, sizeof(netadr_t));
}

void NET_AdjustLag()
{
	static double lasttime = realtime;

	double dt = realtime - lasttime;
	if (dt <= 0.0)
	{
		dt = 0.0;
	}
	else
	{
		if (dt > 0.1)
		{
			dt = 0.1;
		}
	}
	lasttime = realtime;

	if (sv_cheats.value || fakelag.value == 0.0)
	{
		if (fakelag.value != gFakeLag)
		{
			float diff = fakelag.value - gFakeLag;
			float converge = dt * 200.0;
			if (fabs(diff) < converge)
				converge = fabs(diff);
			if (diff < 0.0)
				converge = -converge;
			gFakeLag = gFakeLag + converge;
		}
	}
	else
	{
		Con_Printf("Server must enable cheats to activate fakelag\n");
		Cvar_SetValue("fakelag", 0.0);
		gFakeLag = 0;
	}
}

qboolean NET_LagPacket(qboolean newdata, netsrc_t sock, netadr_t *from, sizebuf_t *data)
{
	if (gFakeLag <= 0.0)
	{
		NET_ClearLagData(TRUE, TRUE);
		return newdata;
	}

	float curtime = realtime;
#ifdef REHLDS_FIXES
	if (newdata && data)
#else
	if (newdata)
#endif
	{
		if (fakeloss.value != 0.0)
		{
			if (sv_cheats.value)
			{
				static int losscount[NS_MAX] = {};
				++losscount[sock];
				if (fakeloss.value <= 0.0f)
				{
					int ninterval = fabs(fakeloss.value);
					if (ninterval < 2)
						ninterval = 2;
					if ((losscount[sock] % ninterval) == 0)
						return FALSE;
				}
				else
				{
					if (RandomLong(0, 100) <= fakeloss.value)
						return FALSE;
				}
			}
			else
			{
				Cvar_SetValue("fakeloss", 0.0);
			}
		}

		packetlag_t *pNewPacketLag = (packetlag_t *)Mem_ZeroMalloc(sizeof(packetlag_t));
		NET_AddToLagged(sock, &g_pLagData[sock], pNewPacketLag, from, *data, curtime);
	}

	packetlag_t *pPacket = g_pLagData[sock].pNext;

	while (pPacket != &g_pLagData[sock])
	{
		if (pPacket->receivedTime <= curtime - gFakeLag / 1000.0)
			break;

		pPacket = pPacket->pNext;
	}

	if (pPacket == &g_pLagData[sock])
		return FALSE;

	NET_RemoveFromPacketList(pPacket);
	NET_TransferRawData(&in_message, pPacket->pPacketData, pPacket->nSize);
	Q_memcpy(&in_from, &pPacket->net_from_, sizeof(in_from));
	if (pPacket->pPacketData)
		free(pPacket->pPacketData);

	Mem_Free(pPacket);
	return TRUE;
}

void NET_FlushSocket(netsrc_t sock)
{
	SOCKET net_socket = ip_sockets[sock];
	if (net_socket != INV_SOCK)
	{
		struct sockaddr from;
		socklen_t fromlen;
		unsigned char buf[MAX_UDP_PACKET];

		fromlen = 16;
		while (CRehldsPlatformHolder::get()->recvfrom(net_socket, (char*)buf, sizeof buf, 0, &from, &fromlen) > 0)
			;
	}
}

qboolean NET_GetLong(unsigned char *pData, int size, int *outSize)
{
	static int gNetSplitFlags[NET_WS_MAX_FRAGMENTS];
	SPLITPACKET *pHeader = (SPLITPACKET *) pData;

	int sequenceNumber = pHeader->sequenceNumber;
	unsigned char packetID = pHeader->packetID;
	unsigned int packetCount = packetID & 0xF;
	unsigned int packetNumber = (unsigned int)packetID >> 4;

	if (packetNumber >= NET_WS_MAX_FRAGMENTS || packetCount > NET_WS_MAX_FRAGMENTS)
	{
		Con_NetPrintf("Malformed packet number (%i/%i)\n", packetNumber + 1, packetCount);
		return FALSE;
	}

	if (gNetSplit.currentSequence == -1 || sequenceNumber != gNetSplit.currentSequence)
	{
		gNetSplit.currentSequence = pHeader->sequenceNumber;
		gNetSplit.splitCount = packetCount;

#ifdef REHLDS_FIXES
		gNetSplit.totalSize = 0;

		// clear part's sequence
		for (int i = 0; i < NET_WS_MAX_FRAGMENTS; i++)
			gNetSplitFlags[i] = -1;
#endif

		if (net_showpackets.value == 4.0f)
			Con_Printf("<-- Split packet restart %i count %i seq\n", gNetSplit.splitCount, sequenceNumber);
	}

	unsigned int packetPayloadSize = size - sizeof(SPLITPACKET);
	if (gNetSplitFlags[packetNumber] == sequenceNumber)
	{
		Con_NetPrintf("%s:  Ignoring duplicated split packet %i of %i ( %i bytes )\n", __func__, packetNumber + 1, packetCount, packetPayloadSize);
	}
	else
	{
		if (packetNumber == packetCount - 1)
			gNetSplit.totalSize = packetPayloadSize + SPLIT_SIZE * (packetCount - 1);

		--gNetSplit.splitCount;
		gNetSplitFlags[packetNumber] = sequenceNumber;

		if (net_showpackets.value == 4.0f)
		{
			Con_Printf("<-- Split packet %i of %i, %i bytes %i seq\n", packetNumber + 1, packetCount, packetPayloadSize, sequenceNumber);
		}

		if (SPLIT_SIZE * packetNumber + packetPayloadSize > MAX_UDP_PACKET)
		{
			Con_NetPrintf("Malformed packet size (%i, %i)\n", SPLIT_SIZE * packetNumber, packetPayloadSize);
#ifdef REHLDS_FIXES
			gNetSplit.currentSequence = -1;
#endif
			return FALSE;
		}

		Q_memcpy(&gNetSplit.buffer[SPLIT_SIZE * packetNumber], pHeader + 1, packetPayloadSize);
	}

	if (gNetSplit.splitCount > 0)
		return FALSE;

	if (packetCount > 0)
	{
		for (unsigned int i = 0; i < packetCount; i++)
		{
			if (gNetSplitFlags[i] != gNetSplit.currentSequence)
			{
				Con_NetPrintf(
					"Split packet without all %i parts, part %i had wrong sequence %i/%i\n",
					packetCount,
					i + 1,
					gNetSplitFlags[i],
					gNetSplit.currentSequence);
#ifdef REHLDS_FIXES
				gNetSplit.currentSequence = -1; // no more parts can be attached, clear it
#endif
				return FALSE;
			}
		}
	}

	gNetSplit.currentSequence = -1;
	if (gNetSplit.totalSize <= MAX_UDP_PACKET)
	{
		Q_memcpy(pData, gNetSplit.buffer, gNetSplit.totalSize);
		*outSize = gNetSplit.totalSize;
		return TRUE;
	}
	else
	{
#ifdef REHLDS_FIXES
		*outSize = 0;
#endif
		Con_NetPrintf("Split packet too large! %d bytes\n", gNetSplit.totalSize);
		return FALSE;
	}
}

qboolean NET_QueuePacket(netsrc_t sock)
{
#ifdef REHLDS_FIXES
	while (true)
#endif // REHLDS_FIXES
	{
		int ret = -1;
		unsigned char buf[MAX_UDP_PACKET];

#ifdef _WIN32
		for (int protocol = 0; protocol < 2; protocol++)
#else
		for (int protocol = 0; protocol < 1; protocol++)
#endif // _WIN32
		{
			SOCKET net_socket = INV_SOCK;

			if (protocol == 0)
				net_socket = ip_sockets[sock];
#ifdef _WIN32
			else
				net_socket = ipx_sockets[sock];
#endif // _WIN32

			if (net_socket == INV_SOCK)
				continue;

			struct sockaddr from;
			socklen_t fromlen = sizeof(from);
			ret = CRehldsPlatformHolder::get()->recvfrom(net_socket, (char *)buf, sizeof buf, 0, &from, &fromlen);
			if (ret == -1)
			{
				int err = NET_GetLastError();
#ifdef _WIN32
				if (err == WSAENETRESET)
					continue;
#endif // _WIN32

				if (err != WSAEWOULDBLOCK && err != WSAECONNRESET && err != WSAECONNREFUSED)
				{
					if (err == WSAEMSGSIZE)
					{
						Con_DPrintf("%s: Ignoring oversized network message\n", __func__);
					}
					else
					{
						if (g_pcls.state != ca_dedicated)
						{
							Sys_Error("%s: %s", __func__, NET_ErrorString(err));
						}
						Con_Printf("%s: %s\n", __func__, NET_ErrorString(err));
					}
				}
				continue;
			}

			SockadrToNetadr(&from, &in_from);
			if (ret != MAX_UDP_PACKET)
				break;

			Con_NetPrintf("%s: Oversize packet from %s\n", __func__, NET_AdrToString(in_from));
		}

#ifdef REHLDS_FIXES
		if (ret == -1)
		{
			return NET_LagPacket(FALSE, sock, NULL, NULL);
		}
		if (ret == MAX_UDP_PACKET)
		{
			continue;
		}
#else // REHLDS_FIXES
		if (ret == -1 || ret == MAX_UDP_PACKET)
		{
			return NET_LagPacket(FALSE, sock, NULL, NULL);
		}
#endif // REHLDS_FIXES

		NET_TransferRawData(&in_message, buf, ret);

		if (*(int32 *)in_message.data != NET_HEADER_FLAG_SPLITPACKET)
		{
			return NET_LagPacket(TRUE, sock, &in_from, &in_message);
		}

		if (in_message.cursize < 9)
		{
			Con_NetPrintf("Invalid split packet length %i\n", in_message.cursize);
#ifdef REHLDS_FIXES
			continue;
#else // REHLDS_FIXES
			return FALSE;
#endif // REHLDS_FIXES
		}

#ifdef REHLDS_FIXES
		// Only server can send split packets, there is no server<->server communication, so server can't receive split packets
		if (sock == NS_SERVER)
		{
			Con_NetPrintf("Someone tries to send split packet to the server\n");
			continue;
		}
#endif

#ifdef REHLDS_FIXES
		if (!NET_GetLong(in_message.data, ret, &in_message.cursize))
		{
			continue;
		}

		return TRUE;
#else // REHLDS_FIXES
		return NET_GetLong(in_message.data, ret, &in_message.cursize);
#endif // REHLDS_FIXES
	}
}

DLL_EXPORT int NET_Sleep_Timeout()
{
	static int32 lasttime;
	static int numFrames;
	static int staggerFrames;

	int fps = (int)sys_ticrate.value;
	int32 curtime = (int)Sys_FloatTime();
	if (lasttime)
	{
		if (curtime - lasttime > 1)
		{
			lasttime = curtime;
			numFrames = fps;
			staggerFrames = fps / 100 + 1;
		}
	}
	else
	{
		lasttime = curtime;
	}

	fd_set fdset;
	FD_ZERO(&fdset);

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = (1000 / fps) * 1000; // TODO: entirely bad code, fix it completely
	if (tv.tv_usec <= 0)
		tv.tv_usec = 1;

	int res;
	if (numFrames > 0 && numFrames % staggerFrames)
	{
		SOCKET number = 0;

		for (int sock = 0; sock < NS_MAX; sock++)
		{
			SOCKET net_socket = ip_sockets[sock];
			if (net_socket != INV_SOCK)
			{
				FD_SET(net_socket, &fdset);

				if (number < net_socket)
					number = net_socket;
			}

#ifdef _WIN32
			net_socket = ipx_sockets[sock];
			if (net_socket != INV_SOCK)
			{
				FD_SET(net_socket, &fdset);

				if (number < net_socket)
					number = net_socket;
			}
#endif // _WIN32
		}
		res = select((int)(number + 1), &fdset, NULL, NULL, &tv);
	}
	else
	{
		res = select(0, NULL, NULL, NULL, &tv);
	}
	--numFrames;
	return res;
}

int NET_Sleep()
{
	fd_set fdset;
	FD_ZERO(&fdset);

	SOCKET number = 0;
	for (int sock = 0; sock < NS_MAX; sock++)
	{
		SOCKET net_socket = ip_sockets[sock];
		if (net_socket != INV_SOCK)
		{
			FD_SET(net_socket, &fdset);

			if (number < net_socket)
				number = net_socket;
		}

#ifdef _WIN32
		net_socket = ipx_sockets[sock];
		if (net_socket != INV_SOCK)
		{
			FD_SET(net_socket, &fdset);

			if (number < net_socket)
				number = net_socket;
		}
#endif // _WIN32
	}

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 20 * 1000;

	return select((int)(number + 1), &fdset, NULL, NULL, net_sleepforever == 0 ? &tv : NULL);
}

#ifdef _WIN32

DWORD WINAPI NET_ThreadMain(LPVOID lpThreadParameter)
{
	while (true)
	{
		while (NET_Sleep())
		{
			qboolean bret = FALSE;
			for (int sock = 0; sock < NS_MAX; sock++)
			{
				NET_ThreadLock();

				bret = NET_QueuePacket((netsrc_t)sock);
				if (bret)
				{
					net_messages_t *pmsg = NET_AllocMsg(in_message.cursize);
					pmsg->next = nullptr;
					Q_memcpy(pmsg->buffer, in_message.data, in_message.cursize);
					Q_memcpy(&pmsg->from, &in_from, sizeof(pmsg->from));

					// add to tail of the list
					net_messages_t *p = messages[sock];
					if (p)
					{
						while (p->next)
							p = p->next;

						p->next = pmsg;
					}
					// add to head
					else
					{
						messages[sock] = pmsg;
					}
				}

				NET_ThreadUnlock();
			}

			if (!bret)
				break;
		}

		Sys_Sleep(1);
	}

	return 0;
}

#endif // _WIN32

void NET_StartThread()
{
	if (use_thread)
	{
		if (!net_thread_initialized)
		{
			net_thread_initialized = TRUE;

#ifdef _WIN32
			InitializeCriticalSection(&net_cs);
			hNetThread = CreateThread(0, 0, NET_ThreadMain, 0, 0, &dwNetThreadId);
			if (!hNetThread)
			{
				DeleteCriticalSection(&net_cs);
				net_thread_initialized = FALSE;
				use_thread = FALSE;
				Sys_Error("%s: Couldn't initialize network thread, run without -netthread\n", __func__);
			}
#endif // _WIN32
		}
	}
}

void NET_StopThread()
{
	if (use_thread)
	{
		if (net_thread_initialized)
		{
#ifdef _WIN32
			TerminateThread(hNetThread, 0);
			DeleteCriticalSection(&net_cs);
#endif // _WIN32
			net_thread_initialized = FALSE;
		}
	}
}

void *net_malloc(size_t size)
{
	return Mem_ZeroMalloc(size);
}

net_messages_t *NET_AllocMsg(int size)
{
	net_messages_t *pmsg;
	if (size <= MSG_QUEUE_SIZE && normalqueue)
	{
		pmsg = normalqueue;
		pmsg->buffersize = size;
		normalqueue = pmsg->next;
	}
	else
	{
		pmsg = (net_messages_t *)net_malloc(sizeof(net_messages_t));
		pmsg->buffer = (unsigned char *)net_malloc(size);
		pmsg->buffersize = size;
		pmsg->preallocated = FALSE;
	}

	return pmsg;
}

void NET_FreeMsg(net_messages_t *pmsg)
{
	if (pmsg->preallocated)
	{
		net_messages_t* tmp = normalqueue;
		normalqueue = pmsg;
		pmsg->next = tmp;
	}
	else
	{
		Mem_Free(pmsg->buffer);
		Mem_Free(pmsg);
	}
}

qboolean NET_GetPacket(netsrc_t sock)
{
	qboolean bret;

	NET_AdjustLag();
	NET_ThreadLock();
	if (NET_GetLoopPacket(sock, &in_from, &in_message))
	{
		bret = NET_LagPacket(TRUE, sock, &in_from, &in_message);
	}
	else
	{
		if (!use_thread)
		{
			bret = NET_QueuePacket(sock);
			if (!bret)
				bret = NET_LagPacket(FALSE, sock, NULL, NULL);
		}
		else
		{
			bret = NET_LagPacket(FALSE, sock, NULL, NULL);
		}
	}

	if (bret)
	{
		Q_memcpy(net_message.data, in_message.data, in_message.cursize);
		net_message.cursize = in_message.cursize;
		Q_memcpy(&net_from, &in_from, sizeof(netadr_t));
		NET_ThreadUnlock();
		return bret;
	}

	net_messages_t *pmsg = messages[sock];
	if (pmsg)
	{
		net_message.cursize = pmsg->buffersize;
		messages[sock] = pmsg->next;
		Q_memcpy(net_message.data, pmsg->buffer, net_message.cursize);
		net_from = pmsg->from;
		msg_readcount = 0;
		NET_FreeMsg(pmsg);
		bret = TRUE;
	}
	NET_ThreadUnlock();
	return bret;
}

void NET_AllocateQueues()
{
	for (int i = 0; i < NUM_MSG_QUEUES; i++)
	{
		net_messages_t *p = (net_messages_t *)Mem_ZeroMalloc(sizeof(net_messages_t));
		p->buffer = (unsigned char *)Mem_ZeroMalloc(MSG_QUEUE_SIZE);
		p->preallocated = TRUE;
		p->next = normalqueue;
		normalqueue = p;
	}

	NET_StartThread();
}

void NET_FlushQueues()
{
	NET_StopThread();

	for (int i = 0; i < NS_MAX; i++)
	{
		net_messages_t *p = messages[i];
		while (p)
		{
			net_messages_t *n = p->next;
			Mem_Free(p->buffer);
			Mem_Free(p);
			p = n;
		}

		messages[i] = NULL;
	}

	net_messages_t *p = normalqueue;
	while (p)
	{
		net_messages_t *n = p->next;
		Mem_Free(p->buffer);
		Mem_Free(p);
		p = n;
	}
	normalqueue = NULL;
}

int NET_SendLong(netsrc_t sock, SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen)
{
	static long gSequenceNumber = 1;

	// Do we need to break this packet up?
	if (sock == NS_SERVER && len > MAX_ROUTEABLE_PACKET)
	{
		// yep
		gSequenceNumber++;
		if (gSequenceNumber < 0)
		{
			gSequenceNumber = 1;
		}

		char packet[MAX_ROUTEABLE_PACKET];
		SPLITPACKET *pPacket = (SPLITPACKET *)packet;
		pPacket->netID = NET_HEADER_FLAG_SPLITPACKET;
		pPacket->sequenceNumber = gSequenceNumber;
		int packetNumber = 0;
		int totalSent = 0;
		int packetCount = (len + SPLIT_SIZE - 1) / SPLIT_SIZE;

		while (len > 0)
		{
			int size = Q_min(int(SPLIT_SIZE), len);

			pPacket->packetID = (packetNumber << 4) + packetCount;

			Q_memcpy(packet + sizeof(SPLITPACKET), buf + (packetNumber * SPLIT_SIZE), size);

			if (net_showpackets.value == 4.0f)
			{
				netadr_t adr;
				Q_memset(&adr, 0, sizeof(adr));

				SockadrToNetadr((struct sockaddr *)to, &adr);

				Con_DPrintf("Sending split %i of %i with %i bytes and seq %i to %s\n",
					packetNumber + 1,
					packetCount,
					size,
					gSequenceNumber,
					NET_AdrToString(adr));
			}

			int ret = CRehldsPlatformHolder::get()->sendto(s, packet, size + sizeof(SPLITPACKET), flags, to, tolen);
			if (ret < 0)
			{
				return ret;
			}

			if (ret >= size)
			{
				totalSent += size;
			}

			len -= size;
			packetNumber++;
		}

		return totalSent;
	}

	int nSend = CRehldsPlatformHolder::get()->sendto(s, buf, len, flags, to, tolen);
	return nSend;
}


void EXT_FUNC NET_SendPacket_api(unsigned int length, void *data, const netadr_t &to)
{
	NET_SendPacket(NS_SERVER, length, data, to);
}

void NET_SendPacket(netsrc_t sock, int length, void *data, const netadr_t& to)
{
	if (to.type == NA_LOOPBACK)
	{
		NET_SendLoopPacket(sock, length, data, to);
		return;
	}

	SOCKET net_socket;
	if (to.type == NA_BROADCAST)
	{
		net_socket = ip_sockets[sock];
		if (net_socket == INV_SOCK)
			return;
	}
	else if (to.type == NA_IP)
	{
		net_socket = ip_sockets[sock];
		if (net_socket == INV_SOCK)
			return;
	}
#ifdef _WIN32
	else if (to.type == NA_IPX)
	{
		net_socket = ipx_sockets[sock];
		if (net_socket == INV_SOCK)
			return;
	}
	else if (to.type == NA_BROADCAST_IPX)
	{
		net_socket = ipx_sockets[sock];
		if (net_socket == INV_SOCK)
			return;
	}
#endif // _WIN32
	else
	{
		Sys_Error("%s: bad address type", __func__);
	}

	struct sockaddr addr;
	NetadrToSockadr(&to, &addr);

	int ret = NET_SendLong(sock, net_socket, (const char *)data, length, 0, &addr, sizeof(addr));
	if (ret == -1)
	{
		int err = NET_GetLastError();

		// wouldblock is silent
		if (err == WSAEWOULDBLOCK)
			return;

		if (err == WSAECONNREFUSED || err == WSAECONNRESET)
			return;

		// some PPP links dont allow broadcasts
		if (err == WSAEADDRNOTAVAIL && (to.type == NA_BROADCAST
#ifdef _WIN32
			|| to.type == NA_BROADCAST_IPX
#endif // _WIN32
			))
			return;

		// let dedicated servers continue after errors
		if (g_pcls.state == ca_dedicated)
		{
			Con_Printf("%s: ERROR: %s\n", __func__, NET_ErrorString(err));
		}
		else
		{
			if (err == WSAEADDRNOTAVAIL || err == WSAENOBUFS)
			{
				Con_DPrintf("%s: Warning: %s : %s\n", __func__, NET_ErrorString(err), NET_AdrToString(to));
			}
			else
			{
				Sys_Error("%s: ERROR: %s\n", __func__, NET_ErrorString(err));
			}
		}
	}
}


SOCKET NET_IPSocket(char *net_interface, int port, qboolean multicast)
{
	SOCKET newsocket;
	qboolean _true = TRUE;

#ifdef _WIN32
	if ((newsocket = CRehldsPlatformHolder::get()->socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
#else
	if ((newsocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
#endif // _WIN32
	{
		int err = NET_GetLastError();
		if (err != WSAEAFNOSUPPORT)
		{
			Con_Printf("WARNING: UDP_OpenSocket: port: %d socket: %s", port, NET_ErrorString(err));
		}
		return INV_SOCK;
	}

#ifdef _WIN32
	if (CRehldsPlatformHolder::get()->ioctlsocket(newsocket, FIONBIO, (u_long *)&_true) == SOCKET_ERROR)
#else
	if (SOCKET_FIONBIO(newsocket, _true) == SOCKET_ERROR)
#endif // _WIN32
	{
		Con_Printf("WARNING: UDP_OpenSocket: port: %d  ioctl FIONBIO: %s\n", port, NET_ErrorString(NET_GetLastError()));
		return INV_SOCK;
	}

	qboolean i = TRUE;
#ifdef _WIN32
	if (CRehldsPlatformHolder::get()->setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == SOCKET_ERROR)
#else
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == SOCKET_ERROR)
#endif
	{
		Con_Printf ("WARNING: UDP_OpenSocket: port: %d  setsockopt SO_BROADCAST: %s\n", port, NET_ErrorString(NET_GetLastError()));
		return INV_SOCK;
	}

	if (COM_CheckParm("-reuse") || multicast)
	{
#ifdef _WIN32
		if (CRehldsPlatformHolder::get()->setsockopt(newsocket, SOL_SOCKET, SO_REUSEADDR, (char *)&_true, sizeof(qboolean)) == SOCKET_ERROR)
#else
		if (setsockopt(newsocket, SOL_SOCKET, SO_REUSEADDR, (char *)&_true, sizeof(qboolean)) == SOCKET_ERROR)
#endif // _WIN32
		{
			Con_Printf ("WARNING: UDP_OpenSocket: port: %d  setsockopt SO_REUSEADDR: %s\n", port, NET_ErrorString(NET_GetLastError()));
			return INV_SOCK;
		}
	}

#ifndef _WIN32
	if (COM_CheckParm("-tos"))
	{
		int i = IPTOS_LOWDELAY;
		Con_Printf("Enabling LOWDELAY TOS option\n");
		if (setsockopt(newsocket, IPPROTO_IP, IP_TOS, (char *)&i, sizeof(i)) == SOCKET_ERROR)
		{
			int err = NET_GetLastError();
			if (err != WSAENOPROTOOPT)
				Con_Printf("WARNING: UDP_OpenSocket: port: %d  setsockopt IP_TOS: %s\n", port, NET_ErrorString(err));
			return INV_SOCK;
		}
	}
#endif // _WIN32

	struct sockaddr_in address;

	if (net_interface && *net_interface && Q_stricmp(net_interface, "localhost"))
		NET_StringToSockaddr(net_interface, (sockaddr *)&address);
	else
		address.sin_addr.s_addr = INADDR_ANY;

	if (port == -1) // TODO: Always false?
		address.sin_port = 0;
	else
		address.sin_port = htons((u_short)port);

	address.sin_family = AF_INET;

#ifdef _WIN32
	if (CRehldsPlatformHolder::get()->bind(newsocket, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
#else
	if (bind(newsocket, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
#endif // _WIN32
	{
		Con_Printf("WARNING: UDP_OpenSocket: port: %d  bind: %s\n", port, NET_ErrorString(NET_GetLastError()));
#ifdef _WIN32
		CRehldsPlatformHolder::get()->closesocket(newsocket);
#else
		SOCKET_CLOSE(newsocket);
#endif // _WIN32
		return INV_SOCK;
	}

	qboolean bLoopBack = COM_CheckParm("-loopback") != 0;
#ifdef _WIN32
	if (CRehldsPlatformHolder::get()->setsockopt(newsocket, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&bLoopBack, sizeof(bLoopBack)) == SOCKET_ERROR)
#else
	if (setsockopt(newsocket, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&bLoopBack, sizeof(bLoopBack)) == SOCKET_ERROR)
#endif // _WIN32
	{
		Con_DPrintf("WARNING: UDP_OpenSocket: port %d setsockopt IP_MULTICAST_LOOP: %s\n", port, NET_ErrorString(NET_GetLastError()));
	}

#if !defined _WIN32 && defined REHLDS_FIXES
	int j = IP_PMTUDISC_DONT;
 	if (setsockopt(newsocket, IPPROTO_IP, IP_MTU_DISCOVER, (char *)&j, sizeof(j)) == SOCKET_ERROR)
	{
		Con_Printf("WARNING: UDP_OpenSocket: port %d  setsockopt IP_MTU_DISCOVER: %s\n", port, NET_ErrorString(NET_GetLastError()));
	}
#endif

	return newsocket;
}

void NET_OpenIP()
{
	//cvar_t *ip;//unused?
	int sv_port = 0;
	int cl_port = 0;
	//int mc_port;//unused?

	int dedicated = g_pcls.state == ca_dedicated;

	NET_ThreadLock();

	if (ip_sockets[NS_SERVER] == INV_SOCK)
	{
		int port = (int)iphostport.value;

		if (!NET_CheckPort(port))
		{
			port = (int)hostport.value;
			if (!NET_CheckPort(port))
			{
				port = (int)defport.value;
				hostport.value = defport.value;
			}
		}
		ip_sockets[NS_SERVER] = NET_IPSocket(ipname.string, port, FALSE);

		if (ip_sockets[NS_SERVER] == INV_SOCK && dedicated)
		{
			Sys_Error("%s: Couldn't allocate dedicated server IP port %d.", __func__, port);
		}
		sv_port = port;
	}

	NET_ThreadUnlock();

	if (dedicated)
		return;

	NET_ThreadLock();

	if (ip_sockets[NS_CLIENT] == INV_SOCK)
	{
		int port = (int)ip_clientport.value;

		if (!NET_CheckPort(port))
		{
			port = (int)clientport.value;
			if (!NET_CheckPort(port))
				port = -1;
		}
		ip_sockets[NS_CLIENT] = NET_IPSocket(ipname.string, port, FALSE);
		if (ip_sockets[NS_CLIENT] == INV_SOCK)
			ip_sockets[NS_CLIENT] = NET_IPSocket(ipname.string, -1, FALSE);
		cl_port = port;
	}

	if (ip_sockets[NS_MULTICAST] == INV_SOCK)
	{
		ip_sockets[NS_MULTICAST] = NET_IPSocket(ipname.string, multicastport.value, TRUE);
		if (ip_sockets[NS_MULTICAST] == INV_SOCK && !dedicated)
			Con_Printf("Warning! Couldn't allocate multicast IP port.\n");
	}

	NET_ThreadUnlock();

	static qboolean bFirst = TRUE;
	if (bFirst)
	{
		bFirst = FALSE;
		Con_Printf("NET Ports:  server %i, client %i\n", sv_port, cl_port);
	}
}

#ifdef _WIN32

SOCKET NET_IPXSocket(int hostshort)
{
	int err;
	u_long optval = 1;
	SOCKET newsocket;

	if((newsocket = CRehldsPlatformHolder::get()->socket(PF_IPX, SOCK_DGRAM, NSPROTO_IPX)) == INVALID_SOCKET)
	{
		err = NET_GetLastError();
		if (err != WSAEAFNOSUPPORT)
		{
			Con_Printf("WARNING: IPX_Socket: port: %d  socket: %s\n", hostshort, NET_ErrorString(err));
		}

		return INV_SOCK;
	}
	if (CRehldsPlatformHolder::get()->ioctlsocket(newsocket, FIONBIO, &optval) == SOCKET_ERROR)
	{
		err = NET_GetLastError();
		Con_Printf("WARNING: IPX_Socket: port: %d  ioctl FIONBIO: %s\n", hostshort, NET_ErrorString(err));
		return INV_SOCK;
	}
	if (CRehldsPlatformHolder::get()->setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (const char *)&optval, sizeof(optval)) == SOCKET_ERROR)
	{
		err = NET_GetLastError();
		Con_Printf("WARNING: IPX_Socket: port: %d  setsockopt SO_BROADCAST: %s\n", hostshort, NET_ErrorString(err));
		return INV_SOCK;
	}
	if (CRehldsPlatformHolder::get()->setsockopt(newsocket, SOL_SOCKET, 4, (const char *)&optval, sizeof(optval)) == SOCKET_ERROR)
	{
#ifndef REHLDS_FIXES
		err = NET_GetLastError();
#endif
		return INV_SOCK;
	}

	SOCKADDR_IPX address;
	address.sa_family = AF_IPX;
	Q_memset(address.sa_netnum, 0, 4);
	Q_memset(address.sa_nodenum, 0, 6);

	if (hostshort == -1)
		address.sa_socket = 0;
	else address.sa_socket = htons((u_short)hostshort);

	if (CRehldsPlatformHolder::get()->bind(newsocket, (struct sockaddr *)&address, sizeof(SOCKADDR_IPX)) == SOCKET_ERROR)
	{
		err = NET_GetLastError();
		Con_Printf("WARNING: IPX_Socket: port: %d  bind: %s\n", hostshort, NET_ErrorString(err));
		CRehldsPlatformHolder::get()->closesocket(newsocket);
		return INV_SOCK;
	}
	return newsocket;
}

void NET_OpenIPX()
{
	int dedicated = g_pcls.state == ca_dedicated;

	NET_ThreadLock();

	if (ipx_sockets[NS_SERVER] == INV_SOCK)
	{
		int port = ipx_hostport.value;
		if (!NET_CheckPort(port))
		{
			port = hostport.value;
			if (!NET_CheckPort(port))
			{
				hostport.value = defport.value;
				port = defport.value;
			}
		}
		ipx_sockets[NS_SERVER] = NET_IPXSocket(port);
	}

	NET_ThreadUnlock();

	if(dedicated)
		return;

	NET_ThreadLock();

	if (ipx_sockets[NS_CLIENT] == INV_SOCK)
	{
		int port = ipx_clientport.value;
		if (!NET_CheckPort(port))
		{
			port = clientport.value;
			if (!NET_CheckPort(port))
				port = -1;
		}
		ipx_sockets[NS_CLIENT] = NET_IPXSocket(port);

		if (ipx_sockets[NS_CLIENT] == INV_SOCK)
			ipx_sockets[NS_CLIENT] = NET_IPXSocket(-1);
	}

	NET_ThreadUnlock();
}

#endif // _WIN32

void NET_GetLocalAddress()
{
	char buff[512];
	struct sockaddr_in address;
	int namelen;
	int net_error;

	Q_memset(&net_local_adr, 0, sizeof(netadr_t));

#ifdef _WIN32
	Q_memset(&net_local_ipx_adr, 0, sizeof(netadr_t));
#endif // _WIN32

	if (noip)
	{
		Con_DPrintf("TCP/IP Disabled.\n");
	}
	else
	{
		if (Q_strcmp(ipname.string, "localhost"))
			Q_strncpy(buff, ipname.string,  ARRAYSIZE(buff) - 1);
		else
		{
#ifdef _WIN32
			CRehldsPlatformHolder::get()->gethostname(buff,  ARRAYSIZE(buff));
#else
			gethostname(buff, ARRAYSIZE(buff));
#endif // _WIN32
		}

		buff[ARRAYSIZE(buff) - 1] = 0;

#ifdef REHLDS_FIXES
		//check if address is valid
		if (NET_StringToAdr(buff, &net_local_adr))
		{
#else
		NET_StringToAdr(buff, &net_local_adr);
#endif
		namelen = sizeof(address);
#ifdef _WIN32
		if (CRehldsPlatformHolder::get()->getsockname(ip_sockets[NS_SERVER], (struct sockaddr *)&address, (socklen_t *)&namelen) == SOCKET_ERROR)
#else
		if (getsockname(ip_sockets[NS_SERVER], (struct sockaddr *)&address, (socklen_t *)&namelen) == SOCKET_ERROR)
#endif // _WIN32
		{
			noip = TRUE;
			net_error = NET_GetLastError();

			Con_Printf("Could not get TCP/IP address, TCP/IP disabled\nReason:  %s\n", NET_ErrorString(net_error));
		}
		else
		{
			net_local_adr.port = address.sin_port;
			Con_Printf("Server IP address %s\n", NET_AdrToString(net_local_adr));
			Cvar_Set("net_address", va(NET_AdrToString(net_local_adr)));
		}

#ifdef REHLDS_FIXES
		}
		else
		{
			Con_Printf("Could not get TCP/IP address, Invalid hostname '%s'\n", buff);
		}
#endif
	}

#ifdef _WIN32
	if (noipx)
	{
		Con_DPrintf("No IPX Support.\n");
	}
	else
	{
		namelen = 14;
		if (CRehldsPlatformHolder::get()->getsockname(ipx_sockets[NS_SERVER], (struct sockaddr *)&address, (socklen_t *)&namelen) == SOCKET_ERROR)
		{
			noipx = TRUE;
			net_error = NET_GetLastError();

#ifdef REHLDS_FIXES
			Con_Printf("Could not get IPX address, IPX disabled\nReason:  %s\n", NET_ErrorString(net_error));
#endif
		}
		else
		{
			SockadrToNetadr((struct sockaddr *)&address, &net_local_ipx_adr);
			Con_Printf("Server IPX address %s\n", NET_AdrToString(net_local_ipx_adr));
		}
	}
#endif //_WIN32
}

int NET_IsConfigured()
{
	return net_configured;
}

bool NET_CheckPort(int port)
{
#ifdef REHLDS_FIXES
	return port > 0 && port <= USHRT_MAX;
#else
	return port != 0;
#endif
}

void NET_Config(qboolean multiplayer)
{
	static qboolean old_config;

	if (old_config == multiplayer)
	{
		return;
	}

	old_config = multiplayer;

	if (multiplayer)
	{
		if (!noip)
			NET_OpenIP();
#ifdef _WIN32
		if (!noipx)
			NET_OpenIPX();
#endif //_WIN32

		static qboolean bFirst = TRUE;
		if (bFirst)
		{
			bFirst = FALSE;
			NET_GetLocalAddress();
		}
	}
	else
	{
		NET_ThreadLock();

		for (int sock = 0; sock < NS_MAX; sock++)
		{
			if (ip_sockets[sock] != INV_SOCK)
			{
#ifdef _WIN32
				CRehldsPlatformHolder::get()->closesocket(ip_sockets[sock]);
#else //_WIN32
				SOCKET_CLOSE(ip_sockets[sock]);
#endif //_WIN32
				ip_sockets[sock] = INV_SOCK;
			}

#ifdef _WIN32
			if (ipx_sockets[sock] != INV_SOCK)
			{
				CRehldsPlatformHolder::get()->closesocket(ipx_sockets[sock]);
				ipx_sockets[sock] = INV_SOCK;
			}
#endif //_WIN32
		}

		NET_ThreadUnlock();
	}

	net_configured = multiplayer ? 1 : 0;
}

void MaxPlayers_f()
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf("\"maxplayers\" is \"%u\"\n", g_psvs.maxclients);
		return;

	}

	if (g_psv.active)
	{
		Con_Printf("maxplayers cannot be changed while a server is running.\n");
		return;
	}


	int n = Q_atoi(Cmd_Argv(1));
	if (n < 1)
		n = 1;

	if (n > g_psvs.maxclientslimit)
	{
		n = g_psvs.maxclientslimit;
		Con_Printf("\"maxplayers\" set to \"%u\"\n", g_psvs.maxclientslimit);
	}
	g_psvs.maxclients = n;

	if (n == 1)
		Cvar_Set("deathmatch", "0");
	else
		Cvar_Set("deathmatch", "1");
}

void NET_Init()
{
	Cmd_AddCommand("maxplayers", MaxPlayers_f);

	Cvar_RegisterVariable(&net_address);
	Cvar_RegisterVariable(&ipname);
	Cvar_RegisterVariable(&iphostport);
	Cvar_RegisterVariable(&hostport);
	Cvar_RegisterVariable(&defport);
	Cvar_RegisterVariable(&ip_clientport);
	Cvar_RegisterVariable(&clientport);
	Cvar_RegisterVariable(&clockwindow);
	Cvar_RegisterVariable(&multicastport);
#ifdef _WIN32
	Cvar_RegisterVariable(&ipx_hostport);
	Cvar_RegisterVariable(&ipx_clientport);
#endif // _WIN32
	Cvar_RegisterVariable(&fakelag);
	Cvar_RegisterVariable(&fakeloss);
	Cvar_RegisterVariable(&net_graph);
	Cvar_RegisterVariable(&net_graphwidth);
	Cvar_RegisterVariable(&net_scale);
	Cvar_RegisterVariable(&net_graphpos);

	if (COM_CheckParm("-netthread"))
		use_thread = TRUE;

	if (COM_CheckParm("-netsleep"))
		net_sleepforever = 0;

#ifdef _WIN32
	if (COM_CheckParm("-noipx"))
		noipx = TRUE;
#endif // _WIN32

	if (COM_CheckParm("-noip"))
		noip = TRUE;

	int port = COM_CheckParm("-port");
	if (port)
		Cvar_SetValue("hostport", Q_atof(com_argv[port + 1]));

	int clockwindow_ = COM_CheckParm("-clockwindow");
	if (clockwindow_)
		Cvar_SetValue("clockwindow", Q_atof(com_argv[clockwindow_ + 1]));

	net_message.data = (byte *)&net_message_buffer;
	net_message.maxsize = sizeof(net_message_buffer);
	net_message.flags = 0;
	net_message.buffername = "net_message";

	in_message.data = (byte *)&in_message_buf;
	in_message.maxsize = sizeof(in_message_buf);
	in_message.flags = 0;
	in_message.buffername = "in_message";

	for (int i = 0; i < NS_MAX; i++)
	{
		g_pLagData[i].pPrev = &g_pLagData[i];
		g_pLagData[i].pNext = &g_pLagData[i];
	}

	NET_AllocateQueues();
	Con_DPrintf("Base networking initialized.\n");
}

void NET_ClearLagData(qboolean bClient, qboolean bServer)
{
	NET_ThreadLock();

	if (bClient)
	{
		NET_ClearLaggedList(&g_pLagData[0]);
		NET_ClearLaggedList(&g_pLagData[2]);
	}

	if (bServer)
	{
		NET_ClearLaggedList(&g_pLagData[1]);
	}

	NET_ThreadUnlock();
}

void NET_Shutdown()
{
	NET_ThreadLock();

	NET_ClearLaggedList(g_pLagData);
	NET_ClearLaggedList(&g_pLagData[1]);

	NET_ThreadUnlock();

	NET_Config(FALSE);
	NET_FlushQueues();
}

NOXREF qboolean NET_JoinGroup(netsrc_t sock, netadr_t& addr)
{
	NOXREFCHECK;

	ip_mreq mreq;
	SIN_SET_ADDR(&mreq.imr_multiaddr, *(unsigned int*)&addr.ip[0]);
	SIN_SET_ADDR(&mreq.imr_interface, 0);

	SOCKET net_socket = ip_sockets[sock];
	if (CRehldsPlatformHolder::get()->setsockopt(net_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) == SOCKET_ERROR)
	{
		int err = NET_GetLastError();
		if (err != WSAEAFNOSUPPORT)
		{
			Con_Printf("WARNING: NET_JoinGroup: IP_ADD_MEMBERSHIP: %s", NET_ErrorString(err));
		}
		return FALSE;
	}

	return TRUE;
}

NOXREF qboolean NET_LeaveGroup(netsrc_t sock, netadr_t& addr)
{
	NOXREFCHECK;

	ip_mreq mreq;
	SIN_SET_ADDR(&mreq.imr_multiaddr, *(unsigned int*)&addr.ip[0]);
	SIN_SET_ADDR(&mreq.imr_interface, 0);

	SOCKET net_socket = ip_sockets[sock];
	if (CRehldsPlatformHolder::get()->setsockopt(net_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) != SOCKET_ERROR)
	{
		if (NET_GetLastError() != WSAEAFNOSUPPORT)
		{
			return FALSE;
		}
	}

	return TRUE;
}
