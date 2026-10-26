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

CRehldsStudio g_RehldsStudio;

int EXT_FUNC CRehldsStudio::SetupBones(const edict_t *entity, studio_bonematrix_t *bones, int maxBones)
{
	const studiohdr_t *pStudioHdr = SV_StudioSetupTraceBones(entity);
	if (!pStudioHdr || !bones || maxBones <= 0)
		return 0;

	const int count = Q_min(pStudioHdr->numbones, maxBones);
	Q_memcpy(bones, bonetransform, count * sizeof(bonetransform[0]));
	return count;
}

int EXT_FUNC CRehldsStudio::GetHitboxes(const edict_t *entity, studio_hitbox_t *hitboxes, int maxHitboxes)
{
	const studiohdr_t *pStudioHdr = SV_StudioSetupTraceBones(entity);
	if (!pStudioHdr || !hitboxes || maxHitboxes <= 0)
		return 0;

	const bool bSkipShield = SV_StudioSkipsShield(entity);
	const int iShield = SV_StudioShieldHitbox();

	const mstudiobbox_t *pbbox = (const mstudiobbox_t *)((const char *)pStudioHdr + pStudioHdr->hitboxindex);

	int count = 0;
	for (int i = 0; i < pStudioHdr->numhitboxes && count < maxHitboxes; i++)
	{
		if (bSkipShield && i == iShield)
			continue;

		studio_hitbox_t *pHitbox = &hitboxes[count++];
		pHitbox->index = i;
		pHitbox->bone = pbbox[i].bone;
		pHitbox->group = pbbox[i].group;
		VectorCopy(pbbox[i].bbmin, pHitbox->mins);
		VectorCopy(pbbox[i].bbmax, pHitbox->maxs);
		Q_memcpy(pHitbox->transform, bonetransform[pbbox[i].bone], sizeof(pHitbox->transform));
	}

	return count;
}

studio_bonematrix_t *EXT_FUNC CRehldsStudio::GetBoneTransform()
{
	return bonetransform;
}
