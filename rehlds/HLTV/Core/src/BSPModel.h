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

#include <HLTV/IBSPModel.h>

#include "l_studio.h"
#include "edict.h"
#include "bspfile.h"

// values for model_t's needload
#define NL_PRESENT      0
#define NL_NEEDS_LOADED 1
#define NL_UNREFERENCED 2

const int MAX_WORLD_SIZE = 32000;

class IBaseSystem;
class BSPModel: public IBSPModel {
public:
	BSPModel() {}
	virtual ~BSPModel() { delete m_wadpath; }

	EXT_FUNC void Init(IBaseSystem *system);
	EXT_FUNC void Clear();
	EXT_FUNC bool Load(const char *name, bool minimal);
	EXT_FUNC bool IsValid();
	EXT_FUNC bool IsMinimal();
	EXT_FUNC void SetPVS(vec_t *point);
	EXT_FUNC bool InPVS(vec_t *point);
	EXT_FUNC bool TraceLine(vec_t *start, vec_t *end, vec_t *impact);
	EXT_FUNC int TruePointContents(vec_t *point);

private:
	void LoadTextures(lump_t *l);
	void LoadEdges(lump_t *l);
	bool LoadFromBuffer(unsigned int *buffer, int length, const char *name);
	void LoadLeafs(lump_t *l);
	void LoadNodes(lump_t *l);
	void LoadVisibility(lump_t *l);
	void LoadPlanes(lump_t *l);
	void LoadVertexes(lump_t *l);
	void LoadTexinfo(lump_t *l);
	void LoadClipnodes(lump_t *l);
	void LoadMarksurfaces(lump_t *l);
	void LoadFaces(lump_t *l);
	void LoadLighting(lump_t *l);
	void LoadEntities(lump_t *l);
	void LoadSurfedges(lump_t *l);
	int HullPointContents(hull_t *hull, int num, const vec_t *point);
	bool RecursiveHullCheck(hull_t *hull, int num, float p1f, float p2f, vec_t *p1, vec_t *p2, trace_t *trace);
	void MakeHull0();
	mleaf_t *PointInLeaf(vec_t *point);
	void GetDimension(vec_t *min, vec_t *max);
	mnode_t *PVSNode(mnode_t *node, vec_t *emins, vec_t *emaxs);
	void PVSMark(unsigned char *ppvs);
	unsigned char *LeafPVS(mleaf_t *leaf);
	void DecompressPVS(unsigned char *in, unsigned char *decompressed, int byteCount);
	unsigned char *DecompressVis(unsigned char *in);
	void SetParent(mnode_t *node, mnode_t *parent);
	void Free(void *ptr);

protected:
	model_t m_model;
	byte m_novis[MAX_MAP_LEAFS / 8];
	byte *m_base;

	int m_visframecount;
	mleaf_t *m_currentLeaf;
	bool m_IsMinimal;
	char *m_wadpath;
	IBaseSystem *m_System;
};
