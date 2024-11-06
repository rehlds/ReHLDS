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

// Pushmove objects do not obey gravity, and do not interact with each other or trigger fields,
// but block normal movement and push normal objects when they move.
//
// onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects
//
// doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
// bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
// corpses are SOLID_NOT and MOVETYPE_TOSS
// crates are SOLID_BBOX and MOVETYPE_TOSS
// walking monsters are SOLID_BBOX and MOVETYPE_STEP
// flying/floating monsters are SOLID_BBOX and MOVETYPE_FLY
//
// solid_edge items only clip against bsp models.

vec3_t *g_moved_from;
edict_t **g_moved_edict;

cvar_t sv_maxvelocity = { "sv_maxvelocity", "2000", 0, 0.0f, NULL };
cvar_t sv_gravity = { "sv_gravity", "800", FCVAR_SERVER, 0.0f, NULL };
cvar_t sv_bounce = { "sv_bounce", "1", FCVAR_SERVER, 0.0f, NULL };
cvar_t sv_stepsize = { "sv_stepsize", "18", FCVAR_SERVER, 0.0f, NULL };
cvar_t sv_friction = { "sv_friction", "4", FCVAR_SERVER, 0.0f, NULL };
cvar_t sv_stopspeed = { "sv_stopspeed", "100", FCVAR_SERVER, 0.0f, NULL };

NOXREF void SV_CheckAllEnts()
{
	NOXREFCHECK;
	int e;
	edict_t *check;

	// see if any solid entities are inside the final position
	for (e = 1; e < g_psv.num_edicts; e++)
	{
		check = &g_psv.edicts[e];

		if (check->free)
			continue;

		if (check->v.movetype == MOVETYPE_NONE
			|| check->v.movetype == MOVETYPE_PUSH
			|| check->v.movetype == MOVETYPE_NOCLIP
			|| check->v.movetype == MOVETYPE_FOLLOW)
				continue;

		if (SV_TestEntityPosition(check) != NULL)
			Con_Printf("entity in invalid position\n");
	}
}

void SV_CheckVelocity(edict_t *ent)
{
	// bound velocity
	for (int i = 0; i < 3; i++)
	{
		if (IS_NAN(ent->v.velocity[i]))
		{
			Con_Printf("Got a NaN velocity on %s\n", &pr_strings[ent->v.classname]);
			ent->v.velocity[i] = 0;
		}

		if (IS_NAN(ent->v.origin[i]))
		{
			Con_Printf("Got a NaN origin on %s\n", &pr_strings[ent->v.classname]);
			ent->v.origin[i] = 0;
		}

		if (ent->v.velocity[i] > sv_maxvelocity.value)
		{
			Con_DPrintf("Got a velocity too high on %s\n", &pr_strings[ent->v.classname]);
			ent->v.velocity[i] = sv_maxvelocity.value;
		}
		else if (ent->v.velocity[i] < -sv_maxvelocity.value)
		{
			Con_DPrintf("Got a velocity too low on %s\n", &pr_strings[ent->v.classname]);
			ent->v.velocity[i] = -sv_maxvelocity.value;
		}
	}
}

// Runs thinking code if time. There is some play in the exact time the think
// function will be called, because it is called before any movement is done
// in a frame. Not used for pushmove objects, because they must be exact.
// Returns false if the entity removed itself.
qboolean SV_RunThink(edict_t *ent)
{
	float thinktime;

	if (!(ent->v.flags & FL_KILLME))
	{
		thinktime = ent->v.nextthink;
		if (thinktime <= 0.0 || thinktime > g_psv.time + host_frametime)
			return TRUE;

		if (thinktime < g_psv.time)
			thinktime = g_psv.time;	// don't let things stay in the past.
									// it is possible to start that way
									// by a trigger with a local time.

		ent->v.nextthink = 0.0f;
		gGlobalVariables.time = thinktime;
		gEntityInterface.pfnThink(ent);
	}

	if (ent->v.flags & FL_KILLME)
	{
		ED_Free(ent);
	}

	return !ent->free;
}

// Two entities have touched, so run their touch functions
void SV_Impact(edict_t *e1, edict_t *e2, trace_t *ptrace)
{
	gGlobalVariables.time = g_psv.time;

	if ((e1->v.flags & FL_KILLME) || (e2->v.flags & FL_KILLME))
		return;

	if (e1->v.groupinfo && e2->v.groupinfo)
	{
		if (g_groupop)
		{
			if (g_groupop == GROUP_OP_NAND && (e1->v.groupinfo & e2->v.groupinfo))
				return;
		}
		else
		{
			if (!(e1->v.groupinfo & e2->v.groupinfo))
				return;
		}
	}

	if (e1->v.solid != SOLID_NOT)
	{
		SV_SetGlobalTrace(ptrace);
		gEntityInterface.pfnTouch(e1, e2);

#ifdef REHLDS_FIXES
		if ((e1->v.flags & FL_KILLME) || (e2->v.flags & FL_KILLME))
			return;
#endif // REHLDS_FIXES
	}

	if (e2->v.solid != SOLID_NOT)
	{
		SV_SetGlobalTrace(ptrace);
		gEntityInterface.pfnTouch(e2, e1);
	}
}

const float STOP_EPSILON = 0.1f;

// Slide off of the impacting object
void SV_ClipVelocity(vec_t *in, vec_t *normal, vec_t *out, float overbounce)
{
	float change;
	float backoff = _DotProduct(in, normal) * overbounce;

	for (int i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;

		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0.0f;
	}
}

#define MAX_CLIP_PLANES 5

// The basic solid body movement clip that slides along multiple planes
void SV_FlyMove(edict_t *ent, float time, float bounce)
{
	int numplanes = 0;		// and not sliding along any planes
	float time_left = time;	// Total time for this movement operation.

	vec3_t move;
	vec3_t planes[MAX_CLIP_PLANES];
	vec3_t primal_velocity, original_velocity, new_velocity;

	VectorCopy(ent->v.velocity, original_velocity); // Store original velocity
	VectorCopy(ent->v.velocity, primal_velocity);

	int moveType;

#ifdef REHLDS_FIXES
	if (ent->v.solid == SOLID_TRIGGER || ent->v.solid == SOLID_NOT)
		moveType = MOVE_NOMONSTERS;
	else
#endif
		moveType = MOVE_NORMAL;

	qboolean monsterClip = (ent->v.flags & FL_MONSTERCLIP) ? TRUE : FALSE;

	for (int bumpcount = 0; bumpcount < MAX_CLIP_PLANES - 1; bumpcount++)
	{
		if (VectorIsZero(ent->v.velocity))
			break;

		// Assume we can move all the way from the current origin to the end point
		VectorMA(ent->v.origin, time_left, ent->v.velocity, move);

		// See if we can make it from origin to end point
		trace_t trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, move, moveType, ent, monsterClip);

		// If we started in a solid object, or we were in solid space the whole way, zero out our velocity.
		if (trace.allsolid)
		{
			// entity is trapped in another solid
			VectorClear(ent->v.velocity);
			break;
		}

		// If we moved some portion of the total distance,
		// the copy the end position into the ent->v.origin and zero the plane counter.
		if (trace.fraction > 0.0f)
		{
			trace_t test = SV_Move(trace.endpos, ent->v.mins, ent->v.maxs, trace.endpos, moveType, ent, monsterClip);
			if (!test.allsolid)
			{
				// actually covered some distance
				VectorCopy(trace.endpos, ent->v.origin);
				VectorCopy(ent->v.velocity, original_velocity);
				numplanes = 0;
			}
		}

		// If we covered the entire distance, we are done and can return
		if (trace.fraction == 1.0f)
		{
			// moved the entire distance
			break;
		}

		if (!trace.ent)
		{
			Sys_Error("%s: !trace.ent", __func__);
		}

		// stop if on ground
		if (trace.plane.normal[2] > 0.7f)
		{
			if (trace.ent->v.solid == SOLID_BSP
				|| trace.ent->v.solid == SOLID_SLIDEBOX
				|| trace.ent->v.movetype == MOVETYPE_PUSHSTEP
				|| (
#ifdef REHLDS_FIXES
					// Should be a case when entity collided into the client that has non-standard SOLID_BBOX set
					trace.
#endif
					ent->v.flags & FL_CLIENT) == FL_CLIENT)
			{
				ent->v.flags |= FL_ONGROUND;
				ent->v.groundentity = trace.ent;
			}

#ifdef REHLDS_FIXES
			if ((ent->v.flags & FL_ONGROUND) && ent->v.movetype == MOVETYPE_TOSS)
			{
				VectorClear(ent->v.velocity);

				// run the impact function
				SV_Impact(ent, trace.ent, &trace);
				return;
			}
#endif
		}

		// run the impact function
		SV_Impact(ent, trace.ent, &trace);

		// break if removed by the impact function
		if (ent->free)
			break;

		// Reduce amount of host_frametime left by total time left * fraction that we covered
		time_left -= time_left * trace.fraction;

		// clipped to another plane
		// Did we run out of planes to clip against?
		if (numplanes >= MAX_CLIP_PLANES)
		{
			// this shouldn't really happen
			// stop our movement if so
			VectorClear(ent->v.velocity);
			break;
		}

		// Set up next clipping plane
		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		if (numplanes == 1 && ent->v.movetype == MOVETYPE_WALK && (!(ent->v.flags & FL_ONGROUND) || ent->v.friction != 1.0f))
		{
			float d;
			if (planes[0][2] <= 0.7f)
			{
				d = (1.0f - ent->v.friction) * sv_bounce.value + 1.0f;
			}
			else
			{
				d = 1.0f;
			}

			SV_ClipVelocity(original_velocity, planes[0], new_velocity, d);

			// go along this plane
			VectorCopy(new_velocity, ent->v.velocity);
			VectorCopy(new_velocity, original_velocity);
		}
		else
		{
			int i, j;

			// modify original_velocity so it parallels all of the clip planes
			for (i = 0; i < numplanes; i++)
			{
				SV_ClipVelocity(original_velocity, planes[i], new_velocity, bounce);

				for (j = 0; j < numplanes; j++)
				{
					if (j != i)
					{
						if (_DotProduct(new_velocity, planes[j]) < 0.0f)
							break; // not ok
					}
				}

				if (j == numplanes)
					break;
			}

			if (i != numplanes)
			{
				// go along this plane
				VectorCopy(new_velocity, ent->v.velocity);
			}
			else
			{
				// go along the crease
				if (numplanes != 2)
				{
					//VectorClear(ent->v.velocity);
					break;
				}

				vec3_t dir;
				CrossProduct(planes[0], planes[1], dir);
				float d = _DotProduct(dir, ent->v.velocity);
				VectorScale(dir, d, ent->v.velocity);
			}

			// if current velocity is against the original velocity,
			// stop dead to avoid tiny occilations in sloping corners
			if (_DotProduct(ent->v.velocity, primal_velocity) <= 0.0f)
			{
				VectorClear(ent->v.velocity);
				break;
			}
		}
	}
}

void SV_AddGravity(edict_t *ent)
{
	float ent_gravity;
	if (ent->v.gravity)
		ent_gravity = ent->v.gravity;
	else
		ent_gravity = 1.0f;

	ent->v.velocity[2] -= (ent_gravity * sv_gravity.value * host_frametime);
	ent->v.velocity[2] += (ent->v.basevelocity[2] * host_frametime);
	ent->v.basevelocity[2] = 0.0f;

	SV_CheckVelocity(ent);
}

NOXREF void SV_AddCorrectGravity(edict_t *ent)
{
	NOXREFCHECK;
	float ent_gravity;
	if (ent->v.gravity)
		ent_gravity = ent->v.gravity;
	else
		ent_gravity = 1.0f;

	ent->v.velocity[2] -= (ent_gravity * sv_gravity.value * 0.5f * host_frametime);
	ent->v.velocity[2] += (ent->v.basevelocity[2] * host_frametime);
	ent->v.basevelocity[2] = 0.0f;

	SV_CheckVelocity(ent);
}

NOXREF void SV_FixupGravityVelocity(edict_t *ent)
{
	NOXREFCHECK;
	float ent_gravity;
	if (ent->v.gravity)
		ent_gravity = ent->v.gravity;
	else
		ent_gravity = 1.0f;

	ent->v.velocity[2] -= (ent_gravity * sv_gravity.value * 0.5f * host_frametime);
	SV_CheckVelocity(ent);
}

trace_t SV_PushEntity(edict_t *ent, vec_t *push)
{
	vec3_t end;
	VectorAdd(push, ent->v.origin, end);

	int moveType;
	if (ent->v.movetype == MOVETYPE_FLYMISSILE)
		moveType = MOVE_MISSILE;
	else if (ent->v.solid == SOLID_TRIGGER || ent->v.solid == SOLID_NOT)
		moveType = MOVE_NOMONSTERS; // only clip against bmodels
	else
		moveType = MOVE_NORMAL;

	qboolean monsterClip = (ent->v.flags & FL_MONSTERCLIP) ? TRUE : FALSE;
	trace_t trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, moveType, ent, monsterClip);

	if (trace.fraction != 0.0f)
	{
		VectorCopy(trace.endpos, ent->v.origin);
	}

	SV_LinkEdict(ent, TRUE);

	if (trace.ent)
	{
		SV_Impact(ent, trace.ent, &trace);
	}

	return trace;
}

void SV_PushMove(edict_t *pusher, float movetime)
{
	if (VectorIsZero(pusher->v.velocity))
	{
		pusher->v.ltime = movetime + pusher->v.ltime;
		return;
	}

	vec3_t move;
	vec3_t mins, maxs;

	for (int i = 0; i < 3; i++)
	{
		move[i] = pusher->v.velocity[i] * movetime;
		mins[i] = pusher->v.absmin[i] + move[i];
		maxs[i] = pusher->v.absmax[i] + move[i];
	}

	vec3_t pushorig;
	VectorCopy(pusher->v.origin, pushorig);

	// move the pusher to it's final position
	VectorAdd(pusher->v.origin, move, pusher->v.origin);
	pusher->v.ltime += movetime;
	SV_LinkEdict(pusher, FALSE);

	if (pusher->v.solid == SOLID_NOT)
		return;

	// see if any solid entities are inside the final position
	int num_moved = 0;
	for (int i = 1; i < g_psv.num_edicts; i++)
	{
		edict_t *check = &g_psv.edicts[i];

		if (check->free)
			continue;

		if (check->v.movetype == MOVETYPE_PUSH
			|| check->v.movetype == MOVETYPE_NONE
			|| check->v.movetype == MOVETYPE_FOLLOW
			|| check->v.movetype == MOVETYPE_NOCLIP)
				continue;

		// if the entity is standing on the pusher, it will definately be moved
		if (!(check->v.flags & FL_ONGROUND) || check->v.groundentity != pusher)
		{
			if (check->v.absmin[0] >= maxs[0]
				|| check->v.absmin[1] >= maxs[1]
				|| check->v.absmin[2] >= maxs[2]
				|| check->v.absmax[0] <= mins[0]
				|| check->v.absmax[1] <= mins[1]
				|| check->v.absmax[2] <= mins[2])
					continue;

			// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition(check))
				continue;
		}

		// remove the onground flag for non-players
		if (check->v.movetype != MOVETYPE_WALK)
			check->v.flags &= ~FL_ONGROUND;

		vec3_t entorig;
		VectorCopy(check->v.origin, entorig);
		VectorCopy(check->v.origin, g_moved_from[num_moved]);
		g_moved_edict[num_moved] = check;
		num_moved++;

		// try moving the contacted entity
		pusher->v.solid = SOLID_NOT;
		SV_PushEntity(check, move);
		pusher->v.solid = SOLID_BSP;

		// if it is still inside the pusher, block
		if (SV_TestEntityPosition(check))
		{
			// fail the move
			if (check->v.mins[0] == check->v.maxs[0])
				continue;

			if (check->v.solid == SOLID_NOT || check->v.solid == SOLID_TRIGGER)
			{
				// corpse
				check->v.mins[0] = 0;
				check->v.mins[1] = 0;

				check->v.maxs[0] = 0;
				check->v.maxs[1] = 0;
				check->v.maxs[2] = check->v.mins[2];

				continue;
			}

			VectorCopy(entorig, check->v.origin);
			SV_LinkEdict(check, TRUE);

			VectorCopy(pushorig, pusher->v.origin);
			SV_LinkEdict(pusher, FALSE);

			pusher->v.ltime -= movetime;
			gEntityInterface.pfnBlocked(pusher, check);

			// move back any entities we already moved
			for (int e = 0; e < num_moved; e++)
			{
				VectorCopy(g_moved_from[e], g_moved_edict[e]->v.origin);
				SV_LinkEdict(g_moved_edict[e], FALSE);
			}

			return;
		}
	}
}

// Returns false if the pusher can't push
qboolean SV_PushRotate(edict_t *pusher, float movetime)
{
	vec3_t amove, pushorig;
	vec3_t forward, right, up;
	vec3_t forwardNow, rightNow, upNow;

	if (VectorIsZero(pusher->v.avelocity))
	{
		pusher->v.ltime += movetime;
		return TRUE;
	}

	VectorScale(pusher->v.avelocity, movetime, amove);
	AngleVectors(pusher->v.angles, forward, right, up);

	VectorCopy(pusher->v.angles, pushorig);

	// move the pusher to it's final position
	VectorAdd(pusher->v.angles, amove, pusher->v.angles);

	AngleVectorsTranspose(pusher->v.angles, forwardNow, rightNow, upNow);
	pusher->v.ltime += movetime;
	SV_LinkEdict(pusher, FALSE);

	// non-solid pushers can't push anything
	if (pusher->v.solid == SOLID_NOT)
		return TRUE;

	// see if any solid entities are inside the final position
	int num_moved = 0;
	for (int i = 1; i < g_psv.num_edicts; i++)
	{
		edict_t *check = &g_psv.edicts[i];

		if (check->free)
			continue;

		if (check->v.movetype == MOVETYPE_PUSH
			|| check->v.movetype == MOVETYPE_NONE
			|| check->v.movetype == MOVETYPE_FOLLOW
			|| check->v.movetype == MOVETYPE_NOCLIP)
				continue;

		// if the entity is standing on the pusher, it will definately be moved
		if (!(check->v.flags & FL_ONGROUND) || check->v.groundentity != pusher)
		{
			if (check->v.absmin[0] >= pusher->v.absmax[0]
				|| check->v.absmin[1] >= pusher->v.absmax[1]
				|| check->v.absmin[2] >= pusher->v.absmax[2]
				|| check->v.absmax[0] <= pusher->v.absmin[0]
				|| check->v.absmax[1] <= pusher->v.absmin[1]
				|| check->v.absmax[2] <= pusher->v.absmin[2])
					continue;

			// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition(check))
				continue;
		}

		// remove the onground flag for non-players
		if (check->v.movetype != MOVETYPE_WALK)
			check->v.flags &= ~FL_ONGROUND;

		vec3_t entorig;
		VectorCopy(check->v.origin, entorig);
		VectorCopy(check->v.origin, g_moved_from[num_moved]);
		g_moved_edict[num_moved] = check;
		num_moved++;

		if (num_moved >= g_psv.max_edicts)
			Sys_Error("%s: Out of edicts in simulator!\n", __func__);

		vec3_t start, end, push, move;

		if (check->v.movetype == MOVETYPE_PUSHSTEP)
		{
			vec3_t org;
			VectorAverage(check->v.absmax, check->v.absmin, org);
			VectorSubtract(org, pusher->v.origin, start);
		}
		else
		{
			VectorSubtract(check->v.origin, pusher->v.origin, start);
		}

		// calculate destination position
		move[0] = _DotProduct(forward, start);
		move[1] = -_DotProduct(right, start);
		move[2] = _DotProduct(up, start);

		end[0] = _DotProduct(forwardNow, move);
		end[1] = _DotProduct(rightNow, move);
		end[2] = _DotProduct(upNow, move);

		VectorSubtract(end, start, push);

		// try moving the contacted entity
		pusher->v.solid = SOLID_NOT;
		SV_PushEntity(check, push);
		pusher->v.solid = SOLID_BSP;

		if (check->v.movetype != MOVETYPE_PUSHSTEP)
		{
			if (check->v.flags & FL_CLIENT)
			{
				check->v.fixangle = 2;
				check->v.avelocity[1] += amove[1];
			}
			else
			{
				check->v.angles[1] += amove[1];
			}
		}

		// if it is still inside the pusher, block
		if (SV_TestEntityPosition(check))
		{
			// fail the move
			if (check->v.mins[0] == check->v.maxs[0])
				continue;

			if (check->v.solid == SOLID_NOT || check->v.solid == SOLID_TRIGGER)
			{
				// corpse
				check->v.mins[0] = 0.0f;
				check->v.mins[1] = 0.0f;
				check->v.maxs[0] = 0.0f;
				check->v.maxs[1] = 0.0f;

				check->v.maxs[2] = check->v.mins[2];
				continue;
			}

			VectorCopy(entorig, check->v.origin);
			SV_LinkEdict(check, TRUE);

			VectorCopy(pushorig, pusher->v.angles);
			SV_LinkEdict(pusher, FALSE);

			pusher->v.ltime -= movetime;
			gEntityInterface.pfnBlocked(pusher, check);

			// move back any entities we already moved
			for (int e = 0; e < num_moved; e++)
			{
				edict_t *movedEnt = g_moved_edict[e];
				VectorCopy(g_moved_from[e], movedEnt->v.origin);

				if (movedEnt->v.flags & FL_CLIENT)
				{
					movedEnt->v.avelocity[1] = 0.0f;
				}
				else if (movedEnt->v.movetype != MOVETYPE_PUSHSTEP)
				{
					movedEnt->v.angles[1] -= amove[1];
				}

				SV_LinkEdict(movedEnt, FALSE);
			}

			return FALSE;
		}
	}

	return TRUE;
}

void SV_Physics_Pusher(edict_t *ent)
{
	float thinktime = ent->v.nextthink;
	float oldltime = ent->v.ltime;
	float movetime;

	if (thinktime < oldltime + host_frametime)
	{
		movetime = thinktime - ent->v.ltime;

		if (movetime < 0.0f)
			movetime = 0.0f;
	}
	else
	{
		movetime = host_frametime;
	}

	if (movetime)
	{
		if (!VectorIsZero(ent->v.avelocity))
		{
			if (!VectorIsZero(ent->v.velocity))
			{
				if (SV_PushRotate(ent, movetime))
				{
					float savetime = ent->v.ltime;

					// reset the local time to what it was before we rotated
					ent->v.ltime = oldltime;
					SV_PushMove(ent, movetime);

					if (ent->v.ltime < savetime)
						ent->v.ltime = savetime;
				}
			}
			else
			{
				SV_PushRotate(ent, movetime);
			}
		}
		else
		{
			SV_PushMove(ent, movetime);
		}
	}

	for (int i = 0; i < 3; i++)
	{
		if (ent->v.angles[i] < -3600.0f || ent->v.angles[i] > 3600.0f)
			ent->v.angles[i] = fmod(ent->v.angles[i], 3600.0f);
	}

	if (thinktime > oldltime && ((ent->v.flags & FL_ALWAYSTHINK) || thinktime <= ent->v.ltime)
#ifdef REHLDS_FIXES
	 && !(ent->v.flags & FL_KILLME)
#endif // REHLDS_FIXES
	)
	{
		ent->v.nextthink = 0;
		gGlobalVariables.time = g_psv.time;
		gEntityInterface.pfnThink(ent);
	}
}

qboolean SV_CheckWater(edict_t *ent)
{
	vec3_t point;
	point[0] = (ent->v.absmax[0] + ent->v.absmin[0]) * 0.5f;
	point[1] = (ent->v.absmax[1] + ent->v.absmin[1]) * 0.5f;
	point[2] = (ent->v.absmin[2] + 1.0f);

	ent->v.watertype = CONTENTS_EMPTY;
	ent->v.waterlevel = 0;

	g_groupmask = ent->v.groupinfo;

	int cont = SV_PointContents(point);
	if (cont <= CONTENTS_WATER && cont > CONTENTS_TRANSLUCENT)
	{
		ent->v.watertype = cont;
		ent->v.waterlevel = 1;

		if (ent->v.absmin[2] == ent->v.absmax[2])
		{
			ent->v.waterlevel = 3;
		}
		else
		{
			g_groupmask = ent->v.groupinfo;
			point[2] = (ent->v.absmax[2] + ent->v.absmin[2]) * 0.5f;

			int truecont = SV_PointContents(point);
			if (truecont <= CONTENTS_WATER && truecont > CONTENTS_TRANSLUCENT)
			{
				ent->v.waterlevel = 2;
				g_groupmask = ent->v.groupinfo;
				point[0] = point[0] + ent->v.view_ofs[0];
				point[1] = point[1] + ent->v.view_ofs[1];
				point[2] = point[2] + ent->v.view_ofs[2];
				truecont = SV_PointContents(point);
				if (truecont <= CONTENTS_WATER && truecont > CONTENTS_TRANSLUCENT)
					ent->v.waterlevel = 3;
			}
		}

		if (cont <= CONTENTS_CURRENT_0 && cont >= CONTENTS_CURRENT_DOWN)
		{
			static vec3_t current_table[] =
			{
				{ 1.0f,  0.0f,  0.0f },
				{ 0.0f,  1.0f,  0.0f },
				{ -1.0f, 0.0f,  0.0f },
				{ 0.0f,  -1.0f, 0.0f },
				{ 0.0f,  0.0f,  1.0f },
				{ 0.0f,  0.0f, -1.0f },
			};

			VectorMA(ent->v.basevelocity, 150.0f * ent->v.waterlevel / 3.0f, current_table[-(cont - CONTENTS_CURRENT_0)], ent->v.basevelocity);
		}
	}

	return (ent->v.waterlevel > 1) ? TRUE : FALSE;
}

float SV_RecursiveWaterLevel(const vec_t *origin, float mins, float maxs, int depth)
{
	float waterlevel = ((mins - maxs) * 0.5f + maxs);
	if (++depth > 5)
		return waterlevel;

	vec3_t test;
	test[0] = origin[0];
	test[1] = origin[1];
	test[2] = origin[2] + waterlevel;

	if (SV_PointContents(test) == CONTENTS_WATER)
		return SV_RecursiveWaterLevel(origin, mins, waterlevel, depth);

	return SV_RecursiveWaterLevel(origin, waterlevel, maxs, depth);
}

float SV_Submerged(edict_t *ent)
{
	vec3_t center;
	VectorAverage(ent->v.absmax, ent->v.absmin, center);

	float waterlevel = ent->v.absmin[2] - center[2];

	switch (ent->v.waterlevel)
	{
	case 1:
		return SV_RecursiveWaterLevel(center, 0.0f, waterlevel, 0) - waterlevel;
	case 2:
		return SV_RecursiveWaterLevel(center, ent->v.absmax[2] - center[2], 0.0f, 0) - waterlevel;
	case 3:
	{
		vec3_t point;
		point[0] = center[0];
		point[1] = center[1];
		point[2] = ent->v.absmax[2];

		g_groupmask = ent->v.groupinfo;

		if (SV_PointContents(point) == CONTENTS_WATER)
		{
			return ent->v.maxs[2] - ent->v.mins[2];
		}

		return SV_RecursiveWaterLevel(center, ent->v.absmax[2] - center[2], 0.0f, 0) - waterlevel;
	}
	default:
		return 0.0f;
	}
}

// Non moving objects can only think.
void SV_Physics_None(edict_t *ent)
{
	// regular thinking
	SV_RunThink(ent);
}

// Just copy angles and origin of parent
void SV_Physics_Follow(edict_t *ent)
{
	// regular thinking
	if (!SV_RunThink(ent))
		return;

	edict_t	*parent = ent->v.aiment;
	if (!ent->v.aiment)
	{
		Con_DPrintf("%s movetype FOLLOW with NULL aiment\n", &pr_strings[ent->v.classname]);
		ent->v.movetype = MOVETYPE_NONE;
		return;
	}

	VectorAdd(parent->v.origin, ent->v.v_angle, ent->v.origin);
	VectorCopy(parent->v.angles, ent->v.angles);

	SV_LinkEdict(ent, TRUE);
}

// A moving object that doesn't obey physics
void SV_Physics_Noclip(edict_t *ent)
{
	// regular thinking
	if (!SV_RunThink(ent))
		return;

	VectorMA(ent->v.origin, host_frametime, ent->v.velocity, ent->v.origin);
	VectorMA(ent->v.angles, host_frametime, ent->v.avelocity, ent->v.angles);

	// noclip ents never touch triggers
	SV_LinkEdict(ent, FALSE);
}

void SV_CheckWaterTransition(edict_t *ent)
{
	vec3_t point;

	point[0] = (ent->v.absmax[0] + ent->v.absmin[0]) * 0.5f;
	point[1] = (ent->v.absmax[1] + ent->v.absmin[1]) * 0.5f;
	point[2] = (ent->v.absmin[2] + 1.0f);

	g_groupmask = ent->v.groupinfo;

	int cont = SV_PointContents(point);
	if (ent->v.watertype == 0)
	{
		// just spawned here
		ent->v.watertype = cont;
		ent->v.waterlevel = 1;
		return;
	}

	if (cont > CONTENTS_WATER || cont <= CONTENTS_TRANSLUCENT)
	{
		if (ent->v.watertype != CONTENTS_EMPTY)
		{
			// just crossed into water
			SV_StartSound(0, ent, CHAN_AUTO, "player/pl_wade2.wav", 255, 1.0f, 0, PITCH_NORM);
		}

		ent->v.watertype = CONTENTS_EMPTY;
		ent->v.waterlevel = 0;
		return;
	}

	if (ent->v.watertype == CONTENTS_EMPTY)
	{
		// just crossed into water
		SV_StartSound(0, ent, CHAN_AUTO, "player/pl_wade1.wav", 255, 1.0f, 0, PITCH_NORM);
		ent->v.velocity[2] *= 0.5f;
	}

	ent->v.watertype = cont;
	ent->v.waterlevel = 1;

	if (ent->v.absmin[2] == ent->v.absmax[2])
	{
		// a point entity
		ent->v.waterlevel = 3;
		return;
	}

	point[2] = (ent->v.absmin[2] + ent->v.absmax[2]) * 0.5f;

	g_groupmask = ent->v.groupinfo;
	cont = SV_PointContents(point);

	if (cont <= CONTENTS_WATER && cont > CONTENTS_TRANSLUCENT)
	{
		ent->v.waterlevel = 2;
		VectorAdd(point, ent->v.view_ofs, point);

		g_groupmask = ent->v.groupinfo;
		cont = SV_PointContents(point);

		if (cont <= CONTENTS_WATER && cont > CONTENTS_TRANSLUCENT)
		{
			ent->v.waterlevel = 3;
		}
	}
}

// Toss movement. When onground, do nothing.
void SV_Physics_Toss(edict_t *ent)
{
	SV_CheckWater(ent);

	// regular thinking
	if (!SV_RunThink(ent))
		return;

	if (ent->v.velocity[2] > 0.0f || !ent->v.groundentity || (ent->v.groundentity->v.flags & (FL_MONSTER | FL_CLIENT))) {
		ent->v.flags &= ~FL_ONGROUND;
	}

	// if on ground and not moving, return.
	if ((ent->v.flags & FL_ONGROUND) && VectorIsZero(ent->v.velocity))
	{
		VectorClear(ent->v.avelocity);

		if (VectorIsZero(ent->v.basevelocity))
			return; // at rest
	}

	SV_CheckVelocity(ent);

	// add gravity
	SV_AddGravity(ent);

	// move angles
	VectorMA(ent->v.angles, host_frametime, ent->v.avelocity, ent->v.angles);

	// move origin
	VectorAdd(ent->v.velocity, ent->v.basevelocity, ent->v.velocity);
	SV_CheckVelocity(ent);

	SV_FlyMove(ent, host_frametime, 1.1f);
	SV_CheckVelocity(ent);

	VectorSubtract(ent->v.velocity, ent->v.basevelocity, ent->v.velocity);
	SV_CheckVelocity(ent);

	// determine if it's on solid ground at all
	{
		int x, y;
		vec3_t point, mins, maxs;

		VectorAdd(ent->v.origin, ent->v.mins, mins);
		VectorAdd(ent->v.origin, ent->v.maxs, maxs);

		// if all of the points under the corners are solid world, don't bother
		// with the tougher checks
		// the corners must be within 16 of the midpoint
		point[2] = mins[2] - 1.0f;

		for (x = 0; x <= 1; x++)
		{
			for (y = 0; y <= 1; y++)
			{
				point[0] = x ? maxs[0] : mins[0];
				point[1] = y ? maxs[1] : mins[1];

				g_groupmask = ent->v.groupinfo;

				if (SV_PointContents(point) == CONTENTS_SOLID)
				{
					ent->v.flags |= FL_ONGROUND;
					break;
				}
			}
		}
	}

	SV_LinkEdict(ent, TRUE);

	// check for in water
	SV_CheckWaterTransition(ent);
}

// Bounce and fly movement. When onground, do nothing.
void SV_Physics_Bounce(edict_t *ent)
{
	SV_CheckWater(ent);

	// regular thinking
	if (!SV_RunThink(ent))
		return;

	if (ent->v.velocity[2] > 0.0f || !ent->v.groundentity || (ent->v.groundentity->v.flags & (FL_MONSTER | FL_CLIENT))) {
		ent->v.flags &= ~FL_ONGROUND;
	}

	// if on ground and not moving, return.
	if ((ent->v.flags & FL_ONGROUND) && VectorIsZero(ent->v.velocity))
	{
		VectorClear(ent->v.avelocity);

		if (VectorIsZero(ent->v.basevelocity))
			return; // at rest
	}

	SV_CheckVelocity(ent);

	// add gravity
	switch (ent->v.movetype)
	{
	case MOVETYPE_FLY:
	case MOVETYPE_FLYMISSILE:
	case MOVETYPE_BOUNCEMISSILE:
		break;
	default:
		SV_AddGravity(ent);
		break;
	}

	// move angles
	VectorMA(ent->v.angles, host_frametime, ent->v.avelocity, ent->v.angles);

	// move origin
	VectorAdd(ent->v.velocity, ent->v.basevelocity, ent->v.velocity);
	SV_CheckVelocity(ent);

	vec3_t move;
	VectorScale(ent->v.velocity, host_frametime, move);
	VectorSubtract(ent->v.velocity, ent->v.basevelocity, ent->v.velocity);

	trace_t trace = SV_PushEntity(ent, move);
	SV_CheckVelocity(ent);

	// If we started in a solid object, or we were in solid space the whole way, zero out our velocity.
	if (trace.allsolid)
	{
		// entity is trapped in another solid
		VectorClear(ent->v.avelocity);
		VectorClear(ent->v.velocity);
		return;
	}

	// If we covered the entire distance, we are done and can return
	if (trace.fraction == 1.0f)
	{
		// moved the entire distance
		SV_CheckWaterTransition(ent);
		return;
	}

	if (ent->free)
		return;

	float backoff;
	if (ent->v.movetype == MOVETYPE_BOUNCE)
		backoff = 2.0f - ent->v.friction;
	else if (ent->v.movetype == MOVETYPE_BOUNCEMISSILE)
		backoff = 2.0f;
	else
		backoff = 1.0f;

	SV_ClipVelocity(ent->v.velocity, trace.plane.normal, ent->v.velocity, backoff);

	// stop if on ground
	if (trace.plane.normal[2] > 0.7f)
	{
		VectorAdd(ent->v.velocity, ent->v.basevelocity, move);
		float vel = _DotProduct(move, move);

		if (move[2] < sv_gravity.value * host_frametime)
		{
			// we're rolling on the ground, add static friction
			ent->v.flags |= FL_ONGROUND;
			ent->v.velocity[2]= 0;
			ent->v.groundentity = trace.ent;
		}

		if (vel < 900.0f || (ent->v.movetype != MOVETYPE_BOUNCE && ent->v.movetype != MOVETYPE_BOUNCEMISSILE))
		{
			ent->v.flags |= FL_ONGROUND;
			ent->v.groundentity = trace.ent;

			VectorClear(ent->v.velocity);
			VectorClear(ent->v.avelocity);
		}
		else
		{
			float scale = (1.0f - trace.fraction) * host_frametime * 0.9f;

			VectorScale(ent->v.velocity, scale, move);
			VectorMA(move, scale, ent->v.basevelocity, move);

			trace = SV_PushEntity(ent, move);

#ifdef REHLDS_FIXES
			if (ent->free)
				return;
#endif
		}
	}

	// check for in water
	SV_CheckWaterTransition(ent);
}

void PF_WaterMove(edict_t *pSelf)
{
	if (pSelf->v.movetype == MOVETYPE_NOCLIP)
	{
		pSelf->v.air_finished = g_psv.time + 12.0f;
		return;
	}

	if (pSelf->v.health < 0.0f)
		return;

	float drownlevel = (pSelf->v.deadflag == DEAD_NO) ? 3.0f : 1.0f;
	int waterlevel = pSelf->v.waterlevel;
	int watertype = pSelf->v.watertype;
	int flags = pSelf->v.flags;

	if (!(flags & (FL_IMMUNE_WATER | FL_GODMODE)))
	{
		if (((flags & FL_SWIM) && waterlevel < drownlevel) || (waterlevel >= drownlevel))
		{
			if (pSelf->v.air_finished < g_psv.time && pSelf->v.pain_finished < g_psv.time)
			{
				pSelf->v.dmg += 2.0f;

				if (pSelf->v.dmg > 15.0f)
					pSelf->v.dmg = 10.0f;

				pSelf->v.pain_finished = g_psv.time + 1.0f;
			}
		}
		else
		{
			pSelf->v.dmg = 2.0f;
			pSelf->v.air_finished = g_psv.time + 12.0f;
		}
	}

	if (!waterlevel)
	{
		// play leave water sound
		if (flags & FL_INWATER)
		{
			switch (RandomLong(0, 3))
			{
			case 0:
				SV_StartSound(0, pSelf, CHAN_BODY, "player/pl_wade1.wav", 255, ATTN_NORM, 0, PITCH_NORM);
				break;
			case 1:
				SV_StartSound(0, pSelf, CHAN_BODY, "player/pl_wade2.wav", 255, ATTN_NORM, 0, PITCH_NORM);
				break;
			case 2:
				SV_StartSound(0, pSelf, CHAN_BODY, "player/pl_wade3.wav", 255, ATTN_NORM, 0, PITCH_NORM);
				break;
			case 3:
				SV_StartSound(0, pSelf, CHAN_BODY, "player/pl_wade4.wav", 255, ATTN_NORM, 0, PITCH_NORM);
				break;
			default:
				break;
			}

			pSelf->v.flags &= ~FL_INWATER;
		}

		pSelf->v.air_finished = g_psv.time + 12.0f;
		return;
	}

	if (watertype == CONTENTS_LAVA)
	{
		if (!(flags & (FL_IMMUNE_LAVA | FL_GODMODE)) && pSelf->v.dmgtime < g_psv.time)
		{
			if (g_psv.time <= pSelf->v.radsuit_finished)
				pSelf->v.dmgtime = g_psv.time + 1.0f;
			else
				pSelf->v.dmgtime = g_psv.time + 0.2f;
		}
	}
	else if (watertype == CONTENTS_SLIME)
	{
		if (!(flags & (FL_IMMUNE_SLIME | FL_GODMODE)) && pSelf->v.dmgtime < g_psv.time)
		{
			if (pSelf->v.radsuit_finished < g_psv.time)
				pSelf->v.dmgtime = g_psv.time + 1.0f;
		}
	}

	if (!(flags & FL_INWATER))
	{
		// player enter water sound
		if (watertype == CONTENTS_WATER)
		{
			switch (RandomLong(0, 3))
			{
			case 0:
				SV_StartSound(0, pSelf, CHAN_BODY, "player/pl_wade1.wav", 255, ATTN_NORM, 0, PITCH_NORM);
				break;
			case 1:
				SV_StartSound(0, pSelf, CHAN_BODY, "player/pl_wade2.wav", 255, ATTN_NORM, 0, PITCH_NORM);
				break;
			case 2:
				SV_StartSound(0, pSelf, CHAN_BODY, "player/pl_wade3.wav", 255, ATTN_NORM, 0, PITCH_NORM);
				break;
			case 3:
				SV_StartSound(0, pSelf, CHAN_BODY, "player/pl_wade4.wav", 255, ATTN_NORM, 0, PITCH_NORM);
				break;
			default:
				break;
			}
		}

		pSelf->v.dmgtime = 0.0f;
		pSelf->v.flags |= FL_INWATER;
	}

	if (!(flags & FL_WATERJUMP))
	{
		VectorMA(pSelf->v.velocity, (-0.8 * pSelf->v.waterlevel * host_frametime), pSelf->v.velocity, pSelf->v.velocity);
	}
}

// Monsters freefall when they don't have a ground entity, otherwise all movement is done with discrete steps.
// This is also used for objects that have become still on the ground,
// but will fall if the floor is pulled out from under them.
void SV_Physics_Step(edict_t *ent)
{
	PF_WaterMove(ent);
	SV_CheckVelocity(ent);

	qboolean wasonground = (ent->v.flags & FL_ONGROUND) ? TRUE : FALSE;
	qboolean inwater = SV_CheckWater(ent);

	if ((ent->v.flags & FL_FLOAT) && ent->v.waterlevel > 0)
	{
		float buoyancy = SV_Submerged(ent) * ent->v.skin * host_frametime;

		SV_AddGravity(ent);
		ent->v.velocity[2] += buoyancy;
	}

	// add gravity except:
	//  flying monsters
	//  swimming monsters who are in the water
	if (!wasonground && !(ent->v.flags & FL_FLY) && (!(ent->v.flags & FL_SWIM) || ent->v.waterlevel <= 0))
	{
		if (!inwater)
			SV_AddGravity(ent);
	}

	if (!VectorIsZero(ent->v.velocity) || !VectorIsZero(ent->v.basevelocity))
	{
		ent->v.flags &= ~FL_ONGROUND;

		// apply friction
		// let dead monsters who aren't completely onground slide
		if (wasonground && (ent->v.health > 0.0f || SV_CheckBottom(ent)))
		{
			float speed = Q_sqrt((double)(ent->v.velocity[0] * ent->v.velocity[0] + ent->v.velocity[1] * ent->v.velocity[1]));
			if (speed)
			{
				float friction = sv_friction.value * ent->v.friction;
				ent->v.friction = 1.0f;

				float control = (speed < sv_stopspeed.value) ? sv_stopspeed.value : speed;
				float newspeed = speed - (host_frametime * control * friction);
				if (newspeed < 0.0f)
					newspeed = 0.0f;

				newspeed = newspeed / speed;

				ent->v.velocity[0] *= newspeed;
				ent->v.velocity[1] *= newspeed;
			}
		}

		VectorAdd(ent->v.velocity, ent->v.basevelocity, ent->v.velocity);
		SV_CheckVelocity(ent);

		SV_FlyMove(ent, host_frametime, 1.0f);
		SV_CheckVelocity(ent);

		VectorSubtract(ent->v.velocity, ent->v.basevelocity, ent->v.velocity);
		SV_CheckVelocity(ent);

		// determine if it's on solid ground at all
		{
			int x, y;
			vec3_t point, mins, maxs;

			VectorAdd(ent->v.origin, ent->v.mins, mins);
			VectorAdd(ent->v.origin, ent->v.maxs, maxs);

			point[2] = mins[2] - 1.0f;

			for (x = 0; x <= 1; x++)
			{
				for (y = 0; y <= 1; y++)
				{
					point[0] = x ? maxs[0] : mins[0];
					point[1] = y ? maxs[1] : mins[1];

					g_groupmask = ent->v.groupinfo;

					if (SV_PointContents(point) == CONTENTS_SOLID)
					{
						ent->v.flags |= FL_ONGROUND;
						break;
					}
				}
			}
		}

		SV_LinkEdict(ent, TRUE);
	}
	else
	{
		if (gGlobalVariables.force_retouch != 0.0f)
		{
			trace_t trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, ent->v.origin, MOVE_NORMAL, ent, (ent->v.flags & FL_MONSTERCLIP) ? TRUE : FALSE);

			// hentacle impact code
			if ((trace.fraction < 1.0f || trace.startsolid) && trace.ent)
			{
				SV_Impact(ent, trace.ent, &trace);
			}
		}
	}

	// regular thinking
	SV_RunThink(ent);
	SV_CheckWaterTransition(ent);
}

void SV_Physics()
{
	// let the progs know that a new frame has started
	gGlobalVariables.time = g_psv.time;
	gEntityInterface.pfnStartFrame();

	// treat each object in turn
	for (int i = 0; i < g_psv.num_edicts; i++)
	{
		edict_t *ent = &g_psv.edicts[i];
		if (ent->free)
			continue;

		if (gGlobalVariables.force_retouch != 0.0f)
		{
			// force retouch even for stationary
			SV_LinkEdict(ent, TRUE);
		}

		if (i > 0 && i <= g_psvs.maxclients)
			continue;

		SV_CheckMovingGround(ent, host_frametime);

		switch (ent->v.movetype)
		{
		case MOVETYPE_NONE:
			SV_Physics_None(ent);
			break;
		case MOVETYPE_PUSH:
			SV_Physics_Pusher(ent);
			break;
		case MOVETYPE_FOLLOW:
			SV_Physics_Follow(ent);
			break;
		case MOVETYPE_NOCLIP:
			SV_Physics_Noclip(ent);
			break;
		case MOVETYPE_STEP:
		case MOVETYPE_PUSHSTEP:
			SV_Physics_Step(ent);
			break;
		case MOVETYPE_TOSS:
#ifdef REHLDS_FIXES
			SV_Physics_Toss(ent);
			break;
#endif
		case MOVETYPE_BOUNCE:
		case MOVETYPE_BOUNCEMISSILE:
		case MOVETYPE_FLY:
		case MOVETYPE_FLYMISSILE:
			SV_Physics_Bounce(ent);
			break;
		default:
			Sys_Error("%s: %s bad movetype %d", __func__, &pr_strings[ent->v.classname], ent->v.movetype);
		}

		if (ent->v.flags & FL_KILLME)
			ED_Free(ent);
	}

	if (gGlobalVariables.force_retouch != 0.0f)
		gGlobalVariables.force_retouch = gGlobalVariables.force_retouch - 1.0f;
}

trace_t SV_Trace_Toss(edict_t *ent, edict_t *ignore)
{
	edict_t *tent, tempent;
	trace_t trace;
	vec3_t move;
	vec3_t end;
	double save_frametime;

	save_frametime = host_frametime;
	host_frametime = 5.0f;

	Q_memcpy(&tempent, ent, sizeof(tempent));
	tent = &tempent;

	do
	{
		SV_CheckVelocity(tent);
		SV_AddGravity(tent);
		VectorMA(tent->v.angles, host_frametime, tent->v.avelocity, tent->v.angles);
		VectorScale(tent->v.velocity, host_frametime, move);
		VectorAdd(tent->v.origin, move, end);
		trace = SV_Move(tent->v.origin, tent->v.mins, tent->v.maxs, end, MOVE_NORMAL, tent, FALSE);
		VectorCopy(trace.endpos, tent->v.origin);
	}
	while (!trace.ent || trace.ent == ignore);

	host_frametime = save_frametime;
	return trace;
}
