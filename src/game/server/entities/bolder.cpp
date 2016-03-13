
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "bolder.h"

#define COOLDOWN 30
#define RADIUS 64
#define SEGMENTS 9

CBolderSpawner::CBolderSpawner(CGameWorld *pGameWorld, CMap *pMap, vec2 Pos, int Damage, int ID)
	: CRemote(CRemote::REMOTETYPE_BOLDER, ID)
{
	m_pGameWorld = pGameWorld;
	m_pMap = pMap;
	m_Pos = Pos;
	m_Damage = Damage;

	m_ActivationTime = 0;

	m_pMap->WorldSection()->AddRemote(this);
}

void CBolderSpawner::Activate()
{
	if(m_ActivationTime < m_pGameWorld->Server()->Tick())
	{
		new CBolder(m_pGameWorld, m_Pos, NULL, m_pMap, m_Damage);
		m_ActivationTime = m_pGameWorld->Server()->Tick() + m_pGameWorld->Server()->TickSpeed() * COOLDOWN;
	}
}

CBolder::CBolder(CGameWorld *pGameWorld, vec2 Pos, CTargetAble *pOwner, CMap *pMap, int Damage)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_BOLDER, pMap, SEGMENTS)
{
	m_Pos = Pos;
	m_pOwner = pOwner;
	m_Damage = Damage;
	GameWorld()->InsertEntity(this);

	m_State = 0;
	mem_zero(m_apHitTargets, sizeof(m_apHitTargets[0])*BOLDER_MAX_HIT_TARGETS);

	m_DisappearTime = Server()->Tick() + Server()->TickSpeed() * 30.0f;
}


void CBolder::HitCharacter()
{
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

		if(abs(CheckPos.x) > RADIUS || abs(CheckPos.y) > RADIUS)//if one coordinate is over range
			continue;
		
		for(int j = 0; j < BOLDER_MAX_HIT_TARGETS; j++)
		{
			if(m_apHitTargets[j] == pTarget)
				break;

			if(m_apHitTargets[j] == NULL)
			{
				m_apHitTargets[j] = pTarget;
				pTarget->TakeDamage(vec2(CheckPos.x, -16), m_Damage, OwnerID, WEAPON_WORLD);
				pTarget->NewStatusEffect(STATUSEFFECT_STUN, OwnerID);
				break;
			}
		}
	}
}

void CBolder::Tick()
{
	vec2 OldPos = m_Pos;

	if(m_State < 2)
	{
		m_Pos.y += 15.0f;

		HitCharacter();
	}
	else
		m_Pos.y += 1.0f;

	vec2 CheckPos = m_Pos+vec2(0, RADIUS);
	if(m_State == 0)
	{
		if(Map()->Collision()->CheckPoint(CheckPos) == false)
			m_State = 1;
	}
	else if(m_State == 1)
	{
		if(Map()->Collision()->CheckPoint(CheckPos) == true)
		{
			m_State = 2;
			m_DisappearTime = Server()->Tick() + Server()->TickSpeed() * 3.0f;
		}
	}

	if(m_DisappearTime < Server()->Tick())
		GameServer()->m_World.DestroyEntity(this);
}

void CBolder::TickPaused()
{

}

void CBolder::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	for(int i = 0; i < SEGMENTS; i++)
	{
		float PartFrom = (i/(float)SEGMENTS)*2*pi;
		float PartTo = ((i+1)/(float)SEGMENTS)*2*pi;
		vec2 From = m_Pos+vec2(sinf(PartFrom), cosf(PartFrom))*RADIUS;
		vec2 To = m_Pos+vec2(sinf(PartTo), cosf(PartTo))*RADIUS;

		RenderLaser(m_aSnapIDs[i], Server()->Tick(), From, To);
	}
}
