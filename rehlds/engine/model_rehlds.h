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

/*

#include "const.h"
#include "d_local.h"
#include "modelgen.h"

#include "quakedef.h"
#include "spritegn.h"
#include "crc.h"
*/

#include "model.h"

#include "maintypes.h"
#include "bspfile.h"
#include "crc.h"

extern model_t* loadmodel;
extern char loadname[MAX_MODEL_NAME];
extern model_t mod_known[MAX_KNOWN_MODELS];
extern int mod_numknown;
extern unsigned char* mod_base;
extern int mod_filelength;
extern char *wadpath;
extern int tested;
extern int ad_enabled;
extern cachewad_t ad_wad;
extern mod_known_info_t mod_known_info[MAX_KNOWN_MODELS];

void SW_Mod_Init(void);
void *Mod_Extradata(model_t *mod);
mleaf_t *Mod_PointInLeaf(vec_t *p, model_t *model);
void Mod_ClearAll(void);
void Mod_FillInCRCInfo(qboolean trackCRC, int model_number);
model_t *Mod_FindName(qboolean trackCRC, const char *name);
NOXREF qboolean Mod_ValidateCRC(const char *name, CRC32_t crc);
NOXREF void Mod_NeedCRC(const char *name, qboolean needCRC);
model_t *Mod_LoadModel(model_t *mod, qboolean crash, qboolean trackCRC);
NOXREF void Mod_MarkClient(model_t *pModel);
model_t *Mod_ForName(const char *name, qboolean crash, qboolean trackCRC);
void Mod_AdInit(void);
void Mod_AdSwap(texture_t *src, int pixels, int entries);
void Mod_LoadTextures(lump_t *l);
void Mod_LoadLighting(lump_t *l);
void Mod_LoadVisibility(lump_t *l);
void Mod_LoadEntities(lump_t *l);
void Mod_LoadVertexes(lump_t *l);
void Mod_LoadSubmodels(lump_t *l);
void Mod_LoadEdges(lump_t *l);
void Mod_LoadTexinfo(lump_t *l);
void CalcSurfaceExtents(msurface_t *s);
void Mod_LoadFaces(lump_t *l);
void Mod_SetParent(mnode_t *node, mnode_t *parent);
void Mod_LoadNodes(lump_t *l);
void Mod_LoadLeafs(lump_t *l);
void Mod_LoadClipnodes(lump_t *l);
void Mod_MakeHull0(void);
void Mod_LoadMarksurfaces(lump_t *l);
void Mod_LoadSurfedges(lump_t *l);
void Mod_LoadPlanes(lump_t *l);
float RadiusFromBounds(vec_t *mins, vec_t *maxs);
void Mod_LoadBrushModel(model_t *mod, void *buffer);
void Mod_LoadBrushModel_internal(model_t *mod, void *buffer);
NOXREF void *Mod_LoadAliasFrame(void *pin, int *pframeindex, int numv, trivertx_t *pbboxmin, trivertx_t *pbboxmax, aliashdr_t *pheader, char *name);
NOXREF void *Mod_LoadAliasGroup(void *pin, int *pframeindex, int numv, trivertx_t *pbboxmin, trivertx_t *pbboxmax, aliashdr_t *pheader, char *name);
NOXREF void *Mod_LoadAliasSkin(void *pin, int *pskinindex, int skinsize, aliashdr_t *pheader);
NOXREF void *Mod_LoadAliasSkinGroup(void *pin, int *pskinindex, int skinsize, aliashdr_t *pheader);
NOXREF void Mod_LoadAliasModel(model_t *mod, void *buffer);
void *Mod_LoadSpriteFrame(void *pin, mspriteframe_t **ppframe);
void *Mod_LoadSpriteGroup(void *pin, mspriteframe_t **ppframe);
void Mod_LoadSpriteModel(model_t *mod, void *buffer);
NOXREF void Mod_UnloadSpriteTextures(model_t *pModel);
void Mod_Print(void);
NOXREF void Mod_ChangeGame(void);
model_t *Mod_Handle(int modelindex);
modtype_t Mod_GetType(int modelindex);
