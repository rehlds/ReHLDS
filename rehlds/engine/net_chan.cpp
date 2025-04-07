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

int net_drop;
char gDownloadFile[256];

cvar_t net_log = { "net_log", "0", 0, 0.0f, nullptr};
cvar_t net_showpackets = { "net_showpackets", "0", 0, 0.0f, nullptr};
cvar_t net_showdrop = { "net_showdrop", "0", 0, 0.0f, nullptr};
cvar_t net_drawslider = { "net_drawslider", "0", 0, 0.0f, nullptr};
cvar_t net_chokeloopback = { "net_chokeloop", "0", 0, 0.0f, nullptr};

cvar_t sv_net_incoming_decompression = { "sv_net_incoming_decompression", "1", 0, 1.0f, nullptr };
cvar_t sv_net_incoming_decompression_max_ratio = { "sv_net_incoming_decompression_max_ratio", "80.0", 0, 80.0f, nullptr };
cvar_t sv_net_incoming_decompression_max_size = { "sv_net_incoming_decompression_max_size", "65536", 0, 65536.0f, nullptr };
cvar_t sv_net_incoming_decompression_min_failures = { "sv_net_incoming_decompression_min_failures", "4", 0, 4.0f, nullptr };
cvar_t sv_net_incoming_decompression_max_failures = { "sv_net_incoming_decompression_max_failures", "10", 0, 10.0f, nullptr };
cvar_t sv_net_incoming_decompression_min_failuretime = { "sv_net_incoming_decompression_min_failuretime", "0.1", 0, 0.1f, nullptr };
cvar_t sv_net_incoming_decompression_punish = { "sv_net_incoming_decompression_punish", "-1", 0, -1.0f, nullptr };

cvar_t sv_filetransfercompression = { "sv_filetransfercompression", "1", 0, 0.0f, nullptr};
cvar_t sv_filetransfermaxsize = { "sv_filetransfermaxsize", "10485760", 0, 0.0f, nullptr};

void Netchan_UnlinkFragment(fragbuf_t *buf, fragbuf_t **list)
{
	fragbuf_t *search;

	if (list == nullptr)
	{
		Con_Printf("%s: Asked to unlink fragment from empty list, ignored\n", __func__);
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

	Con_Printf("%s: Couldn't find fragment\n", __func__);
}

void Netchan_OutOfBand(netsrc_t sock, netadr_t adr, int length, byte *data)
{
	sizebuf_t send;
	byte send_buf[NET_MAX_PAYLOAD];

	send.buffername = "Netchan_OutOfBand";
	send.data = send_buf;
	send.maxsize = sizeof(send_buf);
	send.cursize = 0;
	send.flags = SIZEBUF_ALLOW_OVERFLOW;

	MSG_WriteLong(&send, -1);
	SZ_Write(&send, data, length);

	if (!g_pcls.demoplayback)
	{
		NET_SendPacket(sock, send.cursize, send.data, adr);
	}
}

void Netchan_OutOfBandPrint(netsrc_t sock, netadr_t adr, char *format, ...)
{
	va_list argptr;
	char string[8192];
	size_t len;

	va_start(argptr, format);
	len = Q_vsnprintf(string, sizeof(string), format, argptr);
	va_end(argptr);

	Netchan_OutOfBand(sock, adr, len + 1, (byte *)string);
}

void Netchan_ClearFragbufs(fragbuf_t **ppbuf)
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

void Netchan_ClearFragments(netchan_t *chan)
{
	fragbufwaiting_t *wait, *next;
	for (int i = 0; i < MAX_STREAMS; i++)
	{
		wait = chan->waitlist[i];
		while (wait)
		{
			next = wait->next;
			Netchan_ClearFragbufs(&wait->fragbufs);
			Mem_Free(wait);
			wait = next;
		}
		chan->waitlist[i] = nullptr;

		Netchan_ClearFragbufs(&chan->fragbufs[i]);
		Netchan_FlushIncoming(chan, i);
	}
}

void Netchan_Clear(netchan_t *chan)
{
	Netchan_ClearFragments(chan);

	if (chan->reliable_length)
	{
		Con_DPrintf("%s: reliable length not 0, reliable_sequence: %d, incoming_reliable_acknowledged: %d\n", __func__, chan->reliable_length, chan->incoming_reliable_acknowledged);
		chan->reliable_sequence ^= 1;
		chan->reliable_length = 0;
	}

	chan->cleartime = 0.0;

	for (int i = 0; i < MAX_STREAMS; i++)
	{
		chan->reliable_fragid[i] = 0;
		chan->reliable_fragment[i] = 0;
		chan->fragbufcount[i] = 0;
		chan->frag_startpos[i] = 0;
		chan->frag_length[i] = 0;
		chan->incomingready[i] = FALSE;
	}

	if (chan->tempbuffer)
	{
		Mem_Free(chan->tempbuffer);
		chan->tempbuffer = nullptr;
	}
	chan->tempbuffersize = 0;
}

void Netchan_Setup(netsrc_t socketnumber, netchan_t *chan, netadr_t adr, int player_slot, void *connection_status, qboolean(*pfnNetchan_Blocksize)(void *))
{
	Netchan_Clear(chan);
	g_GameClients[player_slot]->NetchanClear();

	Q_memset(chan, 0, sizeof(netchan_t));

	chan->player_slot = player_slot + 1;
	chan->sock = socketnumber;
	chan->remote_address = adr;
	chan->last_received = (float)realtime;
	chan->connect_time = (float)realtime;

	chan->message.buffername = "netchan->message";
#ifdef REHLDS_FIXES
	if (player_slot != -1)
	{
		chan->message.data = g_GameClients[player_slot]->GetExtendedMessageBuffer();
		chan->message.maxsize = NET_MAX_PAYLOAD;
	}
	else
#endif
	{
		chan->message.data = chan->message_buf;
		chan->message.maxsize = sizeof(chan->message_buf);
	}
#ifdef REHLDS_FIXES
	chan->message.cursize = 0;
#endif // REHLDS_FIXES
	chan->message.flags = SIZEBUF_ALLOW_OVERFLOW;

	chan->rate = DEFAULT_RATE;

	// Prevent the first message from getting dropped after connection is set up.
	chan->outgoing_sequence = 1;

	chan->connection_status = connection_status;
	chan->pfnNetchan_Blocksize = pfnNetchan_Blocksize;
}

qboolean Netchan_CanPacket(netchan_t *chan)
{
	// Never choke loopback packets.
	if (net_chokeloopback.value == 0.0f && chan->remote_address.type == NA_LOOPBACK)
	{
		chan->cleartime = realtime;
		return TRUE;
	}

	return chan->cleartime < realtime ? TRUE : FALSE;
}

void Netchan_UpdateFlow(netchan_t *chan)
{
	if (!chan)
		return;

	int bytes = 0;
	float faccumulatedtime = 0.0f;

	for (int flow = 0; flow < MAX_FLOWS; flow++)
	{
		flow_t *pflow = &chan->flow[flow];
		if (realtime - pflow->nextcompute < 0.1)
			continue;

		pflow->nextcompute = realtime + 0.1;
		int start = pflow->current - 1;
		for (int i = 0; i < MAX_LATENT - 1; i++)
		{
			flowstats_t *pprev = &pflow->stats[(start - i) & 0x1F];
			flowstats_t *pstat = &pflow->stats[(start - i - 1) & 0x1F];

			faccumulatedtime += pprev->time - pstat->time;
			bytes += pstat->size;
		}

		pflow->kbytespersec = (faccumulatedtime == 0.0f) ? 0.0f : bytes / faccumulatedtime / 1024.0f;
		pflow->avgkbytespersec = pflow->avgkbytespersec * (2.0 / 3) + pflow->kbytespersec * (1.0 / 3);
	}
}

void Netchan_Transmit(netchan_t *chan, int length, byte *data)
{
#ifdef REHLDS_FIXES
	byte send_buf[MAX_UDP_PACKET];
#else
	byte send_buf[NET_MAX_MESSAGE];
#endif
	qboolean send_reliable;
	qboolean send_reliable_fragment;
	qboolean send_resending = false;
	unsigned w1, w2;
	int i, j;

	float fRate;

	sizebuf_t sb_send;
	sb_send.data = send_buf;
	sb_send.buffername = "Netchan_Transmit";
	sb_send.maxsize = sizeof(send_buf);
	sb_send.flags = 0;
	sb_send.cursize = 0;

	// check for message overflow
	if (chan->message.flags & 2) {
		Con_Printf("%s:Outgoing message overflow\n", NET_AdrToString(chan->remote_address));
		return;
	}

	// if the remote side dropped the last reliable message, resend it
	send_reliable = false;

	if (chan->incoming_acknowledged > chan->last_reliable_sequence &&
		chan->incoming_reliable_acknowledged != chan->reliable_sequence)
	{
		send_reliable = true;
		send_resending = true;
	}

	//
	// A packet can have "reliable payload + frag payload + unreliable payload
	// frag payload can be a file chunk, if so, it needs to be parsed on the receiving end and reliable payload + unreliable payload need
	// to be passed on to the message queue.  The processing routine needs to be able to handle the case where a message comes in and a file
	// transfer completes
	//
	//
	// if the reliable transmit buffer is empty, copy the current message out
	if (!chan->reliable_length)
	{
		qboolean send_frag = false;
		fragbuf_t *pbuf;

		// Will be true if we are active and should let chan->message get some bandwidth
		int		 send_from_frag[MAX_STREAMS] = { 0, 0 };
		int		 send_from_regular = 0;

#ifdef REHLDS_FIXES
		if (chan->message.cursize > MAX_MSGLEN)
		{
			Netchan_CreateFragments_(chan == &g_pcls.netchan ? 1 : 0, chan, &chan->message);
			SZ_Clear(&chan->message);
		}
#endif

		// If we have data in the waiting list(s) and we have cleared the current queue(s), then
		//  push the waitlist(s) into the current queue(s)
		Netchan_FragSend(chan);

		// Sending regular payload
		send_from_regular = (chan->message.cursize) ? 1 : 0;

		// Check to see if we are sending a frag payload
		//
		for (i = 0; i < MAX_STREAMS; i++)
		{
			if (chan->fragbufs[i])
			{
				send_from_frag[i] = 1;
			}
		}

		// Stall reliable payloads if sending from frag buffer
		if (send_from_regular && (send_from_frag[FRAG_NORMAL_STREAM]))
		{
			send_from_regular = false;

			// If the reliable buffer has gotten too big, queue it at the end of everything and clear out buffer
			//
			if (chan->message.cursize > MAX_RELIABLE_PAYLOAD)
			{
				Netchan_CreateFragments_(chan == &g_pcls.netchan ? 1 : 0, chan, &chan->message);
				SZ_Clear(&chan->message);
			}
		}

		// Startpos will be zero if there is no regular payload
		for (i = 0; i < MAX_STREAMS; i++)
		{
			chan->frag_startpos[i] = 0;

			// Assume no fragment is being sent
			chan->reliable_fragment[i] = 0;
			chan->reliable_fragid[i] = 0;
			chan->frag_length[i] = 0;

			if (send_from_frag[i])
			{
				send_frag = true;
			}
		}

		if (send_from_regular || send_frag)
		{
			chan->reliable_sequence ^= 1;
			send_reliable = true;
		}

		if (send_from_regular) {
#ifdef REHLDS_FIXES
			Q_memcpy(chan->reliable_buf, chan->message.data, chan->message.cursize);
#else
			Q_memcpy(chan->reliable_buf, chan->message_buf, chan->message.cursize);
#endif
			chan->reliable_length = chan->message.cursize;
			SZ_Clear(&chan->message);

			// If we send fragments, this is where they'll start
			for (i = 0; i < MAX_STREAMS; i++) {
				chan->frag_startpos[i] = chan->reliable_length;
			}
		}

		for (i = 0; i < MAX_STREAMS; i++) {
			int fragment_size;

			// Is there something in the fragbuf?
			pbuf = chan->fragbufs[i];

			fragment_size = 0; // Compiler warning.
			if (pbuf)
			{
				fragment_size = pbuf->frag_message.cursize;

				// Files set size a bit differently.
				if (pbuf->isfile && !pbuf->isbuffer)
				{
					fragment_size = pbuf->size;
				}
			}

			// Make sure we have enought space left
			if (send_from_frag[i] && pbuf && ((chan->reliable_length + fragment_size) < MAX_RELIABLE_PAYLOAD)) {

				chan->reliable_fragid[i] = MAKE_FRAGID(pbuf->bufferid, chan->fragbufcount[i]); // Which buffer are we sending?

				// If it's not in-memory, then we'll need to copy it in frame the file handle.
				if (pbuf->isfile && !pbuf->isbuffer)	{
					char compressedfilename[MAX_PATH+5]; // room for extension string
					FileHandle_t hfile;
					if (pbuf->iscompressed)
					{
						Q_snprintf(compressedfilename, sizeof(compressedfilename), "%s.ztmp", pbuf->filename);
						hfile = FS_Open(compressedfilename, "rb");
					}
					else
					{
						hfile = FS_Open(pbuf->filename, "rb");
					}
					FS_Seek(hfile, pbuf->foffset, FILESYSTEM_SEEK_HEAD);
					FS_Read(&pbuf->frag_message.data[pbuf->frag_message.cursize], pbuf->size, 1, hfile);
					pbuf->frag_message.cursize += pbuf->size;
					FS_Close(hfile);
				}


				Q_memcpy(chan->reliable_buf + chan->reliable_length, pbuf->frag_message.data, pbuf->frag_message.cursize);
				chan->reliable_length += pbuf->frag_message.cursize;
				chan->frag_length[i] = pbuf->frag_message.cursize;


				// Unlink  pbuf
				Netchan_UnlinkFragment(pbuf, &chan->fragbufs[i]);

				chan->reliable_fragment[i] = 1;

				// Offset the rest of the starting positions
				for (j = i + 1; j < MAX_STREAMS; j++)
				{
					chan->frag_startpos[j] += chan->frag_length[i];
				}
			}
		}
	}

	// Prepare the packet header
	w1 = chan->outgoing_sequence | (send_reliable << 31);
	w2 = chan->incoming_sequence | (chan->incoming_reliable_sequence << 31);

	send_reliable_fragment = false;

	for (i = 0; i < MAX_STREAMS; i++)
	{
		if (chan->reliable_fragment[i])
		{
			send_reliable_fragment = true;
			break;
		}
	}

	if (send_reliable && send_reliable_fragment)
	{
		w1 |= (1 << 30);
	}

	chan->outgoing_sequence++;

	MSG_WriteLong(&sb_send, w1);
	MSG_WriteLong(&sb_send, w2);

	if (send_reliable && send_reliable_fragment)
	{
		for (i = 0; i < MAX_STREAMS; i++)
		{
			if (chan->reliable_fragment[i])
			{
				MSG_WriteByte(&sb_send, 1);
				MSG_WriteLong(&sb_send, chan->reliable_fragid[i]);
				MSG_WriteShort(&sb_send, chan->frag_startpos[i]);
				MSG_WriteShort(&sb_send, chan->frag_length[i]);
			}
			else
			{
				MSG_WriteByte(&sb_send, 0);
			}
		}
	}

	// Copy the reliable message to the packet first
	if (send_reliable) {
		SZ_Write(&sb_send, chan->reliable_buf, chan->reliable_length);
		chan->last_reliable_sequence = chan->outgoing_sequence - 1;
	}

	// Is there room for the unreliable payload?
	int max_send_size = MAX_ROUTEABLE_PACKET;
	if (!send_resending)
		max_send_size = sb_send.maxsize;

	if ((max_send_size - sb_send.cursize) >= length) {
		SZ_Write(&sb_send, data, length);
	}
	else {
		Con_DPrintf("Netchan_Transmit:  Unreliable would overfow, ignoring\n");
	}

	// Deal with packets that are too small for some networks
	if (sb_send.cursize < 16)	// Packet too small for some networks
	{
		// Go ahead and pad a full 16 extra bytes -- this only happens during authentication / signon
		for (int i = sb_send.cursize; i < 16; i++)
		{
			// Note that the server can parse svc_nop, too.
			MSG_WriteByte(&sb_send, svc_nop);
		}
	}

	int statId = chan->flow[FLOW_OUTGOING].current & 0x1F;
	chan->flow[FLOW_OUTGOING].stats[statId].size = sb_send.cursize + UDP_HEADER_SIZE;
	chan->flow[FLOW_OUTGOING].stats[statId].time = realtime;
	chan->flow[FLOW_OUTGOING].current++;
	Netchan_UpdateFlow(chan);

	if (!g_pcls.demoplayback)
	{
		COM_Munge2(sb_send.data + 8, sb_send.cursize - 8, (unsigned char)(chan->outgoing_sequence - 1));

		if (g_modfuncs.m_pfnProcessOutgoingNet)
			g_modfuncs.m_pfnProcessOutgoingNet(chan, &sb_send);

		NET_SendPacket(chan->sock, sb_send.cursize, sb_send.data, chan->remote_address);
	}

	if (g_psv.active && sv_lan.value != 0.0f && sv_lan_rate.value > MIN_RATE)
		fRate = 1.0 / sv_lan_rate.value;
	else
		fRate = 1.0 / chan->rate;

	if (chan->cleartime < realtime) {
		chan->cleartime = realtime;
	}

	chan->cleartime += (sb_send.cursize + UDP_HEADER_SIZE) * fRate;

	if (net_showpackets.value != 0.0f && net_showpackets.value != 2.0f) {
		char c = (chan == &g_pcls.netchan) ? 'c' : 's';

		Con_Printf(" %c --> sz=%i seq=%i ack=%i rel=%i tm=%f\n"
				   , c
				   , sb_send.cursize
				   , chan->outgoing_sequence - 1
				   , chan->incoming_sequence
				   , send_reliable ? 1 : 0
				   , (float)(chan == &g_pcls.netchan ? g_pcl.time : g_psv.time));
	}
}

fragbuf_t *Netchan_FindBufferById(fragbuf_t **pplist, int id, qboolean allocate)
{
	fragbuf_t *list = *pplist;
	fragbuf_t *pnewbuf;

	while (list)
	{
		if (list->bufferid == id)
			return list;

		list = list->next;
	}

	if (!allocate)
		return nullptr;

	// Create new entry
	pnewbuf = Netchan_AllocFragbuf();
	pnewbuf->bufferid = id;
	Netchan_AddBufferToList(pplist, pnewbuf);

	return pnewbuf;
}

void Netchan_CheckForCompletion(netchan_t *chan, int stream, int intotalbuffers)
{
	int c;
	int size;
	int id;
	fragbuf_t *p;

	size = 0;
	c = 0;

	p = chan->incomingbufs[stream];
	if (!p)
		return;

	while (p)
	{
		size += p->frag_message.cursize;
		c++;

		id = FRAG_GETID(p->bufferid);
		if (id != c && chan == &g_pcls.netchan)
		{
			if (chan->sock == NS_MULTICAST)
			{
				char szCommand[32];
				Q_snprintf(szCommand, sizeof(szCommand), "listen %s\n", NET_AdrToString(chan->remote_address));
				Cbuf_AddText(szCommand);
				return;
			}
			Con_Printf("%s:  Lost/dropped fragment would cause stall, retrying connection\n", __func__);
			Cbuf_AddText("retry\n");
		}

		p = p->next;
	}

	// Received final message
	if (c == intotalbuffers)
	{
		chan->incomingready[stream] = true;
	}
}

qboolean Netchan_Validate(netchan_t *chan, qboolean *frag_message, unsigned int *fragid, int *frag_offset, int *frag_length)
{
	for (int i = 0; i < MAX_STREAMS; i++)
	{
		if (!frag_message[i])
			continue;

#ifndef REHLDS_FIXES
		if (FRAG_GETID(fragid[i]) > MAX_FRAGMENTS || FRAG_GETCOUNT(fragid[i]) > MAX_FRAGMENTS)
		{
			return FALSE;
		}

		if ((unsigned int)frag_length[i] > 0x800 || (unsigned int)frag_offset[i] > 0x4000)
		{
			return FALSE;
		}
#else // REHLDS_FIXES
		// total fragments should be <= MAX_FRAGMENTS and current fragment can't be > total fragments
		if (i == FRAG_NORMAL_STREAM && FRAG_GETCOUNT(fragid[i]) > MAX_NORMAL_FRAGMENTS)
			return FALSE;
		if (i == FRAG_FILE_STREAM && FRAG_GETCOUNT(fragid[i]) > MAX_FILE_FRAGMENTS)
			return FALSE;
		if (FRAG_GETID(fragid[i]) > FRAG_GETCOUNT(fragid[i]))
			return FALSE;
		if (!frag_length[i])
			return FALSE;
		if ((size_t)frag_length[i] > FRAGMENT_MAX_SIZE || (size_t)frag_offset[i] > NET_MAX_PAYLOAD - 1)
			return FALSE;

		int frag_end = frag_offset[i] + frag_length[i];

		// end of fragment is out of the packet
		if (frag_end + msg_readcount > net_message.cursize)
			return FALSE;

		// fragment overlaps next stream's fragment or placed after it
		for (int j = i + 1; j < MAX_STREAMS; j++)
		{
			if (frag_end > frag_offset[j] && frag_message[j]) // don't add msg_readcount for comparison
				return FALSE;
		}
#endif // REHLDS_FIXES
	}

	return TRUE;
}

qboolean Netchan_Process(netchan_t *chan)
{
	int				i;
	unsigned int	sequence, sequence_ack;
	unsigned int	reliable_ack, reliable_message;
	unsigned int	fragid[MAX_STREAMS] = { 0, 0 };
	qboolean		frag_message[MAX_STREAMS] = { false, false };
	int				frag_offset[MAX_STREAMS] = { 0, 0 };
	int				frag_length[MAX_STREAMS] = { 0, 0 };
	qboolean		message_contains_fragments;


	if (!g_pcls.demoplayback && !g_pcls.passive)
	{
		if (!NET_CompareAdr(net_from, chan->remote_address))
			return FALSE;
	}

	chan->last_received = realtime;

	// get sequence numbers
	MSG_BeginReading();
	sequence = MSG_ReadLong();
	sequence_ack = MSG_ReadLong();

	if (sequence_ack & 0x40000000)
	{
		if (!g_modfuncs.m_pfnProcessIncomingNet)
			return FALSE;
	}

	if (g_modfuncs.m_pfnProcessIncomingNet)
	{
		if (!g_modfuncs.m_pfnProcessIncomingNet(chan, &net_message))
			return FALSE;
	}

	reliable_message = sequence >> 31;
	reliable_ack = sequence_ack >> 31;
	message_contains_fragments = sequence & (1 << 30) ? true : false;

	COM_UnMunge2(&net_message.data[8], net_message.cursize - 8, sequence & 0xFF);
	if (message_contains_fragments)
	{
		for (i = 0; i < MAX_STREAMS; i++)
		{
			if (MSG_ReadByte())
			{
				frag_message[i] = true;
				fragid[i] = MSG_ReadLong();
				frag_offset[i] = MSG_ReadShort();
				frag_length[i] = MSG_ReadShort();
			}
		}

		if (!Netchan_Validate(chan, frag_message, fragid, frag_offset, frag_length))
			return FALSE;
	}

	sequence &= ~(1 << 31);
	sequence &= ~(1 << 30);
	sequence_ack &= ~(1 << 31);
	sequence_ack &= ~(1 << 30);

	if (net_showpackets.value != 0.0 && net_showpackets.value != 3.0)
	{
		char c = (chan == &g_pcls.netchan) ? 'c' : 's';

		Con_Printf(
			" %c <-- sz=%i seq=%i ack=%i rel=%i tm=%f\n",
			c,
			net_message.cursize,
			sequence,
			sequence_ack,
			reliable_message,
			(chan == &g_pcls.netchan) ? g_pcl.time : g_psv.time);
	}

	if (sequence <= (unsigned)chan->incoming_sequence)
	{
		if (net_showdrop.value != 0.0) {
			if (sequence == (unsigned)chan->incoming_sequence)
				Con_Printf("%s:duplicate packet %i at %i\n", NET_AdrToString(chan->remote_address), sequence, chan->incoming_sequence);
			else
				Con_Printf("%s:out of order packet %i at %i\n", NET_AdrToString(chan->remote_address), sequence, chan->incoming_sequence);
		}
		return FALSE;
	}

	//
	// dropped packets don't keep the message from being used
	//
	net_drop = sequence - (chan->incoming_sequence + 1);
	if (net_drop > 0 && net_showdrop.value != 0.0)
	{
		Con_Printf("%s:Dropped %i packets at %i\n", NET_AdrToString(chan->remote_address), net_drop, sequence);
	}

	//
	// if the current outgoing reliable message has been acknowledged
	// clear the buffer to make way for the next
	//
	if (reliable_ack == (unsigned)chan->reliable_sequence)
	{
		// Make sure we actually could have ack'd this message
#ifdef REHLDS_FIXES
		if (sequence_ack >= (unsigned)chan->last_reliable_sequence)
#else // REHLDS_FIXES
		if (chan->incoming_acknowledged + 1 >= chan->last_reliable_sequence)
#endif // REHLDS_FIXES
		{
			chan->reliable_length = 0;	// it has been received
		}
	}

	//
	// if this message contains a reliable message, bump incoming_reliable_sequence
	//
	chan->incoming_sequence = sequence;
	chan->incoming_acknowledged = sequence_ack;
	chan->incoming_reliable_acknowledged = reliable_ack;
	if (reliable_message)
	{
		chan->incoming_reliable_sequence ^= 1;
	}

	int statId = chan->flow[FLOW_INCOMING].current & 0x1F;
	chan->flow[FLOW_INCOMING].stats[statId].size = net_message.cursize + UDP_HEADER_SIZE;
	chan->flow[FLOW_INCOMING].stats[statId].time = realtime;
	chan->flow[FLOW_INCOMING].current++;
	Netchan_UpdateFlow(chan);

	if (message_contains_fragments)
	{
		for (i = 0; i < MAX_STREAMS; i++)
		{
			int j;
			fragbuf_t *pbuf;
			int inbufferid;
			int intotalbuffers;

			if (!frag_message[i])
				continue;

			inbufferid = FRAG_GETID(fragid[i]);
			intotalbuffers = FRAG_GETCOUNT(fragid[i]);

			if (fragid[i] != 0)
			{
				pbuf = Netchan_FindBufferById(&chan->incomingbufs[i], fragid[i], true);
				if (pbuf) {
					int len = frag_length[i];
					SZ_Clear(&pbuf->frag_message);
					SZ_Write(&pbuf->frag_message, &net_message.data[msg_readcount + frag_offset[i]], len);
				}
				else {
					Con_Printf("Netchan_Process:  Couldn't allocate or find buffer %i\n", inbufferid);
				}
				// Count # of incoming bufs we've queued? are we done?
				Netchan_CheckForCompletion(chan, i, intotalbuffers);
			}

			// Rearrange incoming data to not have the frag stuff in the middle of it
			int wpos = msg_readcount + frag_offset[i];
			int rpos = wpos + frag_length[i];

			Q_memmove(net_message.data + wpos, net_message.data + rpos, net_message.cursize - rpos);
			net_message.cursize -= frag_length[i];

			for (j = i + 1; j < MAX_STREAMS; j++)
			{
				frag_offset[j] -= frag_length[i]; // fragments order already validated
			}
		}

		// Is there anything left to process?
		if (net_message.cursize <= 16)
			return FALSE;
	}

	return TRUE;
}

void Netchan_FragSend(netchan_t *chan)
{
	fragbufwaiting_t *wait;
	int i;

	if (!chan)
		return;

	for (i = 0; i < MAX_STREAMS; i++)
	{
		// Already something queued up, just leave in waitlist
		if (chan->fragbufs[i])
		{
			continue;
		}

		wait = chan->waitlist[i];

		// Nothing to queue?
		if (!wait)
		{
			continue;
		}

		chan->waitlist[i] = wait->next;

#ifdef REHLDS_FIXES
		if (wait->fragbufs->isfile && !wait->fragbufs->isbuffer && !wait->fragbufs->size)
		{
			if (!Netchan_CreateFileFragments_(true, chan, wait->fragbufs->filename))
			{
				Mem_Free(wait);

				continue;
			}

			fragbufwaiting_t *prev = nullptr;
			while (chan->waitlist[i]->next)
			{
				prev = chan->waitlist[i];
				chan->waitlist[i] = chan->waitlist[i]->next;
			}

			if (prev)
				prev->next = nullptr;

			auto oldWait = wait;
			wait = chan->waitlist[i];

			chan->waitlist[i] = oldWait->next;

			Mem_Free(oldWait);
		}
#endif // REHLDS_FIXES

		wait->next = nullptr;

		// Copy in to fragbuf
		chan->fragbufs[i] = wait->fragbufs;
		chan->fragbufcount[i] = wait->fragbufcount;

		// Throw away wait list
		Mem_Free(wait);
	}
}

void Netchan_AddBufferToList(fragbuf_t **pplist, fragbuf_t *pbuf)
{
	// Find best slot
	fragbuf_t *pprev, *n;
	int		id1, id2;

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
		id1 = FRAG_GETID(n->bufferid);
		id2 = FRAG_GETID(pbuf->bufferid);

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

fragbuf_t *Netchan_AllocFragbuf(void)
{
	fragbuf_t *buf;

	buf = (fragbuf_t *)Mem_ZeroMalloc(sizeof(fragbuf_t));
	buf->bufferid = 0;
	buf->frag_message.cursize = 0;
	buf->frag_message.data = buf->frag_message_buf;
	buf->frag_message.maxsize = sizeof(buf->frag_message_buf);
	buf->frag_message.buffername = "Frag Buffer Alloc'd";
	buf->next = nullptr;

	return buf;
}

void Netchan_AddFragbufToTail(fragbufwaiting_t *wait, fragbuf_t *buf)
{
	fragbuf_t *p;

	buf->next = nullptr;
	wait->fragbufcount++;

	p = wait->fragbufs;
	if (p)
	{
		while (p->next)
		{
			p = p->next;
		}
		p->next = buf;
	}
	else
	{
		wait->fragbufs = buf;
	}
}

void Netchan_CreateFragments_(qboolean server, netchan_t *chan, sizebuf_t *msg)
{
	fragbuf_t *buf;
	int chunksize;
	int send;
	int remaining;
	int pos;
	int bufferid = 1;
	fragbufwaiting_t *wait, *p;

	if (msg->cursize == 0)
	{
		return;
	}

	// Compress if not done already
	if (*(uint32 *)msg->data != MAKEID('B', 'Z', '2', '\0'))
	{
		unsigned char compressed[65536];
		char hdr[4] = "BZ2";
		unsigned int compressedSize = msg->cursize - sizeof(hdr);	// we should fit in same data buffer minus 4 bytes for a header
		if (!BZ2_bzBuffToBuffCompress((char *)compressed, &compressedSize, (char *)msg->data, msg->cursize, 9, 0, 30))
		{
			Con_DPrintf("Compressing split packet (%d -> %d bytes)\n", msg->cursize, compressedSize);
			Q_memcpy(msg->data, hdr, sizeof(hdr));
			Q_memcpy(msg->data + sizeof(hdr), compressed, compressedSize);
			msg->cursize = compressedSize + sizeof(hdr);
		}
	}

	chunksize = chan->pfnNetchan_Blocksize(chan->connection_status);

	wait = (fragbufwaiting_t *)Mem_ZeroMalloc(sizeof(fragbufwaiting_t));

	remaining = msg->cursize;
	pos = 0;
	while (remaining > 0)
	{
		send = min(remaining, chunksize);
		remaining -= send;

		buf = Netchan_AllocFragbuf();
		if (!buf)
		{
			return;
		}

		buf->bufferid = bufferid++;

		// Copy in data
		SZ_Clear(&buf->frag_message);
		SZ_Write(&buf->frag_message, &msg->data[pos], send);
		pos += send;

		Netchan_AddFragbufToTail(wait, buf);
	}

	// Now add waiting list item to the end of buffer queue
	if (!chan->waitlist[FRAG_NORMAL_STREAM])
	{
		chan->waitlist[FRAG_NORMAL_STREAM] = wait;
	}
	else
	{
		p = chan->waitlist[FRAG_NORMAL_STREAM];
		while (p->next)
		{
			p = p->next;
		}
		p->next = wait;
	}
}

void Netchan_CreateFragments(qboolean server, netchan_t *chan, sizebuf_t *msg)
{
	// Always queue any pending reliable data ahead of the fragmentation buffer
	if (chan->message.cursize > 0)
	{
		Netchan_CreateFragments_(server, chan, &chan->message);
		chan->message.cursize = 0;
	}

	Netchan_CreateFragments_(server, chan, msg);
}

void Netchan_CreateFileFragmentsFromBuffer(qboolean server, netchan_t *chan, const char *filename, unsigned char *uncompressed_pbuf, int uncompressed_size)
{
	int chunksize;
	int send;
	fragbufwaiting_t *p;
	fragbuf_t *buf;
	unsigned char *pbuf;
	qboolean bCompressed;
	qboolean firstfragment;
	signed int bufferid;
	int remaining;
	int pos;
	unsigned int size;
	fragbufwaiting_t *wait;

	if (!uncompressed_size)
		return;

	bufferid = 1;
	firstfragment = TRUE;
	size = uncompressed_size;

	pbuf = (unsigned char *)Mem_Malloc(uncompressed_size);
	if (BZ2_bzBuffToBuffCompress((char*)pbuf, &size, (char*)uncompressed_pbuf, uncompressed_size, 9, 0, 30))
	{
		bCompressed = FALSE;
		Mem_Free(pbuf);
		pbuf = uncompressed_pbuf;
		size = uncompressed_size;
	}
	else
	{
		bCompressed = TRUE;
		Con_DPrintf("Compressed %s for transmission (%d -> %d)\n", filename, uncompressed_size, size);
	}

	chunksize = chan->pfnNetchan_Blocksize(chan->connection_status);
	send = chunksize;
	wait = (fragbufwaiting_t *)Mem_ZeroMalloc(0xCu);
	remaining = size;
	pos = 0;

	while (remaining > 0)
	{
		send = min(remaining, chunksize);
		buf = (fragbuf_t *)Netchan_AllocFragbuf();
		if (!buf)
		{
			Con_Printf("Couldn't allocate fragbuf_t\n");
			Mem_Free(wait);
			if (server)
				SV_DropClient(host_client, 0, "Malloc problem");
			else
				rehlds_syserror("%s:Reverse me: client-side code", __func__);

#ifdef REHLDS_FIXES
			if (bCompressed) {
				Mem_Free(pbuf);
			}
#endif
			return;
		}

		buf->bufferid = bufferid++;
		SZ_Clear(&buf->frag_message);
		if (firstfragment)
		{
			firstfragment = FALSE;
			MSG_WriteString(&buf->frag_message, filename);
			MSG_WriteString(&buf->frag_message, bCompressed ? "bz2" : "uncompressed");
			MSG_WriteLong(&buf->frag_message, uncompressed_size);

			// Check if we aren't send more than we should
			if ((chunksize - send) < buf->frag_message.cursize)
				send -= buf->frag_message.cursize;
		}

		buf->isbuffer = TRUE;
		buf->isfile = TRUE;
		buf->size = send;
		buf->foffset = pos;

		MSG_WriteBuf(&buf->frag_message, send, &pbuf[pos]);
		pos += send;
		remaining -= send;

		Netchan_AddFragbufToTail(wait, buf);
	}

	if (!chan->waitlist[FRAG_FILE_STREAM]) {
		chan->waitlist[FRAG_FILE_STREAM] = wait;
	}
	else
	{
		p = chan->waitlist[FRAG_FILE_STREAM];
		while (p->next)
			p = p->next;

		p->next = wait;
	}

#ifdef REHLDS_FIXES
	if (bCompressed) {
		Mem_Free(pbuf);
	}
#endif
}

int Netchan_CreateFileFragments(qboolean server, netchan_t *chan, const char *filename)
#ifdef REHLDS_FIXES
{
	if (!server)
		return Netchan_CreateFileFragments_(server, chan, filename);

	if (!FS_FileExists(filename))
		return FALSE;

	unsigned int nSize = FS_FileSize(filename);
	if (nSize == 0 || nSize > sv_filetransfermaxsize.value)
		return FALSE;

	auto wait = (fragbufwaiting_t *)Mem_ZeroMalloc(sizeof(fragbufwaiting_t));

	auto buf = Netchan_AllocFragbuf();
	buf->bufferid = 1;
	buf->isbuffer = false;
	buf->isfile = true;
	Q_strncpy(buf->filename, filename, sizeof(buf->filename));
	buf->filename[sizeof(buf->filename) - 1] = '\0';

	Netchan_AddFragbufToTail(wait, buf);

	if (!chan->waitlist[FRAG_FILE_STREAM])
	{
		chan->waitlist[FRAG_FILE_STREAM] = wait;
	}
	else
	{
		auto p = chan->waitlist[FRAG_FILE_STREAM];
		while (p->next)
			p = p->next;

		p->next = wait;
	}

	return TRUE;
}

int Netchan_CreateFileFragments_(qboolean server, netchan_t *chan, const char *filename)
#endif // REHLDS_FIXES
{
	int chunksize;
	int compressedFileTime;
	FileHandle_t hfile;
	signed int filesize;
	int remaining;
	fragbufwaiting_t *p;
	int send;
	fragbuf_t *buf;
	char compressedfilename[MAX_PATH];
	qboolean firstfragment;
	int bufferid;
	qboolean bCompressed;
	int pos;
	fragbufwaiting_t *wait;
	int uncompressed_size;

	bufferid = 1;
	firstfragment = TRUE;
	bCompressed = FALSE;
	chunksize = chan->pfnNetchan_Blocksize(chan->connection_status);

	Q_snprintf(compressedfilename, sizeof compressedfilename, "%s.ztmp", filename);
	compressedFileTime = FS_GetFileTime(compressedfilename);
	if (compressedFileTime >= FS_GetFileTime(filename) && (hfile = FS_Open(compressedfilename, "rb")))
	{
		filesize = FS_Size(hfile);
		FS_Close(hfile);
		bCompressed = TRUE;
		hfile = FS_Open(filename, "rb");
		if (!hfile)
		{
			Con_Printf("Warning:  Unable to open %s for transfer\n", filename);
			return 0;
		}

		uncompressed_size = FS_Size(hfile);
		if (uncompressed_size > sv_filetransfermaxsize.value)
		{
			FS_Close(hfile);
			Con_Printf("Warning:  File %s is too big to transfer from host %s\n", filename, NET_AdrToString(chan->remote_address));
			return 0;
		}
	}
	else
	{
		hfile = FS_Open(filename, "rb");
		if (!hfile)
		{
			Con_Printf("Warning:  Unable to open %s for transfer\n", filename);
			return 0;
		}
		filesize = FS_Size(hfile);
		if (filesize > sv_filetransfermaxsize.value)
		{
			FS_Close(hfile);
			Con_Printf("Warning:  File %s is too big to transfer from host %s\n", filename, NET_AdrToString(chan->remote_address));
			return 0;
		}

		uncompressed_size = filesize;
		if (sv_filetransfercompression.value != 0.0)
		{
			unsigned char* uncompressed = (unsigned char*)Mem_Malloc(filesize);
			unsigned char* compressed = (unsigned char*)Mem_Malloc(filesize);
			unsigned int compressedSize = filesize;
			FS_Read(uncompressed, filesize, 1, hfile);
			if (BZ_OK == BZ2_bzBuffToBuffCompress((char*)compressed, &compressedSize, (char*)uncompressed, filesize, 9, 0, 30))
			{
				FileHandle_t destFile = FS_Open(compressedfilename, "wb");
				if (destFile)
				{
					Con_DPrintf("Creating compressed version of file %s (%d -> %d)\n", filename, filesize, compressedSize);
					FS_Write(compressed, compressedSize, 1, destFile);
					FS_Close(destFile);
					filesize = compressedSize;
					bCompressed = TRUE;
				}
			}
			Mem_Free(uncompressed);
			Mem_Free(compressed);
		}
	}
	FS_Close(hfile);

	wait = (fragbufwaiting_t *)Mem_ZeroMalloc(0xCu);
	remaining = filesize;
	pos = 0;

	while (remaining > 0)
	{
		send = min(chunksize, remaining);
		buf = Netchan_AllocFragbuf();
		if (!buf)
		{
			Con_Printf("Couldn't allocate fragbuf_t\n");
			Mem_Free(wait);
			if (server)
			{
#ifdef REHLDS_FIXES
				SV_DropClient(&g_psvs.clients[chan->player_slot - 1], 0, "Malloc problem");
#else // REHLDS_FIXES
				SV_DropClient(host_client, 0, "Malloc problem");
#endif // REHLDS_FIXES
				return 0;
			}
			else
			{
				rehlds_syserror("%s: Reverse clientside code", __func__);
				//return 0;
			}
		}

		buf->bufferid = bufferid++;
		SZ_Clear(&buf->frag_message);
		if (firstfragment)
		{
			firstfragment = FALSE;
			MSG_WriteString(&buf->frag_message, filename);
			MSG_WriteString(&buf->frag_message, bCompressed ? "bz2" : "uncompressed");
			MSG_WriteLong(&buf->frag_message, uncompressed_size);

			// Check if we aren't send more than we should
			if ((chunksize - send) < buf->frag_message.cursize)
				send -= buf->frag_message.cursize;
		}
		buf->isfile = TRUE;
		buf->iscompressed = bCompressed;
		buf->size = send;
		buf->foffset = pos;

		Q_strncpy(buf->filename, filename, MAX_PATH - 1);
		buf->filename[MAX_PATH - 1] = 0;

		pos += send;
		remaining -= send;

		Netchan_AddFragbufToTail(wait, buf);
	}

	if (!chan->waitlist[FRAG_FILE_STREAM])
	{
		chan->waitlist[FRAG_FILE_STREAM] = wait;
	}
	else
	{
		p = chan->waitlist[FRAG_FILE_STREAM];
		while (p->next)
			p = p->next;

		p->next = wait;
	}

	return 1;
}

void Netchan_FlushIncoming(netchan_t *chan, int stream)
{
	fragbuf_t *p, *n;

#ifdef REHLDS_FIXES
	if ((chan->player_slot - 1) == host_client - g_psvs.clients)
#endif
	{
		SZ_Clear(&net_message);
		msg_readcount = 0;
	}

	p = chan->incomingbufs[stream];
	while (p)
	{
		n = p->next;
		Mem_Free(p);
		p = n;
	}

	chan->incomingbufs[stream] = nullptr;
	chan->incomingready[stream] = FALSE;
}

void Netchan_DecompressionCvarsBounds()
{
	if (sv_net_incoming_decompression_min_failures.value < 1)
		Cvar_SetValue("sv_net_incoming_decompression_min_failures", 1);

	else if (sv_net_incoming_decompression_min_failures.value > NET_DECOMPRESS_MAX_TIMES)
		Cvar_SetValue("sv_net_incoming_decompression_min_failures", NET_DECOMPRESS_MAX_TIMES);

	if (sv_net_incoming_decompression_max_failures.value < 1)
		Cvar_SetValue("sv_net_incoming_decompression_max_failures", 1);

	else if (sv_net_incoming_decompression_max_failures.value > NET_DECOMPRESS_MAX_TIMES)
		Cvar_SetValue("sv_net_incoming_decompression_max_failures", NET_DECOMPRESS_MAX_TIMES);

	if (sv_net_incoming_decompression_max_failures.value < sv_net_incoming_decompression_min_failures.value)
	{
		int iTemp = sv_net_incoming_decompression_max_failures.value;
		Cvar_SetValue("sv_net_incoming_decompression_max_failures", sv_net_incoming_decompression_min_failures.value);
		Cvar_SetValue("sv_net_incoming_decompression_min_failures", iTemp);
	}

	if (sv_net_incoming_decompression_min_failuretime.value <= 0.0f)
		Cvar_SetValue("sv_net_incoming_decompression_min_failuretime", 0.1f);
}

// Check for an abnormal size ratio between compressed and uncompressed data
qboolean Netchan_ValidateDecompress(netchan_t *chan, int stream, unsigned int compressedSize, unsigned int uncompressedSize)
{
#ifdef REHLDS_FIXES
	int		i;

	if (sv_net_incoming_decompression_max_ratio.value <= 0)
		return TRUE; // validation is disabled

	if (compressedSize >= uncompressedSize)
		return TRUE;

	float ratio = ((float)(uncompressedSize - compressedSize) / uncompressedSize) * 100.0f;
	if (ratio < sv_net_incoming_decompression_max_ratio.value)
		return TRUE; // no low entropy for uncompressed data

	if ((chan->player_slot - 1) != host_client - g_psvs.clients)
		return TRUE;

	Netchan_DecompressionCvarsBounds();

	FragStats_t &stats = g_GameClients[chan->player_slot - 1]->GetFragStats(stream);

	// check if the client should be rejected based on total failed decompress
	if (stats.num_decompress_failures >= sv_net_incoming_decompression_max_failures.value)
	{
		for (i = 0; i < sv_net_incoming_decompression_max_failures.value - 1; i++)
			stats.decompress_failure_times[i] = stats.decompress_failure_times[i + 1];

		stats.num_decompress_failures = sv_net_incoming_decompression_max_failures.value - 1;
	}

	stats.decompress_failure_times[stats.num_decompress_failures++] = realtime;

	// check if the client should be rejected based on recent failed decompress
	int recent_failures = 0;
	for (i = 0; i < stats.num_decompress_failures; i++)
	{
		if ((realtime - stats.decompress_failure_times[i]) <= sv_net_incoming_decompression_min_failuretime.value)
			recent_failures++;
	}

	if (recent_failures >= sv_net_incoming_decompression_min_failures.value)
	{
		if (chan->player_slot == 0)
			Con_DPrintf("Incoming abnormal uncompressed size with ratio %.2f\n", ratio);
		else
			Con_DPrintf("%s:Incoming abnormal uncompressed size with ratio %.2f from %s\n", NET_AdrToString(chan->remote_address), ratio, host_client->name);

		if (sv_net_incoming_decompression_punish.value >= 0)
		{
			Con_DPrintf("%s:Banned for malformed/abnormal bzip2 fragments from %s\n", NET_AdrToString(chan->remote_address), host_client->name);
			Cbuf_AddText(va("addip %.1f %s\n", sv_net_incoming_decompression_punish.value, NET_BaseAdrToString(chan->remote_address)));
		}

		return FALSE;
	}
#endif

	return TRUE;
}

qboolean Netchan_CopyNormalFragments(netchan_t *chan)
{
	fragbuf_t *p, *n;

	if (!chan->incomingready[FRAG_NORMAL_STREAM])
		return FALSE;

	if (!chan->incomingbufs[FRAG_NORMAL_STREAM])
	{
		Con_Printf("%s:  Called with no fragments readied\n", __func__);
		chan->incomingready[FRAG_NORMAL_STREAM] = FALSE;
		return FALSE;
	}

	p = chan->incomingbufs[FRAG_NORMAL_STREAM];

	chan->incomingbufs[FRAG_NORMAL_STREAM] = nullptr;
	chan->incomingready[FRAG_NORMAL_STREAM] = FALSE;

	SZ_Clear(&net_message);
	MSG_BeginReading();

#ifdef REHLDS_FIXES
	bool overflowed = false;
#endif // REHLDS_FIXES

	while (p)
	{
		n = p->next;

#ifdef REHLDS_FIXES
		if (net_message.cursize + p->frag_message.cursize <= net_message.maxsize)
			SZ_Write(&net_message, p->frag_message.data, p->frag_message.cursize);
		else
			overflowed = true;
#else // REHLDS_FIXES
		SZ_Write(&net_message, p->frag_message.data, p->frag_message.cursize);
#endif // REHLDS_FIXES

		Mem_Free(p);
		p = n;
	}

#ifdef REHLDS_FIXES
	if (overflowed)
	{
		if (chan->player_slot == 0)
		{
			Con_Printf("%s: Incoming overflowed\n", __func__);
		}
		else
		{
			Con_Printf("%s: Incoming overflowed from %s\n", __func__, g_psvs.clients[chan->player_slot - 1].name);
		}

		SZ_Clear(&net_message);
		return FALSE;
	}
#endif // REHLDS_FIXES

	if (*(uint32 *)net_message.data == MAKEID('B', 'Z', '2', '\0'))
	{
		// Determine whether decompression of compressed data is allowed
#ifdef REHLDS_FIXES
		if (!sv_net_incoming_decompression.value)
		{
			if (chan->player_slot == 0)
			{
				Con_DPrintf("Incoming compressed normal fragment disallowed from\n");
				return FALSE;
			}
			// compressed data is expected only after requesting resource list
			else if (host_client->m_sendrescount == 0)
			{
				Con_DPrintf("%s:Incoming compressed normal fragment disallowed from %s\n", NET_AdrToString(chan->remote_address), host_client->name);
				return FALSE;
			}
		}
#endif

		char uncompressed[65536];
		unsigned int uncompressedSize = clamp((int)sv_net_incoming_decompression_max_size.value, 16, 65536); // valid range (16 - 65536) bytes
		unsigned int compressedSize   = net_message.cursize - 4;

		// Decompress net buffer data
		qboolean success = TRUE;

		if (BZ2_bzBuffToBuffDecompress(uncompressed, &uncompressedSize, (char *)net_message.data + 4, compressedSize, 1, 0) != BZ_OK)
		{
			// malformed data or compressed data exceeding sv_net_incoming_decompression_max_size
			success = FALSE;
		}
		else if (!Netchan_ValidateDecompress(chan, FRAG_NORMAL_STREAM, compressedSize, uncompressedSize))
		{
			success = FALSE;
		}

#ifdef REHLDS_FIXES
		if (!success)
		{
			// Drop client if decompression was unsuccessful
			SV_DropClient(host_client, FALSE, "Malformed/abnormal compressed data");
			SZ_Clear(&net_message);
			return FALSE;
		}
#endif

		// Copy uncompressed data back to the net buffer
		Q_memcpy(net_message.data, uncompressed, uncompressedSize);
		net_message.cursize = uncompressedSize;
	}

	return TRUE;
}

qboolean Netchan_CopyFileFragments(netchan_t *chan)
{
	fragbuf_t *p;
	int nsize;
	unsigned char *buffer;
	int pos;
	signed int cursize;
	char filename[MAX_PATH];
	char compressor[32];
	fragbuf_s *n;
	qboolean bCompressed;
	unsigned int uncompressedSize;

	if (!chan->incomingready[FRAG_FILE_STREAM])
		return FALSE;

	p = chan->incomingbufs[FRAG_FILE_STREAM];
	if (!p)
	{
		Con_Printf("%s:  Called with no fragments readied\n", __func__);
		chan->incomingready[FRAG_FILE_STREAM] = FALSE;
		return FALSE;
	}

#ifdef REHLDS_FIXES
	if (chan->player_slot > 0 && (chan->player_slot - 1) == host_client - g_psvs.clients)
	{
		// customization already uploaded with request by operator,
		// do not accept any other customization
		if (host_client->uploaddoneregistering)
		{
			SV_DropClient(host_client, FALSE, "Too many customization have been uploaded (unrequested customization)");
			return FALSE;
		}
	}
#endif

	bCompressed = FALSE;
	SZ_Clear(&net_message);
	MSG_BeginReading();
	SZ_Write(&net_message, p->frag_message.data, p->frag_message.cursize);
	Q_strncpy(filename, MSG_ReadString(), sizeof(filename) - 1);
	filename[sizeof(filename) - 1] = 0;

	Q_strncpy(compressor, MSG_ReadString(), sizeof(compressor) - 1);
	compressor[sizeof(compressor) - 1] = 0;
	if (!Q_stricmp(compressor, "bz2"))
		bCompressed = TRUE;

	uncompressedSize = (unsigned int)MSG_ReadLong();

#ifdef REHLDS_FIXES
	// TODO: this condition is invalid for server->client
	// TODO: add console message for client
	// TODO: add client name to message
	if (uncompressedSize > 1024 * 64) {
		Con_Printf("Received too large file (size=%u)\nFlushing input queue\n", uncompressedSize);
		Netchan_FlushIncoming(chan, FRAG_FILE_STREAM);
		return FALSE;
	}
#endif

	if (Q_strlen(filename) <= 0)
	{
		Con_Printf("File fragment received with no filename\nFlushing input queue\n");
		Netchan_FlushIncoming(chan, FRAG_FILE_STREAM);
		return FALSE;
	}

	if (Q_strstr(filename, ".."))
	{
		Con_Printf("File fragment received with relative path, ignoring\n");
		Netchan_FlushIncoming(chan, FRAG_FILE_STREAM);
		return FALSE;
	}

	if (filename[0] != '!' && !IsSafeFileToDownload(filename))
	{
		Con_Printf("File fragment received with bad path, ignoring\n");
		Netchan_FlushIncoming(chan, FRAG_FILE_STREAM);
		return FALSE;
	}
	// This prohibits to write files to FS on server
	if (g_pcls.state == ca_dedicated && filename[0] != '!')
	{
		Con_Printf("File fragment received with bad path, ignoring (2)\n");
		Netchan_FlushIncoming(chan, FRAG_FILE_STREAM);
		return FALSE;
	}

	Q_strncpy(chan->incomingfilename, filename, MAX_PATH - 1);
	chan->incomingfilename[MAX_PATH - 1] = 0;

	if (filename[0] != '!' && FS_FileExists(filename))
	{
		Con_Printf("Can't download %s, already exists\n", filename);
		Netchan_FlushIncoming(chan, FRAG_FILE_STREAM);
		return TRUE;
	}

	nsize = 0;
	while (p)
	{
		nsize += p->frag_message.cursize;
		if (p == chan->incomingbufs[FRAG_FILE_STREAM])
			nsize -= msg_readcount;
		p = p->next;
	}

	buffer = (unsigned char*)Mem_ZeroMalloc(nsize + 1);
	if (!buffer)
	{
		Con_Printf("Buffer allocation failed on %i bytes\n", nsize + 1);
		Netchan_FlushIncoming(chan, FRAG_FILE_STREAM);
		return FALSE;
	}

	p = chan->incomingbufs[FRAG_FILE_STREAM];
	pos = 0;
	while (p)
	{
		n = p->next;

		cursize = p->frag_message.cursize;
		// First message has the file name, don't write that into the data stream, just write the rest of the actual data
		if (p == chan->incomingbufs[FRAG_FILE_STREAM])
		{
			// Copy it in
			cursize -= msg_readcount;
			Q_memcpy(&buffer[pos], &p->frag_message.data[msg_readcount], cursize);
			p->frag_message.cursize = cursize;
		}
		else
		{
			Q_memcpy(&buffer[pos], p->frag_message.data, cursize);
		}
		pos += p->frag_message.cursize;
		Mem_Free(p);
		p = n;

	}

	// FIXED: We have concat fragment buffer above, make sure that the fisrt fragment is null
	// otherwise we will get memory access violation at next call Netchan_FlushIncoming
#ifdef REHLDS_FIXES
	chan->incomingbufs[FRAG_FILE_STREAM] = nullptr;
	chan->incomingready[FRAG_FILE_STREAM] = FALSE;
#endif

	if (bCompressed)
	{
		// Determine whether decompression of compressed data is allowed
#ifdef REHLDS_FIXES
		if (!sv_net_incoming_decompression.value)
		{
			if (chan->player_slot == 0)
			{
				Con_DPrintf("Incoming compressed file fragment disallowed from\n");
				return FALSE;
			}
			// compressed data is expected only after requesting resource list
			else if (host_client->m_sendrescount == 0)
			{
				Con_DPrintf("%s:Incoming compressed file fragment disallowed from %s\n", NET_AdrToString(chan->remote_address), host_client->name);
				return FALSE;
			}
		}
#endif

		uncompressedSize = clamp(uncompressedSize, 16u, (unsigned)sv_net_incoming_decompression_max_size.value); // valid range (16 - 65536) bytes

		qboolean success = TRUE;
		unsigned char *uncompressedBuffer = (unsigned char *)Mem_Malloc(uncompressedSize);
		unsigned int compressedSize = nsize;

		// Decompress net buffer data
		if (BZ2_bzBuffToBuffDecompress((char *)uncompressedBuffer, &uncompressedSize, (char *)buffer, compressedSize, 1, 0) != BZ_OK)
		{
			// malformed data or compressed data exceeding sv_net_incoming_decompression_max_size
			success = FALSE;
		}
		else if (!Netchan_ValidateDecompress(chan, FRAG_FILE_STREAM, compressedSize, uncompressedSize))
		{
			success = FALSE;
		}

		Mem_Free(buffer);

		if (!success)
		{
			// Drop client if decompression was unsuccessful
			SV_DropClient(host_client, FALSE, "Malformed/abnormal compressed data");
			SZ_Clear(&net_message);
			Mem_Free(uncompressedBuffer);
			return FALSE;
		}

		Con_DPrintf("Decompressing file %s (%d -> %d)\n", filename, compressedSize, uncompressedSize);
		pos = uncompressedSize;
		buffer = uncompressedBuffer;
	}

	if (filename[0] == '!')
	{
		if (chan->tempbuffer)
		{
			Con_DPrintf("Netchan_CopyFragments:  Freeing holdover tempbuffer\n");
			Mem_Free(chan->tempbuffer);
		}
		chan->tempbuffer = buffer;
		chan->tempbuffersize = pos;
	}
	else
	{
		char filedir[MAX_PATH];
		char *pszFileName;
		FileHandle_t handle;

#ifdef REHLDS_CHECKS
		Q_strncpy(filedir, filename, sizeof(filedir) - 1);
		filedir[sizeof(filedir) - 1] = 0;
#else
		Q_strncpy(filedir, filename, sizeof(filedir));
#endif // REHLDS_CHECKS
		COM_FixSlashes(filedir);
		pszFileName = Q_strrchr(filedir, '\\');
		if (pszFileName)
		{
			*pszFileName = 0;

#ifdef REHLDS_FIXES
			FS_CreateDirHierarchy(filedir, "GAMEDOWNLOAD");
#endif
		}

#ifndef REHLDS_FIXES
		FS_CreateDirHierarchy(filedir, "GAMEDOWNLOAD");
#endif
		handle = FS_OpenPathID(filename, "wb", "GAMEDOWNLOAD");
		if (!handle)
		{
			Con_Printf("File open failed %s\n", filename);
			Netchan_FlushIncoming(chan, FRAG_FILE_STREAM);

#ifdef REHLDS_FIXES
			Mem_Free(buffer);
#endif
			return FALSE;
		}

		Sys_Printf("COM_WriteFile: %s\n", filename);
		FS_Write(buffer, pos, 1, handle);
		FS_Close(handle);

		Mem_Free(buffer);
	}
	SZ_Clear(&net_message);

// Moved above
#ifndef REHLDS_FIXES
	chan->incomingbufs[FRAG_FILE_STREAM] = nullptr;
	chan->incomingready[FRAG_FILE_STREAM] = FALSE;
#endif

	msg_readcount = 0;
	return TRUE;
}

NOXREF qboolean Netchan_IsSending(netchan_t *chan)
{
	NOXREFCHECK;
	int i;
	for (i = 0; i < MAX_STREAMS; i++)
	{
		if (chan->fragbufs[i])
			return TRUE;
	}
	return FALSE;
}

NOXREF qboolean Netchan_IsReceiving(netchan_t *chan)
{
	NOXREFCHECK;
	int i;
	for (i = 0; i < MAX_STREAMS; i++)
	{
		if (chan->incomingbufs[i])
			return TRUE;
	}
	return FALSE;
}

qboolean Netchan_IncomingReady(netchan_t *chan)
{
	for (int i = 0; i < MAX_STREAMS; i++)
	{
		if (chan->incomingready[i])
			return TRUE;
	}
	return FALSE;
}

NOXREF void Netchan_UpdateProgress(netchan_t *chan)
{
	NOXREFCHECK;
	fragbuf_t *p;
	int c = 0;
	int total = 0;
	int i;
	float bestpercent = 0.0f;
	float percent;
	char sz[1024];
	char *in;
	char *out;
	int len = 0;

	scr_downloading.value = -1.0f;
	gDownloadFile[0] = 0;

	if (net_drawslider.value != 1.0f)
	{
		if (!chan->incomingbufs[FRAG_FILE_STREAM])
			return;
	}
	for (i = MAX_STREAMS - 1; i >= 0; i--)
	{
		if (chan->incomingbufs[i])
		{
			p = chan->incomingbufs[i];
			total = FRAG_GETCOUNT(p->bufferid);

			while (p)
			{
				c++;
				p = p->next;
			}

			if (total)
			{
				percent = (float)c * 100.0f / (float)total;
				if (percent > bestpercent)
					bestpercent = percent;
			}
			p = chan->incomingbufs[i];
			if (i == FRAG_FILE_STREAM)
			{
				in = (char *)p->frag_message.data;
				out = sz;

				while (*in)
				{
					*out++ = *in++;
					len++;

					if (len > 128)
						break;
				}

				*out = '\0';
				if (Q_strlen(sz) > 0 && (*sz != '!' || !Q_strncmp(sz, "!ModuleC.dll", 11)))
				{
					Q_strncpy(gDownloadFile, sz, 255);
					gDownloadFile[255] = 0;
				}
			}
		}
		else if (chan->fragbufs[i])
		{
			if (chan->fragbufcount[i])
			{
				percent = (float)chan->fragbufs[i]->bufferid * 100.0f / (float)chan->fragbufcount[i];
				if (percent > bestpercent)
					bestpercent = percent;
			}
		}
	}

	scr_downloading.value = bestpercent;
}

void Netchan_Init(void)
{
	Cvar_RegisterVariable(&net_log);
	Cvar_RegisterVariable(&net_showpackets);
	Cvar_RegisterVariable(&net_showdrop);
	Cvar_RegisterVariable(&net_chokeloopback);
	Cvar_RegisterVariable(&net_drawslider);
	Cvar_RegisterVariable(&sv_filetransfercompression);
#ifdef REHLDS_FIXES
	Cvar_RegisterVariable(&sv_net_incoming_decompression);
	Cvar_RegisterVariable(&sv_net_incoming_decompression_max_ratio);
	Cvar_RegisterVariable(&sv_net_incoming_decompression_max_size);
	Cvar_RegisterVariable(&sv_net_incoming_decompression_min_failures);
	Cvar_RegisterVariable(&sv_net_incoming_decompression_max_failures);
	Cvar_RegisterVariable(&sv_net_incoming_decompression_min_failuretime);
	Cvar_RegisterVariable(&sv_net_incoming_decompression_punish);
#endif
	Cvar_RegisterVariable(&sv_filetransfermaxsize);
}

NOXREF qboolean Netchan_CompressPacket(sizebuf_t *chan)
{
	NOXREFCHECK;
	return FALSE;
}

NOXREF qboolean Netchan_DecompressPacket(sizebuf_t *chan)
{
	NOXREFCHECK;
	return FALSE;
}
