#pragma once

#include <game/server/entity.h>

class CWeapon;

class CWeaponMount : public CEntity
{
public:
	CWeaponMount(CGameWorld *pGameWorld, CMap *pMap, vec2 Pos, CWeapon *pWeapon);

	CWeapon *GetWeapon() { return m_pWeapon; }

	virtual void Tick();
	virtual void Snap(int SnappingClient);

	static void PickupGun(void *pInfo, int ClientID);
	static void MakeGlobal(void *pInfo, int ClientID);

private:
	CWeapon *m_pWeapon;
	int64 m_aSpawnTime[MAX_CLIENTS];

	void DoPickupCheck();
};