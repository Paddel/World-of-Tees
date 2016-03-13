#pragma once

#include <game/server/weapon.h>
#include <game/generated/protocol.h>

class CHammer : public CWeapon
{
private:

public:
	CHammer(CGameContext *pGameServer, CTargetAble *pOwner, int Damage);

	virtual int FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos);
	virtual bool SpecialWorldSnap(int SnappingClient, vec2 Pos, vec2 Direction, int Attacktick) { return false; }
	
	virtual int SnapWeapon() { return WEAPON_HAMMER; }
	virtual int ManaCosts() { return false; }
	virtual bool FullAuto() { return false;} 
	virtual bool Ranged() { return false; }
	virtual bool ShootAnim() { return true; }
	virtual bool StackAble() { return false; }
};