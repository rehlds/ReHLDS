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

typedef struct r_studiocache_s
{
	float frame;
	int sequence;
	vec3_t angles;
	vec3_t origin;
	vec3_t size;
	unsigned char controller[4];
	unsigned char blending[2];
	model_t *pModel;
	int nStartHull;
	int nStartPlane;
	int numhulls;
} r_studiocache_t;

studiohdr_t *pstudiohdr;

//auxvert_t auxverts[2048];
//vec_t lightvalues[2048][3];
studio_hull_hitgroup_t cache_hull_hitgroup;
studio_hull_t cache_hull;
studio_planes_t cache_planes;
rgStudioCache_t rgStudioCache;

int nCurrentHull;
int nCurrentPlane;
int r_cachecurrent;

studio_hull_t studio_hull;
studio_hull_hitgroup_t studio_hull_hitgroup;
studio_clipnodes_t studio_clipnodes;
studio_planes_t studio_planes;

bonetransform_t bonetransform;
float rotationmatrix[3][4];

cvar_t r_cachestudio = { "r_cachestudio", "1", 0, 0.0f, nullptr };
sv_blending_interface_t svBlending = { 1, SV_StudioSetupBones };
sv_blending_interface_t *g_pSvBlendingAPI = &svBlending;
server_studio_api_t server_studio_api = { Mem_Calloc, Cache_Check, COM_LoadCacheFile, Mod_Extradata };

void SV_InitStudioHull()
{
	if (studio_hull[0].planes)
		return;	// already initailized

	for (int i = 0; i < 6; i++)
	{
		int side = i & 1;
		studio_clipnodes[i].planenum = i;
		studio_clipnodes[i].children[side] = -1;
		studio_clipnodes[i].children[side ^ 1] = (i < 5) ? i + 1 : -2;
	}

	for (int i = 0; i < STUDIO_NUM_HULLS; i++)
	{
		studio_hull[i].planes = &studio_planes[i * 6];
		studio_hull[i].clipnodes = &studio_clipnodes[0];
		studio_hull[i].firstclipnode = 0;
		studio_hull[i].lastclipnode = 5;
	}
}

r_studiocache_t *R_CheckStudioCache(model_t *pModel, float frame, int sequence, const vec_t *angles, const vec_t *origin, const vec_t *size, const unsigned char *controller, const unsigned char *blending)
{
	for (int i = 0; i < STUDIO_CACHE_SIZE; i++)
	{
		r_studiocache_t *pCached = &rgStudioCache[(r_cachecurrent - i) & STUDIO_CACHEMASK];

		if (pCached->pModel == pModel && pCached->frame == frame && pCached->sequence == sequence
			&& VectorCompare(pCached->angles, angles) && VectorCompare(pCached->origin, origin) && VectorCompare(pCached->size, size)
			&& !Q_memcmp(pCached->controller, controller, 4) && !Q_memcmp(pCached->blending, blending, 2))
		{
			return pCached;
		}
	}

	return nullptr;
}

void R_AddToStudioCache(float frame, int sequence, const vec_t *angles, const vec_t *origin, const vec_t *size, const unsigned char *controller, const unsigned char *pblending, model_t *pModel, hull_t *pHulls, int numhulls)
{
	if (numhulls + nCurrentHull >= MAXSTUDIOBONES)
		R_FlushStudioCache();

	r_cachecurrent++;

	r_studiocache_t *pCache = &rgStudioCache[r_cachecurrent & STUDIO_CACHEMASK];

	pCache->frame = frame;
	pCache->sequence = sequence;

	VectorCopy(angles, pCache->angles);
	VectorCopy(origin, pCache->origin);
	VectorCopy(size, pCache->size);

	Q_memcpy(pCache->controller, controller, sizeof(pCache->controller));
	Q_memcpy(pCache->blending, pblending, sizeof(pCache->blending));

	pCache->pModel = pModel;
	pCache->nStartHull = nCurrentHull;
	pCache->nStartPlane = nCurrentPlane;

	Q_memcpy(&cache_hull[nCurrentHull], pHulls, numhulls * sizeof(hull_t));
	Q_memcpy(&cache_planes[nCurrentPlane], studio_planes, numhulls * sizeof(mplane_t) * 6);
	Q_memcpy(&cache_hull_hitgroup[nCurrentHull], studio_hull_hitgroup, numhulls * sizeof(int));

	pCache->numhulls = numhulls;

	nCurrentHull += numhulls;
	nCurrentPlane += numhulls * 6;
}

void AngleQuaternion(vec_t *angles, vec_t *quaternion)
{
	float angle;
	float sr, sp, sy, cr, cp, cy;

	// FIXME: rescale the inputs to 1/2 angle
	angle = angles[2] * 0.5f;
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[1] * 0.5f;
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[0] * 0.5f;
	sr = sin(angle);
	cr = cos(angle);

	quaternion[0] = sr*cp*cy - cr*sp*sy; // X
	quaternion[1] = cr*sp*cy + sr*cp*sy; // Y
	quaternion[2] = cr*cp*sy - sr*sp*cy; // Z
	quaternion[3] = cr*cp*cy + sr*sp*sy; // W
}

void QuaternionSlerp(vec_t *p, vec_t *q, float t, vec_t *qt)
{
	int i;
	float omega, cosom, sinom, sclp, sclq;

	// decide if one of the quaternions is backwards
	float a = 0;
	float b = 0;
	for (i = 0; i < 4; i++)
	{
		a += (p[i] - q[i])*(p[i] - q[i]);
		b += (p[i] + q[i])*(p[i] + q[i]);
	}

	if (a > b)
	{
		for (i = 0; i < 4; i++)
		{
			q[i] = -q[i];
		}
	}

	cosom = p[0] * q[0] + p[1] * q[1] + p[2] * q[2] + p[3] * q[3];

	if ((1.0 + cosom) > 0.000001)
	{
		if ((1.0 - cosom) > 0.000001)
		{
			omega = acos(cosom);
			sinom = sin(omega);
			sclp = (float)(sin((1.0 - t)*omega) / sinom);
			sclq = (float)(sin(t*omega) / sinom);
		}
		else
		{
			sclp = 1.0f - t;
			sclq = t;
		}
		for (i = 0; i < 4; i++)
		{
			qt[i] = sclp * p[i] + sclq * q[i];
		}
	}
	else
	{
		qt[0] = -p[1];
		qt[1] = p[0];
		qt[2] = -p[3];
		qt[3] = p[2];
		sclp = (float)sin((1.0 - t) * 0.5 * M_PI);
		sclq = (float)sin(t * 0.5 * M_PI);
		for (i = 0; i < 3; i++)
		{
			qt[i] = sclp * p[i] + sclq * qt[i];
		}
	}
}

void QuaternionMatrix(vec_t *quaternion, float matrix[3][4])
{
	matrix[0][0] = 1.0f - 2.0f * quaternion[1] * quaternion[1] - 2.0f * quaternion[2] * quaternion[2];
	matrix[1][0] = 2.0f * quaternion[0] * quaternion[1] + 2.0f * quaternion[3] * quaternion[2];
	matrix[2][0] = 2.0f * quaternion[0] * quaternion[2] - 2.0f * quaternion[3] * quaternion[1];

	matrix[0][1] = 2.0f * quaternion[0] * quaternion[1] - 2.0f * quaternion[3] * quaternion[2];
	matrix[1][1] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[2] * quaternion[2];
	matrix[2][1] = 2.0f * quaternion[1] * quaternion[2] + 2.0f * quaternion[3] * quaternion[0];

	matrix[0][2] = 2.0f * quaternion[0] * quaternion[2] + 2.0f * quaternion[3] * quaternion[1];
	matrix[1][2] = 2.0f * quaternion[1] * quaternion[2] - 2.0f * quaternion[3] * quaternion[0];
	matrix[2][2] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[1] * quaternion[1];
}

void R_StudioCalcBoneAdj(float dadt, float *adj, const unsigned char *pcontroller1, const unsigned char *pcontroller2, unsigned char mouthopen)
{
	int i, j;
	float value;
	mstudiobonecontroller_t *pbonecontroller;
	pbonecontroller = (mstudiobonecontroller_t *)((byte *)pstudiohdr + pstudiohdr->bonecontrollerindex);

	for (j = 0; j < pstudiohdr->numbonecontrollers; j++)
	{
		i = pbonecontroller[j].index;

		if (i >= STUDIO_MOUTH)
		{
			// mouth hardcoded at controller 4
			value = (float)(mouthopen / 64.0);
			if (value > 1.0)
				value = 1.0f;
			value = (float)((1.0 - value) * pbonecontroller[j].start + value * pbonecontroller[j].end);
			// Con_DPrintf("%d %f\n", mouthopen, value);
		}
		else
		{
			// check for 360% wrapping
			if (pbonecontroller[j].type & STUDIO_RLOOP)
			{
				if (abs(pcontroller1[i] - pcontroller2[i]) > 128)
				{
					int a, b;
					a = (pcontroller1[j] + 128) % 256;
					b = (pcontroller2[j] + 128) % 256;
					value = (float)(((a * dadt) + (b * (1 - dadt)) - 128) * (360.0 / 256.0) + pbonecontroller[j].start);
				}
				else
				{
					value = (float)((pcontroller1[i] * dadt + (pcontroller2[i]) * (1.0 - dadt)) * (360.0 / 256.0) + pbonecontroller[j].start);
				}
			}
			else
			{
				value = (float)((pcontroller1[i] * dadt + pcontroller2[i] * (1.0 - dadt)) / 255.0);
				value = clamp(value, 0.0f, 1.0f);
				value = (float)((1.0 - value) * pbonecontroller[j].start + value * pbonecontroller[j].end);
			}
			// Con_DPrintf("%d %d %f : %f\n", m_pCurrentEntity->curstate.controller[j], m_pCurrentEntity->latched.prevcontroller[j], value, dadt);
		}

		switch (pbonecontroller[j].type & STUDIO_TYPES)
		{
		case STUDIO_XR:
		case STUDIO_YR:
		case STUDIO_ZR:
			adj[j] = (float)(value * (M_PI / 180.0));
			break;
		case STUDIO_X:
		case STUDIO_Y:
		case STUDIO_Z:
			adj[j] = value;
			break;
		}
	}
}

void R_StudioCalcBoneQuaterion(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *q)
{
	int					j, k;
	vec4_t				q1, q2;
	vec3_t				angle1, angle2;
	mstudioanimvalue_t	*panimvalue;

	for (j = 0; j < 3; j++)
	{
		if (panim->offset[j + 3] == 0)
		{
			angle2[j] = angle1[j] = pbone->value[j + 3]; // default;
		}
		else
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j + 3]);
			k = frame;
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
			}

			// Bah, missing blend!
			if (panimvalue->num.valid > k)
			{
				angle1[j] = panimvalue[k + 1].value;

				if (panimvalue->num.valid > k + 1)
				{
					angle2[j] = panimvalue[k + 2].value;
				}
				else
				{
					if (panimvalue->num.total > k + 1)
						angle2[j] = angle1[j];
					else
						angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}
			else
			{
				angle1[j] = panimvalue[panimvalue->num.valid].value;
				if (panimvalue->num.total > k + 1)
				{
					angle2[j] = angle1[j];
				}
				else
				{
					angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}

			angle1[j] = pbone->value[j + 3] + angle1[j] * pbone->scale[j + 3];
			angle2[j] = pbone->value[j + 3] + angle2[j] * pbone->scale[j + 3];
		}

		if (pbone->bonecontroller[j + 3] != -1)
		{
			angle1[j] += adj[pbone->bonecontroller[j + 3]];
			angle2[j] += adj[pbone->bonecontroller[j + 3]];
		}
	}

	if (!VectorCompare(angle1, angle2))
	{
		AngleQuaternion(angle1, q1);
		AngleQuaternion(angle2, q2);
		QuaternionSlerp(q1, q2, s, q);
	}
	else
	{
		AngleQuaternion(angle1, q);
	}
}

void R_StudioCalcBonePosition(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *pos)
{
	int					j, k;
	mstudioanimvalue_t	*panimvalue;

	for (j = 0; j < 3; j++)
	{
		pos[j] = pbone->value[j]; // default;
		if (panim->offset[j] != 0)
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j]);

			k = frame;
			// find span of values that includes the frame we want
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
			}

			// if we're inside the span
			if (panimvalue->num.valid > k)
			{
				// and there's more data in the span
				if (panimvalue->num.valid > k + 1)
				{
#ifdef REHLDS_FIXES
					pos[j] += ((long double)panimvalue[k + 1].value * (1.0f - s) + s * (long double)panimvalue[k + 2].value) * pbone->scale[j];
#else // REHLDS_FIXES
					pos[j] += (panimvalue[k + 1].value * (1.0f - s) + s * panimvalue[k + 2].value) * pbone->scale[j];
#endif // REHLDS_FIXES
				}
				else
				{
#ifdef REHLDS_FIXES
					pos[j] += (long double)panimvalue[k + 1].value * pbone->scale[j];
#else // REHLDS_FIXES
					pos[j] += panimvalue[k + 1].value * pbone->scale[j];
#endif // REHLDS_FIXES
				}
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if (panimvalue->num.total <= k + 1)
				{
#ifdef REHLDS_FIXES
					pos[j] += ((long double)panimvalue[panimvalue->num.valid].value * (1.0f - s) + s * (long double)panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
#else // REHLDS_FIXES
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0f - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
#endif // REHLDS_FIXES
				}
				else
				{
#ifdef REHLDS_FIXES
					pos[j] += (long double)panimvalue[panimvalue->num.valid].value * pbone->scale[j];
#else // REHLDS_FIXES
					pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
#endif // REHLDS_FIXES
				}
			}
		}
#ifdef REHLDS_FIXES
		if (adj && pbone->bonecontroller[j] != -1)
#else
		if (pbone->bonecontroller[j] != -1)
#endif
		{
			pos[j] += adj[pbone->bonecontroller[j]];
		}
	}
}

void R_StudioSlerpBones(vec4_t *q1, vec3_t *pos1, vec4_t *q2, vec3_t *pos2, float s)
{
	int			i;
	vec4_t		q3;
	float		s1;

	if (s < 0) s = 0;
	else if (s > 1.0) s = 1.0;

	s1 = 1.0f - s;

	for (i = 0; i < pstudiohdr->numbones; i++)
	{
		QuaternionSlerp(q1[i], q2[i], s, q3);
		q1[i][0] = q3[0];
		q1[i][1] = q3[1];
		q1[i][2] = q3[2];
		q1[i][3] = q3[3];
		pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s;
		pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s;
		pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s;
	}
}

mstudioanim_t *R_GetAnim(model_t *psubmodel, mstudioseqdesc_t *pseqdesc)
{
	mstudioseqgroup_t *pseqgroup;
	cache_user_t *paSequences;

	pseqgroup = (mstudioseqgroup_t *)((char *)pstudiohdr + pstudiohdr->seqgroupindex);
	pseqgroup += pseqdesc->seqgroup;
	if (!pseqdesc->seqgroup)
		return (mstudioanim_t *)((char *)pstudiohdr + pseqdesc->animindex);

	paSequences = (cache_user_t *)psubmodel->submodels;
	if (!paSequences)
	{
		paSequences = (cache_user_t *)Mem_Calloc(MAXSTUDIOGROUPS, sizeof(cache_user_t));
		psubmodel->submodels = (dmodel_t *)paSequences;
	}

	if (!Cache_Check(&paSequences[pseqdesc->seqgroup]))
	{
		Con_DPrintf("loading %s\n", pseqgroup->name);
		COM_LoadCacheFile(pseqgroup->name, &paSequences[pseqdesc->seqgroup]);
	}

	return (mstudioanim_t *)((char *)paSequences[pseqdesc->seqgroup].data + pseqdesc->animindex);
}

void EXT_FUNC SV_StudioSetupBones(model_t *pModel, float frame, int sequence, const vec_t *angles, const vec_t *origin, const unsigned char *pcontroller, const unsigned char *pblending, int iBone, const edict_t *edict)
{
	static vec4_t q1[128];
	static vec3_t pos1[128];
	static vec4_t q2[128];
	static vec3_t pos2[128];

	int chainlength;
	mstudiobone_t *pbones;
	mstudioseqdesc_t *pseqdesc;
	int chain[128];
	float bonematrix[3][4];
	mstudioanim_t *panim;
	float f;
	float adj[8];
	float s;

	chainlength = 0;
	if (sequence < 0 || sequence >= pstudiohdr->numseq)
	{
		Con_DPrintf("SV_StudioSetupBones:  sequence %i/%i out of range for model %s\n", sequence, pstudiohdr->numseq, pstudiohdr->name);
		sequence = 0;
	}

	pbones = (mstudiobone_t *)((char *)pstudiohdr + pstudiohdr->boneindex);
	pseqdesc = (mstudioseqdesc_t *)((char *)pstudiohdr + pstudiohdr->seqindex);
	pseqdesc += sequence;
	panim = R_GetAnim(pModel, pseqdesc);

	if (iBone < -1 || iBone >= pstudiohdr->numbones)
		iBone = 0;

	if (iBone != -1)
	{
		do
		{
			chain[chainlength++] = iBone;
			iBone = pbones[iBone].parent;
		} while (iBone != -1);
	}
	else
	{
		chainlength = pstudiohdr->numbones;
		for (int i = 0; i < pstudiohdr->numbones; i++)
		{
			chain[pstudiohdr->numbones - 1 - i] = i;
		}
	}

	f = (pseqdesc->numframes > 1) ? (pseqdesc->numframes - 1) * frame / 256.0f : 0.0f;

	R_StudioCalcBoneAdj(0.0, adj, pcontroller, pcontroller, 0);
	s = f - (float)(int)f;

	for (int i = chainlength - 1; i >= 0; i--)
	{
		int bone = chain[i];
		R_StudioCalcBoneQuaterion((int)f, s, &pbones[bone], &panim[bone], adj, q1[bone]);
		R_StudioCalcBonePosition((int)f, s, &pbones[bone], &panim[bone], adj, pos1[bone]);

	}

	if (pseqdesc->numblends > 1)
	{
		panim = R_GetAnim(pModel, pseqdesc);
		panim += pstudiohdr->numbones;

		for (int i = chainlength - 1; i >= 0; i--)
		{
			int bone = chain[i];
			R_StudioCalcBoneQuaterion((int)f, s, &pbones[bone], &panim[bone], adj, q2[bone]);
			R_StudioCalcBonePosition((int)f, s, &pbones[bone], &panim[bone], adj, pos2[bone]);
		}

		R_StudioSlerpBones(q1, pos1, q2, pos2, *pblending / 255.0f);
	}

	AngleMatrix(angles, rotationmatrix);
	rotationmatrix[0][3] = origin[0];
	rotationmatrix[1][3] = origin[1];
	rotationmatrix[2][3] = origin[2];
	for (int i = chainlength - 1; i >= 0; i--)
	{
		int bone = chain[i];
		int parent = pbones[bone].parent;
		QuaternionMatrix(q1[bone], bonematrix);

		bonematrix[0][3] = pos1[bone][0];
		bonematrix[1][3] = pos1[bone][1];
		bonematrix[2][3] = pos1[bone][2];
		R_ConcatTransforms((parent == -1) ? rotationmatrix : bonetransform[parent], bonematrix, bonetransform[bone]);
	}
}

void SV_SetStudioHullPlane(mplane_t *pplane, int iBone, int k, float dist)
{
	pplane->type = 5;

	pplane->normal[0] = bonetransform[iBone][0][k];
	pplane->normal[1] = bonetransform[iBone][1][k];
	pplane->normal[2] = bonetransform[iBone][2][k];

	pplane->dist = pplane->normal[2] * bonetransform[iBone][2][3]
		+ pplane->normal[1] * bonetransform[iBone][1][3]
		+ pplane->normal[0] * bonetransform[iBone][0][3]
		+ dist;
}

hull_t *R_StudioHull(model_t *pModel, float frame, int sequence, const vec_t *angles, const vec_t *origin, const vec_t *size, const unsigned char *pcontroller, const unsigned char *pblending, int *pNumHulls, const edict_t *pEdict, int bSkipShield)
{
	SV_InitStudioHull();

	if (r_cachestudio.value != 0)
	{
		r_studiocache_t *pCached = R_CheckStudioCache(pModel, frame, sequence, angles, origin, size, pcontroller, pblending);

		if (pCached)
		{
			Q_memcpy(studio_planes, &cache_planes[pCached->nStartPlane], pCached->numhulls * sizeof(mplane_t) * 6);
			Q_memcpy(studio_hull_hitgroup, &cache_hull_hitgroup[pCached->nStartHull], pCached->numhulls * sizeof(int));
			Q_memcpy(studio_hull, &cache_hull[pCached->nStartHull], pCached->numhulls * sizeof(hull_t));

			*pNumHulls = pCached->numhulls;
			return studio_hull;
		}
	}

	pstudiohdr = (studiohdr_t *)Mod_Extradata(pModel);

	vec_t angles2[3] = { -angles[0], angles[1], angles[2] };
	g_pSvBlendingAPI->SV_StudioSetupBones(pModel, frame, sequence, angles2, origin, pcontroller, pblending, -1, pEdict);

#ifdef REHLDS_FIXES
	const int hitboxShieldIndex = 20;
#else
	// NOTE: numhitboxes range [0,21], so index 21 it's unreachable code for loop
	const int hitboxShieldIndex = 21;
#endif

	mstudiobbox_t *pbbox = (mstudiobbox_t *)((char *)pstudiohdr + pstudiohdr->hitboxindex);
	for (int i = 0; i < pstudiohdr->numhitboxes; i++)
	{
		if (bSkipShield && i == hitboxShieldIndex) continue;

		studio_hull_hitgroup[i] = pbbox[i].group;

		for (int j = 0; j < 3; j++)
		{
			mplane_t *plane0 = &studio_planes[i * 6 + j * 2 + 0];
			mplane_t *plane1 = &studio_planes[i * 6 + j * 2 + 1];
			SV_SetStudioHullPlane(plane0, pbbox[i].bone, j, pbbox[i].bbmax[j]);
			SV_SetStudioHullPlane(plane1, pbbox[i].bone, j, pbbox[i].bbmin[j]);

			plane0->dist += fabs(plane0->normal[0] * size[0]) + fabs(plane0->normal[1] * size[1]) + fabs(plane0->normal[2] * size[2]);
			plane1->dist -= fabs(plane1->normal[0] * size[0]) + fabs(plane1->normal[1] * size[1]) + fabs(plane1->normal[2] * size[2]);
		}
	}

	*pNumHulls = (bSkipShield == 1) ? pstudiohdr->numhitboxes - 1 : pstudiohdr->numhitboxes;
	if (r_cachestudio.value != 0)
	{
		R_AddToStudioCache(frame, sequence, angles, origin, size, pcontroller, pblending, pModel, studio_hull, *pNumHulls);
	}

	return &studio_hull[0];
}

int SV_HitgroupForStudioHull(int index)
{
	return studio_hull_hitgroup[index];
}

void R_InitStudioCache()
{
	Q_memset(rgStudioCache, 0, sizeof(rgStudioCache));

	r_cachecurrent = 0;
	nCurrentHull = 0;
	nCurrentPlane = 0;
}

void R_FlushStudioCache()
{
	R_InitStudioCache();
}

int R_StudioBodyVariations(model_t *model)
{
	if (model->type != mod_studio)
		return 0;

	studiohdr_t *shdr = (studiohdr_t *)Mod_Extradata(model);
	if (!shdr)
		return 0;

	int count = 1;
	mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)((char *)shdr + shdr->bodypartindex);
	for (int i = 0; i < shdr->numbodyparts; i++, pbodypart++)
	{
		count *= pbodypart->nummodels;
	}

	return count;
}

void R_StudioPlayerBlend(mstudioseqdesc_t *pseqdesc, int *pBlend, float *pPitch)
{
	*pBlend = (int)(*pPitch * 3.0);
	if (*pBlend < pseqdesc->blendstart[0])
	{
		*pBlend = 0;
		*pPitch -= (float)(pseqdesc->blendstart[0] / 3.0);
	}
	else if (*pBlend <= pseqdesc->blendend[0])
	{
		float blendDiff = pseqdesc->blendend[0] - pseqdesc->blendstart[0];
		if (blendDiff >= 0.1)
		{
			*pBlend = (int)((*pBlend - pseqdesc->blendstart[0]) * 255.0 / blendDiff);
			*pPitch = 0;
		}
		else
		{
			*pBlend = 127;
			*pPitch = 0;
		}
	}
	else
	{
		*pBlend = 255;
		*pPitch = (float)(*pPitch - pseqdesc->blendend[0] / 3.0);
	}
}

hull_t *SV_HullForStudioModel(const edict_t *pEdict, const vec_t *mins, const vec_t *maxs, vec_t *offset, int *pNumHulls)
{
	qboolean useComplexHull = FALSE;
	vec3_t size;
	float factor = 0.5;
	int bSkipShield = 0;
	size[0] = maxs[0] - mins[0];
	size[1] = maxs[1] - mins[1];
	size[2] = maxs[2] - mins[2];
	if (VectorCompare(vec3_origin, size))
	{
		if (!(gGlobalVariables.trace_flags & FTRACE_SIMPLEBOX))
		{
			useComplexHull = TRUE;
			if (pEdict->v.flags & FL_CLIENT)
			{
				if (sv_clienttrace.value == 0.0)
				{
					useComplexHull = FALSE;
				}
				else
				{
					size[2] = 1.0f;
					size[1] = 1.0f;
					size[0] = 1.0f;
					factor = sv_clienttrace.value * 0.5f;
				}
			}
		}
	}
	if (pEdict->v.gamestate == 1 && (g_eGameType == GT_TerrorStrike || g_eGameType == GT_CStrike || g_eGameType == GT_CZero))
		bSkipShield = 1;

	if ((g_psv.models[pEdict->v.modelindex]->flags & STUDIO_TRACE_HITBOX) || useComplexHull == TRUE)
	{
		VectorScale(size, factor, size);
		offset[0] = 0;
		offset[1] = 0;
		offset[2] = 0;
		if (pEdict->v.flags & FL_CLIENT)
		{
			pstudiohdr = (studiohdr_t *)Mod_Extradata(g_psv.models[pEdict->v.modelindex]);

			mstudioseqdesc_t* pseqdesc = (mstudioseqdesc_t *)((char*)pstudiohdr + pstudiohdr->seqindex);
			pseqdesc += pEdict->v.sequence;

			vec3_t angles;
			angles[0] = pEdict->v.angles[0];
			angles[1] = pEdict->v.angles[1];
			angles[2] = pEdict->v.angles[2];

			int iBlend;
			R_StudioPlayerBlend(pseqdesc, &iBlend, angles);

			unsigned char blending[2] = { (unsigned char)iBlend, 0 };
			unsigned char controller[4] = { 0x7F, 0x7F, 0x7F, 0x7F };
			return R_StudioHull(
				g_psv.models[pEdict->v.modelindex],
				pEdict->v.frame,
				pEdict->v.sequence,
				angles,
				pEdict->v.origin,
				size,
				controller,
				blending,
				pNumHulls,
				pEdict,
				bSkipShield);
		}
		else
		{
			return R_StudioHull(
				g_psv.models[pEdict->v.modelindex],
				pEdict->v.frame,
				pEdict->v.sequence,
				pEdict->v.angles,
				pEdict->v.origin,
				size,
				pEdict->v.controller,
				pEdict->v.blending,
				pNumHulls,
				pEdict,
				bSkipShield);
		}
	}
	else
	{
		*pNumHulls = 1;
		return SV_HullForEntity((edict_t *)pEdict, mins, maxs, offset);
	}
}

qboolean DoesSphereIntersect(float *vSphereCenter, float fSphereRadiusSquared, float *vLinePt, float *vLineDir)
{
	vec3_t P;
	float a;
	float b;
	float c;
	float insideSqr;

	P[0] = *vLinePt - *vSphereCenter;
	P[1] = vLinePt[1] - vSphereCenter[1];
	P[2] = vLinePt[2] - vSphereCenter[2];
	a = vLineDir[0] * vLineDir[0] + vLineDir[1] * vLineDir[1] + vLineDir[2] * vLineDir[2];
	b = P[2] * vLineDir[2] + P[1] * vLineDir[1] + P[0] * vLineDir[0];
	b *= 2;
	c = P[2] * P[2] + P[1] * P[1] + P[0] * P[0] - fSphereRadiusSquared;
	insideSqr = b * b - c * a * 4.0f;
	return insideSqr > 0.000001;
}

qboolean SV_CheckSphereIntersection(edict_t *ent, const vec_t *start, const vec_t *end)
{
	studiohdr_t *studiohdr;
	mstudioseqdesc_t *pseqdesc;

	vec3_t traceOrg;
	vec3_t traceDir;
	float radiusSquared;
	vec3_t maxDim;

	radiusSquared = 0.0f;
	if (!(ent->v.flags & FL_CLIENT))
		return 1;

	traceOrg[0] = start[0];
	traceOrg[1] = start[1];
	traceOrg[2] = start[2];
	traceDir[0] = end[0] - start[0];
	traceDir[1] = end[1] - start[1];
	traceDir[2] = end[2] - start[2];

	studiohdr = (studiohdr_t *)Mod_Extradata(g_psv.models[ent->v.modelindex]);
	pseqdesc = (mstudioseqdesc_t *)((char *)studiohdr + studiohdr->seqindex);
	pseqdesc += ent->v.sequence;
	for (int i = 0; i < 3; i++)
	{
		maxDim[i] = Q_max(fabs(pseqdesc->bbmax[i]), fabs(pseqdesc->bbmin[i]));
	}
	radiusSquared = maxDim[0] * maxDim[0] + maxDim[1] * maxDim[1] + maxDim[2] * maxDim[2];
	return DoesSphereIntersect(ent->v.origin, radiusSquared, traceOrg, traceDir) != 0;
}

void EXT_FUNC AnimationAutomove(const edict_t *pEdict, float flTime)
{
}

void EXT_FUNC GetBonePosition(const edict_t *pEdict, int iBone, float *rgflOrigin, float *rgflAngles)
{
	pstudiohdr = (studiohdr_t *)Mod_Extradata(g_psv.models[pEdict->v.modelindex]);

#ifdef REHLDS_FIXES
	if (!pstudiohdr)
		return;

	if (iBone < 0 || iBone >= pstudiohdr->numbones)
		return; // invalid bone
#endif

	g_pSvBlendingAPI->SV_StudioSetupBones(
		g_psv.models[pEdict->v.modelindex],
		pEdict->v.frame,
		pEdict->v.sequence,
		pEdict->v.angles,
		pEdict->v.origin,
		pEdict->v.controller,
		pEdict->v.blending,
		iBone,
		pEdict
	);

	if (rgflOrigin)
	{
		rgflOrigin[0] = bonetransform[iBone][0][3];
		rgflOrigin[1] = bonetransform[iBone][1][3];
		rgflOrigin[2] = bonetransform[iBone][2][3];
	}
}

void EXT_FUNC GetAttachment(const edict_t *pEdict, int iAttachment, float *rgflOrigin, float *rgflAngles)
{
	mstudioattachment_t *pattachment;
	vec3_t angles;

	pstudiohdr = (studiohdr_t *)Mod_Extradata(g_psv.models[pEdict->v.modelindex]);

#ifdef REHLDS_FIXES
	if (!pstudiohdr)
		return;

	if (iAttachment < 0 || iAttachment >= pstudiohdr->numattachments)
		return; // invalid attachment
#endif

	pattachment = (mstudioattachment_t *)((char *)pstudiohdr + pstudiohdr->attachmentindex);
	pattachment += iAttachment;

	angles[0] = -pEdict->v.angles[0];
	angles[1] = pEdict->v.angles[1];
	angles[2] = pEdict->v.angles[2];

	g_pSvBlendingAPI->SV_StudioSetupBones(
		g_psv.models[pEdict->v.modelindex],
		pEdict->v.frame,
		pEdict->v.sequence,
		angles,
		pEdict->v.origin,
		pEdict->v.controller,
		pEdict->v.blending,
		pattachment->bone,
		pEdict
	);

	if (rgflOrigin)
		VectorTransform(pattachment->org, bonetransform[pattachment->bone], rgflOrigin);
}

int ModelFrameCount(model_t *model)
{
	int count;

	if (!model)
		return 1;

	switch (model->type)
	{
	case mod_sprite:
		count = ((msprite_t*)model->cache.data)->numframes;
		break;

	case mod_studio:
		count = R_StudioBodyVariations(model);
		break;

	default:
		return 1;
	}

	if (count < 1)
		return 1;

	return count;
}

void R_StudioBoundVertex(vec_t *mins, vec_t *maxs, int *vertcount, const vec_t *point)
{
	if (*vertcount)
	{
		for (int i = 0; i < 3; i++)
		{
			if (mins[i] > point[i])
				mins[i] = point[i];

			if (maxs[i] < point[i])
				maxs[i] = point[i];
		}
	}
	else
	{
		for (int i = 0; i < 3; i++)
		{
			mins[i] = point[i];
			maxs[i] = point[i];
		}
	}

	++(*vertcount);
}

void R_StudioBoundBone(vec_t *mins, vec_t *maxs, int *bonecount, const vec_t *point)
{
	if (*bonecount)
	{
		for (int i = 0; i < 3; i++)
		{
			if (mins[i] > point[i])
				mins[i] = point[i];

			if (maxs[i] < point[i])
				maxs[i] = point[i];
		}
	}
	else
	{
		for (int i = 0; i < 3; i++)
		{
			mins[i] = point[i];
			maxs[i] = point[i];
		}
	}

	++(*bonecount);
}

void R_StudioAccumulateBoneVerts(vec_t *mins, vec_t *maxs, int *vertcount, vec_t *bone_mins, vec_t *bone_maxs, int *bonecount)
{
	vec3_t delta;
	vec3_t point;

	if (*bonecount > 0)
	{
		delta[0] = bone_maxs[0] - bone_mins[0];
		delta[1] = bone_maxs[1] - bone_mins[1];
		delta[2] = bone_maxs[2] - bone_mins[2];
		VectorScale(delta, 0.5, point);
		R_StudioBoundVertex(mins, maxs, vertcount, point);
		VectorScale(delta, -0.5, point);
		R_StudioBoundVertex(mins, maxs, vertcount, point);

		for (int i = 0; i < 3; i++)
			bone_mins[i] = bone_maxs[i] = 0;

		*bonecount = 0;
	}
}

int R_StudioComputeBounds(unsigned char *pBuffer, float *mins, float *maxs)
{
	vec3_t bone_mins;
	vec3_t bone_maxs;
	vec3_t vert_mins;
	vec3_t vert_maxs;

	for (int i = 0; i < 3; i++)
		bone_mins[i] = bone_maxs[i] = vert_mins[i] = vert_maxs[i] = 0;

	studiohdr_t * header = (studiohdr_t *)pBuffer;
	mstudiobodyparts_t * bodyparts = (mstudiobodyparts_t *)((char *)header + header->bodypartindex);

	int nummodels = 0;
	for (int i = 0; i < header->numbodyparts; i++)
		nummodels += bodyparts[i].nummodels;

	int vert_count = 0;
	mstudiomodel_t *pmodel = (mstudiomodel_t *)&bodyparts[header->numbodyparts];
	for (int model = 0; model < nummodels; model++, pmodel++)
	{
		int num_verts = pmodel->numverts;
		vec3_t *pverts = (vec3_t *)((char *)header + pmodel->vertindex);
		for (int v = 0; v < num_verts; v++)
			R_StudioBoundVertex(vert_mins, vert_maxs, &vert_count, pverts[v]);
	}

	int bone_count = 0;
	mstudiobone_t* pbones = (mstudiobone_t*)((char *)header + header->boneindex);
	mstudioseqdesc_t *pseq = (mstudioseqdesc_t *)((char *)header + header->seqindex);
	for (int sequence = 0; sequence < header->numseq; sequence++, pseq++)
	{
		int num_frames = pseq->numframes;
		mstudioanim_t *panim = (mstudioanim_t *)((char *)header + pseq->animindex);
		for (int bone = 0; bone < header->numbones; bone++)
		{
			for (int f = 0; f < num_frames; ++f)
			{
				vec3_t bonepos;
				R_StudioCalcBonePosition(f, 0.0, &pbones[bone], panim, NULL, bonepos);
				R_StudioBoundBone(bone_mins, bone_maxs, &bone_count, bonepos);
			}
		}

		R_StudioAccumulateBoneVerts(vert_mins, vert_maxs, &vert_count, bone_mins, bone_maxs, &bone_count);
	}

	for (int i = 0; i < 3; i++)
	{
		mins[i] = vert_mins[i];
		maxs[i] = vert_maxs[i];
	}

	return 1;
}

int R_GetStudioBounds(const char *filename, float *mins, float *maxs)
{
	int iret = 0;
	qboolean usingReadBuffer = 0;

	for (int i = 0; i < 3; i++)
		mins[0] = maxs[0] = vec3_origin[i];

	if (!Q_strstr(filename, "models") || !Q_strstr(filename, ".mdl"))
		return 0;

	FileHandle_t fp = FS_Open(filename, "rb");
	if (!fp)
		return 0;

	int length;
	char *pBuffer = (char *)FS_GetReadBuffer(fp, &length);
	if (pBuffer)
		usingReadBuffer = 1;
	else
		pBuffer = (char*) COM_LoadFile(filename, 5, 0);

	if (pBuffer)
	{
		if (LittleLong(*(unsigned int *)pBuffer) == IDSTUDIOHEADER)
			iret = R_StudioComputeBounds((unsigned char*)pBuffer, mins, maxs);
#ifndef REHLDS_FIXES
		// wrong release memory code
		else
			COM_FreeFile(pBuffer);
#endif
	}

	if (usingReadBuffer)
		FS_ReleaseReadBuffer(fp, pBuffer);
	else
		COM_FreeFile(pBuffer);

	FS_Close(fp);

	return iret;
}

void R_ResetSvBlending()
{
	g_pSvBlendingAPI = &svBlending;
}
