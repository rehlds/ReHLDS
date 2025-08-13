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

int g_contentsresult;
hull_t box_hull_0;
box_clipnodes_t box_clipnodes_0;
box_planes_t box_planes_0;

float EXT_FUNC PM_TraceModel(physent_t *pEnt, vec_t *start, vec_t *end, trace_t *trace)
{
	hull_t *pHull;
	int saveHull;
	vec3_t start_l;
	vec3_t end_l;
	vec3_t offset;

	saveHull = pmove->usehull;
	pmove->usehull = 2;
	pHull = PM_HullForBsp(pEnt, offset);
	pmove->usehull = saveHull;
	start_l[0] = start[0] - offset[0];
	start_l[1] = start[1] - offset[1];
	start_l[2] = start[2] - offset[2];
	end_l[0] = end[0] - offset[0];
	end_l[1] = end[1] - offset[1];
	end_l[2] = end[2] - offset[2];
	SV_RecursiveHullCheck(pHull, pHull->firstclipnode, 0.0, 1.0, start_l, end_l, trace);
	trace->ent = 0;
	return trace->fraction;
}

void EXT_FUNC PM_GetModelBounds(struct model_s *mod, vec_t *mins, vec_t *maxs)
{
	mins[0] = mod->mins[0];
	mins[1] = mod->mins[1];
	mins[2] = mod->mins[2];

	maxs[0] = mod->maxs[0];
	maxs[1] = mod->maxs[1];
	maxs[2] = mod->maxs[2];
}

int EXT_FUNC PM_GetModelType(struct model_s *mod)
{
	return mod->type;
}

void PM_InitBoxHull(void)
{
	box_hull_0.clipnodes = &box_clipnodes_0[0];
	box_hull_0.planes = &box_planes_0[0];
	box_hull_0.firstclipnode = 0;
	box_hull_0.lastclipnode = 5;

	for (int i = 0; i < 6; i++)
	{
		int side = i & 1;
		box_clipnodes_0[i].planenum = i;
		box_clipnodes_0[i].children[side] = CONTENTS_EMPTY;
		box_clipnodes_0[i].children[side ^ 1] = (i != 5) ? i + 1 : CONTENTS_SOLID;
		box_planes_0[i].type = i >> 1;
		box_planes_0[i].normal[i >> 1] = 1.0f;
	}
}

hull_t *PM_HullForBox(vec_t *mins, vec_t *maxs)
{
	box_planes_0[0].dist = maxs[0];
	box_planes_0[1].dist = mins[0];
	box_planes_0[2].dist = maxs[1];
	box_planes_0[3].dist = mins[1];
	box_planes_0[4].dist = maxs[2];
	box_planes_0[5].dist = mins[2];
	return &box_hull_0;
}

int EXT_FUNC PM_HullPointContents(hull_t *hull, int num, vec_t *p)
{
	float d;
	dclipnode_t *node;
	mplane_t *plane;

	if (hull->firstclipnode >= hull->lastclipnode)
		return CONTENTS_EMPTY;

	while (num >= 0)
	{
		if (num < hull->firstclipnode || num > hull->lastclipnode)
			Sys_Error("%s: bad node number", __func__);
		node = &hull->clipnodes[num];
		plane = &hull->planes[node->planenum];

		if (plane->type >= 3)
			d = _DotProduct(p, plane->normal) - plane->dist;
		else
			d = p[plane->type] - plane->dist;

		if (d >= 0.0)
			num = node->children[0];
		else
			num = node->children[1];
	}

	return num;
}

int PM_LinkContents(vec_t *p, int *pIndex)
{
	physent_t *pe;
	vec3_t test;

	for (int i = 1; i < pmove->numphysent; i++) {
		pe = &pmove->physents[i];
		model_t* model = pmove->physents[i].model;
		if (pmove->physents[i].solid || model == NULL)
			continue;

		test[0] = p[0] - pe->origin[0];
		test[1] = p[1] - pe->origin[1];
		test[2] = p[2] - pe->origin[2];
		if (PM_HullPointContents(model->hulls, model->hulls[point_hull].firstclipnode, test) != CONTENTS_EMPTY) {
			if (pIndex)
				*pIndex = pe->info;
			return pe->skin;
		}
	}

	return CONTENTS_EMPTY;
}

int EXT_FUNC PM_PointContents(vec_t *p, int *truecontents)
{
#ifndef SWDS
	g_engdstAddrs.PM_PointContents(&p, &truecontents);
#endif

	if ((int)pmove->physents[0].model != -208)
	{
		int entityContents = PM_HullPointContents(
			pmove->physents[0].model->hulls,
			pmove->physents[0].model->hulls[point_hull].firstclipnode,
			p);
		if (truecontents)
			*truecontents = entityContents;
		if (entityContents > CONTENTS_CURRENT_0 || entityContents < CONTENTS_CURRENT_DOWN)
		{
			if (entityContents == CONTENTS_SOLID)
				return entityContents;
		}
		else
		{
			entityContents = CONTENTS_WATER;
		}
		int cont = PM_LinkContents(p, 0);
		if (cont != CONTENTS_EMPTY)
			return cont;
		return entityContents;
	}
	if (truecontents)
		*truecontents = CONTENTS_EMPTY;
	return CONTENTS_SOLID;
}

int PM_WaterEntity(vec_t *p)
{
	int cont;
	int entityIndex;

#ifndef SWDS
	g_engdstAddrs.PM_WaterEntity(&p);
#endif

	model_t* model = pmove->physents[0].model;
	cont = PM_HullPointContents(model->hulls, model->hulls[point_hull].firstclipnode, p);
	if (cont == CONTENTS_SOLID) {
		return CONTENTS_EMPTY;
	}

	entityIndex = 0;
	return PM_LinkContents(p, &entityIndex);
}

int EXT_FUNC PM_TruePointContents(vec_t *p)
{
	if ((int)pmove->physents[0].model == -208)
		return CONTENTS_EMPTY;
	else
		return PM_HullPointContents(pmove->physents[0].model->hulls, pmove->physents[0].model->hulls[point_hull].firstclipnode, p);
}

hull_t *PM_HullForStudioModel(model_t *pModel, vec_t *offset, float frame, int sequence, const vec_t *angles, const vec_t *origin, const unsigned char *pcontroller, const unsigned char *pblending, int *pNumHulls)
{
	vec3_t size;

	size[0] = player_maxs[pmove->usehull][0] - player_mins[pmove->usehull][0];
	size[1] = player_maxs[pmove->usehull][1] - player_mins[pmove->usehull][1];
	size[2] = player_maxs[pmove->usehull][2] - player_mins[pmove->usehull][2];
	VectorScale(size, 0.5, size);
	offset[0] = 0;
	offset[1] = 0;
	offset[2] = 0;
	return R_StudioHull(pModel, frame, sequence, angles, origin, size, pcontroller, pblending, pNumHulls, 0, 0);
}

hull_t* EXT_FUNC PM_HullForBsp(physent_t *pe, vec_t *offset)
{
	hull_t *hull;

	switch (pmove->usehull) {
	case 1:
		hull = &pe->model->hulls[head_hull];
		break;

	case 2:
		hull = &pe->model->hulls[point_hull];
		break;

	case 3:
		hull = &pe->model->hulls[large_hull];
		break;

	default:
		hull = &pe->model->hulls[human_hull];
		break;
	}

	offset[0] = hull->clip_mins[0] - player_mins[pmove->usehull][0];
	offset[1] = hull->clip_mins[1] - player_mins[pmove->usehull][1];
	offset[2] = hull->clip_mins[2] - player_mins[pmove->usehull][2];
	offset[0] += pe->origin[0];
	offset[1] += pe->origin[1];
	offset[2] += pe->origin[2];
	return hull;
}

int _PM_TestPlayerPosition(vec_t *pos, pmtrace_t *ptrace, int(*pfnIgnore)(physent_t *))
{
	hull_t *hull;
	pmtrace_t tr;
	vec3_t mins;
	vec3_t maxs;
	vec3_t offset;
	int numhulls;
	vec3_t test;

	tr = PM_PlayerTrace(pmove->origin, pmove->origin, PM_NORMAL, -1);
	if (ptrace)
		Q_memcpy(ptrace, &tr, sizeof(tr));

	for (int i = 0; i < pmove->numphysent; i++)
	{
		physent_t *pe = &pmove->physents[i];
		if (pfnIgnore && pfnIgnore(pe))
			continue;

		if (pe->model && pe->solid == SOLID_NOT && pe->skin != 0)
			continue;

		offset[0] = pe->origin[0];
		offset[1] = pe->origin[1];
		offset[2] = pe->origin[2];
		numhulls = 1;

		if (pe->model)
		{
			hull = PM_HullForBsp(pe, offset);
		}
		else
		{
			if (pe->studiomodel && pe->studiomodel->type == mod_studio && ((pe->studiomodel->flags & STUDIO_TRACE_HITBOX) || pmove->usehull == 2))
			{
				hull = PM_HullForStudioModel(pe->studiomodel, offset, pe->frame, pe->sequence, pe->angles, pe->origin, pe->controller, pe->blending, &numhulls);
			}
			else
			{
				mins[0] = pe->mins[0] - player_maxs[pmove->usehull][0];
				mins[1] = pe->mins[1] - player_maxs[pmove->usehull][1];
				mins[2] = pe->mins[2] - player_maxs[pmove->usehull][2];
				maxs[0] = pe->maxs[0] - player_mins[pmove->usehull][0];
				maxs[1] = pe->maxs[1] - player_mins[pmove->usehull][1];
				maxs[2] = pe->maxs[2] - player_mins[pmove->usehull][2];
				hull = PM_HullForBox(mins, maxs);
			}
		}
		test[0] = pos[0] - offset[0];
		test[1] = pos[1] - offset[1];
		test[2] = pos[2] - offset[2];
		if (pe->solid == SOLID_BSP && (pe->angles[0] != 0.0 || pe->angles[1] != 0.0 || pe->angles[2] != 0.0))
		{
			vec3_t forward, right, up;
			AngleVectors(pe->angles, forward, right, up);

			vec3_t temp = {test[0], test[1], test[2]};
			test[0] = _DotProduct(forward, temp);
			test[1] = -_DotProduct(right, temp);
			test[2] = _DotProduct(up, temp);
		}
		if (numhulls != 1)
		{
			for (int j = 0; j < numhulls; j++)
			{
				g_contentsresult = PM_HullPointContents(&hull[j], hull[j].firstclipnode, test);
				if (g_contentsresult == CONTENTS_SOLID)
					return i;
			}
		}
		else
		{
			g_contentsresult = PM_HullPointContents(hull, hull->firstclipnode, test);
			if (g_contentsresult == CONTENTS_SOLID)
				return i;
		}
	}

	return -1;

}

int EXT_FUNC PM_TestPlayerPosition(vec_t *pos, pmtrace_t *ptrace)
{
	return _PM_TestPlayerPosition(pos, ptrace, 0);
}

int EXT_FUNC PM_TestPlayerPositionEx(vec_t *pos, pmtrace_t *ptrace, int(*pfnIgnore)(physent_t *))
{
	return _PM_TestPlayerPosition(pos, ptrace, pfnIgnore);
}

pmtrace_t _PM_PlayerTrace(vec_t *start, vec_t *end, int traceFlags, int numphysent, physent_t *physents, int ignore_pe, int(*pfnIgnore)(physent_t *))
{
	hull_t *hull;
	pmtrace_t trace;
	pmtrace_t testtrace;
	pmtrace_t total;
	vec3_t maxs;
	vec3_t mins;
	int closest;
	bool rotated;
	int pNumHulls;
	vec_t end_l[3];
	vec_t start_l[3];
	vec3_t offset;

	Q_memset(&trace, 0, sizeof(trace));
	trace.fraction = 1.0f;
	trace.ent = -1;
	trace.endpos[0] = end[0];
	trace.endpos[1] = end[1];
	trace.endpos[2] = end[2];

	for (int i = 0; i < numphysent; i++)
	{
		physent_t* pe = &physents[i];
		if (i > 0 && (traceFlags & PM_WORLD_ONLY))
			break;

		if (pfnIgnore)
		{
			if (pfnIgnore(pe))
				continue;
		}
		else
		{
			if (ignore_pe != -1 && i == ignore_pe)
				continue;
		}

		if ((pe->model && !pe->solid && pe->skin) || ((traceFlags & PM_GLASS_IGNORE) && pe->rendermode))
			continue;


		offset[0] = pe->origin[0];
		offset[1] = pe->origin[1];
		offset[2] = pe->origin[2];
		pNumHulls = 1;
		if (pe->model)
		{
			switch (pmove->usehull)
			{
			case 1:
				hull = &pe->model->hulls[head_hull];
				break;
			case 2:
				hull = &pe->model->hulls[point_hull];
				break;
			case 3:
				hull = &pe->model->hulls[large_hull];
				break;
			default:
				hull = &pe->model->hulls[human_hull];
				break;
			}
			offset[0] = hull->clip_mins[0] - player_mins[pmove->usehull][0];
			offset[1] = hull->clip_mins[1] - player_mins[pmove->usehull][1];
			offset[2] = hull->clip_mins[2] - player_mins[pmove->usehull][2];
			offset[0] = offset[0] + pe->origin[0];
			offset[1] = offset[1] + pe->origin[1];
			offset[2] = offset[2] + pe->origin[2];
		}
		else
		{
			hull = NULL;
			if (pe->studiomodel)
			{
				if (traceFlags & PM_STUDIO_IGNORE)
					continue;


				if (pe->studiomodel->type == mod_studio && (pe->studiomodel->flags & STUDIO_TRACE_HITBOX || (pmove->usehull == 2 && !(traceFlags & PM_STUDIO_BOX))))
				{
					hull = PM_HullForStudioModel(pe->studiomodel, offset, pe->frame, pe->sequence, pe->angles, pe->origin, pe->controller, pe->blending, &pNumHulls);
				}
			}

			if (!hull)
			{
				mins[0] = pe->mins[0] - player_maxs[pmove->usehull][0];
				mins[1] = pe->mins[1] - player_maxs[pmove->usehull][1];
				mins[2] = pe->mins[2] - player_maxs[pmove->usehull][2];
				maxs[0] = pe->maxs[0] - player_mins[pmove->usehull][0];
				maxs[1] = pe->maxs[1] - player_mins[pmove->usehull][1];
				maxs[2] = pe->maxs[2] - player_mins[pmove->usehull][2];
				hull = PM_HullForBox(mins, maxs);
			}
		}

		start_l[0] = start[0] - offset[0];
		start_l[1] = start[1] - offset[1];
		start_l[2] = start[2] - offset[2];
		end_l[0] = end[0] - offset[0];
		end_l[1] = end[1] - offset[1];
		end_l[2] = end[2] - offset[2];

		if (pe->solid == SOLID_BSP && (pe->angles[0] != 0.0 || pe->angles[1] != 0.0 || pe->angles[2] != 0.0))
		{
			vec3_t forward, right, up;
			AngleVectors(pe->angles, forward, right, up);

			vec3_t temp_start = {start_l[0], start_l[1], start_l[2]};
			start_l[0] = _DotProduct(forward, temp_start);
			start_l[1] = -_DotProduct(right, temp_start);
			start_l[2] = _DotProduct(up, temp_start);

			vec3_t temp_end = {end_l[0], end_l[1], end_l[2]};
			end_l[0] = _DotProduct(forward, temp_end);
			end_l[1] = -_DotProduct(right, temp_end);
			end_l[2] = _DotProduct(up, temp_end);

			rotated = true;
		}
		else
		{
			rotated = false;
		}

		Q_memset(&total, 0, sizeof(total));
		total.endpos[0] = end[0];
		total.endpos[1] = end[1];
		total.endpos[2] = end[2];
		total.fraction = 1.0f;
		total.allsolid = 1;
		if (pNumHulls <= 0)
		{
			total.allsolid = 0;
		}
		else
		{
			if (pNumHulls == 1)
			{
				PM_RecursiveHullCheck(hull, hull->firstclipnode, 0.0, 1.0, start_l, end_l, &total);
			}
			else
			{
				closest = 0;
				for (int j = 0; j < pNumHulls; j++)
				{
					Q_memset(&testtrace, 0, sizeof(testtrace));
					testtrace.endpos[0] = end[0];
					testtrace.endpos[1] = end[1];
					testtrace.endpos[2] = end[2];
					testtrace.fraction = 1.0f;
					testtrace.allsolid = 1;
					PM_RecursiveHullCheck(&hull[j], hull[j].firstclipnode, 0.0, 1.0, start_l, end_l, &testtrace);
					if (j == 0 || testtrace.allsolid || testtrace.startsolid || testtrace.fraction < total.fraction)
					{
						bool remember = (total.startsolid == 0);
						Q_memcpy(&total, &testtrace, sizeof(total));
						if (!remember)
							total.startsolid = 1;
						closest = j;
					}
					total.hitgroup = SV_HitgroupForStudioHull(closest);
				}
			}

			if (total.allsolid)
				total.startsolid = 1;

		}

		if (total.startsolid)
			total.fraction = 0;

		if (total.fraction != 1.0)
		{
			if (rotated)
			{
				vec3_t forward, right, up;
				AngleVectorsTranspose(pe->angles, forward, right, up);

				vec3_t temp = {total.plane.normal[0], total.plane.normal[1], total.plane.normal[2]};
				total.plane.normal[0] = _DotProduct(forward, temp);
				total.plane.normal[1] = _DotProduct(right, temp);
				total.plane.normal[2] = _DotProduct(up, temp);
			}
			total.endpos[0] = (end[0] - start[0]) * total.fraction + start[0];
			total.endpos[1] = (end[1] - start[1]) * total.fraction + start[1];
			total.endpos[2] = (end[2] - start[2]) * total.fraction + start[2];
		}

		if (total.fraction < trace.fraction)
		{
			Q_memcpy(&trace, &total, sizeof(trace));
			trace.ent = i;
		}
	}

	return trace;
}

pmtrace_t EXT_FUNC PM_PlayerTrace(vec_t *start, vec_t *end, int traceFlags, int ignore_pe)
{
	pmtrace_t tr = _PM_PlayerTrace(start, end, traceFlags, pmove->numphysent, pmove->physents, ignore_pe, NULL);
	return tr;
}

pmtrace_t EXT_FUNC PM_PlayerTraceEx(vec_t *start, vec_t *end, int traceFlags, int(*pfnIgnore)(physent_t *))
{
	pmtrace_t tr = _PM_PlayerTrace(start, end, traceFlags, pmove->numphysent, pmove->physents, -1, pfnIgnore);
	return tr;
}

struct pmtrace_s* EXT_FUNC PM_TraceLine(float *start, float *end, int flags, int usehull, int ignore_pe)
{
	int oldhull;
	static pmtrace_t tr;

#ifndef SWDS
	g_engdstAddrs.PM_TraceLine(&start, &end, &flags, &usehull, &ignore_pe);
#endif

	oldhull = pmove->usehull;
	pmove->usehull = usehull;
	if (flags != PM_TRACELINE_PHYSENTSONLY)
	{
		if (flags == PM_TRACELINE_ANYVISIBLE)
			tr = _PM_PlayerTrace(start, end, PM_NORMAL, pmove->numvisent, pmove->visents, ignore_pe, NULL);
	}
	else
	{
		tr = _PM_PlayerTrace(start, end, PM_NORMAL, pmove->numphysent, pmove->physents, ignore_pe, NULL);
	}
	pmove->usehull = oldhull;
	return &tr;
}

struct pmtrace_s* EXT_FUNC PM_TraceLineEx(float *start, float *end, int flags, int usehull, int(*pfnIgnore)(physent_t *))
{
	int oldhull;
	static pmtrace_t tr;

	oldhull = pmove->usehull;
	pmove->usehull = usehull;
	if (flags != PM_TRACELINE_PHYSENTSONLY)
	{
		tr = _PM_PlayerTrace(start, end, PM_NORMAL, pmove->numvisent, pmove->visents, -1, pfnIgnore);
	}
	else
	{
		tr = PM_PlayerTraceEx(start, end, PM_NORMAL, pfnIgnore);
	}
	pmove->usehull = oldhull;
	return &tr;
}

#ifndef REHLDS_OPT_PEDANTIC
qboolean PM_RecursiveHullCheck(hull_t *hull, int num, float p1f, float p2f, const vec_t *p1, const vec_t *p2, pmtrace_t *trace)
{
	dclipnode_t *node;
	mplane_t *plane;
	vec3_t mid;
	float pdif;
	float frac;
	float t1;
	float t2;
	float midf;

	float DIST_EPSILON = 0.03125f;

	if (num < 0)
	{
		if (num == CONTENTS_SOLID)
		{
			trace->startsolid = TRUE;
		}
		else
		{
			trace->allsolid = FALSE;
			if (num == CONTENTS_EMPTY)
			{
				trace->inopen = TRUE;
			}
			else
			{
				trace->inwater = TRUE;
			}
		}
		return TRUE;
	}

	if (hull->firstclipnode >= hull->lastclipnode)
	{
		trace->allsolid = FALSE;
		trace->inopen = TRUE;
		return TRUE;
	}

	node = &hull->clipnodes[num];
	plane = &hull->planes[node->planenum];
	if (plane->type >= 3u)
	{
		t1 = _DotProduct(p1, plane->normal) - plane->dist;
		t2 = _DotProduct(p2, plane->normal) - plane->dist;
	}
	else
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
	}
	if (t1 >= 0.0 && t2 >= 0.0)
		return PM_RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2, trace);

	if (t1 >= 0.0)
	{
		midf = t1 - DIST_EPSILON;
	}
	else
	{
		if (t2 < 0.0)
			return PM_RecursiveHullCheck(hull, node->children[1], p1f, p2f, p1, p2, trace);

		midf = t1 + DIST_EPSILON;
	}
	midf = midf / (t1 - t2);
	if (midf >= 0.0)
	{
		if (midf > 1.0)
			midf = 1.0;
	}
	else
	{
		midf = 0.0;
	}

	pdif = p2f - p1f;
	frac = pdif * midf + p1f;
	mid[0] = (p2[0] - p1[0]) * midf + p1[0];
	mid[1] = (p2[1] - p1[1]) * midf + p1[1];
	mid[2] = (p2[2] - p1[2]) * midf + p1[2];

	int side = (t1 >= 0.0) ? 0 : 1;

	if (!PM_RecursiveHullCheck(hull, node->children[side], p1f, frac, p1, mid, trace))
		return 0;

	if (PM_HullPointContents(hull, node->children[side ^ 1], mid) != CONTENTS_SOLID)
		return PM_RecursiveHullCheck(hull, node->children[side ^ 1], frac, p2f, mid, p2, trace);

	if (trace->allsolid)
		return 0;

	if (side)
	{
		trace->plane.normal[0] = vec3_origin[0] - plane->normal[0];
		trace->plane.normal[1] = vec3_origin[1] - plane->normal[1];
		trace->plane.normal[2] = vec3_origin[2] - plane->normal[2];
		trace->plane.dist = -plane->dist;
	}
	else
	{
		trace->plane.normal[0] = plane->normal[0];
		trace->plane.normal[1] = plane->normal[1];
		trace->plane.normal[2] = plane->normal[2];
		trace->plane.dist = plane->dist;
	}

	if (PM_HullPointContents(hull, hull->firstclipnode, mid) != CONTENTS_SOLID)
	{
		trace->fraction = frac;
		trace->endpos[0] = mid[0];
		trace->endpos[1] = mid[1];
		trace->endpos[2] = mid[2];
		return 0;
	}

	while (true)
	{
		midf = (float)(midf - 0.05);
		if (midf < 0.0)
			break;

		frac = pdif * midf + p1f;
		mid[0] = (p2[0] - p1[0]) * midf + p1[0];
		mid[1] = (p2[1] - p1[1]) * midf + p1[1];
		mid[2] = (p2[2] - p1[2]) * midf + p1[2];
		if (PM_HullPointContents(hull, hull->firstclipnode, mid) != CONTENTS_SOLID)
		{
			trace->fraction = frac;
			trace->endpos[0] = mid[0];
			trace->endpos[1] = mid[1];
			trace->endpos[2] = mid[2];
			return 0;
		}
	}

	trace->fraction = frac;
	trace->endpos[0] = mid[0];
	trace->endpos[1] = mid[1];
	trace->endpos[2] = mid[2];
	Con_DPrintf("Trace backed up past 0.0.\n");
	return 0;
}
#else // REHLDS_OPT_PEDANTIC
// version with unrolled tail recursion
qboolean PM_RecursiveHullCheck(hull_t *hull, int num, float p1f, float p2f, const vec_t *p1, const vec_t *p2, pmtrace_t *trace)
{
	dclipnode_t *node;
	mplane_t *plane;
	vec3_t mid;
	float pdif;
	float frac;
	float t1;
	float t2;
	float midf;
	vec3_t custom_p1; // for holding custom p1 value

	float DIST_EPSILON = 0.03125f;

	while (true)
	{
		if (num < 0)
		{
			if (num == CONTENTS_SOLID)
			{
				trace->startsolid = TRUE;
			}
			else
			{
				trace->allsolid = FALSE;
				if (num == CONTENTS_EMPTY)
				{
					trace->inopen = TRUE;
				}
				else
				{
					trace->inwater = TRUE;
				}
			}
			return TRUE;
		}

		if (hull->firstclipnode >= hull->lastclipnode)
		{
			trace->allsolid = FALSE;
			trace->inopen = TRUE;
			return TRUE;
		}

		// find the point distances
		node = &hull->clipnodes[num];
		plane = &hull->planes[node->planenum];
		if (plane->type >= 3u)
		{
			t1 = _DotProduct(p1, plane->normal) - plane->dist;
			t2 = _DotProduct(p2, plane->normal) - plane->dist;
		}
		else
		{
			t1 = p1[plane->type] - plane->dist;
			t2 = p2[plane->type] - plane->dist;
		}
		if (t1 >= 0.0 && t2 >= 0.0)
		{
			num = node->children[0]; // only 1 arg changed
			continue;
		}

		if (t1 >= 0.0)
		{
			midf = t1 - DIST_EPSILON;
		}
		else
		{
			if (t2 < 0.0)
			{
				num = node->children[1];
				continue;
			}

			midf = t1 + DIST_EPSILON;
		}
		midf = midf / (t1 - t2);
		if (midf >= 0.0)
		{
			if (midf > 1.0)
				midf = 1.0;
		}
		else
		{
			midf = 0.0;
		}

		pdif = p2f - p1f;
		frac = pdif * midf + p1f;
		mid[0] = (p2[0] - p1[0]) * midf + p1[0];
		mid[1] = (p2[1] - p1[1]) * midf + p1[1];
		mid[2] = (p2[2] - p1[2]) * midf + p1[2];

		int side = (t1 >= 0.0) ? 0 : 1;

		if (!PM_RecursiveHullCheck(hull, node->children[side], p1f, frac, p1, mid, trace))
			return 0;

		if (PM_HullPointContents(hull, node->children[side ^ 1], mid) != CONTENTS_SOLID)
		{
			num = node->children[side ^ 1];
			p1f = frac;
			p1 = custom_p1;
			custom_p1[0] = mid[0];
			custom_p1[1] = mid[1];
			custom_p1[2] = mid[2];
			continue;
		}

		if (trace->allsolid)
			return 0;

		if (side)
		{
			trace->plane.normal[0] = vec3_origin[0] - plane->normal[0];
			trace->plane.normal[1] = vec3_origin[1] - plane->normal[1];
			trace->plane.normal[2] = vec3_origin[2] - plane->normal[2];
			trace->plane.dist = -plane->dist;
		}
		else
		{
			trace->plane.normal[0] = plane->normal[0];
			trace->plane.normal[1] = plane->normal[1];
			trace->plane.normal[2] = plane->normal[2];
			trace->plane.dist = plane->dist;
		}

		if (PM_HullPointContents(hull, hull->firstclipnode, mid) != CONTENTS_SOLID)
		{
			trace->fraction = frac;
			trace->endpos[0] = mid[0];
			trace->endpos[1] = mid[1];
			trace->endpos[2] = mid[2];
			return 0;
		}

		while (true)
		{
			midf = (float)(midf - 0.05);
			if (midf < 0.0)
				break;

			frac = pdif * midf + p1f;
			mid[0] = (p2[0] - p1[0]) * midf + p1[0];
			mid[1] = (p2[1] - p1[1]) * midf + p1[1];
			mid[2] = (p2[2] - p1[2]) * midf + p1[2];
			if (PM_HullPointContents(hull, hull->firstclipnode, mid) != CONTENTS_SOLID)
			{
				trace->fraction = frac;
				trace->endpos[0] = mid[0];
				trace->endpos[1] = mid[1];
				trace->endpos[2] = mid[2];
				return 0;
			}
		}

		trace->fraction = frac;
		trace->endpos[0] = mid[0];
		trace->endpos[1] = mid[1];
		trace->endpos[2] = mid[2];
		Con_DPrintf("Trace backed up past 0.0.\n");
		return 0;
	}
}
#endif // REHLDS_OPT_PEDANTIC
