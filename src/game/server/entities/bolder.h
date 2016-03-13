#pragma once

#include <game/server/entity.h>
#include <game/server/remote.h>

#define BOLDER_MAX_HIT_TARGETS 16

class CBolderSpawner : public CRemote
{
private:
	CGameWorld *m_pGameWorld;
	CMap *m_pMap;
	vec2 m_Pos;
	int m_Damage;
	int64 m_ActivationTime;

public:
	CBolderSpawner(CGameWorld *pGameWorld, CMap *pMap, vec2 Pos, int Damage, int ID);

	virtual void Activate();
};

class CBolder : public CEntity
{
public:
	CBolder(CGameWorld *pGameWorld, vec2 Pos, CTargetAble *pOwner, CMap *pMap, int Damage);

	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

protected:
	void HitCharacter();

private:
	CTargetAble *m_pOwner;
	int64 m_DisappearTime;
	int m_Damage;
	int m_State;

	CTargetAble *m_apHitTargets[BOLDER_MAX_HIT_TARGETS];
};
