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

void ED_ClearEdict(edict_t *e)
{
	Q_memset(&e->v, 0, sizeof(e->v));
	e->free = FALSE;
	ReleaseEntityDLLFields(e);
	InitEntityDLLFields(e);
}

edict_t *ED_Alloc(void)
{
	return g_RehldsHookchains.m_ED_Alloc.callChain(ED_Alloc_internal);
}

edict_t *EXT_FUNC ED_Alloc_internal(void)
{
	int i;
	edict_t *e;

	// Search for free entity
	for (i = g_psvs.maxclients + 1; i < g_psv.num_edicts; i++)
	{
		e = &g_psv.edicts[i];
		if (e->free && (e->freetime <= 2.0 || g_psv.time - e->freetime >= 0.5))
		{
			ED_ClearEdict(e);
			return e;
		}
	}

	// Check if we are out of free edicts
	if (i >= g_psv.max_edicts)
	{
		if (!g_psv.max_edicts)
		{
			Sys_Error("%s: no edicts yet", __func__);
		}
		Sys_Error("%s: no free edicts", __func__);
	}

	// Use new one
	++g_psv.num_edicts;
	e = &g_psv.edicts[i];

	ED_ClearEdict(e);
	return e;
}

void ED_Free(edict_t *ed)
{
	g_RehldsHookchains.m_ED_Free.callChain(ED_Free_internal, ed);	
}

void EXT_FUNC ED_Free_internal(edict_t *ed)
{
	if (!ed->free)
	{
		SV_UnlinkEdict(ed);
		FreeEntPrivateData(ed);
		ed->serialnumber++;
		ed->freetime = (float)g_psv.time;
		ed->free = TRUE;
		ed->v.flags = 0;
		ed->v.model = 0;

		ed->v.takedamage = 0;
		ed->v.modelindex = 0;
		ed->v.colormap = 0;
		ed->v.skin = 0;
		ed->v.frame = 0;
		ed->v.scale = 0;
		ed->v.gravity = 0;
		ed->v.nextthink = -1.0;
		ed->v.solid = SOLID_NOT;

		ed->v.origin[0] = vec3_origin[0];
		ed->v.origin[1] = vec3_origin[1];
		ed->v.origin[2] = vec3_origin[2];
		ed->v.angles[0] = vec3_origin[0];
		ed->v.angles[1] = vec3_origin[1];
		ed->v.angles[2] = vec3_origin[2];
	}
}

NOXREF void ED_Count(void)
{
	NOXREFCHECK;

	int i;
	edict_t *ent;
	int active = 0, models = 0, solid = 0, step = 0;

	for (i = 0; i < g_psv.num_edicts; i++)
	{
		ent = &g_psv.edicts[i];
		if (!ent->free)
		{
			++active;
			models += (ent->v.model) ? 1 : 0;
			solid += (ent->v.solid) ? 1 : 0;
			step += (ent->v.movetype == MOVETYPE_STEP) ? 1 : 0;
		}
	}

	Con_Printf("num_edicts:%3i\n", g_psv.num_edicts);
	Con_Printf("active    :%3i\n", active);
	Con_Printf("view      :%3i\n", models);
	Con_Printf("touch     :%3i\n", solid);
	Con_Printf("step      :%3i\n", step);
}

char *ED_NewString(const char *string)
{
	char *new_s;

	// Engine string pooling
#ifdef REHLDS_FIXES

	// escaping is done inside Ed_StrPool_Alloc()
	new_s = Ed_StrPool_Alloc(string);

#else // REHLDS_FIXES

	int l = Q_strlen(string);
	new_s = (char *)Hunk_Alloc(l + 1);
	char* new_p = new_s;

	for (int i = 0; i < l; i++, new_p++)
	{
		if (string[i] == '\\')
		{
			if (string[i + 1] == 'n')
				*new_p = '\n';
			else
				*new_p = '\\';
			i++;
		}
		else
		{
			*new_p = string[i];
		}
	}
	*new_p = 0;

#endif // REHLDS_FIXES

	return new_s;
}

char *ED_ParseEdict(char *data, edict_t *ent)
{
	qboolean init = FALSE;
	char keyname[256];
	int n;
	ENTITYINIT pEntityInit;
	char *className;
	KeyValueData kvd;

	if (ent != g_psv.edicts)
	{
		Q_memset(&ent->v, 0, sizeof(ent->v));
	}

	InitEntityDLLFields(ent);

	if (SuckOutClassname(data, ent))
	{
		className = (char *)(pr_strings + ent->v.classname);

		pEntityInit = GetEntityInit(className);
		if (pEntityInit)
		{
			pEntityInit(&ent->v);
			init = TRUE;
		}
		else
		{
			pEntityInit = GetEntityInit("custom");
			if (pEntityInit)
			{
				pEntityInit(&ent->v);
				kvd.szClassName = "custom";
				kvd.szKeyName = "customclass";
				kvd.szValue = className;
				kvd.fHandled = FALSE;
				gEntityInterface.pfnKeyValue(ent, &kvd);
				init = TRUE;
			}
			else
			{
				Con_DPrintf("Can't init %s\n", className);
				init = FALSE;
			}
		}

		while (true)
		{
			data = COM_Parse(data);
			if (com_token[0] == '}')
			{
				break;
			}
			if (!data)
			{
				Host_Error("%s: EOF without closing brace", __func__);
			}

			Q_strncpy(keyname, com_token, ARRAYSIZE(keyname) - 1);
			keyname[ARRAYSIZE(keyname) - 1] = 0;
			// Remove tail spaces
			for (n = Q_strlen(keyname) - 1; n >= 0 && keyname[n] == ' '; n--)
			{
				keyname[n] = 0;
			}

			data = COM_Parse(data);
			if (!data)
			{
				Host_Error("%s: EOF without closing brace", __func__);
			}
			if (com_token[0] == '}')
			{
				Host_Error("%s: closing brace without data", __func__);
			}

			if (className != NULL && !Q_strcmp(className, com_token))
			{
				continue;
			}

			if (!Q_strcmp(keyname, "angle"))
			{
				float value = (float)Q_atof(com_token);
				if (value >= 0.0)
				{
					Q_snprintf(com_token, COM_TOKEN_LEN, "%f %f %f", ent->v.angles[0], value, ent->v.angles[2]);
				}
				else if ((int)value == -1)
				{
					Q_snprintf(com_token, COM_TOKEN_LEN, "-90 0 0");
				}
				else
				{
					Q_snprintf(com_token, COM_TOKEN_LEN, "90 0 0");
				}

				Q_strcpy(keyname, "angles");
			}
#ifdef REHLDS_FIXES
			else if (!Q_strcmp(keyname, "model"))
			{
				// local model?
				if (com_token[0] == '*')
				{
					// make sure that local model not pre-cached yet
					if (!SV_LookupModelIndex(com_token))
					{
						// find empty slot
						int i;
						for (i = 0; i < MAX_MODELS; i++)
						{
							if (!g_psv.model_precache[i])
								break;
						}

						int index = Q_atoi(com_token + 1);
						if (i == MAX_MODELS)
						{
							Host_Error("%s: Model '%s' failed to precache because the item count is over the %d limit.\n"
								"Reduce the number of brush models and/or regular models in the map to correct this.", __func__,
								localmodels[index], MAX_MODELS);
						}

						g_psv.model_precache[i] = localmodels[index];
						g_psv.models[i] = Mod_ForName(localmodels[index], FALSE, FALSE);
						g_psv.model_precache_flags[i] |= RES_FATALIFMISSING;

#ifdef REHLDS_OPT_PEDANTIC
						g_rehlds_sv.modelsMap.put(g_psv.model_precache[i], i);
#endif
					}
				}
			}
#endif

			kvd.szClassName = className;
			kvd.szKeyName = keyname;
			kvd.szValue = com_token;
			kvd.fHandled = FALSE;
			gEntityInterface.pfnKeyValue(ent, &kvd);
		}
	}

	if (!init)
	{
		ent->free = 1;
		ent->serialnumber++;
	}
	return data;
}

void ED_LoadFromFile(char *data)
{
	edict_t *ent;
	int inhibit;

	gGlobalVariables.time = (float)g_psv.time;

	ent = NULL;
	inhibit = 0;
	while (true)
	{
		data = COM_Parse(data);
		if (!data)
		{
			break;
		}
		if (com_token[0] != '{')
		{
			Host_Error("%s: found %s when expecting {", __func__, com_token);
		}

		if (ent)
		{
			ent = ED_Alloc();
		}
		else
		{
			ent = g_psv.edicts;
			ReleaseEntityDLLFields(g_psv.edicts);	// TODO: May be better to call ED_ClearEdict here?
			InitEntityDLLFields(ent);
		}

		data = ED_ParseEdict(data, ent);
		if (ent->free)
		{
			continue;
		}

		if (deathmatch.value != 0.0 && (ent->v.spawnflags & SF_NOTINDEATHMATCH))
		{
			ED_Free(ent);
			++inhibit;
		}
		else
		{
			if (ent->v.classname)
			{
				if (gEntityInterface.pfnSpawn(ent) < 0 || (ent->v.flags & FL_KILLME))
				{
					ED_Free(ent);
				}
			}
			else
			{
				Con_Printf("No classname for:\n");
				ED_Free(ent);
			}
		}
	}
	Con_DPrintf("%i entities inhibited\n", inhibit);
}

NOXREF void PR_Init(void)
{
	NOXREFCHECK;
}

edict_t *EDICT_NUM(int n)
{
	if (n < 0 || n >= g_psv.max_edicts)
	{
		Sys_Error("%s: bad number %i", __func__, n);
	}
	return &g_psv.edicts[n];
}

int NUM_FOR_EDICT(const edict_t *e)
{
	int b;
	b = e - g_psv.edicts;

	if (b < 0 || b >= g_psv.num_edicts)
	{
		Sys_Error("%s: bad pointer", __func__);
	}

	return b;
}

bool SuckOutClassname(char *szInputStream, edict_t *pEdict)
{
	char szKeyName[256];
	KeyValueData kvd;

	// key
	szInputStream = COM_Parse(szInputStream);
	while (szInputStream && com_token[0] != '}')
	{
		Q_strncpy(szKeyName, com_token, ARRAYSIZE(szKeyName) - 1);
		szKeyName[ARRAYSIZE(szKeyName) - 1] = 0;

		// value
		szInputStream = COM_Parse(szInputStream);

		if (!Q_strcmp(szKeyName, "classname"))
		{
			kvd.szClassName = NULL;
			kvd.szKeyName = szKeyName;
			kvd.szValue = com_token;
			kvd.fHandled = FALSE;

			gEntityInterface.pfnKeyValue(pEdict, &kvd);

			if (kvd.fHandled == FALSE)
			{
				Host_Error("%s: parse error", __func__);
			}

			return true;
		}

		if (!szInputStream)
		{
			break;
		}

		// next key
		szInputStream = COM_Parse(szInputStream);
	}

#ifdef REHLDS_FIXES
	if (pEdict == g_psv.edicts)
	{
		kvd.szClassName = NULL;
		kvd.szKeyName = "classname";
		kvd.szValue = "worldspawn";
		kvd.fHandled = FALSE;

		gEntityInterface.pfnKeyValue(pEdict, &kvd);

		if (kvd.fHandled == FALSE)
		{
			Host_Error("%s: parse error", __func__);
		}

		return true;
	}
#endif

	// classname not found
	return false;
}

void ReleaseEntityDLLFields(edict_t *pEdict)
{
	FreeEntPrivateData(pEdict);
}

void InitEntityDLLFields(edict_t *pEdict)
{
	pEdict->v.pContainingEntity = pEdict;
}

void* EXT_FUNC PvAllocEntPrivateData(edict_t *pEdict, int32 cb)
{
	FreeEntPrivateData(pEdict);

	if (cb <= 0)
	{
		return NULL;
	}

	pEdict->pvPrivateData = Mem_Calloc(1, cb);

#ifdef REHLDS_FLIGHT_REC
	if (rehlds_flrec_pvdata.string[0] != '0') {
		FR_AllocEntPrivateData(pEdict->pvPrivateData, cb);
	}
#endif // REHLDS_FLIGHT_REC

	return pEdict->pvPrivateData;
}

void* EXT_FUNC PvEntPrivateData(edict_t *pEdict)
{
	if (!pEdict)
	{
		return NULL;
	}

	return pEdict->pvPrivateData;
}

void EXT_FUNC FreeEntPrivateData(edict_t *pEdict)
{
	if (pEdict->pvPrivateData)
	{
		if (gNewDLLFunctions.pfnOnFreeEntPrivateData)
		{
			gNewDLLFunctions.pfnOnFreeEntPrivateData(pEdict);
		}

#ifdef REHLDS_FLIGHT_REC
		if (rehlds_flrec_pvdata.string[0] != '0') {
			FR_FreeEntPrivateData(pEdict->pvPrivateData);
		}
#endif // REHLDS_FLIGHT_REC

		Mem_Free(pEdict->pvPrivateData);
		pEdict->pvPrivateData = 0;
	}
}

void FreeAllEntPrivateData(void)
{
	for (int i = 0; i < g_psv.num_edicts; i++)
	{
		FreeEntPrivateData(&g_psv.edicts[i]);
	}
}

edict_t* EXT_FUNC PEntityOfEntOffset(int iEntOffset)
{
	return (edict_t *)((char *)g_psv.edicts + iEntOffset);
}

int EXT_FUNC EntOffsetOfPEntity(const edict_t *pEdict)
{
	return (char *)pEdict - (char *)g_psv.edicts;
}

int EXT_FUNC IndexOfEdict(const edict_t *pEdict)
{
	int index = 0;
	if (pEdict)
	{
		index = pEdict - g_psv.edicts;
#ifdef REHLDS_FIXES
		if (index < 0 || index >= g_psv.max_edicts) // max_edicts is not valid entity index
#else // REHLDS_FIXES
		if (index < 0 || index > g_psv.max_edicts)
#endif // REHLDS_FIXES
		{
			Sys_Error("%s: bad entity", __func__);
		}
	}
	return index;
}

// HL25 SDK (enginefuncs_t::pfnPEntityOfEntIndexAllEntities). Matches the HL25 engine (HLDS 10211, black-box test):
// NULL for an index out of range, a free edict, or an edict above the client slots without private data.
// Unlike the original PEntityOfEntIndex it also returns the last client slot (index == maxclients) while it is empty.
edict_t* EXT_FUNC PEntityOfEntIndexAllEntities(int iEntIndex)
{
	if (iEntIndex < 0 || iEntIndex >= g_psv.max_edicts)
		return NULL;

	edict_t *pEdict = EDICT_NUM(iEntIndex);
	if (pEdict->free || (iEntIndex > g_psvs.maxclients && !pEdict->pvPrivateData))
		return NULL;

	return pEdict;
}

edict_t* EXT_FUNC PEntityOfEntIndex(int iEntIndex)
{
	if (iEntIndex < 0 || iEntIndex >= g_psv.max_edicts)
	{
		return NULL;
	}

	edict_t *pEdict = EDICT_NUM(iEntIndex);

#ifdef REHLDS_FIXES
	if (pEdict && (pEdict->free || (iEntIndex > g_psvs.maxclients && !pEdict->pvPrivateData))) // Simplified confition
	{
		return NULL;
	}
#else // REHLDS_FIXES
	if ((!pEdict || pEdict->free || !pEdict->pvPrivateData) && (iEntIndex >= g_psvs.maxclients || pEdict->free))
	{
		if (iEntIndex >= g_psvs.maxclients || pEdict->free)
		{
			pEdict = NULL;
		}
	}
#endif // REHLDS_FIXES

	return pEdict;
}

const char* EXT_FUNC SzFromIndex(int iString)
{
	return (const char *)(pr_strings + iString);
}

entvars_t* EXT_FUNC GetVarsOfEnt(edict_t *pEdict)
{
	return &pEdict->v;
}

edict_t* EXT_FUNC FindEntityByVars(entvars_t *pvars)
{
	for (int i = 0; i < g_psv.num_edicts; i++)
	{
		edict_t *pEdict = &g_psv.edicts[i];
		if (&pEdict->v == pvars)
		{
			return pEdict;
		}
	}
	return NULL;
}

float EXT_FUNC CVarGetFloat(const char *szVarName)
{
	return Cvar_VariableValue(szVarName);
}

const char* EXT_FUNC CVarGetString(const char *szVarName)
{
	return Cvar_VariableString(szVarName);
}

cvar_t* EXT_FUNC CVarGetPointer(const char *szVarName)
{
	return Cvar_FindVar(szVarName);
}

void EXT_FUNC CVarSetFloat(const char *szVarName, float flValue)
{
	Cvar_SetValue(szVarName, flValue);
}

void EXT_FUNC CVarSetString(const char *szVarName, const char *szValue)
{
	Cvar_Set(szVarName, szValue);
}

void EXT_FUNC CVarRegister(cvar_t *pCvar)
{
	if (pCvar)
	{
		pCvar->flags |= FCVAR_EXTDLL;
		Cvar_RegisterVariable(pCvar);
	}
}

int EXT_FUNC AllocEngineString(const char *szValue)
{
	return ED_NewString(szValue) - pr_strings;
}

void EXT_FUNC SaveSpawnParms(edict_t *pEdict)
{
	int eoffset = NUM_FOR_EDICT(pEdict);
	if (eoffset < 1 || eoffset > g_psvs.maxclients)
	{
		Host_Error("%s: Entity is not a client", __func__);
	}
	// Nothing more for this function even on client-side
}

void* EXT_FUNC GetModelPtr(edict_t *pEdict)
{
	if (!pEdict)
	{
		return NULL;
	}

	return Mod_Extradata(Mod_Handle(pEdict->v.modelindex));
}
