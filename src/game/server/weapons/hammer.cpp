
#include <game/balancing.h>
#include <game/server/gamecontext.h>
#include <game/server/targetable.h>
#include <game/server/entities/character.h>

#include "hammer.h"

CHammer::CHammer(CGameContext *pGameServer, CTargetAble *pOwner, int Damage)
	: CWeapon(pGameServer, pOwner, WEAP_HAMMER, Damage, -1)
{
}

int CHammer::FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos)
{
	int PlayerID = GameServer()->m_World.GetPlayerID(Owner());
	// reset objects Hit
	GameServer()->CreateSound(PlayerPos, SOUND_HAMMER_FIRE, Owner()->TargetMap());

	CCharacter *apEnts[MAX_CLIENTS];
	int Hits = 0;
	bool BuildingHit = false;

	CWorldSection *pSection = Owner()->TargetMap()->WorldSection();
	for(int i = 0; i < pSection->m_lpTargetAbles.size(); i++)
	{
		CTargetAble *pTarget = pSection->m_lpTargetAbles[i];

		vec2 ClosestPos = pTarget->GetClosestPos(ProjPos);

		if(pTarget == Owner() ||
			pTarget->IsEnemy(Owner()) == false ||
			pTarget->GetDistance(ProjPos) > Owner()->GetProximityRadius()*0.5f+pTarget->GetProximityRadius())
			continue;
					
		if(pTarget->IsBuilding())
		{
			if(BuildingHit)
				continue;

			BuildingHit = true;
		}
		else
		{
			if(Owner()->TargetMap()->Collision()->IntersectLine(ProjPos, ClosestPos, NULL, NULL))
				continue;
		}

		// set his velocity to fast upward (for now)
		if(length(ClosestPos-ProjPos) > 0.0f)
			GameServer()->CreateHammerHit(ClosestPos-normalize(ClosestPos-ProjPos)*Owner()->GetProximityRadius()*0.5f, Owner()->TargetMap());
		else
			GameServer()->CreateHammerHit(ProjPos, Owner()->TargetMap());

		vec2 Dir;
		if (length(ClosestPos - PlayerPos) > 0.0f)
			Dir = normalize(ClosestPos - PlayerPos);
		else
			Dir = vec2(0.f, -1.f);

		pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, Damage(), PlayerID, GetType());
		Hits++;
	}

	// if we Hit anything, we have to wait for the reload
	if(Hits)
		return 300.0f;


	return g_pData->m_Weapons.m_aId[WEAPON_HAMMER].m_Firedelay;
}