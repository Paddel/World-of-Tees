
#include <game/server/gamecontext.h>
#include <game/server/targetable.h>
#include <game/server/entities/boomerang_proj.h>

#include "boomerang.h"

CBoomerang::CBoomerang(CGameContext *pGameServer, CTargetAble *pOwner, int Damage)
	: CWeapon(pGameServer, pOwner, WEAP_BOOMERANG, Damage, -1)
{
	int NumSnapIDs = sizeof(m_aSnapIDs) / sizeof(m_aSnapIDs[0]);
	for (int i = 0; i < NumSnapIDs; i++)
		m_aSnapIDs[i] = pGameServer->Server()->SnapNewID();

	m_Thrown = false;
}

void CBoomerang::Return()
{
	m_Thrown = false;
}

void CBoomerang::Destroy()
{
	int NumSnapIDs = sizeof(m_aSnapIDs) / sizeof(m_aSnapIDs[0]);
	for (int i = 0; i < NumSnapIDs; i++)
		GameServer()->Server()->SnapFreeID(m_aSnapIDs[i]);
}


int CBoomerang::FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos)
{
	if (m_Thrown)
		return 0;

	GameServer()->CreateSound(PlayerPos, SOUND_HAMMER_FIRE, Owner()->TargetMap());

	new CBoomerangProj(&GameServer()->m_World, PlayerPos, Owner(), Owner()->TargetMap(), this, Direction, Damage());
	m_Thrown = true;
	return 0;
}

bool CBoomerang::SpecialWorldSnap(int SnappingClient, vec2 Pos, vec2 Direction, int Attacktick)
{
	if (m_Thrown)
		return true;

	int InvX = Direction.x > 0 ? -1 : 1;
	RenderLaser(m_aSnapIDs[0], Server()->Tick(), Pos- vec2(32 * InvX, 32), Pos);
	RenderLaser(m_aSnapIDs[1], Server()->Tick(), Pos - vec2(32 * InvX, 32), Pos - vec2(0, 64));

	return true;
}


bool CBoomerang::ShootAnim()
{
	return m_Thrown == false;
}