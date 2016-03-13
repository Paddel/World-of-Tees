
#include <engine/server.h>
#include <engine/server/maploader.h>
#include <game/server/gamecontext.h>

#include "door.h"

CDoor::CDoor(CGameWorld *pGameWorld, CMap *pMap, vec2 From, int PosType, int ID, int Time)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_DOOR, pMap, 2), CRemote(CRemote::REMOTETYPE_DOOR, ID)
{
	m_Pos = From;
	m_PosType = PosType;
	m_Time = Time;

	vec2 To = m_Pos+Direction()*COLLLASER_DIST;

	Map()->Collision()->IntersectLine(m_Pos, To, NULL, &To);
	m_MaxDoorDist = distance(m_Pos, To);

	m_DoorDist = 0;

	GameWorld()->InsertEntity(this);
	Map()->Collision()->AddColLaser(this);
	//GameServer()->m_pDoors.add(this);
	Map()->WorldSection()->AddRemote(this);

	m_ActivateTime = 0;
}

void CDoor::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	RenderLaser(m_aSnapIDs[0], Server()->Tick()-1, m_Pos, To());
	RenderLaser(m_aSnapIDs[1], Server()->Tick()-1, m_Pos, m_Pos);
}

void CDoor::TickDefered()
{
	if(m_Activated)
	{
		if(m_DoorDist > 0.0f)
			m_DoorDist = clamp(m_DoorDist-20.0f, 0.0f, m_MaxDoorDist);
		else
			m_Activated = false;

		m_ActivateTime = Server()->Tick()+Server()->TickSpeed()*m_Time;

	}
	else if(m_DoorDist < m_MaxDoorDist && m_ActivateTime < Server()->Tick())
	{
		m_DoorDist = clamp(m_DoorDist+20.0f, 0.0f, m_MaxDoorDist);
	}
}

vec2 CDoor::To()
{
	return m_Pos+Direction()*m_DoorDist;
}

vec2 CDoor::Direction()
{
	if(m_PosType == DOORTYPE_V)
		return vec2(0, 1);
	else if(m_PosType == DOORTYPE_H)
		return vec2(1, 0);

	return vec2(0, 0);
}

bool CDoor::Active()
{
	return m_DoorDist != 0.0f;
}

CDoorTrigger::CDoorTrigger(CGameWorld *pGameWorld, CMap *pMap, vec2 Pos)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_DOOR, pMap, 5)
{
	m_Pos = Pos;

	GameWorld()->InsertEntity(this);
}

void CDoorTrigger::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	RenderLaser(m_aSnapIDs[0], Server()->Tick()-2, m_Pos+vec2(16, 0), m_Pos+vec2(0, 16));
	RenderLaser(m_aSnapIDs[1], Server()->Tick()-2, m_Pos+vec2(0, 16), m_Pos+vec2(-16, 0));
	RenderLaser(m_aSnapIDs[2], Server()->Tick()-2, m_Pos+vec2(-16, 0), m_Pos+vec2(0, -16));
	RenderLaser(m_aSnapIDs[3], Server()->Tick()-2, m_Pos+vec2(0, -16), m_Pos+vec2(16, 0));
	RenderProjectile(m_aSnapIDs[4], Server()->Tick()-1, m_Pos, vec2(0, 0), WEAPON_SHOTGUN);
}