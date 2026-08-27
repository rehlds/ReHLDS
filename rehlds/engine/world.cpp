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

hull_t box_hull;
hull_t beam_hull;

box_clipnodes_t box_clipnodes;
box_planes_t box_planes;
beam_planes_t beam_planes;
areanode_t sv_areanodes[32];
int sv_numareanodes;

#ifdef REHLDS_FIXES
static link_t *touchLinksNext = nullptr;
#endif // REHLDS_FIXES

cvar_t sv_force_ent_intersection = { "sv_force_ent_intersection", "0", 0, 0.0f, NULL };

// ClearLink is used for new headnodes
void ClearLink(link_t *l)
{
	l->next = l->prev = l;
}

// Remove link from chain
void RemoveLink(link_t *l)
{
#ifdef REHLDS_FIXES
	if (touchLinksNext == l)
	{
		touchLinksNext = l->next;
	}
#endif // REHLDS_FIXES

	l->next->prev = l->prev;
	l->prev->next = l->next;
}

// Kept trigger and solid entities seperate
void InsertLinkBefore(link_t *l, link_t *before)
{
	l->next = before;
	l->prev = before->prev;
	l->next->prev = l;
	l->prev->next = l;

#ifdef REHLDS_FIXES
	if (touchLinksNext == before)
	{
		touchLinksNext = l;
	}
#endif // REHLDS_FIXES
}

NOXREF void InsertLinkAfter(link_t *l, link_t *after)
{
	NOXREFCHECK;

	l->prev = after;
	l->next = after->next;

	after->next = l;
	l->next->prev = l;
}

// Set up the planes and clipnodes so that the six floats of a bounding box
// can just be stored out and get a proper hull_t structure.
void SV_InitBoxHull()
{
	box_hull.clipnodes = box_clipnodes;
	box_hull.planes = box_planes;
	box_hull.firstclipnode = 0;
	box_hull.lastclipnode = 5;

	Q_memcpy(&beam_hull, &box_hull, sizeof(beam_hull));
	beam_hull.planes = beam_planes;

	for (int i = 0; i < 6; i++)
	{
		int side = i & 1;
		box_clipnodes[i].planenum = i;
		box_clipnodes[i].children[side] = CONTENTS_EMPTY;
		box_clipnodes[i].children[side ^ 1] = (i != 5) ? i + 1 : CONTENTS_SOLID;

		box_planes[i].type = i >> 1;
		box_planes[i].normal[i >> 1] = 1.0f;
		beam_planes[i].type = 5;
	}
}

// To keep everything totally uniform, bounding boxes are turned into small
// BSP trees instead of being compared directly.
hull_t *SV_HullForBox(const vec_t *mins, const vec_t *maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = mins[0];
	box_planes[2].dist = maxs[1];
	box_planes[3].dist = mins[1];
	box_planes[4].dist = maxs[2];
	box_planes[5].dist = mins[2];

	return &box_hull;
}

NOXREF hull_t *SV_HullForBeam(const vec_t *start, const vec_t *end, const vec_t *size)
{
	NOXREFCHECK;
	vec3_t tmp = { 0, 0, 0 };

	VectorSubtract(end, start, beam_planes[0].normal);
	VectorNormalize(beam_planes[0].normal);
	VectorCopy(beam_planes[0].normal, beam_planes[1].normal);

	beam_planes[0].dist = _DotProduct(end, beam_planes[0].normal);
	beam_planes[1].dist = _DotProduct(start, beam_planes[0].normal);

	if (fabs(beam_planes[0].normal[2]) < 0.9f)
		tmp[2] = 1.0f;
	else
		tmp[0] = 1.0f;

	CrossProduct(beam_planes[0].normal, tmp, beam_planes[2].normal);
	VectorNormalize(beam_planes[2].normal);
	VectorCopy(beam_planes[2].normal, beam_planes[3].normal);

	beam_planes[2].dist = (start[0] + beam_planes[2].normal[0]) * beam_planes[2].normal[0] + (start[1] + beam_planes[2].normal[1]) * beam_planes[2].normal[1] + (start[2] + beam_planes[2].normal[2]) * beam_planes[2].normal[2];

	VectorSubtract(start, beam_planes[2].normal, tmp);

	beam_planes[3].dist = _DotProduct(tmp, beam_planes[2].normal);
	CrossProduct(beam_planes[2].normal, beam_planes[0].normal, beam_planes[4].normal);
	VectorNormalize(beam_planes[4].normal);

	VectorCopy(beam_planes[4].normal, beam_planes[5].normal);

	beam_planes[4].dist = _DotProduct(start, beam_planes[4].normal);
	beam_planes[5].dist = (start[0] - beam_planes[4].normal[0]) * beam_planes[4].normal[0] + (start[1] - beam_planes[4].normal[1]) * beam_planes[4].normal[1] + (start[2] - beam_planes[4].normal[2]) * beam_planes[4].normal[2];

	beam_planes[0].dist += fabs(beam_planes[0].normal[0] * size[0]) + fabs(beam_planes[0].normal[1] * size[1]) + fabs(beam_planes[0].normal[2] * size[2]);
	beam_planes[1].dist -= fabs(beam_planes[1].normal[0] * size[0]) + fabs(beam_planes[1].normal[1] * size[1]) + fabs(beam_planes[1].normal[2] * size[2]);
	beam_planes[2].dist += fabs(beam_planes[2].normal[0] * size[0]) + fabs(beam_planes[2].normal[1] * size[1]) + fabs(beam_planes[2].normal[2] * size[2]);
	beam_planes[3].dist -= fabs(beam_planes[3].normal[0] * size[0]) + fabs(beam_planes[3].normal[1] * size[1]) + fabs(beam_planes[3].normal[2] * size[2]);
	beam_planes[4].dist += fabs(beam_planes[4].normal[0] * size[0]) + fabs(beam_planes[4].normal[1] * size[1]) + fabs(beam_planes[4].normal[2] * size[2]);
	beam_planes[5].dist -= fabs(beam_planes[4].normal[0] * size[0]) + fabs(beam_planes[4].normal[1] * size[1]) + fabs(beam_planes[4].normal[2] * size[2]);

	return &beam_hull;
}

// Forcing to select BSP hull
hull_t *SV_HullForBsp(edict_t *ent, const vec_t *mins, const vec_t *maxs, vec_t *offset)
{
	hull_t *hull;
	model_t *model;
	vec3_t size;

	model = Mod_Handle(ent->v.modelindex);
	if (!model || model->type != mod_brush)
	{
		Sys_Error("%s: Hit a %s with no model (%s)", __func__, &pr_strings[ent->v.classname], &pr_strings[ent->v.model]);
	}

	VectorSubtract(maxs, mins, size);

	if (size[0] <= 8.0f)
	{
		hull = &model->hulls[point_hull];
		VectorCopy(hull->clip_mins, offset);
	}
	else
	{
		if (size[0] <= 36.0f)
		{
#ifdef REHLDS_FIXES
			if (size[2] <= (player_maxs[1][2] - player_mins[1][2]))
#else
			if (size[2] <= 36.0f)
#endif
				hull = &model->hulls[head_hull];
			else
				hull = &model->hulls[human_hull];
		}
		else
		{
			hull = &model->hulls[large_hull];
		}

		// calculate an offset value to center the origin
		VectorSubtract(hull->clip_mins, mins, offset);

#ifdef REHLDS_FIXES
		if (sv_rehlds_hull_centering.value && (mins[0] + maxs[0]) == 0.0f)
			offset[0] = 0.0f;

		if (sv_rehlds_hull_centering.value && (mins[1] + maxs[1]) == 0.0f)
			offset[1] = 0.0f;
#endif
	}

	VectorAdd(offset, ent->v.origin, offset);
	return hull;
}

hull_t *SV_HullForEntity(edict_t *ent, const vec_t *mins, const vec_t *maxs, vec_t *offset)
{
	vec3_t hullmins;
	vec3_t hullmaxs;

	// decide which clipping hull to use, based on the size
	if (ent->v.solid == SOLID_BSP)
	{
		// explicit hulls in the BSP model
		if (ent->v.movetype != MOVETYPE_PUSH && ent->v.movetype != MOVETYPE_PUSHSTEP)
		{
			Sys_Error("%s: SOLID_BSP without MOVETYPE_PUSH\nEntity classname = %s, model = %s",
				__func__, STRING(ent->v.classname), STRING(ent->v.model));
		}

		return SV_HullForBsp(ent, mins, maxs, offset);
	}

	// create a temp hull from bounding box sizes
	VectorSubtract(ent->v.mins, maxs, hullmins);
	VectorSubtract(ent->v.maxs, mins, hullmaxs);
	VectorCopy(ent->v.origin, offset);

	return SV_HullForBox(hullmins, hullmaxs);
}

// Builds a uniformly subdivided tree for the given world size
areanode_t *SV_CreateAreaNode(int depth, vec_t *mins, vec_t *maxs)
{
	areanode_t *anode;
	vec3_t size;
	vec3_t mins1, maxs2, maxs1, mins2;

	anode = &sv_areanodes[sv_numareanodes++];

	ClearLink(&anode->trigger_edicts);
	ClearLink(&anode->solid_edicts);

	if (depth == AREA_DEPTH)
	{
		anode->axis = -1;
		anode->children[0] = anode->children[1] = nullptr;
		return anode;
	}

	VectorSubtract(maxs, mins, size);

	if (size[0] > size[1])
		anode->axis = 0;
	else
		anode->axis = 1;

	anode->dist = 0.5f * (maxs[anode->axis] + mins[anode->axis]);

	VectorCopy(mins, mins1);
	VectorCopy(mins, mins2);
	VectorCopy(maxs, maxs1);
	VectorCopy(maxs, maxs2);

	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;

	anode->children[0] = SV_CreateAreaNode(depth + 1, mins2, maxs2);
	anode->children[1] = SV_CreateAreaNode(depth + 1, mins1, maxs1);

	return anode;
}

// called after the world model has been loaded, before linking any entities
void SV_ClearWorld()
{
	SV_InitBoxHull();

	Q_memset(sv_areanodes, 0, sizeof(sv_areanodes));
	sv_numareanodes = 0;
	SV_CreateAreaNode(0, g_psv.worldmodel->mins, g_psv.worldmodel->maxs);
}

// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself
// flags ent->v.modified
void SV_UnlinkEdict(edict_t *ent)
{
	if (!ent->area.prev)
	{
		// not linked in anywhere
		return;
	}

	RemoveLink(&ent->area);
	ent->area.prev = ent->area.next = nullptr;
}

void SV_TouchLinks(edict_t *ent, areanode_t *node)
{
	vec3_t localPosition, offset;
	edict_t *touch;

#ifndef REHLDS_FIXES
	link_t *touchLinksNext;
#endif // REHLDS_FIXES

	// touch linked edicts
	for (link_t *l = node->trigger_edicts.next; l != &node->trigger_edicts; l = touchLinksNext)
	{
		touchLinksNext = l->next;
		touch = EDICT_FROM_AREA(l);

		if (touch == ent)
			continue;

		if (ent->v.groupinfo && touch->v.groupinfo)
		{
			if (g_groupop)
			{
				if (g_groupop == GROUP_OP_NAND && (ent->v.groupinfo & touch->v.groupinfo))
					continue;
			}
			else
			{
				if (!(ent->v.groupinfo & touch->v.groupinfo))
					continue;
			}
		}

#ifdef REHLDS_FIXES
		if ((ent->v.flags & FL_KILLME) || (touch->v.flags & FL_KILLME))
			continue;
#endif // REHLDS_FIXES

		if (touch->v.solid != SOLID_TRIGGER)
			continue;

		if (!BoundsIntersect(ent->v.absmin, ent->v.absmax, touch->v.absmin, touch->v.absmax))
			continue;

		// check brush triggers accuracy
		if (Mod_GetType(touch->v.modelindex) == mod_brush)
		{
			// force to select bsp-hull
			hull_t *hull = SV_HullForBsp(touch, ent->v.mins, ent->v.maxs, offset);

			// offset the test point appropriately for this hull
			VectorSubtract(ent->v.origin, offset, localPosition);

			// test hull for intersection with this model
			if (SV_HullPointContents(hull, hull->firstclipnode, localPosition) != CONTENTS_SOLID)
				continue;
		}

		gGlobalVariables.time = g_psv.time;
		gEntityInterface.pfnTouch(touch, ent);
	}

#ifdef REHLDS_FIXES
	touchLinksNext = nullptr;
#endif // REHLDS_FIXES

	// recurse down both sides
	if(node->axis == -1)
		return;

	if (ent->v.absmax[node->axis] > node->dist)
		SV_TouchLinks(ent, node->children[0]);

	if (node->dist > ent->v.absmin[node->axis])
		SV_TouchLinks(ent, node->children[1]);
}

#ifndef REHLDS_OPT_PEDANTIC

void SV_FindTouchedLeafs(edict_t *ent, mnode_t *node, int *topnode)
{
	if (node->contents == CONTENTS_SOLID)
		return;

	// add an efrag if the node is a leaf
	if (node->contents < 0)
	{
		if (ent->num_leafs > (MAX_ENT_LEAFS - 1))
		{
			// continue counting leafs,
			// so we know how many it's overrun
			ent->num_leafs = (MAX_ENT_LEAFS + 1);
		}
		else
		{
			mleaf_t *leaf = (mleaf_t *)node;
			int leafnum = leaf - g_psv.worldmodel->leafs - 1;
			ent->leafnums[ent->num_leafs] = leafnum;
			ent->num_leafs++;
		}
		return;
	}

	// NODE_MIXED
	mplane_t *splitplane = node->plane;
	int sides = BOX_ON_PLANE_SIDE(ent->v.absmin, ent->v.absmax, splitplane);

	if (sides == 3 && *topnode == -1)
	{
		*topnode = node - g_psv.worldmodel->nodes;
	}

	if (sides & 1) SV_FindTouchedLeafs(ent, node->children[0], topnode);
	if (sides & 2) SV_FindTouchedLeafs(ent, node->children[1], topnode);
}

#else // REHLDS_OPT_PEDANTIC

// unrolled some tail recursion
void SV_FindTouchedLeafs(edict_t *ent, mnode_t *node, int *topnode)
{
	while (true)
	{
		// if no collision model
		if (!node)
			return;

		if (node->contents == CONTENTS_SOLID)
			return;

		// add an efrag if the node is a leaf
		if (node->contents < 0)
		{
			if (ent->num_leafs > (MAX_ENT_LEAFS - 1))
			{
				// continue counting leafs,
				// so we know how many it's overrun
				ent->num_leafs = (MAX_ENT_LEAFS + 1);
			}
			else
			{
				mleaf_t *leaf = (mleaf_t *)node;
				int leafnum = leaf - g_psv.worldmodel->leafs - 1;
				ent->leafnums[ent->num_leafs] = leafnum;
				ent->num_leafs++;
			}

			return;
		}

		// NODE_MIXED
		mplane_t *splitplane = node->plane;
		int sides = BOX_ON_PLANE_SIDE(ent->v.absmin, ent->v.absmax, splitplane);

		switch (sides)
		{
		case 1: node = node->children[0]; break; // do only SV_FindTouchedLeafs(ent, node->children[0], topnode);
		case 2: node = node->children[1]; break; // do only SV_FindTouchedLeafs(ent, node->children[1], topnode);
		case 3:
		{
			if (*topnode == -1)
				*topnode = node - g_psv.worldmodel->nodes;

			// do both children nodes
			SV_FindTouchedLeafs(ent, node->children[0], topnode);
			node = node->children[1];
			break;
		}
		default:
			return;
		}
	}
}

#endif // REHLDS_OPT_PEDANTIC

// Needs to be called any time an entity changes origin, mins, maxs, or solid
// flags ent->v.modified
// sets ent->v.absmin and ent->v.absmax
// if touchtriggers, calls prog functions for the intersected triggers
void SV_LinkEdict(edict_t *ent, qboolean touch_triggers)
{
	static int iTouchLinkSemaphore = 0; // prevent recursion when SV_TouchLinks is active
	areanode_t *node;

	// unlink from old position
	if (ent->area.prev)
		SV_UnlinkEdict(ent);

	// don't add the world or free ents
	if (ent == &g_psv.edicts[0] || ent->free)
		return;

	// set the abs box
	gEntityInterface.pfnSetAbsBox(ent);

	if (ent->v.movetype == MOVETYPE_FOLLOW && ent->v.aiment)
	{
		ent->headnode = ent->v.aiment->headnode;
		ent->num_leafs = ent->v.aiment->num_leafs;
		Q_memcpy(ent->leafnums, ent->v.aiment->leafnums, sizeof(ent->leafnums));
	}
	else
	{
		int topnode = -1;

		// link to PVS leafs
		ent->num_leafs = 0;
		ent->headnode = -1;

		if (ent->v.modelindex)
			SV_FindTouchedLeafs(ent, g_psv.worldmodel->nodes, &topnode);

		if (ent->num_leafs > MAX_ENT_LEAFS)
		{
			Q_memset(ent->leafnums, -1, sizeof(ent->leafnums));

			ent->num_leafs = 0; // so we use headnode instead
			ent->headnode = topnode;
		}
	}

	// ignore non-solid bodies
	if (ent->v.solid == SOLID_NOT && ent->v.skin >= -1)
		return;

	if (ent->v.solid == SOLID_BSP && !Mod_Handle(ent->v.modelindex) && Q_strlen(&pr_strings[ent->v.model]) <= 0)
	{
		Con_DPrintf("Inserted %s with no model\n", &pr_strings[ent->v.classname]);
		return;
	}

	// find the first node that the ent's box crosses
	node = sv_areanodes;
	while (true)
	{
		if (node->axis == -1)
			break;

		if (ent->v.absmin[node->axis] > node->dist)
			node = node->children[0];
		else if (ent->v.absmax[node->axis] < node->dist)
			node = node->children[1];
		else break; // crosses the node
	}

	// link it in
	if (ent->v.solid == SOLID_TRIGGER)
		InsertLinkBefore(&ent->area, &node->trigger_edicts);
	else
		InsertLinkBefore(&ent->area, &node->solid_edicts);

	if (touch_triggers && !iTouchLinkSemaphore)
	{
		iTouchLinkSemaphore = 1;
		SV_TouchLinks(ent, sv_areanodes);
		iTouchLinkSemaphore = 0;
	}
}

int SV_HullPointContents(hull_t *hull, int num, const vec_t *p)
{
	float d;
	dclipnode_t *node;
	mplane_t *plane;

	while (num >= 0)
	{
		if (num < hull->firstclipnode || num > hull->lastclipnode)
			Sys_Error("%s: bad node number", __func__);

		node = &hull->clipnodes[num];
		plane = &hull->planes[node->planenum];

		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = _DotProduct(plane->normal, p) - plane->dist;

		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	return num;
}

int SV_LinkContents(areanode_t *node, const vec_t *pos)
{
	vec3_t localPosition, offset;
	link_t *next, *l;
	edict_t *touch;

#ifdef REHLDS_OPT_PEDANTIC
	// unroll tail recursion
	while (true)
#endif
	{
		for (l = node->solid_edicts.next; l != &node->solid_edicts; l = next)
		{
			touch = EDICT_FROM_AREA(l);
			next = l->next;

			if (touch->v.solid != SOLID_NOT)
				continue;

			if (touch->v.groupinfo)
			{
				if (g_groupop)
				{
					if (g_groupop == GROUP_OP_NAND && (touch->v.groupinfo & g_groupmask))
						continue;
				}
				else
				{
					if (!(touch->v.groupinfo & g_groupmask))
						continue;
				}
			}

			if (Mod_GetType(touch->v.modelindex) != mod_brush)
				continue;

			if (!BoundsIntersect(pos, pos, touch->v.absmin, touch->v.absmax))
				continue;

			int contents = touch->v.skin;
			if (contents < -100 || contents > 100)
				Con_DPrintf("Invalid contents on trigger field: %s\n", &pr_strings[touch->v.classname]);

			// force to select bsp-hull
			hull_t *hull = SV_HullForBsp(touch, vec3_origin, vec3_origin, offset);

			// offset the test point appropriately for this hull
			VectorSubtract(pos, offset, localPosition);

			// test hull for intersection with this model
			if (SV_HullPointContents(hull, hull->firstclipnode, localPosition) != CONTENTS_EMPTY)
				return contents;
		}

		if (node->axis == -1)
			return CONTENTS_EMPTY;

#ifndef REHLDS_OPT_PEDANTIC
		if (pos[node->axis] > node->dist)
			return SV_LinkContents(node->children[0], pos);

		else if (pos[node->axis] < node->dist)
			return SV_LinkContents(node->children[1], pos);

#else // REHLDS_OPT_PEDANTIC

		if (pos[node->axis] > node->dist)
		{
			node = node->children[0];
			continue;
		}
		else if (pos[node->axis] < node->dist)
		{
			node = node->children[1];
			continue;
		}

		break;
#endif // REHLDS_OPT_PEDANTIC
	}

	return CONTENTS_EMPTY;
}

// Returns the CONTENTS_* value from the world at the given point.
// does not check any entities at all
int SV_PointContents(const vec_t *p)
{
	int cont = SV_HullPointContents(g_psv.worldmodel->hulls, 0, p);
	if (cont <= CONTENTS_CURRENT_0 && cont >= CONTENTS_CURRENT_DOWN)
	{
		cont = CONTENTS_WATER;
	}
	else
	{
		if (cont == CONTENTS_SOLID)
			return CONTENTS_SOLID;
	}

	int entityContents = SV_LinkContents(&sv_areanodes[0], p);
	return (entityContents != CONTENTS_EMPTY) ? entityContents : cont;
}

// Returns true if the entity is in solid currently
edict_t *SV_TestEntityPosition(edict_t *ent)
{
	qboolean monsterClip = (ent->v.flags & FL_MONSTERCLIP) ? TRUE : FALSE;
	trace_t trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, ent->v.origin, MOVE_NORMAL, ent, monsterClip);

	if (trace.startsolid)
	{
		SV_SetGlobalTrace(&trace);
		return trace.ent;
	}

	return nullptr;
}

const float DIST_EPSILON = 0.03125f;

#ifndef REHLDS_OPT_PEDANTIC
qboolean SV_RecursiveHullCheck(hull_t *hull, int num, float p1f, float p2f, const vec_t *p1, const vec_t *p2, trace_t *trace)
{
	dclipnode_t *node;
	mplane_t *plane;
	float t1, t2;
	float frac, midf, pointf;
	vec3_t mid;
	int side;
	float pdif = p2f - p1f;

	if (num < 0)
	{
		if (num != CONTENTS_SOLID)
		{
			trace->allsolid = FALSE;

			if (num == CONTENTS_EMPTY)
				trace->inopen = TRUE;

			else if (num != CONTENTS_TRANSLUCENT)
				trace->inwater = TRUE;
		}
		else
		{
			trace->startsolid = TRUE;
		}

		// empty
		return TRUE;
	}

	if (num < hull->firstclipnode || num > hull->lastclipnode || !hull->planes)
		Sys_Error("%s: bad node number", __func__);

	// find the point distances
	node = &hull->clipnodes[num];
	plane = &hull->planes[hull->clipnodes[num].planenum];

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
	}
	else
	{
		t1 = _DotProduct(plane->normal, p1) - plane->dist;
		t2 = _DotProduct(plane->normal, p2) - plane->dist;
	}

	if (t1 >= 0.0f && t2 >= 0.0f)
		return SV_RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2, trace);

	if (t1 < 0.0f && t2 < 0.0f)
		return SV_RecursiveHullCheck(hull, node->children[1], p1f, p2f, p1, p2, trace);

	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < 0.0f)
	{
		frac = (t1 + DIST_EPSILON) / (t1 - t2);
	}
	else
	{
		frac = (t1 - DIST_EPSILON) / (t1 - t2);
	}

	if (frac < 0.0f)
		frac = 0.0f;

	else if (frac > 1.0f)
		frac = 1.0f;

	if (IS_NAN(frac))
	{
		// not a number
		return FALSE;
	}

	midf = p1f + pdif * frac;

	real3_t point;
	VectorSubtract(p2, p1, point);
	VectorMA(p1, frac, point, mid);

	side = (t1 < 0.0f) ? 1 : 0;

	// move up to the node
	if (!SV_RecursiveHullCheck(hull, node->children[side], p1f, midf, p1, mid, trace))
		return FALSE;

	if (SV_HullPointContents(hull, node->children[side ^ 1], mid) != CONTENTS_SOLID)
	{
		// go past the node
		return SV_RecursiveHullCheck(hull, node->children[side ^ 1], midf, p2f, mid, p2, trace);
	}

	if (trace->allsolid)
	{
		// never got out of the solid area
		return FALSE;
	}

	// the other side of the node is solid, this is the impact point
	if (!side)
	{
		VectorCopy(plane->normal, trace->plane.normal);
		trace->plane.dist = plane->dist;
	}
	else
	{
		VectorNegate(plane->normal, trace->plane.normal);
		trace->plane.dist = -plane->dist;
	}

	while (SV_HullPointContents(hull, hull->firstclipnode, mid) == CONTENTS_SOLID)
	{
		// shouldn't really happen, but does occasionally
		frac -= 0.1f;
		if (frac < 0.0f)
		{
			trace->fraction = midf;
			VectorCopy(mid, trace->endpos);
			Con_DPrintf("backup past 0\n");
			return FALSE;
		}

		midf = p1f + pdif * frac;

		real3_t point;
		VectorSubtract(p2, p1, point);
		VectorMA(p1, frac, point, mid);
	}

	trace->fraction = midf;
	VectorCopy(mid, trace->endpos);

	return FALSE;
}

#else // REHLDS_OPT_PEDANTIC

// version with unrolled tail recursion
qboolean SV_RecursiveHullCheck(hull_t *hull, int num, float p1f, float p2f, const vec_t *p1, const vec_t *p2, trace_t *trace)
{
	dclipnode_t *node;
	mplane_t *plane;
	float t1, t2;
	float frac, midf;
	vec3_t mid, custom_p1; // for holding custom p1 value
	int side;
	float pdif;

	while (num >= 0)
	{
		pdif = p2f - p1f;

		if (num < hull->firstclipnode || num > hull->lastclipnode || !hull->planes)
			Sys_Error("%s: bad node number", __func__);

		// find the point distances
		node = &hull->clipnodes[num];
		plane = &hull->planes[hull->clipnodes[num].planenum];

		if (plane->type < 3)
		{
			t1 = p1[plane->type] - plane->dist;
			t2 = p2[plane->type] - plane->dist;
		}
		else
		{
			t1 = _DotProduct(plane->normal, p1) - plane->dist;
			t2 = _DotProduct(plane->normal, p2) - plane->dist;
		}

		if (t1 >= 0.0f && t2 >= 0.0f)
		{
			num = node->children[0];
			continue;
		}

		if (t1 < 0.0f && t2 < 0.0f)
		{
			num = node->children[1];
			continue;
		}

		// put the crosspoint DIST_EPSILON pixels on the near side
		if (t1 < 0.0f)
		{
			frac = (t1 + DIST_EPSILON) / (t1 - t2);
		}
		else
		{
			frac = (t1 - DIST_EPSILON) / (t1 - t2);
		}

		if (frac < 0.0f)
			frac = 0.0f;

		else if (frac > 1.0f)
			frac = 1.0f;

		if (IS_NAN(frac))
		{
			// not a number
			return FALSE;
		}

		midf = p1f + pdif * frac;

		real3_t point;
		VectorSubtract(p2, p1, point);
		VectorMA(p1, frac, point, mid);

		side = (t1 < 0.0f) ? 1 : 0;

		// move up to the node
		if (!SV_RecursiveHullCheck(hull, node->children[side], p1f, midf, p1, mid, trace))
			return FALSE;

		if (SV_HullPointContents(hull, node->children[side ^ 1], mid) != CONTENTS_SOLID)
		{
			// go past the node
			num = node->children[side ^ 1];
			p1f = midf;
			p1 = custom_p1;
			VectorCopy(mid, custom_p1);
			continue;
		}

		if (trace->allsolid)
		{
			// never got out of the solid area
			return FALSE;
		}

		// the other side of the node is solid, this is the impact point
		if (!side)
		{
			VectorCopy(plane->normal, trace->plane.normal);
			trace->plane.dist = plane->dist;
		}
		else
		{
			VectorNegate(plane->normal, trace->plane.normal);
			trace->plane.dist = -plane->dist;
		}

		while (SV_HullPointContents(hull, hull->firstclipnode, mid) == CONTENTS_SOLID)
		{
			// shouldn't really happen, but does occasionally
			frac -= 0.1f;
			if (frac < 0.0f)
			{
				trace->fraction = midf;
				VectorCopy(mid, trace->endpos);
				Con_DPrintf("backup past 0\n");
				return FALSE;
			}

			midf = p1f + pdif * frac;

			real3_t point;
			VectorSubtract(p2, p1, point);
			VectorMA(p1, frac, point, mid);
		}

		trace->fraction = midf;
		VectorCopy(mid, trace->endpos);
		return FALSE;
	}

	if (num != CONTENTS_SOLID)
	{
		trace->allsolid = FALSE;

		if (num == CONTENTS_EMPTY)
			trace->inopen = TRUE;

		else if (num != CONTENTS_TRANSLUCENT)
			trace->inwater = TRUE;
	}
	else
	{
		trace->startsolid = TRUE;
	}

	// empty
	return TRUE;
}
#endif // REHLDS_OPT_PEDANTIC

void SV_SingleClipMoveToEntity(edict_t *ent, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, trace_t *trace)
{
	hull_t *hull;
	vec3_t offset;
	bool rotated;
	vec3_t end_l;
	vec3_t start_l;
	int numhulls;

	// fill in a default trace
	Q_memset(trace, 0, sizeof(trace_t));
	VectorCopy(end, trace->endpos);
	trace->fraction = 1.0f;
	trace->allsolid = TRUE;

#ifdef REHLDS_FIXES
	if (Mod_GetType(ent->v.modelindex) == mod_studio)
#else
	if (g_psv.models[ent->v.modelindex]->type == mod_studio)
#endif // REHLDS_FIXES
	{
		// get the clipping hull from studio model
		hull = SV_HullForStudioModel(ent, mins, maxs, offset, &numhulls);
	}
	else
	{
		// get the clipping hull
		hull = SV_HullForEntity(ent, mins, maxs, offset);
		numhulls = 1;
	}

	VectorSubtract(start, offset, start_l);
	VectorSubtract(end, offset, end_l);

	// rotate start and end into the models frame of reference
	if (ent->v.solid == SOLID_BSP && !VectorIsZero(ent->v.angles))
	{
		vec3_t forward, right, up;
		vec3_t temp;

		AngleVectors(ent->v.angles, forward, right, up);

		VectorCopy(start_l, temp);
		start_l[0] = _DotProduct(temp, forward);
		start_l[1] = -_DotProduct(temp, right);
		start_l[2] = _DotProduct(temp, up);

		VectorCopy(end_l, temp);
		end_l[0] = _DotProduct(temp, forward);
		end_l[1] = -_DotProduct(temp, right);
		end_l[2] = _DotProduct(temp, up);

		rotated = true;
	}
	else
	{
		rotated = false;
	}

	// trace a line through the appropriate clipping hull
	if (numhulls == 1)
	{
		SV_RecursiveHullCheck(hull, hull->firstclipnode, 0.0f, 1.0f, start_l, end_l, trace);
	}
	else
	{
		int last_hitgroup = 0;
		for (int i = 0; i < numhulls; i++)
		{
			// fill in a default trace
			trace_t trace_hitbox;
			Q_memset(&trace_hitbox, 0, sizeof(trace_hitbox));
			VectorCopy(end, trace_hitbox.endpos);
			trace_hitbox.fraction = 1.0f;
			trace_hitbox.allsolid = TRUE;

			SV_RecursiveHullCheck(&hull[i], hull[i].firstclipnode, 0.0f, 1.0f, start_l, end_l, &trace_hitbox);

			if (i == 0 || trace_hitbox.allsolid || trace_hitbox.startsolid || trace_hitbox.fraction < trace->fraction)
			{
				if (trace->startsolid)
				{
					*trace = trace_hitbox;
					trace->startsolid = TRUE;
				}
				else
				{
					*trace = trace_hitbox;
				}

				last_hitgroup = i;
			}
		}

		trace->hitgroup = SV_HitgroupForStudioHull(last_hitgroup);
	}

	if (trace->fraction != 1.0f)
	{
		if (rotated)
		{
			vec3_t forward, right, up;
			vec3_t temp;

			AngleVectorsTranspose(ent->v.angles, forward, right, up);
			VectorCopy(trace->plane.normal, temp);

			trace->plane.normal[0] = _DotProduct(temp, forward);
			trace->plane.normal[1] = _DotProduct(temp, right);
			trace->plane.normal[2] = _DotProduct(temp, up);
		}

		real3_t point;
		VectorSubtract(end, start, point);
		VectorMA(start, trace->fraction, point, trace->endpos);
	}

	// did we clip the move?
	if (trace->fraction < 1.0f || trace->startsolid)
		trace->ent = ent;
}

// Handles selection or creation of a clipping hull, and offseting (and eventually rotation) of the end points
trace_t SV_ClipMoveToEntity(edict_t *ent, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end)
{
	trace_t trace;
	SV_SingleClipMoveToEntity(ent, start, mins, maxs, end, &trace);
	return trace;
}

// Mins and maxs enclose the entire area swept by the move
void SV_ClipToLinks(areanode_t *node, moveclip_t *clip)
{
	link_t *next, *l;
	edict_t *touch;

	// touch linked edicts
	for (l = node->solid_edicts.next; l != &node->solid_edicts; l = next)
	{
		next = l->next;
		touch = EDICT_FROM_AREA(l);

		if (touch->v.groupinfo && clip->passedict && clip->passedict->v.groupinfo)
		{
			if (g_groupop)
			{
				if (g_groupop == GROUP_OP_NAND && (clip->passedict->v.groupinfo & touch->v.groupinfo))
					continue;
			}
			else
			{
				if (!(clip->passedict->v.groupinfo & touch->v.groupinfo))
					continue;
			}
		}

		if (touch->v.solid == SOLID_NOT || touch == clip->passedict)
			continue;

		if (touch->v.solid == SOLID_TRIGGER)
			Sys_Error("%s: Trigger in clipping list", __func__);

#ifndef REHLDS_OPT_PEDANTIC
		if (gNewDLLFunctions.pfnShouldCollide && !gNewDLLFunctions.pfnShouldCollide(touch, clip->passedict))
#ifdef REHLDS_FIXES
			// https://github.com/dreamstalker/rehlds/issues/46
			continue;
#else
			return;
#endif // REHLDS_FIXES
#endif // REHLDS_OPT_PEDANTIC

		// monsterclip filter
		if (touch->v.solid == SOLID_BSP)
		{
			if ((touch->v.flags & FL_MONSTERCLIP) && !clip->monsterClipBrush)
				continue;
		}
		else
		{
			// ignore all monsters but pushables
			if (clip->type == MOVE_NOMONSTERS && touch->v.movetype != MOVETYPE_PUSHSTEP)
				continue;
		}

		if (clip->ignoretrans && touch->v.rendermode != kRenderNormal && !(touch->v.flags & FL_WORLDBRUSH))
			continue;

		if (!BoundsIntersect(clip->boxmins, clip->boxmaxs, touch->v.absmin, touch->v.absmax))
			continue;

		if (touch->v.solid != SOLID_SLIDEBOX
#ifdef REHLDS_FIXES
			|| sv_force_ent_intersection.value
#endif
)
		{
			if (!SV_CheckSphereIntersection(touch, clip->start, clip->end))
				continue;
		}

		if (clip->passedict && clip->passedict->v.size[0] && !touch->v.size[0])
			continue; // points never interact

		// might intersect, so do an exact clip
		if (clip->trace.allsolid)
			return;

		if (clip->passedict)
		{
			if (touch->v.owner == clip->passedict)
				continue; // don't clip against own missiles

			if (clip->passedict->v.owner == touch)
				continue; // don't clip against owner
		}

#ifdef REHLDS_OPT_PEDANTIC
		if (gNewDLLFunctions.pfnShouldCollide && !gNewDLLFunctions.pfnShouldCollide(touch, clip->passedict))
#ifdef REHLDS_FIXES
			// https://github.com/dreamstalker/rehlds/issues/46
			continue;
#else
			return;
#endif // REHLDS_FIXES
#endif // REHLDS_OPT_PEDANTIC

		trace_t trace;
		if (touch->v.flags & FL_MONSTER)
			trace = SV_ClipMoveToEntity(touch, clip->start, clip->mins2, clip->maxs2, clip->end);
		else
			trace = SV_ClipMoveToEntity(touch, clip->start, clip->mins, clip->maxs, clip->end);

		if (trace.allsolid || trace.startsolid || trace.fraction < clip->trace.fraction)
		{
			trace.ent = touch;
			if (clip->trace.startsolid)
			{
				clip->trace = trace;
				clip->trace.startsolid = TRUE;
			}
			else
			{
				clip->trace = trace;
			}
		}
	}

	// recurse down both sides
	if (node->axis == -1)
		return;

	if (clip->boxmaxs[node->axis] > node->dist)
		SV_ClipToLinks(node->children[0], clip);

	if (node->dist > clip->boxmins[node->axis])
		SV_ClipToLinks(node->children[1], clip);
}

// Mins and maxs enclose the entire area swept by the move
void SV_ClipToWorldbrush(areanode_t *node, moveclip_t *clip)
{
	link_t *l;
	link_t *next;
	edict_t *touch;

	for (l = node->solid_edicts.next; l != &node->solid_edicts; l = next)
	{
		next = l->next;
		touch = EDICT_FROM_AREA(l);

		if (touch->v.solid != SOLID_BSP)
			continue;

		if (!(touch->v.flags & FL_WORLDBRUSH))
			continue;

		if (!BoundsIntersect(clip->boxmins, clip->boxmaxs, touch->v.absmin, touch->v.absmax))
			continue;

		if (clip->trace.allsolid)
			return;

		trace_t trace = SV_ClipMoveToEntity(touch, clip->start, clip->mins, clip->maxs, clip->end);
		if (trace.allsolid || trace.startsolid || trace.fraction < clip->trace.fraction)
		{
			trace.ent = touch;
			if (clip->trace.startsolid)
			{
				clip->trace = trace;
				clip->trace.startsolid = TRUE;
			}
			else
			{
				clip->trace = trace;
			}
		}
	}

	// recurse down both sides
	if (node->axis == -1)
		return;

	if (clip->boxmaxs[node->axis] > node->dist)
		SV_ClipToWorldbrush(node->children[0], clip);

	if (node->dist > clip->boxmins[node->axis])
		SV_ClipToWorldbrush(node->children[1], clip);
}

void SV_MoveBounds(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, vec_t *boxmins, vec_t *boxmaxs)
{
	for (int i = 0; i < 3; i++)
	{
		if (end[i] > start[i])
		{
			boxmins[i] = start[i] + mins[i] - 1.0f;
			boxmaxs[i] = end[i] + maxs[i] + 1.0f;
		}
		else
		{
			boxmins[i] = end[i] + mins[i] - 1.0f;
			boxmaxs[i] = start[i] + maxs[i] + 1.0f;
		}
	}
}

trace_t SV_MoveNoEnts(const vec_t *start, vec_t *mins, vec_t *maxs, const vec_t *end, int type, edict_t *passedict)
{
	moveclip_t clip;
	vec3_t trace_endpos;
	float trace_fraction;

	Q_memset(&clip, 0, sizeof(clip));
	clip.trace = SV_ClipMoveToEntity(g_psv.edicts, start, mins, maxs, end);

	if (clip.trace.fraction != 0.0f)
	{
		VectorCopy(clip.trace.endpos, trace_endpos);

		trace_fraction = clip.trace.fraction;

		clip.trace.fraction = 1.0f;
		clip.start = start;
		clip.end = trace_endpos;

		clip.type = (type & 0xff);
		clip.ignoretrans = (type >> 8);
		clip.passedict = passedict;
		clip.monsterClipBrush = FALSE;

		clip.mins = mins;
		clip.maxs = maxs;

		VectorCopy(mins, clip.mins2);
		VectorCopy(maxs, clip.maxs2);

		SV_MoveBounds(start, clip.mins2, clip.maxs2, trace_endpos, clip.boxmins, clip.boxmaxs);
		SV_ClipToWorldbrush(sv_areanodes, &clip);

		clip.trace.fraction *= trace_fraction;
		gGlobalVariables.trace_ent = clip.trace.ent;
	}

	return clip.trace;
}

// mins and maxs are reletive
// if the entire move stays in a solid volume, trace.allsolid will be set
// if the starting point is in a solid, it will be allowed to move out to an open area
// nomonsters is used for line of sight or edge testing,
// where mosnters shouldn't be considered solid objects
// passedict is explicitly excluded from clipping checks (normally NULL)
trace_t SV_Move(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int type, edict_t *passedict, qboolean monsterClipBrush)
{
	moveclip_t clip;
	vec3_t trace_endpos;
	float trace_fraction;

	Q_memset(&clip, 0, sizeof(clip));
	clip.trace = SV_ClipMoveToEntity(g_psv.edicts, start, mins, maxs, end);

	if (clip.trace.fraction != 0.0f)
	{
		VectorCopy(clip.trace.endpos, trace_endpos);

		trace_fraction = clip.trace.fraction;

		clip.trace.fraction = 1.0f;
		clip.start = start;
		clip.end = trace_endpos;

		clip.type = (type & 0xff);
		clip.ignoretrans = (type >> 8);
		clip.passedict = passedict;
		clip.monsterClipBrush = monsterClipBrush;

		clip.mins = mins;
		clip.maxs = maxs;

		if (type == MOVE_MISSILE)
		{
			for (int i = 0; i < 3; i++)
			{
				clip.mins2[i] = -15.0f;
				clip.maxs2[i] = +15.0f;
			}
		}
		else
		{
			VectorCopy(mins, clip.mins2);
			VectorCopy(maxs, clip.maxs2);
		}

		SV_MoveBounds(start, clip.mins2, clip.maxs2, trace_endpos, clip.boxmins, clip.boxmaxs);
		SV_ClipToLinks(sv_areanodes, &clip);

		clip.trace.fraction *= trace_fraction;
		gGlobalVariables.trace_ent = clip.trace.ent;
	}

	return clip.trace;
}

#ifdef REHLDS_OPT_PEDANTIC

// Optimized version of SV_Move routines for moving point hull throw world
void SV_SingleClipMoveToPoint(const vec_t *start, const vec_t *end, trace_t *trace)
{
	Q_memset(trace, 0, sizeof(trace_t));
	trace->fraction = 1.0f;
	trace->allsolid = TRUE;
	VectorCopy(end, trace->endpos);

	hull_t *hull = &g_psv.models[1]->hulls[point_hull]; // world point hull
	SV_RecursiveHullCheck(hull, hull->firstclipnode, 0.0f, 1.0f, start, end, trace);

	if (trace->fraction != 1.0f)
	{
		trace->endpos[0] = (end[0] - start[0]) * trace->fraction + start[0];
		trace->endpos[1] = (end[1] - start[1]) * trace->fraction + start[1];
		trace->endpos[2] = (end[2] - start[2]) * trace->fraction + start[2];
	}

	if (trace->fraction < 1.0f || trace->startsolid)
		trace->ent = &g_psv.edicts[0];
}

void SV_MoveBounds_Point(const vec_t *start, const vec_t *end, vec_t *boxmins, vec_t *boxmaxs)
{
	for (int i = 0; i < 3; i++)
	{
		if (end[i] > start[i])
		{
			boxmins[i] = start[i] - 1.0f;
			boxmaxs[i] = end[i] + 1.0f;
		}
		else
		{
			boxmins[i] = end[i] - 1.0f;
			boxmaxs[i] = start[i] + 1.0f;
		}
	}
}

trace_t SV_Move_Point(const vec_t *start, const vec_t *end, int type, edict_t *passedict)
{
	moveclip_t clip;
	vec3_t trace_endpos;
	float trace_fraction;

	Q_memset(&clip, 0, sizeof(clip));
	SV_SingleClipMoveToPoint(start, end, &clip.trace);

	if (clip.trace.fraction != 0.0f)
	{
		VectorCopy(clip.trace.endpos, trace_endpos);

		trace_fraction = clip.trace.fraction;

		clip.trace.fraction = 1.0f;
		clip.start = start;
		clip.end = trace_endpos;

		clip.type = (type & 0xff);
		clip.ignoretrans = (type >> 8);
		clip.passedict = passedict;
		clip.monsterClipBrush = FALSE;

		clip.mins = vec3_origin;
		clip.maxs = vec3_origin;

		if (type == MOVE_MISSILE)
		{
			for (int i = 0; i < 3; i++)
			{
				clip.mins2[i] = -15.0f;
				clip.maxs2[i] = +15.0f;
			}
		}
		else
		{
			VectorClear(clip.mins2);
			VectorClear(clip.maxs2);
		}

		SV_MoveBounds_Point(start, trace_endpos, clip.boxmins, clip.boxmaxs);
		SV_ClipToLinks(sv_areanodes, &clip);

		clip.trace.fraction *= trace_fraction;
		gGlobalVariables.trace_ent = clip.trace.ent;
	}

	return clip.trace;
}

#endif // REHLDS_OPT_PEDANTIC
