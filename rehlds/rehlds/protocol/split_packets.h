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

#include <queue>
#include <vector>

#ifndef _WIN32
#include <pthread.h>
#endif

extern cvar_t sv_rehlds_splitrate;
extern cvar_t sv_rehlds_maxroutable;

const int NET_HEADER_FLAG_SPLITPACKET_EXT = -3;

// Header of every part of a packet sent to a client that agreed on PROTOCOL_EXT_PACKETSIZE,
// the SPLITPACKET of Source. All parts of one packet carry the same sequence number and split size.
#pragma pack(push, 1)
typedef struct SPLITPACKET_EXT_t
{
	int netID;
	int sequenceNumber;
	uint16 packetID;	// part number in the high byte, part count in the low one
	uint16 nSplitSize;	// packet bytes in every part but the last one
} SPLITPACKET_EXT;
#pragma pack(pop)

// UDP payload of one part, the header included
const int MIN_USER_MAXROUTABLE_SIZE = 576;
const int MAX_USER_MAXROUTABLE_SIZE = MAX_ROUTEABLE_PACKET;

const int MIN_SPLIT_SIZE = MIN_USER_MAXROUTABLE_SIZE - (int)sizeof(SPLITPACKET_EXT);
const int MAX_SPLIT_SIZE = MAX_USER_MAXROUTABLE_SIZE - (int)sizeof(SPLITPACKET_EXT);

// The part count takes a byte of packetID
const int MAX_SPLITPACKET_SPLITS = 255;

// The largest packet sent to a client that agreed on PROTOCOL_EXT_PACKETSIZE
const int NET_MAX_EXT_PACKET = 131072;

static_assert(NET_MAX_EXT_PACKET <= MAX_SPLITPACKET_SPLITS * MIN_SPLIT_SIZE, "the smallest parts have to carry the largest packet");

// Sends the parts of split packets that are not due yet from its own thread, each at its time.
// The thread starts with the first packet queued, so a server without extended clients never runs it.
class CQueuedPacketSender
{
public:
	CQueuedPacketSender();
	virtual ~CQueuedPacketSender();

	void QueuePacket(const void *channel, SOCKET s, const struct sockaddr &to, const byte *data, int length, uint32 msecDelay);

	// Drops what is still queued for the channel
	void ClearQueuedPackets(const void *channel);

	// Sends at once and in order what is still queued for the channel
	void FlushQueuedPackets(const void *channel);

	// Sends every queued packet that is due; returns milliseconds until the next one is
	uint32 SendDuePackets();

	void Shutdown();

protected:
	// Milliseconds of Sys_FloatTime; wraps around, compared by difference
	virtual uint32 GetTime() const;

	// A sender driven by SendDuePackets alone, without the thread
	explicit CQueuedPacketSender(bool bUseThread);

private:
	void StartThread();
	void Lock();
	void Unlock();

	// Waits at most msecWait or until a packet is queued; called with the lock held
	void Wait(uint32 msecWait);
	void Wake();

#ifdef _WIN32
	static DWORD WINAPI ThreadMain(LPVOID param);
#else
	static void *ThreadMain(void *param);
#endif

	struct queued_packet_t
	{
		uint32 sendTime;
		const void *channel;	// an identifier, never dereferenced
		SOCKET s;
		struct sockaddr to;
		int length;
		byte data[MAX_USER_MAXROUTABLE_SIZE];
	};

	struct later_t
	{
		bool operator()(const queued_packet_t *lhs, const queued_packet_t *rhs) const { return lhs->sendTime > rhs->sendTime; }
	};

	void Run();

	std::priority_queue<queued_packet_t *, std::vector<queued_packet_t *>, later_t> m_QueuedPackets;

#ifdef _WIN32
	CRITICAL_SECTION m_QueuedPacketsCS;
	HANDLE m_hThreadEvent;	// auto-reset: a packet queued while the thread sends is not missed
	HANDLE m_hThread;
#else
	pthread_mutex_t m_QueuedPacketsCS;
	pthread_cond_t m_ThreadEvent;
	pthread_t m_Thread;
	bool m_bPacketQueued;	// the auto-reset event of Windows: stays set until the thread looks
#endif

	bool m_bUseThread;
	bool m_bThreadRunning;
	bool m_bThreadShouldExit;
};

extern CQueuedPacketSender g_QueuedPacketSender;

// The split packets of one client, NET_SendLong of Source: the first sv_rehlds_splitrate parts go at once,
// part n goes n part times after the packet at the rate of the client, so that a large packet does not
// overrun the receive buffer of the client or a router on the way
class CSplitPacketSender
{
public:
	explicit CSplitPacketSender(CQueuedPacketSender &queue = g_QueuedPacketSender);
	~CSplitPacketSender();

	// Returns the bytes sent, the headers of the parts and their UDP headers included
	int Send(netsrc_t sock, const netadr_t &to, const byte *data, int length, double rate);

	// UDP payload of one part, the header included; a packet above it goes in parts
	int GetMaxRoutable() const { return m_MaxRoutable; }
	void SetMaxRoutable(int maxRoutable) { m_MaxRoutable = maxRoutable; }

	// Drops the queued parts; the sequence number keeps counting, so parts of a packet
	// from before a reconnect never match a new packet
	void Clear();

private:
	CQueuedPacketSender &m_Queue;
	int m_SequenceNumber;
	int m_MaxRoutable;
};
