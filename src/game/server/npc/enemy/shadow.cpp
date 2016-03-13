
#include <game/server/gamecontext.h>

#include "shadow.h"

CShadow::CShadow(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos)
		: CEnemyNpc(pGameServer, pMap, pNpcHolder, Pos, NPC_SHADOW)
{
	m_SpawnTime = Server()->Tick();
}

void CShadow::SubTick()
{
	bool DayTime = GameServer()->TimeBetween(5, 40, 18, 30);
	if(DayTime)//zombies do not like light
		OnDeath(-1, WEAPON_WORLD, vec2(0, 0));

	if(m_pTarget && m_TeleportTime < Server()->Tick())
	{
		vec2 TargetPos = m_pTarget->GetClosestPos(GetPos());
		float dist = distance(GetPos(), TargetPos);

		if(dist > 90)
		{
			bool Teleport = true;
			vec2 TelePos = dist > 300?GetPos()+normalize(TargetPos-GetPos())*300:TargetPos-normalize(TargetPos-GetPos())*30;
			if(Map()->Collision()->IntersectBox(GetPos(), TelePos, 28, NULL, &TelePos))
			{
				if(distance(GetPos(), TelePos) < 100)
					Teleport = false;
			}

			if(Teleport)
			{
				GameServer()->CreatePlayerSpawn(GetPos(), Map());
				GameServer()->CreatePlayerSpawn(TelePos, Map());
				SetEmote(EMOTE_ANGRY, Server()->Tick()+Server()->TickSpeed()*3.0f);
				m_Core.m_Vel = normalize(TelePos-GetPos())*30.0f;
				m_Core.m_Pos = TelePos;
			}
		}

		m_TeleportTime = Server()->Tick()+Server()->TickSpeed()*0.7f;
	}
}

int CShadow::SkinColorBody()
{
	return sin(m_SpawnTime+Server()->Tick()*0.1f)*40+40;
}