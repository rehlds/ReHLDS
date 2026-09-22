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
#include "model.h"

typedef struct areanode_s
{
	int axis;
	float dist;
	struct areanode_s *children[2];
	link_t trigger_edicts;
	link_t solid_edicts;
} areanode_t;

const int AREA_DEPTH = 4;
const int AREA_NODES = 32;

typedef struct moveclip_s	// TODO: Move it to world.cpp someday
{
	vec3_t boxmins;
	vec3_t boxmaxs;
	const float *mins;
	const float *maxs;
	vec3_t mins2;
	vec3_t maxs2;
	const float *start;
	const float *end;
	trace_t trace;
	short int type;
	short int ignoretrans;
	edict_t *passedict;
	qboolean monsterClipBrush;
} moveclip_t;

#define CONTENTS_NONE       0 // no custom contents specified

#define MOVE_NORMAL         0 // normal trace
#define MOVE_NOMONSTERS     1 // ignore monsters (edicts with flags (FL_MONSTER|FL_FAKECLIENT|FL_CLIENT) set)
#define MOVE_MISSILE        2 // extra size for monsters

#define FMOVE_IGNORE_GLASS  0x100
#define FMOVE_SIMPLEBOX     0x200

typedef dclipnode_t box_clipnodes_t[6];
typedef mplane_t box_planes_t[6];
typedef mplane_t beam_planes_t[6];

extern hull_t box_hull;
extern hull_t beam_hull;
extern box_clipnodes_t box_clipnodes;
extern box_planes_t box_planes;
extern beam_planes_t beam_planes;
extern areanode_t sv_areanodes[AREA_NODES];
extern int sv_numareanodes;

extern cvar_t sv_force_ent_intersection;

void ClearLink(link_t *l);
void RemoveLink(link_t *l);
void InsertLinkBefore(link_t *l, link_t *before);
NOXREF void InsertLinkAfter(link_t *l, link_t *after);
void SV_InitBoxHull();
hull_t *SV_HullForBox(const vec_t *mins, const vec_t *maxs);
NOXREF hull_t *SV_HullForBeam(const vec_t *start, const vec_t *end, const vec_t *size);
hull_t *SV_HullForBsp(edict_t *ent, const vec_t *mins, const vec_t *maxs, vec_t *offset);
hull_t *SV_HullForEntity(edict_t *ent, const vec_t *mins, const vec_t *maxs, vec_t *offset);
areanode_t *SV_CreateAreaNode(int depth, vec_t *mins, vec_t *maxs);
void SV_ClearWorld();
void SV_UnlinkEdict(edict_t *ent);
void SV_TouchLinks(edict_t *ent, areanode_t *node);
void SV_FindTouchedLeafs(edict_t *ent, mnode_t *node, int *topnode);
void SV_LinkEdict(edict_t *ent, qboolean touch_triggers);
int SV_HullPointContents(hull_t *hull, int num, const vec_t *p);
int SV_LinkContents(areanode_t *node, const vec_t *pos);
int SV_PointContents(const vec_t *p);
edict_t *SV_TestEntityPosition(edict_t *ent);
edict_t *SV_TestEntityPosition_internal(edict_t *ent);
qboolean SV_RecursiveHullCheck(hull_t *hull, int num, float p1f, float p2f, const vec_t *p1, const vec_t *p2, trace_t *trace);
void SV_SingleClipMoveToEntity(edict_t *ent, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, trace_t *trace);
trace_t SV_ClipMoveToEntity(edict_t *ent, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end);
void SV_ClipToLinks(areanode_t *node, moveclip_t *clip);
void SV_ClipToWorldbrush(areanode_t *node, moveclip_t *clip);
void SV_MoveBounds(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, vec_t *boxmins, vec_t *boxmaxs);
trace_t SV_MoveNoEnts(const vec_t *start, vec_t *mins, vec_t *maxs, const vec_t *end, int type, edict_t *passedict);
trace_t SV_Move(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int type, edict_t *passedict, qboolean monsterClipBrush);

#ifdef REHLDS_OPT_PEDANTIC
trace_t SV_Move_Point(const vec_t *start, const vec_t *end, int type, edict_t *passedict);
#endif // REHLDS_OPT_PEDANTIC
