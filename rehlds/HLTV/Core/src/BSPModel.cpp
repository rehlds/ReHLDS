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

void BSPModel::Init(IBaseSystem *system)
{
	m_System = system;
	m_visframecount = 0;
	m_currentLeaf = nullptr;
	m_base = nullptr;
	m_wadpath = nullptr;
	m_IsMinimal = false;

	Q_memset(&m_model, 0, sizeof(m_model));
	Q_memset(m_novis, 0xFF, sizeof(m_novis));
}

bool BSPModel::Load(const char *name, bool minimal)
{
	int length;
	unsigned int *buffer = (unsigned int *)m_System->LoadFile(name, &length);
	if (!buffer) {
		return false;
	}

	m_IsMinimal = minimal;
	return LoadFromBuffer(buffer, length, COM_SkipPath((char *)name));
}

void BSPModel::LoadLeafs(lump_t *l)
{
	dleaf_t *in;
	mleaf_t *out;
	int i, j, count, p;

	in = (dleaf_t *)(m_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		m_System->Errorf("BSPModel::LoadLeafs: funny lump size in %s\n", m_model.name);
	}

	count = l->filelen / sizeof(*in);
	out = (mleaf_t *)Mem_ZeroMalloc(count * sizeof(*out));

	if (!out) {
		m_System->Errorf("BSPModel::LoadLeafs: not enough memory to load leafs from %s\n", m_model.name);
	}

	m_model.leafs = out;
	m_model.numleafs = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = _LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = _LittleShort(in->maxs[j]);
		}

		out->contents = _LittleLong(in->contents);
		out->firstmarksurface = m_model.marksurfaces + _LittleShort(in->firstmarksurface);
		out->nummarksurfaces = _LittleShort(in->nummarksurfaces);

		p = _LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = nullptr;
		else
			out->compressed_vis = m_model.visdata + p;

		out->efrags = nullptr;

		for (j = 0; j < 4; j++) {
			out->ambient_sound_level[j] = in->ambient_level[j];
		}
	}
}

void BSPModel::LoadNodes(lump_t *l)
{
	int i, j, count, p;
	dnode_t *in;
	mnode_t *out;

	in = (dnode_t *)(m_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		m_System->Errorf("BSPModel::LoadNodes: funny lump size in %s\n", m_model.name);
	}

	count = l->filelen / sizeof(*in);
	out = (mnode_t *)Mem_ZeroMalloc(count * sizeof(*out));
	if (!out) {
		m_System->Errorf("BSPModel::LoadNodes: not enough memory to load nodes from %s\n", m_model.name);
		return;
	}

	m_model.nodes = out;
	m_model.numnodes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = _LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = _LittleShort(in->maxs[j]);
		}

		out->plane = m_model.planes + _LittleLong(in->planenum);
		out->firstsurface = _LittleShort(in->firstface);
		out->numsurfaces = _LittleShort(in->numfaces);

		for (j = 0; j < 2; j++)
		{
			p = _LittleShort(in->children[j]);
			if (p >= 0)
				out->children[j] = m_model.nodes + p;
			else
				out->children[j] = (mnode_t *)(m_model.leafs + (-1 - p));
		}
	}

	if (count) {
		// sets nodes and leafs
		SetParent(m_model.nodes, nullptr);
	}
}

void BSPModel::SetParent(mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents < 0)
		return;

	SetParent(node->children[0], node);
	SetParent(node->children[1], node);
}

byte *BSPModel::LeafPVS(mleaf_t *leaf)
{
	if (leaf == m_model.leafs) {
		return m_novis;
	}

	return DecompressVis(leaf->compressed_vis);
}

byte *BSPModel::DecompressVis(unsigned char *in)
{
	static unsigned char decompressed[MAX_MAP_LEAFS / 8];
	if (in == nullptr) {
		return m_novis;
	}

	DecompressPVS(in, decompressed, (m_model.numleafs + 7) / 8);
	return decompressed;
}

void BSPModel::DecompressPVS(unsigned char *in, unsigned char *decompressed, int byteCount)
{
	int c;
	unsigned char *out;

	if (in == nullptr)
	{
		// Make all visible
		Q_memcpy(decompressed, m_novis, byteCount);
		return;
	}

	out = decompressed;
	while (out < decompressed + byteCount)
	{
		// Non zero is not copmpressed
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];	// TODO: Check that input buffer is correct (last byte on the buffer could be zero and we will go out of the buffer - check the model)
		in += 2;

		// Prevent buffer overrun
		if (c > decompressed + byteCount - out)
		{
			c = decompressed + byteCount - out;
		}

		// Unpack zeros
		Q_memset(out, 0, c);
		out += c;
	}
}

mleaf_t *BSPModel::PointInLeaf(vec_t *point)
{
	mnode_t *node;
	float d;
	mplane_t *plane;

	if (!m_model.nodes) {
		m_System->Errorf("BSPModel::PointInLeaf: bad model, no leafs.\n");
		return nullptr;
	}

	node = m_model.nodes;
	while (node->contents >= 0)
	{
		plane = node->plane;
		if (plane->type >= 3u)
			d = DotProduct(plane->normal, point) - plane->dist;
		else
			d = point[plane->type] - plane->dist;

		node = node->children[(d <= 0) ? 1 : 0];
	}

	return (mleaf_t *)node;
}

void BSPModel::PVSMark(unsigned char *ppvs)
{
	m_visframecount++;

	for (int i = 0; i < m_model.numleafs - 1; i++)
	{
		if ((1 << (i & 7)) & ppvs[i >> 3])
		{
			mnode_t *node = (mnode_t *)&m_model.leafs[i + 1];
			do
			{
				if (node->visframe == m_visframecount)
					break;

				node->visframe = m_visframecount;
				node = node->parent;
			}
			while (node);
		}
	}
}

mnode_t *BSPModel::PVSNode(mnode_t *node, vec_t *emins, vec_t *emaxs)
{
	mplane_t *splitplane;
	int sides;
	mnode_t *splitNode;

	if (node->visframe != m_visframecount) {
		return nullptr;
	}

	if (node->contents < 0) {
		return node->contents != CONTENTS_SOLID ? node : nullptr;
	}

	splitplane = node->plane;
	if (splitplane->type >= 3)
	{
		sides = BoxOnPlaneSide(emins, emaxs, splitplane);
	}
	else
	{
		if (splitplane->dist > emins[splitplane->type])
		{
			if (splitplane->dist < emaxs[splitplane->type])
				sides = 3;
			else
				sides = 2;
		}
		else
		{
			sides = 1;
		}
	}

	if (sides & 1)
	{
		splitNode = PVSNode(node->children[0], emins, emaxs);
		if (splitNode)
			return splitNode;
	}

	if (sides & 2) {
		return PVSNode(node->children[1], emins, emaxs);
	}

	return nullptr;
}

void BSPModel::LoadPlanes(lump_t *l)
{
	int i, j;
	mplane_t *out;
	dplane_t *in;
	int count;
	int bits;

	in = (dplane_t *)(m_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		m_System->Errorf("BSPModel::LoadPlanes: funny lump size in %s\n", m_model.name);
		return;
	}

	count = l->filelen / sizeof(*in);
	out = (mplane_t *)Mem_ZeroMalloc(count * 2 * sizeof(*out));
	if (!out) {
		m_System->Errorf("BSPModel::LoadPlanes: not enough memory to load planes from %s\n", m_model.name);
		return;
	}

	m_model.planes = out;
	m_model.numplanes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		bits = 0;
		for (j = 0; j < 3; j++)
		{
			out->normal[j] = _LittleFloat(in->normal[j]);
			if (out->normal[j] < 0) {
				bits |= (1 << j);
			}
		}

		out->dist = _LittleFloat(in->dist);
		out->type = _LittleLong(in->type);
		out->signbits = bits;
	}
}

void BSPModel::LoadVisibility(lump_t *l)
{
	if (!l->filelen)
	{
		m_model.visdata = nullptr;
		return;
	}

	m_model.visdata = (byte *)Mem_ZeroMalloc(l->filelen);
	if (!m_model.visdata) {
		m_System->Errorf("BSPModel::LoadPlanes: not enough memory to load planes from %s\n", m_model.name);
		return;
	}

	Q_memcpy(m_model.visdata, m_base + l->fileofs, l->filelen);
}

bool BSPModel::IsValid()
{
	return m_model.name[0] != '\0';
}

void BSPModel::SetPVS(vec_t *point)
{
	unsigned char *ppvs;
	mleaf_t *pleaf = PointInLeaf(point);

	if (m_currentLeaf != pleaf)
	{
		ppvs = LeafPVS(pleaf);

		PVSMark(ppvs);
		m_currentLeaf = pleaf;
	}
}

bool BSPModel::InPVS(vec_t *point)
{
	vec3_t mins, maxs;
	for (int i = 0; i < 3; i++)
	{
		mins[i] = point[i] - 32;
		maxs[i] = point[i] + 32;
	}

	if (PVSNode(m_model.nodes, mins, maxs)) {
		return true;
	}

	return false;
}

void BSPModel::Free(void *ptr)
{
	if (ptr) {
		Mem_Free(ptr);
	}
}

void BSPModel::Clear()
{
	Free(m_model.leafs);
	Free(m_model.nodes);
	Free(m_model.planes);
	Free(m_model.visdata);
	Free(m_model.vertexes);
	Free(m_model.entities);
	Free(m_model.edges);
	Free(m_model.lightdata);
	Free(m_model.surfedges);
	Free(m_model.surfaces);
	Free(m_model.marksurfaces);
	Free(m_model.clipnodes);
	Free(m_model.hulls[point_hull].clipnodes);
	Free(m_model.texinfo);

	if (m_model.textures)
	{
		for (int i = 0; i < m_model.numtextures; ++i) {
			Free(m_model.textures[i]);
		}

		Free(m_model.textures);
	}

	Free(m_wadpath);

	Q_memset(&m_model, 0, sizeof(m_model));

	m_visframecount = 0;
	m_wadpath = nullptr;
	m_currentLeaf = nullptr;
}

void BSPModel::GetDimension(vec_t *min, vec_t *max)
{
	min[0] = MAX_WORLD_SIZE;
	min[1] = MAX_WORLD_SIZE;
	min[2] = MAX_WORLD_SIZE;

	max[0] = -MAX_WORLD_SIZE;
	max[1] = -MAX_WORLD_SIZE;
	max[2] = -MAX_WORLD_SIZE;

	for (int i = 0; i < m_model.numvertexes; i++)
	{
		vec3_t v;
		VectorCopy(m_model.vertexes[i].position, v);

		if (v[0] > max[0]) max[0] = v[0];
		if (v[1] > max[1]) max[1] = v[1];
		if (v[2] > max[2]) max[2] = v[2];
		if (v[0] < min[0]) min[0] = v[0];
		if (v[1] < min[1]) min[1] = v[1];
		if (v[2] < min[2]) min[2] = v[2];
	}
}

void BSPModel::LoadVertexes(lump_t *l)
{
	dvertex_t *in;
	mvertex_t *out;
	int i, count;

	in = (dvertex_t *)(m_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		m_System->Errorf("BSPModel::LoadVertexes: funny lump size in %s\n", m_model.name);
	}

	count = l->filelen / sizeof(*in);
	out = (mvertex_t *)Mem_ZeroMalloc(count * sizeof(*out));

	m_model.vertexes = out;
	m_model.numvertexes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->position[0] = _LittleFloat(in->point[0]);
		out->position[1] = _LittleFloat(in->point[1]);
		out->position[2] = _LittleFloat(in->point[2]);
	}
}

void BSPModel::MakeHull0()
{
	mnode_t *in, *child;
	dclipnode_t *out;
	int i, j, count;
	hull_t *hull;

	hull = &m_model.hulls[point_hull];

	in = m_model.nodes;
	count = m_model.numnodes;
	out = (dclipnode_t *)Mem_ZeroMalloc(count * sizeof(*out));

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = m_model.planes;

	for (i = 0; i < count; i++, out++, in++)
	{
		out->planenum = in->plane - m_model.planes;
		for (j = 0; j < 2; j++)
		{
			child = in->children[j];
			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - m_model.nodes;
		}
	}
}

bool BSPModel::TraceLine(vec_t *start, vec_t *end, vec_t *impact)
{
	trace_t trace;
	Q_memset(&trace, 0, sizeof(trace));
	auto res = RecursiveHullCheck(m_model.hulls, 0, 0, 1, start, end, &trace);
	VectorCopy(trace.endpos, impact);
	return res;
}

bool BSPModel::RecursiveHullCheck(hull_t *hull, int num, float p1f, float p2f, vec_t *p1, vec_t *p2, trace_t *trace)
{
	int i;
	dclipnode_t *node;
	mplane_t *plane;
	float t2;
	vec3_t mid;
	float frac;
	float t1;
	int side;
	float midf;
	float pdif = p2f - p1f;

	float DIST_EPSILON = 0.03125f;

	if (num >= 0)
	{
		if (num < hull->firstclipnode || num > hull->lastclipnode || !hull->planes) {
			m_System->Errorf("BSPModel::RecursiveHullCheck: bad node number\n");
		}

		node = &hull->clipnodes[num];
		plane = &hull->planes[hull->clipnodes[num].planenum];
		if (plane->type >= 3)
		{
			t1 = DotProduct(p1, plane->normal) - plane->dist;
			t2 = DotProduct(p2, plane->normal) - plane->dist;
		}
		else
		{
			t1 = p1[plane->type] - plane->dist;
			t2 = p2[plane->type] - plane->dist;
		}

		if (t1 >= 0.0f && t2 >= 0.0f) {
			return RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2, trace);
		}

		if (t1 >= 0.0f)
		{
			midf = t1 - DIST_EPSILON;
		}
		else
		{
			if (t2 < 0.0f) {
				return RecursiveHullCheck(hull, node->children[1], p1f, p2f, p1, p2, trace);
			}

			midf = t1 + DIST_EPSILON;
		}

		midf = midf / (t1 - t2);
		midf = Q_clamp(midf, 0.0f, 1.0f);

		// not a number
		if (!IS_NAN(midf))
		{
			frac = pdif * midf + p1f;
			for (i = 0; i < 3; i++) {
				mid[i] = (p2[i] - p1[i]) * midf + p1[i];
			}
			side = (t1 < 0.0f) ? 1 : 0;

			if (RecursiveHullCheck(hull, node->children[side], p1f, frac, p1, mid, trace))
			{
				if (HullPointContents(hull, node->children[side ^ 1], mid) != CONTENTS_SOLID) {
					return RecursiveHullCheck(hull, node->children[side ^ 1], frac, p2f, mid, p2, trace);
				}

				if (!trace->allsolid)
				{
					if (side)
					{
						VectorSubtract(vec3_origin, plane->normal, trace->plane.normal);
						trace->plane.dist = -plane->dist;
					}
					else
					{
						VectorCopy(plane->normal, trace->plane.normal);
						trace->plane.dist = plane->dist;
					}

					while (HullPointContents(hull, hull->firstclipnode, mid) == CONTENTS_SOLID)
					{
						midf -= 0.1f;
						if (midf < 0.0f)
						{
							trace->fraction = frac;
							VectorCopy(mid, trace->endpos);
							m_System->Printf("BSPModel::RecursiveHullCheck: backup past 0\n");
							return false;
						}

						frac = pdif * midf + p1f;
						for (i = 0; i < 3; i++) {
							mid[i] = (p2[i] - p1[i]) * midf + p1[i];
						}
					}

					trace->fraction = frac;
					VectorCopy(mid, trace->endpos);
					m_System->Printf("BSPModel::RecursiveHullCheck: backup past 0\n");
					return false;
				}
			}
		}

		return false;
	}

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

		else if (num != CONTENTS_TRANSLUCENT)
		{
			trace->inwater = TRUE;
		}
	}

	return true;
}

int BSPModel::HullPointContents(hull_t *hull, int num, const vec_t *point)
{
	float d;
	dclipnode_t *node;
	mplane_t *plane;

	while (num >= 0)
	{
		if (num < hull->firstclipnode || num > hull->lastclipnode) {
			m_System->Errorf("BSPModel::HullPointContents: bad node number\n");
		}

		node = &hull->clipnodes[num];
		plane = &hull->planes[node->planenum];

		if (plane->type >= 3)
			d = DotProduct(point, plane->normal) - plane->dist;
		else
			d = point[plane->type] - plane->dist;

		num = node->children[(d < 0) ? 1 : 0];
	}

	return num;
}

bool BSPModel::LoadFromBuffer(unsigned int *buffer, int length, const char *name)
{
	dheader_t *header;
	int i;

	Clear();
	Q_strlcpy(m_model.name, name);

	m_System->DPrintf("Loading model: %s (%i kB)\n", name, length / 1024);
	m_model.needload = NL_PRESENT;

	// call the apropriate loader
	switch (_LittleLong(*(uint32 *)buffer))
	{
	case IDPOLYHEADER:	// old-format of the model from the quake1
	case IDSPRITEHEADER:
	case IDSTUDIOHEADER:
		m_System->Printf("ERROR! BSPModel::Load: only .BSP models supported.\n");
		if (buffer) {
			m_System->FreeFile((unsigned char *)buffer);
		}
		return false;
	default:
		break;
	}

	m_model.type = mod_brush;
	header = (dheader_t *)buffer;

	i = _LittleLong(header->version);
	if (
#ifdef HLTV_FIXES
		i != Q1BSP_VERSION &&
#endif
		i != HLBSP_VERSION)
	{
		m_System->Errorf("BSPModel::LoadFromBuffer: %s has wrong version number (%i should be %i)\n", m_model.name, i, HLBSP_VERSION);
	}

	// swap all the lumps
	m_base = (byte *)header;

	for (i = 0; i < sizeof(dheader_t) / 4; i++) {
		((int *)header)[i] = _LittleLong(((int *)header)[i]);
	}

	// load into heap
	if (!m_IsMinimal)
	{
		LoadVertexes(&header->lumps[LUMP_VERTEXES]);
		LoadEdges(&header->lumps[LUMP_EDGES]);
		LoadSurfedges(&header->lumps[LUMP_SURFEDGES]);
		LoadEntities(&header->lumps[LUMP_ENTITIES]);
		LoadTextures(&header->lumps[LUMP_TEXTURES]);
		LoadLighting(&header->lumps[LUMP_LIGHTING]);
	}

	LoadPlanes(&header->lumps[LUMP_PLANES]);
	if (!m_IsMinimal)
	{
		LoadTexinfo(&header->lumps[LUMP_TEXINFO]);
		LoadFaces(&header->lumps[LUMP_FACES]);
		LoadMarksurfaces(&header->lumps[LUMP_MARKSURFACES]);
	}

	LoadVisibility(&header->lumps[LUMP_VISIBILITY]);
	LoadLeafs(&header->lumps[LUMP_LEAFS]);
	LoadNodes(&header->lumps[LUMP_NODES]);

	if (!m_IsMinimal)
	{
		LoadClipnodes(&header->lumps[LUMP_CLIPNODES]);
	}

	MakeHull0();

	// regular and alternate animation
	m_model.numframes = 2;
	m_model.flags = 0;

	m_System->FreeFile((unsigned char *)buffer);
	return true;
}

bool BSPModel::IsMinimal()
{
	return m_IsMinimal;
}

void BSPModel::LoadEdges(lump_t *l)
{
	dedge_t *in;
	medge_t *out;
	int i, count;

	in = (dedge_t *)(m_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		m_System->Errorf("BSPModel::LoadEdges funny lump size in %s\n", m_model.name);
	}

	count = l->filelen / sizeof(*in);
	out = (medge_t *)Mem_ZeroMalloc((count + 1) * sizeof(*out));

	m_model.edges = out;
	m_model.numedges = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->v[0] = (unsigned short)_LittleShort(in->v[0]);
		out->v[1] = (unsigned short)_LittleShort(in->v[1]);
	}
}

void BSPModel::LoadSurfedges(lump_t *l)
{
	int i, count;
	int *in, *out;

	in = (int *)(m_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		m_System->Errorf("BSPModel::LoadSurfedges; funny lump size in %s\n", m_model.name);
	}

	count = l->filelen / sizeof(*in);
	out = (int *)Mem_ZeroMalloc(count * sizeof(*out));

	m_model.surfedges = out;
	m_model.numsurfedges = count;

	for (i = 0; i < count; i++) {
		out[i] = _LittleLong(in[i]);
	}
}

void BSPModel::LoadEntities(lump_t *l)
{
	if (!l->filelen) {
		m_model.entities = nullptr;
		return;
	}

	m_model.entities = (char *)Mem_ZeroMalloc(l->filelen);
	Q_memcpy(m_model.entities, (const void *)(m_base + l->fileofs), l->filelen);

	char *pszInputStream = COM_Parse(m_model.entities);
	if (*pszInputStream)
	{
		while (com_token[0] != '}')
		{
			if (!Q_strcmp(com_token, "wad"))
			{
				COM_Parse(pszInputStream);
				if (m_wadpath) {
					Mem_Free(m_wadpath);
				}

				m_wadpath = Q_strdup(com_token);
				return;
			}

			pszInputStream = COM_Parse(pszInputStream);
			if (!*pszInputStream)
				return;
		}
	}
}

void BSPModel::LoadLighting(lump_t *l)
{
	if (l->filelen)
	{
		m_model.lightdata = (color24 *)Mem_ZeroMalloc(l->filelen);
		Q_memcpy(m_model.lightdata, (const void *)(m_base + l->fileofs), l->filelen);
	}
	else
	{
		m_model.lightdata = nullptr;
	}
}

void BSPModel::LoadFaces(lump_t *l)
{
	dface_t *in;
	msurface_t *out;
	int i, count, surfnum;
	int planenum, side;

	in = (dface_t *)(m_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		m_System->Errorf("BSPModel::LoadFaces: funny lump size in %s\n", m_model.name);
	}

	count = l->filelen / sizeof(*in);
	out = (msurface_t *)Mem_ZeroMalloc(count * sizeof(*out));

	m_model.surfaces = out;
	m_model.numsurfaces = count;

	for (surfnum = 0; surfnum < count; surfnum++, in++, out++)
	{
		out->firstedge = _LittleLong(in->firstedge);
		out->numedges = _LittleShort(in->numedges);
		out->flags = 0;
		out->pdecals = nullptr;

		planenum = _LittleShort(in->planenum);
		side = _LittleShort(in->side);
		if (side) {
			out->flags |= SURF_PLANEBACK;
		}

		out->plane = m_model.planes + planenum;
		out->texinfo = m_model.texinfo + _LittleShort(in->texinfo);

		// lighting info
		for (i = 0; i < MAXLIGHTMAPS; i++) {
			out->styles[i] = in->styles[i];
		}

		i = _LittleLong(in->lightofs);
		if (i == -1)
			out->samples = nullptr;
		else
			out->samples = m_model.lightdata + i;

		if (out->texinfo->texture)
		{
			// set the drawing flags flag
			const char *texName = out->texinfo->texture->name;
			if (!Q_strncmp(texName, "sky", 3)) {
				out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
				continue;
			}

			if (!Q_strncmp(texName, "aaatrigger", 10)) {
				out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
				continue;
			}
		}
	}
}

void BSPModel::LoadMarksurfaces(lump_t *l)
{
	int i, j, count;
	short *in;
	msurface_t **out;

	in = (short *)(m_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		m_System->Errorf("BSPModel::LoadMarksurfaces: funny lump size in %s\n", m_model.name);
	}

	count = l->filelen / sizeof(*in);
	out = (msurface_t **)Mem_ZeroMalloc(count * sizeof(*out));

	m_model.marksurfaces = out;
	m_model.nummarksurfaces = count;

	for (i = 0; i < count; i++)
	{
		j = _LittleShort(in[i]);
		if (j >= m_model.numsurfaces) {
			m_System->Errorf("BSPModel::LoadMarksurfaces: bad surface number\n");
		}

		out[i] = m_model.surfaces + j;
	}
}

void BSPModel::LoadClipnodes(lump_t *l)
{
	dclipnode_t *in, *out;
	int i, count;
	hull_t *hull;

	in = (dclipnode_t *)(m_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		m_System->Errorf("BSPModel::LoadClipnodes: funny lump size in %s\n", m_model.name);
	}

	count = l->filelen / sizeof(*in);
	out = (dclipnode_t *)Mem_ZeroMalloc(count * sizeof(*out));

	m_model.clipnodes = out;
	m_model.numclipnodes = count;

	hull = &m_model.hulls[human_hull];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = m_model.planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -36;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 36;

	hull = &m_model.hulls[large_hull];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = m_model.planes;
	hull->clip_mins[0] = -32;
	hull->clip_mins[1] = -32;
	hull->clip_mins[2] = -32;
	hull->clip_maxs[0] = 32;
	hull->clip_maxs[1] = 32;
	hull->clip_maxs[2] = 32;

	hull = &m_model.hulls[head_hull];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = m_model.planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -18;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 18;

	for (i = 0; i < count; i++, out++, in++)
	{
		out->planenum = _LittleLong(in->planenum);
		out->children[0] = _LittleShort(in->children[0]);
		out->children[1] = _LittleShort(in->children[1]);
	}
}

int BSPModel::TruePointContents(vec_t *point)
{
	hull_t *hull = &m_model.hulls[point_hull];
	if (hull->firstclipnode >= hull->lastclipnode) {
		return CONTENTS_EMPTY;
	}

	return HullPointContents(hull, hull->firstclipnode, point);
}

void BSPModel::LoadTexinfo(lump_t *l)
{
	texinfo_t *in;
	mtexinfo_t *out;
	int i, j, count;
	int miptex;
	float len1, len2;

	in = (texinfo_t *)(m_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		m_System->Errorf("BSPModel::LoadTexinfo: funny lump size in %s\n", m_model.name);
	}

	count = l->filelen / sizeof(*in);
	out = (mtexinfo_t *)Mem_ZeroMalloc(count * sizeof(*out));

	if (!out) {
		m_System->Errorf("BSPModel::LoadTexinfo: not enough memory to load planes from %s\n", m_model.name);
		return;
	}

	m_model.texinfo = out;
	m_model.numtexinfo = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 4; j++)
		{
			out->vecs[0][j] = _LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = _LittleFloat(in->vecs[1][j]);
		}

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

		miptex = _LittleLong(in->_miptex);
		out->flags = _LittleLong(in->flags);

		if (!m_model.textures)
		{
			out->texture = nullptr;
			out->flags = 0;
		}
		else
		{
			if (miptex >= m_model.numtextures) {
				m_System->Errorf("BSPModel::LoadTexinfo: miptex >= m_model.numtextures\n");
				return;
			}

			out->texture = m_model.textures[miptex];
			if (!out->texture)
			{
				out->texture = nullptr; // texture not found
				out->flags = 0;
			}
		}
	}
}

void BSPModel::LoadTextures(lump_t *l)
{
	int i, j;
	int pixels;

	miptex_t *mt;
	texture_t *tx;
	dmiptexlump_t *m;

	if (!l->filelen) {
		m_model.textures = nullptr;
		return;
	}

	m = (dmiptexlump_t *)(m_base + l->fileofs);
	m->_nummiptex = _LittleLong(m->_nummiptex);

	m_model.numtextures = m->_nummiptex;
	m_model.textures = (texture_t **)Mem_ZeroMalloc(4 * m->_nummiptex);

	if (!m_model.textures) {
		m_System->Errorf("BSPModel::LoadTextures: not enough memory to load planes from %s\n", m_model.name);
		return;
	}

	for (i = 0; i < m->_nummiptex; i++)
	{
		m->dataofs[i] = _LittleLong(m->dataofs[i]);
		if (m->dataofs[i] == -1) {
			continue;
		}

		mt = (miptex_t *)((char *)m + m->dataofs[i]);
		for (j = 0; j < MIPLEVELS; j++) {
			mt->offsets[j] = _LittleLong(mt->offsets[j]);
		}

		mt->width = _LittleLong(mt->width);
		mt->height = _LittleLong(mt->height);
		if ((mt->width & 0xF) || (mt->height & 0xF)) {
			m_System->Errorf("BSPModel::LoadTextures: Texture %s is not 16 aligned\n", mt->name);
			return;
		}

		pixels = (mt->width * mt->height) / 64;
		tx = (texture_t *)Mem_ZeroMalloc(85 * pixels + 66);

		m_model.textures[i] = tx;
		Q_memcpy(tx->name, mt->name, sizeof(tx->name));

		if (Q_strchr(tx->name, '~')) {
			tx->name[2] = ' ';
		}

		tx->width = mt->width;
		tx->height = mt->height;

		m_System->Printf("Texture: %s %i,%i\n", tx->name, tx->width, tx->height);
	}
}
