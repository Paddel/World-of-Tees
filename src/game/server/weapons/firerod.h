#pragma once

#include <game/server/weapon.h>
#include <game/generated/protocol.h>

class CFireRod : public CWeapon
{
private:
	int m_aSnapIDs[4];
	int64 m_Cooldown;
	int64 m_ManaCooldown;

public:
	CFireRod(CGameContext *pGameServer, CTargetAble *pOwner, int Damage);

	virtual void Destroy();

	virtual int FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos);
	virtual bool SpecialWorldSnap(int SnappingClient, vec2 Pos, vec2 Direction, int Attacktick);
	
	virtual int SnapWeapon() { return WEAPON_HAMMER; }
	virtual int ManaCosts();
	virtual bool FullAuto() { return true;} 
	virtual bool Ranged() { return true; }
	virtual bool ShootAnim() { return false; }
	virtual bool StackAble() { return false; }
};