#pragma once

#include <game/server/entity.h>
#define SHURIKEN_MAX_HIT_TARGETS 16

class CShurikenProj : public CEntity
{
public:
	CShurikenProj(CGameWorld *pGameWorld, vec2 Pos, CTargetAble *pOwner, CMap *pMap, vec2 Direction, int Damage);

	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	void Destroy();

protected:
	bool HitCharacter();

private:
	CTargetAble *m_pOwner;
	vec2 m_Direction;
	vec2 m_Vel;
	int64 m_SpawnTime;
	int m_Damage;
	int m_State;
	float m_Rotation;
	
	CTargetAble *m_apHitTargets[SHURIKEN_MAX_HIT_TARGETS];
};
