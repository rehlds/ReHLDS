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

// The transform of a bone to the world
typedef float studio_bonematrix_t[3][4];

// A hitbox in the pose traces take against the entity: the box lies between mins and maxs
// in the space of its bone, the transform takes that space to the world
struct studio_hitbox_t
{
	int		index;				// in the model; traces of CS skip the shield of a player without one
	int		bone;
	int		group;				// hitgroup
	vec3_t	mins;
	vec3_t	maxs;
	studio_bonematrix_t transform;
};

// The bones and hitboxes of entities with studio models, as the blending interface the game
// gave the engine sets them up
class IRehldsStudio
{
public:
	virtual ~IRehldsStudio() {}

	// The bones in the world in the pose traces take against the entity: a player is posed as
	// for a trace, with its pitch in the blend. The number of bones written, 0 for an entity
	// without a studio model
	virtual int SetupBones(const edict_t *entity, studio_bonematrix_t *bones, int maxBones) = 0;

	// The hitboxes a trace meets now; during the command of a player the others are where lag
	// compensation put them. The number written, 0 for an entity without a studio model
	virtual int GetHitboxes(const edict_t *entity, studio_hitbox_t *hitboxes, int maxHitboxes) = 0;

	// The bones of the last setup, whoever asked for it; a hook of SV_StudioSetupBones reads and
	// may change them after the original
	virtual studio_bonematrix_t *GetBoneTransform() = 0;
};
