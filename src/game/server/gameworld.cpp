/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "gameworld.h"
#include "entity.h"
#include "gamecontext.h"

#include <algorithm>
#include <utility>

//////////////////////////////////////////////////
// game world
//////////////////////////////////////////////////
CGameWorld::CGameWorld()
{
	m_pGameServer = 0x0;
	m_pServer = 0x0;

	m_Paused = false;
	m_ResetRequested = false;
	for(int i = 0; i < NUM_ENTTYPES; i++)
		m_apFirstEntityTypes[i] = 0;
}

CGameWorld::~CGameWorld()
{
	// delete all entities
	for(int i = 0; i < NUM_ENTTYPES; i++)
		while(m_apFirstEntityTypes[i])
			delete m_apFirstEntityTypes[i];
}

void CGameWorld::SetGameServer(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();
}

CEntity *CGameWorld::FindFirst(int Type)
{
	return Type < 0 || Type >= NUM_ENTTYPES ? 0 : m_apFirstEntityTypes[Type];
}

int CGameWorld::FindEntities(vec2 Pos, float Radius, CEntity **ppEnts, int Max, int Type, CMap *pMap)
{
	if(Type < 0 || Type >= NUM_ENTTYPES)
		return 0;

	int Num = 0;
	for(CEntity *pEnt = m_apFirstEntityTypes[Type];	pEnt; pEnt = pEnt->m_pNextTypeEntity)
	{
		if(distance(pEnt->m_Pos, Pos) < Radius+pEnt->m_ProximityRadius && pMap == pEnt->Map())
		{
			if(ppEnts)
				ppEnts[Num] = pEnt;
			Num++;
			if(Num == Max)
				break;
		}
	}

	return Num;
}

void CGameWorld::InsertEntity(CEntity *pEnt)
{
#ifdef CONF_DEBUG
	for(CEntity *pCur = m_apFirstEntityTypes[pEnt->m_ObjType]; pCur; pCur = pCur->m_pNextTypeEntity)
		dbg_assert(pCur != pEnt, "err");
#endif

	// insert it
	if(m_apFirstEntityTypes[pEnt->m_ObjType])
		m_apFirstEntityTypes[pEnt->m_ObjType]->m_pPrevTypeEntity = pEnt;
	pEnt->m_pNextTypeEntity = m_apFirstEntityTypes[pEnt->m_ObjType];
	pEnt->m_pPrevTypeEntity = 0x0;
	m_apFirstEntityTypes[pEnt->m_ObjType] = pEnt;
}

void CGameWorld::DestroyEntity(CEntity *pEnt)
{
	pEnt->m_MarkedForDestroy = true;
}

void CGameWorld::RemoveEntity(CEntity *pEnt)
{
	// not in the list
	if(!pEnt->m_pNextTypeEntity && !pEnt->m_pPrevTypeEntity && m_apFirstEntityTypes[pEnt->m_ObjType] != pEnt)
		return;

	// remove
	if(pEnt->m_pPrevTypeEntity)
		pEnt->m_pPrevTypeEntity->m_pNextTypeEntity = pEnt->m_pNextTypeEntity;
	else
		m_apFirstEntityTypes[pEnt->m_ObjType] = pEnt->m_pNextTypeEntity;
	if(pEnt->m_pNextTypeEntity)
		pEnt->m_pNextTypeEntity->m_pPrevTypeEntity = pEnt->m_pPrevTypeEntity;

	// keep list traversing valid
	if(m_pNextTraverseEntity == pEnt)
		m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;

	pEnt->m_pNextTypeEntity = 0;
	pEnt->m_pPrevTypeEntity = 0;
}

//
void CGameWorld::Snap(int SnappingClient)
{
	for(int i = 0; i < NUM_ENTTYPES; i++)
	{
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			pEnt->Snap(SnappingClient);
			pEnt = m_pNextTraverseEntity;
		}
	}

	int NumMaps = Server()->m_MapLoader.GetNumMaps();
	for(int i = 0; i < NumMaps; i++)
	{
		CMap *pMap = Server()->m_MapLoader.GetMap(i);
		if(pMap->Active() == false)
			continue;

		CWorldSection *pSection = pMap->WorldSection();
		for(int i = 0; i < pSection->m_lpPlayerItems.size(); i++)
		{
			if(!pSection->m_lpPlayerItems[i])
				continue;

			pSection->m_lpPlayerItems[i]->Snap(SnappingClient);
		}
	}
}

void CGameWorld::Reset()
{
	// reset all entities
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			pEnt->Reset();
			pEnt = m_pNextTraverseEntity;
		}
	RemoveEntities();

	GameServer()->m_pController->PostReset();
	RemoveEntities();

	m_ResetRequested = false;
}

void CGameWorld::RemoveEntities()
{
	// destroy objects marked for destruction
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			if(pEnt->m_MarkedForDestroy)
			{
				RemoveEntity(pEnt);
				pEnt->Destroy();
			}
			pEnt = m_pNextTraverseEntity;
		}
}

void CGameWorld::AddPlayerItem(CPlayerItem *pPlayerItem, CMap *pMap)
{
	CWorldSection *pSection = pMap->WorldSection();
	dbg_assert(pSection->m_lpPlayerItems.size() < MAX_PLAYER_ITEMS, "Reached MAX_PLYYER_ITEMS");

	pSection->m_lpPlayerItems.add(pPlayerItem);
}

void CGameWorld::RemovePlayerItem(int Index, CMap *pMap)
{
	CWorldSection *pSection = pMap->WorldSection();

	if(Index < 0 ||  Index >= pSection->m_lpPlayerItems.size())
		return;

	pSection->m_lpPlayerItems.remove_index(Index);

	//predict id change in client because ids will be moved
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (!Server()->ClientIngame(i))
			continue;

		CPlayer *pPlayer = GameServer()->m_apPlayers[i];
		if(!pPlayer || pPlayer->GetMap() != pMap)
			continue;
		

		int* map = Server()->GetIdMap(i);

		for(int j = 0; j < VANILLA_MAX_CLIENTS; j++)
		{
			if(map[j] == -1)
				continue;

			if(Index == map[j])
				map[j] = -1;
			else if(Index < map[j])
				map[j]--;
		}
	}

	//hooked player should be moved to be moved
	for(int i = 0; i < pSection->m_lpPlayerItems.size(); i++)
	{
		CPlayerItem *pPlayerItem = pSection->m_lpPlayerItems[i];

		int HookedPayer = pPlayerItem->GetCore()->m_HookedPlayer;
		if(HookedPayer == -1)
			continue;

		if(HookedPayer == Index)//hooked player died. this should not happen.
			pPlayerItem->GetCore()->m_HookedPlayer = -1;
		else if(Index < pPlayerItem->GetCore()->m_HookedPlayer)
			pPlayerItem->GetCore()->m_HookedPlayer--;
	}
}

void CGameWorld::RemovePlayerItem(CPlayerItem *pPlayerItem, CMap *pMap)
{
	RemovePlayerItem(GetPlayerItemId(pPlayerItem, pMap), pMap);
}

int CGameWorld::GetPlayerItemId(CPlayerItem *pPlayerItem, CMap *pMap)
{
	CWorldSection *pSection = pMap->WorldSection();
	for(int i = 0; i < pSection->m_lpPlayerItems.size(); i++)
		if(pSection->m_lpPlayerItems[i] == pPlayerItem)
			return i;
	return -1;
}

int CGameWorld::GetPlayerID(CPlayerItem *pPlayerItem)
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i] == pPlayerItem)
			return i;
	return -1;
}

void CGameWorld::AddTargetAble(CTargetAble *pTarget, CMap *pMap)
{
	CWorldSection *pSection = pMap->WorldSection();
	for(int i = 0; i < pSection->m_lpTargetAbles.size(); i++)
	{
		if(pSection->m_lpTargetAbles[i] == pTarget)
			return;//already in list
	}

	pSection->m_lpTargetAbles.add(pTarget);
}

void CGameWorld::RemoveTargetAble(CTargetAble *pTarget, CMap *pMap)
{
	CWorldSection *pSection = pMap->WorldSection();
	for(int i = 0; i < pSection->m_lpTargetAbles.size(); i++)
	{
		if(pSection->m_lpTargetAbles[i] == pTarget)
		{
			pSection->m_lpTargetAbles.remove_index(i);
			return;
		}
	}
}

int CGameWorld::GetPlayerID(CTargetAble *pTargetAble)
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->GetPlayerChar(i) == pTargetAble)
			return i;
	return -1;
}

//find the closest target
CTargetAble *CGameWorld::FindClosestTarget(vec2 Pos, float Radius, CTargetAble *pNotThis, bool CollisionCheck, CMap *pMap)
{
	CTargetAble * pClosest = NULL;
	float ClosestRange = Radius*Radius;
	CWorldSection *pSection = pMap->WorldSection();

	for(int i = 0; i < pSection->m_lpTargetAbles.size(); i++)
	{
		CTargetAble *pTarget = pSection->m_lpTargetAbles[i];
	    if (pTarget == pNotThis)
			continue;

		vec2 TargetPos = pTarget->GetClosestPos(Pos);

		vec2 CheckPos = TargetPos - Pos;
		if(abs(CheckPos.x) > Radius || abs(CheckPos.y) > Radius)//if one coordinate is over range
			continue;

		if(CollisionCheck)
		{
			if(pMap->Collision()->IntersectLine(Pos, TargetPos, NULL, NULL))
				continue;
		}
		
		
		float Len = CheckPos.x*CheckPos.x+CheckPos.y*CheckPos.y;
		if (Len < Radius*Radius)
		{
			if (Len <= ClosestRange)
			{
				pClosest = pTarget;
				ClosestRange = Len;			
			}		
		}
		
	}

	//to get the length do sqrt(ClosestRange); :))
	
	return pClosest;
}

CTargetAble *CGameWorld::IntersectTarget(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, CTargetAble *pNotThis, CMap *pMap)
{
	// Find other targets
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	CTargetAble *pClosest = 0;
	CWorldSection *pSection = pMap->WorldSection();

	for(int i = 0; i < pSection->m_lpTargetAbles.size(); i++)
	{
		CTargetAble *pTarget = pSection->m_lpTargetAbles[i];
		if (pTarget == pNotThis)
			continue;

		vec2 IntersectPos = pTarget->GetIntersectPos(Pos0, Pos1);

		float Len = distance(pTarget->GetClosestPos(IntersectPos), IntersectPos);
		if(Len < pTarget->GetProximityRadius()+Radius)
		{
			Len = distance(Pos0, IntersectPos);
			if(Len < ClosestLen)
			{
				NewPos = IntersectPos;
				ClosestLen = Len;
				pClosest = pTarget;
			}
		}
	}

	return pClosest;
}

void CGameWorld::Tick()
{
	if(m_ResetRequested)
		Reset();

	if(!m_Paused)
	{
		if(GameServer()->m_pController->IsForceBalanced())
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, "Teams have been balanced");
		// update all objects
		for(int i = 0; i < NUM_ENTTYPES; i++)
		{
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->Tick();
				pEnt = m_pNextTraverseEntity;
			}

		}

		int NumMaps = Server()->m_MapLoader.GetNumMaps();
		for(int i = 0; i < NumMaps; i++)
		{
			CMap *pMap = Server()->m_MapLoader.GetMap(i);
			if(pMap->Active() == false)
				continue;

			CWorldSection *pSection = pMap->WorldSection();
			for(int i = 0; i < pSection->m_lpPlayerItems.size(); i++)
			{
				if(!pSection->m_lpPlayerItems[i])
					continue;

				pSection->m_lpPlayerItems[i]->Tick();
				pSection->m_lpPlayerItems[i]->PostTick();
			}
		}

		for(int i = 0; i < NUM_ENTTYPES; i++)
		{
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->TickDefered();
				pEnt = m_pNextTraverseEntity;
			}
		}
	}
	else
	{
		// update all objects
		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->TickPaused();
				pEnt = m_pNextTraverseEntity;
			}
	}

	RemoveEntities();
	UpdatePlayerMaps();
}


// TODO: should be more general
CCharacter *CGameWorld::IntersectCharacter(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, CEntity *pNotThis)
{
	// Find other players
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	CCharacter *pClosest = 0;

	CCharacter *p = (CCharacter *)FindFirst(ENTTYPE_CHARACTER);
	for(; p; p = (CCharacter *)p->TypeNext())
 	{
		if(p == pNotThis)
			continue;

		vec2 IntersectPos = closest_point_on_line(Pos0, Pos1, p->m_Pos);
		float Len = distance(p->m_Pos, IntersectPos);
		if(Len < p->m_ProximityRadius+Radius)
		{
			Len = distance(Pos0, IntersectPos);
			if(Len < ClosestLen)
			{
				NewPos = IntersectPos;
				ClosestLen = Len;
				pClosest = p;
			}
		}
	}

	return pClosest;
}


CCharacter *CGameWorld::ClosestCharacter(vec2 Pos, float Radius, CMap *pMap, CEntity *pNotThis)
{
	// Find other players
	float ClosestRange = Radius*2;
	CCharacter *pClosest = 0;

	CCharacter *p = (CCharacter *)GameServer()->m_World.FindFirst(ENTTYPE_CHARACTER);
	for(; p; p = (CCharacter *)p->TypeNext())
 	{
		if(p == pNotThis)
			continue;

		if(p->m_pMap != pMap)
			continue;

		float Len = distance(Pos, p->m_Pos);
		if(Len < p->m_ProximityRadius+Radius)
		{
			if(Len < ClosestRange)
			{
				ClosestRange = Len;
				pClosest = p;
			}
		}
	}

	return pClosest;
}

bool distCompare(std::pair<float,int> a, std::pair<float,int> b)
{
	return (a.first < b.first);
}

void CGameWorld::UpdatePlayerMaps()
{
	//if (Server()->Tick() % 5 != 0) return;
	std::pair<float,int> dist[MAX_PLAYER_ITEMS];
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (!Server()->ClientIngame(i)) continue;
		int* map = Server()->GetIdMap(i);
		CPlayer *pPlayer = GameServer()->m_apPlayers[i];
		if(!pPlayer) continue;
		CWorldSection *pSection = pPlayer->GetMap()->WorldSection();
		int Size = pSection->m_lpPlayerItems.size();

		CMap *pMap = Server()->CurrentMap(i);
		int OwnPlayerItemID = -1;

		// compute distances
		for (int j = 0; j < MAX_PLAYER_ITEMS; j++)//maybe bottleneck
		{
			dist[j].first = 1e10;

			if(j >= Size)
				continue;

			dist[j].second = j;
			if(pSection->m_lpPlayerItems[j] == pPlayer)
			{
				OwnPlayerItemID = j;

				if(j != 0)
				{
					std::pair<float,int> DistBuf = dist[0];
					dist[0] = dist[j];
					dist[j] = DistBuf;
				}
				continue;
			}
			CMap *pItemMap = pSection->m_lpPlayerItems[j]? pSection->m_lpPlayerItems[j]->GetMap() : NULL;
			if(!pItemMap || pItemMap != pPlayer->GetMap() || pItemMap->Active() == false || pSection->m_lpPlayerItems[j]->Spectating())
				continue;

			vec2 CheckPos = pSection->m_lpPlayerItems[j]->GetPos()-pPlayer->m_ViewPos;
			dist[j].first = CheckPos.x*CheckPos.x+CheckPos.y*CheckPos.y;//distance(pPlayer->m_ViewPos, m_lpPlayerItems[j]->GetPos());
		}

		// compute reverse map
		int rMap[MAX_PLAYER_ITEMS];
		for (int j = 0; j < MAX_PLAYER_ITEMS; j++)
		{
			rMap[j] = -1;
		}
		for (int j = 1; j < VANILLA_MAX_CLIENTS; j++)
		{

			if (map[j] == -1) continue;
			if (dist[map[j]].first > 1e9) map[j] = -1;
			else rMap[map[j]] = j;
		}

		// always send the player himself
		//dist[i].first = 0;

		std::nth_element(&dist[1], &dist[VANILLA_MAX_CLIENTS], &dist[Size], distCompare);


		int mapc = 1;
		for (int j = 1; j < VANILLA_MAX_CLIENTS; j++)
		{
			int k = dist[j].second;
			if (rMap[k] != -1 || dist[j].first > 1e9) continue;
			while (mapc < VANILLA_MAX_CLIENTS && map[mapc] != -1) mapc++;
			if (mapc < VANILLA_MAX_CLIENTS)
			{
				map[mapc] = k;
			}
		}
		for (int j = Size - 1; j > VANILLA_MAX_CLIENTS - 1; j--)
		{
			int k = dist[j].second;
			if (rMap[k] != -1)
			{
				map[rMap[k]] = -1;
			}
		}

		map[0] = OwnPlayerItemID;
		//map[VANILLA_MAX_CLIENTS - 1] = -1; // player with empty name to say chat msgs
	}
}