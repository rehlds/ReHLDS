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

#include "const.h"
#include "modelgen.h"
#include "spritegn.h"
#include "bspfile.h"
#include "crc.h"
#include "com_model.h"
#include "commonmacros.h"

// header
#define ALIAS_MODEL_VERSION	0x006
#define IDPOLYHEADER		MAKEID('I', 'D', 'P', 'O') // little-endian "IDPO"

#define MAX_LBM_HEIGHT		480
#define MAX_ALIAS_MODEL_VERTS	2000

#define SURF_PLANEBACK		2
#define SURF_DRAWSKY		4
#define SURF_DRAWSPRITE		8
#define SURF_DRAWTURB		0x10
#define SURF_DRAWTILED		0x20
#define SURF_DRAWBACKGROUND	0x40

#define MAX_MODEL_NAME		64
#define MIPLEVELS			4
#define NUM_AMBIENTS		4		// automatic ambient sounds
#define MAXLIGHTMAPS		4
#define MAX_KNOWN_MODELS	1024

typedef struct mvertex_s
{
	vec3_t			position;
} mvertex_t;

typedef struct mplane_s
{
	vec3_t			normal;			// surface normal
	float			dist;			// closest appoach to origin
	byte			type;			// for texture axis selection and fast side tests
	byte			signbits;		// signx + signy<<1 + signz<<1
	byte			pad[2];
} mplane_t;

typedef struct texture_s
{
	char			name[16];
	unsigned		width, height;

#if !defined(SWDS) && !defined(HLTV)
	int			gl_texturenum;
	struct msurface_s *	texturechain;
#endif

	int				anim_total;			// total tenths in sequence ( 0 = no)
	int				anim_min, anim_max;	// time for this frame min <=time< max
	struct texture_s *anim_next;		// in the animation sequence
	struct texture_s *alternate_anims;	// bmodels in frame 1 use these
	unsigned		offsets[MIPLEVELS];	// four mip maps stored

#if defined(SWDS) || defined(HLTV)
	unsigned		paloffset;
#else
	byte *pPal;
#endif

} texture_t;

typedef struct medge_s
{
	unsigned short	v[2];
	unsigned int	cachededgeoffset;
} medge_t;

typedef struct mtexinfo_s
{
	float			vecs[2][4];		// [s/t] unit vectors in world space.
									// [i][3] is the s/t offset relative to the origin.
									// s or t = dot(3Dpoint,vecs[i])+vecs[i][3]
	float			mipadjust;		// ?? mipmap limits for very small surfaces
	texture_t		*texture;
	int				flags;			// sky or slime, no lightmap or 256 subdivision
} mtexinfo_t;
#define	TEX_SPECIAL		1		// sky or slime, no lightmap or 256 subdivision

typedef struct msurface_s msurface_t;
typedef struct decal_s decal_t;

// JAY: Compress this as much as possible
struct decal_s
{
	decal_t			*pnext;			// linked list for each surface
	msurface_t		*psurface;		// Surface id for persistence / unlinking
	short			dx;				// Offsets into surface texture (in texture coordinates, so we don't need floats)
	short			dy;
	short			texture;		// Decal texture
	byte			scale;			// Pixel scale
	byte			flags;			// Decal flags

	short			entityIndex;	// Entity this is attached to
};

struct msurface_s
{
	int				visframe;		// should be drawn when node is crossed

	int				dlightframe;	// last frame the surface was checked by an animated light
	int				dlightbits;		// dynamically generated. Indicates if the surface illumination
									// is modified by an animated light.

	mplane_t		*plane;			// pointer to shared plane
	int				flags;			// see SURF_ #defines

	int				firstedge;	// look up in model->surfedges[], negative numbers
	int				numedges;	// are backwards edges

	// surface generation data
	struct surfcache_s *cachespots[MIPLEVELS];

	// Maximum s/t texture size on the surface
#if defined(GLQUAKE) || defined(SWDS)
	#define MAX_SURFACE_TEXTURE_SIZE 512
#else
	#define MAX_SURFACE_TEXTURE_SIZE 256 // Software rendering is limited to 256
#endif

	short			texturemins[2]; // smallest s/t position on the surface.
	short			extents[2];		// ?? s/t texture size, 1..256 for all non-sky surfaces

	mtexinfo_t		*texinfo;

	// lighting info
	byte			styles[MAXLIGHTMAPS];	// index into d_lightstylevalue[] for animated lights
											// no one surface can be effected by more than 4
											// animated lights.
	color24			*samples;

	decal_t			*pdecals;
};

typedef struct mnode_s
{
	// common with leaf
	int				contents;		// 0, to differentiate from leafs
	int				visframe;		// node needs to be traversed if current

	short			minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

	// node specific
	mplane_t		*plane;
	struct mnode_s	*children[2];

	unsigned short	firstsurface;
	unsigned short	numsurfaces;
} mnode_t;

typedef struct mleaf_s
{
	// common with node
	int				contents;		// wil be a negative contents number
	int				visframe;		// node needs to be traversed if current

	short			minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

	// leaf specific
	byte			*compressed_vis;
	struct efrag_s	*efrags;

	msurface_t		**firstmarksurface;
	int				nummarksurfaces;
	int				key;			// BSP sequence number for leaf's contents
	byte			ambient_sound_level[NUM_AMBIENTS];
} mleaf_t;

typedef struct hull_s
{
	dclipnode_t		*clipnodes;
	mplane_t		*planes;
	int				firstclipnode;
	int				lastclipnode;
	vec3_t			clip_mins, clip_maxs;
} hull_t;
typedef enum { point_hull = 0, human_hull = 1, large_hull = 2, head_hull = 3 };

typedef struct mspriteframe_t
{
	int				width;
	int				height;
	void			*pcachespot;
	float			up, down, left, right;
	byte			pixels[4];
} mspriteframe_s;

typedef struct mspritegroup_s
{
	int				numframes;
	float			*intervals;
	mspriteframe_t	*frames[1];
} mspritegroup_t;

typedef struct mspriteframedesc_s
{
	spriteframetype_t type;
	mspriteframe_t	*frameptr;
} mspriteframedesc_t;

typedef struct msprite_s
{
	short int		type;
	short int		texFormat;
	int				maxwidth, maxheight;
	int				numframes;
	int				paloffset;
	float			beamlength;
	void			*cachespot;
	mspriteframedesc_t frames[1];
} msprite_t;

typedef struct maliasframedesc_s
{
	aliasframetype_t type;
	trivertx_t		bboxmin, bboxmax;
	int				frame;
	char			name[16];
} maliasframedesc_t;

typedef struct maliasskindesc_s
{
	aliasskintype_t type;
	void			*pcachespot;
	int				skin;
} maliasskindesc_t;

typedef struct maliasgroupframedesc_s
{
	trivertx_t		bboxmin, bboxmax;
	int				frame;
} maliasgroupframedesc_t;

typedef struct maliasgroup_s
{
	int				numframes;
	int				intervals;
	maliasgroupframedesc_t frames[1];
} maliasgroup_t;

typedef struct maliasskingroup_s
{
	int				numskins;
	int				intervals;
	maliasskindesc_t skindescs[1];
} maliasskingroup_t;

typedef struct mtriangle_s
{
	int				facesfront;
	int				vertindex[3];
} mtriangle_t;

typedef struct aliashdr_s
{
	int				model;
	int				stverts;
	int				skindesc;
	int				triangles;
	int				palette;
	maliasframedesc_t frames[1];
} aliashdr_t;

typedef enum modtype_e
{
	mod_bad = -1,
	mod_brush,
	mod_sprite,
	mod_alias,
	mod_studio,
} modtype_t;

typedef struct model_s
{
	char			name[MAX_MODEL_NAME];

	int		needload;		// bmodels and sprites don't cache normally

	modtype_t		type;
	int				numframes;
	synctype_t		synctype;

	int				flags;

	//
	// volume occupied by the model
	//
	vec3_t			mins, maxs;
	float			radius;

	//
	// brush model
	//
	int				firstmodelsurface, nummodelsurfaces;

	int				numsubmodels;
	dmodel_t		*submodels;

	int				numplanes;
	mplane_t		*planes;

	int				numleafs;		// number of visible leafs, not counting 0
	struct mleaf_s	*leafs;

	int				numvertexes;
	mvertex_t		*vertexes;

	int				numedges;
	medge_t			*edges;

	int				numnodes;
	mnode_t			*nodes;

	int				numtexinfo;
	mtexinfo_t		*texinfo;

	int				numsurfaces;
	msurface_t		*surfaces;

	int				numsurfedges;
	int				*surfedges;

	int				numclipnodes;
	dclipnode_t		*clipnodes;

	int				nummarksurfaces;
	msurface_t		**marksurfaces;

	hull_t			hulls[MAX_MAP_HULLS];

	int				numtextures;
	texture_t		**textures;

	byte			*visdata;

	color24			*lightdata;

	char			*entities;

	//
	// additional model data
	//
	cache_user_t	cache;			// only access through Mod_Extradata
} model_t;

typedef struct cachepic_s
{
	char			name[64];
	cache_user_t	cache;
} cachepic_t;

typedef struct cachewad_s cachewad_t;

typedef void(*PFNCACHE)(cachewad_t *, unsigned char *);

typedef struct cachewad_s
{
	char			*name;
	cachepic_t		*cache;
	int				cacheCount;
	int				cacheMax;
	struct lumpinfo_s *lumps;
	int				lumpCount;
	int				cacheExtra;
	PFNCACHE		pfnCacheBuild;
	int				numpaths;
	char			**basedirs;
	int				*lumppathindices;
#ifndef SWDS
	int				tempWad;
#endif // SWDS
} cachewad_t;

typedef struct mod_known_info_s
{
	qboolean		shouldCRC;
	qboolean		firstCRCDone;
	CRC32_t			initialCRC;
} mod_known_info_t;

