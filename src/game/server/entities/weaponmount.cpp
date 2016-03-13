
#include <game/server/gamecontext.h>
#include <game/server/weapon.h>
#include <game/server/entities/weaponpickup.h>

#include "weaponmount.h"

CWeaponMount::CWeaponMount(CGameWorld *pGameWorld, CMap *pMap, vec2 Pos, CWeapon *pWeapon)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_WEAPONMOUNT, pMap, 0)
{
	m_Pos = Pos;
	m_pWeapon = pWeapon;

	mem_zero(&m_aSpawnTime, sizeof(m_aSpawnTime));

	GameWorld()->InsertEntity(this);
}

void CWeaponMount::DoPickupCheck()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CCharacter *pChr = GameServer()->GetPlayerChar(i);
		float Dist = 0;
		if(pChr == NULL || pChr->Map() != Map())
			continue;

		if(m_aSpawnTime[i] < Server()->Tick())
		{
			Dist = distance(pChr->m_Pos, m_Pos);
			if(Dist < 64)
			{
				vec2 Vel = normalize(vec2((rand()%200-100)*0.01f, -1))*16.0f;
				CWeapon *pWeapon = CreateWeapon(m_pWeapon->GetType(), GameServer(), NULL, m_pWeapon->Damage());
				new CWeaponPickup(GameWorld(), Map(), m_Pos, Vel, i, pWeapon);
				m_aSpawnTime[i] = Server()->Tick() + Server()->TickSpeed() * 300.0f;
			}
		}
	}
}

void CWeaponMount::Tick()
{
	DoPickupCheck();
}

void CWeaponMount::Snap(int SnappingClient)
{
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