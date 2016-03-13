/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "laser.h"

#define LASER_SHOWN_DELAY 300
#define LASER_MOVE_SPEED 90

CLaser::CLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, CTargetAble *pOwner, CMap *pMap, int Damage)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER, pMap, 1)
{
	m_Pos = Pos;
	m_From = Pos;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_pOwner = pOwner;
	m_Bounces = 0;
	m_EvalTick = 0;
	m_Damage = Damage;
	GameWorld()->InsertEntity(this);
}


bool CLaser::HitCharacter(vec2 From, vec2 To)
{
	vec2 At;
	CTargetAble *pHit = GameServer()->m_World.IntersectTarget(From, To, 0.f, At, m_pOwner, Map());
	if(!pHit || pHit->IsEnemy(m_pOwner) == false)
		return false;

	int PlayerID = GameServer()->m_World.GetPlayerID(m_pOwner);
	m_Pos = At;
	m_Energy = -1;
	pHit->TakeDamage(vec2(0.f, 0.f), m_Damage, PlayerID, WEAPON_RIFLE);
	return true;
}


void CLaser::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CLaser::Tick()
{
	if(m_Energy > 0)
	{
		m_EvalTick = Server()->Tick();

		vec2 OldPos = m_Pos;

		m_Pos += m_Dir*min(m_Energy, (float)LASER_MOVE_SPEED);

		m_Energy -= distance(m_Pos, OldPos);

		if(HitCharacter(OldPos, m_Pos) ||
			Map()->Collision()->IntersectLine(OldPos, m_Pos, NULL, &m_Pos))
			m_Energy = -1;
	}

	if(m_Energy <= 0 && Server()->Tick()-m_EvalTick > Server()->TickSpeed()*LASER_SHOWN_DELAY/1000.0f)
		Reset();
}

void CLaser::TickPaused()
{
	++m_EvalTick;
}

void CLaser::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	int64 Tick = Server()->Tick()-(Server()->Tick()-m_EvalTick)*(GameServer()->Tuning()->m_LaserBounceDelay/(float)LASER_SHOWN_DELAY);
	RenderLaser(m_aSnapIDs[0], Tick, m_From, m_Pos);
	//RenderLaser(m_aSnapIDs[1], Tick, m_From, m_From);
}
