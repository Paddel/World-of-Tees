#pragma once

#include <game/server/weapons/_include.h>
#include <game/server/npc.h>

class CGameContext;
class CMap;

class CMummy : public CEnemyNpc
{
private:

public:
	CMummy(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos)
		: CEnemyNpc(pGameServer, pMap, pNpcHolder, Pos, NPC_MUMMY) {}

	virtual CWeapon *CreateWeapon() { return new CHammer(GameServer(), this, BaseDamage()); }
	virtual char *NpcName() { return "Mummy"; }
	virtual int MaxHealth() { return 200; }
	virtual int MoneyAmount() { return 12; }
	virtual int MoneyAddAmount() { return 19; }
	virtual int BaseDamage() { return 10; }
	virtual int Experience() { return 43; }
	virtual int AttackSpeed() { return 2750; }
	virtual float WeaponDropChance() { return 0.0f; }
	
	//skin
	virtual char *SkinName() { return "bluestripe"; }
	virtual int SkinCostumColor() { return 1; }
	virtual int SkinColorBody() { return 255; }
	virtual int SkinColorFeet() { return 255; }

	virtual bool UseHook() { return true; }
	virtual float Speed() { return 6.5; }
};