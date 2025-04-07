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

playermove_t *pmove;
movevars_t sv_movevars;

cvar_t pm_showclip = { "pm_showclip", "0", 0, 0.0f, NULL };

vec3_t player_mins[MAX_MAP_HULLS] = {
	{ -16.0f, -16.0f, -36.0f, },
	{ -16.0f, -16.0f, -18.0f, },
	{  0.0f,   0.0f,   0.0f,  },
	{ -32.0f, -32.0f, -32.0f, }
};

vec3_t player_maxs[MAX_MAP_HULLS] = {
	{ 16.0f, 16.0f, 36.0f, },
	{ 16.0f, 16.0f, 18.0f, },
	{ 0.0f,  0.0f,  0.0f,  },
	{ 32.0f, 32.0f, 32.0f, }
};

qboolean PM_AddToTouched(pmtrace_t tr, vec_t *impactvelocity)
{
	int i;
	for (i = 0; i < pmove->numtouch; i++)
	{
		if (pmove->touchindex[i].ent == tr.ent)
		{
			return FALSE;
		}
	}

	if (pmove->numtouch >= MAX_PHYSENTS)
	{
		pmove->Con_DPrintf("Too many entities were touched!\n");
#ifdef REHLDS_FIXES
		return FALSE;	// FIXED: added for preventing buffer overrun
#endif
	}

	VectorCopy(impactvelocity, tr.deltavelocity);
	pmove->touchindex[pmove->numtouch++] = tr;

	return TRUE;
}

void EXT_FUNC PM_StuckTouch(int hitent, pmtrace_t *ptraceresult)
{
#ifdef REHLDS_CHECKS
	if (hitent >= MAX_PHYSENTS)	// FIXED: added for preventing buffer overrun
	{
		return;
	}
#endif

	if (pmove->server)
	{
		int n = pmove->physents[hitent].info;
		edict_t *info = EDICT_NUM(n);	// looks like just entity index check
		PM_AddToTouched(*ptraceresult, pmove->velocity);
	}
}

void PM_Init(playermove_t *ppm)
{
	PM_InitBoxHull();
	for (int i = 0; i < MAX_MAP_HULLS; i++)
	{
		VectorCopy(player_mins[i], ppm->player_mins[i]);
		VectorCopy(player_maxs[i], ppm->player_maxs[i]);
	}

	ppm->movevars = &sv_movevars;

	ppm->PM_Info_ValueForKey = Info_ValueForKey;
	ppm->PM_Particle = CL_Particle;
	ppm->PM_TestPlayerPosition = PM_TestPlayerPosition;
	ppm->Con_NPrintf = Con_NPrintf;
	ppm->Con_DPrintf = Con_DPrintf;
	ppm->Con_Printf = Con_Printf;
	ppm->Sys_FloatTime = Sys_FloatTime;
	ppm->PM_StuckTouch = PM_StuckTouch;
	ppm->PM_PointContents = PM_PointContents;
	ppm->PM_TruePointContents = PM_TruePointContents;
	ppm->PM_HullPointContents = PM_HullPointContents;
	ppm->PM_PlayerTrace = PM_PlayerTrace;
	ppm->PM_TraceLine = PM_TraceLine;
	ppm->PM_TraceModel = PM_TraceModel;
	ppm->RandomLong = RandomLong;
	ppm->RandomFloat = RandomFloat;
	ppm->PM_GetModelType = PM_GetModelType;
	ppm->PM_HullForBsp = PM_HullForBsp;
	ppm->PM_GetModelBounds = PM_GetModelBounds;
	ppm->COM_FileSize = COM_FileSize;
	ppm->COM_LoadFile = COM_LoadFile;
	ppm->COM_FreeFile = COM_FreeFile;
	ppm->memfgets = memfgets;
	ppm->PM_PlayerTraceEx = PM_PlayerTraceEx;
	ppm->PM_TestPlayerPositionEx = PM_TestPlayerPositionEx;
	ppm->PM_TraceLineEx = PM_TraceLineEx;
}
