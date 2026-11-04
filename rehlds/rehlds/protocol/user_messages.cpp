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

CClientUserMessages g_ClientUserMessages;

bool CClientUserMessages::Register(const char *name, ClientUserMsgHandler_t handler)
{
	if (!name || !name[0] || Q_strlen(name) >= MAX_USERMESSAGES_LENGTH || !handler)
	{
		Con_Printf("%s: invalid user message \"%s\"\n", __func__, name ? name : "");
		return false;
	}

	for (const handler_t &entry : m_Handlers)
	{
		if (!Q_strcmp(entry.name, name))
		{
			Con_Printf("%s: user message %s already has a handler\n", __func__, name);
			return false;
		}
	}

	handler_t entry;
	Q_strlcpy(entry.name, name);
	entry.handler = handler;
	m_Handlers.push_back(entry);

	return true;
}

bool CClientUserMessages::Unregister(const char *name, ClientUserMsgHandler_t handler)
{
	if (!name)
		return false;

	for (size_t i = 0; i < m_Handlers.size(); i++)
	{
		if (!Q_strcmp(m_Handlers[i].name, name) && m_Handlers[i].handler == handler)
		{
			m_Handlers.erase(m_Handlers.begin() + i);
			return true;
		}
	}

	return false;
}

bool CClientUserMessages::Dispatch(IGameClient *client, const char *name, const uint8 *data, int size) const
{
	for (const handler_t &entry : m_Handlers)
	{
		if (Q_strcmp(entry.name, name))
			continue;

		// handler may unregister itself, which invalidates the entry
		ClientUserMsgHandler_t handler = entry.handler;
		handler(client, data, size);
		return true;
	}

	return false;
}

bool SV_WriteUserMessage(sizebuf_t *sb, const char *name, const void *data, int size)
{
	int length = 1 + Q_strlen(name) + 1 + MSG_ByteSizeVarInt32(size) + size;
	if (!SZ_HasSpace(sb, length))
		return false;

	MSG_WriteByte(sb, svc_usermessage);
	MSG_WriteString(sb, name);
	MSG_WriteVarInt32(sb, size);
	SZ_Write(sb, data, size);

	return true;
}
