
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/targetable.h>
#include "firetile.h"

CFireTile::CFireTile(CGameWorld *pGameWorld, CTargetAble *pOwner, vec2 Pos, vec2 Dir, int Damage, int Weapon, CMap *pMap)
: CRenderPoint(pGameWorld, WEAPON_GRENADE, Pos, Dir, false, pMap)
{
	m_Pos = Pos;
	m_SpawnPos = Pos;
	m_Direction = Dir;
	m_pOwner = pOwner;
	m_Damage = Damage;
	m_Weapon = Weapon;
	m_Speed = 25.0f+rand()%10;
	m_LifeSpan = Server()->TickSpeed()*0.8f;
}


void CFireTile::Tick()
{
	vec2 PrevPos = m_Pos;
	if(m_Speed)
		m_Pos += m_Direction*m_Speed;
	int Collide = Map()->Collision()->IntersectLine(PrevPos, m_Pos, &m_Pos, 0);
	//CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	//CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);
	CTargetAble *pTarget = GameServer()->m_World.IntersectTarget(PrevPos, m_Pos, 6.0f, m_Pos, m_pOwner, Map());
	int PlayerID = GameServer()->m_World.GetPlayerID(m_pOwner);
	m_Speed *= 0.9f;
	m_LifeSpan--;

	if(Collide)
	{
		m_Speed = 0.0f;
		m_Pos = PrevPos;
	}


	if((pTarget && pTarget->IsEnemy(m_pOwner)) || m_LifeSpan < 0 || GameLayerClipped(m_Pos))
	{

		if(pTarget && pTarget->IsEnemy(m_pOwner))
		{
			pTarget->TakeDamage(vec2(0, 0), m_Damage, PlayerID, m_Weapon);
			pTarget->NewStatusEffect(STATUSEFFECT_FIRE, PlayerID);
		}

		GameServer()->m_World.DestroyEntity(this);
	}
}

void CFireTile::TickPaused()
{
	++m_StartTick;
}