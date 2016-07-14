#pragma once

#include <base/tl/array.h>
#include <base/math.h>
#include <base/vmath.h>

#include "modules.h"

class IStorage;

class CGenerator
{
private:
	IStorage *m_pStorage;
	array<CGenEnvelope*> m_lEnvelopeTemplates;
	array<CDoodad*> m_lDoodads;
	array<CMinimalisticShape> m_lMinimalisticShapes;

	int Save(class IStorage *pStorage, CGeneratingMap *pMap);// const char *pFileName);
	bool LoadPNG(CGenImageInfo *pImg, char *pFilename, int StorageType);
	void EnvelopeTemplates();
	void Doodads();
	void MinimalisticShapes();

	void GenerateCollisionMap(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateBaseLine(CGeneratingMap& GenMap, CGenLayerTiles *pLayer);
	void GenerateMidLine(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer);
	void GenerateTerrain(CGeneratingMap& GenMap, CGenLayerTiles *pLayer, int EruptionLevel, int From, int To);
	void GeneratePlain(CGeneratingMap& GenMap, CGenLayerTiles *pLayer, int From, int To);
	int GenerateSpecialChunk(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining);
	int GenerateSpecialChunkRare(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining);
	int GenerateSpecialChunkMedium(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining);
	int GenerateSpecialChunkNormal(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining);
	int GenerateWoodMid(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining);
	int GenerateChestSmall(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining);
	int GenerateChestMedium(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining);
	int GenerateRiver(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining);
	void GenerateCollisionMapWaterland(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateMidLineWaterland(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer);
	void GenerateWater(CGeneratingMap& GenMap, CGenLayerTiles *pLayer);
	void GenerateFlyingBlocks(CGeneratingMap& GenMap, CGenLayerTiles *pLayer);
	void GenerateTerrainWaterland(CGeneratingMap& GenMap, CGenLayerTiles *pLayer, int EruptionLevel, int From, int To);

public:
	CGenerator();

	void SetStorage(IStorage *pStorage);
	bool Generate(char *pName, CPrevMapInfo PrevMapInfo);
	void SetMapInfo(CGeneratingMap& GenMap);
	void GenerateMap(CGeneratingMap& GenMap);

	void GeneratePlairie(CGeneratingMap& GenMap);
	void GeneratePolar(CGeneratingMap& GenMap);
	void GenerateDesert(CGeneratingMap& GenMap);
	void GenerateForest(CGeneratingMap& GenMap);
	void GenerateGrassland(CGeneratingMap& GenMap);
	void GenerateTundra(CGeneratingMap& GenMap);
	void GenerateWaterland(CGeneratingMap& GenMap);

	void GeneratePara0(CGeneratingMap& GenMap);
	void GenerateBackgroundColor(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateBackgroundAquarell(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateBackgroundSunColor(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);

	void GeneratePara10(CGeneratingMap& GenMap);
	void GenerateStars(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateRandomStar(CGeneratingMap& GenMap, CGenLayerQuads *pLayer, vec2 Pos, int MinSize, int MaxSize);
	void GenerateStar(CGeneratingMap& GenMap, CGenLayerQuads *pLayer, vec2 Pos, int Size, int Type, float Rotation);

	void GeneratePlanets(CGeneratingMap& GenMap);
	void GenerateMoon(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateSunshine(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateSun(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);

	void GenerateClouds(CGeneratingMap& GenMap);
	void GenerateRandomCloud(CGeneratingMap& GenMap, CGenLayerQuads *pCloudLayer, vec2 Size, int PosX, int MaxHeight);
	void GenerateCloud(CGeneratingMap& GenMap, CGenLayerQuads *pCloudLayer, vec2 Size, vec2 Pos);

	void GeneratePara25(CGeneratingMap& GenMap);
	void GeneratePara30(CGeneratingMap& GenMap);
	void GeneratePara25Polar(CGeneratingMap& GenMap);
	void GeneratePara30Polar(CGeneratingMap& GenMap);
	void GeneratePara45Polar(CGeneratingMap& GenMap);
	void GeneratePara25Desert(CGeneratingMap& GenMap);
	void GeneratePara30Desert(CGeneratingMap& GenMap);
	void GeneratePara25Errupted(CGeneratingMap& GenMap);
	void GeneratePara30Errupted(CGeneratingMap& GenMap);
	void GeneratePara45Errupted(CGeneratingMap& GenMap);
	void GeneratePara70Forest(CGeneratingMap& GenMap);
	void GeneratePara25Tundra(CGeneratingMap& GenMap);
	void GeneratePara30Tundra(CGeneratingMap& GenMap);
	void GenerateMountains(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, vec3 Color, int Image, int TexX, int ShiftY);
	void GenerateSequoias(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, vec3 Color, int Image, int TexX, int ShiftY);

	void GeneratePreGroup(CGeneratingMap& GenMap);
	void GeneratePreGroupGrassland(CGeneratingMap& GenMap);
	void GenerateWoodBack(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateDoodadsBack(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);

	void GenerateGameGroup(CGeneratingMap& GenMap);
	void GenerateGameLayer(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateExGameLayer(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);

	void GenerateMidGroup(CGeneratingMap& GenMap);
	void GenerateWaterTiles(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateWaterQuads(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateMidGroupPolar(CGeneratingMap& GenMap);
	void GenerateIceLayer(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateMidGroupForest(CGeneratingMap& GenMap);
	void GenerateWaterGreenTiles(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateWaterGreenQuads(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);

	void GenerateMainGroup(CGeneratingMap& GenMap);
	void GenerateMainLayer(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateMainTransitions(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);

	void GeneratePostGroup(CGeneratingMap& GenMap);
	void GeneratePostGroupGrassland(CGeneratingMap& GenMap);
	void GenerateDoodadsMinimalistic(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	void GenerateDoodadsFront(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);

	void GraphicalMap(CGeneratingMap& GenMap);
	void AutomapMainLayer(CGeneratingMap& GenMap);
	void GenerateWoodLeft(CGeneratingMap& GenMap);
	void GenerateWoodRight(CGeneratingMap& GenMap);
	void GenerateVegetation(CGeneratingMap& GenMap, int Variation);
	void GenerateVegetation(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pTileLayer, int Variation);
	int GenerateVegetation(CGenLayerTiles *pTileLayer, CDoodad *pDoodad, int PosX, int Space, int Height);
	void GenerateVegetationMinimalistic(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pTileLayer);
	void GenerateVegetationMinimalistic(CGenLayerTiles *pTileLayer, CMinimalisticShape *pShape, CGenLayerGroup *pGroup, CGenLayerGroup *pGameGroup, int PosX, int Space, int Height);
	void GenerateRiverWater(CGeneratingMap& GenMap);
	void GenerateRiverWaterWaterland(CGeneratingMap& GenMap);
	void GenerateRiverFrozen(CGeneratingMap& GenMap);
	void GenerateGameLayerStuff(CGeneratingMap& GenMap);
	void GenerateLightSources(CGeneratingMap& GenMap);
	void SetMainTransitions(CGeneratingMap& GenMap);

	void GenerateForestVegetation(CGeneratingMap& GenMap);

	void GenerateSnow(CGeneratingMap& GenMap, int Para, int Skip, int Size);
	void GenerateSnowFlakes(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, int Skip, int Size);
	void GenerateSnowFlake(CGeneratingMap& GenMap, CGenLayerQuads *pQuadLayer, vec2 Pos, int Size);

	void GenerateForeground(CGeneratingMap& GenMap);
	void GenerateDarkening(CGeneratingMap& GenMap, CGenLayerGroup *pGroup);
	int UseImage(CGeneratingMap& GenMap, char *pName, bool External, bool Automapper);
	int UseEnvelope(CGeneratingMap& GenMap, char *pName);
	void AddEnvelopeTemplate(CGenEnvelope *pEnvelope, char *pName);
	void AddDoodad(CDoodad *pDoodads, char *pName, int Variation);
	void AddMinimalisticShape(CMinimalisticShape Shape, char *pName);
	void CleanGeneratedMap(CGeneratingMap &GenMap);
	int GetLayerHeight(CGenLayerTiles *pLayer, int PosX);
	int GetLayerSpace(CGenLayerTiles *pLayer, int PosX, int Height);
	CGenLayerGroup *GetLayerGroup(CGeneratingMap& GenMap, char *pName);
	CGenLayerTiles *UseTileLayer(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, char *pName, char *pImage = "", bool External = false, char *pEnv = "", CColor Color = CColor(255, 255, 255, 255));
	CGenLayerQuads *UseQuadLayer(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, char *pName, char *pImage, bool External = false);
	CGenLayerTiles *GetTileLayer(CGenLayerGroup *pGroup, char *pName);
	CGenLayerQuads *GetQuadLayer(CGenLayerGroup *pGroup, char *pName);
	CDoodad *GetDoodad(CGeneratingMap& GenMap, CGenLayerTiles *pTileLayer, int Variation);
	CMinimalisticShape *GetMinimalisticShape(CGeneratingMap& GenMap, CGenLayerTiles *pTileLayer);

	IStorage *Storage() const { return m_pStorage; }
};