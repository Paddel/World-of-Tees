#pragma once

#include <game/server/entity.h>

class CWeapon;

const int WeapPhysSize = 23;

class CWeaponPickup : public CEntity
{
public:
	CWeaponPickup(CGameWorld *pGameWorld, CMap *pMap, vec2 Pos, vec2 m_StartVel, int Owner, CWeapon *pWeapon);

	void Remove();

	CWeapon *GetWeapon() { return m_pWeapon; }

	virtual void Tick();
	virtual void Snap(int SnappingClient);

	static void Pickup(void *pInfo, int ClientID);
	static void MakeGlobal(void *pInfo, int ClientID);

private:
	CWeapon *m_pWeapon;
	vec2 m_Vel;
	int64 m_SpawnTime;
	int m_Owner;
	bool m_aInRadius[MAX_CLIENTS];

	bool IsGrounded();
	void DoPickupCheck();
};