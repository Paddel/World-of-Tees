
#include <game/server/gamecontext.h>

#include "healingstone.h"

CHealingStone::CHealingStone(CGameContext *pGameServer, CMap *pMap, vec2 Pos, int Height)
	:CTargetAble(&pGameServer->m_World, CTargetAble::TYPE_HEALINGSTONE, pMap)
{
	m_pGameServer = pGameServer;
	m_Pos = Pos;
	m_Height = Height;

	m_To = m_Pos - vec2(0, 32*Height);
}

vec2 CHealingStone::GetClosestPos(vec2 Pos)
{
	return closest_point_on_line(m_Pos, m_To, Pos);
}

vec2 CHealingStone::GetIntersectPos(vec2 Pos0, vec2 Pos1)
{
	vec2 Pos = vec2(0, 0);
	lines_crossed(Pos0, Pos1, m_Pos, m_To, &Pos);
	return Pos;
}

float CHealingStone::GetDistance(vec2 Pos)
{
	return distance(closest_point_on_line(m_Pos, m_To, Pos), Pos);
}

bool CHealingStone::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	if(From == -1 || Weapon != WEAP_HAMMER)
		return true;

	CPlayer *pPlayer = GameServer()->m_apPlayers[From];
	if(!pPlayer)
		return true;

	float h = (rand()%6+5)/100.0f;
	pPlayer->Heal(h);

	return true;
}

bool CHealingStone::IsEnemy(CTargetAble *pTarget)
{
	if(!pTarget)
		return false;

	int Type = pTarget->GetType();
	if(Type == CTargetAble::TYPE_CHAR)
		return true;
	return false;
}