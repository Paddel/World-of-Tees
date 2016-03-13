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
	virtual int MaxHealth() { return 130; }
	virtual int MoneyAmount() { return 6; }
	virtual int MoneyAddAmount() { return 6; }
	virtual int BaseDamage() { return 8; }
	virtual int Experience() { return 53; }
	virtual int AttackSpeed() { return 850; }
	virtual float WeaponDropChance() { return 0.0f; }
	
	//skin
	virtual char *SkinName() { return "cammo"; }
	virtual int SkinCostumColor() { return 0; }
	virtual int SkinColorBody() { return 0; }
	virtual int SkinColorFeet() { return 0; }

	virtual bool UseHook() { return false; }
};