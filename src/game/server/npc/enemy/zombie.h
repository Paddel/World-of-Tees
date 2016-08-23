#pragma once

#include <game/server/weapons/_include.h>
#include <game/server/npc.h>

class CGameContext;
class CMap;

class CZombie : public CEnemyNpc
{
private:

public:
	CZombie(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos);

	virtual void SubTick();

	virtual CWeapon *CreateWeapon() { return new CHammer(GameServer(), this, BaseDamage()); }
	virtual char *NpcName() { return "Zombie"; }
	virtual int MaxHealth() { return 370; }
	virtual int MoneyAmount() { return 7; }
	virtual int MoneyAddAmount() { return 5; }
	virtual int BaseDamage() { return 14; }
	virtual int Experience() { return 73; }
	virtual int AttackSpeed() { return 700; }
	virtual float WeaponDropChance() { return 0.0f; }
	
	//skin
	virtual char *SkinName() { return "cammo"; }
	virtual int SkinCostumColor() { return 0; }
	virtual int SkinColorBody() { return 0; }
	virtual int SkinColorFeet() { return 0; }

	virtual bool UseHook() { return true; }
};