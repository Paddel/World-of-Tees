
#include "spawner.h"

#include <game/server/npc/_include.h>
#include <game/server/gamecontext.h>

#define BOSS_RESPAWN_TIME 300

CSpawner::CSpawner(CGameContext *pGameServer, CMap *pMap, vec2 Pos, char *pType, int MaxNum, int RespawnTime)
{
	m_pGameServer = pGameServer;
	m_pServer = pGameServer->Server();
	m_pMap = pMap;
	m_Pos = Pos+vec2(0, 1);
	str_copy(m_aType, pType, sizeof(m_aType));
	m_MaxNum = MaxNum;
	m_RespawnTime = RespawnTime;
	m_RespawnTimer = pGameServer->Server()->Tick() + pGameServer->Server()->TickSpeed()*RespawnTime;

	CNpc *pNpc = SpawnNpc(pType);
	m_Active = pNpc?true:false;
	if(pNpc)
		pNpc->OnDeath(-1, WEAPON_WORLD, vec2(0, 0));

	if(Active() == false)
		dbg_msg("Spawner", "Type %s not found on map %s and Pos %f %f", pType, pMap->GetName(), Pos.x, Pos.y);
}

CNpc *CSpawner::SpawnNpc(char *pName)
{
	int Type = GetNpcType(pName);
	if(Type == -1)
		return NULL;

	return CreateNpc(Type, GameServer(), Map(), this, m_Pos);
}

void CSpawner::Tick()
{
	if(!Active())
		return;

	if(m_RespawnTimer < Server()->Tick())
	{
		if(GetNum() < m_MaxNum)
		{
			CNpc *pNpc = SpawnNpc(m_aType);
			pNpc->Spawn();
		}

		m_RespawnTimer = Server()->Tick() + Server()->TickSpeed()*m_RespawnTime;
	}
}

void CSpawner::OnActive(int64 LastActive)
{
	float Secs = (Server()->Tick()-LastActive)/(float)Server()->TickSpeed();

	int MissedSpawns = Secs/m_RespawnTime;
	for(int i = 0; i < MissedSpawns; i++)
	{
		if(GetNum() >= m_MaxNum)
			break;

		CNpc *pNpc = SpawnNpc(m_aType);
		pNpc->Spawn();
	}
}

CBossSpawner::CBossSpawner(CGameContext *pGameServer, CMap *pMap, vec2 Pos, char *pType, int DoorID)
	:	CSpawner(pGameServer, pMap, Pos, pType, 1, BOSS_RESPAWN_TIME)

{
	m_DoorID = DoorID;
}

void CBossSpawner::Tick()
{
	CSpawner::Tick();

	if(GetNum() == 0)
		Map()->WorldSection()->OnRemote(CRemote::REMOTETYPE_DOOR, m_DoorID);
}