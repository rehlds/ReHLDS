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

NetChannel::NetChannel()
{
	for (int i = 0; i < MAX_STREAMS; i++)
	{
		m_waitlist[i] = nullptr;
		m_fragbufs[i] = nullptr;
		m_incomingbufs[i] = nullptr;

		m_frag_length[i] = 0;
		m_frag_startpos[i] = 0;
		m_fragbufcount[i] = 0;

		m_reliable_fragid[i] = 0;
		m_reliable_fragment[i] = 0;
	}

	m_Socket = nullptr;
	m_tempBuffer = nullptr;
}

void NetChannel::UnlinkFragment(fragbuf_t *buf, int stream)
{
	fragbuf_t *search;
	fragbuf_t **list = &m_fragbufs[stream];

	if (list == nullptr)
	{
		m_System->DPrintf("Netchan_UnlinkFragment: Asked to unlink fragment from empty list, ignored\n");
		return;
	}

	if (*list == buf)
	{
		*list = buf->next;
		Mem_Free(buf);
		return;
	}

	search = *list;
	while (search->next)
	{
		if (search->next == buf)
		{
			search->next = buf->next;
			Mem_Free(buf);
			return;
		}

		search = search->next;
	}

	m_System->DPrintf("Netchan_UnlinkFragment: Couldn't find fragment\n");
}

void NetChannel::OutOfBandPrintf(const char *format, ...)
{
	va_list argptr;
	char string[NET_MAX_MESSAGE];
	BitBuffer data(string, sizeof(string));

	if (m_Socket)
	{
		*(int *)string = CONNECTIONLESS_HEADER;

		va_start(argptr, format);
		Q_vsnprintf(&string[4], sizeof(string) - 4, format, argptr);
		va_end(argptr);

		data.SkipBytes(Q_strlen(string));
		m_Socket->SendPacket(&m_remote_address, data.GetData(), data.CurrentSize());
	}
}

void NetChannel::ClearFragbufs(fragbuf_t **ppbuf)
{
	fragbuf_t *buf, *n;

	if (!ppbuf)
	{
		return;
	}

	// Throw away any that are sitting around
	buf = *ppbuf;
	while (buf)
	{
		n = buf->next;
		Mem_Free(buf);
		buf = n;
	}

	*ppbuf = nullptr;
}

void NetChannel::ClearFragments()
{
	fragbufwaiting_t *wait, *next;
	for (int i = 0; i < MAX_STREAMS; i++)
	{
		wait = m_waitlist[i];
		while (wait)
		{
			next = wait->next;
			ClearFragbufs(&wait->fragbufs);
			Mem_Free(wait);
			wait = next;
		}
		m_waitlist[i] = nullptr;

		ClearFragbufs(&m_fragbufs[i]);
		FlushIncoming(i);
	}
}

void NetChannel::FlushIncoming(int stream)
{
	fragbuf_t *p, *n;

	NetPacket *in;
	while ((in = (NetPacket *)m_incomingPackets.RemoveTail())) {
		FreePacket(in);
	}

	p = m_incomingbufs[stream];
	while (p)
	{
		n = p->next;
		Mem_Free(p);
		p = n;
	}

	m_incomingbufs[stream] = nullptr;
}

void NetChannel::Reset()
{
	m_keep_alive = true;
	m_crashed = false;
	m_connected = false;

	m_connect_time = m_System->GetTime();

	SetTimeOut(30);
	SetRate(10000);
	SetUpdateRate(20);

	m_incoming_sequence = 0;
	m_incoming_acknowledged = 0;
	m_incoming_reliable_acknowledged = 0;
	m_incoming_reliable_sequence = 0;
	m_outgoing_sequence = 1;
	m_reliable_sequence = 0;
	m_last_reliable_sequence = 0;
}

void NetChannel::Clear()
{
	NetPacket *packet;
	while ((packet = (NetPacket *)m_incomingPackets.RemoveHead())) {
		FreePacket(packet);
	}

	ClearFragments();

	for (int i = 0; i < MAX_STREAMS; i++)
	{
		m_reliable_fragid[i] = 0;
		m_reliable_fragment[i] = 0;
		m_fragbufcount[i] = 0;
		m_frag_startpos[i] = 0;
		m_frag_length[i] = 0;
	}

	if (m_tempBuffer)
	{
		Mem_Free(m_tempBuffer);
		m_tempBuffer = nullptr;
	}

	m_tempBufferSize = 0;
	m_reliableOutSize = 0;

	Q_memset(m_reliableOutBuffer, 0, sizeof(m_reliableOutBuffer));
	Q_memset(m_flow, 0, sizeof(m_flow));

	m_reliableStream.Clear();
	m_unreliableStream.Clear();

	m_last_send =
	m_last_received =
		m_cleartime = m_System->GetTime();

	m_loss = 0;
}

bool NetChannel::Create(IBaseSystem *system, INetSocket *netsocket, NetAddress *adr)
{
	m_System = system;
	m_incomingPackets.Init();
	m_blocksize = FRAGMENT_S2C_MAX_SIZE;

	if (!m_reliableStream.Resize(MAX_MSGLEN))
	{
		m_System->Errorf("NetChannel::Create: m_reliableStream out of memory.\n");
		return false;
	}

	if (!m_unreliableStream.Resize(MAX_MSGLEN))
	{
		m_System->Errorf("NetChannel::Create: m_unreliableStream out of memory.\n");
		return false;
	}

	m_remote_address.FromNetAddress(adr);
	m_Socket = netsocket;
	if (m_Socket)
	{
		if (!m_Socket->AddChannel(this))
		{
			m_System->Errorf("NetChannel::Create: could not bound to NetSocket\n");
			return false;
		}
	}
	else
	{
		m_System->DPrintf("Creating fake network channel.\n");
	}

	Clear();
	Reset();

	return true;
}

void NetChannel::Close()
{
	if (m_Socket) {
		m_Socket->RemoveChannel(this);
	}

	Clear();
	m_reliableStream.Free();
	m_unreliableStream.Free();
}

bool NetChannel::IsReadyToSend()
{
	if (m_System->GetTime() > m_cleartime) {
		return true;
	}

	return false;
}

void NetChannel::UpdateFlow(int stream)
{
	int bytes = 0;
	float faccumulatedtime = 0.0f;
	flow_t *pflow = &m_flow[stream];

	if (pflow->nextcompute > m_System->GetTime()) {
		return;
	}

	pflow->nextcompute = m_System->GetTime() + 0.5f;

	flowstats_t *pstat;
	int start = pflow->current - 1;
	for (int i = 1; i < (MAX_LATENT / 2); i++)
	{
		pstat = &pflow->stats[(start - i) & 0x1f];
		bytes += pstat->size;
	}

	faccumulatedtime = m_System->GetTime() - pstat->time;

	pflow->kbytespersec = (faccumulatedtime == 0.0f) ? 0.0f : bytes / faccumulatedtime / 1024.0f;
	pflow->avgkbytespersec = pflow->avgkbytespersec * (2.0 / 3) + pflow->kbytespersec * (1.0 / 3);
}

void NetChannel::TransmitOutgoing()
{
#ifdef HLTV_FIXES
	byte send_buf[MAX_UDP_PACKET];
#else
	byte send_buf[NET_MAX_MESSAGE];
#endif

	BitBuffer data(send_buf, sizeof(send_buf));

	bool send_reliable;
	bool send_reliable_fragment;
	bool send_resending = false;
	unsigned w1, w2;
	int i, j;

	if (IsFakeChannel())
	{
		m_outgoing_sequence++;

		m_last_send = m_System->GetTime();
		m_cleartime = m_last_send + m_send_interval;

		m_reliableStream.FastClear();
		m_unreliableStream.FastClear();

		FakeAcknowledgement();
		return;
	}

	// check for reliable message overflow
	if (m_reliableStream.IsOverflowed())
	{
		m_System->DPrintf("NetChannel::Transmit:Outgoing m_reliableStream overflow (%s)\n", m_remote_address.ToString());
		m_reliableStream.Clear();
		return;
	}

	// check for unreliable message overflow
	if (m_unreliableStream.IsOverflowed())
	{
		m_System->DPrintf("NetChannel::Transmit:Outgoing m_unreliableStream overflow (%s)\n", m_remote_address.ToString());
		m_unreliableStream.Clear();
	}

	// if the remote side dropped the last reliable message, resend it
	send_reliable = false;

	if (m_incoming_acknowledged > m_last_reliable_sequence && m_incoming_reliable_acknowledged != m_reliable_sequence)
	{
		send_reliable = true;
		send_resending = true;
	}

	// A packet can have "reliable payload + frag payload + unreliable payload
	// frag payload can be a file chunk, if so, it needs to be parsed on the receiving end and reliable payload + unreliable payload need
	// to be passed on to the message queue. The processing routine needs to be able to handle the case where a message comes in and a file
	// transfer completes
	//
	// if the reliable transmit buffer is empty, copy the current message out
	if (!m_reliableOutSize)
	{
		bool send_frag = false;
		fragbuf_t *pbuf;

		// Will be true if we are active and should let chan->message get some bandwidth
		int send_from_frag[MAX_STREAMS] = { 0, 0 };
		int send_from_regular = 0;

		// If we have data in the waiting list(s) and we have cleared the current queue(s), then
		// push the m_waitlist(s) into the current queue(s)
		FragSend();

		// Sending regular payload
		send_from_regular = m_reliableStream.CurrentSize() ? 1 : 0;

		// Check to see if we are sending a frag payload
		for (i = 0; i < MAX_STREAMS; i++)
		{
			if (m_fragbufs[i])
			{
				send_from_frag[i] = 1;
			}
		}

		/*if (m_reliableStream.CurrentSize() > sizeof(send_buf))
		{
			CreateFragmentsFromBuffer(m_reliableStream.GetData(), m_reliableStream.CurrentSize(), FRAG_NORMAL_STREAM);
			m_reliableStream.FastClear();
		}*/

		// Stall reliable payloads if sending from frag buffer
		if (send_from_regular && (send_from_frag[FRAG_NORMAL_STREAM]))
		{
			send_from_regular = false;

			// If the reliable buffer has gotten too big, queue it at the end of everything and clear out buffer
			if (m_reliableStream.CurrentSize() > MAX_RELIABLE_PAYLOAD)
			{
				CreateFragmentsFromBuffer(m_reliableStream.GetData(), m_reliableStream.CurrentSize(), FRAG_NORMAL_STREAM);
				m_reliableStream.FastClear();
			}
		}

		// Startpos will be zero if there is no regular payload
		for (i = 0; i < MAX_STREAMS; i++)
		{
			m_frag_startpos[i] = 0;

			// Assume no fragment is being sent
			m_reliable_fragment[i] = 0;
			m_reliable_fragid[i] = 0;
			m_frag_length[i] = 0;

			if (send_from_frag[i])
			{
				send_frag = true;
			}
		}

		if (send_from_regular || send_frag)
		{
			m_reliable_sequence ^= 1u;
			send_reliable = true;
		}

		if (send_from_regular)
		{
			Q_memcpy(m_reliableOutBuffer, m_reliableStream.GetData(), m_reliableStream.CurrentSize());

			m_reliableOutSize = m_reliableStream.CurrentSize();
			m_reliableStream.FastClear();

			// If we send fragments, this is where they'll start
			for (i = 0; i < MAX_STREAMS; i++) {
				m_frag_startpos[i] = m_reliableOutSize;
			}
		}

		for (i = 0; i < MAX_STREAMS; i++)
		{
			int fragment_size = 0;

			// Is there something in the fragbuf?
			pbuf = m_fragbufs[i];
			if (pbuf)
			{
				fragment_size = pbuf->size;

				// Files set size a bit differently.
				if (pbuf->isfile && !pbuf->isbuffer)
				{
					fragment_size = pbuf->size;
				}
			}

			// Make sure we have enought space left
			if (send_from_frag[i] && pbuf && (m_reliableOutSize + fragment_size) < MAX_RELIABLE_PAYLOAD)
			{
				m_reliable_fragid[i] = MAKE_FRAGID(pbuf->bufferId, m_fragbufcount[i]); // Which buffer are we sending?

				// If it's not in-memory, then we'll need to copy it in frame the file handle.
				if (pbuf->isfile && !pbuf->isbuffer) {
					m_System->Printf("TODO! NetChannel::Transmit: system file support\n");
				}

				Q_memcpy(m_reliableOutBuffer + m_reliableOutSize, pbuf->data, pbuf->size);

				m_reliableOutSize += pbuf->size;
				m_frag_length[i] = pbuf->size;

				// Unlink pbuf
				UnlinkFragment(pbuf, i);
				m_reliable_fragment[i] = 1;

				// Offset the rest of the starting positions
				for (j = i + 1; j < MAX_STREAMS; j++)
				{
					m_frag_startpos[j] += m_frag_length[i];
				}
			}
		}
	}

	// Prepare the packet header
	w1 = m_outgoing_sequence | (send_reliable << 31);
	w2 = m_incoming_sequence | (m_incoming_reliable_sequence << 31);

	send_reliable_fragment = false;

	for (i = 0; i < MAX_STREAMS; i++)
	{
		if (m_reliable_fragment[i])
		{
			send_reliable_fragment = true;
			break;
		}
	}

	if (send_reliable && send_reliable_fragment) {
		w1 |= (1 << 30);
	}

	m_outgoing_sequence++;

	data.Clear();
	data.WriteLong(w1);
	data.WriteLong(w2);

	if (send_reliable && send_reliable_fragment)
	{
		for (i = 0; i < MAX_STREAMS; i++)
		{
			if (m_reliable_fragment[i])
			{
				data.WriteByte(1);
				data.WriteLong(m_reliable_fragid[i]);
				data.WriteShort(m_frag_startpos[i]);
				data.WriteShort(m_frag_length[i]);
			}
			else
			{
				data.WriteByte(0);
			}
		}
	}

	// Copy the reliable message to the packet first
	if (send_reliable) {
		data.WriteBuf(m_reliableOutBuffer, m_reliableOutSize);
		m_last_reliable_sequence = m_outgoing_sequence - 1;
	}

	// Is there room for the unreliable payload?
	int max_send_size = send_resending ? MAX_ROUTEABLE_PACKET : NET_MAX_MESSAGE;
	if ((max_send_size - data.CurrentSize()) >= m_unreliableStream.CurrentSize()) {
		data.ConcatBuffer(&m_unreliableStream);
	}
	else {
		m_System->DPrintf("WARNING! TransmitOutgoing: Unreliable would overfow, ignoring.\n");
	}

	m_unreliableStream.FastClear();

	// Deal with packets that are too small for some networks
	// Packet too small for some networks
	if (data.CurrentSize() < 16)
	{
		// Go ahead and pad a full 16 extra bytes -- this only happens during authentication / signon
		for (int i = data.CurrentSize(); i < 16; i++)
		{
			// Note that the server can parse svc_nop, too.
			data.WriteByte(svc_nop);
		}
	}

	int statId = m_flow[FLOW_OUTGOING].current & 0x1f;
	m_flow[FLOW_OUTGOING].stats[statId].size = data.CurrentSize() + UDP_HEADER_SIZE;
	m_flow[FLOW_OUTGOING].stats[statId].time = m_System->GetTime();
	m_flow[FLOW_OUTGOING].current++;

	COM_Munge2(data.GetData() + 8, data.CurrentSize() - 8, (unsigned char)(m_outgoing_sequence - 1));

	if (m_Socket) {
		m_Socket->SendPacket(&m_remote_address, data.GetData(), data.CurrentSize());
	}

	m_last_send = m_System->GetTime();
	m_cleartime = Q_max(m_send_interval, (data.CurrentSize() + UDP_HEADER_SIZE) * (1.0 / m_max_bandwidth_rate)) + m_last_send;
}

NetChannel::fragbuf_t *NetChannel::FindBufferById(fragbuf_t **pplist, int id, bool allocate)
{
	fragbuf_t *list = *pplist;
	fragbuf_t *pnewbuf;

	while (list)
	{
		if (list->bufferId == id)
			return list;

		list = list->next;
	}

	if (!allocate)
		return nullptr;

	// Create new entry
	pnewbuf = (fragbuf_t *)Mem_ZeroMalloc(sizeof(fragbuf_t));
	pnewbuf->bufferId = id;
	AddBufferToList(pplist, pnewbuf);

	return pnewbuf;
}

bool NetChannel::CheckForCompletion(int stream, int intotalbuffers)
{
	int c;
	int size;
	int id;
	fragbuf_t *p;

	size = 0;
	c = 0;

	if (stream != FRAG_NORMAL_STREAM && stream != FRAG_FILE_STREAM) {
		m_System->DPrintf("ERROR! NetChannel::CheckForCompletion: invalid stream number %i.\n", stream);
		return false;
	}

	p = m_incomingbufs[stream];
	if (!p) {
		return false;
	}

	while (p)
	{
		size += p->size;
		c++;

		id = FRAG_GETID(p->bufferId);
		if (id != c)
		{
			m_System->DPrintf("WARNING! NetChannel::CheckForCompletion: lost/dropped fragment Lost/dropped fragment would cause stall, retrying connection\n");
			m_crashed = true;
			return false;
		}

		p = p->next;
	}

	// Received final message
	if (c == intotalbuffers)
	{
		switch (stream)
		{
		case FRAG_NORMAL_STREAM:
			CopyNormalFragments();
			break;
		case FRAG_FILE_STREAM:
			m_System->Printf("TODO! NetChannel::CheckForCompletion: create file from fragments.\n");
			break;
		}

		return true;
	}

	return false;
}

bool NetChannel::ValidateFragments(BitBuffer &buf, bool *frag_message, unsigned int *fragid, int *frag_offset, int *frag_length)
{
	for (int i = 0; i < MAX_STREAMS; i++)
	{
		if (!frag_message[i])
			continue;

		// total fragments should be <= MAX_FRAGMENTS and current fragment can't be > total fragments
		if (i == FRAG_NORMAL_STREAM && FRAG_GETCOUNT(fragid[i]) > MAX_NORMAL_FRAGMENTS)
			return false;

		if (i == FRAG_FILE_STREAM && FRAG_GETCOUNT(fragid[i]) > MAX_FILE_FRAGMENTS)
			return false;

		if (FRAG_GETID(fragid[i]) > FRAG_GETCOUNT(fragid[i]))
			return false;

		if (!frag_length[i])
			return false;

		if ((size_t)frag_length[i] > FRAGMENT_MAX_SIZE || (size_t)frag_offset[i] > MAX_POSSIBLE_MSG - 1)
			return false;

		int frag_end = frag_offset[i] + frag_length[i];

		// end of fragment is out of the packet
		if (frag_end + buf.CurrentSize() > buf.GetMaxSize())
			return false;

		// fragment overlaps next stream's fragment or placed after it
		for (int j = i + 1; j < MAX_STREAMS; j++)
		{
			if (frag_end > frag_offset[j] && frag_message[j]) // don't add buf.CurrentSize() for comparison
				return false;
		}
	}

	return true;
}

void NetChannel::ProcessIncoming(unsigned char *data, int size)
{
	BitBuffer message(data, size);

	int i;
	unsigned int sequence, sequence_ack;
	unsigned int reliable_ack, reliable_message;
	unsigned int fragid[MAX_STREAMS] = { 0, 0 };

	bool frag_message[MAX_STREAMS] = { false, false };
	int frag_offset[MAX_STREAMS] = { 0, 0 };
	int frag_length[MAX_STREAMS] = { 0, 0 };

	bool message_contains_fragments;
	int net_drop;
	float newLoss;
	float weight;

	// get sequence numbers
	sequence = message.ReadLong();
	if (sequence == CONNECTIONLESS_HEADER)
	{
		NetPacket *p = new NetPacket;

		p->connectionless = true;
		p->time = m_System->GetTime();
		p->seqnr = -1;

		p->address.FromNetAddress(&m_remote_address);
		p->data.Resize(size - 4);
		p->data.WriteBuf(data + 4, size - 4);
		p->data.Reset();

		m_incomingPackets.AddHead(p);
		return;
	}

	if (!m_connected) {
		return;
	}

	sequence_ack = message.ReadLong();

	COM_UnMunge2(message.GetData() + 8, size - 8, sequence & 0xFF);

	reliable_message = sequence >> 31;
	reliable_ack = sequence_ack >> 31;
	message_contains_fragments = sequence & (1 << 30) ? true : false;

	// TODO: Looks like need to move it above COM_UnMunge2
	if (sequence_ack & 0x40000000)
	{
		m_crashed = true;
		return;
	}

	if (message_contains_fragments)
	{
		for (i = 0; i < MAX_STREAMS; i++)
		{
			if (message.ReadByte())
			{
				frag_message[i] = true;
				fragid[i] = message.ReadLong();
				frag_offset[i] = message.ReadShort();
				frag_length[i] = message.ReadShort();
			}
		}

#ifdef HLTV_FIXES
		if (!ValidateFragments(message, frag_message, fragid, frag_offset, frag_length))
			return;
#endif
	}

	sequence &= ~(1 << 31);
	sequence &= ~(1 << 30);

	sequence_ack &= ~(1 << 31);
	sequence_ack &= ~(1 << 30);

	if (sequence <= (unsigned int)m_incoming_sequence)
	{
		if (sequence == (unsigned int)m_incoming_sequence)
			m_System->DPrintf("NetChannel::ProcessIncoming: duplicate packet %i at %i from %s\n", sequence, m_incoming_sequence, m_remote_address.ToString());
		else
			m_System->DPrintf("NetChannel::ProcessIncoming: out of order packet %i at %i from %s\n", sequence, m_incoming_sequence, m_remote_address.ToString());

		return;
	}

	// dropped packets don't keep the message from being used
	net_drop = sequence - (m_incoming_sequence + 1);
	if (net_drop < 0) {
		net_drop = 0;
	}

	newLoss = (net_drop + 1) * (1.0f / 200.0f);
	if (newLoss < 1.0f)
	{
		weight = (float)net_drop / (float)(net_drop + 1);
		m_loss = (1.0 - newLoss) * m_loss + weight * newLoss;
	}
	else
		m_loss = 1;

	// if the current outgoing reliable message has been acknowledged
	// clear the buffer to make way for the next
	if (reliable_ack == (unsigned int)m_reliable_sequence)
	{
		// Make sure we actually could have ack'd this message
#ifdef HLTV_FIXES
		if (sequence_ack >= (unsigned)m_last_reliable_sequence)
#else
		if (m_incoming_acknowledged + 1 >= m_last_reliable_sequence)
#endif
		{
			// it has been received
			m_reliableOutSize = 0;
		}
	}

	// if this message contains a reliable message, bump incoming_reliable_sequence
	m_incoming_sequence = sequence;
	m_incoming_acknowledged = sequence_ack;
	m_incoming_reliable_acknowledged = reliable_ack;

	if (reliable_message) {
		m_incoming_reliable_sequence ^= 1u;
	}

	int statId = m_flow[FLOW_INCOMING].current & 0x1f;
	m_flow[FLOW_INCOMING].stats[statId].size = size + UDP_HEADER_SIZE;
	m_flow[FLOW_INCOMING].stats[statId].time = m_System->GetTime();
	m_flow[FLOW_INCOMING].current++;

	m_last_received = m_System->GetTime();

	if (message_contains_fragments)
	{
		for (i = 0; i <= 1; ++i)
		{
			int j;
			fragbuf_t *pbuf;
			int inbufferid;
			int intotalbuffers;

			if (!frag_message[i])
				continue;

			inbufferid = FRAG_GETID(fragid[i]);
			intotalbuffers = FRAG_GETCOUNT(fragid[i]);

			if (fragid[i])
			{
				pbuf = FindBufferById(&m_incomingbufs[i], fragid[i], true);
				if (pbuf)
				{
					Q_memcpy(pbuf->data, message.GetData() + message.CurrentSize() + frag_offset[i], frag_length[i]);
					pbuf->size = frag_length[i];
				}
				else
				{
					m_System->Printf("NetChannel::ProcessIncoming: couldn't allocate or find buffer %i\n", inbufferid);
				}

				// Count # of incoming bufs we've queued? are we done?
				CheckForCompletion(i, intotalbuffers);
			}

			// Rearrange incoming data to not have the frag stuff in the middle of it
			int wpos = message.CurrentSize() + frag_offset[i];
			int rpos = wpos + frag_length[i];

			Q_memmove(message.GetData() + wpos, message.GetData() + rpos, message.GetMaxSize() - rpos);
			message.m_MaxSize -= frag_length[i];

			for (j = i + 1; j < MAX_STREAMS; j++)
			{
				// fragments order already validated
				frag_offset[j] -= frag_length[i];
			}
		}
	}

	int curLen = message.GetMaxSize() - message.CurrentSize();
	if (curLen > 0)
	{
		NetPacket *p = new NetPacket;
		p->connectionless = 0;
		p->hasReliableData = reliable_message != 0;
		p->time = m_System->GetTime();
		p->seqnr = m_incoming_sequence;
		p->address.FromNetAddress(&m_remote_address);
		p->data.Resize(curLen);
		p->data.WriteBuf(message.m_CurByte, curLen);
		p->data.Reset();
		m_incomingPackets.AddHead(p);
	}
}

void NetChannel::FragSend()
{
	fragbufwaiting_t *wait;
	int i;

	for (i = 0; i < MAX_STREAMS; i++)
	{
		// Already something queued up, just leave in waitlist
		if (m_fragbufs[i]) {
			continue;
		}

		wait = m_waitlist[i];

		// Nothing to queue?
		if (!wait) {
			continue;
		}

		m_waitlist[i] = wait->next;
		wait->next = nullptr;

		// Copy in to fragbuf
		m_fragbufs[i] = wait->fragbufs;
		m_fragbufcount[i] = wait->fragbufcount;

		// Throw away wait list
		Mem_Free(wait);
	}
}

void NetChannel::AddBufferToList(fragbuf_t **pplist, fragbuf_t *pbuf)
{
	// Find best slot
	fragbuf_t *pprev, *n;
	int id1, id2;

	pbuf->next = nullptr;

	if (!pplist)
		return;

	if (!*pplist)
	{
		pbuf->next = *pplist;
		*pplist = pbuf;
		return;
	}

	pprev = *pplist;
	while (pprev->next)
	{
		n = pprev->next; // Next item in list
		id1 = FRAG_GETID(n->bufferId);
		id2 = FRAG_GETID(pbuf->bufferId);

		if (id1 > id2)
		{
			// Insert here
			pbuf->next = n->next;
			pprev->next = pbuf;
			return;
		}

		pprev = pprev->next;
	}

	// Insert at end
	pprev->next = pbuf;
}

bool NetChannel::CreateFragmentsFromBuffer(void *buffer, int size, int streamtype, char *filename)
{
	fragbuf_t *buf;
	int chunksize;
	int sendsize;
	int remaining;
	int pos;
	int bufferid = 1;
	bool firstfragment = true;
	fragbufwaiting_t *wait, *p;

	if (IsFakeChannel())
	{
		m_System->Printf("NetChannel::CreateFragmentsFromBuffer: IsFakeChannel()\n");
		return true;
	}

	if (size == 0) {
		return true;
	}

	unsigned char compressed[65536];
	char hdr[4] = "BZ2";

	unsigned int header_size = sizeof(hdr);
	unsigned int compressedSize = size - sizeof(hdr);	// we should fit in same data buffer minus 4 bytes for a header

	if (streamtype == FRAG_FILE_STREAM) {
		compressedSize -= 4;
	}

	if (!BZ2_bzBuffToBuffCompress((char *)compressed, &compressedSize, (char *)buffer, size, 9, 0, 30))
	{
		m_System->DPrintf("Compressing split packet (%d -> %d bytes)\n", size, compressedSize);
		Q_memcpy(buffer, hdr, sizeof(hdr));

		if (streamtype == FRAG_FILE_STREAM) {
			Q_memcpy((char *)buffer + 4, &size, sizeof(int));
			header_size = 8;
		}

		Q_memcpy((char *)buffer + header_size, compressed, compressedSize);
		size = header_size + compressedSize;
	}

	chunksize = FRAGMENT_S2C_MIN_SIZE;
	wait = (fragbufwaiting_t *)Mem_ZeroMalloc(sizeof(fragbufwaiting_t));

	remaining = size;
	pos = 0;
	while (remaining > 0)
	{
		sendsize = Q_min(remaining, chunksize);
		remaining -= sendsize;

		buf = (fragbuf_t *)Mem_ZeroMalloc(sizeof(fragbuf_t));
		if (!buf)
		{
			m_System->Printf("NetChannel::CreateFragmentsFromBuffer:Couldn't allocate fragbuf_t\n");
			Mem_Free(wait);
			return false;
		}

		if (firstfragment && filename)
		{
			firstfragment = false;

			unsigned int len = Q_strlen(filename) + 1;
			Q_memcpy(buf->data, filename, len);
			sendsize -= len;

			Q_memcpy(&buf->data[len], (char *)buffer + pos, sendsize);
			buf->size = len + sendsize;
		}
		else
		{
			Q_memcpy(buf->data, (char *)buffer + pos, sendsize);
			buf->size = sendsize;
		}

		buf->bufferId = bufferid++;
		buf->isfile = filename ? true : false;
		buf->isbuffer = true;

		// Copy in data
		pos += sendsize;

		AddFragbufToTail(wait, buf);
	}

	// Now add waiting list item to the end of buffer queue
	if (!m_waitlist[streamtype])
	{
		m_waitlist[streamtype] = wait;
	}
	else
	{
		p = m_waitlist[streamtype];
		while (p->next) {
			p = p->next;
		}

		p->next = wait;
	}

	return true;
}

void NetChannel::AddFragbufToTail(fragbufwaiting_t *wait, fragbuf_t *buf)
{
	fragbuf_t *p;

	buf->next = nullptr;
	wait->fragbufcount++;

	p = wait->fragbufs;
	if (p)
	{
		while (p->next) {
			p = p->next;
		}

		p->next = buf;
	}
	else
	{
		wait->fragbufs = buf;
	}
}

NetPacket *NetChannel::GetPacket()
{
	return (NetPacket *)m_incomingPackets.RemoveTail();
}

void NetChannel::FreePacket(NetPacket *packet)
{
	if (packet) {
		delete packet;
	}
}

void NetChannel::SetUpdateRate(int newupdaterate)
{
	m_updaterate = newupdaterate;
	if (newupdaterate > 100) {
		m_updaterate = 100;
	} else if (newupdaterate < 0) {
		m_updaterate = 1;
	}

	m_send_interval = (1.0f / m_updaterate);
}

void NetChannel::CopyNormalFragments()
{
	fragbuf_t *p, *n;
	int totalSize;

	if (!m_incomingbufs[FRAG_NORMAL_STREAM]) {
		m_System->DPrintf("WARNING! NetChannel::CopyNormalFragments: called with no fragments readied.\n");
		return;
	}

	totalSize = 0;
	p = m_incomingbufs[FRAG_NORMAL_STREAM];
	while (p)
	{
		totalSize += p->size;
		p = p->next;
	}

	NetPacket *packet = new NetPacket;
	packet->seqnr = m_incoming_sequence;
	packet->connectionless = false;
	packet->time = m_System->GetTime();
	packet->address.FromNetAddress(&m_remote_address);
	packet->data.Resize(totalSize);

	p = m_incomingbufs[FRAG_NORMAL_STREAM];
	while (p)
	{
		n = p->next;
		packet->data.WriteBuf(p->data, p->size);

		Mem_Free(p);
		p = n;
	}

#ifdef HLTV_FIXES
	if (packet->data.IsOverflowed())
	{
		if (packet->address.IsValid())
		{
			m_System->Printf("WARNING! NetChannel::CopyNormalFragments: Incoming overflowed from %s\n", packet->address.ToString());
		}
		else
		{
			m_System->Printf("WARNING! NetChannel::CopyNormalFragments: Incoming overflowed\n");
		}

		packet->data.Clear();
		m_incomingbufs[FRAG_NORMAL_STREAM] = nullptr;
		return;
	}
#endif

	if (totalSize >= 4 && *(uint32 *)packet->data.GetData() == MAKEID('B', 'Z', '2', '\0'))
	{
		char uncompressed[65536];
		unsigned int uncompressedSize = 65536;

		int result = BZ2_bzBuffToBuffDecompress(uncompressed, &uncompressedSize, (char *)packet->data.GetData() + 4, totalSize - 4, 1, 0);
		if (result == BZ_OK)
		{
			packet->data.Resize(uncompressedSize);
			packet->data.WriteBuf(uncompressed, uncompressedSize);
		}
		else
		{
			if (packet->address.IsValid())
			{
				m_System->Printf("WARNING! NetChannel::CopyNormalFragments: bzip2 decompression failed (%i) from %s\n", result, packet->address.ToString());
			}
			packet->data.Clear();
			m_incomingbufs[FRAG_NORMAL_STREAM] = nullptr;
			return;
		}
	}

	packet->data.Reset();

	m_incomingPackets.AddHead(packet);
	m_incomingbufs[FRAG_NORMAL_STREAM] = nullptr;
}

void NetChannel::SetConnected(bool flag)
{
	m_connected = flag;
}

void NetChannel::SetRate(int newRate)
{
	m_max_bandwidth_rate = Q_clamp(newRate, MIN_NETCHAN_RATE, MAX_NETCHAN_RATE);
}

void NetChannel::GetFlowStats(float *avgInKBSec, float *avgOutKBSec)
{
	UpdateFlow(FLOW_OUTGOING);
	UpdateFlow(FLOW_INCOMING);

	*avgInKBSec = m_flow[FLOW_OUTGOING].avgkbytespersec;
	*avgOutKBSec = m_flow[FLOW_INCOMING].avgkbytespersec;
}

void NetChannel::SetKeepAlive(bool flag)
{
	m_keep_alive = flag;
}

bool NetChannel::KeepAlive()
{
	return m_keep_alive;
}

NetAddress *NetChannel::GetTargetAddress()
{
	return &m_remote_address;
}

bool NetChannel::IsFakeChannel()
{
	if (m_Socket) {
		return false;
	}

	return true;
}

bool NetChannel::IsConnected()
{
	return m_connected;
}

void NetChannel::SetTimeOut(float time)
{
	if (time > 0) {
		m_timeout = time;
	}
}

bool NetChannel::IsTimedOut()
{
	if (m_Socket && m_System->GetTime() > m_timeout + m_last_received) {
		return true;
	}

	return false;
}

float NetChannel::GetIdleTime()
{
	return m_System->GetTime() - m_last_send;
}

int NetChannel::GetRate()
{
	return m_max_bandwidth_rate;
}

int NetChannel::GetUpdateRate()
{
	return m_updaterate;
}

float NetChannel::GetLoss()
{
	return m_loss;
}

void NetChannel::FakeAcknowledgement()
{
	m_incoming_sequence = 0;
	m_incoming_acknowledged = m_outgoing_sequence - 1;
	m_incoming_reliable_acknowledged = m_reliable_sequence;
	m_reliableOutSize = 0;

	m_last_received = m_System->GetTime();
}

bool NetChannel::CreateFragmentsFromFile(char *fileName)
{
	if (IsFakeChannel()) {
		m_System->Printf("NetChannel::CreateFragmentsFromBuffer: IsFakeChannel()\n");
		return true;
	}

	m_System->Printf("WARNING! Ignoring file request %s.\n", fileName);
	return false;
}

bool NetChannel::IsCrashed()
{
	return m_crashed;
}

bool NetChannel::CopyFileFragments()
{
	fragbuf_t *p;
	fragbuf_s *n;
	char filename[MAX_PATH];
	int totalSize = 0;

	if (!m_incomingbufs[FRAG_FILE_STREAM]) {
		m_System->DPrintf("WARNING! NetChannel::CopyFileFragments: called with no fragments readied.\n");
		return false;
	}

	totalSize = 0;
	p = m_incomingbufs[FRAG_FILE_STREAM];
	while (p)
	{
		totalSize += p->size;
		p = p->next;
	}

	BitBuffer filecontent(totalSize);
	p = m_incomingbufs[FRAG_FILE_STREAM];
	while (p)
	{
		n = p->next;
		filecontent.WriteBuf(p->data, p->size);
		Mem_Free(p);
		p = n;
	}

	filecontent.Reset();
	Q_strlcpy(filename, filecontent.ReadString());

	if (!Q_strlen(filename)) {
		m_System->Printf("File fragment received with no filename\n");
		FlushIncoming(FRAG_FILE_STREAM);
		return false;
	}

	if (Q_strstr(filename, "..")) {
		m_System->Printf("File fragment received with relative path, ignoring\n");
		FlushIncoming(FRAG_FILE_STREAM);
		return false;
	}

	// TODO: Here is the missing code.
	// TODO: Check me, value of return function only false.

	totalSize -= Q_strlen(filename) - 1;
	m_incomingbufs[FRAG_FILE_STREAM] = nullptr;

	return false;
}
