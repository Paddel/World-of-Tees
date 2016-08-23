#pragma once

#include <game/server/weapon.h>
#include <game/generated/protocol.h>

class CBoomerang : public CWeapon
{
private:
	int m_aSnapIDs[2];
	bool m_Thrown;

public:
	CBoomerang(CGameContext *pGameServer, CTargetAble *pOwner, int Damage);

	void Return();

	virtual void Destroy();

	virtual int FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos);
	virtual bool SpecialWorldSnap(int SnappingClient, vec2 Pos, vec2 Direction, int Attacktick);
	
	virtual int SnapWeapon() { return WEAPON_HAMMER; }
	virtual int ManaCosts() { return false; }
	virtual int AmmoCosts() { return 0; }
	virtual bool FullAuto() { return false; }
	virtual bool Ranged() { return true; }
	virtual bool ShootAnim();
	virtual bool StackAble() { return false; }
};