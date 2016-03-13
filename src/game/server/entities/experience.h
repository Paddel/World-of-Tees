#pragma once

#include <game/server/entity.h>

class CExp : public CEntity
{
public:
	CExp(CGameWorld *pGameWorld, CMap *pMap, vec2 Pos, int Owner, int Amount);

	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	int m_Owner;
	int m_Amount;
};