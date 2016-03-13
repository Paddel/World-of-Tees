
#include <game/balancing.h>
#include <game/server/gamecontext.h>
#include <game/server/targetable.h>
#include <game/server/entities/character.h>
#include <game/server/entities/shuriken_proj.h>

#include "shuriken.h"


CShuriken::CShuriken(CGameContext *pGameServer, CTargetAble *pOwner, int Damage)
	: CWeapon(pGameServer, pOwner, WEAP_SWOSHROD, Damage, 250)
{
	static const int NumSnapIDs = sizeof(m_aSnapIDs)/sizeof(m_aSnapIDs[0]);
	for(int i = 0; i < NumSnapIDs; i++)
		m_aSnapIDs[i] = pGameServer->Server()->SnapNewID();
}

void CShuriken::Destroy()
{
	static const int NumSnapIDs = sizeof(m_aSnapIDs) / sizeof(m_aSnapIDs[0]);
	for(int i = 0; i < NumSnapIDs; i++)
		GameServer()->Server()->SnapFreeID(m_aSnapIDs[i]);
}

int CShuriken::FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos)
{
	new CShurikenProj(&GameServer()->m_World, ProjPos, Owner(), Owner()->TargetMap(), Direction, Damage());
	return 250;
}

bool CShuriken::SpecialWorldSnap(int SnappingClient, vec2 Pos, vec2 Direction, int Attacktick)
{
	int InvX = Direction.x > 0 ? -1 : 1;
	vec2 Shift = vec2(-18 * InvX, -14);

	vec2 From = Pos + Shift - vec2(16, 0);
	vec2 To = Pos + Shift + vec2(16, 0);
	RenderLaser(m_aSnapIDs[0], Server()->Tick(), From, To);
	RenderLaser(m_aSnapIDs[1], Server()->Tick(), From, From);
	From = Pos + Shift - vec2(0, 16);
	To = Pos + Shift + vec2(0, 16);
	RenderLaser(m_aSnapIDs[2], Server()->Tick(), From, To);
	RenderLaser(m_aSnapIDs[3], Server()->Tick(), From, From);
	return true;
}