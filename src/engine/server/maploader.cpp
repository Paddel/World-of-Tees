
#include <engine/map.h>
#include <engine/server.h>
#include <engine/storage.h>

#include "maploader.h"

CMap::CMap(char *pName, CMapLoader *pMapLoader, void *pGameServerInfo)
{
	str_copy(m_aName, pName, sizeof(m_aName));
	m_pServer = pMapLoader->Server();

	m_pWorldSection = new CWorldSection(this, pGameServerInfo);

	m_MapSize = -1;
	m_pMapData = 0;
	m_MapCrc = 0;
	m_Err = false;

	if(!LoadMapFile())
	{
		m_Err = true;
		return;
	}


}

int CMap::LoadMapFile()
{
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "maps/%s.map", m_aName);

	if (!m_EngineMap.Load(aBuf, Server()->Storage()))
	{
		str_format(aBuf, sizeof(aBuf), "maps/wot/%s.map", m_aName);
		if (!m_EngineMap.Load(aBuf, Server()->Storage()))
		{
			str_format(aBuf, sizeof(aBuf), "maps/generated/%s.map", m_aName);
			if (!m_EngineMap.Load(aBuf, Server()->Storage()))
				return 0;
		}
	}


	// get the crc of the map
	m_MapCrc = m_EngineMap.Crc();

	// load complete map into memory for download
	{
		IOHANDLE File = Server()->Storage()->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_ALL);
		m_MapSize = (int)io_length(File);
		if(m_pMapData)
			mem_free(m_pMapData);

		m_pMapData = (unsigned char *)mem_alloc(m_MapSize, 1);
		io_read(File, m_pMapData, m_MapSize);
		io_close(File);
	}
	return 1;
}

void CMap::Deinit()
{
	GetEngineMap()->Unload();

	if(m_pMapData)
		mem_free(m_pMapData);
}

void CMap::Init()
{
	m_Layers.Init(&m_EngineMap);
	m_Collision.Init(&m_Layers);
	LoadMapType();

	Server()->InitMapTiles(this);
}

void CMap::LoadMapType()
{
	m_MapType = MAPTYPE_WILDNESS;
	m_Temperature = 0;
	m_Moisture = 100;
	m_Eruption = 8;

	CMapItemInfoEx *pItem = (CMapItemInfoEx *)m_EngineMap.FindItem(MAPITEMTYPE_INFO, 0);
	if(pItem && pItem->m_Version == 2)
	{
		m_MapType = pItem->m_MapType;
		m_Temperature = pItem->m_Temperature;
		m_Moisture = pItem->m_Moisture;
		m_MapBiome = CalculateBiome(pItem->m_Temperature, pItem->m_Moisture);
		m_Eruption = pItem->m_Eruption;
	}
}

void CMap::OnClientEnterMap(int ClientID)
{
	WorldSection()->OnClientEnterMap(ClientID);
}

void CMap::Tick()
{
	if(m_LastActive != Active())
	{
		if(Active())
			WorldSection()->OnActive();
		else
			WorldSection()->OnNotActive();
	}

	if(Active())
	{
		WorldSection()->Tick();
	}

	m_LastActive = Active();
}

CMapLoader::CMapLoader()
{
	m_lpMaps.clear();
	m_pDefaultMap = 0;
	m_Inited = false;
}

void CMapLoader::Init(IServer *pServer)
{
	m_pServer = pServer;
	m_Inited = true;
	m_Generator.SetStorage(pServer->Storage());
}

void CMapLoader::Deinit()
{
	for(int i = 0; i < m_lpMaps.size(); i++)
	{
		m_lpMaps[i]->GetEngineMap()->Unload();

		if(m_lpMaps[i]->m_pMapData)
			mem_free(m_lpMaps[i]->m_pMapData);
	}
	m_lpMaps.clear();

	m_pDefaultMap = 0;

	m_Inited = false;
}

void CMapLoader::Tick()
{//maps with players on and where spawners didnt finish spawning are active and have to be ticked
	for(int i = 0; i < m_lpMaps.size(); i++)
	{
		if(m_lpMaps[i]->WorldSection()->CanBeDeactivated() == false)
			m_lpMaps[i]->m_Active = true;
		else
			m_lpMaps[i]->m_Active = false;
	}

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CMap *pMap = Server()->CurrentMap(i);
		if(!pMap)
			continue;

		pMap->m_Active = true;
	}

	for(int i = 0; i < m_lpMaps.size(); i++)
	{
		m_lpMaps[i]->Tick();
	}
}

CMap *CMapLoader::GetMap(char *pName)
{
	for(int i = 0; i < m_lpMaps.size(); i++)
	{
		if(str_comp(m_lpMaps[i]->GetName(), pName) == 0)
			return m_lpMaps[i];
	}

	return NULL;
}

CMap *CMapLoader::GetMap(int Index)
{
	if(Index < 0 && Index >= m_lpMaps.size())
		return NULL;

	return m_lpMaps[Index];
}

bool CMapLoader::AddDefaultMap(char *pName)
{
	dbg_assert(m_Inited, "MapLoader not inited.");

	if(m_pDefaultMap)
		return false;

	m_pDefaultMap = new CMap(pName, this, m_pGameServerInfo);

	if(m_pDefaultMap->GetErr())
		return false;

	m_lpMaps.add(m_pDefaultMap);

	dbg_msg("Maploader", "Map '%s' added as default map.", pName);

	m_pDefaultMap->Init();

	return true;
}

bool CMapLoader::AddMap(char *pName)
{
	dbg_assert(m_Inited, "MapLoader not inited.");

	for(int i = 0; i < m_lpMaps.size(); i++)
	{
		if(str_comp(m_lpMaps[i]->GetName(), pName) == 0)
			return false;//map already in list. dont add a map twice
	}

	CMap *pMap = new CMap(pName, this, m_pGameServerInfo);

	if(pMap->GetErr())//error while loading the map
		return false;

	m_lpMaps.add(pMap);

	dbg_msg("Maploader", "Map '%s' added.", pName);

	pMap->Init();

	return true;
}

bool CMapLoader::ReloadMap(char *pName)
{
	dbg_assert(m_Inited, "MapLoader not inited.");

	for(int i = 0; i < m_lpMaps.size(); i++)
	{
		if(str_comp(m_lpMaps[i]->GetName(), pName) == 0)
		{
			bool Default = m_lpMaps[i] == m_pDefaultMap;
			m_lpMaps[i]->Deinit();
			m_lpMaps.remove_index(i);
			
			if(Default)
			{
				m_pDefaultMap = 0;
				return AddDefaultMap(pName);
			}
			else
				return AddMap(pName);
		}
	}

	return false;
}

CMap *CMapLoader::Generate(CMap *pPrevMap, char *pName, int TransitionID)
{
	dbg_assert(m_Inited, "MapLoader not inited.");

	CPrevMapInfo MapInfo = CPrevMapInfo(pPrevMap->GetName(), TransitionID, pPrevMap->GetMapEruption(), pPrevMap->GetMapTemperature(), pPrevMap->GetMapMoisture());
	if (m_Generator.Generate(pName, MapInfo) == false)
		return NULL;

	int LastSize = m_lpMaps.size();

	if (AddMap(pName) == false)
		return NULL;

	if (LastSize != m_lpMaps.size() - 1)
		return NULL;

	CMap *pLastMap = m_lpMaps[LastSize];
	return pLastMap;
}