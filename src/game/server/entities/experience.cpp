/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "experience.h"

CExp::CExp(CGameWorld *pGameWorld, CMap *pMap, vec2 Pos, int Owner, int Amount)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_EXP, pMap, 1)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_Amount = Amount;

	GameWorld()->InsertEntity(this);
}

void CExp::Tick()
{
	CCharacter *pChr = GameServer()->GetPlayerChar(m_Owner);
	if(!pChr)
	{
		GameServer()->m_World.DestroyEntity(this);
		return;
	}

	m_Pos += normalize(pChr->m_Pos-m_Pos)*12;

	if(distance(m_Pos, pChr->m_Pos) < pChr->m_ProximityRadius)
	{
		CPlayer *pPlayer = pChr->GetPlayer();
		pPlayer->GainExp(m_Amount);
		GameServer()->m_World.DestroyEntity(this);
		GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH, Map());
	}
}

void CExp::Snap(int SnappingClient)
{
	if(m_Owner != -1 && m_Owner != SnappingClient)
		return;

	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_aSnapIDs[0], sizeof(CNetObj_Projectile)));
	if(!pProj)
		return;


	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)0;
	pProj->m_VelY = (int)0;
	pProj->m_StartTick = Server()->Tick()-1;
	pProj->m_Type = WEAPON_HAMMER;
}