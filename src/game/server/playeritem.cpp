
#include <engine/server/maploader.h>
#include <game/server/gamecontext.h>

#include "playeritem.h"

CPlayerItem::CPlayerItem()
{
	m_Inited = false;
	ResetInput();
}

void CPlayerItem::ResetInput()
{
	m_Input.m_Direction = 0;
	m_Input.m_Hook = 0;
	// simulate releasing the fire button
	if((m_Input.m_Fire&1) != 0)
		m_Input.m_Fire++;
	m_Input.m_Fire &= INPUT_STATE_MASK;
	m_Input.m_Jump = 0;
}

void CPlayerItem::Init(CGameContext *pGameServer, CMap *pMap, vec2 Pos, int Type, int Flags)
{
	m_pGameServer = pGameServer;
	m_Type = Type;
	m_InterFlags = Flags;

	GetCore()->Reset();
	GetCore()->Init(&GameServer()->m_World.m_Core, pMap->Collision(), GameServer(), pMap);
	GetCore()->m_Pos = Pos;

	m_Inited = true;
	GameServer()->m_World.AddPlayerItem(this, pMap);
}

void CPlayerItem::UpdateCore(float Slow)
{
	dbg_assert(m_Inited, "PlayerItem: Not initieted.");

	GetCore()->m_Input = m_Input;
	GetCore()->Tick(true, (bool)(m_InterFlags&INTERFLAG_COL));
	GetCore()->Move(Slow);
}