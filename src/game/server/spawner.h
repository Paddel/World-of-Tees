#pragma once

#include <game/server/npc.h>

 class IServer;
 class CGameContext;
 class CMap;

class CSpawner : public CNpcHolder
{
private:
	CMap *m_pMap;
	IServer *m_pServer;
	CGameContext *m_pGameServer;
	vec2 m_Pos;
	char m_aType[64];
	int m_MaxNum;
	int m_RespawnTime;
	bool m_Active;
	int64 m_RespawnTimer;

	CNpc *SpawnNpc(char *pName);

public:
	CSpawner(CGameContext *pGameServer, CMap *pMap, vec2 Pos, char *pType, int MaxNum, int RespawnTime);

	virtual void Tick();
	void OnActive(int64 LastActive);

	bool Active() const { return m_Active; }

	virtual bool Camp() { return true; }
	virtual vec2 CampPos() { return m_Pos; }

	CMap *Map() { return m_pMap; }
	IServer *Server() { return m_pServer; }
	CGameContext *GameServer() { return m_pGameServer; }
};

class CBossSpawner : public CSpawner
{
private:
	int m_DoorID;

public:
	CBossSpawner(CGameContext *pGameServer, CMap *pMap, vec2 Pos, char *pType, int DoorID);

	virtual void Tick();
};