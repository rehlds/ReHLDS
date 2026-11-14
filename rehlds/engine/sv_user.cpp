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

sv_adjusted_positions_t truepositions[MAX_CLIENTS];
qboolean g_balreadymoved;

float s_LastFullUpdate[33];

edict_t *sv_player;

qboolean nofind;

#if defined(SWDS) && defined(REHLDS_FIXES)
const char *clcommands[] = { "status", "name", "kill", "pause", "spawn", "new", "sendres", "dropclient", "kick", "ping", "dlfile", "setinfo", "sendents", "fullupdate", "setpause", "unpause", "noclip", "god", "notarget", NULL };
#else
const char *clcommands[23] = { "status", "god", "notarget", "fly", "name", "noclip", "kill", "pause", "spawn", "new", "sendres", "dropclient", "kick", "ping", "dlfile", "nextdl", "setinfo", "showinfo", "sendents", "fullupdate", "setpause", "unpause", NULL };
#endif

cvar_t sv_edgefriction = { "edgefriction", "2", FCVAR_SERVER, 0.0f, NULL };
cvar_t sv_maxspeed = { "sv_maxspeed", "320", FCVAR_SERVER, 0.0f, NULL };
cvar_t sv_accelerate = { "sv_accelerate", "10", FCVAR_SERVER, 0.0f, NULL };
cvar_t sv_footsteps = { "mp_footsteps", "1", FCVAR_SERVER, 0.0f, NULL };
cvar_t sv_rollspeed = { "sv_rollspeed", "0.0", 0, 0.0f, NULL };
cvar_t sv_rollangle = { "sv_rollangle", "0.0", 0, 0.0f, NULL };
cvar_t sv_unlag = { "sv_unlag", "1", 0, 0.0f, NULL };
cvar_t sv_maxunlag = { "sv_maxunlag", "0.5", 0, 0.0f, NULL };
cvar_t sv_unlagpush = { "sv_unlagpush", "0.0", 0, 0.0f, NULL };
cvar_t sv_unlagsamples = { "sv_unlagsamples", "1", 0, 0.0f, NULL };
cvar_t mp_consistency = { "mp_consistency", "1", FCVAR_SERVER, 0.0f, NULL };
cvar_t sv_voiceenable = { "sv_voiceenable", "1", FCVAR_SERVER | FCVAR_ARCHIVE, 0.0f, NULL };

clc_func_t sv_clcfuncs[] = {
	{ clc_bad,             "clc_bad",             nullptr                      },
	{ clc_nop,             "clc_nop",             nullptr                      },
	{ clc_move,            "clc_move",            &SV_ParseMove                },
	{ clc_stringcmd,       "clc_stringcmd",       &SV_ParseStringCommand       },
	{ clc_delta,           "clc_delta",           &SV_ParseDelta               },
	{ clc_resourcelist,    "clc_resourcelist",    &SV_ParseResourceList        },
	{ clc_tmove,           "clc_tmove",           nullptr                      },
	{ clc_fileconsistency, "clc_fileconsistency", &SV_ParseConsistencyResponse },
	{ clc_voicedata,       "clc_voicedata",       &SV_ParseVoiceData           },
	{ clc_hltv,            "clc_hltv",            &SV_IgnoreHLTV               },
	{ clc_cvarvalue,       "clc_cvarvalue",       &SV_ParseCvarValue           },
	{ clc_cvarvalue2,      "clc_cvarvalue2",      &SV_ParseCvarValue2          },
	{ clc_endoflist,       "End of List",         nullptr                      },
};

// Layout of resource_t::rguc_reserved for a model with a bounds check:
// the force type byte, then mins and maxs as three floats each
const int CONSISTENCY_MINS_OFFSET = 1;
const int CONSISTENCY_MAXS_OFFSET = 13;

// Resource list the consistency check works on
resource_t *SV_GetResourceList()
{
#ifdef REHLDS_FIXES
	return g_rehlds_sv.resources;
#else // REHLDS_FIXES
	return g_psv.resourcelist;
#endif // REHLDS_FIXES
}

void SV_GetConsistencyBounds(const unsigned char *reserved, vec3_t mins, vec3_t maxs)
{
	Q_memcpy(mins, &reserved[CONSISTENCY_MINS_OFFSET], sizeof(vec3_t));
	Q_memcpy(maxs, &reserved[CONSISTENCY_MAXS_OFFSET], sizeof(vec3_t));
}

void SV_SetConsistencyBounds(unsigned char *reserved, const vec_t *mins, const vec_t *maxs)
{
	Q_memcpy(&reserved[CONSISTENCY_MINS_OFFSET], mins, sizeof(vec3_t));
	Q_memcpy(&reserved[CONSISTENCY_MAXS_OFFSET], maxs, sizeof(vec3_t));
}

bool EXT_FUNC SV_CheckConsistencyResponse_API(IGameClient *client, resource_t *res, uint32 hash)
{
	return (hash != *(uint32 *)&res->rgucMD5_hash[0]);
}

// Reads the model bounds of a resource and compares them with the forced ones.
// Returns true when the client violates them. An unknown force type flags a bad read.
bool SV_ReadModelBoundsResponse(const resource_t *r)
{
	vec3_t cmins;
	vec3_t cmaxs;
	MSG_ReadBitData(cmins, 12);
	MSG_ReadBitData(cmaxs, 12);

	// Requirement is stored scrambled
	unsigned char required[sizeof(r->rguc_reserved)];
	Q_memcpy(required, r->rguc_reserved, sizeof(required));
	COM_UnMunge(required, sizeof(required), g_psvs.spawncount);

	vec3_t mins;
	vec3_t maxs;
	SV_GetConsistencyBounds(required, mins, maxs);

	switch ((FORCE_TYPE)required[0])
	{
	case force_model_samebounds:
		return !VectorCompare(cmins, mins) || !VectorCompare(cmaxs, maxs);

	case force_model_specifybounds:
		return BoundsExceed(cmins, cmaxs, mins, maxs);

	case force_model_specifybounds_if_avail:
		// Bounds of -1 on every axis are not checked
		if (cmins[0] == -1.0f && cmins[1] == -1.0f && cmins[2] == -1.0f &&
			cmaxs[0] == -1.0f && cmaxs[1] == -1.0f && cmaxs[2] == -1.0f)
			return false;

		return BoundsExceed(cmins, cmaxs, mins, maxs);

	default:
		msg_badread = TRUE;
		return true;
	}
}

// Parses the consistency info sent by the client to find out how many resources differ or are missing
void SV_ParseConsistencyResponse(client_t *pSenderClient)
{
	int length = MSG_ReadShort();

	if (length <= 0 || !SZ_HasSomethingToRead(&net_message, length))
	{
		msg_badread = TRUE;
		Con_DPrintf("%s:  %s:%s invalid length: %d\n", __func__, host_client->name, NET_AdrToString(host_client->netchan.remote_address), length);
		SV_DropClient(host_client, FALSE, "Invalid length");
		return;
	}

	COM_UnMunge(&net_message.data[msg_readcount], length, g_psvs.spawncount);
	MSG_StartBitReading(&net_message);

	resource_t *resources = SV_GetResourceList();
	unsigned char nullbuffer[sizeof(resource_t::rguc_reserved)] = {};

	// 0 - all matches, -1 - malformed data, otherwise the index of the last differing resource + 1
	int dropclient = 0;
	int count = 0;

	// Read consistency info
	while (MSG_ReadBits(1))
	{
		int idx = MSG_ReadBits(12);
		if (idx < 0 || idx >= g_psv.num_resources)
		{
			dropclient = -1;
			break;
		}

		resource_t *r = &resources[idx];
		if (!(r->ucFlags & RES_CHECKFILE))
		{
			dropclient = -1;
			break;
		}

		// Models carry the bounds requirement in the reserved block, other resources are checked by their hash only
		bool differs;
		if (!Q_memcmp(r->rguc_reserved, nullbuffer, sizeof(nullbuffer)))
		{
			uint32 hash = MSG_ReadBits(32);
			differs = g_RehldsHookchains.m_SV_CheckConsistencyResponse.callChain(SV_CheckConsistencyResponse_API, pSenderClient->pGameClient, r, hash);
		}
		else
		{
			differs = SV_ReadModelBoundsResponse(r);
		}

		if (differs)
			dropclient = idx + 1;

		if (msg_badread)
			break;

		++count;
	}

	MSG_EndBitReading(&net_message);

	if (dropclient < 0 || count != g_psv.num_consistency)
	{
		msg_badread = TRUE;
		Con_Printf("SV_ParseConsistencyResponse:  %s:%s sent bad file data\n", host_client->name, NET_AdrToString(host_client->netchan.remote_address));
		SV_DropClient(host_client, FALSE, "Bad file data");
		return;
	}

	if (dropclient > 0)
	{
		const char *filename = resources[dropclient - 1].szFileName;
		char dropmessage[256]{};

#ifdef REHLDS_FIXES
		if (gEntityInterface.pfnInconsistentFile(host_client->edict, filename, dropmessage))
		{
			if (dropmessage[0])
				SV_ClientPrintf("%s", dropmessage);

			SV_DropClient(host_client, FALSE, "Bad file %s", filename); // only filename. reason was printed in console if exists.
			return;
		}
#else // REHLDS_FIXES
		if (gEntityInterface.pfnInconsistentFile(host_client->edict, filename, dropmessage))
		{
			if (Q_strlen(dropmessage) > 0)
				SV_ClientPrintf("%s", dropmessage);

			SV_DropClient(host_client, FALSE, "Bad file %s", dropmessage);
		}
		return;
#endif // REHLDS_FIXES
	}

	// Reset has_force_unmodified if we have received response from the client
	host_client->has_force_unmodified = FALSE;
}

qboolean EXT_FUNC SV_FileInConsistencyList(const char *filename, consistency_t **ppconsist)
{
	for (int i = 0; i < ARRAYSIZE(g_psv.consistency_list); i++)
	{
		consistency_t &entry = g_psv.consistency_list[i];

		// the list ends with the first empty record
		if (!entry.filename)
			return FALSE;

		if (!Q_stricmp(filename, entry.filename))
		{
			if (ppconsist)
				*ppconsist = &entry;

			return TRUE;
		}
	}

	return FALSE;
}

int SV_TransferConsistencyInfo(void)
{
	return g_RehldsHookchains.m_SV_TransferConsistencyInfo.callChain(SV_TransferConsistencyInfo_internal);
}

// Stores the bounds requirement of a model resource in its reserved block, scrambled for the wire.
// Types without bounds leave the block untouched.
void SV_SetupModelConsistency(resource_t *r, const consistency_t *pc, const char *filename)
{
	if (pc->check_type == force_model_samebounds)
	{
		vec3_t mins;
		vec3_t maxs;

		if (!R_GetStudioBounds(filename, mins, maxs))
			Host_Error("%s: Server unable to get bounds for %s\n", __func__, filename);

		SV_SetConsistencyBounds(r->rguc_reserved, mins, maxs);
	}
	else if (pc->check_type == force_model_specifybounds || pc->check_type == force_model_specifybounds_if_avail)
	{
		SV_SetConsistencyBounds(r->rguc_reserved, pc->mins, pc->maxs);
	}
	else
	{
		return;
	}

	r->rguc_reserved[0] = pc->check_type;
	COM_Munge(r->rguc_reserved, sizeof(r->rguc_reserved), g_psvs.spawncount);
}

int EXT_FUNC SV_TransferConsistencyInfo_internal(void)
{
	resource_t *resources = SV_GetResourceList();

	int count = 0;
	for (int i = 0; i < g_psv.num_resources; i++)
	{
		resource_t *r = &resources[i];
		if (r->ucFlags == (RES_CUSTOM | RES_REQUESTED | RES_UNK_6) || (r->ucFlags & RES_CHECKFILE))
			continue;

		consistency_t *pc;
		if (!SV_FileInConsistencyList(r->szFileName, &pc))
			continue;

		r->ucFlags |= RES_CHECKFILE;

		char filename[MAX_PATH];
		if (r->type == t_sound)
		{
			Q_snprintf(filename, MAX_PATH, "sound/%s", r->szFileName);
		}
		else
		{
			Q_strlcpy(filename, r->szFileName);
		}
		MD5_Hash_File(r->rgucMD5_hash, filename, FALSE, FALSE, NULL);

		if (r->type == t_model)
			SV_SetupModelConsistency(r, pc, filename);

		++count;
	}

	return count;
}

// Consistency data goes only to the real clients of a multiplayer server,
// and only when mp_consistency is on or the caller forces it
bool EXT_FUNC SV_ShouldSendConsistencyList_mod(IGameClient *cl, bool forceConsistency)
{
	if (g_psvs.maxclients == 1 || g_psv.num_consistency == 0 || cl->IsProxy())
		return false;

	if (!forceConsistency && !mp_consistency.value)
		return false;

	return true;
}

bool SV_ShouldSendConsistencyList(client_t *client, bool forceConsistency)
{
	return g_RehldsHookchains.m_SV_ShouldSendConsistencyList.callChain(SV_ShouldSendConsistencyList_mod, client->pGameClient, forceConsistency);
}

// Sends the indices of the resources the client must check.
// Each index is the distance from the previous one when it fits 5 bits, otherwise the absolute value in 10 bits.
void SV_SendConsistencyList(sizebuf_t *msg)
{
	host_client->has_force_unmodified = FALSE;

	if (!SV_ShouldSendConsistencyList(host_client, false))
	{
		MSG_WriteBits(0, 1);
		return;
	}

	host_client->has_force_unmodified = TRUE;

	MSG_WriteBits(1, 1);

	const resource_t *resources = SV_GetResourceList();
	int lastcheck = 0;

	for (int i = 0; i < g_psv.num_resources; i++)
	{
		if (!(resources[i].ucFlags & RES_CHECKFILE))
			continue;

		MSG_WriteBits(1, 1);

		int delta = i - lastcheck;
		if (delta > 31)
		{
			MSG_WriteBits(0, 1);
			MSG_WriteBits(i, 10);	// LIMIT: Here it write index, not a diff, with resolution of 10 bits. So, it limits not adjacent index to 1023 max.
		}
		else
		{
			// Write 5 bits index delta, so it is up to 31
			MSG_WriteBits(1, 1);
			MSG_WriteBits(delta, 5);
		}

		lastcheck = i;
	}

	MSG_WriteBits(0, 1);
}

void SV_PreRunCmd(void)
{
}

// Copies the state of an edict that the player movement code needs
void SV_CopyEdictToPhysent(physent_t *pe, int e, edict_t *check)
{
	VectorCopy(check->v.origin, pe->origin);

	pe->info = e;

	if (e <= 0 || e > g_psvs.maxclients)
	{
		pe->player = 0;
	}
	else
	{
		SV_GetTrueOrigin(e - 1, pe->origin);
		pe->player = pe->info;
	}

	VectorCopy(check->v.angles, pe->angles);

	pe->studiomodel = nullptr;
	pe->rendermode = check->v.rendermode;

	switch (check->v.solid)
	{
	case SOLID_BSP:
		pe->model = g_psv.models[check->v.modelindex];
		Q_strlcpy(pe->name, pe->model->name);
		break;

	case SOLID_NOT:
		if (check->v.modelindex)
		{
			pe->model = g_psv.models[check->v.modelindex];
			Q_strlcpy(pe->name, pe->model->name);
		}
		else
		{
			pe->model = nullptr;
		}
		break;

	case SOLID_BBOX:
		pe->model = nullptr;

		if (check->v.modelindex)
		{
			model_t *pModel = g_psv.models[check->v.modelindex];
			if (pModel)
			{
				if (pModel->flags & STUDIO_TRACE_HITBOX)
					pe->studiomodel = pModel;

				Q_strlcpy(pe->name, pModel->name);
			}
		}

		VectorCopy(check->v.mins, pe->mins);
		VectorCopy(check->v.maxs, pe->maxs);
		break;

	default:
		pe->model = nullptr;

		VectorCopy(check->v.mins, pe->mins);
		VectorCopy(check->v.maxs, pe->maxs);

		if (check->v.classname)
			Q_strlcpy(pe->name, &pr_strings[check->v.classname]);
		else
			Q_sprintf(pe->name, "?");
		break;
	}

	pe->skin = check->v.skin;
	pe->frame = check->v.frame;
	pe->solid = check->v.solid;
	pe->sequence = check->v.sequence;
	pe->movetype = check->v.movetype;

	Q_memcpy(&pe->controller[0], &check->v.controller[0], 4 * sizeof(byte));
	Q_memcpy(&pe->blending[0], &check->v.blending[0], 2 * sizeof(byte));

	pe->iuser1 = check->v.iuser1;
	pe->iuser2 = check->v.iuser2;
	pe->iuser3 = check->v.iuser3;
	pe->iuser4 = check->v.iuser4;
	pe->fuser1 = check->v.fuser1;
	pe->fuser2 = check->v.fuser2;
	pe->fuser3 = check->v.fuser3;
	pe->fuser4 = check->v.fuser4;

	VectorCopy(check->v.vuser1, pe->vuser1);
	VectorCopy(check->v.vuser2, pe->vuser2);
	VectorCopy(check->v.vuser3, pe->vuser3);
	VectorCopy(check->v.vuser4, pe->vuser4);

	pe->takedamage = DAMAGE_NO;
	pe->blooddecal = 0;
}

bool EXT_FUNC SV_AllowPhysent_mod(edict_t* check, edict_t* sv_player)
{
	return true;
}

bool SV_AllowPhysent(edict_t* check, edict_t* sv_player)
{
	return g_RehldsHookchains.m_SV_AllowPhysent.callChain(SV_AllowPhysent_mod, check, sv_player);
}

// The edict is hidden from the player by the group filter
bool SV_IsFilteredByGroup(edict_t *check)
{
	if (!check->v.groupinfo)
		return false;

	bool shared = (check->v.groupinfo & sv_player->v.groupinfo) != 0;

	if (g_groupop)
		return g_groupop == GROUP_OP_NAND && shared;

	return !shared;
}

// The visible edict takes part in the collisions of the player movement
bool SV_IsCollidableInPM(edict_t *check, int e, float *pmove_mins, float *pmove_maxs)
{
	if (check->v.solid == SOLID_NOT && (!check->v.skin || !check->v.modelindex))
		return false;

	if ((check->v.flags & FL_MONSTERCLIP) && check->v.solid == SOLID_BSP)
		return false;

	if (check == sv_player)
		return false;

	if ((check->v.flags & FL_CLIENT) && check->v.health <= 0.0)
		return false;

	if (check->v.mins[2] == 0.0 && check->v.maxs[2] == 1.0)
		return false;

	if (Length(check->v.size) == 0.0)
		return false;

	float *fmin = check->v.absmin;
	float *fmax = check->v.absmax;
	if (check->v.flags & FL_CLIENT)
		SV_GetTrueMinMax(e - 1, &fmin, &fmax);

	return BoundsIntersect(pmove_mins, pmove_maxs, fmin, fmax);
}

void SV_AddLinksToPM_(areanode_t *node, float *pmove_mins, float *pmove_maxs)
{
	link_t *next;

	for (link_t *l = node->solid_edicts.next; l != &node->solid_edicts; l = next)
	{
		edict_t *check = (edict_t *)&l[-1];
		next = l->next;

		if (SV_IsFilteredByGroup(check))
			continue;

		if (check->v.owner == sv_player)
			continue;

		if (check->v.solid != SOLID_BSP &&
			check->v.solid != SOLID_BBOX &&
			check->v.solid != SOLID_SLIDEBOX &&
			check->v.solid != SOLID_NOT)
			continue;

		// Apply our own custom checks
		if (!SV_AllowPhysent(check, sv_player))
			continue;

		int e = NUM_FOR_EDICT(check);

		// A full visible list does not keep the edict out of the collision lists
		physent_t *ve = nullptr;
		if (pmove->numvisent < MAX_PHYSENTS)
		{
			ve = &pmove->visents[pmove->numvisent++];
			SV_CopyEdictToPhysent(ve, e, check);
		}

		if (!SV_IsCollidableInPM(check, e, pmove_mins, pmove_maxs))
			continue;

		physent_t *pe;
		if (check->v.solid || check->v.skin != -16)
		{
			if (pmove->numphysent >= MAX_PHYSENTS)
			{
				Con_DPrintf("SV_AddLinksToPM:  pmove->numphysent >= MAX_PHYSENTS\n");
				return;
			}
			pe = &pmove->physents[pmove->numphysent++];
		}
		else
		{
			if (pmove->nummoveent >= MAX_MOVEENTS)
			{
				Con_DPrintf("SV_AddLinksToPM:  pmove->nummoveent >= MAX_MOVEENTS\n");
				continue;
			}
			pe = &pmove->moveents[pmove->nummoveent++];
		}

		if (ve)
			Q_memcpy(pe, ve, sizeof(physent_t));
		else
			SV_CopyEdictToPhysent(pe, e, check);
	}

	if (node->axis != -1)
	{
		if (pmove_maxs[node->axis] > node->dist)
			SV_AddLinksToPM_(node->children[0], pmove_mins, pmove_maxs);

		if (pmove_mins[node->axis] < node->dist)
			SV_AddLinksToPM_(node->children[1], pmove_mins, pmove_maxs);
	}
}

// Builds the lists of entities within reach of the player: the world first, then the solid edicts of the area tree
void SV_AddLinksToPM(areanode_t *node, vec_t *origin)
{
	const float reach = 256.0f;

	physent_t *pe = &pmove->physents[WORLD_ID];
	physent_t *ve = &pmove->visents[WORLD_ID];

	Q_memset(pe, 0, sizeof(*pe));
	Q_memset(ve, 0, sizeof(*ve));

	// Assign world model
	pe->model = g_psv.worldmodel;
	if (pe->model)
	{
		Q_strlcpy(pe->name, pe->model->name);
	}

	// setup physents
	pmove->numphysent = 1;
	pmove->numvisent = 1;
	pmove->nummoveent = 0;

	pe->info = 0;
	pe->solid = SOLID_BSP;
	pe->movetype = MOVETYPE_NONE;
	pe->takedamage = DAMAGE_YES;
	pe->blooddecal = 0;

	*ve = *pe;

	vec3_t pmove_mins, pmove_maxs;
	for (int i = 0; i < 3; i++)
	{
		pmove_mins[i] = origin[i] - reach;
		pmove_maxs[i] = origin[i] + reach;
	}

	SV_AddLinksToPM_(node, pmove_mins, pmove_maxs);
}

void SV_PlayerRunPreThink(edict_t *player, float time)
{
	gGlobalVariables.time = time;
	gEntityInterface.pfnPlayerPreThink(player);
}

// Runs the think function of the player entity when its time has come.
// Returns false when the entity has been freed.
qboolean SV_PlayerRunThink(edict_t *ent, float frametime, double clienttimebase)
{
	if (!(ent->v.flags & (FL_DORMANT | FL_KILLME)))
	{
		float thinktime = ent->v.nextthink;
		if (thinktime <= 0.0 || frametime + clienttimebase < thinktime)
			return TRUE;

		if (thinktime < clienttimebase)
			thinktime = (float)clienttimebase;

		ent->v.nextthink = 0;
		gGlobalVariables.time = thinktime;
		gEntityInterface.pfnThink(ent);
	}

	if (ent->v.flags & FL_KILLME)
		ED_Free(ent);

	return ent->free == 0;
}

// Applies the speed of a conveyor under the player to its base velocity
void SV_CheckMovingGround(edict_t *ent, float frametime)
{
	if ((ent->v.flags & FL_ONGROUND) && ent->v.groundentity && (ent->v.groundentity->v.flags & FL_CONVEYOR))
	{
		edict_t *groundentity = ent->v.groundentity;

		if (ent->v.flags & FL_BASEVELOCITY)
			VectorMA(ent->v.basevelocity, groundentity->v.speed, groundentity->v.movedir, ent->v.basevelocity);
		else
			VectorScale(groundentity->v.movedir, groundentity->v.speed, ent->v.basevelocity);

		ent->v.flags |= FL_BASEVELOCITY;
	}

	if (!(ent->v.flags & FL_BASEVELOCITY))
	{
		VectorMA(ent->v.velocity, frametime * 0.5f + 1.0f, ent->v.basevelocity, ent->v.velocity);
		VectorClear(ent->v.basevelocity);
	}

	ent->v.flags &= ~FL_BASEVELOCITY;
}

void SV_ConvertPMTrace(trace_t *dest, pmtrace_t *src, edict_t *ent)
{
	dest->allsolid = src->allsolid;
	dest->startsolid = src->startsolid;
	dest->inopen = src->inopen;
	dest->inwater = src->inwater;
	dest->fraction = src->fraction;
	VectorCopy(src->endpos, dest->endpos);
	VectorCopy(src->plane.normal, dest->plane.normal);
	dest->plane.dist = src->plane.dist;
	dest->hitgroup = src->hitgroup;
	dest->ent = ent;
}

// Sends the current state of every client to the one that is being processed
void SV_ForceFullClientsUpdate(void)
{
	byte data[9216];
	sizebuf_t msg;

	Q_memset(&msg, 0, sizeof(msg));
	msg.buffername = "Force Update";
	msg.data = data;
	msg.cursize = 0;
	msg.maxsize = sizeof(data);

	for (int i = 0; i < g_psvs.maxclients; ++i)
	{
		client_t *client = &g_psvs.clients[i];
		if (client == host_client || client->active || client->connected || client->spawned)
			SV_FullClientUpdate(client, &msg);
	}

	Con_DPrintf("Client %s started recording. Send full update.\n", host_client->name);
	Netchan_CreateFragments(1, &host_client->netchan, &msg);
	Netchan_FragSend(&host_client->netchan);
}

// Fills the player movement state from the entity and the command
void SV_SetupPMove(playermove_t *ppmove, client_t *client, edict_t *ent, const usercmd_t *cmd, float frametime)
{
	ppmove->server = TRUE;
	ppmove->multiplayer = (g_psvs.maxclients > 1) ? TRUE : FALSE;
	ppmove->time = (float)(1000.0 * client->svtimebase);
	ppmove->usehull = (ent->v.flags & FL_DUCKING) == FL_DUCKING;
	ppmove->maxspeed = sv_maxspeed.value;
	ppmove->clientmaxspeed = ent->v.maxspeed;
	ppmove->flDuckTime = (float)ent->v.flDuckTime;
	ppmove->bInDuck = ent->v.bInDuck;
	ppmove->flTimeStepSound = ent->v.flTimeStepSound;
	ppmove->iStepLeft = ent->v.iStepLeft;
	ppmove->flFallVelocity = ent->v.flFallVelocity;
	ppmove->flSwimTime = (float)ent->v.flSwimTime;
	ppmove->oldbuttons = ent->v.oldbuttons;

	Q_strlcpy(ppmove->physinfo, client->physinfo);
	VectorCopy(ent->v.velocity, ppmove->velocity);
	VectorCopy(ent->v.movedir, ppmove->movedir);
	VectorCopy(ent->v.v_angle, ppmove->angles);
	VectorCopy(ent->v.basevelocity, ppmove->basevelocity);
	VectorCopy(ent->v.view_ofs, ppmove->view_ofs);
	VectorCopy(ent->v.punchangle, ppmove->punchangle);

	ppmove->deadflag = ent->v.deadflag;
	ppmove->effects = ent->v.effects;
	ppmove->gravity = ent->v.gravity;
	ppmove->friction = ent->v.friction;
	ppmove->spectator = FALSE;
	ppmove->waterjumptime = ent->v.teleport_time;

	Q_memcpy(&ppmove->cmd, cmd, sizeof(ppmove->cmd));

	ppmove->dead = ent->v.health <= 0.0 ? TRUE : FALSE;
	ppmove->movetype = ent->v.movetype;
	ppmove->flags = ent->v.flags;
	ppmove->player_index = NUM_FOR_EDICT(ent) - 1;

	ppmove->iuser1 = ent->v.iuser1;
	ppmove->iuser2 = ent->v.iuser2;
	ppmove->iuser3 = ent->v.iuser3;
	ppmove->iuser4 = ent->v.iuser4;
	ppmove->fuser1 = ent->v.fuser1;
	ppmove->fuser2 = ent->v.fuser2;
	ppmove->fuser3 = ent->v.fuser3;
	ppmove->fuser4 = ent->v.fuser4;

	VectorCopy(ent->v.vuser1, ppmove->vuser1);
	VectorCopy(ent->v.vuser2, ppmove->vuser2);
	VectorCopy(ent->v.vuser3, ppmove->vuser3);
	VectorCopy(ent->v.vuser4, ppmove->vuser4);

	VectorCopy(ent->v.origin, ppmove->origin);

	SV_AddLinksToPM(sv_areanodes, ppmove->origin);

	ppmove->runfuncs = TRUE;
	ppmove->frametime = frametime;
	ppmove->PM_PlaySound = PM_SV_PlaySound;
	ppmove->PM_TraceTexture = PM_SV_TraceTexture;
	ppmove->PM_PlaybackEventFull = PM_SV_PlaybackEventFull;
}

// Copies the result of the player movement back to the entity
void SV_FinishPMove(playermove_t *ppmove, edict_t *ent)
{
	ent->v.deadflag = ppmove->deadflag;
	ent->v.effects = ppmove->effects;
	ent->v.teleport_time = ppmove->waterjumptime;
	ent->v.waterlevel = ppmove->waterlevel;
	ent->v.watertype = ppmove->watertype;
	ent->v.flags = ppmove->flags;
	ent->v.friction = ppmove->friction;
	ent->v.movetype = ppmove->movetype;
	ent->v.maxspeed = ppmove->clientmaxspeed;
	ent->v.iStepLeft = ppmove->iStepLeft;

	VectorCopy(ppmove->view_ofs, ent->v.view_ofs);
	VectorCopy(ppmove->movedir, ent->v.movedir);
	VectorCopy(ppmove->punchangle, ent->v.punchangle);

	if (ppmove->onground == -1)
	{
		ent->v.flags &= ~FL_ONGROUND;
	}
	else
	{
		ent->v.flags |= FL_ONGROUND;
		ent->v.groundentity = EDICT_NUM(ppmove->physents[ppmove->onground].info);
	}

	VectorCopy(ppmove->origin, ent->v.origin);
	VectorCopy(ppmove->velocity, ent->v.velocity);
	VectorCopy(ppmove->basevelocity, ent->v.basevelocity);

	// The model shows a 1/3 the pitch and the whole yaw and roll
	if (ent->v.fixangle == FIXANGLE_NONE)
	{
		VectorCopy(ppmove->angles, ent->v.v_angle);
		ent->v.angles[0] = (float)(-ppmove->angles[0] / 3.0);
		ent->v.angles[1] = ppmove->angles[1];
		ent->v.angles[2] = ppmove->angles[2];
	}

	ent->v.bInDuck = ppmove->bInDuck;
	ent->v.flDuckTime = (int)ppmove->flDuckTime;
	ent->v.flTimeStepSound = ppmove->flTimeStepSound;
	ent->v.flFallVelocity = ppmove->flFallVelocity;
	ent->v.flSwimTime = (int)ppmove->flSwimTime;
	ent->v.oldbuttons = ppmove->cmd.buttons;

	ent->v.iuser1 = ppmove->iuser1;
	ent->v.iuser2 = ppmove->iuser2;
	ent->v.iuser3 = ppmove->iuser3;
	ent->v.iuser4 = ppmove->iuser4;
	ent->v.fuser1 = ppmove->fuser1;
	ent->v.fuser2 = ppmove->fuser2;
	ent->v.fuser3 = ppmove->fuser3;
	ent->v.fuser4 = ppmove->fuser4;

	VectorCopy(ppmove->vuser1, ent->v.vuser1);
	VectorCopy(ppmove->vuser2, ent->v.vuser2);
	VectorCopy(ppmove->vuser3, ent->v.vuser3);
	VectorCopy(ppmove->vuser4, ent->v.vuser4);

	SetMinMaxSize(ent, player_mins[ppmove->usehull], player_maxs[ppmove->usehull], 0);
}

// Relinks the player and runs the touch functions of everything it hit during the movement
void SV_ProcessImpacts(client_t *client, edict_t *ent)
{
	if (!client->edict->v.solid)
		return;

	SV_LinkEdict(ent, TRUE);

	// The velocity of every touch is applied temporarily
	vec3_t oldvel;
	VectorCopy(ent->v.velocity, oldvel);

	for (int i = 0; i < pmove->numtouch; ++i)
	{
		pmtrace_t *tr = &pmove->touchindex[i];
		edict_t *touch = EDICT_NUM(pmove->physents[tr->ent].info);

		trace_t trace;
		SV_ConvertPMTrace(&trace, tr, touch);

		VectorCopy(tr->deltavelocity, ent->v.velocity);
		SV_Impact(touch, ent, &trace);
	}

	VectorCopy(oldvel, ent->v.velocity);
}

// Runs a single movement command of the player
void SV_RunCmd(usercmd_t* ucmd, int random_seed, qboolean fNetCmd, qboolean fChopped)
{
	usercmd_t cmd = *ucmd;

#ifdef REHLDS_FIXES
	// client could disconnect during CheckLimits or game-DLL callbacks
	if (!host_client->connected)
		return;
#endif // REHLDS_FIXES

	if (host_client->ignorecmdtime > realtime)
	{
		host_client->cmdtime = (double)ucmd->msec / 1000.0 + host_client->cmdtime;
		return;
	}

	host_client->ignorecmdtime = 0;

#ifdef REHLDS_FIXES
	if (fNetCmd			// analyze network packets only
		&& !fChopped
		&& !host_client->fakeclient)
	{
		if (g_UserCmdTimeLimiter.CheckLimits(host_client - g_psvs.clients, ucmd))
			return;
	}
#endif // REHLDS_FIXES

	// Long commands are run in two halves
	if (cmd.msec > 50)
	{
		cmd.msec = (byte)(ucmd->msec / 2.0);
		SV_RunCmd(&cmd, random_seed, fNetCmd, TRUE);
		cmd.msec = (byte)(ucmd->msec / 2.0);
		cmd.impulse = 0;
		SV_RunCmd(&cmd, random_seed, fNetCmd, TRUE);
		return;
	}

	if (!host_client->fakeclient)
		SV_SetupMove(host_client);

#ifdef REHLDS_FIXES
	if (sv_usercmd_custom_random_seed.value)
	{
		float fltTimeNow = (float)(Sys_FloatTime() * 1000.0);
		Q_memcpy(&random_seed, &fltTimeNow, sizeof(random_seed));
	}
#endif // REHLDS_FIXES

	gEntityInterface.pfnCmdStart(sv_player, ucmd, random_seed);

	float frametime = (float)(ucmd->msec * 0.001);
	host_client->svtimebase = frametime + host_client->svtimebase;
	host_client->cmdtime = ucmd->msec / 1000.0 + host_client->cmdtime;

	if (ucmd->impulse)
	{
		sv_player->v.impulse = ucmd->impulse;

		// impulse 204 forces a full update only without REHLDS_FIXES
#ifndef REHLDS_FIXES
		if (ucmd->impulse == 204)
			SV_ForceFullClientsUpdate();
#endif // REHLDS_FIXES
	}

	VectorClear(sv_player->v.clbasevelocity);

	sv_player->v.button = ucmd->buttons;
#ifdef REHLDS_FIXES
	sv_player->v.light_level = ucmd->lightlevel;
#endif // REHLDS_FIXES

	SV_CheckMovingGround(sv_player, frametime);

	VectorCopy(sv_player->v.v_angle, pmove->oldangles);

	// The game module can take over the view angles with fixangle
	if (sv_player->v.fixangle == FIXANGLE_NONE)
		VectorCopy(ucmd->viewangles, sv_player->v.v_angle);

	SV_PlayerRunPreThink(sv_player, (float)host_client->svtimebase);
	SV_PlayerRunThink(sv_player, frametime, host_client->svtimebase);

	// A conveyor or the think function can set the base velocity, the client needs it as well
	if (Length(sv_player->v.basevelocity) > 0)
		VectorCopy(sv_player->v.basevelocity, sv_player->v.clbasevelocity);

#ifdef REHLDS_FIXES
	const usercmd_t *pmcmd = ucmd;
#else // REHLDS_FIXES
	const usercmd_t *pmcmd = &cmd;
#endif // REHLDS_FIXES

	SV_SetupPMove(pmove, host_client, sv_player, pmcmd, frametime);

	const movevars_t movevars = host_client->pGameClient->GetMoveVars(); // preserve current movevars
	host_client->pGameClient->SetMoveVars(sv_movevars); // always use global movevars as a base
	pmove->movevars = host_client->pGameClient->GetMoveVarsForPlayerMove();

	gEntityInterface.pfnPM_Move(pmove, TRUE);

	// Determine whether movevars has changed or not
	if (!host_client->fakeclient && Q_memcmp(&movevars, pmove->movevars, sizeof(movevars)) != 0)
		SV_WriteMovevarsToClient(&host_client->netchan.message, pmove->movevars); // sync movevars for the client

	SV_FinishPMove(pmove, sv_player);
	SV_ProcessImpacts(host_client, sv_player);

	gGlobalVariables.time = (float)host_client->svtimebase;
	gGlobalVariables.frametime = frametime;
	gEntityInterface.pfnPlayerPostThink(sv_player);
	gEntityInterface.pfnCmdEnd(sv_player);

	if (!host_client->fakeclient)
		SV_RestoreMove(host_client);
}

// True for the commands the engine runs itself, false for the ones that go to the game module
bool SV_ValidateClientCommand(char *pszCommand)
{
	COM_Parse(pszCommand);

	for (int i = 0; clcommands[i] != NULL; i++)
	{
		if (!Q_stricmp(com_token, clcommands[i]))
			return true;
	}

	return false;
}

// Average ping of the latest frames. It is 0 while the pings of the very latest ones scatter by more than 0.2 s.
float SV_CalcClientTime(client_t *cl)
{
	int backtrack = (int)sv_unlagsamples.value;
	if (backtrack < 1)
		backtrack = 1;

	int maxsamples = min(SV_UPDATE_BACKUP, 16);
	if (backtrack >= maxsamples)
		backtrack = maxsamples;

	if (backtrack <= 0)
		return 0.0f;

	float ping = 0.0f;
	int count = 0;

	for (int i = 0; i < backtrack; i++)
	{
		client_frame_t *frame = &cl->frames[SV_UPDATE_MASK & (cl->netchan.incoming_acknowledged - i)];
		if (frame->ping_time <= 0.0f)
			continue;

		++count;
		ping += frame->ping_time;
	}

	if (!count)
		return 0.0f;

	ping /= count;

	float minping = 9999.0f;
	float maxping = -9999.0f;

	for (int i = 0; i < min(SV_UPDATE_BACKUP, 4); i++)
	{
		client_frame_t *frame = &cl->frames[SV_UPDATE_MASK & (cl->netchan.incoming_acknowledged - i)];
		if (frame->ping_time <= 0.0f)
			continue;

		if (frame->ping_time < minping)
			minping = frame->ping_time;

		if (frame->ping_time > maxping)
			maxping = frame->ping_time;
	}

	if (maxping < minping || fabs(maxping - minping) <= 0.2)
		return ping;

	return 0.0f;
}

void SV_ComputeLatency(client_t *cl)
{
	cl->latency = SV_CalcClientTime(cl);
}

// A distance of more than 128 units on any axis is a teleport
int SV_UnlagCheckTeleport(vec_t *v1, vec_t *v2)
{
	for (int i = 0; i < 3; i++)
	{
		if (fabs(v1[i] - v2[i]) > 128)
			return TRUE;
	}

	return FALSE;
}

// Position of a player that was moved by the lag compensation of the current client, NULL if it was not
const sv_adjusted_positions_t *SV_GetMovedPosition(int player)
{
	if (!host_client->lw || !host_client->lc)
		return NULL;

	if (!sv_unlag.value || g_psvs.maxclients <= 1 || !host_client->active)
		return NULL;

	if (player < 0 || player >= g_psvs.maxclients)
		return NULL;

	const sv_adjusted_positions_t *pos = &truepositions[player];
	if (!pos->active || !pos->needrelink)
		return NULL;

	return pos;
}

// Origin of a player before the lag compensation moved it
void SV_GetTrueOrigin(int player, vec_t *origin)
{
	const sv_adjusted_positions_t *pos = SV_GetMovedPosition(player);
	if (pos)
		VectorCopy(pos->oldorg, origin);
}

// Absolute bounds of a player before the lag compensation moved it
void SV_GetTrueMinMax(int player, float **fmin, float **fmax)
{
	const sv_adjusted_positions_t *pos = SV_GetMovedPosition(player);
	if (!pos)
		return;

	*fmin = (float *)pos->oldabsmin;
	*fmax = (float *)pos->oldabsmax;
}

entity_state_t *SV_FindEntInPack(int index, packet_entities_t *pack)
{
	for (int i = 0; i < pack->num_entities; i++)
	{
		if (pack->entities[i].number == index)
			return &pack->entities[i];
	}

	return NULL;
}

// The game module and the client allow positions to be rewound for this command
bool SV_IsLagCompensationActive(client_t *client)
{
	if (!gEntityInterface.pfnAllowLagCompensation())
		return false;

	if (!sv_unlag.value || g_psvs.maxclients <= 1)
		return false;

	return client->lw && client->lc && client->active;
}

// Remembers where the other players are right now
void SV_SaveTruePositions(client_t *client)
{
	for (int i = 0; i < g_psvs.maxclients; i++)
	{
		client_t *cl = &g_psvs.clients[i];
		if (cl == client || !cl->active)
			continue;

		sv_adjusted_positions_t *pos = &truepositions[i];

		VectorCopy(cl->edict->v.origin, pos->oldorg);
		VectorCopy(cl->edict->v.absmin, pos->oldabsmin);
		VectorCopy(cl->edict->v.absmax, pos->oldabsmax);
		pos->active = 1;
	}
}

// Time in the past the other players are rewound to: the latency of the client plus its interpolation delay
double SV_GetUnlagTargetTime(client_t *client)
{
	float latency = client->latency;
	if (latency > 1.5)
		latency = 1.5f;

	if (sv_maxunlag.value)
	{
		if (sv_maxunlag.value < 0)
			Cvar_SetValue("sv_maxunlag", 0);

		if (latency >= sv_maxunlag.value)
			latency = sv_maxunlag.value;
	}

	float cl_interptime = client->lastcmd.lerp_msec / 1000.0f;

	if (cl_interptime > 0.1)
		cl_interptime = 0.1f;

	if (client->next_messageinterval > cl_interptime)
		cl_interptime = (float)client->next_messageinterval;

#ifdef REHLDS_FIXES
	// FP Precision fix (targettime is double there, not float)
	double targettime = realtime - latency - cl_interptime + sv_unlagpush.value;
	if (targettime > realtime)
		targettime = realtime;
#else // REHLDS_FIXES
	float targettime = (float)(realtime - latency - cl_interptime + sv_unlagpush.value);
	if (targettime > realtime)
		targettime = (float)realtime;
#endif // REHLDS_FIXES

	return targettime;
}

// Flags the players of a frame that must not be interpolated: dead, flagged by the game module or teleported
void SV_FlagNonInterpolatedPlayers(client_frame_t *frame)
{
	for (int j = 0; j < frame->entities.num_entities; j++)
	{
		entity_state_t *state = &frame->entities.entities[j];

#ifdef REHLDS_OPT_PEDANTIC
		if (state->number <= 0)
			continue;

		if (state->number > g_psvs.maxclients)
			break; // players are always in the beginning of the list, no need to look more
#else
		if (state->number <= 0 || state->number > g_psvs.maxclients)
			continue;
#endif

		sv_adjusted_positions_t *pos = &truepositions[state->number - 1];
		if (pos->deadflag != DEAD_NO)
			continue;

		if (state->health <= 0 || (state->effects & EF_NOINTERP))
			pos->deadflag = DEAD_DYING;

		if (!pos->temp_org_setflag)
			pos->temp_org_setflag = 1;
		else if (SV_UnlagCheckTeleport(state->origin, pos->temp_org))
			pos->deadflag = DEAD_DYING;

		VectorCopy(state->origin, pos->temp_org);
	}
}

// Walks the sent frames back in time up to the first one that is not newer than targettime.
// That frame is returned as older and the one sent after it as newer (NULL if older is the latest).
// Returns false when no usable frame exists.
bool SV_FindUnlagFrames(client_t *client, double targettime, client_frame_t **older, client_frame_t **newer)
{
	if (SV_UPDATE_BACKUP <= 0)
		return false;

	client_frame_t *frame = NULL;
	client_frame_t *prev = NULL;
	int i;

	for (i = 0; i < SV_UPDATE_BACKUP; i++, prev = frame)
	{
		frame = &client->frames[SV_UPDATE_MASK & (client->netchan.outgoing_sequence - i - 1)];

		SV_FlagNonInterpolatedPlayers(frame);

		if (targettime > frame->senttime)
			break;
	}

	// The packets are too old to predict from
	if (i >= SV_UPDATE_BACKUP || targettime - frame->senttime > 1.0)
		return false;

	*older = frame;
	*newer = prev;
	return true;
}

// Position of targettime between two frames: 0 at the older one, 1 at the newer one
float SV_CalcUnlagFraction(const client_frame_t *older, const client_frame_t *newer, double targettime)
{
	float timeDiff = (float)(newer->senttime - older->senttime);
	if (timeDiff == 0.0)
		return 0.0f;

	float frac = (float)((targettime - older->senttime) / timeDiff);
	if (frac > 1.0)
		return 1.0f;

	if (frac < 0.0)
		return 0.0f;

	return frac;
}

// Moves the other players to their interpolated positions of the frame pair
void SV_ApplyAdjustedPositions(client_t *client, client_frame_t *older, client_frame_t *newer, float frac)
{
	for (int i = 0; i < older->entities.num_entities; i++)
	{
		entity_state_t *state = &older->entities.entities[i];
		if (state->number <= 0 || state->number > g_psvs.maxclients)
			continue;

		client_t *cl = &g_psvs.clients[state->number - 1];
		if (cl == client || !cl->active)
			continue;

		sv_adjusted_positions_t *pos = &truepositions[state->number - 1];
		if (pos->deadflag != DEAD_NO)
			continue;

		if (!pos->active)
		{
			Con_DPrintf("tried to store off position of bogus player %i/%s\n", i, cl->name);
			continue;
		}

		vec3_t origin;
		entity_state_t *pnextstate = SV_FindEntInPack(state->number, &newer->entities);
		if (pnextstate)
		{
			vec3_t delta;
			VectorSubtract(pnextstate->origin, state->origin, delta);
			VectorMA(state->origin, frac, delta, origin);
		}
		else
		{
			VectorCopy(state->origin, origin);
		}

		VectorCopy(origin, pos->neworg);
		VectorCopy(origin, pos->initial_correction_org);

		if (!VectorCompare(origin, cl->edict->v.origin))
		{
			VectorCopy(origin, cl->edict->v.origin);
			SV_LinkEdict(cl->edict, FALSE);
			pos->needrelink = 1;
		}
	}
}

// Lag compensation: rewinds the other players to the time the client saw them at when it issued the command
void SV_SetupMove(client_t *client)
{
	Q_memset(truepositions, 0, sizeof(truepositions));
	nofind = TRUE;

	if (!SV_IsLagCompensationActive(client))
		return;

	nofind = FALSE;

	SV_SaveTruePositions(client);

	double targettime = SV_GetUnlagTargetTime(client);

	client_frame_t *older;
	client_frame_t *newer;
	if (!SV_FindUnlagFrames(client, targettime, &older, &newer))
	{
		Q_memset(truepositions, 0, sizeof(truepositions));
		nofind = TRUE;
		return;
	}

	float frac = 0.0f;
	if (newer)
		frac = SV_CalcUnlagFraction(older, newer, targettime);
	else
		newer = older;

	SV_ApplyAdjustedPositions(client, older, newer, frac);
}

// Puts the players moved by SV_SetupMove back to where they are
void SV_RestoreMove(client_t *client)
{
	if (nofind)
	{
		nofind = FALSE;
		return;
	}

	if (!SV_IsLagCompensationActive(client))
		return;

	for (int i = 0; i < g_psvs.maxclients; i++)
	{
		client_t *cli = &g_psvs.clients[i];
		sv_adjusted_positions_t *pos = &truepositions[i];

		if (cli == client || !cli->active)
			continue;

		if (VectorCompare(pos->neworg, pos->oldorg) || !pos->needrelink)
			continue;

		if (!pos->active)
		{
			Con_DPrintf("SV_RestoreMove:  Tried to restore 'inactive' player %i/%s\n", i, cli->name);
			continue;
		}

		// Skip the players the game module has moved in the meantime
		if (VectorCompare(pos->initial_correction_org, cli->edict->v.origin))
		{
			VectorCopy(pos->oldorg, cli->edict->v.origin);
			SV_LinkEdict(cli->edict, FALSE);
		}
	}
}

void SV_ParseStringCommand(client_t *pSenderClient)
{
#ifdef REHLDS_FIXES
	// check string commands rate for this player
	g_StringCommandsRateLimiter.StringCommandIssued(pSenderClient - g_psvs.clients);

	if (!pSenderClient->connected)
		return; // player was kicked
#endif // REHLDS_FIXES

	char *s = MSG_ReadString();

	if (SV_ValidateClientCommand(s))
	{
		Cmd_ExecuteString(s, src_client);
		return;
	}

	// Commands for the game module are cut to 127 characters
#ifndef REHLDS_OPT_PEDANTIC
	if (Q_strlen(s) > 127)
#endif
		s[127] = 0;

	Cmd_TokenizeString(s);
	gEntityInterface.pfnClientCommand(sv_player);
}

void SV_ParseDelta(client_t *pSenderClient)
{
	host_client->delta_sequence = MSG_ReadByte();
}

void EXT_FUNC SV_EstablishTimeBase_mod(IGameClient *cl, usercmd_t *cmds, int dropped, int numbackup, int numcmds)
{
	SV_EstablishTimeBase_internal(cl->GetClient(), cmds, dropped, numbackup, numcmds);
}

void SV_EstablishTimeBase(client_t *cl, usercmd_t *cmds, int dropped, int numbackup, int numcmds)
{
	return g_RehldsHookchains.m_SV_EstablishTimeBase.callChain(SV_EstablishTimeBase_mod, cl->pGameClient, cmds, dropped, numbackup, numcmds);
}

// Commands lost on the way are replayed only while their number stays below this limit
const int MAX_DROPPED_CMDS = 24;

// Server time the first command of the message starts at: the end of the message minus the duration of all its commands
void SV_EstablishTimeBase_internal(client_t *cl, usercmd_t *cmds, int dropped, int numbackup, int numcmds)
{
	double runcmd_time = 0.0;
	double time_at_end = 0.0;

	// If we haven't dropped too many packets, then run some commands
	if (dropped < MAX_DROPPED_CMDS)
	{
		int droppedcmds = dropped;

		// Run the last known cmd for each dropped cmd we don't have a backup for
		while (droppedcmds > numbackup)
		{
			runcmd_time += cl->lastcmd.msec / 1000.0;
			droppedcmds--;
		}

		// Now run the "history" commands if we still have dropped packets
		while (droppedcmds > 0)
		{
			int cmdnum = numcmds + droppedcmds - 1;
			runcmd_time += cmds[cmdnum].msec / 1000.0;
			droppedcmds--;
		}
	}

	// Now run any new command(s). Go backward because the most recent command is at index 0
	for (int i = numcmds - 1; i >= 0; i--)
		time_at_end += cmds[i].msec / 1000.0;

	cl->svtimebase = host_frametime + g_psv.time - (time_at_end + runcmd_time);
}

// Strips the movement from the commands and keeps the view where it is
void SV_ClearMoveCmds(usercmd_t *cmds, int numcmds)
{
	for (int i = 0; i < numcmds; i++)
	{
		usercmd_t *cmd = &cmds[i];
		cmd->msec = 0;
		cmd->forwardmove = 0;
		cmd->sidemove = 0;
		cmd->upmove = 0;
		cmd->buttons = 0;

		if (sv_player->v.flags & FL_FROZEN)
			cmd->impulse = 0;

		VectorCopy(sv_player->v.v_angle, cmd->viewangles);
	}
}

// Replays the commands lost on the way: the last known one for the gaps without a backup, the backup ones for the rest
void SV_RunDroppedCmds(usercmd_t *cmds, int numcmds, int numbackup)
{
	if (net_drop >= MAX_DROPPED_CMDS)
		return;

	while (net_drop > numbackup)
	{
		SV_RunCmd(&host_client->lastcmd, 0, FALSE);
		net_drop--;
	}

	while (net_drop > 0)
	{
		int cmdnum = numcmds + net_drop - 1;
		SV_RunCmd(&cmds[cmdnum], host_client->netchan.incoming_sequence - cmdnum, TRUE);
		net_drop--;
	}
}

// Parses the move message: the movement commands of the latest packets together with the backup of the earlier ones
void SV_ParseMove(client_t *pSenderClient)
{
	// Only one move message per frame is accepted
	if (g_balreadymoved)
	{
		msg_badread = TRUE;
		return;
	}
	g_balreadymoved = TRUE;

	client_frame_t *frame = &host_client->frames[SV_UPDATE_MASK & host_client->netchan.incoming_acknowledged];

	int placeholder = msg_readcount + 1;
	int mlen = MSG_ReadByte();
	byte cbchecksum = MSG_ReadByte();

	if (mlen <= 0 || !SZ_HasSpaceToRead(&net_message, mlen))
	{
		msg_badread = TRUE;
		Con_DPrintf("%s:  %s:%s invalid length: %d\n", __func__, host_client->name, NET_AdrToString(host_client->netchan.remote_address), mlen);
		SV_DropClient(host_client, FALSE, "Invalid length");
		return;
	}

	COM_UnMunge(&net_message.data[placeholder + 1], mlen, host_client->netchan.incoming_sequence);

	unsigned int packetLossByte = MSG_ReadByte();
	int numbackup = MSG_ReadByte();
	int numcmds = MSG_ReadByte();

	// The high bit tells whether the voice loops back to the sender, the rest is the packet loss in percent
	float packet_loss = (float)(packetLossByte & 0x7F);
	pSenderClient->m_bLoopback = (packetLossByte >> 7) & 1;

	int totalcmds = numcmds + numbackup;
	net_drop += 1 - numcmds;
	if (totalcmds < 0 || totalcmds >= CMD_MAXBACKUP - 1)
	{
		Con_Printf("SV_ReadClientMessage: too many cmds %i sent for %s/%s\n", totalcmds, host_client->name, NET_AdrToString(host_client->netchan.remote_address));
		SV_DropClient(host_client, FALSE, "CMD_MAXBACKUP hit");
		msg_badread = TRUE;
		return;
	}

	// The oldest command is sent first and ends up last in the array
	usercmd_t cmds[CMD_MAXBACKUP]{};
	usercmd_t cmdNull{};

	usercmd_t *from = &cmdNull;
	for (int i = totalcmds - 1; i >= 0; i--)
	{
		MSG_ReadUsercmd(&cmds[i], from);
		from = &cmds[i];
	}

	if (!g_psv.active || !(host_client->active || host_client->spawned))
		return;

	if (msg_badread)
	{
		Con_Printf("Client %s:%s sent a bogus command packet\n", host_client->name, NET_AdrToString(host_client->netchan.remote_address));
		return;
	}

	byte cbpktchecksum = COM_BlockSequenceCRCByte(&net_message.data[placeholder + 1], msg_readcount - placeholder - 1, host_client->netchan.incoming_sequence);
	if (cbpktchecksum != cbchecksum)
	{
		Con_DPrintf("Failed command checksum for %s:%s\n", host_client->name, NET_AdrToString(host_client->netchan.remote_address));
		msg_badread = TRUE;
		return;
	}

	host_client->packet_loss = packet_loss;

	// A paused server, a player in the console in singleplayer and a frozen player get no movement
	if (g_psv.paused || (g_psvs.maxclients <= 1 && key_dest) || (sv_player->v.flags & FL_FROZEN))
	{
		SV_ClearMoveCmds(cmds, numcmds);
		net_drop = 0;
	}
	else
	{
		VectorCopy(cmds[CMD_MOSTRECENT].viewangles, sv_player->v.v_angle);
	}

#ifdef REHLDS_FIXES
	// check move commands rate for this player
	int numCmdsToIssue = numcmds;
	if (net_drop > 0)
		numCmdsToIssue += net_drop;

	g_MoveCommandRateLimiter.MoveCommandsIssued(host_client - g_psvs.clients, numCmdsToIssue);

	if (!host_client->connected)
		return; // player was kicked
#endif // REHLDS_FIXES

#ifndef REHLDS_FIXES
	// SV_RunCmd sets both of them as well
	sv_player->v.button = cmds[CMD_MOSTRECENT].buttons;
	sv_player->v.light_level = cmds[CMD_MOSTRECENT].lightlevel;
#endif // REHLDS_FIXES

	SV_EstablishTimeBase(host_client, cmds, net_drop, numbackup, numcmds);
	SV_RunDroppedCmds(cmds, numcmds, numbackup);

	// Go backward because the most recent command is at index 0
	for (int i = numcmds - 1; i >= 0; i--)
		SV_RunCmd(&cmds[i], host_client->netchan.incoming_sequence - i, TRUE);

#ifdef REHLDS_FIXES
	// skip if client dropped while executing commands
	if (!host_client->connected)
		return;

	if (numcmds)
		host_client->lastcmd = cmds[numcmds - 1];
	else if (numbackup)
		host_client->lastcmd = cmds[CMD_MOSTRECENT];
#else // REHLDS_FIXES
	host_client->lastcmd = cmds[CMD_MOSTRECENT];
#endif // REHLDS_FIXES

	// The message left the client in the middle of its frame, so half of the last command is not part of the ping
	frame->ping_time -= (float)(host_client->lastcmd.msec * 0.5 / 1000.0);
	if (frame->ping_time < 0.0)
		frame->ping_time = 0;

	if (sv_player->v.animtime > host_frametime + g_psv.time)
		sv_player->v.animtime = (float)(host_frametime + g_psv.time);
}

// Forwards the voice data of a client to every client that listens to it
void SV_ParseVoiceData(client_t *cl)
{
	char chReceived[4096];
	int iClient = cl - g_psvs.clients;
	unsigned int nDataLength = MSG_ReadShort();
	if (nDataLength > sizeof(chReceived))
	{
		Con_DPrintf("SV_ParseVoiceData: invalid incoming packet.\n");
		SV_DropClient(cl, FALSE, "Invalid voice data\n");
		return;
	}

	MSG_ReadBuf(nDataLength, chReceived);
	cl->m_lastvoicetime = g_psv.time;

	if (!sv_voiceenable.value)
		return;

	for (int i = 0; i < g_psvs.maxclients; i++)
	{
		client_t *pDestClient = &g_psvs.clients[i];
		bool isSender = (i == iClient);

		if (!isSender)
		{
			bool listening = ((1 << (i & 0x1F)) & cl->m_VoiceStreams[i >> 5]) != 0;
			if (!listening)
				continue;

			if (!pDestClient->active && !pDestClient->connected)
				continue;
		}

		// The sender gets its own voice back only in loopback mode
		int nSendLength = nDataLength;
		if (isSender && !pDestClient->m_bLoopback)
			nSendLength = 0;

		int bytesAvailable = pDestClient->datagram.maxsize - pDestClient->datagram.cursize;
		if (bytesAvailable <= (6 + nSendLength))
			continue;

		MSG_WriteByte(&pDestClient->datagram, svc_voicedata);
		MSG_WriteByte(&pDestClient->datagram, iClient);
		MSG_WriteShort(&pDestClient->datagram, nSendLength);
		MSG_WriteBuf(&pDestClient->datagram, nSendLength, chReceived);
	}
}

void SV_IgnoreHLTV(client_t *cl)
{
}

void SV_ParseCvarValue(client_t *cl)
{
	const char *value = MSG_ReadString();

	if (gNewDLLFunctions.pfnCvarValue)
		gNewDLLFunctions.pfnCvarValue(cl->edict, value);

	Con_DPrintf("Cvar query response: name:%s, value:%s\n", cl->name, value);
}

void SV_ParseCvarValue2(client_t *cl)
{
	int requestID = MSG_ReadLong();

	char cvarName[255];
	Q_strncpy(cvarName, MSG_ReadString(), sizeof(cvarName));
	cvarName[sizeof(cvarName) - 1] = 0;

	const char *value = MSG_ReadString();

	if (gNewDLLFunctions.pfnCvarValue2)
		gNewDLLFunctions.pfnCvarValue2(cl->edict, requestID, cvarName, value);

	Con_DPrintf("Cvar query response: name:%s, request ID %d, cvar:%s, value:%s\n", cl->name, requestID, cvarName, value);
}

void EXT_FUNC SV_HandleClientMessage_api(IGameClient* client, uint8 opcode)
{
	client_t* cl = client->GetClient();
	if (opcode < clc_bad || opcode > clc_cvarvalue2)
	{
		// TODO: Are we forced to use msg_badread for break the loop.
		static_assert(REHLDS_API_VERSION_MAJOR <= 3, "Bump major API DETECTED!! You shall rework the hookchain, make function returnable");
		msg_badread = TRUE;

		Con_Printf("SV_ReadClientMessage: unknown command char (%d)\n", opcode);
		SV_DropClient(cl, FALSE, "Bad command character in client command");
		return;
	}

#ifdef REHLDS_FIXES
	// Save current name of the client before a possible kick
	char name[32];
	Q_strlcpy(name, host_client->name);
#endif // REHLDS_FIXES

	void(*func)(client_t *) = sv_clcfuncs[opcode].pfnParse;
	if (func)
		func(cl);

#ifdef REHLDS_FIXES
	if (msg_badread)
		Con_Printf("SV_ReadClientMessage: badread on %s, opcode %s\n", name, sv_clcfuncs[opcode].pszname);
#endif // REHLDS_FIXES
}

// Processes the commands of a client message one by one until the message ends or turns out to be broken
void SV_ExecuteClientMessage(client_t *cl)
{
	g_balreadymoved = FALSE;

	// Ping of the frame the client has just acknowledged
	client_frame_t *frame = &cl->frames[SV_UPDATE_MASK & cl->netchan.incoming_acknowledged];
	frame->ping_time = realtime - frame->senttime - cl->next_messageinterval;
	if (frame->senttime == 0.0)
		frame->ping_time = 0;

	if (realtime - cl->connection_started < 2.0 && frame->ping_time > 0.0)
		frame->ping_time = 0;

	SV_ComputeLatency(cl);

	host_client = cl;
	sv_player = cl->edict;
	cl->delta_sequence = -1;
	pmove = &g_svmove;

	while (true)
	{
		if (msg_badread)
		{
#ifdef REHLDS_FIXES
			Con_Printf("SV_ReadClientMessage: badread on %s\n", host_client->name);
			if (host_client->active)
				SV_ClientPrintf("Badread\n");
#else // REHLDS_FIXES
			Con_Printf("SV_ReadClientMessage: badread\n");
#endif // REHLDS_FIXES
			return;
		}

		int c = MSG_ReadByte();
		if (c == -1)
			return;

		g_RehldsHookchains.m_HandleNetCommand.callChain(SV_HandleClientMessage_api, cl->pGameClient, c);

#ifdef REHLDS_FIXES
		// FIXED: Don't handle remaining packets if got dropclient above
		if (!cl->connected && !cl->active && !cl->spawned)
			break;
#endif // REHLDS_FIXES
	}
}

qboolean SV_SetPlayer(int idnum)
{
	for (int i = 0; i < g_psvs.maxclients; i++)
	{
		client_t *cl = &g_psvs.clients[i];
		if (!cl->spawned || !cl->active || !cl->connected)
			continue;

		if (cl->userid == idnum)
		{
			host_client = cl;
			sv_player = cl->edict;
			return TRUE;
		}
	}

	Con_Printf("Userid %i is not on the server\n", idnum);
	return FALSE;
}

void SV_ShowServerinfo_f(void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

#ifdef REHLDS_FIXES
	// fix print message client console
	SV_ClientPrintf("%s", Info_Serverinfo());
#else // REHLDS_FIXES
	Info_Print(Info_Serverinfo());
#endif // REHLDS_FIXES
}

void SV_SendEnts_f(void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (!host_client->active || !host_client->spawned || !host_client->connected)
		return;

	host_client->fully_connected = TRUE;

#ifdef REHLDS_FIXES
	// See SV_CheckFile function
	if (!sv_delayed_spray_upload.value)
		return;

	// TODO: all this is already checked earlier
	resource_t *res = host_client->resourcesneeded.pNext;
	if (res == &host_client->resourcesneeded)
		return;

	if (!(res->ucFlags & RES_WASMISSING) || res->type != t_decal || !(res->ucFlags & RES_CUSTOM))
		return;

	if (sv_rehlds_force_dlmax.value)
	{
		MSG_WriteByte(&host_client->netchan.message, svc_stufftext);
		MSG_WriteString(&host_client->netchan.message, va("cl_dlmax %i\n", FRAGMENT_MAX_SIZE));
	}

	MSG_WriteByte(&host_client->netchan.message, svc_stufftext);
	MSG_WriteString(&host_client->netchan.message, va("upload !MD5%s\n", MD5_Print(res->rgucMD5_hash)));
#endif // REHLDS_FIXES
}

// Limits how often a client can ask for a full update. Returns false when the request comes too soon after the last one.
bool SV_CheckFullUpdateRate(client_t *client)
{
	int entIndex = IndexOfEdict(client->edict);
	if (s_LastFullUpdate[entIndex] > g_psv.time)
		s_LastFullUpdate[entIndex] = 0;

	float ltime = g_psv.time - s_LastFullUpdate[entIndex];
	if (ltime <= 0.0)
		ltime = 0.0;

	if (ltime < 0.45 && g_psv.time > 0.45)
	{
		Con_DPrintf(
			"%s is spamming fullupdate: (%f) (%f) (%f)\n",
			client->name,
			g_psv.time,
			s_LastFullUpdate[entIndex],
			ltime);
		return false;
	}

	s_LastFullUpdate[entIndex] = g_psv.time;
	return true;
}

void SV_FullUpdate_f(void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (host_client->active && !SV_CheckFullUpdateRate(host_client))
		return;

#ifdef REHLDS_FIXES
	// it's not need until not active
	if (!host_client->active)
		return;
#endif // REHLDS_FIXES

	SV_ForceFullClientsUpdate();
	gEntityInterface.pfnClientCommand(sv_player);
}
