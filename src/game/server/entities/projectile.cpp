/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/targetable.h>
#include "projectile.h"

CProjectile::CProjectile(CGameWorld *pGameWorld, CTargetAble *pOwner, int Type, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon, CMap *pMap)
: CRenderPoint(pGameWorld, Type, Pos, Dir, false, pMap)
{
	m_Type = Type;
	m_Pos = Pos;
	m_SpawnPos = Pos;
	m_Direction = Dir;
	m_LifeSpan = Span;
	m_pOwner = pOwner;
	m_Force = Force;
	m_Damage = Damage;
	m_SoundImpact = SoundImpact;
	m_Weapon = Weapon;
	m_Explosive = Explosive;
}

vec2 CProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch(m_Type)
	{
		case WEAPON_GRENADE:
			Curvature = 7.0f;
			Speed = 1000.0f;
			break;

		case WEAPON_SHOTGUN:
			Curvature = 1.25f;
			Speed = 2050.0f;
			break;

		case WEAPON_GUN:
			Curvature = 1.24f;
			Speed = 1800.0f;
			break;
	}

	return CalcPos(m_SpawnPos, m_Direction, Curvature, Speed, Time);
}


void CProjectile::Tick()
{
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	m_Pos = GetPos(Ct);
	int Collide = Map()->Collision()->IntersectLine(PrevPos, m_Pos, &m_Pos, 0);
	//CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	//CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);
	CTargetAble *pTarget = GameServer()->m_World.IntersectTarget(PrevPos, m_Pos, 6.0f, m_Pos, m_pOwner, Map());
	int PlayerID = GameServer()->m_World.GetPlayerID(m_pOwner);
	m_LifeSpan--;

	if((pTarget && pTarget->IsEnemy(m_pOwner)) || Collide || m_LifeSpan < 0 || GameLayerClipped(m_Pos))
	{
		if(m_LifeSpan >= 0 || m_Weapon == WEAPON_GRENADE)
			GameServer()->CreateSound(m_Pos, m_SoundImpact, Map());

		if(m_Explosive)
		{
			GameServer()->CreateExplosion(m_Pos, PlayerID, m_Weapon, m_Damage, Map(), m_pOwner);
		}

		else if(pTarget && pTarget->IsEnemy(m_pOwner))
			pTarget->TakeDamage(m_Direction * max(0.001f, m_Force), m_Damage, PlayerID, m_Weapon);

		GameServer()->m_World.DestroyEntity(this);
	}
}

void CProjectile::TickPaused()
{
	++m_StartTick;
}