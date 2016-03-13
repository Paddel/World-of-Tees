
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/weapons/boomerang.h>
#include "boomerang_proj.h"

static void Rotate(vec2 Center, vec2 *pPoint, float Rotation)
{
	int x = pPoint->x - Center.x;
	int y = pPoint->y - Center.y;
	pPoint->x = (int)(x * cosf(Rotation) - y * sinf(Rotation) + Center.x);
	pPoint->y = (int)(x * sinf(Rotation) + y * cosf(Rotation) + Center.y);
}

CBoomerangProj::CBoomerangProj(CGameWorld *pGameWorld, vec2 Pos, CTargetAble *pOwner, CMap *pMap, CBoomerang *pBoomerang, vec2 Direction, int Damage)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_BOLDER, pMap, 2)
{
	m_Pos = Pos;
	m_pOwner = pOwner;
	m_Damage = Damage;
	m_Direction = Direction;
	m_Vel = m_Direction * 32.0f;
	m_pBoomerang = pBoomerang;
	GameWorld()->InsertEntity(this);

	m_SpawnTime = Server()->Tick();
	m_State = 0;
	mem_zero(m_apHitTargets, sizeof(m_apHitTargets[0])*BOOMERANG_MAX_HIT_TARGETS);
}


bool CBoomerangProj::HitCharacter()
{
	bool Hit = false;
	int OwnerID = -1;
	if(m_pOwner)
		OwnerID = GameServer()->m_World.GetPlayerID(m_pOwner);

	for(int i = 0; i < Map()->WorldSection()->m_lpTargetAbles.size(); i++)
	{
		CTargetAble *pTarget = Map()->WorldSection()->m_lpTargetAbles[i];
		if (pTarget == m_pOwner)
			continue;

		if(m_pOwner && pTarget->IsEnemy(m_pOwner) == false)
			continue;

		vec2 TargetPos = pTarget->GetClosestPos(m_Pos);
		vec2 CheckPos = TargetPos-m_Pos;

		if(abs(CheckPos.x) > 32 || abs(CheckPos.y) > 32)//if one coordinate is over range
			continue;
		
		for(int j = 0; j < BOOMERANG_MAX_HIT_TARGETS; j++)
		{
			if(m_apHitTargets[j] == pTarget)
				break;

			if(m_apHitTargets[j] == NULL)
			{
				m_apHitTargets[j] = pTarget;
				pTarget->TakeDamage(normalize(m_Vel)*16, m_Damage, OwnerID, WEAPON_WORLD);
				//pTarget->NewStatusEffect(STATUSEFFECT_STUN, OwnerID);
				Hit = true;
				break;
			}
		}
	}

	return Hit;
}

void CBoomerangProj::Tick()
{
	if (HitCharacter())
		m_State = 1;

	if (m_State == 0)
	{
		m_Vel.y -= 1;
		if (Map()->Collision()->MoveBox(&m_Pos, &m_Vel, vec2(16, 16), 0.6f))
			m_State = 1;

		if (Server()->Tick() - m_SpawnTime > Server()->TickSpeed()*0.7f)
			m_State = 1;
	}
	else
	{
		int OwnerID = GameServer()->m_World.GetPlayerID(m_pOwner);
		if (OwnerID != -1)
		{
			vec2 OwnerPos = m_pOwner->GetClosestPos(m_Pos);
			if (distance(OwnerPos, m_Pos) <= 32.0f)
				Destroy();
			else
				m_Pos += normalize(OwnerPos - m_Pos) * 32.0f;
		}
		else
			Destroy();
	}
}

void CBoomerangProj::TickPaused()
{

}

void CBoomerangProj::Destroy()
{
	if (m_pBoomerang)
		m_pBoomerang->Return();

	GameServer()->m_World.DestroyEntity(this);
}

void CBoomerangProj::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	float Rotation = (Server()->Tick()- m_SpawnTime) / (float)Server()->TickSpeed() * 4.0f * pi;

	vec2 RotationPoint = m_Pos + vec2(32, 0);
	vec2 To = m_Pos;
	vec2 From1 = m_Pos + vec2(32, -32);
	vec2 From2 = m_Pos + vec2(32, 32);
	Rotate(RotationPoint, &From1, Rotation);
	Rotate(RotationPoint, &From2, Rotation);
	Rotate(RotationPoint, &To, Rotation);
	RenderLaser(m_aSnapIDs[0], Server()->Tick(), To, From1);
	RenderLaser(m_aSnapIDs[1], Server()->Tick(), To, From2);
}
