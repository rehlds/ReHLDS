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

model_t *loadmodel;
char loadname[MAX_MODEL_NAME];
model_t mod_known[MAX_KNOWN_MODELS];
int mod_numknown;
unsigned char* mod_base;
char *wadpath;
int tested;
int ad_enabled;
cachewad_t ad_wad;
mod_known_info_t mod_known_info[MAX_KNOWN_MODELS];

// values for model_t's needload
enum
{
	NL_PRESENT = 0,
	NL_NEEDS_LOADED,
	NL_UNREFERENCED
};

void SW_Mod_Init(void)
{
#ifndef SWDS
	// TODO: Add client-side code
#endif
}

void* EXT_FUNC Mod_Extradata(model_t *mod)
{
	void *r;

	if (!mod)
	{
		return NULL;
	}

	r = Cache_Check(&mod->cache);
	if (r)
	{
		return r;
	}

	if (mod->type == mod_brush)
	{
		Sys_Error("%s: called with mod_brush!\n", __func__);
	}

	Mod_LoadModel(mod, 1, 0);

	if (mod->cache.data == NULL)
	{
		Sys_Error("%s: caching failed", __func__);
	}

	return mod->cache.data;
}

mleaf_t *Mod_PointInLeaf(vec_t *p, model_t *model)
{
	mnode_t *node;
	float d;
	mplane_t *plane;

	if (!model || !model->nodes)
		Sys_Error("%s: bad model", __func__);

	node = model->nodes;
	while (node->contents >= 0)
	{
		plane = node->plane;
		if (plane->type >= 3u)
			d = _DotProduct(plane->normal, p) - plane->dist;
		else
			d = p[plane->type] - plane->dist;

		if (d <= 0.0)
			node = node->children[1];
		else
			node = node->children[0];
	}
	return (mleaf_t *)node;
}

void Mod_ClearAll(void)
{
	int i;
	model_t *mod;
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (mod->type != mod_alias && mod->needload != (NL_NEEDS_LOADED | NL_UNREFERENCED))
		{
			mod->needload = NL_UNREFERENCED;
			if (mod->type == mod_sprite)
				mod->cache.data = NULL;
		}
	}
}

void Mod_FillInCRCInfo(qboolean trackCRC, int model_number)
{
	mod_known_info_t *p;

	p = &mod_known_info[model_number];
	p->shouldCRC = trackCRC;
	p->firstCRCDone = 0;
	p->initialCRC = 0;
}

model_t *Mod_FindName(qboolean trackCRC, const char *name)
{
	model_t *avail;
	int i;
	model_t *mod;

	avail = NULL;
	if (!name[0])
		Sys_Error("%s: NULL name", __func__);

	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!Q_stricmp(mod->name, name))
			break;

		if (mod->needload == NL_UNREFERENCED)
		{
			if (!avail || (mod->type != mod_alias && mod->type != mod_studio))
				avail = mod;
		}
	}

	if (i == mod_numknown)
	{
		if (mod_numknown < MAX_KNOWN_MODELS)
		{
			Mod_FillInCRCInfo(trackCRC, mod_numknown);
			++mod_numknown;
		}
		else
		{
			if (!avail)
				Sys_Error("%s: mod_numknown >= MAX_KNOWN_MODELS", __func__);
			mod = avail;
			Mod_FillInCRCInfo(trackCRC, avail - mod_known);
		}
		Q_strncpy(mod->name, name, 63);
		mod->name[63] = 0;

		if (mod->needload != (NL_NEEDS_LOADED | NL_UNREFERENCED))
			mod->needload = NL_NEEDS_LOADED;
	}

	return mod;
}

NOXREF qboolean Mod_ValidateCRC(const char *name, CRC32_t crc)
{
	NOXREFCHECK;
	model_t *mod;
	mod_known_info_t *p;

	mod = Mod_FindName(TRUE, name);
	p = &mod_known_info[mod - mod_known];

	if (p->firstCRCDone)
	{
		if (p->initialCRC != crc)
			return FALSE;
	}
	return TRUE;
}

NOXREF void Mod_NeedCRC(const char *name, qboolean needCRC)
{
	NOXREFCHECK;
	model_t *mod;
	mod_known_info_t *p;

	mod = Mod_FindName(FALSE, name);
	p = &mod_known_info[mod - mod_known];

	p->shouldCRC = needCRC;
}


//TODO: move to czmodelcheck.cpp
qboolean IsCZPlayerModel(uint32 crc, const char  * filename)
{
	if (crc == 0x27FB4D2F)
		return Q_stricmp(filename, "models/player/spetsnaz/spetsnaz.mdl") ? 0 : 1;

	if (crc == 0xEC43F76D || crc == 0x270FB2D7)
		return Q_stricmp(filename, "models/player/terror/terror.mdl") ? 0 : 1;

	if (crc == 0x1AAA3360 || crc == 0x35AC6FED)
		return Q_stricmp(filename, "models/player/gign/gign.mdl") ? 0 : 1;

	if (crc == 0x02B95E5F || crc == 0x72DB74E4)
		return Q_stricmp(filename, "models/player/vip/vip.mdl") ? 0 : 1;

	if (crc == 0x1F3CD80B || crc == 0x1B6C4115)
		return Q_stricmp(filename, "models/player/guerilla/guerilla.mdl") ? 0 : 1;

	if (crc == 0x3BCAA016)
		return Q_stricmp(filename, "models/player/militia/militia.mdl") ? 0 : 1;

	if (crc == 0x43E67FF3 || crc == 0xF141AE3F)
		return Q_stricmp(filename, "models/player/sas/sas.mdl") ? 0 : 1;

	if (crc == 0xDA8922A || crc == 0x56DD2D02)
		return Q_stricmp(filename, "models/player/gsg9/gsg9.mdl") ? 0 : 1;

	if (crc == 0xA37D8680 || crc == 0x4986827B)
		return Q_stricmp(filename, "models/player/arctic/arctic.mdl") ? 0 : 1;

	if (crc == 0xC37369F6 || crc == 0x29FE156C)
		return Q_stricmp(filename, "models/player/leet/leet.mdl") ? 0 : 1;

	if (crc == 0xC7F0DBF3 || crc == 0x068168DB)
		return Q_stricmp(filename, "models/player/urban/urban.mdl") ? 0 : 1;

	return 0;
}

model_t *Mod_LoadModel(model_t *mod, qboolean crash, qboolean trackCRC)
{
	unsigned char *buf;
	char tmpName[MAX_PATH];
	int length;
	CRC32_t currentCRC;

	if (mod->type == mod_alias || mod->type == mod_studio)
	{
		if (Cache_Check(&mod->cache))
		{
			mod->needload = NL_PRESENT;
			return mod;
		}
	}
	else
	{
		if (mod->needload == NL_PRESENT || mod->needload == (NL_NEEDS_LOADED | NL_UNREFERENCED))
			return mod;
	}

	if (COM_CheckParm("-steam") && mod->name[0] == '/')
	{
		char* p = mod->name;
		while (*(++p) == '/')
			;

		Q_strncpy(tmpName, p, sizeof(tmpName) - 1);
		tmpName[sizeof(tmpName) - 1] = '\0';

		Q_strncpy(mod->name, tmpName, sizeof(mod->name) - 1);
		mod->name[sizeof(mod->name) - 1] = '\0';
	}

	// load the file
	buf = COM_LoadFileForMe(mod->name, &length);
	if (!buf)
	{
		if (crash)
			Sys_Error("%s: %s not found", __func__, mod->name);
		return 0;
	}


	if (trackCRC)
	{
		mod_known_info_t *p = &mod_known_info[mod - mod_known];
		if (p->shouldCRC)
		{
			CRC32_Init(&currentCRC);
			CRC32_ProcessBuffer(&currentCRC, buf, length);
			currentCRC = CRC32_Final(currentCRC);
			if (p->firstCRCDone)
			{
				if (currentCRC != p->initialCRC)
				{
					Sys_Error("%s: %s has been modified since starting the engine.  Consider running system diagnostics to check for faulty hardware.\n", __func__, mod->name);
				}
			}
			else
			{
				p->firstCRCDone = 1;
				p->initialCRC = currentCRC;
#ifndef REHLDS_FIXES
				SetCStrikeFlags();
#endif
				if (!IsGameSubscribed("czero") && g_eGameType == GT_CStrike && IsCZPlayerModel(currentCRC, mod->name) && g_pcls.state)
				{
					COM_ExplainDisconnection(TRUE, "Cannot continue with altered model %s, disconnecting.", mod->name);
					CL_Disconnect();
					return 0;
				}
			}
		}
	}

	if (developer.value > 1.0)
		Con_DPrintf("loading %s\n", mod->name);

	// allocate a new model
	if (!COM_FileBase_s(mod->name, loadname, sizeof(loadname)))
	{
		Sys_Error("%s: Bad model name length: %s", __func__, mod->name);
		return NULL;
	}

	loadmodel = mod;

	mod->needload = NL_PRESENT;

	// call the apropriate loader
	switch (LittleLong(*(uint32 *)buf))
	{
	case IDPOLYHEADER:
		// old-format of the model from the quake1
		Mod_LoadAliasModel(mod, buf);
		break;
	case IDSPRITEHEADER:
		Mod_LoadSpriteModel(mod, buf);
		break;
	case IDSTUDIOHEADER:
		Mod_LoadStudioModel(mod, buf);
		break;
	default:
		Mod_LoadBrushModel(mod, buf);
		break;
	}

	if (g_modfuncs.m_pfnModelLoad)
		g_modfuncs.m_pfnModelLoad(mod, buf);

	Mem_Free(buf);
	return mod;
}

NOXREF void Mod_MarkClient(model_t *pModel)
{
	NOXREFCHECK;
	pModel->needload = (NL_NEEDS_LOADED | NL_UNREFERENCED);
}

model_t *Mod_ForName(const char *name, qboolean crash, qboolean trackCRC)
{
	model_t *mod;

	mod = Mod_FindName(trackCRC, name);
	if (!mod)
		return NULL;

	return Mod_LoadModel(mod, crash, trackCRC);
}

void Mod_AdInit(void)
{
	int i;
	char *s;
	static char filename[MAX_PATH];

	tested = 1;
	i = COM_CheckParm("-ad");
	if (i)
	{
		s = com_argv[i + 1];
		if (s && *s)
		{
			Q_snprintf(filename, MAX_PATH, "%s", s);
			if (FS_FileSize(filename) > 0)
			{
				Draw_CacheWadInit(filename, 16, &ad_wad);
				Draw_CacheWadHandler(&ad_wad, Draw_MiptexTexture, DECAL_EXTRASIZE);
				ad_enabled = 1;
			}
			else
			{
				Con_Printf("No -ad file specified, skipping\n");
			}
		}
	}
}

void Mod_AdSwap(texture_t *src, int pixels, int entries)
{
	if (!tested)
		return;

	uint8 *mippal;
	uint16 *texpal;
	texture_t *tex;

	tex = (texture_t *)Draw_CacheGet(&ad_wad, Draw_CacheIndex(&ad_wad, "img"));
	if (!tex)
		return;

	Q_memcpy(src + 1, tex + 1, pixels);

	mippal = (uint8 *)&tex[1] + pixels + 2;
	texpal = (uint16 *)((char *)&src[1] + pixels + 2);

	for (int j = 0; j < entries; j++)
	{
		texpal[0] = mippal[2];
		texpal[1] = mippal[1];
		texpal[2] = mippal[0];
		texpal[3] = 0;

		texpal++;
		mippal++;
	}
}

void Mod_LoadTextures(lump_t *l)
{
	dmiptexlump_t *m;
	miptex_t *mt;
	int palette;
	int pixels;
	uint8 *mippal;
	uint16 *texpal;
	int max;
	texture_t *tx2;
	int num;
	char dtexdata[348996];
	texture_t *anims[10];
	texture_t *altanims[10];
	double starttime;
	qboolean wads_parsed;
	texture_t *tx;

	wads_parsed = 0;
	starttime = Sys_FloatTime();
	if (!tested)
		Mod_AdInit();

	if (!l->filelen)
	{
		loadmodel->textures = 0;
		return;
	}
	m = (dmiptexlump_t *)(mod_base + l->fileofs);

	m->_nummiptex = LittleLong(m->_nummiptex);
	loadmodel->numtextures = m->_nummiptex;
	loadmodel->textures = (texture_t **)Hunk_AllocName(4 * loadmodel->numtextures, loadname);

	for (int i = 0; i < m->_nummiptex; i++)
	{
		m->dataofs[i] = LittleLong(m->dataofs[i]);
		if (m->dataofs[i] == -1)
			continue;

		mt = (miptex_t *)((char *)m + m->dataofs[i]);
		if (r_wadtextures.value != 0.0 || !LittleLong(mt->offsets[0]))
		{
			if (!wads_parsed)
			{
				TEX_InitFromWad(wadpath);
				TEX_AddAnimatingTextures();
				wads_parsed = 1;
			}
			if (!TEX_LoadLump(mt->name, (unsigned char *)dtexdata))
			{
				m->dataofs[i] = -1;
				continue;
			}
			mt = (miptex_t *)dtexdata;
		}

		for (int j = 0; j < MIPLEVELS; j++)
			mt->offsets[j] = LittleLong(mt->offsets[j]);

		mt->width = LittleLong(mt->width);
		mt->height = LittleLong(mt->height);
		if (mt->width & 0xF || mt->height & 0xF)
			Sys_Error("%s: Texture %s is not 16 aligned", __func__, mt);

		pixels = 85 * mt->height * mt->width / 64;
		palette = *(uint16*)((char*)mt + sizeof(miptex_t) + pixels);

		tx = (texture_t *)Hunk_AllocName(2 + 8 * palette + pixels + sizeof(texture_t), loadname);
		loadmodel->textures[i] = tx;

		Q_memcpy(tx->name, mt->name, sizeof(tx->name));
		if (Q_strchr(tx->name, '~'))
			tx->name[2] = ' ';

		tx->width = mt->width;
		tx->height = mt->height;

		for (int j = 0; j < MIPLEVELS; j++)
			tx->offsets[j] = mt->offsets[j] + sizeof(texture_t) - sizeof(miptex_t);

		tx->paloffset = sizeof(texture_t) + pixels + 2;
		Q_memcpy(&tx[1], &mt[1], pixels + 2); //copy pixels & palette size (2)

		if (!Q_strncmp(mt->name, "sky", 3))
		{
			//R_InitSky();
		}

		mippal = (uint8 *)&mt[1] + pixels + 2;
		texpal = (uint16 *)((char*)&tx[1] + pixels + 2);
		for (int j = 0; j < palette; j++, mippal += 3, texpal += 4)
		{
			texpal[0] = texgammatable[mippal[2]];
			texpal[1] = texgammatable[mippal[1]];
			texpal[2] = texgammatable[mippal[0]];
			texpal[3] = 0;
		}

		if (ad_enabled && !Q_stricmp(tx->name, "DEFAULT"))
			Mod_AdSwap(tx, pixels, palette);
	}

	if (wads_parsed)
		TEX_CleanupWadInfo();

	for (int i = 0; i < m->_nummiptex; i++)
	{
		tx = loadmodel->textures[i];
		if (!tx)
			continue;

		if (tx->name[0] != '+' && tx->name[0] != '-')
			continue;

		if (tx->anim_next)
			continue;

		Q_memset(anims, 0, sizeof(anims));
		Q_memset(altanims, 0, sizeof(altanims));
		max = tx->name[1];
		int altmax = 0;
		if (max >= 'a' && max <= 'z')
			max -= 32;
		if (max < '0' || max > '9')
		{
			if (max < 'A' || max > 'J')
				Sys_Error("%s: Bad animating texture %s", __func__, tx);
			altmax = max - 'A';
			max = 0;
			altanims[altmax] = tx;
			altmax++;
		}
		else
		{
			max -= '0';
			altmax = 0;
			anims[max] = tx;
			max++;
		}

		for (int j = i + 1; j < m->_nummiptex; j++)
		{
			tx2 = loadmodel->textures[j];
			if (!tx2)
				continue;

			if (tx2->name[0] != '+' && tx2->name[0] != '-')
				continue;

			if (Q_strcmp(&tx2->name[2], &tx->name[2]))
				continue;

			num = tx2->name[1];
			if (num >= 'a' && num <= 'z')
				num -= 'a' - 'A';

			if (num < '0' || num > '9')
			{
				if (num < 'A' || num > 'J')
					Sys_Error("%s: Bad animating texture %s", __func__, tx);

				num -= 'A';
				altanims[num] = tx2;
				if ((num + 1) > altmax)
					altmax = num + 1;
			}
			else
			{
				num -= '0';
				anims[num] = tx2;
				if ((num + 1) > max)
					max = num + 1;
			}

		}

		for (int j = 0; j < max; j++)
		{
			tx2 = anims[j];
			if (!tx2)
				Sys_Error("%s: Missing frame %i of %s", __func__, j, tx);
			tx2->anim_min = j;
			tx2->anim_total = max;
			tx2->anim_max = j + 1;
			tx2->anim_next = anims[(j + 1) % max];
			if (altmax)
				tx2->alternate_anims = altanims[0];
		}

		for (int j = 0; j < altmax; j++)
		{
			tx2 = altanims[j];
			if (!tx2)
				Sys_Error("%s: Missing frame %i of %s", __func__, j, tx);

			tx2->anim_min = j;
			tx2->anim_total = altmax;
			tx2->anim_max = j + 1;
			tx2->anim_next = altanims[(j + 1) % altmax];
			if (max)
				tx2->alternate_anims = anims[0];
		}
	}

	Con_DPrintf("Texture load: %6.1fms\n", (Sys_FloatTime() - starttime) * 1000.0);
}

void Mod_LoadLighting(lump_t *l)
{
	if (l->filelen)
	{
		loadmodel->lightdata = (color24 *)Hunk_AllocName(l->filelen, loadname);
		Q_memcpy(loadmodel->lightdata, (const void *)(mod_base + l->fileofs), l->filelen);
	}
	else
	{
		loadmodel->lightdata = 0;
	}
}

void Mod_LoadVisibility(lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->visdata = NULL;
		return;
	}
	loadmodel->visdata = (byte*) Hunk_AllocName(l->filelen, loadname);
	Q_memcpy(loadmodel->visdata, mod_base + l->fileofs, l->filelen);
}

void Mod_LoadEntities(lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->entities = NULL;
		return;
	}

	loadmodel->entities = (char *)Hunk_AllocName(l->filelen, loadname);
	Q_memcpy(loadmodel->entities, (const void *)(mod_base + l->fileofs), l->filelen);
	if (loadmodel->entities)
	{
		char *pszInputStream = COM_Parse(loadmodel->entities);
		if (*pszInputStream)
		{
			while (com_token[0] != '}')
			{
				if (!Q_strcmp(com_token, "wad"))
				{
					COM_Parse(pszInputStream);
					if (wadpath)
						Mem_Free(wadpath);
					wadpath = Mem_Strdup(com_token);
					return;
				}
				pszInputStream = COM_Parse(pszInputStream);
				if (!*pszInputStream)
					return;
			}
		}
	}

}

void Mod_LoadVertexes(lump_t *l)
{
	dvertex_t	*in;
	mvertex_t	*out;
	int			i, count;

	in = (dvertex_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("%s: funny lump size in %s", __func__, loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mvertex_t*) Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->position[0] = LittleFloat(in->point[0]);
		out->position[1] = LittleFloat(in->point[1]);
		out->position[2] = LittleFloat(in->point[2]);
	}
}

void Mod_LoadSubmodels(lump_t *l)
{
	dmodel_t	*in;
	dmodel_t	*out;
	int			i, j, count;

	in = (dmodel_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("%s: funny lump size in %s", __func__, loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (dmodel_t *)Hunk_AllocName(count*sizeof(*out), loadname);

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}
		for (j = 0; j < MAX_MAP_HULLS; j++)
			out->headnode[j] = LittleLong(in->headnode[j]);
		out->visleafs = LittleLong(in->visleafs);
		out->firstface = LittleLong(in->firstface);
		out->numfaces = LittleLong(in->numfaces);
	}
}

void Mod_LoadEdges(lump_t *l)
{
	dedge_t *in;
	medge_t *out;
	int 	i, count;

	in = (dedge_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("%s: funny lump size in %s", __func__, loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (medge_t*) Hunk_AllocName((count + 1) * sizeof(*out), loadname);

	loadmodel->edges = out;
	loadmodel->numedges = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

void Mod_LoadTexinfo(lump_t *l)
{
	texinfo_t *in;
	mtexinfo_t *out;
	int 	i, j, count;
	int		_miptex;
	float	len1, len2;

	in = (texinfo_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("%s: funny lump size in %s", __func__, loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mtexinfo_t*) Hunk_AllocName(count*sizeof(*out), loadname);

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for (i = 0; i < count; i++, in++, out++)
	{
#ifdef REHLDS_FIXES

		for (j = 0; j < 4; j++)
		{
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
		}

#else
		for (j = 0; j < 8; j++)
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
#endif // REHLDS_FIXES

		len1 = Length(out->vecs[0]);
		len2 = Length(out->vecs[1]);
		len1 = (len1 + len2) / 2;
		if (len1 < 0.32)
			out->mipadjust = 4;
		else if (len1 < 0.49)
			out->mipadjust = 3;
		else if (len1 < 0.99)
			out->mipadjust = 2;
		else
			out->mipadjust = 1;

		_miptex = LittleLong(in->_miptex);
		out->flags = LittleLong(in->flags);

		if (!loadmodel->textures)
		{
			out->texture = r_notexture_mip;	// checkerboard texture
			out->flags = 0;
		}
		else
		{
			if (_miptex >= loadmodel->numtextures)
				Sys_Error("%s: miptex >= loadmodel->numtextures", __func__);
			out->texture = loadmodel->textures[_miptex];
			if (!out->texture)
			{
				out->texture = r_notexture_mip; // texture not found
				out->flags = 0;
			}
		}
	}
}

void CalcSurfaceExtents(msurface_t *s)
{
	float	mins[2], maxs[2], val;
	int		i, j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	vec3_t		middle{};
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;

	for (i = 0; i < s->numedges; i++)
	{
		e = loadmodel->surfedges[s->firstedge + i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		VectorAdd(middle, v->position, middle);

		for (j = 0; j < 2; j++)
		{
			// FIXED: loss of floating point
			val = v->position[0] * (double)tex->vecs[j][0] +
				v->position[1] * (double)tex->vecs[j][1] +
				v->position[2] * (double)tex->vecs[j][2] +
				(double)tex->vecs[j][3];

			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	VectorScale(middle, 1.0f / s->numedges, middle);

	for (i = 0; i < 2; i++)
	{
		bmins[i] = (int) floor(mins[i] / 16);
		bmaxs[i] = (int) ceil(maxs[i] / 16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;

		if (!(tex->flags & TEX_SPECIAL) && s->extents[i] > MAX_SURFACE_TEXTURE_SIZE)
		{
			int surfID = s - loadmodel->surfaces;
			Sys_Error("%s: Bad #%d surface extents %d/%d on %s at position (%d,%d,%d)",
				__func__, surfID, s->extents[0], s->extents[1],
				tex->texture->name,
				(int)middle[0], (int)middle[1], (int)middle[2]
			);
		}
	}
}

void Mod_LoadFaces(lump_t *l)
{
	dface_t		*in;
	msurface_t 	*out;
	int			i, count, surfnum;
	int			planenum, side;

	in = (dface_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("%s: funny lump size in %s", __func__, loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (msurface_t *) Hunk_AllocName(count*sizeof(*out), loadname);

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for (surfnum = 0; surfnum < count; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);
		out->flags = 0;

		planenum = LittleShort(in->planenum);
		side = LittleShort(in->side);
		if (side)
			out->flags |= SURF_PLANEBACK;

		out->plane = loadmodel->planes + planenum;

		out->texinfo = loadmodel->texinfo + LittleShort(in->texinfo);

		CalcSurfaceExtents(out);

		// lighting info

		for (i = 0; i < MAXLIGHTMAPS; i++)
			out->styles[i] = in->styles[i];
		i = LittleLong(in->lightofs);
		if (i == -1)
			out->samples = NULL;
		else
			out->samples = loadmodel->lightdata + i;

		// set the drawing flags flag
		const char* texName = out->texinfo->texture->name;

		if (!Q_strncmp(texName, "sky", 3))	// sky
		{
			out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
			continue;
		}

		if (!Q_strncmp(texName, "scroll", 6))
		{
			out->flags |= SURF_DRAWTILED;
			out->extents[0] = out->texinfo->texture->width;
			out->extents[1] = out->texinfo->texture->height;
			continue;
		}

		if (texName[0] == '!' || !Q_strnicmp(texName, "laser", 5) || !Q_strnicmp(texName, "water", 5))		// turbulent
		{
			out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
			for (i = 0; i < 2; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
				out->texinfo->flags |= TEX_SPECIAL;
			}
			continue;
		}

		if (out->texinfo->flags & TEX_SPECIAL)
		{
			out->flags |= SURF_DRAWTILED;
			out->extents[0] = out->texinfo->texture->width;
			out->extents[1] = out->texinfo->texture->height;
			continue;
		}
	}
}

void Mod_SetParent(mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents < 0)
		return;
	Mod_SetParent(node->children[0], node);
	Mod_SetParent(node->children[1], node);
}

void Mod_LoadNodes(lump_t *l)
{
	int			i, j, count, p;
	dnode_t		*in;
	mnode_t 	*out;

	in = (dnode_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("%s: funny lump size in %s", __func__, loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mnode_t*) Hunk_AllocName(count*sizeof(*out), loadname);

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = LittleShort(in->firstface);
		out->numsurfaces = LittleShort(in->numfaces);

		for (j = 0; j < 2; j++)
		{
			p = LittleShort(in->children[j]);
			if (p >= 0)
				out->children[j] = loadmodel->nodes + p;
			else
				out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
		}
	}

	Mod_SetParent(loadmodel->nodes, NULL);	// sets nodes and leafs
}

void Mod_LoadLeafs(lump_t *l)
{
	dleaf_t 	*in;
	mleaf_t 	*out;
	int			i, j, count, p;

	in = (dleaf_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("%s: funny lump size in %s", __func__, loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mleaf_t*) Hunk_AllocName(count*sizeof(*out), loadname);

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

		out->firstmarksurface = loadmodel->marksurfaces +
			LittleShort(in->firstmarksurface);
		out->nummarksurfaces = LittleShort(in->nummarksurfaces);

		p = LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadmodel->visdata + p;
		out->efrags = NULL;

		for (j = 0; j < 4; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];
	}
}

void Mod_LoadClipnodes(lump_t *l)
{
	dclipnode_t *in, *out;
	int			i, count;
	hull_t		*hull;

	in = (dclipnode_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("%s: funny lump size in %s", __func__, loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (dclipnode_t*) Hunk_AllocName(count*sizeof(*out), loadname);

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count;

	hull = &loadmodel->hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -36;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 36;

	hull = &loadmodel->hulls[2];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -32;
	hull->clip_mins[1] = -32;
	hull->clip_mins[2] = -32;
	hull->clip_maxs[0] = 32;
	hull->clip_maxs[1] = 32;
	hull->clip_maxs[2] = 32;

	hull = &loadmodel->hulls[3];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -18;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 18;

	for (i = 0; i < count; i++, out++, in++)
	{
		out->planenum = LittleLong(in->planenum);
		out->children[0] = LittleShort(in->children[0]);
		out->children[1] = LittleShort(in->children[1]);
	}
}

void Mod_MakeHull0(void)
{
	mnode_t		*in, *child;
	dclipnode_t *out;
	int			i, j, count;
	hull_t		*hull;

	hull = &loadmodel->hulls[0];

	in = loadmodel->nodes;
	count = loadmodel->numnodes;
	out = (dclipnode_t*) Hunk_AllocName(count * sizeof(*out), loadname);

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;

	for (i = 0; i < count; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->planes;
		for (j = 0; j < 2; j++)
		{
			child = in->children[j];
			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - loadmodel->nodes;
		}
	}
}

void Mod_LoadMarksurfaces(lump_t *l)
{
	int		i, j, count;
	short		*in;
	msurface_t **out;

	in = (short *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("%s: funny lump size in %s", __func__, loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (msurface_t **) Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for (i = 0; i < count; i++)
	{
		j = LittleShort(in[i]);
		if (j >= loadmodel->numsurfaces)
			Sys_Error("%s: bad surface number", __func__);
		out[i] = loadmodel->surfaces + j;
	}
}

void Mod_LoadSurfedges(lump_t *l)
{
	int		i, count;
	int		*in, *out;

	in = (int *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("%s: funny lump size in %s", __func__, loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (int*) Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for (i = 0; i < count; i++)
		out[i] = LittleLong(in[i]);
}

void Mod_LoadPlanes(lump_t *l)
{
	int			i, j;
	mplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;

	in = (dplane_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("%s: funny lump size in %s", __func__, loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mplane_t*) Hunk_AllocName(count * 2 * sizeof(*out), loadname);

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		bits = 0;
		for (j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1 << j;
		}

		out->dist = LittleFloat(in->dist);
		out->type = LittleLong(in->type);
		out->signbits = bits;
	}
}

float RadiusFromBounds(vec_t *mins, vec_t *maxs)
{
	int		i;
	vec3_t	corner;

	for (i = 0; i < 3; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return Length(corner);
}

void Mod_LoadBrushModel(model_t *mod, void *buffer)
{
	g_RehldsHookchains.m_Mod_LoadBrushModel.callChain(&Mod_LoadBrushModel_internal, mod, buffer);
}

void EXT_FUNC Mod_LoadBrushModel_internal(model_t *mod, void *buffer)
{
	dmodel_t *bm;
	dheader_t *header;
	int i;

	loadmodel->type = mod_brush;
	header = (dheader_t *)buffer;

	i = LittleLong(header->version);
	if (i != Q1BSP_VERSION && i != HLBSP_VERSION)
		Sys_Error("%s: %s has wrong version number (%i should be %i)", __func__, mod, i, HLBSP_VERSION);

	// swap all the lumps
	mod_base = (byte *)header;

	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int *)header)[i] = LittleLong(((int *)header)[i]);

	// load into heap
	Mod_LoadVertexes(&header->lumps[LUMP_VERTEXES]);
	Mod_LoadEdges(&header->lumps[LUMP_EDGES]);
	Mod_LoadSurfedges(&header->lumps[LUMP_SURFEDGES]);

	if (Q_stricmp(com_gamedir, "bshift") == 0)
	{
		Mod_LoadEntities(&header->lumps[LUMP_ENTITIES + 1]);
		Mod_LoadTextures(&header->lumps[LUMP_TEXTURES]);
		Mod_LoadLighting(&header->lumps[LUMP_LIGHTING]);
		Mod_LoadPlanes(&header->lumps[LUMP_PLANES - 1]);
	}
	else
	{
		Mod_LoadEntities(&header->lumps[LUMP_ENTITIES]);
		Mod_LoadTextures(&header->lumps[LUMP_TEXTURES]);
		Mod_LoadLighting(&header->lumps[LUMP_LIGHTING]);
		Mod_LoadPlanes(&header->lumps[LUMP_PLANES]);
	}

	Mod_LoadTexinfo(&header->lumps[LUMP_TEXINFO]);
	Mod_LoadFaces(&header->lumps[LUMP_FACES]);
	Mod_LoadMarksurfaces(&header->lumps[LUMP_MARKSURFACES]);
	Mod_LoadVisibility(&header->lumps[LUMP_VISIBILITY]);
	Mod_LoadLeafs(&header->lumps[LUMP_LEAFS]);
	Mod_LoadNodes(&header->lumps[LUMP_NODES]);
	Mod_LoadClipnodes(&header->lumps[LUMP_CLIPNODES]);
	Mod_LoadSubmodels(&header->lumps[LUMP_MODELS]);
	Mod_MakeHull0();

	// regular and alternate animation
	mod->numframes = 2;
	mod->flags = 0;

	// set up the submodels (FIXME: this is confusing)
	for (i = 0; i < mod->numsubmodels; i++)
	{
		bm = &mod->submodels[i];

		mod->hulls[0].firstclipnode = bm->headnode[0];
		for (int j = 1; j < MAX_MAP_HULLS; j++)
		{
			mod->hulls[j].firstclipnode = bm->headnode[j];
			mod->hulls[j].lastclipnode = mod->numclipnodes - 1;
		}

		mod->firstmodelsurface = bm->firstface;
		mod->nummodelsurfaces = bm->numfaces;

		mod->maxs[0] = bm->maxs[0];
		mod->maxs[2] = bm->maxs[2];
		mod->maxs[1] = bm->maxs[1];

		mod->mins[0] = bm->mins[0];
		mod->mins[1] = bm->mins[1];
		mod->mins[2] = bm->mins[2];

		mod->radius = RadiusFromBounds(mod->mins, mod->maxs);
		mod->numleafs = bm->visleafs;

		if (i < mod->numsubmodels - 1)
		{
			char name[12];
			Q_snprintf(name, ARRAYSIZE(name), "*%i", i + 1);

			loadmodel = Mod_FindName(0, name);
			*loadmodel = *mod;

			Q_strncpy(loadmodel->name, name, sizeof(loadmodel->name) - 1);
			loadmodel->name[sizeof(loadmodel->name) - 1] = 0;
			mod = loadmodel;
		}
	}
}

void *Mod_LoadAliasFrame(void *pin, int *pframeindex, int numv, trivertx_t *pbboxmin, trivertx_t *pbboxmax, aliashdr_t *pheader, char *name)
{
	trivertx_t *pframe;
	trivertx_t *pinframe;
	int i;
	int j;

	daliasframe_t *pdaliasframe = (daliasframe_t *)pin;

	Q_strncpy(name, pdaliasframe->name, 15);
	name[15] = 0;

	for (i = 0; i < 3; i++)
	{
		pbboxmax->v[i] = pdaliasframe->bboxmax.v[i];
		pbboxmin->v[i] = pdaliasframe->bboxmin.v[i];
	}

	pinframe = (trivertx_t *)(pdaliasframe + 1);
	pframe = (trivertx_t *)Hunk_AllocName(sizeof(trivertx_t) * numv, loadname);

	*pframeindex = (byte *)pframe - (byte *)pheader;

	for (j = 0; j < numv; j++)
	{
		pframe[j].lightnormalindex = pinframe[j].lightnormalindex;

		for (int k = 0; k < 3; k++)
			pframe[j].v[k] = pinframe[j].v[k];
	}

	return (void *)&pinframe[numv];
}

void *Mod_LoadAliasGroup(void *pin, int *pframeindex, int numv, trivertx_t *pbboxmin, trivertx_t *pbboxmax, aliashdr_t *pheader, char *name)
{
	daliasgroup_t *pingroup;
	maliasgroup_t *paliasgroup;
	int i;
	int numframes;
	daliasinterval_t *pin_intervals;
	float *poutintervals;
	void *ptemp;

	pingroup = (daliasgroup_t *)pin;

	numframes = LittleLong(pingroup->numframes);
	paliasgroup = (maliasgroup_t *)Hunk_AllocName(sizeof(paliasgroup->frames[0]) * (numframes - 1) + sizeof(maliasgroup_t), loadname);
	paliasgroup->numframes = numframes;

	for (i = 0; i < 3; i++)
	{
		pbboxmin->v[i] = pingroup->bboxmin.v[i];
		pbboxmax->v[i] = pingroup->bboxmax.v[i];
	}

	*pframeindex = (byte *)paliasgroup - (byte *)pheader;

	pin_intervals = (daliasinterval_t *)(pingroup + 1);
	poutintervals = (float *)Hunk_AllocName(numframes * sizeof(float), loadname);
	paliasgroup->intervals = (byte *)poutintervals - (byte *)pheader;

	for (i = 0; i < numframes; i++)
	{
		*poutintervals = LittleFloat(pin_intervals->interval);

		if (*poutintervals <= 0.0f)
			Sys_Error("%s: interval<=0", __func__);

		poutintervals++;
		pin_intervals++;
	}

	ptemp = (void *)pin_intervals;
	for (i = 0; i < numframes; i++)
		ptemp = Mod_LoadAliasFrame(ptemp, &paliasgroup->frames[i].frame, numv, &paliasgroup->frames[i].bboxmin, &paliasgroup->frames[i].bboxmax, pheader, name);

	return ptemp;
}

void *Mod_LoadAliasSkin(void *pin, int *pskinindex, int skinsize, aliashdr_t *pheader)
{
	unsigned char *pskin;
	unsigned char *pinskin;

	pskin = (unsigned char *)Hunk_AllocName(skinsize * r_pixbytes, loadname);
	pinskin = (unsigned char *)pin;

	*pskinindex = pskin - (unsigned char *)pheader;

	if (r_pixbytes == 1)
		Q_memcpy(pskin, pinskin, skinsize);
	else if (r_pixbytes != 2)
		Sys_Error("%s: driver set invalid r_pixbytes: %d\n", __func__, r_pixbytes);

	return (void *)&pinskin[skinsize];
}

void *Mod_LoadAliasSkinGroup(void *pin, int *pskinindex, int skinsize, aliashdr_t *pheader)
{
	daliasskingroup_t *pinskingroup;
	maliasskingroup_t *paliasskingroup;
	int i;
	int numskins;
	daliasskininterval_t *pinskinintervals;
	float *poutskinintervals;
	void *ptemp;

	pinskingroup = (daliasskingroup_t *)pin;
	numskins = LittleLong(pinskingroup->numskins);
	paliasskingroup = (maliasskingroup_t *)Hunk_AllocName(sizeof(paliasskingroup->skindescs[0]) * (numskins - 1) + sizeof(maliasskingroup_t), loadname);
	paliasskingroup->numskins = numskins;
	*pskinindex = (byte *)paliasskingroup - (byte *)pheader;
	poutskinintervals = (float *)Hunk_AllocName(sizeof(float) * numskins, loadname);
	paliasskingroup->intervals = (byte *)poutskinintervals - (byte *)pheader;

	pinskinintervals = (daliasskininterval_t *)(pinskingroup + 1);

	for (i = 0; i < numskins; i++)
	{
		*poutskinintervals = LittleFloat(pinskinintervals->interval);

		if (*poutskinintervals <= 0.0f)
			Sys_Error("%s: interval<=0", __func__);

		poutskinintervals++;
		pinskinintervals++;
	}

	ptemp = (void *)pinskinintervals;

	for (i = 0; i < numskins; i++)
		ptemp = Mod_LoadAliasSkin(ptemp, &paliasskingroup->skindescs[i].skin, skinsize, pheader);

	return ptemp;
}

void Mod_LoadAliasModel(model_t *mod, void *buffer)
{
	int i;
	mdl_t *pmodel;
	mdl_t *pinmodel;
	stvert_t *pstverts;
	stvert_t *pinstverts;
	aliashdr_t *pheader;
	mtriangle_t *ptri;
	dtriangle_t *pintriangles;
	int version;
	int numframes;
	int numskins;
	int size;
	daliasframetype_t *pframetype;
	daliasskintype_t *pskintype;
	maliasskindesc_t *pskindesc;
	int skinsize;
	int start;
	int end;
	int total;

	start = Hunk_LowMark();
	pinmodel = (mdl_t *)buffer;
	version = LittleLong(pinmodel->version);

	if (version != ALIAS_MODEL_VERSION)
		Sys_Error("%s: %s has wrong version number (%i should be %i)", __func__, mod->name, version, ALIAS_MODEL_VERSION);

	// allocate space for a working header, plus all the data except the frames,
	// skin and group info
	size = sizeof(mtriangle_t) * LittleLong(pinmodel->numtris) +
		sizeof(stvert_t) * LittleLong(pinmodel->numverts) +
		sizeof(mdl_t) + sizeof(aliashdr_t) +
		sizeof(pheader->frames[0]) * (LittleLong(pinmodel->numframes) - 1);

	pheader = (aliashdr_t *)Hunk_AllocName(size, loadname);
	pmodel = (mdl_t *)(pheader->frames + LittleLong(pinmodel->numframes));
	mod->flags = LittleLong(pinmodel->flags);

	// endian-adjust and copy the data, starting with the alias model header
	pmodel->boundingradius = LittleFloat(pinmodel->boundingradius);
	pmodel->numskins = LittleLong(pinmodel->numskins);
	pmodel->skinwidth = LittleLong(pinmodel->skinwidth);
	pmodel->skinheight = LittleLong(pinmodel->skinheight);

	if (pmodel->skinheight > MAX_LBM_HEIGHT)
		Sys_Error("%s: model %s has a skin taller than %d", __func__, mod->name, MAX_LBM_HEIGHT);

	pmodel->numverts = LittleLong(pinmodel->numverts);

	if (pmodel->numverts <= 0)
		Sys_Error("%s: model %s has no vertices", __func__, mod->name);

	if (pmodel->numverts > MAX_ALIAS_MODEL_VERTS)
		Sys_Error("%s: model %s has too many vertices", __func__, mod->name);

	pmodel->numtris = LittleLong(pinmodel->numtris);

	if (pmodel->numtris <= 0)
		Sys_Error("%s: model %s has no triangles", __func__, mod->name);

	pmodel->numframes = LittleLong(pinmodel->numframes);
	pmodel->size = LittleFloat(pinmodel->size) * 0.09090909090909091;

	mod->synctype = (synctype_t)LittleLong(pinmodel->synctype);
	mod->numframes = pmodel->numframes;

	for (i = 0; i < 3; i++)
	{
		pmodel->scale[i] = LittleFloat(pinmodel->scale[i]);
		pmodel->scale_origin[i] = LittleFloat(pinmodel->scale_origin[i]);
		pmodel->eyeposition[i] = LittleFloat(pinmodel->eyeposition[i]);
	}

	numskins = pmodel->numskins;
	numframes = pmodel->numframes;

	if ((pmodel->skinwidth % 4) != 0)
		Sys_Error("%s: skinwidth not multiple of 4", __func__);

	pheader->model = (byte *)pmodel - (byte *)pheader;

	// load the skins
	skinsize = pmodel->skinwidth * pmodel->skinheight;

	if (numskins < 1)
		Sys_Error("%s: Invalid # of skins: %d\n", __func__, numskins);

	pskintype = (daliasskintype_t *)(pinmodel + 1);
	pskindesc = (maliasskindesc_t *)Hunk_AllocName(sizeof(maliasskindesc_t) * numskins, loadname);
	pheader->skindesc = (byte *)pskintype - (byte *)pheader;

	for (i = 0; i < numskins; i++)
	{
		aliasskintype_t skintype = (aliasskintype_t)LittleLong(pskintype->type);

		pskindesc[i].type = skintype;

		if (skintype == ALIAS_SKIN_SINGLE)
			pskintype = (daliasskintype_t *)Mod_LoadAliasSkin(pskintype + 1, &pskindesc[i].skin, skinsize, pheader);
		else
			pskintype = (daliasskintype_t *)Mod_LoadAliasSkinGroup(pskintype + 1, &pskindesc[i].skin, skinsize, pheader);
	}

	// set base s and t vertices
	pstverts = (stvert_t *)(pmodel + 1);
	pinstverts = (stvert_t *)pskintype;

	pheader->stverts = (byte *)pstverts - (byte *)pheader;
	for (i = 0; i < pmodel->numverts; i++)
	{
		pstverts[i].onseam = LittleLong(pinstverts[i].onseam);

		// put s and t in 16.16 format
		pstverts[i].s = (LittleLong(pinstverts[i].s) << 16);
		pstverts[i].t = (LittleLong(pinstverts[i].t) << 16);
	}

	// set up the triangles
	ptri = (mtriangle_t *)(pstverts + pmodel->numverts);
	pintriangles = (dtriangle_t *)(pinstverts + pmodel->numverts);

	pheader->triangles = (byte *)ptri - (byte *)pheader;
	for (i = 0; i < pmodel->numtris; i++)
	{
		ptri[i].facesfront = LittleLong(pintriangles[i].facesfront);

		for (int j = 0; j < 3; j++)
			ptri[i].vertindex[j] = LittleLong(pintriangles[i].vertindex[j]);
	}

	// load the frames
	if (numframes < 1)
		Sys_Error("%s: Invalid # of frames: %d\n", __func__, numframes);

	pframetype = (daliasframetype_t *)(pintriangles + pmodel->numtris);
	for (i = 0; i < numframes; i++)
	{
		aliasframetype_t frametype = (aliasframetype_t)LittleLong(pframetype->type);
		pheader->frames[i].type = frametype;

		if (frametype == ALIAS_SINGLE)
			pframetype = (daliasframetype_t *)Mod_LoadAliasFrame(pframetype + 1, &pheader->frames[i].frame, pmodel->numverts, &pheader->frames[i].bboxmin, &pheader->frames[i].bboxmax, pheader, pheader->frames[i].name);
		else
			pframetype = (daliasframetype_t *)Mod_LoadAliasGroup(pframetype + 1, &pheader->frames[i].frame, pmodel->numverts, &pheader->frames[i].bboxmin, &pheader->frames[i].bboxmax, pheader, pheader->frames[i].name);
	}

	mod->type = mod_alias;

	// FIXME: do this right
	mod->mins[0] = mod->mins[1] = mod->mins[2] = -16.0f;
	mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16.0f;

	PackedColorVec *pPal = (PackedColorVec *)Hunk_AllocName(sizeof(PackedColorVec) * 256, loadname);
	color24 *pPalSrc = (color24 *)pframetype;

	for (i = 0; i < 256; i++)
	{
		pPal[i].b = pPalSrc->r;
		pPal[i].g = pPalSrc->g;
		pPal[i].r = pPalSrc->b;
		pPal[i].a = 0;

		pPalSrc++;
	}

	pheader->palette = (byte *)pPal - (byte *)pheader;

	// move the complete, relocatable alias model to the cache
	end = Hunk_LowMark();
	total = end - start;
	Cache_Alloc(&mod->cache, total, loadname);

	if (mod->cache.data)
	{
		Q_memcpy(mod->cache.data, pheader, total);
		Hunk_FreeToLowMark(start);
	}
}

void *Mod_LoadSpriteFrame(void *pin, mspriteframe_t **ppframe)
{
	dspriteframe_t		*pinframe;
	mspriteframe_t		*pspriteframe;
	int					width, height, size, origin[2];
	unsigned short		*ppixout;
	byte				*ppixin;

	pinframe = (dspriteframe_t *)pin;

	width = LittleLong(pinframe->width);
	height = LittleLong(pinframe->height);
	size = width * height;

	pspriteframe = (mspriteframe_t*) Hunk_AllocName(sizeof(mspriteframe_t) + size * r_pixbytes, loadname);

	Q_memset(pspriteframe, 0, sizeof(mspriteframe_t) + size);
	*ppframe = pspriteframe;

	pspriteframe->width = width;
	pspriteframe->height = height;
	origin[0] = LittleLong(pinframe->origin[0]);
	origin[1] = LittleLong(pinframe->origin[1]);

	pspriteframe->up = (float)origin[1];
	pspriteframe->down = (float)(origin[1] - height);
	pspriteframe->left = (float)origin[0];
	pspriteframe->right = (float)(width + origin[0]);

	if (r_pixbytes == 1)
	{
		Q_memcpy(&pspriteframe->pixels[0], (byte *)(pinframe + 1), size);
	}
	else if (r_pixbytes == 2)
	{
		ppixin = (byte *)(pinframe + 1);
		ppixout = (unsigned short *)&pspriteframe->pixels[0];

		/*
		//seems to be disabled on server
		for (i = 0; i < size; i++)
			ppixout[i] = d_8to16table[ppixin[i]];
		*/

	}
	else
	{
		Sys_Error("%s: driver set invalid r_pixbytes: %d\n", __func__, r_pixbytes);
	}

	return (void *)((byte *)pinframe + sizeof(dspriteframe_t) + size);
}

void *Mod_LoadSpriteGroup(void *pin, mspriteframe_t **ppframe)
{
	dspritegroup_t		*pingroup;
	mspritegroup_t		*pspritegroup;
	int					i, numframes;
	dspriteinterval_t	*pin_intervals;
	float				*poutintervals;
	void				*ptemp;

	pingroup = (dspritegroup_t *)pin;
	numframes = LittleLong(pingroup->numframes);

	pspritegroup = (mspritegroup_t*)Hunk_AllocName(sizeof(mspritegroup_t) + (numframes - 1) * sizeof(pspritegroup->frames[0]), loadname);

	pspritegroup->numframes = numframes;

	*ppframe = (mspriteframe_t *)pspritegroup;

	pin_intervals = (dspriteinterval_t *)(pingroup + 1);

	poutintervals = (float*)Hunk_AllocName(numframes * sizeof(float), loadname);

	pspritegroup->intervals = poutintervals;

	for (i = 0; i < numframes; i++)
	{
		*poutintervals = LittleFloat(pin_intervals->interval);
		if (*poutintervals <= 0.0f)
			Sys_Error("%s: interval<=0", __func__);

		poutintervals++;
		pin_intervals++;
	}

	ptemp = (void *)pin_intervals;

	for (i = 0; i < numframes; i++)
	{
		ptemp = Mod_LoadSpriteFrame(ptemp, &pspritegroup->frames[i]);
	}

	return ptemp;
}

void Mod_LoadSpriteModel(model_t *mod, void *buffer)
{
	int					i;
	int					version;
	dsprite_t			*pin;
	msprite_t			*psprite;
	int					numframes;
	int					size;
	dspriteframetype_t	*pframetype;

	pin = (dsprite_t *)buffer;

	version = LittleLong(pin->version);
	if (version != SPRITE_VERSION)
		Sys_Error("%s: %s has wrong version number (%i should be %i)", __func__, mod->name, version, SPRITE_VERSION);

	numframes = LittleLong(pin->numframes);
	int palsize = *(uint16*)&pin[1];
	size = sizeof(msprite_t) + (numframes - 1) * sizeof(psprite->frames) + 2 + 8 * palsize;

	psprite = (msprite_t*) Hunk_AllocName(size, loadname);

	mod->cache.data = psprite;

	psprite->type = LittleLong(pin->type);
	psprite->maxwidth = LittleLong(pin->width);
	psprite->maxheight = LittleLong(pin->height);
	psprite->beamlength = LittleFloat(pin->beamlength);
	mod->synctype = (synctype_t) LittleLong(pin->synctype);
	psprite->numframes = numframes;

	mod->mins[0] = mod->mins[1] = float(-psprite->maxwidth / 2);
	mod->maxs[0] = mod->maxs[1] = float(psprite->maxwidth / 2);
	mod->mins[2] = float (- psprite->maxheight / 2);
	mod->maxs[2] = float(psprite->maxheight / 2);

	unsigned char *palsrc = (unsigned char*)(&pin[1]) + 2; //header + palette size
	uint16* paldest = (uint16*)((char*)(psprite + 1) + sizeof(psprite->frames) * (numframes - 1)); //sprite + [frames-1]
	*(paldest++) = palsize; //write palette size
	for (i = 0; i < palsize; i++, paldest += 4, palsrc += 3)
	{
		paldest[3] = 0;
		paldest[0] = palsrc[0];
		paldest[1] = palsrc[1];
		paldest[2] = palsrc[2];
	}

	//
	// load the frames
	//
	if (numframes < 1)
		Sys_Error("%s: Invalid # of frames: %d\n", __func__, numframes);

	mod->numframes = numframes;
	mod->flags = 0;

	pframetype = (dspriteframetype_t *)((char*)(pin + 1) + 2 + 3*palsize);

	for (i = 0; i < numframes; i++)
	{
		spriteframetype_t	frametype;

		frametype = (spriteframetype_t) LittleLong(pframetype->type);
		psprite->frames[i].type = frametype;

		if (frametype == SPR_SINGLE)
		{
			pframetype = (dspriteframetype_t *)
				Mod_LoadSpriteFrame(pframetype + 1,
				&psprite->frames[i].frameptr);
		}
		else
		{
			pframetype = (dspriteframetype_t *)
				Mod_LoadSpriteGroup(pframetype + 1,
				&psprite->frames[i].frameptr);
		}
	}

	mod->type = mod_sprite;
}

NOXREF void Mod_UnloadSpriteTextures(model_t *pModel)
{
	NOXREFCHECK;
	if (!pModel)
		return;

	if (pModel->type == mod_sprite)
	{
		pModel->needload = NL_NEEDS_LOADED;
#ifndef SWDS
		char name[256];
		msprite_t *spt = (msprite_t *)pModel->cache.data;
		if (spt)
		{
			for (int i = 0; i < spt->numframes; i++)
			{
				Q_sprintf(name, "%s_%i", pModel->name, i);
				GL_UnloadTexture(name);
			}
		}
#endif // SWDS
	}
}

void Mod_Print(void)
{
	int i;
	model_t *mod;
	Con_Printf("Cached models:\n");
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		Con_Printf("%8p : %s", mod->cache.data, mod->name);
		if (mod->needload & NL_UNREFERENCED)
			Con_Printf(" (!R)");
		if (mod->needload & NL_NEEDS_LOADED)
			Con_Printf(" (!P)");
		Con_Printf("\n");
	}
}

NOXREF void Mod_ChangeGame(void)
{
	NOXREFCHECK;
	int i;
	model_t *mod;
	mod_known_info_t *p;

	for (i = 0; i < mod_numknown; i++)
	{
		mod = &mod_known[i];

		if (mod->type == mod_studio)
		{
			if (Cache_Check(&mod->cache))
				Cache_Free(&mod->cache);
		}

		p = &mod_known_info[i];

		p->firstCRCDone = FALSE;
		p->initialCRC = 0;
	}
}

model_t *Mod_Handle(int modelindex)
{
#ifdef REHLDS_FIXES
	if (modelindex < 0 || modelindex > MAX_MODELS - 1) {
		Sys_Error("%s: bad modelindex #%i\n", __func__, modelindex);
	}
#endif

	return g_psv.models[modelindex];
}

modtype_t Mod_GetType(int modelindex)
{
	model_t *mod = Mod_Handle(modelindex);
	if (!mod) {
		return mod_bad;
	}

	return mod->type;
}
