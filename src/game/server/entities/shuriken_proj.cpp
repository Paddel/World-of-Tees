
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/weapons/boomerang.h>
#include "shuriken_proj.h"

static void Rotate(vec2 Center, vec2 *pPoint, float Rotation)
{
	int x = pPoint->x - Center.x;
	int y = pPoint->y - Center.y;
	pPoint->x = (int)(x * cosf(Rotation) - y * sinf(Rotation) + Center.x);
	pPoint->y = (int)(x * sinf(Rotation) + y * cosf(Rotation) + Center.y);
}

CShurikenProj::CShurikenProj(CGameWorld *pGameWorld, vec2 Pos, CTargetAble *pOwner, CMap *pMap, vec2 Direction, int Damage)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_BOLDER, pMap, 4)
{
	m_Pos = Pos;
	m_pOwner = pOwner;
	m_Damage = Damage;
	m_Direction = Direction;
	m_Vel = m_Direction * 30.0f;
	GameWorld()->InsertEntity(this);

	m_State = 0;

	m_SpawnTime = Server()->Tick();
	mem_zero(m_apHitTargets, sizeof(m_apHitTargets[0])*SHURIKEN_MAX_HIT_TARGETS);
}


bool CShurikenProj::HitCharacter()
{
	bool Hit = false;
	int OwnerID = -1;
	if (m_pOwner)
		OwnerID = GameServer()->m_World.GetPlayerID(m_pOwner);

	for (int i = 0; i < Map()->WorldSection()->m_lpTargetAbles.size(); i++)
	{
		CTargetAble *pTarget = Map()->WorldSection()->m_lpTargetAbles[i];
		if (pTarget == m_pOwner)
			continue;

		if (m_pOwner && pTarget->IsEnemy(m_pOwner) == false)
			continue;

		vec2 TargetPos = pTarget->GetClosestPos(m_Pos);
		vec2 CheckPos = TargetPos - m_Pos;

		if (abs(CheckPos.x) > 32 || abs(CheckPos.y) > 32)//if one coordinate is over range
			continue;

		for (int j = 0; j < SHURIKEN_MAX_HIT_TARGETS; j++)
		{
			if (m_apHitTargets[j] == pTarget)
				break;

			if (m_apHitTargets[j] == NULL)
			{
				m_apHitTargets[j] = pTarget;
				pTarget->TakeDamage(normalize(m_Vel) * 16, m_Damage, OwnerID, WEAPON_WORLD);
				//pTarget->NewStatusEffect(STATUSEFFECT_STUN, OwnerID);
				Hit = true;
				break;
			}
		}
	}

	return Hit;
}

void CShurikenProj::Tick()
{
	if (m_State == 0)
	{
		HitCharacter();

		m_Vel.y += 0.7f;
		if (Map()->Collision()->MoveBox(&m_Pos, &m_Vel, vec2(16, 16), 0.6f))
			m_State = 1;

		if (Server()->Tick() - m_SpawnTime > Server()->TickSpeed() * 10)
			m_State = 1;

		m_Rotation = (Server()->Tick() - m_SpawnTime) / (float)Server()->TickSpeed() * 4.0f * pi;
	}
	else
	{
		m_Pos.y++;
		if (Map()->Collision()->CheckPoint(m_Pos - vec2(0, 28)) || 
			Server()->Tick() - m_SpawnTime > Server()->TickSpeed() * 20)
			Destroy();
	}
}

void CShurikenProj::TickPaused()
{

}

void CShurikenProj::Destroy()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CShurikenProj::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	vec2 From1 = m_Pos - vec2(16, 0);
	vec2 To1 = m_Pos + vec2(16, 0);
	vec2 From2 = m_Pos - vec2(0, 16);
	vec2 To2 = m_Pos + vec2(0, 16);

	Rotate(m_Pos, &From1, m_Rotation);
	Rotate(m_Pos, &From2, m_Rotation);
	Rotate(m_Pos, &To1, m_Rotation);
	Rotate(m_Pos, &To2, m_Rotation);

	RenderLaser(m_aSnapIDs[0], Server()->Tick(), From1, To1);
	RenderLaser(m_aSnapIDs[1], Server()->Tick(), From1, From1);
	RenderLaser(m_aSnapIDs[2], Server()->Tick(), From2, To2);
	RenderLaser(m_aSnapIDs[3], Server()->Tick(), From2, From2);
}
