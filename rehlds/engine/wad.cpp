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

wadlist_t wads[NUM_WADS];

void W_CleanupName(char *in, char *out)
{
	int i;
	int c;

	for (i = 0; i < 16; i++)
	{
		c = in[i];
		if (!c)
			break;
		if (c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
		out[i] = c;
	}
	if (i < 16)
		Q_memset(&out[i], 0, 16 - i);
}

int W_LoadWadFile(char *filename)
{
	int slot = 0;
	for (; slot < NUM_WADS; slot++)
	{
		if (!wads[slot].loaded)
			break;
	}

	if (slot >= NUM_WADS)
	{
		Con_Printf("No room for wad %s\n", filename);
		return -1;
	}

	wadlist_t *wad = &wads[slot];

	wad->wad_base = (byte *)COM_LoadHunkFile(filename);
	if (!wad->wad_base)
	{
		if (!slot)
			Sys_Error("%s: couldn't load %s", __func__, filename);
		Con_Printf("WARNING:  %s, couldn't load %s\n", __func__, filename);
		return -1;
	}

	Q_strncpy(wad->wadname, filename, sizeof(wad->wadname) - 1);
	wadinfo_t *header = (wadinfo_t *)wad->wad_base;
	wad->wadname[sizeof(wad->wadname) - 1] = 0;
	wad->loaded = TRUE;
	if (*(uint32 *)header->identification != MAKEID('W', 'A', 'D', '3'))
		Sys_Error("%s: Wad file %s doesn't have WAD3 id\n", __func__, filename);
	wad->wad_numlumps = LittleLong(header->numlumps);
	int infotableofs = LittleLong(header->infotableofs);

	// Validate the header and lump table against the actual file size to
	// prevent OOB reads when swapping lumps / pictures below.
	int wadsize = COM_FileSize(filename);
	if (wadsize < (int)sizeof(wadinfo_t) || wad->wad_numlumps < 0 ||
		infotableofs < (int)sizeof(wadinfo_t) ||
		(size_t)infotableofs + (size_t)wad->wad_numlumps * sizeof(lumpinfo_t) > (size_t)wadsize)
	{
		Sys_Error("%s: Wad file %s has invalid lump table (numlumps %i, infotableofs %i, file size %i)\n",
			__func__, filename, wad->wad_numlumps, infotableofs, wadsize);
	}

	lumpinfo_t * lump_p = (lumpinfo_t *)&wad->wad_base[infotableofs];
	wad->wad_lumps = lump_p;

	for (int i = 0; i < wad->wad_numlumps; ++i, ++lump_p)
	{
		int filepos = LittleLong(lump_p->filepos);
		int size = LittleLong(lump_p->size);
		if (filepos < 0 || size < 0 || (size_t)filepos + (size_t)size > (size_t)wadsize)
			Sys_Error("%s: Wad file %s lump #%i has invalid data range (filepos %i, size %i, file size %i)\n",
				__func__, filename, i, filepos, size, wadsize);
		lump_p->filepos = filepos;
		lump_p->size = size;
		W_CleanupName(lump_p->name, lump_p->name);
		if (lump_p->type == 66)
			SwapPic((qpic_t *)&wad->wad_base[lump_p->filepos]);
	}

	return slot;
}

lumpinfo_t *W_GetLumpinfo(int wad, char *name, qboolean doerror)
{
	int i;
	lumpinfo_t *lump_p;
	char clean[16];

	W_CleanupName(name, clean);
	lump_p = wads[wad].wad_lumps;
	for (i = 0; i < wads[wad].wad_numlumps; i++)
	{
		if (!Q_strcmp(clean, lump_p->name))
			return lump_p;
		lump_p++;
	}

	if (doerror)
		Sys_Error("%s: %s not found", __func__, name);

	return NULL;
}

void *W_GetLumpName(int wad, char *name)
{
	lumpinfo_t *lump = W_GetLumpinfo(wad, name, TRUE);
	if (lump != NULL)
		return &wads[wad].wad_base[lump->filepos];
	return NULL;
}

NOXREF void *W_GetLumpNum(int wad, int num)
{
	NOXREFCHECK;
	lumpinfo_t *lump;
	if (num < 0 || num > wads[wad].wad_numlumps)
		Sys_Error("%s: bad number: %i", __func__, num);

	lump = wads[wad].wad_lumps;
	return (void *)&wads[wad].wad_base[lump->filepos];
}

void W_Shutdown(void)
{
	for (int slot = 0; slot < NUM_WADS; slot++)
	{
		wadlist_t *wad = &wads[slot];
		if (!wad->loaded)
			break;

		Q_memset(wad, 0, sizeof(wadlist_t));
	}
}

void SwapPic(qpic_t *pic)
{
	pic->width = LittleLong(pic->width);
	pic->height = LittleLong(pic->height);
}
