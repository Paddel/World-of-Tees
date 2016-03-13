#pragma once

#include <game/server/entity.h>
#define BOOMERANG_MAX_HIT_TARGETS 16

class CBoomerang;

class CBoomerangProj : public CEntity
{
public:
	CBoomerangProj(CGameWorld *pGameWorld, vec2 Pos, CTargetAble *pOwner, CMap *pMap, CBoomerang *pBoomerang, vec2 Direction, int Damage);

	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	void Destroy();

protected:
	bool HitCharacter();

private:
	CBoomerang *m_pBoomerang;
	CTargetAble *m_pOwner;
	vec2 m_Direction;
	vec2 m_Vel;
	int64 m_SpawnTime;
	int m_Damage;
	int m_State;

	CTargetAble *m_apHitTargets[BOOMERANG_MAX_HIT_TARGETS];
};
