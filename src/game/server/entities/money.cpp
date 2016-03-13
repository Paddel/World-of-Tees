/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "money.h"

#define MONEY_DESPAWN_TIME 32

CMoney::CMoney(CGameWorld *pGameWorld, CMap *pMap, vec2 Pos, vec2 m_StartVel, int Amount, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_MONEY, pMap, 1)
{
	m_Pos = Pos;
	m_Vel = m_StartVel;
	m_Amount = Amount;
	m_Owner = Owner;
	m_SpawnTime = Server()->Tick();

	m_ProximityRadius = MoneyPhysSize;

	GameWorld()->InsertEntity(this);
}

bool CMoney::IsGrounded()
{
	if(Map()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	if(Map()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	return false;
}

void CMoney::Tick()
{
	//Move
	if(IsGrounded())
		m_Vel.x *= 0.3f;
	m_Vel.y += 0.5f;
	Map()->Collision()->MoveBox(&m_Pos, &m_Vel, vec2(m_ProximityRadius, m_ProximityRadius), 0.6f);

	if(Server()->Tick()-m_SpawnTime > Server()->TickSpeed()*0.5f)
	{
		CCharacter *pChr = NULL;
		if(m_Owner != -1)
			pChr = GameServer()->GetPlayerChar(m_Owner);
		else
			pChr = GameServer()->m_World.ClosestCharacter(m_Pos, m_ProximityRadius+28.0f, Map(), 0);

		if(pChr && pChr->IsAlive())
		{
			int dist = m_Owner!=-1?distance(pChr->m_Pos, m_Pos):1;
			if(dist < m_ProximityRadius+pChr->m_ProximityRadius)
			{
				pChr->GetPlayer()->EarnMoney(m_Amount);
				GameServer()->m_World.RemoveEntity(this);
				int Mask = m_Owner!=-1?CmaskOne(m_Owner):m_Owner;
				GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR, Map(), Mask);
			}
		}
	}

	if(m_SpawnTime+Server()->TickSpeed()*MONEY_DESPAWN_TIME < Server()->Tick())
	{
		if(m_Owner != -1)
			m_Owner = -1;
		else
			GameServer()->m_World.RemoveEntity(this);

		GameServer()->CreateSound(m_Pos, SOUND_CTF_RETURN, Map());
		m_SpawnTime = Server()->Tick();
	}

	
}

void CMoney::Snap(int SnappingClient)
{
	if(m_Owner != -1 && m_Owner != SnappingClient)
		return;

	if(m_SpawnTime+Server()->TickSpeed()*(MONEY_DESPAWN_TIME-3) < Server()->Tick())
	{
		if(Server()->Tick()%10 == 0)
			return;
	}

	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_aSnapIDs[0], sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = POWERUP_ARMOR;
	pP->m_Subtype = 0;
}
