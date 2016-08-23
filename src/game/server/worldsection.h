#pragma once

#include <base/tl/array.h>
#include <base/vmath.h>
#include <engine/server/maploader.h>
#include <game/chest.h>
#include <game/server/npc.h>
#include <game/server/spawner.h>
#include <game/server/remote.h>

#define MAX_CHESTS_PER_MAP 100

 class IServer;
 class CGameContext;

struct CMapTransition
{
	vec2 m_Pos;
	int m_ID;
	bool m_From;
	CMapTransition(int ID, vec2 Pos)
	{
		m_Pos = Pos;
		m_From = false;
		m_ID = ID;
	}
};


struct CMapTransitionFrom : CMapTransition
{
	CMap *m_pMap;
	int m_TicketLevel;
	bool m_HammerNeeded;
	CMapTransitionFrom(int ID, vec2 Pos, CMap *pMap, bool HammerNeeded, int TicketLevel) : CMapTransition(ID, Pos)
	{
		m_From = true;
		m_pMap = pMap;
		m_HammerNeeded = HammerNeeded;
		m_TicketLevel = TicketLevel;
	}

	CMap *Map() { return m_pMap; }
};

class CWorldSection : CNpcHolder
{
private:
	/*array<CNpcSpawner *> m_pNpcSpawners;
	array<CNpc *> m_pNpcs;*/
	CMap *m_pMap;
	IServer *m_pServer;
	CGameContext *m_pGameServer;
	int64 m_aRandomMobSpawnTime[MAX_CLIENTS];
	int64 m_LastActive;

	void NewMapTransitionTo(vec2 Pos, char *pArgs);
	void HandleTransitionFromExtra(char *pExtra, int& TicketLevel);
	void NewMapTransitionFrom(vec2 Pos, char *pArgs);
	void NewHelper(vec2 Pos, char *pArgs);
	void NewNpcSpawner(vec2 Pos, char *pArgs);
	void NewPuzzle(vec2 Pos, char *pArgs);
	void NewNpcTicketSeller(vec2 Pos, char *pArgs);
	void NewBossSpawner(vec2 Pos, char *pArgs);
	void NewChest(vec2 Pos, char *pArgs);

	void OnInitPuzzle(vec2 Pos, char *pType, int ID);
	void DoPuzzleCheck();
	void OnPuzzleState(int PuzzleType, int State);

	bool GetRandomSpawnPosition(vec2 Near, vec2 *pOut);
	void DoRandomMobSpawn();
	CEnemyNpc *DoSpawn(int MapType, int MapBiome, int TicketLevel, bool DayTime, vec2 SpawnPos);
	int SpawnRandomMob(vec2 Pos,int ClientID);
	int GetTempMobNum();
	int GetPlayerSpawnNum(int ClientID);
	void KillAllMobs();

public:
	CWorldSection(CMap *pMap, void *pGameServerInfo);

	array<CMapTransition *> m_pMapTransitions;
	array<CTargetAble *> m_lpTargetAbles;
	array<CPlayerItem *> m_lpPlayerItems;
	array<CSpawner *> m_pSpawners;
	array<CRemote *> m_pRemotes;
	CChest m_aChests[MAX_CHESTS_PER_MAP];

	void Tick();
	void InitTile(int Index, vec2 Pos);
	void InitExTile(int Index, vec2 Pos, char *pArgs);
	//void SnapMapItems(int SnappingClient);

	void OnActive();
	void OnNotActive();
	bool CanBeDeactivated();

	void OnClientEnterMap(int ClientID);

	void AddRemote(CRemote *pRemote);
	void OnRemote(int Type, int ID);

	virtual bool Camp() { return false; }
	virtual vec2 CampPos() { return vec2(0, 0); }

	CMap *Map() { return m_pMap; }
	IServer *Server() { return m_pServer; }
	CGameContext *GameServer() { return m_pGameServer; }
};