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

static bool IsReplicated(const cvar_t *var)
{
	return (var->flags & FCVAR_REPLICATED) && !(var->flags & FCVAR_PROTECTED);
}

void SV_WriteNetSetConVar(sizebuf_t *sb, const cvar_t *var)
{
	MSG_BeginNetMessage(sb, NET_SETCONVAR);
	MSG_WriteVarInt32(sb, 1);
	MSG_WriteString(sb, var->name);
	MSG_WriteString(sb, var->string);
	MSG_EndNetMessage(sb);
}

void SV_WriteReplicatedCvars(sizebuf_t *sb)
{
	uint32 count = 0;
	for (cvar_t *var = cvar_vars; var; var = var->next)
	{
		if (IsReplicated(var))
			count++;
	}

	if (!count)
		return;

	MSG_BeginNetMessage(sb, NET_SETCONVAR);
	MSG_WriteVarInt32(sb, count);

	for (cvar_t *var = cvar_vars; var; var = var->next)
	{
		if (!IsReplicated(var))
			continue;

		MSG_WriteString(sb, var->name);
		MSG_WriteString(sb, var->string);
	}

	MSG_EndNetMessage(sb);
}

void SV_BroadcastReplicatedCvar(const cvar_t *var)
{
	if (!IsReplicated(var))
		return;

	for (int i = 0; i < g_psvs.maxclients; i++)
	{
		client_t *client = &g_psvs.clients[i];
		if (!client->connected || client->fakeclient)
			continue;

		if (!client->pGameClient->HasProtocolExtension(PROTOCOL_EXT_REPLICATEDCVARS))
			continue;

		SV_WriteNetSetConVar(&client->netchan.message, var);
	}
}
