
#include <game/server/gamecontext.h>
#include <game/server/entities/projectile.h>

#include "skeleton_small.h"

#define NPC_MOVE_FARWORD_PREDICT 5

CSkeletonSmall::CSkeletonSmall(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos)
		: CEnemyNpc(pGameServer, pMap, pNpcHolder, Pos, NPC_SKELETON_SMALL)
{

}

void CSkeletonSmall::SubTick()
{
	bool DayTime = GameServer()->TimeBetween(5, 40, 18, 30);
	if(DayTime)//zombies do not like light
		OnDeath(-1, WEAPON_WORLD, vec2(0, 0));
}