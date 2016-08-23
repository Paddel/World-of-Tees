
#include <game/server/gamecontext.h>

#include "zombie_small.h"

CZombieSmall::CZombieSmall(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos)
		: CEnemyNpc(pGameServer, pMap, pNpcHolder, Pos, NPC_ZOMBIE_SMALL)
{

}

void CZombieSmall::SubTick()
{
	bool DayTime = GameServer()->TimeBetween(5, 40, 18, 30);
	if(DayTime)//zombies do not like light
		OnDeath(-1, WEAPON_WORLD, vec2(0, 0));
}
