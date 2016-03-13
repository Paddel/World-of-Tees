#pragma once

#include <game/server/weapons/_include.h>
#include <game/server/npc.h>

class CGameContext;
class CMap;

class CNomad : public CEnemyNpc
{
private:

public:
	CNomad(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos)
		: CEnemyNpc(pGameServer, pMap, pNpcHolder, Pos, NPC_NOMAD) {}

	virtual CWeapon *CreateWeapon() { return new CHammer(GameServer(), this, BaseDamage()); }
	virtual char *NpcName() { return "Nomad"; }
	virtual int MaxHealth() { return 170; }
	virtual int MoneyAmount() { return 11; }
	virtual int MoneyAddAmount() { return 4; }
	virtual int BaseDamage() { return 10; }
	virtual int Experience() { return 42; }
	virtual int AttackSpeed() { return 1400; }
	virtual float WeaponDropChance() { return 0.0f; }
	
	//skin
	virtual char *SkinName() { return "pinky"; }
	virtual int SkinCostumColor() { return 1; }
	virtual int SkinColorBody() { return 2926118; }
	virtual int SkinColorFeet() { return 3947833; }

	virtual bool UseHook() { return false; }
};