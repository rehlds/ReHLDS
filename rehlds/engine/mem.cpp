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

void *Mem_Malloc(size_t size)
{
	return malloc(size);
}

void *Mem_ZeroMalloc(size_t size)
{
	void *p = malloc(size);
	if (p)
		Q_memset(p, 0, size);
	return p;
}

void *Mem_Realloc(void *memblock, size_t size)
{
	return realloc(memblock, size);
}

void* EXT_FUNC Mem_Calloc(int num, size_t size)
{
	return calloc(num, size);
}

char *Mem_Strdup(const char *strSource)
{
	return _strdup(strSource);
}

void Mem_Free(void *p)
{
	free(p);
}
