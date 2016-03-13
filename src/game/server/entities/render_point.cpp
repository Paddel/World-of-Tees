/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "projectile.h"

CRenderPoint::CRenderPoint(CGameWorld *pGameWorld, int Type, vec2 Pos, vec2 Dir, bool Predict, CMap *pMap)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE, pMap, 1)
{
	m_Type = Type;
	m_Pos = Pos;
	m_Direction = Dir;
	m_Predict = Predict;
	m_StartTick = Server()->Tick();
	GameWorld()->InsertEntity(this);
}

void CRenderPoint::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CRenderPoint::FillInfo(CNetObj_Projectile *pProj)
{
	vec2 ShownVel = normalize(m_Direction)*300.0f;//(10.0f+m_Predict*90.0f);
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(ShownVel.x);
	pProj->m_VelY = (int)(ShownVel.y);
	pProj->m_StartTick = m_Predict?m_StartTick:Server()->Tick()-10;
	pProj->m_Type = m_Type;
}

void CRenderPoint::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_aSnapIDs[0], sizeof(CNetObj_Projectile)));
	if(pProj)
		FillInfo(pProj);
}
