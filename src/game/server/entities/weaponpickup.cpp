
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/weapon.h>
#include <game/server/elements/questionhandler.h>

#include "weaponpickup.h"

#define WEAPON_DESPAWN_TIME 32

CWeaponPickup::CWeaponPickup(CGameWorld *pGameWorld, CMap *pMap, vec2 Pos, vec2 m_StartVel, int Owner, CWeapon *pWeapon)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP, pMap, 1)
{
	m_Pos = Pos;
	m_Vel = m_StartVel;
	m_Owner = Owner;
	m_pWeapon = pWeapon;
	m_SpawnTime = Server()->Tick();

	m_ProximityRadius = WeapPhysSize;
	mem_zero(&m_aInRadius, sizeof(m_aInRadius));

	GameWorld()->InsertEntity(this);
}

bool CWeaponPickup::IsGrounded()
{
	if(Map()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	if(Map()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	return false;
}

void CWeaponPickup::Pickup(void *pInfo, int ClientID)
{
	CWeaponPickup *pThis = (CWeaponPickup *)pInfo;
	CCharacter *pChr = pThis->GameServer()->GetPlayerChar(ClientID);
	if(pChr == NULL)
		return;

	if (pChr->PickupWeapon(pThis) == false)
		return;

	pThis->Remove();
	int Mask = ClientID!=-1?CmaskOne(ClientID):-1;
	pThis->GameServer()->CreateSound(pThis->m_Pos, SOUND_PICKUP_SHOTGUN, pThis->Map(), Mask);
}

void CWeaponPickup::MakeGlobal(void *pInfo, int ClientID)
{
	CWeaponPickup *pThis = (CWeaponPickup *)pInfo;
	if(pThis->m_Owner != -1)
	{
		pThis->m_SpawnTime = pThis->Server()->Tick();
		pThis->m_Owner = -1;
	}
}

void CWeaponPickup::Remove()
{
	GameServer()->m_World.RemoveEntity(this);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CCharacter *pChr = GameServer()->GetPlayerChar(i);
		if(pChr == NULL || m_aInRadius[i] == false)
			continue;

		pChr->GetPlayer()->QuestionHandler()->EndQuestion(CQuestionHandler::TYPE_WEAPONPICKUP);
	}
}

void CWeaponPickup::DoPickupCheck()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CCharacter *pChr = GameServer()->GetPlayerChar(i);
		CPlayer *pPlayer = NULL;
		if(pChr == NULL || pChr->Map() != Map())
			continue;

		if(m_Owner != -1 && m_Owner != i)
			continue;

		pPlayer = pChr->GetPlayer();
		float Dist = distance(m_Pos, pChr->m_Pos);
		if(m_aInRadius[i] == false)
		{
			if(Dist <= m_ProximityRadius && pChr->HasHealth())
			{
				if (m_pWeapon->StackAble() == true)
				{
					CWeapon *pWeapon = pChr->HasWeapon(m_pWeapon->GetType());
					if (pWeapon)
					{
						pWeapon->AddAmmo(m_pWeapon->Ammo());
						Remove();
						m_pWeapon->Destroy();
						GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN, Map());
						return;
					}
				}

				pPlayer->QuestionHandler()->Ask(this, CQuestionHandler::TYPE_WEAPONPICKUP, Pickup, MakeGlobal, "Pickup weapon", "");
				m_aInRadius[i] = true;
			}
		}
		else
		{
			if(Dist > m_ProximityRadius*1.5f || pChr->HasHealth() == false)
			{
				pPlayer->QuestionHandler()->EndQuestion(CQuestionHandler::TYPE_WEAPONPICKUP);
				m_aInRadius[i] = false;
			}
		}
	}
}

void CWeaponPickup::Tick()
{
	//Move
	if(IsGrounded())
		m_Vel.x *= 0.3f;
	m_Vel.y += 0.5f;
	Map()->Collision()->MoveBox(&m_Pos, &m_Vel, vec2(m_ProximityRadius, m_ProximityRadius), 0.6f);

	if (m_SpawnTime + Server()->TickSpeed() < Server()->Tick())
		DoPickupCheck();

	if(m_SpawnTime+Server()->TickSpeed()*WEAPON_DESPAWN_TIME < Server()->Tick())
	{
		if(m_Owner != -1)
			m_Owner = -1;
		else
		{
			Remove();
			m_pWeapon->Destroy();
		}

		GameServer()->CreateSound(m_Pos, SOUND_CTF_RETURN, Map());
		m_SpawnTime = Server()->Tick();
	}

	
}

void CWeaponPickup::Snap(int SnappingClient)
{
	if(m_Owner != -1 && m_Owner != SnappingClient)
		return;

	if(m_SpawnTime+Server()->TickSpeed()*(WEAPON_DESPAWN_TIME-3) < Server()->Tick())
	{
		if(Server()->Tick()%10 == 0)
			return;
	}

	if(NetworkClipped(SnappingClient))
		return;

	if(m_pWeapon->SpecialWorldSnap(SnappingClient, m_Pos, vec2(1, 0), 0))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_aSnapIDs[0], sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = POWERUP_WEAPON;
	pP->m_Subtype = m_pWeapon->SnapWeapon();
}