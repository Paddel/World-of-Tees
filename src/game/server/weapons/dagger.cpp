
#include <game/balancing.h>
#include <game/server/gamecontext.h>
#include <game/server/targetable.h>
#include <game/server/entities/character.h>

#include "dagger.h"

CDagger::CDagger(CGameContext *pGameServer, CTargetAble *pOwner, int Damage)
	: CWeapon(pGameServer, pOwner, WEAP_DAGGER, Damage, -1)
{
}

int CDagger::FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos)
{
	int PlayerID = GameServer()->m_World.GetPlayerID(Owner());
	CTargetAble *pTarget = GameServer()->m_World.FindClosestTarget(ProjPos, 32, Owner(), false, Owner()->TargetMap());
	if(pTarget && pTarget->IsEnemy(Owner()))
	{
		pTarget->TakeDamage(vec2(0, 0), Damage(), PlayerID, GetType());
		GameServer()->CreateSound(PlayerPos, SOUND_NINJA_HIT, Owner()->TargetMap());
		return 300;
	}
	else
		GameServer()->CreateSound(PlayerPos, SOUND_HAMMER_FIRE, Owner()->TargetMap());
				
	return 250;
}