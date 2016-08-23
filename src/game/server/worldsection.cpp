
#include <base/stringseperation.h>
#include <engine/server/maploader.h>
#include <game/server/gamecontext.h>
#include <game/server/npc/_include.h>

#include "worldsection.h"

#define MAX_RANDOM_MOBS 40
#define MAX_RANDOM_MOBS_PER_PLAYER 8

CWorldSection::CWorldSection(CMap *pMap, void *pGameServerInfo)
{
	m_pMap = pMap;
	m_pServer = pMap->Server();
	m_pGameServer = (CGameContext *)pGameServerInfo;

	m_LastActive = Server()->Tick();
}

void CWorldSection::Tick()
{
	m_LastActive = Server()->Tick();
	/*for(int i = 0; i < m_pNpcSpawners.size(); i++)
		m_pNpcSpawners[i]->Tick();

	for(int i = 0; i < m_pNpcs.size(); i++)
		m_pNpcs[i]->Tick();*/

	DoRandomMobSpawn();
	DoPuzzleCheck();

	for(int i = 0; i < m_pSpawners.size(); i++)
		m_pSpawners[i]->Tick();

	for (int i = 0; i < MAX_CHESTS_PER_MAP; i++)
		m_aChests[i].Tick();
}

void CWorldSection::OnActive()
{
	if(Server()->Tick()-m_LastActive > Server()->TickSpeed()*30)
	{
		KillTemporary();
	}

	for(int i = 0; i < m_pSpawners.size(); i++)
		m_pSpawners[i]->OnActive(m_LastActive);
}

void CWorldSection::OnNotActive()
{
	/*for(int i = 0; i < m_pNpcSpawners.size(); i++)
		m_pNpcSpawners[i]->OnNotActive();

	for(int i = 0; i < m_pNpcs.size(); i++)
		m_pNpcs[i]->OnNotActive();*/
}

bool CWorldSection::CanBeDeactivated()
{
	/*for(int i = 0; i < m_pNpcSpawners.size(); i++)
	{
		if(m_pNpcSpawners[i]->DoneSpawning() == false)
			return false;
	}*/
	return true;
}

void CWorldSection::InitTile(int Index, vec2 Pos)
{
	if(Index == TILE_NPC_SPAWN_GUARD)
	{
		CNpc *pNpc = new CGuard(GameServer(), Map(), this, Pos);
		pNpc->Spawn();
	}
	else if(Index == TILE_NPC_SPAWN_BEGGAR)
	{
		CNpc *pNpc = new CBeggar(GameServer(), Map(), this, Pos);
		pNpc->Spawn();
	}
	else if(Index == TILE_NPC_SPAWN_SHOP)
	{
		CNpc *pNpc = new CAmmoShop(GameServer(), Map(), this, Pos);
		pNpc->Spawn();
	}
}

void CWorldSection::InitExTile(int Index, vec2 Pos, char *pArgs)
{
	switch(Index)
	{
	case EXTILE_MAPTRANSITION_FROM: NewMapTransitionFrom(Pos, pArgs); break;
	case EXTILE_MAPTRANSITION_TO:	NewMapTransitionTo(Pos, pArgs); break;
	case EXTILE_NPC_HELPER:			NewHelper(Pos, pArgs); break;
	case EXTILE_NPC_SPAWNER:		NewNpcSpawner(Pos, pArgs); break;
	case EXTILE_PUZZLE:				NewPuzzle(Pos, pArgs); break;
	case EXTILE_NPC_TICKETSELLER:	NewNpcTicketSeller(Pos, pArgs); break;
	case EXTILE_BOSS_SPAWNER:		NewBossSpawner(Pos, pArgs); break;
	case EXTILE_CHEST:				NewChest(Pos, pArgs); break;
	}
}

void CWorldSection::NewMapTransitionTo(vec2 Pos, char *pArgs)
{
	int ID = GameServer()->GetMapInteger(&pArgs, "CMapTransition 'To'", Pos, Map());
	m_pMapTransitions.add( new CMapTransition(ID, Pos) );
}

void CWorldSection::HandleTransitionFromExtra(char *pExtra, int& TicketLevel)
{
	char aBuf[MAX_EXTENTED_STR];
	char *pBuf = aBuf;
	str_copy(aBuf, pExtra, sizeof(aBuf));
	char *pArg = GetSepStr(';', &pBuf);
	if(str_comp(pArg, "-") == 0)
		return;

	while(pArg[0])
	{
		char *pName = GetSepStr('=', &pArg);
		if(str_comp(pName, "ticket") == 0)
			TicketLevel = atoi(pArg);

		pArg = GetSepStr(';', &pBuf);
	}
}

void CWorldSection::NewMapTransitionFrom(vec2 Pos, char *pArgs)
{
	char *pMapName = GameServer()->GetMapString(&pArgs, "MapTransition 'From' name", Pos, Map());
	CMap *pMap = Server()->m_MapLoader.GetMap(pMapName);

	if(!pMap)
	{
		Server()->m_MapLoader.AddMap(pMapName);
		pMap = Server()->m_MapLoader.GetMap(pMapName);

		if(!pMap)
		{
			dbg_msg("MapTransition", "Map %s not found.", pMapName);
		}
	}
	int ID = GameServer()->GetMapInteger(&pArgs, "CMapTransition 'From' ID", Pos, Map());

	bool HammerNeeded = false;
	if(GameServer()->GetMapInteger(&pArgs, "CMapTransition 'From' HammerNeeded", Pos, Map()) > 0)
		HammerNeeded = true;

	char *pExtra = GameServer()->GetMapString(&pArgs, "MapTransition 'From' Extra", Pos, Map());
	int TicketLevel = 1;
	HandleTransitionFromExtra(pExtra, TicketLevel);

	m_pMapTransitions.add( new CMapTransitionFrom(ID, Pos, pMap, HammerNeeded, TicketLevel) );
}

void CWorldSection::NewHelper(vec2 Pos, char *pArgs)
{
	int ID = GameServer()->GetMapInteger(&pArgs, "Helper", Pos, Map());
	CNpc *pNpc = new CHelper(GameServer(), Map(), this, Pos, ID);
	pNpc->Spawn();
}

void CWorldSection::NewNpcSpawner(vec2 Pos, char *pArgs)
{
	char *pType = GameServer()->GetMapString(&pArgs, "Npc Spawner", Pos, Map());
	int MaxNum = GameServer()->GetMapInteger(&pArgs, "Npc Spawner", Pos, Map());
	int RespawnTime = GameServer()->GetMapInteger(&pArgs, "Npc Spawner", Pos, Map());
	m_pSpawners.add( new CSpawner(GameServer(), Map(), Pos, pType, MaxNum, RespawnTime) );
}

void CWorldSection::NewPuzzle(vec2 Pos, char *pArgs)
{
	char *pType = GameServer()->GetMapString(&pArgs, "Puzzle", Pos, Map());
	int ID = GameServer()->GetMapInteger(&pArgs, "Puzzle", Pos, Map());
	OnInitPuzzle(Pos, pType, ID);
}

void CWorldSection::NewNpcTicketSeller(vec2 Pos, char *pArgs)
{
	int Ticket = GameServer()->GetMapInteger(&pArgs, "Ticket-Seller", Pos, Map());
	CNpc *pNpc = new CTicketSeller(GameServer(), Map(), this, Pos, Ticket);
	pNpc->Spawn();
}

void CWorldSection::NewBossSpawner(vec2 Pos, char *pArgs)
{
	char *pType = GameServer()->GetMapString(&pArgs, "Boss Spawner", Pos, Map());
	int DoorID = GameServer()->GetMapInteger(&pArgs, "Boss Spawner", Pos, Map());
	m_pSpawners.add( new CBossSpawner(GameServer(), Map(), Pos, pType, DoorID) );
}

void CWorldSection::NewChest(vec2 Pos, char *pArgs)
{
	int ChestID = GameServer()->GetMapInteger(&pArgs, "Chest", Pos, Map());
	int RealChestID = -(ChestID + 1);
	if (ChestID >= 0 || RealChestID < 0 || RealChestID >= MAX_CHESTS_PER_MAP)
		return;

	int ContentID = GameServer()->GetMapInteger(&pArgs, "Chest", Pos, Map());
	m_aChests[RealChestID].SetContentID(ContentID);
}

void CWorldSection::OnInitPuzzle(vec2 Pos, char *pType, int ID)
{
	if(str_comp_nocase(pType, "Pyramide") == 0)
	{
		if(ID <= 3 && ID >= 0)
		{
			CNpc *pNpc = new CPuzzleTee(GameServer(), Map(), this, Pos, 0, ID);
			pNpc->Spawn();
		}
	}
	else
	{
		dbg_msg("Puzzle", "Type %s not found. ID=%I", pType, ID);
	}
}

void CWorldSection::DoPuzzleCheck()
{
	int CheckingType = 0;

	while(true)
	{
		bool FoundPuzzle = false;
		int PuzzleState = 1;
		for(int i = 0; i < m_pNpcs.size(); i++)
		{
			if(m_pNpcs[i]->GetType() != NPC_PUZZLETEE)
				continue;

			CPuzzleTee *pPuzzleTee = (CPuzzleTee *)m_pNpcs[i];
			if(pPuzzleTee->GetPuzzleType() != CheckingType)
				continue;

			if(pPuzzleTee->IsRight() == -1)
			{
				PuzzleState = -1;
				break;
			}
			else if(pPuzzleTee->IsRight() == 0)
				PuzzleState = 0;


			FoundPuzzle = true;
		}

		OnPuzzleState(CheckingType, PuzzleState);

		if(FoundPuzzle == false)
			break;//all puzzles checked

		CheckingType++;
	}
}

void CWorldSection::OnPuzzleState(int PuzzleType, int State)
{
	if(PuzzleType == 0)
	{
		if(State == 0 || State == 1)
		{
			for(int i = 0; i < m_pNpcs.size(); i++)
			{
				if(m_pNpcs[i]->GetType() != NPC_PUZZLETEE)
					continue;

				CPuzzleTee *pPuzzleTee = (CPuzzleTee *)m_pNpcs[i];
				if(pPuzzleTee->GetPuzzleType() != 0)
					continue;

				pPuzzleTee->Reset();
			}

			if(State == 1)
			{
				OnRemote(CRemote::REMOTETYPE_DOOR, 8);
			}
			else
			{
				for(int i = 0; i < m_pNpcs.size(); i++)
				{
					if(m_pNpcs[i]->GetType() != NPC_PUZZLETEE)
						continue;

					CPuzzleTee *pPuzzleTee = (CPuzzleTee *)m_pNpcs[i];
					if(pPuzzleTee->GetPuzzleType() != 0)
						continue;

					CNpc *pNpc = new CMummy(GameServer(), Map(), this, pPuzzleTee->GetPos()-vec2(0, 8));
					pNpc->SetTemporary(true);
					pNpc->Spawn();
				}
			}
		}
	}
}

bool CWorldSection::GetRandomSpawnPosition(vec2 Near, vec2 *pOut)
{
	int Direction = rand()%2? 1 : -1;
	
	for(int w = 0; w < 2; w++)
	{
		for(int i = 0; i < 16; i++)
		{
			int Distance = 900+i*32;
			vec2 MidPos = Near+vec2(Distance*Direction, 0);
			bool Found = false;
			if(Map()->Collision()->TestBox(MidPos, vec2(28.0f, 28.0f)))
			{
				for(int h = 0; h < 8; h++)
				{
					MidPos = Near+vec2(Distance*Direction, h*32);
					if(Map()->Collision()->TestBox(MidPos, vec2(28.0f, 28.0f)) == false)
					{
						Found = true;
						break;
					}
					MidPos = Near+vec2(Distance*Direction, h*-32);
					if(Map()->Collision()->TestBox(MidPos, vec2(28.0f, 28.0f)) == false)
					{
						Found = true;
						break;
					}

				};
			}
			else
				Found = true;


			if(Found)
			{//TODO: Fix bug why still sometimes the pos is in a wall
				//dbg_msg("Wallcheck", "%i %i", Map()->Collision()->TestBox(MidPos, vec2(28.0f, 28.0f)), Map()->Collision()->GetCollisionAt(MidPos));
				*pOut = MidPos;
				return true;
			}
		}
		Direction *= -1;
	}


	return false;
}

void CWorldSection::DoRandomMobSpawn()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(Server()->ClientIngame(i) == false)
			continue;

		CCharacter *pChr = GameServer()->GetPlayerChar(i);
		if(!pChr || pChr->Map() != Map() || pChr->HasHealth() == false)
			continue;

		if(m_aRandomMobSpawnTime[i] < Server()->Tick())
		{
			int NextSpawn = SpawnRandomMob(pChr->m_Pos, i);
			m_aRandomMobSpawnTime[i] = Server()->Tick()+Server()->TickSpeed()*NextSpawn;
		}
	}
}

CEnemyNpc *CWorldSection::DoSpawn(int MapType, int MapBiome, int TicketLevel, bool DayTime, vec2 SpawnPos)
{
	if(MapType == MAPTYPE_WILDNESS)
	{
		if(TicketLevel == 0)
		{
			if(DayTime == false)
			{
				if(rand()%4 == 0)
					return new CSkeletonSmall(GameServer(), Map(), this, SpawnPos);
				else
					return new CZombieSmall(GameServer(), Map(), this, SpawnPos);
			}
			else
			{
				return new CWildteeSmall(GameServer(), Map(), this, SpawnPos);
			}
		}
		else
		{
			if(MapBiome == MAPBIOME_PLAIRIE)
			{
				if(DayTime == false)
				{
					if(rand()%4 == 0)
						return new CSkeleton(GameServer(), Map(), this, SpawnPos);
					else
						return new CZombie(GameServer(), Map(), this, SpawnPos);
				}
				else
				{
					return new CWildtee(GameServer(), Map(), this, SpawnPos);
				}
			}
			else if(MapBiome == MAPBIOME_DESERT)
			{
				//return new CShadow(GameServer(), Map(), this, SpawnPos);
				if(DayTime == false)
				{
					if(rand()%3 == 0)
					{
						if(rand()%3 == 0)
							return new CShadow(GameServer(), Map(), this, SpawnPos);
						else
							return new CSkeleton(GameServer(), Map(), this, SpawnPos);
					}
					else
						return new CZombie(GameServer(), Map(), this, SpawnPos);
				}
				else
				{
					return new CNomad(GameServer(), Map(), this, SpawnPos);
				}
			}
			else
			{
				if (DayTime == false)
				{
					if (rand() % 4 == 0)
						return new CSkeleton(GameServer(), Map(), this, SpawnPos);
					else
						return new CZombie(GameServer(), Map(), this, SpawnPos);
				}
				else
				{
					return new CWildtee(GameServer(), Map(), this, SpawnPos);
				}
			}
		}
	}

	return NULL;
}

int CWorldSection::SpawnRandomMob(vec2 Pos, int ClientID)
{
	bool DayTime = GameServer()->TimeBetween(5, 40, 18, 30);
	vec2 SpawnPos;

	if(GetTempPlayerSpawnNum(ClientID) < MAX_RANDOM_MOBS_PER_PLAYER && GetTemporaryNum() < MAX_RANDOM_MOBS)
	{
		if(GetRandomSpawnPosition(Pos, &SpawnPos))
		{
			CEnemyNpc *pNpc = DoSpawn(Map()->GetMapType(), Map()->GetMapBiome(), Map()->GetMapTicketLevel(), DayTime, SpawnPos);

			if(pNpc)
			{
				pNpc->Spawn();
				int Dir = SpawnPos.x>Pos.x? -1:1;
				pNpc->SetMoveDirection(Dir, 3.0f);
				pNpc->SetTemporary(true);
				pNpc->SetOwner(ClientID);
			}
		}
	}


	int Time = 0;
	if(DayTime)
		Time = rand()%38+19;
	else
		Time = rand()%27+3;

	return Time;
}

void CWorldSection::OnClientEnterMap(int ClientID)
{
	bool DayTime = GameServer()->TimeBetween(5, 40, 18, 30);
	int Time = 0;
	if(DayTime)
		Time = rand()%38+19+22;
	else
		Time = rand()%27+3+12;

	Time = 10;

	m_aRandomMobSpawnTime[ClientID] = Server()->Tick()+Server()->TickSpeed()*Time;
}

void CWorldSection::AddRemote(CRemote *pRemote)
{
	m_pRemotes.add(pRemote);
}

void CWorldSection::OnRemote(int Type, int ID)
{
	for(int i = 0; i < m_pRemotes.size(); i++)
	{
		if(m_pRemotes[i]->GetType() != Type || m_pRemotes[i]->GetID() != ID)
			continue;

		m_pRemotes[i]->Activate();
	}
}