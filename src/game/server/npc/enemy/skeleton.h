#pragma once

#include <game/server/weapons/_include.h>
#include <game/server/npc.h>

class CGameContext;
class CMap;

class CSkeleton : public CEnemyNpc
{
private:

public:
	CSkeleton(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos);

	virtual void SubTick();

	virtual CWeapon *CreateWeapon() { return new CGun(GameServer(), this, BaseDamage()); }
	virtual char *NpcName() { return "Skeleton"; }
	virtual int MaxHealth() { return 150; }
	virtual int MoneyAmount() { return 6; }
	virtual int MoneyAddAmount() { return 5; }
	virtual int BaseDamage() { return 9; }
	virtual int Experience() { return 69; }
	virtual int AttackSpeed() { return 350; }
	virtual float WeaponDropChance() { return 0.15f; }
	
	//skin
	virtual char *SkinName() { return "toptri"; }
	virtual int SkinCostumColor() { return 1; }
	virtual int SkinColorBody() { return 255; }
	virtual int SkinColorFeet() { return 255; }

	virtual bool UseHook() { return false; }
};