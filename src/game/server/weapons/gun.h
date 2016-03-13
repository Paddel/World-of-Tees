#pragma once

#include <game/server/weapon.h>
#include <game/generated/protocol.h>

class CGun : public CWeapon
{
private:

public:
	CGun(CGameContext *pGameServer, CTargetAble *pOwner, int Damage);

	virtual int FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos);
	virtual bool SpecialWorldSnap(int SnappingClient, vec2 Pos, vec2 Direction, int Attacktick) { return false; }
	
	virtual int SnapWeapon() { return WEAPON_GUN; }
	virtual int ManaCosts() { return false; }
	virtual bool FullAuto() { return false;}
	virtual bool Ranged() { return true; }
	virtual bool ShootAnim() { return true; }
	virtual bool StackAble() { return false; }
};