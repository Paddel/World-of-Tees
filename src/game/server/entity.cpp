/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <engine/server/maploader.h>

#include "entity.h"
#include "gamecontext.h"

//////////////////////////////////////////////////
// Entity
//////////////////////////////////////////////////
CEntity::CEntity(CGameWorld *pGameWorld, int ObjType, CMap *pMap, int SnapIDNum)
	: m_SnapIDNum(SnapIDNum)
{
	m_pGameWorld = pGameWorld;

	m_ObjType = ObjType;
	m_pMap = pMap;
	m_Pos = vec2(0,0);
	m_ProximityRadius = 0;

	m_MarkedForDestroy = false;
	m_aSnapIDs = new int[m_SnapIDNum];

	for(int i = 0; i < m_SnapIDNum; i++)
		m_aSnapIDs[i] = Server()->SnapNewID();

	m_pPrevTypeEntity = 0;
	m_pNextTypeEntity = 0;
}

CEntity::~CEntity()
{
	GameWorld()->RemoveEntity(this);
	
	for(int i = 0; i < m_SnapIDNum; i++)
		Server()->SnapFreeID(m_aSnapIDs[i]);
}

int CEntity::NetworkClipped(int SnappingClient)
{
	return NetworkClipped(SnappingClient, m_Pos);
}

int CEntity::NetworkClipped(int SnappingClient, vec2 CheckPos)
{
	return GameServer()->NetworkClipped(Map(), SnappingClient, CheckPos);
}

bool CEntity::GameLayerClipped(vec2 CheckPos)
{
	return round(CheckPos.x)/32 < -200 || round(CheckPos.x)/32 > Map()->Collision()->GetWidth()+200 ||
			round(CheckPos.y)/32 < -200 || round(CheckPos.y)/32 > Map()->Collision()->GetHeight()+200 ? true : false;
}

void CEntity::RenderLaser(int ID, int64 Tick, vec2 From, vec2 To)
{
	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)To.x;
	pObj->m_Y = (int)To.y;
	pObj->m_FromX = (int)From.x;
	pObj->m_FromY = (int)From.y;
	pObj->m_StartTick = Tick;
}

void CEntity::RenderProjectile(int ID, int64 Tick, vec2 Pos, vec2 Vel, int Type)
{

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, ID, sizeof(CNetObj_Projectile)));
	if(!pProj)
		return;

	pProj->m_X = (int)Pos.x;
	pProj->m_Y = (int)Pos.y;
	pProj->m_VelX = (int)(Vel.x);
	pProj->m_VelY = (int)(Vel.y);
	pProj->m_StartTick = Tick;
	pProj->m_Type = Type;
}

void CEntity::RenderPickup(int ID, vec2 Pos, int Type, int SubType)
{
	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)Pos.x;
	pP->m_Y = (int)Pos.y;
	pP->m_Type = Type;
	pP->m_Subtype = SubType;
}

void CEntity::RenderFlag(vec2 Pos, int Team)
{
	CNetObj_Flag *pFlag = (CNetObj_Flag *)Server()->SnapNewItem(NETOBJTYPE_FLAG, Team, sizeof(CNetObj_Flag));
	if(!pFlag)
		return;

	pFlag->m_X = (int)Pos.x;
	pFlag->m_Y = (int)Pos.y;
	pFlag->m_Team = Team;
}