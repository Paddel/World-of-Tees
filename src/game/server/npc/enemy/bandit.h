#pragma once

#include <game/server/weapons/_include.h>
#include <game/server/npc.h>

class CGameContext;
class CMap;

class CBandit : public CEnemyNpc
{
private:

public:
	CBandit(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos)
		: CEnemyNpc(pGameServer, pMap, pNpcHolder, Pos, NPC_BANDIT) {}

	virtual CWeapon *CreateWeapon() { return new CHammer(GameServer(), this, BaseDamage()); }
	virtual char *NpcName() { return "Bandit"; }
	virtual int MaxHealth() { return 250; }
	virtual int MoneyAmount() { return 8; }
	virtual int MoneyAddAmount() { return 3; }
	virtual int BaseDamage() { return 2; }
	virtual int Experience() { return 35; }
	virtual int AttackSpeed() { return 1750; }
	virtual float WeaponDropChance() { return 0.0f; }
	
	//skin
	virtual char *SkinName() { return "brownbear"; }
	virtual int SkinCostumColor() { return 1; }
	virtual int SkinColorBody() { return 2140240; }
	virtual int SkinColorFeet() { return 851864; }

	virtual bool UseHook() { return true; }
};