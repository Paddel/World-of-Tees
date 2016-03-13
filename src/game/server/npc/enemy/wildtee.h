#pragma once

#include <game/server/weapons/_include.h>
#include <game/server/npc.h>

class CGameContext;
class CMap;

class CWildtee : public CEnemyNpc
{
private:

public:
	CWildtee(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos)
		: CEnemyNpc(pGameServer, pMap, pNpcHolder, Pos, NPC_WILDTEE) {}

	virtual CWeapon *CreateWeapon() { return new CHammer(GameServer(), this, BaseDamage()); }
	virtual char *NpcName() { return "Wild Tee"; }
	virtual int MaxHealth() { return 70; }
	virtual int MoneyAmount() { return 3; }
	virtual int MoneyAddAmount() { return 5; }
	virtual int BaseDamage() { return 5; }
	virtual int Experience() { return 35; }
	virtual int AttackSpeed() { return 900; }
	virtual float WeaponDropChance() { return 0.0f; }
	
	//skin
	virtual char *SkinName() { return "cammostripes"; }
	virtual int SkinCostumColor() { return 1; }
	virtual int SkinColorBody() { return 2569470; }
	virtual int SkinColorFeet() { return 4700416; }

	virtual bool UseHook() { return false; }
};