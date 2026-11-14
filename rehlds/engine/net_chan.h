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
#include "cvar.h"
#include "net.h"

extern char gDownloadFile[256];

extern int net_drop;
extern cvar_t net_log;
extern cvar_t net_showpackets;
extern cvar_t net_showdrop;
extern cvar_t net_drawslider;
extern cvar_t net_chokeloopback;
extern cvar_t sv_filetransfercompression;
extern cvar_t sv_filetransfermaxsize;

void Netchan_UnlinkFragment(fragbuf_t *buf, fragbuf_t **list);
void Netchan_OutOfBand(netsrc_t sock, netadr_t adr, int length, byte *data);
void Netchan_OutOfBandPrint(netsrc_t sock, netadr_t adr, char *format, ...);
void Netchan_ClearFragbufs(fragbuf_t **ppbuf);
void Netchan_ClearFragments(netchan_t *chan);
void Netchan_Clear(netchan_t *chan);
void Netchan_Setup(netsrc_t socketnumber, netchan_t *chan, netadr_t adr, int player_slot, void *connection_status, qboolean(*pfnNetchan_Blocksize)(void *));
qboolean Netchan_CanPacket(netchan_t *chan);
void Netchan_UpdateFlow(netchan_t *chan);
void Netchan_Transmit(netchan_t *chan, int length, byte *data);
fragbuf_t *Netchan_FindBufferById(fragbuf_t **pplist, int id, qboolean allocate);
void Netchan_CheckForCompletion(netchan_t *chan, int stream, int intotalbuffers);
qboolean Netchan_Validate(netchan_t *chan, qboolean *frag_message, unsigned int *fragid, int *frag_offset, int *frag_length);
qboolean Netchan_Process(netchan_t *chan);
void Netchan_FragSend(netchan_t *chan);
void Netchan_AddBufferToList(fragbuf_t **pplist, fragbuf_t *pbuf);
fragbuf_t *Netchan_AllocFragbuf(void);
void Netchan_AddFragbufToTail(fragbufwaiting_t *wait, fragbuf_t *buf);
void Netchan_CreateFragments_(qboolean server, netchan_t *chan, sizebuf_t *msg);
void Netchan_CreateFragments(qboolean server, netchan_t *chan, sizebuf_t *msg);
int Netchan_CreateFileFragmentsFromBuffer(qboolean server, netchan_t *chan, const char *filename, unsigned char *uncompressed_pbuf, int uncompressed_size);
int Netchan_CreateFileFragments(qboolean server, netchan_t *chan, const char *filename);
// TRUE if a transfer of this file is already queued or in flight on the file
// stream, so a duplicate request must not queue it again (issue #1200).
qboolean Netchan_IsFileTransferActive(netchan_t *chan, const char *filename);
#ifdef REHLDS_FIXES
int Netchan_CreateFileFragments_(qboolean server, netchan_t *chan, const char *filename);
#endif // REHLDS_FIXES
void Netchan_FlushIncoming(netchan_t *chan, int stream);
qboolean Netchan_CopyNormalFragments(netchan_t *chan);
qboolean Netchan_CopyFileFragments(netchan_t *chan);
NOXREF qboolean Netchan_IsSending(netchan_t *chan);
NOXREF qboolean Netchan_IsReceiving(netchan_t *chan);
qboolean Netchan_IncomingReady(netchan_t *chan);
NOXREF void Netchan_UpdateProgress(netchan_t *chan);
void Netchan_Init(void);
NOXREF qboolean Netchan_CompressPacket(sizebuf_t *chan);
NOXREF qboolean Netchan_DecompressPacket(sizebuf_t *chan);
