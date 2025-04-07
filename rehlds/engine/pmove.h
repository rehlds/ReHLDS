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
#include "pm_defs.h"
#include "pm_movevars.h"

extern cvar_t pm_showclip;

extern vec3_t player_mins[MAX_MAP_HULLS];
extern vec3_t player_maxs[MAX_MAP_HULLS];

extern playermove_t *pmove;
extern movevars_t sv_movevars;

qboolean PM_AddToTouched(pmtrace_t tr, vec_t *impactvelocity);
void PM_StuckTouch(int hitent, pmtrace_t *ptraceresult);
void PM_Init(playermove_t *ppm);
