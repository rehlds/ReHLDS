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

#if defined(REHLDS_SSE)

// Intrisics guide: https://software.intel.com/sites/landingpage/IntrinsicsGuide/
// Shufps calculator: http://wurstcaptures.untergrund.net/assembler_tricks.html

// aligned vec4_t
typedef ALIGN16 vec4_t avec4_t;
typedef ALIGN16 unsigned int aivec4_t[4];

// conversion multiplier
const avec4_t deg2rad =
{
	(float)M_PI / 180.f,
	(float)M_PI / 180.f,
	(float)M_PI / 180.f,
	(float)M_PI / 180.f
};

const aivec4_t negmask =
{
	0x80000000,
	0x80000000,
	0x80000000,
	0x80000000
};

const aivec4_t negmask_1001 =
{
	0x80000000,
	0,
	0,
	0x80000000
};

const aivec4_t negmask_0010 =
{
	0,
	0,
	0x80000000,
	0
};

// save 4d xmm to 3d vector. we can't optimize many simple vector3 functions because saving back to 3d is slow.
inline void xmm2vec(vec_t *v, const __m128 m)
{
	_mm_storel_pi((__m64*)v, m);
	_mm_store_ss(v + 2, _mm_shuffle_ps(m, m, 0x02));
}

FUNC_TARGET("sse4.1")
inline __m128 dotProduct3D(__m128 v1, __m128 v2)
{
	if (cpuinfo.sse4_1)
		return _mm_dp_ps(v1, v2, 0x71);
	__m128 v = _mm_mul_ps(v1, v2);
	return _mm_add_ps(_mm_movehl_ps(v, v), _mm_hadd_ps(v, v)); // SSE3
}

inline __m128 crossProduct3D(__m128 a, __m128 b)
{
	__m128 tmp1 = _mm_mul_ps(a, _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1)));
	__m128 tmp2 = _mm_mul_ps(b, _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1)));
	__m128 m = _mm_sub_ps(tmp1, tmp2);

	return _mm_shuffle_ps(m, m, _MM_SHUFFLE(3, 0, 2, 1));
}

inline __m128 length3D(__m128 v)
{
	return _mm_sqrt_ps(dotProduct3D(v, v));
}

inline __m128 length2D(__m128 v)
{
	v = _mm_mul_ps(v, v);
	return _mm_sqrt_ps(_mm_hadd_ps(v, v)); // hadd = SSE3
}

int BoxOnPlaneSide(vec_t *emins, vec_t *emaxs, mplane_t *p)
{
	double dist1, dist2;
	int sides = 0;

	__m128 emin = _mm_loadu_ps(emins);
	__m128 emax = _mm_loadu_ps(emaxs);
	avec4_t d1, d2;

	// general case
	switch (p->signbits)
	{
	case 0:
		_mm_store_ps(d1, emax);
		_mm_store_ps(d2, emin);
		break;
	case 1:
		_mm_store_ps(d1, emax);
		_mm_store_ps(d2, emin);
		d1[0] = emins[0];
		d2[0] = emaxs[0];
		break;
	case 2:
		_mm_store_ps(d1, emax);
		_mm_store_ps(d2, emin);
		d1[1] = emins[1];
		d2[1] = emaxs[1];
		break;
	case 3:
		_mm_store_ps(d1, emin);
		_mm_store_ps(d2, emax);
		d1[2] = emaxs[2];
		d2[2] = emins[2];
		break;
	case 4:
		_mm_store_ps(d1, emax);
		_mm_store_ps(d2, emin);
		d1[2] = emins[2];
		d2[2] = emaxs[2];
		break;
	case 5:
		_mm_store_ps(d1, emin);
		_mm_store_ps(d2, emax);
		d1[1] = emaxs[1];
		d2[1] = emins[1];
		break;
	case 6:
		_mm_store_ps(d1, emin);
		_mm_store_ps(d2, emax);
		d1[0] = emaxs[0];
		d2[0] = emins[0];
		break;
	case 7:
		_mm_store_ps(d1, emin);
		_mm_store_ps(d2, emax);
		break;
	default:
		BOPS_Error();
		dist1 = dist2 = 0.0;
		break;
	}

	dist1 = _DotProduct(p->normal, d1);
	dist2 = _DotProduct(p->normal, d2);

	if (dist1 >= p->dist)
		sides = 1;

	if (dist2 < p->dist)
		sides |= 2;

	return sides;
}

qboolean VectorCompare(const vec_t *v1, const vec_t *v2)
{
	// Load the 3D vectors into 128-bit SIMD registers.
	// The 4th component is set to 0.0f to safely fill the full register.
	__m128 vec1 = _mm_setr_ps(v1[0], v1[1], v1[2], 0.0f);
	__m128 vec2 = _mm_setr_ps(v2[0], v2[1], v2[2], 0.0f);

	// Perform component-wise comparison for inequality (vec1[i] != vec2[i]).
	__m128 cmp_result = _mm_cmpneq_ps(vec1, vec2);

	// Extract the most significant bit of each float comparison result.
	// This produces a 4-bit mask: bits [x y z w]
	int mask = _mm_movemask_ps(cmp_result);

	// Check if any of the first 3 components (x, y, z) are different.
	// Bit 0: x, Bit 1: y, Bit 2: z
	// If none of these bits are set, the vectors are equal.
	return (mask & 0x7) == 0;
}

void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up)
{
#ifndef SWDS
	g_engdstAddrs.pfnAngleVectors(&angles, &forward, &right, &up);
#endif // SWDS

	__m128 s, c;
	sincos_ps(_mm_mul_ps(_mm_loadu_ps(angles), _mm_load_ps(deg2rad)), &s, &c);

	__m128 m1 = _mm_shuffle_ps(c, s, 0x90); // [cp][cp][sy][sr]
	__m128 m2 = _mm_shuffle_ps(c, c, 0x09); // [cy][cr][cp][cp]
	__m128 cp_mults = _mm_mul_ps(m1, m2); // [cp * cy][cp * cr][cp * sy][cp * sr];

	m1 = _mm_shuffle_ps(c, s, 0x15); // [cy][cy][sy][sp]
	m2 = _mm_shuffle_ps(s, c, 0xA0); // [sp][sp][cr][cr]
	m1 = _mm_shuffle_ps(m1, m1, 0xC8); // [cy][sy][cy][sp]

	__m128 m3 = _mm_shuffle_ps(s, s, 0x4A); // [sr][sr][sp][sy];
	m3 = _mm_mul_ps(m3, _mm_mul_ps(m1, m2)); // [sp*cy*sr][sp*sy*sr][cr*cy*sp][cr*sp*sy]

	m2 = _mm_shuffle_ps(s, c, 0x65); // [sy][sy][cr][cy]
	m1 = _mm_shuffle_ps(c, s, 0xA6); // [cr][cy][sr][sr]
	m2 = _mm_shuffle_ps(m2, m2, 0xD8); // [sy][cr][sy][cy]
	m1 = _mm_xor_ps(m1, _mm_load_ps((float *)&negmask_1001)); // [-cr][cy][sr][-sr]
	m1 = _mm_mul_ps(m1, m2); // [-cr*sy][cy*cr][sr*sy][-sr*cy]

	m3 = _mm_add_ps(m3, m1);

	if (forward)
	{
		_mm_storel_pi((__m64 *)forward, _mm_shuffle_ps(cp_mults, cp_mults, 0x08));
		forward[2] = -_mm_cvtss_f32(s);
	}
	if (right)
	{
		__m128 r = _mm_shuffle_ps(m3, cp_mults, 0xF4); // [m3(0)][m3(1)][cp(3)][cp(3)]
		xmm2vec(right, _mm_xor_ps(r, _mm_load_ps((float *)&negmask)));
	}
	if (up)
	{
		_mm_storel_pi((__m64 *)up, _mm_shuffle_ps(m3, m3, 0x0E));
		up[2] = _mm_cvtss_f32(_mm_shuffle_ps(cp_mults, cp_mults, 0x01));
	}
}

void AngleVectorsTranspose(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up)
{
	__m128 s, c;
	sincos_ps(_mm_mul_ps(_mm_loadu_ps(angles), _mm_load_ps(deg2rad)), &s, &c);

	__m128 m1 = _mm_shuffle_ps(c, s, 0x90); // [cp][cp][sy][sr]
	__m128 m2 = _mm_shuffle_ps(c, c, 0x09); // [cy][cr][cp][cp]
	__m128 cp_mults = _mm_mul_ps(m1, m2); // [cp * cy][cp * cr][cp * sy][cp * sr];

	m1 = _mm_shuffle_ps(s, s, 0x50); // [sp][sp][sy][sy]
	m2 = _mm_shuffle_ps(c, s, 0x05); // [cy][cy][sp][sp]

	__m128 m3 = _mm_shuffle_ps(s, c, 0xAA); // [sr][sr][cr][cr]
	m1 = _mm_mul_ps(m1, m2);
	m3 = _mm_shuffle_ps(m3, m3, 0xD8); // [sr][cr][sr][cr]
	m3 = _mm_mul_ps(m3, m1); // [sp*cy*sr][sp*cy*cr][sy*sp*sr][sy*sp*cr]

	m2 = _mm_shuffle_ps(c, s, 0xA6); // [cr][cy][sr][sr]
	m1 = _mm_shuffle_ps(s, c, 0x65); // [sy][sy][cr][cy]
	m2 = _mm_shuffle_ps(m2, m2, 0xD8); // [cr][sr][cy][sr]
	m1 = _mm_xor_ps(m1, _mm_load_ps((float *)&negmask_1001)); // [-cr][cy][sr][-sr]
	m1 = _mm_mul_ps(m1, m2); // [-cr*sy][sr*sy][cy*cr][-sr*cy]

	m3 = _mm_add_ps(m3, m1);

	if (forward)
	{
		forward[0] = _mm_cvtss_f32(cp_mults);
		_mm_storel_pi((__m64*)(forward + 1), m3); // (sr*sp*cy + cr*-sy);
	}
	if (right)
	{
		right[0] = _mm_cvtss_f32(_mm_shuffle_ps(cp_mults, cp_mults, 0x02));
		_mm_storel_pi((__m64*)(right + 1), _mm_shuffle_ps(m3, m3, 0x0E));
	}
	if (up)
	{
		up[0] = -_mm_cvtss_f32(s);
		_mm_storel_pi((__m64 *)&up[1], _mm_shuffle_ps(cp_mults, cp_mults, 0x07));
	}
}

void AngleMatrix(const vec_t *angles, float(*matrix)[4])
{
	__m128 s, c;
	sincos_ps(_mm_mul_ps(_mm_loadu_ps(angles), _mm_load_ps(deg2rad)), &s, &c);

	/*
	matrix[0][1] = sr * sp * cy - cr * sy;
	matrix[1][1] = sr * sp * sy + cr * cy;
	matrix[0][2] = cr * sp * cy + sr * sy;
	matrix[1][2] = cr * sp * sy - sr * cy;
	*/
	__m128 m1;
	__m128 m2 = _mm_shuffle_ps(s, c, 0x00); // [sp][sp][cp][cp]
	__m128 m3 = _mm_shuffle_ps(c, s, 0x55); // [cy][cy][sy][sy]

	m1 = _mm_shuffle_ps(s, c, 0xAA);   // [sr][sr][cr][cr]
	m2 = _mm_shuffle_ps(m2, m2, 0x00); // [sp][sp][sp][sp]
	m3 = _mm_shuffle_ps(m3, m3, 0xD8); // [cy][sy][cy][sy]

	m2 = _mm_mul_ps(m2, _mm_mul_ps(m1, m3)); // m1*m2*m3

	m1 = _mm_shuffle_ps(m1, m1, 0x1B); // [cr][cr][sr][sr]
	m3 = _mm_shuffle_ps(m3, m3, 0xB1); // [sy][cy][sy][cy]
	m3 = _mm_xor_ps(m3, _mm_load_ps((float *)&negmask_1001));
	m3 = _mm_mul_ps(m3, m1);

	m2 = _mm_add_ps(m2, m3);

	/*
	matrix[0][0] = cp * cy;
	matrix[1][0] = cp * sy;
	matrix[2][1] = sr * cp;
	matrix[2][2] = cr * cp;
	*/
	m1 = _mm_shuffle_ps(s, c, 0x29); // [sy][sr][cr][cp]
	c = _mm_shuffle_ps(c, c, 0x40);  // [cp][cp][cp][cy]
	m1 = _mm_mul_ps(m1, c);

	// matrix[0]
	m3 = _mm_shuffle_ps(m2, m2, 0xE1);
	_mm_storeu_ps(&matrix[0][0], m3);
	matrix[0][0] = _mm_cvtss_f32(_mm_shuffle_ps(m1, m1, 0x03));
	*(int *)&matrix[0][3] = 0;

	// matrix[1]
	m2 = _mm_shuffle_ps(m2, m2, 0xB4);
	_mm_storeu_ps(&matrix[1][0], m2);
	matrix[1][0] = _mm_cvtss_f32(m1);
	*(int *)&matrix[1][3] = 0;

	// matrix[2]
	_mm_storeu_ps(&matrix[2][0], m1);
	matrix[2][0] = -_mm_cvtss_f32(s);
	*(int *)&matrix[2][3] = 0;
}

void VectorMA(const vec_t *veca, float scale, const vec_t *vecm, vec_t *out)
{
	xmm2vec(out, _mm_add_ps(_mm_mul_ps(_mm_set_ps1(scale), _mm_loadu_ps(vecm)), _mm_loadu_ps(veca)));
}

float _DotProduct(const vec_t *v1, const vec_t *v2)
{
	// _mm_loadu_ps - load xmm from unaligned address
	// _mm_cvtss_f32 - return low float value of xmm
	// _mm_dp_ps - dot product
	// 0x71 = 0b01110001 - mask for multiplying operands and result
	// dpps isn't binary compatible with separate sse2 instructions (max difference is about 0.0002f, but usually < 0.00001f)

	return _mm_cvtss_f32(dotProduct3D(_mm_loadu_ps(v1), _mm_loadu_ps(v2)));
}

float Length(const vec_t *v)
{
	return _mm_cvtss_f32(length3D(_mm_loadu_ps(v))); // rsqrt is very inaccurate :(
}

float Length2D(const vec_t *v)
{
	return _mm_cvtss_f32(length2D(_mm_loadu_ps(v)));
}

void CrossProduct(const vec_t *v1, const vec_t *v2, vec_t *cross)
{
	xmm2vec(cross, crossProduct3D(_mm_loadu_ps(v1), _mm_loadu_ps(v2)));
}

void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4])
{
	for (size_t i = 0; i < 3; i++)
	{
		__m128 a1 = _mm_mul_ps(_mm_set_ps1(in1[i][0]), _mm_loadu_ps(in2[0]));
		__m128 a2 = _mm_mul_ps(_mm_set_ps1(in1[i][1]), _mm_loadu_ps(in2[1]));
		__m128 a3 = _mm_mul_ps(_mm_set_ps1(in1[i][2]), _mm_loadu_ps(in2[2]));
		_mm_storeu_ps(out[i], _mm_add_ps(a1, _mm_add_ps(a2, a3)));
		out[i][3] += in1[i][3];
	}
}

#endif // #if defined(REHLDS_SSE)
