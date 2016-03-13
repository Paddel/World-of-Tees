#pragma once

#include <game/server/entity.h>

const int MoneyPhysSize = 14;

class CMoney : public CEntity
{
public:
	CMoney(CGameWorld *pGameWorld, CMap *pMap, vec2 Pos, vec2 m_StartVel, int Amount, int Owner);

	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	vec2 m_Vel;
	int64 m_SpawnTime;
	int m_Amount;
	int m_Owner;

	bool IsGrounded();
};