#pragma once

#include <game/server/weapons/_include.h>
#include <game/server/npc.h>

class CGameContext;
class CMap;

class CPharao : public CEnemyNpc
{
private:
	int64 m_SpawnTime;
	int64 m_TeleportTime;

public:
	CPharao(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos);

	virtual CWeapon *CreateWeapon() { return new CFireRod(GameServer(), this, BaseDamage()); }
	virtual char *NpcName() { return "Pharao"; }
	virtual int MaxHealth() { return 500; }
	virtual int MoneyAmount() { return 70; }
	virtual int MoneyAddAmount() { return 30; }
	virtual int BaseDamage() { return 1; }
	virtual int Experience() { return 100; }
	virtual int AttackSpeed() { return 0; }
	virtual float WeaponDropChance() { return 0.0f; }
	
	//skin
	virtual char *SkinName() { return "toptri"; }
	virtual int SkinCostumColor() { return 1; }
	virtual int SkinColorBody() { return 2716160; }
	virtual int SkinColorFeet() { return 255; }

	virtual bool UseHook() { return true; }
};