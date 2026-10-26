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

#include "studio.h"
#include "model.h"
#include "r_studioint.h"
#include "cvardef.h"

typedef struct r_studiocache_s r_studiocache_t;

typedef hull_t studio_hull_t[STUDIO_NUM_HULLS];
typedef int studio_hull_hitgroup_t[STUDIO_NUM_HULLS];
typedef dclipnode_t studio_clipnodes_t[6];
typedef mplane_t studio_planes_t[STUDIO_NUM_PLANES];
typedef r_studiocache_t rgStudioCache_t[STUDIO_CACHE_SIZE];
typedef float bonetransform_t[STUDIO_NUM_HULLS][3][4];

extern studiohdr_t *pstudiohdr;

extern studio_hull_hitgroup_t cache_hull_hitgroup;
extern studio_hull_t cache_hull;
extern studio_planes_t cache_planes;

extern int r_cachecurrent;
extern int nCurrentHull;
extern int nCurrentPlane;

extern studio_hull_t studio_hull;
extern studio_hull_hitgroup_t studio_hull_hitgroup;
extern studio_clipnodes_t studio_clipnodes;
extern studio_planes_t studio_planes;
extern rgStudioCache_t rgStudioCache;

extern bonetransform_t bonetransform;
extern sv_blending_interface_t *g_pSvBlendingAPI;
extern sv_blending_interface_t svBlending;
extern server_studio_api_t server_studio_api;
extern cvar_t r_cachestudio;
extern float rotationmatrix[3][4];

void SV_InitStudioHull();
r_studiocache_t *R_CheckStudioCache(model_t *pModel, float frame, int sequence, const vec_t *angles, const vec_t *origin, const vec_t *size, const unsigned char *controller, const unsigned char *blending);
void R_AddToStudioCache(float frame, int sequence, const vec_t *angles, const vec_t *origin, const vec_t *size, const unsigned char *controller, const unsigned char *pblending, model_t *pModel, hull_t *pHulls, int numhulls);
void AngleQuaternion(vec_t *angles, vec_t *quaternion);
void QuaternionSlerp(vec_t *p, vec_t *q, float t, vec_t *qt);
void QuaternionMatrix(vec_t *quaternion, float matrix[3][4]);
void R_StudioCalcBoneAdj(float dadt, float *adj, const unsigned char *pcontroller1, const unsigned char *pcontroller2, unsigned char mouthopen);
void R_StudioCalcBoneQuaterion(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *q);
void R_StudioCalcBonePosition(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *pos);
void R_StudioSlerpBones(vec4_t *q1, vec3_t *pos1, vec4_t *q2, vec3_t *pos2, float s);
mstudioanim_t *R_GetAnim(model_t *psubmodel, mstudioseqdesc_t *pseqdesc);
void SV_StudioSetupBones(model_t *pModel, float frame, int sequence, const vec_t *angles, const vec_t *origin, const unsigned char *pcontroller, const unsigned char *pblending, int iBone, const edict_t *edict);
void SV_StudioSetupBones_internal(model_t *pModel, float frame, int sequence, const vec_t *angles, const vec_t *origin, const unsigned char *pcontroller, const unsigned char *pblending, int iBone, const edict_t *edict);
void SV_BlendingSetupBones(model_t *pModel, float frame, int sequence, const vec_t *angles, const vec_t *origin, const unsigned char *pcontroller, const unsigned char *pblending, int iBone, const edict_t *edict);
int SV_StudioShieldHitbox();
bool SV_StudioSkipsShield(const edict_t *pEdict);

// Sets up the bones of the entity in the pose traces take against it, through the hooks of modules;
// the header of its studio model, or NULL
studiohdr_t *SV_StudioSetupTraceBones(const edict_t *pEdict);

void SV_SetStudioHullPlane(mplane_t *pplane, int iBone, int k, float dist);
hull_t *R_StudioHull(model_t *pModel, float frame, int sequence, const vec_t *angles, const vec_t *origin, const vec_t *size, const unsigned char *pcontroller, const unsigned char *pblending, int *pNumHulls, const edict_t *pEdict, int bSkipShield);
int SV_HitgroupForStudioHull(int index);
void R_InitStudioCache();
void R_FlushStudioCache();
int R_StudioBodyVariations(model_t *model);
void R_StudioPlayerBlend(mstudioseqdesc_t *pseqdesc, int *pBlend, float *pPitch);
hull_t *SV_HullForStudioModel(const edict_t *pEdict, const vec_t *mins, const vec_t *maxs, vec_t *offset, int *pNumHulls);
qboolean DoesSphereIntersect(float *vSphereCenter, float fSphereRadiusSquared, float *vLinePt, float *vLineDir);
qboolean SV_CheckSphereIntersection(edict_t *ent, const vec_t *start, const vec_t *end);
void AnimationAutomove(const edict_t *pEdict, float flTime);
void GetBonePosition(const edict_t *pEdict, int iBone, float *rgflOrigin, float *rgflAngles);
void GetAttachment(const edict_t *pEdict, int iAttachment, float *rgflOrigin, float *rgflAngles);
int ModelFrameCount(model_t *model);
void R_StudioBoundVertex(vec_t *mins, vec_t *maxs, int *vertcount, const vec_t *point);
void R_StudioBoundBone(vec_t *mins, vec_t *maxs, int *bonecount, const vec_t *point);
void R_StudioAccumulateBoneVerts(vec_t *mins, vec_t *maxs, int *vertcount, vec_t *bone_mins, vec_t *bone_maxs, int *bonecount);
int R_StudioComputeBounds(unsigned char *pBuffer, float *mins, float *maxs);
int R_GetStudioBounds(const char *filename, float *mins, float *maxs);
void R_ResetSvBlending();
