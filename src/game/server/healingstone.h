#pragma once

#include <game/server/targetable.h>

class CGameContext;

class CHealingStone : public CTargetAble
{
private:
	CGameContext *m_pGameServer;
	vec2 m_Pos;
	vec2 m_To;
	int m_Height;

public:
	CHealingStone(CGameContext *pGameServer, CMap *pMap, vec2 Pos, int Height);

	virtual vec2 GetClosestPos(vec2 Pos);
	virtual vec2 GetIntersectPos(vec2 Pos0, vec2 Pos1);
	virtual float GetDistance(vec2 Pos);// { return distance(GetClosestPos(Pos), Pos); }
	virtual int GetProximityRadius() {return 32; }
	virtual bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon);
	virtual bool IsEnemy(CTargetAble *pTarget);
	virtual bool IsBuilding() { return true; }
	virtual int MaxHealth() { return 1; }

	CGameContext *GameServer() const { return m_pGameServer; }
};