#pragma once

class CRenderPoint : public CEntity
{
public:
	CRenderPoint(CGameWorld *pGameWorld, int Type, vec2 Pos, vec2 Dir, bool Predict, CMap *pMap);

	vec2 m_Direction;
	int m_StartTick;
	int m_Type;
	bool m_Predict;

	void FillInfo(CNetObj_Projectile *pProj);

	virtual void Reset();
	virtual void Tick() = 0;
	virtual void TickPaused() = 0;
	virtual void Snap(int SnappingClient);
};