#pragma once

#include <game/collaser.h>
#include <game/server/entity.h>
#include <game/server/remote.h>

enum
{
	DOORTYPE_V=0,
	DOORTYPE_H,
	NUM_DOORTYPES,
};

class CDoor : public CColLaser, public CEntity, public CRemote
{
private:
	int64 m_ActivateTime;
	int m_PosType;
	int m_Time;
	float m_DoorDist;
	float m_MaxDoorDist;
	bool m_Activated;

	vec2 Direction();

public:
	CDoor(CGameWorld *pGameWorld, CMap *pMap, vec2 From, int PosType, int ID, int Time);
	
	virtual void Activate() { m_Activated = true; }

	virtual void Snap(int SnappingClient);
	virtual void TickDefered();

	virtual vec2 From() { return m_Pos; }
	virtual vec2 To();
	virtual bool Active();
};

class CDoorTrigger : public CEntity
{
public:
	CDoorTrigger(CGameWorld *pGameWorld, CMap *pMap, vec2 Pos);

	virtual void Snap(int SnappingClient);
};