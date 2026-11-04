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

// Parts of packets sent to clients with PROTOCOL_EXT_PACKETSIZE: parts that go at once,
// and the server bound of the part size a client asks for with "proute"
cvar_t sv_rehlds_splitrate = { "sv_rehlds_splitrate", "1", 0, 1.0f, NULL };
cvar_t sv_rehlds_maxroutable = { "sv_rehlds_maxroutable", "1200", 0, 1200.0f, NULL };

// Past this the queue refuses packets, whatever client they are for
const int MAX_QUEUED_PACKETS = 1024;

// With nothing queued the thread still looks at the queue this often
const uint32 QUEUE_IDLE_WAIT = 500;

CQueuedPacketSender g_QueuedPacketSender;

CQueuedPacketSender::CQueuedPacketSender() : CQueuedPacketSender(true)
{
}

CQueuedPacketSender::CQueuedPacketSender(bool bUseThread)
{
	m_bUseThread = bUseThread;
	m_bThreadRunning = false;
	m_bThreadShouldExit = false;

#ifdef _WIN32
	InitializeCriticalSection(&m_QueuedPacketsCS);
	m_hThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hThread = NULL;
#else
	pthread_mutex_init(&m_QueuedPacketsCS, NULL);
	pthread_cond_init(&m_ThreadEvent, NULL);
	m_bPacketQueued = false;
#endif
}

CQueuedPacketSender::~CQueuedPacketSender()
{
	Shutdown();

	while (!m_QueuedPackets.empty())
	{
		delete m_QueuedPackets.top();
		m_QueuedPackets.pop();
	}

#ifdef _WIN32
	CloseHandle(m_hThreadEvent);
	DeleteCriticalSection(&m_QueuedPacketsCS);
#else
	pthread_cond_destroy(&m_ThreadEvent);
	pthread_mutex_destroy(&m_QueuedPacketsCS);
#endif
}

void CQueuedPacketSender::QueuePacket(const void *channel, SOCKET s, const struct sockaddr &to, const byte *data, int length, uint32 msecDelay)
{
	if (length <= 0 || length > MAX_USER_MAXROUTABLE_SIZE)
		return;

	queued_packet_t *packet = new queued_packet_t;
	packet->sendTime = GetTime() + msecDelay;
	packet->channel = channel;
	packet->s = s;
	packet->to = to;
	packet->length = length;
	Q_memcpy(packet->data, data, length);

	Lock();

	if ((int)m_QueuedPackets.size() >= MAX_QUEUED_PACKETS)
	{
		Unlock();
		delete packet;

		static int warnings = 5;
		if (--warnings > 0)
			Con_Printf("%s: %i packets queued, not queueing more\n", __func__, MAX_QUEUED_PACKETS);

		return;
	}

	m_QueuedPackets.push(packet);

	if (m_bUseThread && !m_bThreadRunning)
		StartThread();

	Wake();
	Unlock();
}

void CQueuedPacketSender::ClearQueuedPackets(const void *channel)
{
	Lock();

	std::vector<queued_packet_t *> kept;
	while (!m_QueuedPackets.empty())
	{
		queued_packet_t *packet = m_QueuedPackets.top();
		m_QueuedPackets.pop();

		if (packet->channel == channel)
			delete packet;
		else
			kept.push_back(packet);
	}

	for (queued_packet_t *packet : kept)
		m_QueuedPackets.push(packet);

	Unlock();
}

void CQueuedPacketSender::FlushQueuedPackets(const void *channel)
{
	Lock();

	std::vector<queued_packet_t *> kept;
	while (!m_QueuedPackets.empty())
	{
		queued_packet_t *packet = m_QueuedPackets.top();
		m_QueuedPackets.pop();

		if (packet->channel != channel)
		{
			kept.push_back(packet);
			continue;
		}

		CRehldsPlatformHolder::get()->sendto(packet->s, (const char *)packet->data, packet->length, 0, &packet->to, sizeof(packet->to));
		delete packet;
	}

	for (queued_packet_t *packet : kept)
		m_QueuedPackets.push(packet);

	Unlock();
}

uint32 CQueuedPacketSender::SendDuePackets()
{
	uint32 waitInterval = QUEUE_IDLE_WAIT;

	Lock();

	const uint32 now = GetTime();
	while (!m_QueuedPackets.empty())
	{
		queued_packet_t *packet = m_QueuedPackets.top();
		if ((int32)(packet->sendTime - now) > 0)
		{
			waitInterval = packet->sendTime - now;
			break;
		}

		m_QueuedPackets.pop();
		CRehldsPlatformHolder::get()->sendto(packet->s, (const char *)packet->data, packet->length, 0, &packet->to, sizeof(packet->to));
		delete packet;
	}

	Unlock();
	return waitInterval;
}

void CQueuedPacketSender::Shutdown()
{
	Lock();
	if (!m_bThreadRunning)
	{
		Unlock();
		return;
	}

	m_bThreadShouldExit = true;
	Wake();
	Unlock();

#ifdef _WIN32
	WaitForSingleObject(m_hThread, INFINITE);
	CloseHandle(m_hThread);
	m_hThread = NULL;
#else
	pthread_join(m_Thread, NULL);
#endif

	m_bThreadRunning = false;
	m_bThreadShouldExit = false;
}

// Sys_FloatTime takes its lock on Windows, so the thread may call it too
uint32 CQueuedPacketSender::GetTime() const
{
	return (uint32)(Sys_FloatTime() * 1000.0);
}

void CQueuedPacketSender::Run()
{
	uint32 waitInterval = QUEUE_IDLE_WAIT;
	while (true)
	{
		Lock();
		Wait(waitInterval);
		const bool bExit = m_bThreadShouldExit;
		Unlock();

		if (bExit)
			return;

		waitInterval = SendDuePackets();
	}
}

void CQueuedPacketSender::StartThread()
{
#ifdef _WIN32
	m_hThread = CreateThread(NULL, 0, ThreadMain, this, 0, NULL);
	m_bThreadRunning = (m_hThread != NULL);
#else
	m_bThreadRunning = (pthread_create(&m_Thread, NULL, ThreadMain, this) == 0);
#endif

	if (!m_bThreadRunning)
	{
		Con_Printf("%s: couldn't start the thread, split packets go without their delays\n", __func__);
		m_bUseThread = false;
	}
}

#ifdef _WIN32

void CQueuedPacketSender::Lock()
{
	EnterCriticalSection(&m_QueuedPacketsCS);
}

void CQueuedPacketSender::Unlock()
{
	LeaveCriticalSection(&m_QueuedPacketsCS);
}

void CQueuedPacketSender::Wait(uint32 msecWait)
{
	Unlock();
	WaitForSingleObject(m_hThreadEvent, msecWait);
	Lock();
}

void CQueuedPacketSender::Wake()
{
	SetEvent(m_hThreadEvent);
}

DWORD WINAPI CQueuedPacketSender::ThreadMain(LPVOID param)
{
	((CQueuedPacketSender *)param)->Run();
	return 0;
}

#else // _WIN32

void CQueuedPacketSender::Lock()
{
	pthread_mutex_lock(&m_QueuedPacketsCS);
}

void CQueuedPacketSender::Unlock()
{
	pthread_mutex_unlock(&m_QueuedPacketsCS);
}

void CQueuedPacketSender::Wait(uint32 msecWait)
{
	if (!m_bPacketQueued && !m_bThreadShouldExit)
	{
		struct timespec deadline;
		clock_gettime(CLOCK_REALTIME, &deadline);
		deadline.tv_sec += msecWait / 1000;
		deadline.tv_nsec += (msecWait % 1000) * 1000000;
		if (deadline.tv_nsec >= 1000000000)
		{
			deadline.tv_sec++;
			deadline.tv_nsec -= 1000000000;
		}

		pthread_cond_timedwait(&m_ThreadEvent, &m_QueuedPacketsCS, &deadline);
	}

	m_bPacketQueued = false;
}

void CQueuedPacketSender::Wake()
{
	m_bPacketQueued = true;
	pthread_cond_signal(&m_ThreadEvent);
}

void *CQueuedPacketSender::ThreadMain(void *param)
{
	((CQueuedPacketSender *)param)->Run();
	return NULL;
}

#endif // _WIN32

CSplitPacketSender::CSplitPacketSender(CQueuedPacketSender &queue) : m_Queue(queue)
{
	m_SequenceNumber = 0;
	m_MaxRoutable = MAX_USER_MAXROUTABLE_SIZE;
}

CSplitPacketSender::~CSplitPacketSender()
{
	Clear();
}

int CSplitPacketSender::Send(netsrc_t sock, const netadr_t &to, const byte *data, int length, double rate)
{
	const int splitSize = m_MaxRoutable - sizeof(SPLITPACKET_EXT);
	const int packetCount = (length + splitSize - 1) / splitSize;

	if (packetCount > MAX_SPLITPACKET_SPLITS)
	{
		Con_Printf("%s: packet of %i bytes needs more than %i parts, dropped\n", __func__, length, MAX_SPLITPACKET_SPLITS);
		return length + UDP_HEADER_SIZE;
	}

	// The client assembles one packet at a time and drops it when parts of the next one come: what is
	// left of the previous packet goes first. Netchan holds the next packet until the last part is due,
	// so this is only for a thread that wakes late
	m_Queue.FlushQueuedPackets(this);

	if (++m_SequenceNumber <= 0)
		m_SequenceNumber = 1;

	struct sockaddr addr;
	NetadrToSockadr(&to, &addr);

	byte packet[MAX_USER_MAXROUTABLE_SIZE];
	SPLITPACKET_EXT *header = (SPLITPACKET_EXT *)packet;
	header->netID = NET_HEADER_FLAG_SPLITPACKET_EXT;
	header->sequenceNumber = m_SequenceNumber;
	header->nSplitSize = splitSize;

	const int splitRate = (int)sv_rehlds_splitrate.value;

	for (int packetNumber = 0; packetNumber < packetCount; packetNumber++)
	{
		const int size = Q_min(splitSize, length - packetNumber * splitSize);

		header->packetID = (packetNumber << 8) + packetCount;
		Q_memcpy(packet + sizeof(SPLITPACKET_EXT), data + packetNumber * splitSize, size);

		if (net_showpackets.value == 4.0f)
			Con_DPrintf("Sending split %i of %i with %i bytes and seq %i to %s\n", packetNumber + 1, packetCount, size, m_SequenceNumber, NET_AdrToString(to));

		if (packetNumber < splitRate || rate <= 0.0)
		{
			NET_SendPacket(sock, size + sizeof(SPLITPACKET_EXT), packet, to);
			continue;
		}

		// When this part is due, measured from now
		const uint32 delay = (uint32)(1000.0 * packetNumber * (m_MaxRoutable + UDP_HEADER_SIZE) / rate + 0.5);
		m_Queue.QueuePacket(this, ip_sockets[sock], addr, packet, size + sizeof(SPLITPACKET_EXT), delay);
	}

	// Netchan counts the time of the packet by this: in Source it counts the packet alone, so a packet
	// in parts lets the next one go before its last parts, and the client drops the packet it was assembling
	return length + packetCount * (sizeof(SPLITPACKET_EXT) + UDP_HEADER_SIZE);
}

void CSplitPacketSender::Clear()
{
	m_Queue.ClearQueuedPackets(this);
}
