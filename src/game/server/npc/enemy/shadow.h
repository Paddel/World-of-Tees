#pragma once

#include <game/server/weapons/_include.h>
#include <game/server/npc.h>

class CGameContext;
class CMap;

class CShadow : public CEnemyNpc
{
private:
	int64 m_SpawnTime;
	int64 m_TeleportTime;

public:
	CShadow(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos);

	virtual void SubTick();

	virtual CWeapon *CreateWeapon() { return new CDagger(GameServer(), this, BaseDamage()); }
	virtual char *NpcName() { return "Shadow"; }
	virtual int MaxHealth() { return 100; }
	virtual int MoneyAmount() { return 12; }
	virtual int MoneyAddAmount() { return 12; }
	virtual int BaseDamage() { return 25; }
	virtual int Experience() { return 63; }
	virtual int AttackSpeed() { return 650; }
	virtual float WeaponDropChance() { return 0.25f; }
	
	//skin
	virtual char *SkinName() { return "cammostripes"; }
	virtual int SkinCostumColor() { return 1; }
	virtual int SkinColorBody();
	virtual int SkinColorFeet() { return 16777215; }

	virtual bool UseHook() { return false; }
};