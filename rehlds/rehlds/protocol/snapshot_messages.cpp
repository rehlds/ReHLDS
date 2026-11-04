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

CSnapshotMessages::CSnapshotMessages()
{
	m_Data = NULL;
	m_Capacity = 0;

	Clear();
}

CSnapshotMessages::~CSnapshotMessages()
{
	Mem_Free(m_Data);
}

bool CSnapshotMessages::Add(const char *name, const void *data, int size, int maxBytes)
{
	if (!name || !name[0] || Q_strlen(name) >= MAX_USERMESSAGES_LENGTH || size < 0 || (size && !data))
		return false;

	if (m_MessageCount >= MAX_SNAPSHOT_MESSAGES)
		return false;

	const int wireSize = 1 + Q_strlen(name) + 1 + MSG_ByteSizeVarInt32(size) + size;
	if (m_Size + wireSize > maxBytes)
		return false;

	// Room for a whole packet of the client, taken once
	if (maxBytes > m_Capacity)
	{
		m_Data = (byte *)Mem_Realloc(m_Data, maxBytes);
		m_Capacity = maxBytes;
	}

	sizebuf_t buf;
	buf.buffername = "Snapshot Messages";
	buf.flags = SIZEBUF_ALLOW_OVERFLOW;
	buf.data = m_Data + m_Size;
	buf.maxsize = m_Capacity - m_Size;
	buf.cursize = 0;

	if (!SV_WriteUserMessage(&buf, name, data, size))
		return false;

	m_Size += buf.cursize;
	m_MessageEnds[m_MessageCount++] = m_Size;
	return true;
}

void CSnapshotMessages::Emit(sizebuf_t *msg, int room, const char *clientName)
{
	// The longest run of whole messages from the first that fits
	int count = m_MessageCount;
	while (count > 0 && m_MessageEnds[count - 1] > room)
		count--;

	if (count > 0)
		SZ_Write(msg, m_Data, m_MessageEnds[count - 1]);

	if (count < m_MessageCount)
		Con_DPrintf("Warning: %d snapshot messages of %s dropped, the packet is full\n", m_MessageCount - count, clientName);

	Clear();
}

void CSnapshotMessages::Clear()
{
	m_Size = 0;
	m_MessageCount = 0;
}
