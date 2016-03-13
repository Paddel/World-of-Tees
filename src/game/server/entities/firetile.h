/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PROJECTILE_H
#define GAME_SERVER_ENTITIES_PROJECTILE_H

#include <game/server/entities/render_point.h>

class CFireTile : public CRenderPoint
{
public:
	CFireTile(CGameWorld *pGameWorld, CTargetAble *pOwner, vec2 Pos, vec2 Dir, int Damage, int Weapon, CMap *pMap);

	virtual void Tick();
	virtual void TickPaused();

private:
	CTargetAble *m_pOwner;
	vec2 m_SpawnPos;
	int m_Damage;
	int m_Weapon;
	float m_Speed;
	int m_LifeSpan;
};

#endif
