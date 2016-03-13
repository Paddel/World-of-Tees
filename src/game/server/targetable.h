#pragma once

#include <base/math.h>
#include <base/vmath.h>

class CGameWorld;
class IServer;
class CMap;

enum
{
	STATUSEFFECT_FIRE=0,
	STATUSEFFECT_ICE,
	STATUSEFFECT_POISON,
	STATUSEFFECT_STUN,
	NUM_STATUSEFFECT,
};

struct CStatusEffect
{
	int64 m_EndTime;
	int m_From;
};

static int s_aStatusEffectColors[NUM_STATUSEFFECT] = {50432, 9224224, 5943296, 0};

class CTargetAble
{
public:
	enum
	{
		TYPE_CHAR = 0,
		TYPE_NPC_ENEMY,
		TYPE_NPC_VILLIGER,
		TYPE_HEALINGSTONE,
	};

private:
	CGameWorld *m_pWorld;
	IServer *m_pServer;
	CMap *m_pTargetMap;
	CStatusEffect *m_pStatusEffect;
	int m_Type;
	int m_ShownStatusEffect;
	int64 m_ShownStatusEffectTime;
	int64 m_FireDamageTime;

public:
	CTargetAble(CGameWorld *pWorld, int Type, CMap *pMap);

	int GetType() const { return m_Type; }
	void AddTargetAble();
	void ResetEnemies();
	void OnTargetDeath();
	void SetStatusEffect(CStatusEffect *pStatusEffect);
	void HandleStatusEffects();
	bool StatusEffectSkin(int *pColorBody, int *pColorFeet);
	void NewStatusEffect(int StatusEffect, int From);

	int GetType() { return m_Type; }
	CMap *TargetMap() { return m_pTargetMap; }
	float GetSlow();
	bool GetStun();

	virtual void ResetEnemy(CTargetAble *pTarget) {}
    
	//virtual vec2 IntersectTargetPos(vec2 Pos0, vec2 Pos1) = 0;
	//virtual CMap *GetMap() = 0;
	virtual vec2 GetClosestPos(vec2 Pos) = 0;
	virtual vec2 GetIntersectPos(vec2 Pos0, vec2 Pos1) = 0;
	virtual float GetDistance(vec2 Pos) { return distance(GetClosestPos(Pos), Pos); }
	virtual int GetProximityRadius() = 0;
	virtual bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon) = 0;
	virtual bool IsEnemy(CTargetAble *pTarget) = 0;
	virtual bool IsBuilding() = 0;
	virtual int MaxHealth() = 0;
};