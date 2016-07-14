#pragma once

#include <base/tl/array.h>
#include <engine/map.h>
#include <engine/generator/generator.h>
#include <game/layers.h>
#include <game/collision.h>
#include <game/server/worldsection.h>

class IServer;
class CMapLoader;

enum
{
	MAX_MAP_NAME=256,
};

class CMap
{
	friend class CMapLoader;

private:
	char m_aName[MAX_MAP_NAME];

	CWorldSection *m_pWorldSection;
	int m_MapType;
	int m_MapBiome;
	int m_Temperature;
	int m_Moisture;
	int m_Eruption;
	int m_TicketLevel;
	bool m_LastActive;

	int LoadMapFile();

protected:
	int m_MapSize;
	unsigned char *m_pMapData;
	unsigned m_MapCrc;
	CMapLoader *m_pMapLoader;
	IServer *m_pServer;
	CEngineMap m_EngineMap;

	CLayers m_Layers;
	CCollision m_Collision;

	bool m_Err;
	bool m_Active;

	void Deinit();
	void Init();

	void LoadMapType();

public:
	CMap(char *pName, CMapLoader *pMapLoader, void *pGameServerInfo);

	void OnClientEnterMap(int ClientID);
	void Tick();

	CEngineMap *GetEngineMap() { return &m_EngineMap; }
	char *GetName() { return m_aName; }
	bool GetErr() const { return m_Err; }
	int GetMapSize() const { return m_MapSize; }
	unsigned char *GetMapData() const { return m_pMapData; }
	unsigned GetMapCrc() const { return m_MapCrc; }
	bool Active() const { return m_Active; }
	int GetMapType() const { return m_MapType; }
	int GetMapTemperature() const { return m_Temperature; }
	int GetMapMoisture() const { return m_Moisture; }
	int GetMapBiome() const { return m_MapBiome; }
	int GetMapEruption() const { return m_Eruption; }
	int GetMapTicketLevel() const { return m_TicketLevel; }

	CMapLoader *MapLoader() const { return m_pMapLoader; }
	IServer *Server() const { return m_pServer; }

	CLayers *Layers() { return &m_Layers; }
	CCollision *Collision() { return &m_Collision; }
	CWorldSection *WorldSection() const { return m_pWorldSection; }
};

class CMapLoader
{
private:
	array<CMap *> m_lpMaps;
	void *m_pGameServerInfo;
	CMap *m_pDefaultMap;
	IServer *m_pServer;
	CGenerator m_Generator;
	bool m_Inited;

public:
	CMapLoader();

	void Init(IServer *pServer);
	void Deinit();
	void Tick();
	bool AddDefaultMap(char *pName);
	bool AddMap(char *pName);
	bool ReloadMap(char *pName);

	CMap *Generate(CMap *pPrevMap, char *pName, int TransitionID);

	CMap *GetMap(char *pName);
	CMap *GetMap(int Index);
	CMap *GetDefaultMap() const { return m_pDefaultMap; }
	int GetNumMaps() const { return m_lpMaps.size(); }

	void SetGameServer(void *pGameServerInfo) { m_pGameServerInfo = pGameServerInfo; }

	IServer *Server() const { return m_pServer; }
	CGenerator *Generator() { return &m_Generator; }
};