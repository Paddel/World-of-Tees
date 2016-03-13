/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PROJECTILE_H
#define GAME_SERVER_ENTITIES_PROJECTILE_H

#include <game/server/entities/render_point.h>

class CProjectile : public CRenderPoint
{
public:
	CProjectile(CGameWorld *pGameWorld, CTargetAble *pOwner, int Type, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon, CMap *pMap);

	vec2 GetPos(float Time);

	virtual void Tick();
	virtual void TickPaused();

private:
	CTargetAble *m_pOwner;
	vec2 m_SpawnPos;
	int m_LifeSpan;
	int m_Damage;
	int m_SoundImpact;
	int m_Weapon;
	float m_Force;
	bool m_Explosive;
};

#endif
