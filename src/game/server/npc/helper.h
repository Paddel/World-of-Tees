#pragma once

#include <game/server/npc.h>

class CGameContext;
class CMap;

class CHelper : public CNpc, public CTargetAble
{
private:

protected:
	vec2 m_SpawnPos;
	bool m_RandJump;
	int m_HelpingText;
	int64 m_EmoticonTime;

	struct
	{
		int64 m_ChatTime;
		bool m_Close;
	} m_aChatHelp[MAX_CLIENTS];

public:
	CHelper(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos, int HelpingText);

	virtual void Tick();
	virtual void Snap(int SnappingClient);

	virtual void SetInput();
	virtual void NoTargetInput();

	virtual void SubSpawn();
	virtual void OnDeath(int From, int Weapon, vec2 DeathVel);
	virtual bool Alive() { return true; }

	//targetable
	virtual void ResetEnemy(CTargetAble *pTarget) {}
	virtual vec2 GetClosestPos(vec2 Pos) { return GetPos(); }
	virtual vec2 GetIntersectPos(vec2 Pos0, vec2 Pos1) { return closest_point_on_line(Pos0, Pos1, GetPos()); }
	virtual bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon);
	virtual int GetProximityRadius() { return 28; }
	virtual bool IsEnemy(CTargetAble *pTarget);
	virtual bool IsBuilding() { return false; }
	virtual int MaxHealth() { return 1; }
	//virtual void Slow(float t, float SlowRate) { m_Core.Slow(t, SlowRate); }

	char *NpcName() { return "HammerMeForHelp"; }
	int MoneyAmount() { return 2000; }
	int MoneyAddAmount() { return 500; }
	int BaseDamage() { return 0; }
	int Experience() { return 500; }
	int Weapon() { return WEAPON_HAMMER; }
	
	//skin
	char *SkinName() { return "limekitty"; }
	int SkinCostumColor() { return 1; }
	int SkinColorBody() { return 13517314; }
	int SkinColorFeet() { return 16759230; }

	//overwriteable functions
	virtual void SetExtraCollision();
};