#pragma once

#include <game/server/npc.h>

class CGameContext;
class CMap;

class CBeggar : public CNpc
{
private:

protected:
	vec2 m_SpawnPos;
	bool m_RandJump;
	int m_HelpingText;
	int64 m_MoveEyeTime;

public:
	CBeggar(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos);

	virtual void Tick();
	virtual void Snap(int SnappingClient);

	virtual void SetInput();
	virtual void NoTargetInput();

	virtual void SubSpawn();
	virtual void OnDeath(int From, int Weapon, vec2 DeathVel);
	virtual bool Alive() { return true; }

	char *NpcName() { return "Beggar"; }
	int MaxHealth() { return 1; }
	int MoneyAmount() { return 2000; }
	int MoneyAddAmount() { return 500; }
	int BaseDamage() { return 0; }
	int Experience() { return 500; }
	int Weapon() { return WEAPON_HAMMER; }
	
	//skin
	char *SkinName() { return "limekitty"; }
	int SkinCostumColor() { return 1; }
	int SkinColorBody() { return 0; }
	int SkinColorFeet() { return 0; }

	//overwriteable functions
	virtual void SetExtraCollision();
};