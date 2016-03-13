#pragma once

#include <game/server/npc.h>

class CGameContext;
class CMap;

class CGuard : public CNpc, public CTargetAble
{
private:

protected:
	CDamageIndicatorHandler m_DamageIndHandler;
	CTargetAble *m_pTarget;
	int64 m_HealthRegTime;
	vec2 m_SpawnPos;
	int m_Health;
	int m_AttackTick;

	void CheckHealthRegeneration();

	bool DealDamage(CTargetAble *pTarget, vec2 Force, int Damage, int Weapon);

	enum
	{
		TIME_NOTAR_CHANGE_MOVEDIR=0,
		TIME_NOTAR_JUMP,
		TIME_NOTAR_NEW_EYE_DIR,
		TIME_TAR_COL_CHECK,
		TIME_TAR_COL_LOOSE,
		TIME_TAR_ATTACK,

		NUM_TIMES,
	};

	int64 m_aTimes[NUM_TIMES];
	bool m_RandJump;


public:
	CGuard(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos);

	virtual void Tick();
	virtual void Snap(int SnappingClient);

	virtual void SetInput();
	virtual void NoTargetInput();
	virtual void TargetInput();

	virtual void SubSpawn();
	virtual void OnDeath(int From, int Weapon, vec2 DeathVel);
	virtual bool Alive() { return m_Health > 0; }

	virtual void OnNotActive();

	//targetable
	virtual void ResetEnemy(CTargetAble *pTarget);
	virtual vec2 GetClosestPos(vec2 Pos) { return GetPos(); }
	virtual vec2 GetIntersectPos(vec2 Pos0, vec2 Pos1) { return closest_point_on_line(Pos0, Pos1, GetPos()); }
	virtual bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon);
	virtual int GetProximityRadius() { return 28; }
	virtual bool IsEnemy(CTargetAble *pTarget);
	virtual bool IsBuilding() { return false; }
	virtual int MaxHealth() { return 3000; }
	//virtual void Slow(float t, float SlowRate) { m_Core.Slow(t, SlowRate); }

	char *NpcName() { return "Guard"; }
	int MoneyAmount() { return 200; }
	int MoneyAddAmount() { return 50; }
	int BaseDamage() { return 50; }
	int Experience() { return 300; }
	int Weapon() { return WEAPON_HAMMER; }
	int AttackSpeed() { return 40; }
	
	//skin
	char *SkinName() { return "limekitty"; }
	int SkinCostumColor() { return 1; }
	int SkinColorBody() { return 5963833; }
	int SkinColorFeet() { return 186; }

	//overwriteable functions
	virtual void SetExtraCollision();
};