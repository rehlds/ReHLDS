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

#include "maintypes.h"

// Max key/value length (with a NULL char)
const int MAX_KV_LEN = 127;

// Key + value + 2 x slash + NULL
const int MAX_INFO_STRING = 256;

const int INFO_MAX_BUFFER_VALUES = 4;

#ifdef REHLDS_FIXES
const int MAX_LOCALINFO = 4096;
#else
const int MAX_LOCALINFO = MAX_INFO_STRING * 128;
#endif // REHLDS_FIXES

const char *Info_ValueForKey(const char *s, const char *key);
void Info_RemoveKey(char *s, const char *key);
void Info_RemovePrefixedKeys(char *s, const char prefix);
qboolean Info_IsKeyImportant(const char *key);
const char *Info_FindLargestKey(const char *s, int maxsize);
qboolean Info_SetValueForStarKey(char *s, const char *key, const char *value, size_t maxsize);
void Info_SetValueForKey(char *s, const char *key, const char *value, int maxsize);
void Info_Print(const char *s);
qboolean Info_IsValid(const char *s);
#ifdef REHLDS_FIXES
void Info_SetFieldsToTransmit();
void Info_CollectFields(char *destInfo, const char *srcInfo, size_t maxsize);
#endif
