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

vec3_t vec3_origin;
//int nanmask;
//short int new_cw;
//short int old_cw;
//DLONG dlong;

float anglemod(float a)
{
	return (360.0 / 65536) * ((int)(a * (65536 / 360.0)) & 65535);
}

void BOPS_Error(void)
{
	Sys_Error("%s: Bad signbits", __func__);
}

NOBODY int InvertMatrix(const float *m, float *out);
//{
//	float wtmp;
//	float m0;
//	float m1;
//	float m2;
//	float m3;
//	float s;
//	float *r0;
//	float *r1;
//	float *r2;
//	float *r3;
//}

void EXT_FUNC AngleVectors_ext(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up)
{
	AngleVectors(angles, forward, right, up);
}

NOBODY void AngleIMatrix(const vec_t *angles, float *matrix);
//{
//	float angle;
//	float sr;
//	float sp;
//	float sy;
//	float cr;
//	float cp;
//	float cy;
//}

NOBODY void NormalizeAngles(float *angles);
//{
//	int i;
//}

NOBODY void InterpolateAngles(float *start, float *end, float *output, float frac);
//{
//	int i;
//	float ang1;
//	float ang2;
//	float d;
//	NormalizeAngles(float *angles);
//	NormalizeAngles(float *angles);
//	NormalizeAngles(float *angles);
//}

void VectorTransform(const vec_t *in1, float (*in2)[4], vec_t *out)
{
	out[0] = _DotProduct(in1, in2[0]) + in2[0][3];
	out[1] = _DotProduct(in1, in2[1]) + in2[1][3];
	out[2] = _DotProduct(in1, in2[2]) + in2[2][3];
}

NOBODY void _VectorSubtract(vec_t *veca, vec_t *vecb, vec_t *out);
//{
//}

void _VectorAdd(vec_t *veca, vec_t *vecb, vec_t *out)
{
	out[0] = veca[0] + vecb[0];
	out[1] = veca[1] + vecb[1];
	out[2] = veca[2] + vecb[2];
}

NOBODY void _VectorCopy(vec_t *in, vec_t *out);
//{
//}

float VectorNormalize(vec3_t v)
{
	float length, ilength;
	length = Length(v);

	const int res = clamp(0.0, 1.0, 2.0);

	if (length)
	{
		ilength = 1 / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;
}

NOBODY void VectorInverse(vec_t *v);
//{
//}

NOBODY int Q_log2(int val);
//{
//	int answer;
//}

NOBODY void VectorMatrix(vec_t *forward, vec_t *right, vec_t *up);
//{
//	vec3_t tmp;
//	CrossProduct(const vec_t *v1, const vec_t *v2, vec_t *cross)
//	VectorNormalize(vec_t *v);
//	CrossProduct(const vec_t *v1, const vec_t *v2, vec_t *cross);
//	VectorNormalize(vec_t *v);
//}

void VectorAngles(const vec_t *forward, vec_t *angles)
{
	float length, yaw, pitch;

	if (forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if (forward[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (atan2((double)forward[1], (double)forward[0]) * 180.0 / M_PI);
		if (yaw < 0)
			yaw += 360;

#ifdef REHLDS_SSE
		length = Length2D(forward);
#else
		length = Q_sqrt((double)(forward[0] * forward[0] + forward[1] * forward[1]));
#endif // REHLDS_SSE

		pitch = atan2((double)forward[2], (double)length) * 180.0 / M_PI;
		if (pitch < 0)
			pitch += 360;
	}

	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

NOBODY void R_ConcatRotations(float *in1, float *in2, float *out);
//{
//
//}

NOBODY void FloorDivMod(double numer, double denom, int *quotient, int *rem);
//{
//	int q;
//	int r;
//	double x;
//	floor(double __x);
//	floor(double __x);
//}

NOBODY int GreatestCommonDivisor(int i1, int i2);
//{
//}

NOBODY fixed16_t Invert24To16(fixed16_t val);
//{
//}

#if !defined(REHLDS_SSE)
void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up)
{
	float sr, sp, sy, cr, cp, cy;

#ifndef SWDS
	g_engdstAddrs.pfnAngleVectors(&angles, &forward, &right, &up);
#endif // SWDS

	float angle;
	angle = (float)(angles[YAW] * (M_PI * 2 / 360));
	sy = sin(angle);
	cy = cos(angle);
	angle = (float)(angles[PITCH] * (M_PI * 2 / 360));
	sp = sin(angle);
	cp = cos(angle);
	angle = (float)(angles[ROLL] * (M_PI * 2 / 360));
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1 * sr*sp*cy + -1 * cr*-sy);
		right[1] = (-1 * sr*sp*sy + -1 * cr*cy);
		right[2] = -1 * sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy + -sr*-sy);
		up[1] = (cr*sp*sy + -sr*cy);
		up[2] = cr*cp;
	}
}

void AngleVectorsTranspose(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up)
{
	float sr, sp, sy, cr, cp, cy;

	float angle;
	angle = (float)(angles[YAW] * (M_PI * 2 / 360));
	sy = sin(angle);
	cy = cos(angle);
	angle = (float)(angles[PITCH] * (M_PI * 2 / 360));
	sp = sin(angle);
	cp = cos(angle);
	angle = (float)(angles[ROLL] * (M_PI * 2 / 360));
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = (sr*sp*cy + cr*-sy);
		forward[2] = (cr*sp*cy + -sr*-sy);
	}
	if (right)
	{
		right[0] = cp*sy;
		right[1] = (sr*sp*sy + cr*cy);
		right[2] = (cr*sp*sy + -sr*cy);
	}
	if (up)
	{
		up[0] = -sp;
		up[1] = sr*cp;
		up[2] = cr*cp;
	}
}

void AngleMatrix(const vec_t *angles, float(*matrix)[4])
{
	float sr, sp, sy, cr, cp, cy;

	float angle;
	angle = (float)(angles[ROLL] * (M_PI * 2 / 360));
	sy = sin(angle);
	cy = cos(angle);
	angle = (float)(angles[YAW] * (M_PI * 2 / 360));
	sp = sin(angle);
	cp = cos(angle);
	angle = (float)(angles[PITCH] * (M_PI * 2 / 360));
	sr = sin(angle);
	cr = cos(angle);

	matrix[0][0] = cr * cp;
	matrix[0][1] = sy * sr * cp - cy * sp;
	matrix[0][2] = cy * sr * cp + sy * sp;
	matrix[0][3] = 0.0f;

	matrix[1][0] = cr * sp;
	matrix[1][1] = sy * sr * sp + cy * cp;
	matrix[1][2] = cy * sr * sp - sy * cp;
	matrix[1][3] = 0.0f;

	matrix[2][0] = -sr;
	matrix[2][1] = sy * cr;
	matrix[2][2] = cy * cr;
	matrix[2][3] = 0.0f;
}

void VectorMA(const vec_t *veca, float scale, const vec_t *vecm, vec_t *out)
{
	out[0] = scale * vecm[0] + veca[0];
	out[1] = scale * vecm[1] + veca[1];
	out[2] = scale * vecm[2] + veca[2];
}

real_t _DotProduct(const vec_t *v1, const vec_t *v2)
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void CrossProduct(const vec_t *v1, const vec_t *v2, vec_t *cross)
{
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3];
}

float Length(const vec_t *v)
{
	return Q_sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

int BoxOnPlaneSide(vec_t *emins, vec_t *emaxs, mplane_t *p)
{
	// Engine actual types
	real_t dist1, dist2;
	int sides = 0;

	// general case
	switch (p->signbits)
	{
	case 0:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 1:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 2:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 3:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 4:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	default:
		BOPS_Error();
		dist1 = dist2 = 0.0;
		break;
	}

	if (dist1 >= p->dist)
		sides = 1;

	if (dist2 < p->dist)
		sides |= 2;

	return sides;
}

qboolean VectorCompare(const vec_t *v1, const vec_t *v2)
{
	for (int i = 0; i < 3; i++)
	{
		if (v1[i] != v2[i])
			return FALSE;
	}

	return TRUE;
}

#endif // #if !defined(REHLDS_SSE)

qboolean BoundsIntersect(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2)
{
	if (mins1[0] > maxs2[0] || mins1[1] > maxs2[1] || mins1[2] > maxs2[2])
		return FALSE;
	if (maxs1[0] < mins2[0] || maxs1[1] < mins2[1] || maxs1[2] < mins2[2])
		return FALSE;
	return TRUE;
}

// The first box reaches out of the second one on at least one axis
qboolean BoundsExceed(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2)
{
	for (int i = 0; i < 3; i++)
	{
		if (mins1[i] < mins2[i] || maxs1[i] > maxs2[i])
			return TRUE;
	}

	return FALSE;
}
