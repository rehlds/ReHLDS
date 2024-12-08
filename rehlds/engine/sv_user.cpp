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

//cvar_t sv_voicecodec;
//cvar_t sv_voicequality;

edict_t *sv_player;

//int giSkip;
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

bool EXT_FUNC SV_CheckConsistencyResponse_API(IGameClient *client, resource_t *res, uint32 hash) {
	return (hash != *(uint32 *)&res->rgucMD5_hash[0]);
}

void SV_ParseConsistencyResponse(client_t *pSenderClient)
{
	vec3_t mins;
	vec3_t maxs;
	vec3_t cmins;
	vec3_t cmaxs;
	unsigned char nullbuffer[32];
	unsigned char resbuffer[32];

	int length = 0;
	int c = 0;
	Q_memset(nullbuffer, 0, sizeof(nullbuffer));
	int value = MSG_ReadShort();

	if (value <= 0 || !SZ_HasSomethingToRead(&net_message, value))
	{
		msg_badread = TRUE;
		Con_DPrintf("%s:  %s:%s invalid length: %d\n", __func__, host_client->name, NET_AdrToString(host_client->netchan.remote_address), value);
		SV_DropClient(host_client, FALSE, "Invalid length");
		return;
	}

	COM_UnMunge(&net_message.data[msg_readcount], value, g_psvs.spawncount);
	MSG_StartBitReading(&net_message);

	while (MSG_ReadBits(1))
	{
		int idx = MSG_ReadBits(12);
		if (idx < 0 || idx >= g_psv.num_resources)
		{
			c = -1;
			break;
		}

#ifdef REHLDS_FIXES
		resource_t *r = &g_rehlds_sv.resources[idx];
#else // REHLDS_FIXES
		resource_t *r = &g_psv.resourcelist[idx];
#endif // REHLDS_FIXES
		if (!(r->ucFlags & RES_CHECKFILE))
		{
			c = -1;
			break;
		}

		Q_memcpy(resbuffer, r->rguc_reserved, sizeof(resbuffer));
		if (!Q_memcmp(resbuffer, nullbuffer, sizeof(resbuffer)))
		{
			uint32 hash = MSG_ReadBits(32);
			if (g_RehldsHookchains.m_SV_CheckConsistencyResponse.callChain(SV_CheckConsistencyResponse_API, GetRehldsApiClient(pSenderClient), r, hash))
				c = idx + 1;
		}
		else
		{
			MSG_ReadBitData(cmins, 12);
			MSG_ReadBitData(cmaxs, 12);
			Q_memcpy(resbuffer, r->rguc_reserved, sizeof(resbuffer));
			COM_UnMunge(resbuffer, sizeof(resbuffer), g_psvs.spawncount);
			FORCE_TYPE ft = (FORCE_TYPE)resbuffer[0];
			if (ft == force_model_samebounds)
			{
				Q_memcpy(mins, &resbuffer[1], 12);
				Q_memcpy(maxs, &resbuffer[13], 12);
				if (!VectorCompare(cmins, mins) || !VectorCompare(cmaxs, maxs))
					c = idx + 1;
			}
			else if (ft == force_model_specifybounds)
			{
				Q_memcpy(mins, &resbuffer[1], 12);
				Q_memcpy(maxs, &resbuffer[13], 12);
				for (int i = 0; i < 3; i++)
				{
					if (cmins[i] < mins[i] || cmaxs[i] > maxs[i])
					{
						c = idx + 1;
						break;
					}
				}
			}
			else if (ft == force_model_specifybounds_if_avail)
			{
				Q_memcpy(mins, &resbuffer[1], 12);
				Q_memcpy(maxs, &resbuffer[13], 12);
				if (cmins[0] != -1.0 || cmins[1] != -1.0 || cmins[2] != -1.0 || cmaxs[0] != -1.0 || cmaxs[1] != -1.0 || cmaxs[2] != -1.0)
				{
					for (int i = 0; i < 3; i++)
					{
						if (cmins[i] < mins[i] || cmaxs[i] > maxs[i])
						{
							c = idx + 1;
							break;
						}
					}
				}
			}
			else
			{
				msg_badread = 1;
				c = idx + 1;
				break;
			}

		}

		if (msg_badread)
			break;

		++length;
	}

	MSG_EndBitReading(&net_message);
	if (c < 0 || length != g_psv.num_consistency)
	{
		msg_badread = 1;
		Con_Printf("SV_ParseConsistencyResponse:  %s:%s sent bad file data\n", host_client->name, NET_AdrToString(host_client->netchan.remote_address));
		SV_DropClient(host_client, FALSE, "Bad file data");
		return;
	}

	if (c > 0)
	{
		char dropmessage[256];
#ifdef REHLDS_FIXES
		dropmessage[0] = '\0';

		if (gEntityInterface.pfnInconsistentFile(host_client->edict, g_rehlds_sv.resources[c - 1].szFileName, dropmessage))
		{
			if (dropmessage[0])
				SV_ClientPrintf("%s", dropmessage);

			SV_DropClient(host_client, FALSE, "Bad file %s", g_rehlds_sv.resources[c - 1].szFileName); // only filename. reason was printed in console if exists.
			return;
		}
#else // REHLDS_FIXES
		if (gEntityInterface.pfnInconsistentFile(host_client->edict, g_psv.resourcelist[c - 1].szFileName, dropmessage))
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
		if (!g_psv.consistency_list[i].filename)
			return 0;

		if (!Q_stricmp(filename, g_psv.consistency_list[i].filename))
		{
			if (ppconsist)
				*ppconsist = &g_psv.consistency_list[i];
			return 1;
		}
	}

	return 0;
}

int SV_TransferConsistencyInfo(void)
{
	return g_RehldsHookchains.m_SV_TransferConsistencyInfo.callChain(SV_TransferConsistencyInfo_internal);
}

int EXT_FUNC SV_TransferConsistencyInfo_internal(void)
{
	consistency_t *pc;

	int c = 0;
	for (int i = 0; i < g_psv.num_resources; i++)
	{
#ifdef REHLDS_FIXES
		resource_t *r = &g_rehlds_sv.resources[i];
#else // REHLDS_FIXES
		resource_t *r = &g_psv.resourcelist[i];
#endif // REHLDS_FIXES
		if (r->ucFlags == (RES_CUSTOM | RES_REQUESTED | RES_UNK_6) || (r->ucFlags & RES_CHECKFILE))
			continue;

		if (!SV_FileInConsistencyList(r->szFileName, &pc))
			continue;

		r->ucFlags |= RES_CHECKFILE;

		char filename[MAX_PATH];
		if (r->type != t_sound)
		{
			Q_strncpy(filename, r->szFileName, MAX_PATH - 1);
			filename[MAX_PATH - 1] = 0;
		}
		else
		{
			Q_snprintf(filename, MAX_PATH, "sound/%s", r->szFileName);
		}
		MD5_Hash_File(r->rgucMD5_hash, filename, FALSE, FALSE, NULL);

		if (r->type == t_model)
		{
			if (pc->check_type == force_model_samebounds)
			{
				vec3_t mins;
				vec3_t maxs;

				if (!R_GetStudioBounds(filename, mins, maxs))
					Host_Error("%s: Server unable to get bounds for %s\n", __func__, filename);

				Q_memcpy(&r->rguc_reserved[1], mins, sizeof(mins));
				Q_memcpy(&r->rguc_reserved[13], maxs, sizeof(maxs));
			}
			else if (pc->check_type == force_model_specifybounds || pc->check_type == force_model_specifybounds_if_avail)
			{
				Q_memcpy(&r->rguc_reserved[1], pc->mins, sizeof(pc->mins));
				Q_memcpy(&r->rguc_reserved[13], pc->maxs, sizeof(pc->maxs));
			}
			else
			{
				++c;
				continue;
			}
			r->rguc_reserved[0] = pc->check_type;
			COM_Munge(r->rguc_reserved, 32, g_psvs.spawncount);
		}
		++c;
	}
	return c;
}

bool EXT_FUNC SV_ShouldSendConsistencyList_mod(IGameClient *cl, bool forceConsistency)
{
	if (g_psvs.maxclients == 1 || g_psv.num_consistency == 0 || cl->IsProxy())
		return false;

	if (!forceConsistency && mp_consistency.value == 0.0f)
		return false;

	return true;
}

bool SV_ShouldSendConsistencyList(client_t *client, bool forceConsistency)
{
	return g_RehldsHookchains.m_SV_ShouldSendConsistencyList.callChain(SV_ShouldSendConsistencyList_mod, GetRehldsApiClient(client), forceConsistency);
}

void SV_SendConsistencyList(sizebuf_t *msg)
{
	host_client->has_force_unmodified = FALSE;

	if (!SV_ShouldSendConsistencyList(host_client, false))
	{
		MSG_WriteBits(0, 1);
		return;
	}

	host_client->has_force_unmodified = TRUE;

	int delta = 0;
	int lastcheck = 0;
#ifdef REHLDS_FIXES
	resource_t *r = g_rehlds_sv.resources;
#else // REHLDS_FIXES
	resource_t *r = g_psv.resourcelist;
#endif // REHLDS_FIXES

	MSG_WriteBits(1, 1);

	for (int i = 0; i < g_psv.num_resources; i++, r++)
	{
		if (r && (r->ucFlags & RES_CHECKFILE) != 0)
		{
			MSG_WriteBits(1, 1);
			delta = i - lastcheck;

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
	}

	MSG_WriteBits(0, 1);
}

void SV_PreRunCmd(void)
{
}

void SV_CopyEdictToPhysent(physent_t *pe, int e, edict_t *check)
{
	model_t *pModel;

	pe->origin[0] = check->v.origin[0];
	pe->origin[1] = check->v.origin[1];
	pe->origin[2] = check->v.origin[2];

	pe->info = e;

	if (e < 1 || e > g_psvs.maxclients)
	{
		pe->player = 0;
	}
	else
	{
		SV_GetTrueOrigin(e - 1, pe->origin);
		pe->player = pe->info;
	}

	pe->angles[0] = check->v.angles[0];
	pe->angles[1] = check->v.angles[1];
	pe->angles[2] = check->v.angles[2];

	pe->studiomodel = nullptr;
	pe->rendermode = check->v.rendermode;

	if (check->v.solid == SOLID_BSP)
	{
		pe->model = g_psv.models[check->v.modelindex];
		Q_strncpy(pe->name, pe->model->name, sizeof(pe->name) - 1);
		pe->name[sizeof(pe->name) - 1] = 0;
	}
	else if (check->v.solid == SOLID_NOT)
	{
		if (check->v.modelindex)
		{
			pe->model = g_psv.models[check->v.modelindex];
			Q_strncpy(pe->name, pe->model->name, sizeof(pe->name) - 1);
			pe->name[sizeof(pe->name) - 1] = 0;
		}
		else
		{
			pe->model = nullptr;
		}
	}
	else
	{
		pe->model = nullptr;
		if (check->v.solid != SOLID_BBOX)
		{
			pe->mins[0] = check->v.mins[0];
			pe->mins[1] = check->v.mins[1];
			pe->maxs[0] = check->v.maxs[0];
			pe->mins[2] = check->v.mins[2];
			pe->maxs[1] = check->v.maxs[1];
			pe->maxs[2] = check->v.maxs[2];

			if (check->v.classname)
			{
				Q_strncpy(pe->name, &pr_strings[check->v.classname], sizeof(pe->name) - 1);
				pe->name[sizeof(pe->name) - 1] = 0;
			}
			else
			{
				Q_sprintf(pe->name, "?");
			}
		}
		else
		{
			if (check->v.modelindex)
			{
				pModel = g_psv.models[check->v.modelindex];
				if (pModel)
				{
					if (pModel->flags & STUDIO_TRACE_HITBOX)
						pe->studiomodel = pModel;

					Q_strncpy(pe->name, pModel->name, sizeof(pe->name) - 1);
					pe->name[sizeof(pe->name) - 1] = 0;
				}
			}

			pe->mins[0] = check->v.mins[0];
			pe->mins[1] = check->v.mins[1];
			pe->mins[2] = check->v.mins[2];
			pe->maxs[0] = check->v.maxs[0];
			pe->maxs[1] = check->v.maxs[1];
			pe->maxs[2] = check->v.maxs[2];
		}
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

	pe->vuser1[0] = check->v.vuser1[0];
	pe->vuser1[1] = check->v.vuser1[1];
	pe->vuser1[2] = check->v.vuser1[2];
	pe->vuser2[0] = check->v.vuser2[0];
	pe->vuser2[1] = check->v.vuser2[1];
	pe->vuser2[2] = check->v.vuser2[2];
	pe->vuser3[0] = check->v.vuser3[0];
	pe->vuser3[1] = check->v.vuser3[1];
	pe->vuser3[2] = check->v.vuser3[2];

	pe->takedamage = 0;
	pe->blooddecal = 0;

	pe->vuser4[0] = check->v.vuser4[0];
	pe->vuser4[1] = check->v.vuser4[1];
	pe->vuser4[2] = check->v.vuser4[2];
}

bool EXT_FUNC SV_AllowPhysent_mod(edict_t* check, edict_t* sv_player) {
	return true;
}

bool SV_AllowPhysent(edict_t* check, edict_t* sv_player) {
	return g_RehldsHookchains.m_SV_AllowPhysent.callChain(SV_AllowPhysent_mod, check, sv_player);
}

void SV_AddLinksToPM_(areanode_t *node, float *pmove_mins, float *pmove_maxs)
{
	struct link_s *l;
	edict_t *check;
	int e;
	physent_t *ve;
	link_t *next;
	float *fmax;
	float *fmin;
	physent_t *pe;

	for (l = node->solid_edicts.next; l != &node->solid_edicts; l = next)
	{
		check = (edict_t *)&l[-1];
		next = l->next;
		if (check->v.groupinfo)
		{
			if (g_groupop)
			{
				if (g_groupop == GROUP_OP_NAND && (check->v.groupinfo & sv_player->v.groupinfo))
					continue;
			}
			else
			{
				if (!(check->v.groupinfo & sv_player->v.groupinfo))
					continue;
			}
		}

		if (check->v.owner == sv_player)
			continue;

		if (check->v.solid != SOLID_BSP && check->v.solid != SOLID_BBOX && check->v.solid != SOLID_SLIDEBOX && check->v.solid != SOLID_NOT)
			continue;

		// Apply our own custom checks
		if (!SV_AllowPhysent(check, sv_player)) {
			continue;
		}

		e = NUM_FOR_EDICT(check);
		ve = &pmove->visents[pmove->numvisent];
		pmove->numvisent = pmove->numvisent + 1;
		SV_CopyEdictToPhysent(ve, e, check);

		if ((check->v.solid == SOLID_NOT) && (!check->v.skin || !check->v.modelindex))
			continue;

		if ((check->v.flags & FL_MONSTERCLIP) && check->v.solid == SOLID_BSP)
			continue;

		if (check == sv_player)
			continue;

		if ((check->v.flags & FL_CLIENT) && check->v.health <= 0.0)
			continue;

		if (check->v.mins[2] == 0.0 && check->v.maxs[2] == 1.0)
			continue;

		if (Length(check->v.size) == 0.0)
			continue;

		fmin = check->v.absmin;
		fmax = check->v.absmax;
		if (check->v.flags & FL_CLIENT)
			SV_GetTrueMinMax(e - 1, &fmin, &fmax);

		if (!BoundsIntersect(pmove_mins, pmove_maxs, fmin, fmax))
			continue;

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

		Q_memcpy(pe, ve, sizeof(physent_t));
	}

	if (node->axis != -1)
	{
		if (pmove_maxs[node->axis] > node->dist)
			SV_AddLinksToPM_(node->children[0], pmove_mins, pmove_maxs);

		if (pmove_mins[node->axis] < node->dist)
			SV_AddLinksToPM_(node->children[1], pmove_mins, pmove_maxs);
	}
}

void SV_AddLinksToPM(areanode_t *node, vec_t *origin)
{
	vec3_t pmove_mins;
	vec3_t pmove_maxs;

	Q_memset(&pmove->physents[0], 0, sizeof(physent_t));
	Q_memset(&pmove->visents[0], 0, sizeof(physent_t));

	pmove->physents[0].model = g_psv.worldmodel;
	if (g_psv.worldmodel != NULL)
	{
		Q_strncpy(pmove->physents[0].name, g_psv.worldmodel->name, sizeof(pmove->physents[0].name) - 1);
		pmove->physents[0].name[sizeof(pmove->physents[0].name) - 1] = 0;
	}
	pmove->physents[0].origin[0] = vec3_origin[0];
	pmove->physents[0].origin[1] = vec3_origin[1];
	pmove->physents[0].origin[2] = vec3_origin[2];
	pmove->physents[0].info = 0;
	pmove->physents[0].solid = 4;
	pmove->physents[0].movetype = 0;
	pmove->physents[0].takedamage = 1;
	pmove->physents[0].blooddecal = 0;
	pmove->numphysent = 1;
	pmove->numvisent = 1;
	pmove->visents[0] = pmove->physents[0];
	pmove->nummoveent = 0;
	for (int i = 0; i < 3; i++)
	{
		pmove_mins[i] = origin[i] - 256.0f;
		pmove_maxs[i] = origin[i] + 256.0f;
	}
	SV_AddLinksToPM_(node, pmove_mins, pmove_maxs);
}

void SV_PlayerRunPreThink(edict_t *player, float time)
{
	gGlobalVariables.time = time;
	gEntityInterface.pfnPlayerPreThink(player);
}

qboolean SV_PlayerRunThink(edict_t *ent, float frametime, double clienttimebase)
{
	float thinktime;

	if (!(ent->v.flags & (FL_DORMANT | FL_KILLME)))
	{
		thinktime = ent->v.nextthink;
		if (thinktime <= 0.0 || frametime + clienttimebase < thinktime)
			return 1;

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

void SV_CheckMovingGround(edict_t *ent, float frametime)
{
	edict_t *groundentity;

	if (ent->v.flags & FL_ONGROUND)
	{
		groundentity = ent->v.groundentity;
		if (groundentity)
		{
			if (groundentity->v.flags & FL_CONVEYOR)
			{
				if (ent->v.flags & FL_BASEVELOCITY)
					VectorMA(ent->v.basevelocity, groundentity->v.speed, groundentity->v.movedir, ent->v.basevelocity);
				else
					VectorScale(groundentity->v.movedir, groundentity->v.speed, ent->v.basevelocity);
				ent->v.flags |= FL_BASEVELOCITY;
			}
		}
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
	dest->endpos[0] = src->endpos[0];
	dest->endpos[1] = src->endpos[1];
	dest->endpos[2] = src->endpos[2];
	dest->plane.normal[0] = src->plane.normal[0];
	dest->plane.normal[1] = src->plane.normal[1];
	dest->plane.normal[2] = src->plane.normal[2];
	dest->plane.dist = src->plane.dist;
	dest->hitgroup = src->hitgroup;
	dest->ent = ent;
}

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
		client_t * client = &g_psvs.clients[i];
		if (client == host_client || client->active || client->connected || client->spawned)
			SV_FullClientUpdate(client, &msg);
	}

	Con_DPrintf("Client %s started recording. Send full update.\n", host_client->name);
	Netchan_CreateFragments(1, &host_client->netchan, &msg);
	Netchan_FragSend(&host_client->netchan);
}

void SV_RunCmd(usercmd_t *ucmd, int random_seed)
{
	usercmd_t cmd = *ucmd;
	int i;
	edict_t *ent;
	trace_t trace;
	float frametime;

	if (host_client->ignorecmdtime > realtime)
	{
		host_client->cmdtime = (double)ucmd->msec / 1000.0 + host_client->cmdtime;
		return;
	}


	host_client->ignorecmdtime = 0;
	if (cmd.msec > 50)
	{
		cmd.msec = (byte)(ucmd->msec / 2.0);
		SV_RunCmd(&cmd, random_seed);
		cmd.msec = (byte)(ucmd->msec / 2.0);
		cmd.impulse = 0;
		SV_RunCmd(&cmd, random_seed);
		return;
	}


	if (!host_client->fakeclient)
		SV_SetupMove(host_client);

#ifdef REHLDS_FIXES
	if (sv_usercmd_custom_random_seed.value)
	{
		float fltTimeNow = float(Sys_FloatTime() * 1000.0);
		random_seed = *reinterpret_cast<int *>((char *)&fltTimeNow);
	}
#endif

	gEntityInterface.pfnCmdStart(sv_player, ucmd, random_seed);
	frametime = float(ucmd->msec * 0.001);
	host_client->svtimebase = frametime + host_client->svtimebase;
	host_client->cmdtime = ucmd->msec / 1000.0 + host_client->cmdtime;
	if (ucmd->impulse)
	{
		sv_player->v.impulse = ucmd->impulse;

		// Disable fullupdate via impulse 204
#ifndef REHLDS_FIXES
		if (ucmd->impulse == 204)
			SV_ForceFullClientsUpdate();
#endif // REHLDS_FIXES
	}
	sv_player->v.clbasevelocity[0] = 0;
	sv_player->v.clbasevelocity[1] = 0;
	sv_player->v.clbasevelocity[2] = 0;
	sv_player->v.button = ucmd->buttons;
#ifdef REHLDS_FIXES
	sv_player->v.light_level = ucmd->lightlevel;
#endif
	SV_CheckMovingGround(sv_player, frametime);
	pmove->oldangles[0] = sv_player->v.v_angle[0];
	pmove->oldangles[1] = sv_player->v.v_angle[1];
	pmove->oldangles[2] = sv_player->v.v_angle[2];
	if (!sv_player->v.fixangle)
	{
		sv_player->v.v_angle[0] = ucmd->viewangles[0];
		sv_player->v.v_angle[1] = ucmd->viewangles[1];
		sv_player->v.v_angle[2] = ucmd->viewangles[2];
	}
	SV_PlayerRunPreThink(sv_player, (float)host_client->svtimebase);
	SV_PlayerRunThink(sv_player, frametime, host_client->svtimebase);
	if (Length(sv_player->v.basevelocity) > 0.0)
	{
		sv_player->v.clbasevelocity[0] = sv_player->v.basevelocity[0];
		sv_player->v.clbasevelocity[1] = sv_player->v.basevelocity[1];
		sv_player->v.clbasevelocity[2] = sv_player->v.basevelocity[2];
	}

	pmove->server = TRUE;
	pmove->multiplayer = (g_psvs.maxclients > 1) ? TRUE : FALSE;
	pmove->time = float(1000.0 * host_client->svtimebase);
	pmove->usehull = (sv_player->v.flags & FL_DUCKING) == FL_DUCKING;
	pmove->maxspeed = sv_maxspeed.value;
	pmove->clientmaxspeed = sv_player->v.maxspeed;
	pmove->flDuckTime = (float)sv_player->v.flDuckTime;
	pmove->bInDuck = sv_player->v.bInDuck;
	pmove->flTimeStepSound = sv_player->v.flTimeStepSound;
	pmove->iStepLeft = sv_player->v.iStepLeft;
	pmove->flFallVelocity = sv_player->v.flFallVelocity;
	pmove->flSwimTime = (float)sv_player->v.flSwimTime;
	pmove->oldbuttons = sv_player->v.oldbuttons;

	Q_strncpy(pmove->physinfo, host_client->physinfo, sizeof(pmove->physinfo) - 1);
	pmove->physinfo[sizeof(pmove->physinfo) - 1] = 0;

	pmove->velocity[0] = sv_player->v.velocity[0];
	pmove->velocity[1] = sv_player->v.velocity[1];
	pmove->velocity[2] = sv_player->v.velocity[2];

	pmove->movedir[0] = sv_player->v.movedir[0];
	pmove->movedir[1] = sv_player->v.movedir[1];
	pmove->movedir[2] = sv_player->v.movedir[2];

	pmove->angles[0] = sv_player->v.v_angle[0];
	pmove->angles[1] = sv_player->v.v_angle[1];
	pmove->angles[2] = sv_player->v.v_angle[2];

	pmove->basevelocity[0] = sv_player->v.basevelocity[0];
	pmove->basevelocity[1] = sv_player->v.basevelocity[1];
	pmove->basevelocity[2] = sv_player->v.basevelocity[2];

	pmove->view_ofs[0] = sv_player->v.view_ofs[0];
	pmove->view_ofs[1] = sv_player->v.view_ofs[1];
	pmove->view_ofs[2] = sv_player->v.view_ofs[2];

	pmove->punchangle[0] = sv_player->v.punchangle[0];
	pmove->punchangle[1] = sv_player->v.punchangle[1];
	pmove->punchangle[2] = sv_player->v.punchangle[2];

	pmove->deadflag = sv_player->v.deadflag;
	pmove->effects = sv_player->v.effects;
	pmove->gravity = sv_player->v.gravity;
	pmove->friction = sv_player->v.friction;
	pmove->spectator = 0;
	pmove->waterjumptime = sv_player->v.teleport_time;

#ifdef REHLDS_FIXES
	Q_memcpy(&pmove->cmd, ucmd, sizeof(pmove->cmd));
#else
	Q_memcpy(&pmove->cmd, &cmd, sizeof(pmove->cmd));
#endif

	pmove->dead = sv_player->v.health <= 0.0;
	pmove->movetype = sv_player->v.movetype;
	pmove->flags = sv_player->v.flags;
	pmove->player_index = NUM_FOR_EDICT(sv_player) - 1;

	pmove->iuser1 = sv_player->v.iuser1;
	pmove->iuser2 = sv_player->v.iuser2;
	pmove->iuser3 = sv_player->v.iuser3;
	pmove->iuser4 = sv_player->v.iuser4;
	pmove->fuser1 = sv_player->v.fuser1;
	pmove->fuser2 = sv_player->v.fuser2;
	pmove->fuser3 = sv_player->v.fuser3;
	pmove->fuser4 = sv_player->v.fuser4;

	pmove->vuser1[0] = sv_player->v.vuser1[0];
	pmove->vuser1[1] = sv_player->v.vuser1[1];
	pmove->vuser1[2] = sv_player->v.vuser1[2];

	pmove->vuser2[0] = sv_player->v.vuser2[0];
	pmove->vuser2[1] = sv_player->v.vuser2[1];
	pmove->vuser2[2] = sv_player->v.vuser2[2];

	pmove->vuser3[0] = sv_player->v.vuser3[0];
	pmove->vuser3[1] = sv_player->v.vuser3[1];
	pmove->vuser3[2] = sv_player->v.vuser3[2];

	pmove->vuser4[0] = sv_player->v.vuser4[0];
	pmove->vuser4[1] = sv_player->v.vuser4[1];
	pmove->vuser4[2] = sv_player->v.vuser4[2];

	pmove->origin[0] = sv_player->v.origin[0];
	pmove->origin[1] = sv_player->v.origin[1];
	pmove->origin[2] = sv_player->v.origin[2];

	SV_AddLinksToPM(sv_areanodes, pmove->origin);

	pmove->frametime = frametime;
	pmove->runfuncs = TRUE;
	pmove->PM_PlaySound = PM_SV_PlaySound;
	pmove->PM_TraceTexture = PM_SV_TraceTexture;
	pmove->PM_PlaybackEventFull = PM_SV_PlaybackEventFull;
	pmove->movevars = &host_client->movevars;

	const movevars_t movevars = *pmove->movevars; // preserve current movevars
	host_client->movevars = sv_movevars; // always use global movevars as a base

	gEntityInterface.pfnPM_Move(pmove, TRUE);

	// Determine whether movevars has changed or not
	if (!host_client->fakeclient && Q_memcmp(&movevars, pmove->movevars, sizeof(movevars)) != 0)
		SV_WriteMovevarsToClient(&host_client->netchan.message, pmove->movevars); // sync movevars for the client

	sv_player->v.deadflag = pmove->deadflag;
	sv_player->v.effects = pmove->effects;
	sv_player->v.teleport_time = pmove->waterjumptime;
	sv_player->v.waterlevel = pmove->waterlevel;
	sv_player->v.watertype = pmove->watertype;
	sv_player->v.flags = pmove->flags;
	sv_player->v.friction = pmove->friction;
	sv_player->v.movetype = pmove->movetype;
	sv_player->v.maxspeed = pmove->clientmaxspeed;
	sv_player->v.iStepLeft = pmove->iStepLeft;

	sv_player->v.view_ofs[0] = pmove->view_ofs[0];
	sv_player->v.view_ofs[1] = pmove->view_ofs[1];
	sv_player->v.view_ofs[2] = pmove->view_ofs[2];

	sv_player->v.movedir[0] = pmove->movedir[0];
	sv_player->v.movedir[1] = pmove->movedir[1];
	sv_player->v.movedir[2] = pmove->movedir[2];

	sv_player->v.punchangle[0] = pmove->punchangle[0];
	sv_player->v.punchangle[1] = pmove->punchangle[1];
	sv_player->v.punchangle[2] = pmove->punchangle[2];

	if (pmove->onground == -1)
	{
		sv_player->v.flags &= ~FL_ONGROUND;
	}
	else
	{
		sv_player->v.flags |= FL_ONGROUND;
		sv_player->v.groundentity = EDICT_NUM(pmove->physents[pmove->onground].info);
	}

	sv_player->v.origin[0] = pmove->origin[0];
	sv_player->v.origin[1] = pmove->origin[1];
	sv_player->v.origin[2] = pmove->origin[2];

	sv_player->v.velocity[0] = pmove->velocity[0];
	sv_player->v.velocity[1] = pmove->velocity[1];
	sv_player->v.velocity[2] = pmove->velocity[2];

	sv_player->v.basevelocity[0] = pmove->basevelocity[0];
	sv_player->v.basevelocity[1] = pmove->basevelocity[1];
	sv_player->v.basevelocity[2] = pmove->basevelocity[2];

	if (!sv_player->v.fixangle)
	{
		sv_player->v.v_angle[0] = pmove->angles[0];
		sv_player->v.v_angle[1] = pmove->angles[1];
		sv_player->v.v_angle[2] = pmove->angles[2];
		sv_player->v.angles[0] = float(-pmove->angles[0] / 3.0);
		sv_player->v.angles[1] = pmove->angles[1];
		sv_player->v.angles[2] = pmove->angles[2];
	}

	sv_player->v.bInDuck = pmove->bInDuck;
	sv_player->v.flDuckTime = (int)pmove->flDuckTime;
	sv_player->v.flTimeStepSound = pmove->flTimeStepSound;
	sv_player->v.flFallVelocity = pmove->flFallVelocity;
	sv_player->v.flSwimTime = (int)pmove->flSwimTime;
	sv_player->v.oldbuttons = pmove->cmd.buttons;

	sv_player->v.iuser1 = pmove->iuser1;
	sv_player->v.iuser2 = pmove->iuser2;
	sv_player->v.iuser3 = pmove->iuser3;
	sv_player->v.iuser4 = pmove->iuser4;
	sv_player->v.fuser1 = pmove->fuser1;
	sv_player->v.fuser2 = pmove->fuser2;
	sv_player->v.fuser3 = pmove->fuser3;
	sv_player->v.fuser4 = pmove->fuser4;

	sv_player->v.vuser1[0] = pmove->vuser1[0];
	sv_player->v.vuser1[1] = pmove->vuser1[1];
	sv_player->v.vuser1[2] = pmove->vuser1[2];

	sv_player->v.vuser2[0] = pmove->vuser2[0];
	sv_player->v.vuser2[1] = pmove->vuser2[1];
	sv_player->v.vuser2[2] = pmove->vuser2[2];

	sv_player->v.vuser3[0] = pmove->vuser3[0];
	sv_player->v.vuser3[1] = pmove->vuser3[1];
	sv_player->v.vuser3[2] = pmove->vuser3[2];

	sv_player->v.vuser4[0] = pmove->vuser4[0];
	sv_player->v.vuser4[1] = pmove->vuser4[1];
	sv_player->v.vuser4[2] = pmove->vuser4[2];

	SetMinMaxSize(sv_player, player_mins[pmove->usehull], player_maxs[pmove->usehull], 0);
	if (host_client->edict->v.solid)
	{
		SV_LinkEdict(sv_player, TRUE);
		vec3_t vel;

		vel[0] = sv_player->v.velocity[0];
		vel[1] = sv_player->v.velocity[1];
		vel[2] = sv_player->v.velocity[2];
		for (i = 0; i < pmove->numtouch; ++i)
		{
			pmtrace_t *tr = &pmove->touchindex[i];
			ent = EDICT_NUM(pmove->physents[tr->ent].info);
			SV_ConvertPMTrace(&trace, tr, ent);
			sv_player->v.velocity[0] = tr->deltavelocity[0];
			sv_player->v.velocity[1] = tr->deltavelocity[1];
			sv_player->v.velocity[2] = tr->deltavelocity[2];
			SV_Impact(ent, sv_player, &trace);
		}
		sv_player->v.velocity[0] = vel[0];
		sv_player->v.velocity[1] = vel[1];
		sv_player->v.velocity[2] = vel[2];
	}

	gGlobalVariables.time = (float)host_client->svtimebase;
	gGlobalVariables.frametime = frametime;
	gEntityInterface.pfnPlayerPostThink(sv_player);
	gEntityInterface.pfnCmdEnd(sv_player);

	if (!host_client->fakeclient)
		SV_RestoreMove(host_client);
}

int SV_ValidateClientCommand(char *pszCommand)
{
	const char *p;
	int i = 0;

	COM_Parse(pszCommand);
	while ((p = clcommands[i]) != NULL)
	{
		if (!Q_stricmp(com_token, p))
		{
			return 1;
		}
		i++;
	}
	return 0;
}

float SV_CalcClientTime(client_t *cl)
{
	float minping;
	float maxping;
	int backtrack;

	float ping = 0.0;
	int count = 0;
	backtrack = (int)sv_unlagsamples.value;

	if (backtrack < 1)
		backtrack = 1;

	if (backtrack >= (SV_UPDATE_BACKUP <= 16 ? SV_UPDATE_BACKUP : 16))
		backtrack = (SV_UPDATE_BACKUP <= 16 ? SV_UPDATE_BACKUP : 16);

	if (backtrack <= 0)
		return 0.0f;

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

	minping = 9999.0;
	maxping = -9999.0;
	ping /= count;

	for (int i = 0; i < (SV_UPDATE_BACKUP <= 4 ? SV_UPDATE_BACKUP : 4); i++)
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

int SV_UnlagCheckTeleport(vec_t *v1, vec_t *v2)
{
	for (int i = 0; i < 3; i++)
	{
		if (fabs(v1[i] - v2[i]) > 128)
			return 1;
	}

	return 0;
}

void SV_GetTrueOrigin(int player, vec_t *origin)
{
	if (!host_client->lw || !host_client->lc)
		return;

	if (sv_unlag.value == 0 || g_psvs.maxclients <= 1 || !host_client->active)
		return;

	if (player < 0 || player >= g_psvs.maxclients)
		return;

	if (truepositions[player].active && truepositions[player].needrelink)
	{
		origin[0] = truepositions[player].oldorg[0];
		origin[1] = truepositions[player].oldorg[1];
		origin[2] = truepositions[player].oldorg[2];
	}
}

void SV_GetTrueMinMax(int player, float **fmin, float **fmax)
{
	if (!host_client->lw || !host_client->lc)
		return;

	if (sv_unlag.value == 0.0f || g_psvs.maxclients <= 1)
		return;

	if (!host_client->active || player < 0 || player >= g_psvs.maxclients)
		return;

	if (truepositions[player].active && truepositions[player].needrelink)
	{
		*fmin = truepositions[player].oldabsmin;
		*fmax = truepositions[player].oldabsmax;
	}
}

entity_state_t *SV_FindEntInPack(int index, packet_entities_t *pack)
{
	if (pack->num_entities <= 0)
		return NULL;

	for (int i = 0; i < pack->num_entities; i++)
	{
		if (pack->entities[i].number == index)
			return &pack->entities[i];
	}

	return NULL;
}

void SV_SetupMove(client_t *_host_client)
{
	struct client_s *cl;
	float cl_interptime;
	client_frame_t *nextFrame;
	entity_state_t *state;
	sv_adjusted_positions_t *pos;
	float frac;
	entity_state_t *pnextstate;
	int i;
	client_frame_t *frame;
	vec3_t origin;
	vec3_t delta;


#ifdef REHLDS_FIXES
	double targettime; // FP precision fix
#else
	float targettime;
#endif // REHLDS_FIXES

	Q_memset(truepositions, 0, sizeof(truepositions));
	nofind = 1;
	if (!gEntityInterface.pfnAllowLagCompensation())
		return;

	if (sv_unlag.value == 0.0f || !_host_client->lw || !_host_client->lc)
		return;

	if (g_psvs.maxclients <= 1 || !_host_client->active)
		return;

	nofind = 0;
	for (int i = 0; i < g_psvs.maxclients; i++)
	{
		cl = &g_psvs.clients[i];
		if (cl == _host_client || !cl->active)
			continue;

		truepositions[i].oldorg[0] = cl->edict->v.origin[0];
		truepositions[i].oldorg[1] = cl->edict->v.origin[1];
		truepositions[i].oldorg[2] = cl->edict->v.origin[2];
		truepositions[i].oldabsmin[0] = cl->edict->v.absmin[0];
		truepositions[i].oldabsmin[1] = cl->edict->v.absmin[1];
		truepositions[i].oldabsmin[2] = cl->edict->v.absmin[2];
		truepositions[i].oldabsmax[0] = cl->edict->v.absmax[0];
		truepositions[i].oldabsmax[1] = cl->edict->v.absmax[1];
		truepositions[i].active = 1;
		truepositions[i].oldabsmax[2] = cl->edict->v.absmax[2];
	}

	float clientLatency = _host_client->latency;
	if (clientLatency > 1.5)
		clientLatency = 1.5f;

	if (sv_maxunlag.value != 0.0f)
	{
		if (sv_maxunlag.value < 0.0)
			Cvar_SetValue("sv_maxunlag", 0.0);

		if (clientLatency >= sv_maxunlag.value)
			clientLatency = sv_maxunlag.value;
	}

	cl_interptime = _host_client->lastcmd.lerp_msec / 1000.0f;

	if (cl_interptime > 0.1)
		cl_interptime = 0.1f;

	if (_host_client->next_messageinterval > cl_interptime)
		cl_interptime = (float) _host_client->next_messageinterval;

#ifdef REHLDS_FIXES
	// FP Precision fix (targettime is double there, not float)
	targettime = realtime - clientLatency - cl_interptime + sv_unlagpush.value;
	if (targettime > realtime)
		targettime = realtime;
#else
	targettime = float(realtime - clientLatency - cl_interptime + sv_unlagpush.value);
	if (targettime > realtime)
		targettime = float(realtime);
#endif // REHLDS_FIXES

	if (SV_UPDATE_BACKUP <= 0)
	{
		Q_memset(truepositions, 0, sizeof(truepositions));
		nofind = 1;
		return;
	}

	frame = nextFrame = NULL;
	for (i = 0; i < SV_UPDATE_BACKUP; i++, frame = nextFrame)
	{
		nextFrame = &_host_client->frames[SV_UPDATE_MASK & (_host_client->netchan.outgoing_sequence + ~i)];
		for (int j = 0; j < nextFrame->entities.num_entities; j++)
		{
			state = &nextFrame->entities.entities[j];

#ifdef REHLDS_OPT_PEDANTIC
			if (state->number <= 0)
				continue;

			if (state->number > g_psvs.maxclients)
				break; // players are always in the beginning of the list, no need to look more
#else
			if (state->number <= 0 || state->number > g_psvs.maxclients)
				continue;
#endif

			pos = &truepositions[state->number - 1];
			if (pos->deadflag)
				continue;


			if (state->health <= 0)
				pos->deadflag = 1;

			if (state->effects & EF_NOINTERP)
				pos->deadflag = 1;

			if (pos->temp_org_setflag)
			{
				if (SV_UnlagCheckTeleport(state->origin, pos->temp_org))
					pos->deadflag = 1;
			}
			else
			{
				pos->temp_org_setflag = 1;
			}

			pos->temp_org[0] = state->origin[0];
			pos->temp_org[1] = state->origin[1];
			pos->temp_org[2] = state->origin[2];
		}

		if (targettime > nextFrame->senttime)
			break;
	}

	if ( i >= SV_UPDATE_BACKUP || targettime - nextFrame->senttime > 1.0)
	{
		Q_memset(truepositions, 0, sizeof(truepositions));
		nofind = 1;
		return;
	}

	if (frame)
	{
		float timeDiff = float(frame->senttime - nextFrame->senttime);
		if (timeDiff == 0.0)
			frac = 0.0;
		else
		{
			frac = float((targettime - nextFrame->senttime) / timeDiff);
			if (frac <= 1.0)
			{
				if (frac < 0.0)
					frac = 0.0;
			}
			else
				frac = 1.0;
		}
	}
	else
	{
		frame = nextFrame;
		frac = 0.0;
	}

	for (i = 0; i < nextFrame->entities.num_entities; i++)
	{
		state = &nextFrame->entities.entities[i];
		if (state->number <= 0 || state->number > g_psvs.maxclients)
			continue;

		cl = &g_psvs.clients[state->number - 1];
		if (cl == _host_client || !cl->active)
			continue;

		pos = &truepositions[state->number - 1];
		if (pos->deadflag)
			continue;

		if (!pos->active)
		{
			Con_DPrintf("tried to store off position of bogus player %i/%s\n", i, cl->name);
			continue;
		}

		pnextstate = SV_FindEntInPack(state->number, &frame->entities);

		if (pnextstate)
		{
			delta[0] = pnextstate->origin[0] - state->origin[0];
			delta[1] = pnextstate->origin[1] - state->origin[1];
			delta[2] = pnextstate->origin[2] - state->origin[2];
			VectorMA(state->origin, frac, delta, origin);
		}
		else
		{
			origin[0] = state->origin[0];
			origin[1] = state->origin[1];
			origin[2] = state->origin[2];
		}
		pos->neworg[0] = origin[0];
		pos->neworg[1] = origin[1];
		pos->neworg[2] = origin[2];
		pos->initial_correction_org[0] = origin[0];
		pos->initial_correction_org[1] = origin[1];
		pos->initial_correction_org[2] = origin[2];
		if (!VectorCompare(origin, cl->edict->v.origin))
		{
			cl->edict->v.origin[0] = origin[0];
			cl->edict->v.origin[1] = origin[1];
			cl->edict->v.origin[2] = origin[2];
			SV_LinkEdict(cl->edict, FALSE);
			pos->needrelink = 1;
		}
	}
}

void SV_RestoreMove(client_t *_host_client)
{
	sv_adjusted_positions_t *pos;
	client_t *cli;

	if (nofind)
	{
		nofind = 0;
		return;
	}

	if (!gEntityInterface.pfnAllowLagCompensation())
		return;

	if (g_psvs.maxclients <= 1 || sv_unlag.value == 0.0)
		return;

	if (!_host_client->lw || !_host_client->lc || !_host_client->active)
		return;

	for (int i = 0; i < g_psvs.maxclients; i++)
	{
		cli = &g_psvs.clients[i];
		pos = &truepositions[i];

		if (cli == _host_client ||! cli->active)
			continue;

		if (VectorCompare(pos->neworg, pos->oldorg) || !pos->needrelink)
			continue;

		if (!pos->active)
		{
			Con_DPrintf("SV_RestoreMove:  Tried to restore 'inactive' player %i/%s\n", i, &cli->name[4]);
			continue;
		}

		if (VectorCompare(pos->initial_correction_org, cli->edict->v.origin))
		{
			cli->edict->v.origin[0] = pos->oldorg[0];
			cli->edict->v.origin[1] = pos->oldorg[1];
			cli->edict->v.origin[2] = pos->oldorg[2];
			SV_LinkEdict(cli->edict, FALSE);
		}
	}
}

void SV_ParseStringCommand(client_t *pSenderClient)
{
	//check string commands rate for this player
#ifdef REHLDS_FIXES
	g_StringCommandsRateLimiter.StringCommandIssued(pSenderClient - g_psvs.clients);

	if (!pSenderClient->connected) {
		return; //return if player was kicked
	}
#endif

	char *s = MSG_ReadString();
	int ret = SV_ValidateClientCommand(s);
	switch (ret)
	{
	case 0:
#ifndef REHLDS_OPT_PEDANTIC
		if (Q_strlen(s) > 127)
#endif
		{
			s[127] = 0;
		}
		Cmd_TokenizeString(s);
		gEntityInterface.pfnClientCommand(sv_player);
		break;
	case 1:
		Cmd_ExecuteString(s, src_client);
		break;
	case 2:	// TODO: Check not used path
		Cbuf_InsertText(s);
		break;
	}
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
	return g_RehldsHookchains.m_SV_EstablishTimeBase.callChain(SV_EstablishTimeBase_mod, GetRehldsApiClient(cl), cmds, dropped, numbackup, numcmds);
}

void SV_EstablishTimeBase_internal(client_t *cl, usercmd_t *cmds, int dropped, int numbackup, int numcmds)
{
	int		i;
	double	runcmd_time = 0.0;
	double	time_at_end = 0.0;
	constexpr int MAX_DROPPED_CMDS = 24;

	// If we haven't dropped too many packets, then run some commands
	if (dropped < MAX_DROPPED_CMDS)
	{
		if (dropped > numbackup)
		{
			// Con_Printf("%s: lost %i cmds\n", __func__, dropped - numbackup);
		}

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
	for (i = numcmds - 1; i >= 0; i--)
	{
		time_at_end += cmds[i].msec / 1000.0;
	}

	cl->svtimebase = host_frametime + g_psv.time - (time_at_end + runcmd_time);
}

void SV_ParseMove(client_t *pSenderClient)
{
	client_frame_t *frame;
	int placeholder;
	int mlen;
	unsigned int packetLossByte;
	int numcmds;
	int totalcmds;
	byte cbpktchecksum;
	usercmd_t *cmd;
	usercmd_t cmds[64];
	usercmd_t cmdNull;
	float packet_loss;
	byte cbchecksum;
	int numbackup;

	if (g_balreadymoved)
	{
		msg_badread = 1;
		return;
	}
	g_balreadymoved = 1;

	frame = &host_client->frames[SV_UPDATE_MASK & host_client->netchan.incoming_acknowledged];
	Q_memset(&cmdNull, 0, sizeof(cmdNull));

	placeholder = msg_readcount + 1;
	mlen = MSG_ReadByte();
	cbchecksum = MSG_ReadByte();

	if (mlen <= 0 || !SZ_HasSpaceToRead(&net_message, mlen))
	{
		msg_badread = TRUE;
		Con_DPrintf("%s:  %s:%s invalid length: %d\n", __func__, host_client->name, NET_AdrToString(host_client->netchan.remote_address), mlen);
		SV_DropClient(host_client, FALSE, "Invalid length");
		return;
	}

	COM_UnMunge(&net_message.data[placeholder + 1], mlen, host_client->netchan.incoming_sequence);

	packetLossByte = MSG_ReadByte();
	numbackup = MSG_ReadByte();
	numcmds = MSG_ReadByte();

	packet_loss = float(packetLossByte & 0x7F);
	pSenderClient->m_bLoopback = (packetLossByte >> 7) & 1;
	totalcmds = numcmds + numbackup;
	net_drop += 1 - numcmds;
	if (totalcmds < 0 || totalcmds >= CMD_MAXBACKUP - 1)
	{
		Con_Printf("SV_ReadClientMessage: too many cmds %i sent for %s/%s\n", totalcmds, host_client->name, NET_AdrToString(host_client->netchan.remote_address));
		SV_DropClient(host_client, FALSE, "CMD_MAXBACKUP hit");
		msg_badread = 1;
		return;
	}

	usercmd_t* from = &cmdNull;
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

	cbpktchecksum = COM_BlockSequenceCRCByte(&net_message.data[placeholder + 1], msg_readcount - placeholder - 1, host_client->netchan.incoming_sequence);
	if (cbpktchecksum != cbchecksum)
	{
		Con_DPrintf("Failed command checksum for %s:%s\n", host_client->name, NET_AdrToString(host_client->netchan.remote_address));
		msg_badread = 1;
		return;
	}

	host_client->packet_loss = packet_loss;
	if (!g_psv.paused && (g_psvs.maxclients > 1 || !key_dest) && !(sv_player->v.flags & FL_FROZEN))
	{
		sv_player->v.v_angle[0] = cmds[0].viewangles[0];
		sv_player->v.v_angle[1] = cmds[0].viewangles[1];
		sv_player->v.v_angle[2] = cmds[0].viewangles[2];
	}
	else
	{
		for (int i = 0; i < numcmds; i++)
		{
			cmd = &cmds[i];
			cmd->msec = 0;
			cmd->forwardmove = 0;
			cmd->sidemove = 0;
			cmd->upmove = 0;
			cmd->buttons = 0;

			if (sv_player->v.flags & FL_FROZEN)
				cmd->impulse = 0;

			cmd->viewangles[0] = sv_player->v.v_angle[0];
			cmd->viewangles[1] = sv_player->v.v_angle[1];
			cmd->viewangles[2] = sv_player->v.v_angle[2];
		}

		net_drop = 0;
	}

	//check move commands rate for this player
#ifdef REHLDS_FIXES
	int numCmdsToIssue = numcmds;
	if (net_drop > 0) {
		numCmdsToIssue += net_drop;
	}
	g_MoveCommandRateLimiter.MoveCommandsIssued(host_client - g_psvs.clients, numCmdsToIssue);

	if (!host_client->connected) {
		return; //return if player was kicked
	}
#endif

#ifndef REHLDS_FIXES
	// dup and more correct in SV_RunCmd
	sv_player->v.button = cmds[0].buttons;
	sv_player->v.light_level = cmds[0].lightlevel;
#endif
	SV_EstablishTimeBase(host_client, cmds, net_drop, numbackup, numcmds);
	if (net_drop < 24)
	{
		while (net_drop > numbackup)
		{
			SV_RunCmd(&host_client->lastcmd, 0);
			net_drop--;
		}

		while (net_drop > 0)
		{
			SV_RunCmd(&cmds[numcmds + net_drop - 1], host_client->netchan.incoming_sequence - (numcmds + net_drop - 1));
			net_drop--;
		}

	}

	for (int i = numcmds - 1; i >= 0; i--)
	{
		SV_RunCmd(&cmds[i], host_client->netchan.incoming_sequence - i);
	}

#ifdef REHLDS_FIXES
	if (numcmds)
		host_client->lastcmd = cmds[numcmds - 1];
	else if (numbackup)
		host_client->lastcmd = cmds[0];
#else
	host_client->lastcmd = cmds[0];
#endif

	frame->ping_time -= float(host_client->lastcmd.msec * 0.5 / 1000.0);
	if (frame->ping_time < 0.0)
		frame->ping_time = 0;

	if (sv_player->v.animtime > host_frametime + g_psv.time)
		sv_player->v.animtime = float(host_frametime + g_psv.time);
}

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

	if (sv_voiceenable.value == 0.0f)
		return;

	for (int i = 0; i < g_psvs.maxclients; i++)
	{
		client_t *pDestClient = &g_psvs.clients[i];
		if (!((1 << (i & 0x1F)) & cl->m_VoiceStreams[i >> 5]) && i != iClient)
			continue;

		if (!pDestClient->active && !pDestClient->connected && i != iClient)
			continue;

		int nSendLength = nDataLength;
		if (i == iClient && !pDestClient->m_bLoopback)
			nSendLength = 0;

		if (pDestClient->datagram.cursize + nSendLength + 6 < pDestClient->datagram.maxsize)
		{
			MSG_WriteByte(&pDestClient->datagram, svc_voicedata);
			MSG_WriteByte(&pDestClient->datagram, iClient);
			MSG_WriteShort(&pDestClient->datagram, nSendLength);
			MSG_WriteBuf(&pDestClient->datagram, nSendLength, chReceived);
		}
	}
}

void SV_IgnoreHLTV(client_t *cl)
{
}

void SV_ParseCvarValue(client_t *cl)
{
	char *value;
	value = MSG_ReadString();
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

	char* value = MSG_ReadString();

	if (gNewDLLFunctions.pfnCvarValue2)
		gNewDLLFunctions.pfnCvarValue2(cl->edict, requestID, cvarName, value);

	Con_DPrintf("Cvar query response: name:%s, request ID %d, cvar:%s, value:%s\n", cl->name, requestID, cvarName, value);
}

void EXT_FUNC SV_HandleClientMessage_api(IGameClient* client, uint8 opcode) {
	client_t* cl = client->GetClient();
	if (opcode < clc_bad || opcode > clc_cvarvalue2)
	{
		// TODO: Are we forced to use msg_badread for break the loop.
		static_assert(REHLDS_API_VERSION_MAJOR <= 3, "Bump major API DETECTED!! You shall rework the hookchain, make function returnable");
		msg_badread = 1;

		Con_Printf("SV_ReadClientMessage: unknown command char (%d)\n", opcode);
		SV_DropClient(cl, FALSE, "Bad command character in client command");
		return;
	}

#ifdef REHLDS_FIXES
	// Save current name of the client before a possible kick
	char name[32];
	Q_strlcpy(name, host_client->name);
#endif

	void(*func)(client_t *) = sv_clcfuncs[opcode].pfnParse;
	if (func)
		func(cl);

#ifdef REHLDS_FIXES
	if (msg_badread)
	{
		Con_Printf("SV_ReadClientMessage: badread on %s, opcode %s\n", name, sv_clcfuncs[opcode].pszname);
	}
#endif

}

void SV_ExecuteClientMessage(client_t *cl)
{
	g_balreadymoved = 0;
	client_frame_t * frame = &cl->frames[SV_UPDATE_MASK & cl->netchan.incoming_acknowledged];
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
	IGameClient* apiClient = GetRehldsApiClient(cl);

	while (1)
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

		g_RehldsHookchains.m_HandleNetCommand.callChain(SV_HandleClientMessage_api, apiClient, c);

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
			return 1;
		}
	}

	Con_Printf("Userid %i is not on the server\n", idnum);
	return 0;
}

void SV_ShowServerinfo_f(void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
	}
	else
	{
#ifdef REHLDS_FIXES
		// fix print message client console
		SV_ClientPrintf(Info_Serverinfo());
#else
		Info_Print(Info_Serverinfo());
#endif // REHLDS_FIXES
	}
}

void SV_SendEnts_f(void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
	}
	else
	{
		if (host_client->active && host_client->spawned)
		{
			if (host_client->connected)
			{
				host_client->fully_connected = TRUE;

#ifdef REHLDS_FIXES
				// See SV_CheckFile function
				if (sv_delayed_spray_upload.value)
				{
					resource_t *res = host_client->resourcesneeded.pNext;
					if (res != &host_client->resourcesneeded)
					{
						// TODO: all this is already checked earlier
						if (res->ucFlags & RES_WASMISSING && res->type == t_decal && res->ucFlags & RES_CUSTOM)
						{
							if (sv_rehlds_force_dlmax.value)
							{
								MSG_WriteByte(&host_client->netchan.message, svc_stufftext);
								MSG_WriteString(&host_client->netchan.message, va("cl_dlmax %i\n", FRAGMENT_MAX_SIZE));
							}

							MSG_WriteByte(&host_client->netchan.message, svc_stufftext);
							MSG_WriteString(&host_client->netchan.message, va("upload !MD5%s\n", MD5_Print(res->rgucMD5_hash)));
						}
					}
				}
#endif // REHLDS_FIXES
			}
		}
	}
}

void SV_FullUpdate_f(void)
{
	int entIndex;
	float ltime;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (host_client->active)
	{
		entIndex = IndexOfEdict(host_client->edict);
		if (s_LastFullUpdate[entIndex] > g_psv.time)
			s_LastFullUpdate[entIndex] = 0;

		ltime = g_psv.time - s_LastFullUpdate[entIndex];
		if (ltime <= 0.0)
			ltime = 0.0;

		if (ltime < 0.45 && g_psv.time > 0.45)
		{
			Con_DPrintf(
				"%s is spamming fullupdate: (%f) (%f) (%f)\n",
				host_client->name,
				g_psv.time,
				s_LastFullUpdate[entIndex],
				ltime);
			return;
		}
		s_LastFullUpdate[entIndex] = g_psv.time;

#ifdef REHLDS_FIXES
		// it's not need until not active
		SV_ForceFullClientsUpdate();
		gEntityInterface.pfnClientCommand( sv_player );
#endif // REHLDS_FIXES
	}

#ifndef REHLDS_FIXES
	SV_ForceFullClientsUpdate();
	gEntityInterface.pfnClientCommand(sv_player);
#endif // REHLDS_FIXES
}
