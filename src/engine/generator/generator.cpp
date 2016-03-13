
#include <base/stringseperation.h>
#include <engine/storage.h>
#include <engine/shared/datafile.h>
#include <engine/external/pnglite/pnglite.h>
#include <game/gamecore.h>
#include <game/chest.h>

#include "generator.h"

#define NEXT_MAP_BORDER 60
#define MAP_WIDTH 1500
#define MAP_HEIGHT 250
#define SCREEN_WIDTH 800000
#define SCREEN_HEIGHT 600000
#define CHUNK_SIZE 15

static void Rotate(const CPoint *pCenter, CPoint *pPoint, float Rotation)
{
	int x = pPoint->x - pCenter->x;
	int y = pPoint->y - pCenter->y;
	pPoint->x = (int)(x * cosf(Rotation) - y * sinf(Rotation) + pCenter->x);
	pPoint->y = (int)(x * sinf(Rotation) + y * cosf(Rotation) + pCenter->y);
}

static void RotateQuad(CQuad *pQuad, float Rotation)
{
	for (int v = 0; v < 4; v++)
	{
		Rotate(&pQuad->m_aPoints[4], &pQuad->m_aPoints[v], Rotation);
	}
}

static int GetMapInteger(char **pSrc, char *pCall, vec2 Pos)
{
	char *pInd = GetSepStr(0xff, pSrc);
	if (pInd[0] != 'i')
	{
		dbg_msg(pCall, "Indicator error '%s' at Pos %f, %f", pInd, Pos.x, Pos.y);
		return -1;
	}

	return GetSepInt(0xff, pSrc);
}

static char *GetMapString(char **pSrc, char *pCall, vec2 Pos)
{
	char *pInd = GetSepStr(0xff, pSrc);
	if (pInd[0] != 's')
	{
		dbg_msg(pCall, "Indicator error '%s' at Pos %f, %f on Map %s", pInd, Pos.x, Pos.y);
		return "";
	}
	return GetSepStr(0xff, pSrc);;
}

int CGenerator::Save(class IStorage *pStorage, CGeneratingMap *pMap)
{
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "saving to '%s'...", pMap->m_aFileName);
	dbg_msg("generator", aBuf);
	CDataFileWriter df;
	if (!df.Open(pStorage, pMap->m_aFileName))
	{
		str_format(aBuf, sizeof(aBuf), "failed to open file '%s'...", pMap->m_aFileName);
		dbg_msg("generator", aBuf);
		return 0;
	}

	// save version
	{
		CMapItemVersion Item;
		Item.m_Version = 1;
		df.AddItem(MAPITEMTYPE_VERSION, 0, sizeof(Item), &Item);
	}

	// save map info
	{
		if (true)//m_pEditor->m_UseExMapInfo
		{
			CMapItemInfoEx Item;
			Item.m_Version = 2;

			Item.m_Author = df.AddData(str_length("Paddel") + 1, "Paddel");
			Item.m_MapVersion = -1;
			Item.m_Credits = df.AddData(str_length("-") + 1, "-");
			Item.m_License = df.AddData(str_length("GPLv3") + 1, "GPLv3");

			Item.m_MapType = pMap->m_MapType;
			Item.m_Temperature = pMap->m_Temperature;
			Item.m_Moisture = pMap->m_Moisture;
			Item.m_Eruption = pMap->m_Eruption;

			df.AddItem(MAPITEMTYPE_INFO, 0, sizeof(Item), &Item);
		}
	}

	// save images
	for (int i = 0; i < pMap->m_lImages.size(); i++)
	{
		CGeneratorImage *pImg = pMap->m_lImages[i];

		// analyse the image for when saving (should be done when we load the image)
		// TODO!
		//pImg->AnalyseTileFlags();

		CMapItemImage Item;
		Item.m_Version = 1;

		Item.m_Width = pImg->m_Width;
		Item.m_Height = pImg->m_Height;
		Item.m_External = pImg->m_External;
		Item.m_ImageName = df.AddData(str_length(pImg->m_aName) + 1, pImg->m_aName);
		if (pImg->m_External)
			Item.m_ImageData = -1;
		else
			Item.m_ImageData = df.AddData(Item.m_Width*Item.m_Height * 4, pImg->m_pData);
		df.AddItem(MAPITEMTYPE_IMAGE, i, sizeof(Item), &Item);
	}

	// save layers
	int LayerCount = 0, GroupCount = 0;
	for (int g = 0; g < pMap->m_lGroups.size(); g++)
	{
		CGenLayerGroup *pGroup = pMap->m_lGroups[g];

		CMapItemGroup GItem;
		GItem.m_Version = CMapItemGroup::CURRENT_VERSION;

		GItem.m_ParallaxX = pGroup->m_ParallaxX;
		GItem.m_ParallaxY = pGroup->m_ParallaxY;
		GItem.m_OffsetX = pGroup->m_OffsetX;
		GItem.m_OffsetY = pGroup->m_OffsetY;
		GItem.m_UseClipping = pGroup->m_UseClipping;
		GItem.m_ClipX = pGroup->m_ClipX;
		GItem.m_ClipY = pGroup->m_ClipY;
		GItem.m_ClipW = pGroup->m_ClipW;
		GItem.m_ClipH = pGroup->m_ClipH;
		GItem.m_StartLayer = LayerCount;
		GItem.m_NumLayers = 0;

		// save group name
		StrToInts(GItem.m_aName, sizeof(GItem.m_aName) / sizeof(int), pGroup->m_aName);

		for (int l = 0; l < pGroup->m_lLayers.size(); l++)
		{
			if (!pGroup->m_lLayers[l]->m_SaveToMap)
			{
				dbg_msg(0, "skipped %i", l);
				continue;
			}

			if (pGroup->m_lLayers[l]->m_Type == LAYERTYPE_TILES || pGroup->m_lLayers[l]->m_Type == LAYERTYPE_HIDDEN)
			{
				CGenLayerTiles *pLayer = (CGenLayerTiles *)pGroup->m_lLayers[l];
				pLayer->PrepareForSave();

				CMapItemLayerTilemap Item;
				Item.m_Version = 3;

				Item.m_Layer.m_Flags = pLayer->m_Flags;
				Item.m_Layer.m_Type = pLayer->m_Type;

				Item.m_Color = pLayer->m_Color;
				Item.m_ColorEnv = pLayer->m_ColorEnv;
				Item.m_ColorEnvOffset = pLayer->m_ColorEnvOffset;

				Item.m_Width = pLayer->m_Width;
				Item.m_Height = pLayer->m_Height;
				Item.m_LayerType = pLayer->m_LayerType;

				Item.m_Image = pLayer->m_Image;
				Item.m_Data = df.AddData(pLayer->m_Width*pLayer->m_Height*sizeof(CTile), pLayer->m_pTiles);

				if (pLayer->m_LayerType == LAYERTILETYPE_EX)
				{
					Item.m_ExDataSize = sizeof(CExTile);
					Item.m_ExData = df.AddData(pLayer->m_Width*pLayer->m_Height*Item.m_ExDataSize, ((CGenLayerExTiles*)pLayer)->m_pExTiles);

					Item.m_Layer.m_Type = LAYERTYPE_HIDDEN;
				}

				// save layer name
				StrToInts(Item.m_aName, sizeof(Item.m_aName) / sizeof(int), pLayer->m_aName);

				df.AddItem(MAPITEMTYPE_LAYER, LayerCount, sizeof(Item), &Item);

				GItem.m_NumLayers++;
				LayerCount++;
			}
			else if (pGroup->m_lLayers[l]->m_Type == LAYERTYPE_QUADS)
			{
				CGenLayerQuads *pLayer = (CGenLayerQuads *)pGroup->m_lLayers[l];
				if (pLayer->m_lQuads.size())
				{
					CMapItemLayerQuads Item;
					Item.m_Version = 2;
					Item.m_Layer.m_Flags = pLayer->m_Flags;
					Item.m_Layer.m_Type = pLayer->m_Type;
					Item.m_Image = pLayer->m_Image;

					// add the data
					Item.m_NumQuads = pLayer->m_lQuads.size();
					Item.m_Data = df.AddDataSwapped(pLayer->m_lQuads.size()*sizeof(CQuad), pLayer->m_lQuads.base_ptr());

					// save layer name
					StrToInts(Item.m_aName, sizeof(Item.m_aName) / sizeof(int), pLayer->m_aName);

					df.AddItem(MAPITEMTYPE_LAYER, LayerCount, sizeof(Item), &Item);

					// clean up
					//mem_free(quads);

					GItem.m_NumLayers++;
					LayerCount++;
				}
			}
		}

		df.AddItem(MAPITEMTYPE_GROUP, GroupCount++, sizeof(GItem), &GItem);
	}

	// save envelopes
	int PointCount = 0;
	for (int e = 0; e < pMap->m_lEnvelopes.size(); e++)
	{
		CMapItemEnvelope Item;
		Item.m_Version = CMapItemEnvelope::CURRENT_VERSION;
		Item.m_Channels = pMap->m_lEnvelopes[e]->m_Channels;
		Item.m_StartPoint = PointCount;
		Item.m_NumPoints = pMap->m_lEnvelopes[e]->m_lPoints.size();
		Item.m_Synchronized = pMap->m_lEnvelopes[e]->m_Synchronized;
		StrToInts(Item.m_aName, sizeof(Item.m_aName) / sizeof(int), pMap->m_lEnvelopes[e]->m_aName);

		df.AddItem(MAPITEMTYPE_ENVELOPE, e, sizeof(Item), &Item);
		PointCount += Item.m_NumPoints;
	}

	// save points
	int TotalSize = sizeof(CEnvPoint) * PointCount;
	CEnvPoint *pPoints = (CEnvPoint *)mem_alloc(TotalSize, 1);
	PointCount = 0;

	for (int e = 0; e < pMap->m_lEnvelopes.size(); e++)
	{
		int Count = pMap->m_lEnvelopes[e]->m_lPoints.size();
		mem_copy(&pPoints[PointCount], pMap->m_lEnvelopes[e]->m_lPoints.base_ptr(), sizeof(CEnvPoint)*Count);
		PointCount += Count;
	}

	df.AddItem(MAPITEMTYPE_ENVPOINTS, 0, TotalSize, pPoints);
	mem_free(pPoints);

	// finish the data file
	df.Finish();
	dbg_msg("generator", "saving done");

	return 1;
}

void CGenerator::GenerateCollisionMap(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerTiles *pGameLayer = GetTileLayer(pGroup, "Game");
	if (pGameLayer == NULL)
		return;

	int Width = pGameLayer->m_Width;
	int Height = pGameLayer->m_Height;

	GenerateBaseLine(GenMap, pGameLayer);
	GenerateMidLine(GenMap, pGroup, pGameLayer);
	GeneratePlain(GenMap, pGameLayer, Width - NEXT_MAP_BORDER, Width);
}

void CGenerator::GenerateCollisionMapWaterland(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerTiles *pGameLayer = GetTileLayer(pGroup, "Game");
	if (pGameLayer == NULL)
		return;

	int Width = pGameLayer->m_Width;
	int Height = pGameLayer->m_Height;

	GenerateBaseLine(GenMap, pGameLayer);
	GenerateMidLineWaterland(GenMap, pGroup, pGameLayer);
	GeneratePlain(GenMap, pGameLayer, Width - NEXT_MAP_BORDER, Width);
	GenerateFlyingBlocks(GenMap, pGameLayer);
	GenerateFlyingBlocks(GenMap, pGameLayer);
	GenerateWater(GenMap, pGameLayer);
}

void CGenerator::GenerateBaseLine(CGeneratingMap& GenMap, CGenLayerTiles *pLayer)
{
	int Width = pLayer->m_Width;
	int Height = pLayer->m_Height;

	for (int h = Height / 2; h < Height; h++)
	{
		for (int w = 0; w < Width; w++)
		{
			int Tile = h * Width + w;
			pLayer->m_pTiles[Tile].m_Index = TILE_SOLID;
		}
	}
}

void CGenerator::GenerateFlyingBlocks(CGeneratingMap& GenMap, CGenLayerTiles *pLayer)
{
	int Width = pLayer->m_Width;
	const int MaxBlockWidth = 10;
	for (int x = NEXT_MAP_BORDER + MaxBlockWidth; x < Width - NEXT_MAP_BORDER - MaxBlockWidth; x++)
	{
		int Height = GetLayerHeight(pLayer, x);
		/*if (pLayer->m_pTiles[(Height - 1) * Width + x].m_Index != TILE_WATER)
			continue;*/

		if (rand() % MaxBlockWidth)
		{
			int BlockWidth = (rand() % (MaxBlockWidth - 2)) + 2;
			int BlockDist = rand() % 10 + 1;
			int BlockStartHeight = rand() % 5 + 2;
			
			for (int y = Height - BlockDist - BlockStartHeight; y < Height - BlockDist; y++)
			{
				for (int w = 0; w < BlockWidth; w++)
				{
					int Tile = y * Width + x + w;
					pLayer->m_pTiles[Tile].m_Index = TILE_SOLID;
				}
			}

			x += BlockWidth + 2;
		}
	}
}

void CGenerator::GenerateWater(CGeneratingMap& GenMap, CGenLayerTiles *pLayer)
{
	int Width = pLayer->m_Width;
	int Height = pLayer->m_Height;

	for (int h = Height / 2 + 1; h < Height; h++)
	{
		for (int w = 0; w < Width; w++)
		{
			int Tile = h * Width + w;
			if (pLayer->m_pTiles[Tile].m_Index == TILE_AIR)
				pLayer->m_pTiles[Tile].m_Index = TILE_WATER;
		}
	}
}

void CGenerator::GenerateMidLine(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer)
{
	int Width = pLayer->m_Width;
	int Height = pLayer->m_Height;

	int NumChunks = (Width - NEXT_MAP_BORDER * 2) / CHUNK_SIZE;
	for (int c = 0 ; c < NumChunks; c++)
	{
		int ChunkFrom = c * CHUNK_SIZE + NEXT_MAP_BORDER;
		int ChunkTo = ChunkFrom + CHUNK_SIZE;
		bool LastChunk = false;

		if (c == NumChunks-1)
		{
			LastChunk = true;
			ChunkTo = Width - NEXT_MAP_BORDER;//fill the rest
		}

		if (LastChunk || c == 0)
			GenerateTerrain(GenMap, pLayer, 10, ChunkFrom, ChunkTo);
		else
		{
			if (rand() % (NumChunks / 2) == 0)//NumChunks
			{//special chunk
				GeneratePlain(GenMap, pLayer, ChunkFrom, ChunkTo);
				c += GenerateSpecialChunk(GenMap, pGroup, pLayer, ChunkFrom, ChunkTo, NumChunks - c - 1);
			}
			else
			{
				int RiverChance = 0;
				if (GenMap.m_Moisture >= 70)
					RiverChance = 24 - ((GenMap.m_Moisture / 10) - 7);

				if (RiverChance > 0 && rand() % RiverChance == 0)
				{
					GeneratePlain(GenMap, pLayer, ChunkFrom, ChunkTo);
					c += GenerateRiver(GenMap, pGroup, pLayer, ChunkFrom, ChunkTo, NumChunks - c - 1);
				}
				else
					GenerateTerrain(GenMap, pLayer, GenMap.m_Eruption, ChunkFrom, ChunkTo);
			}
		}
	}
}

void CGenerator::GenerateTerrain(CGeneratingMap& GenMap, CGenLayerTiles *pLayer, int EruptionLevel, int From, int To)
{
	int Width = pLayer->m_Width;
	int Height = pLayer->m_Height;

	if (From < 1 || To > Width)
	{
		dbg_msg("Generator", "Error generating Terrain");
		return;
	}

	for (int w = From; w < To; w++)
	{
		int HighestTile = GetLayerHeight(pLayer, w-1);

		if (rand() % EruptionLevel == 0)
		{
			if (rand() % 2)
			{//higher
				if (HighestTile > 15)
				{
					HighestTile--;
					for (int i = 0; i < 5; i++)
					{
						if (rand() % (i + 2) == 0)
							HighestTile--;
						else break;
					}
				}
			}
			else
			{//lower
				if (pLayer->m_pTiles[HighestTile * Width + w - 2].m_Index == TILE_AIR)
				{
					for (int h = 0; h < Height; h++)
						pLayer->m_pTiles[h * Width + w].m_Index = h < HighestTile ? TILE_AIR : TILE_SOLID;

					w++;
				}

				if (HighestTile < Height - 15)
				{
					HighestTile++;
					for (int i = 0; i < 5; i++)
					{
						if (rand() % (i + 2) == 0)
							HighestTile++;
						else break;
					}
				}
			}
		}

		for (int h = 0; h < Height; h++)
			pLayer->m_pTiles[h * Width + w].m_Index = h < HighestTile ? TILE_AIR : TILE_SOLID;
	}
}

void CGenerator::GenerateMidLineWaterland(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer)
{
	int Width = pLayer->m_Width;
	int Height = pLayer->m_Height;

	int NumChunks = (Width - NEXT_MAP_BORDER * 2) / CHUNK_SIZE;
	for (int c = 0; c < NumChunks; c++)
	{
		int ChunkFrom = c * CHUNK_SIZE + NEXT_MAP_BORDER;
		int ChunkTo = ChunkFrom + CHUNK_SIZE;
		bool LastChunk = false;

		if (c == NumChunks - 1)
		{
			LastChunk = true;
			ChunkTo = Width - NEXT_MAP_BORDER;//fill the rest
		}

		if (LastChunk || c == 0)
			GenerateTerrain(GenMap, pLayer, 10, ChunkFrom, ChunkTo);
		else
		{
			if (rand() % (NumChunks / 2) == 0)//NumChunks
			{//special chunk
				GeneratePlain(GenMap, pLayer, ChunkFrom, ChunkTo);
				c += GenerateSpecialChunk(GenMap, pGroup, pLayer, ChunkFrom, ChunkTo, NumChunks - c - 1);
			}
			else
				GenerateTerrainWaterland(GenMap, pLayer, GenMap.m_Eruption, ChunkFrom, ChunkTo);
		}
	}
}

void CGenerator::GenerateTerrainWaterland(CGeneratingMap& GenMap, CGenLayerTiles *pLayer, int EruptionLevel, int From, int To)
{
	int Width = pLayer->m_Width;
	int Height = pLayer->m_Height;
	int MidHeight = Height / 2;

	if (From < 1 || To > Width)
	{
		dbg_msg("Generator", "Error generating Terrain");
		return;
	}

	for (int w = From; w < To; w++)
	{
		int HighestTile = GetLayerHeight(pLayer, w - 1);

		if (rand() % EruptionLevel == 0)
		{
			if (rand() % 2)
			{//higher
				if (HighestTile > MidHeight - 15)
				{
					HighestTile--;
					for (int i = 0; i < 5; i++)
					{
						if (rand() % (i + 2) == 0)
							HighestTile--;
						else break;
					}
				}
			}
			else
			{//lower
				if (pLayer->m_pTiles[HighestTile * Width + w - 2].m_Index == TILE_AIR)
				{
					for (int h = 0; h < Height; h++)
						pLayer->m_pTiles[h * Width + w].m_Index = h < HighestTile ? TILE_AIR : TILE_SOLID;

					w++;
				}

				if (HighestTile < MidHeight + 15)
				{
					HighestTile++;
					for (int i = 0; i < 5; i++)
					{
						if (rand() % (i + 2) == 0)
							HighestTile++;
						else break;
					}
				}
			}
		}

		for (int h = 0; h < Height; h++)
			pLayer->m_pTiles[h * Width + w].m_Index = h < HighestTile ? TILE_AIR : TILE_SOLID;
	}
}

void CGenerator::GeneratePlain(CGeneratingMap& GenMap, CGenLayerTiles *pLayer, int From, int To)
{
	int Width = pLayer->m_Width;
	int Height = pLayer->m_Height;

	if (From < 1)
	{
		dbg_msg("Generator", "Error generating Endline");
		return;
	}

	for (int w = From; w < To; w++)
	{
		int HighestTile = Height / 2;
		for (int h = 0; h < Height; h++)
		{
			int Tile = h * Width + w - 1;
			if (pLayer->m_pTiles[Tile].m_Index == TILE_SOLID)
			{
				HighestTile = h;
				break;
			}
		}

		for (int h = 0; h < Height; h++)
			pLayer->m_pTiles[h * Width + w].m_Index = h < HighestTile ? TILE_AIR : TILE_SOLID;
	}
}

int CGenerator::GenerateSpecialChunk(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining)
{
	int Percent = rand() % 100;
	int Rarity = 0;
	if (Percent < 80)// 0 - 79 = 80%
		Rarity = 0;
	else if (Percent < 98)//80 - 97 = 18%
		Rarity = 1;
	else if (Percent < 100)//98 - 99 = 2%
		Rarity = 2;

	if (Rarity == 2)
		return GenerateSpecialChunkRare(GenMap, pGroup, pLayer, ChunkFrom, ChunkTo, Chunksremaining);
	else if (Rarity == 1)
		return GenerateSpecialChunkMedium(GenMap, pGroup, pLayer, ChunkFrom, ChunkTo, Chunksremaining);
	else
		return GenerateSpecialChunkNormal(GenMap, pGroup, pLayer, ChunkFrom, ChunkTo, Chunksremaining);
}

int CGenerator::GenerateSpecialChunkRare(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining)
{
	int ChosenChunk = rand() % 1;
	return -1;
}

int CGenerator::GenerateSpecialChunkMedium(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining)
{
	int ChosenChunk = rand() % 1;
	switch (ChosenChunk)
	{
	case 0: return GenerateChestMedium(GenMap, pGroup, pLayer, ChunkFrom, ChunkTo, Chunksremaining);
	default: return -1;
	}
}

int CGenerator::GenerateSpecialChunkNormal(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining)
{
	int ChosenChunk = rand() % 2;
	switch (ChosenChunk)
	{
	case 0: return GenerateWoodMid(GenMap, pGroup, pLayer, ChunkFrom, ChunkTo, Chunksremaining);
	case 1: return GenerateChestSmall(GenMap, pGroup, pLayer, ChunkFrom, ChunkTo, Chunksremaining);
	default: return -1;
	}
}

int CGenerator::GenerateWoodMid(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining)
{
	CGenLayerGroup *pPreGroup = GetLayerGroup(GenMap, "Pre");
	if (pPreGroup == NULL)
		return -1;

	int Height = GetLayerHeight(pLayer, ChunkFrom);
	int Width = pLayer->m_Width;

	CGenLayerExTiles *pExTileLayer = static_cast<CGenLayerExTiles *>(GetTileLayer(pGroup, "Extension"));
	CGenLayerTiles *pBackWoodLayer = GetTileLayer(pPreGroup, "Tree Back");
	if (pBackWoodLayer == NULL || pExTileLayer == NULL)
		return -1;

	for (int x = 0; x < 5; x++)
	{
		for (int y = 0; y < 5; y++)
		{
			int PosX = ChunkFrom + x + 1;
			int PosY = Height + y - 4;
			int Index = 120 + x + y * 16;
			int Tile = PosY * Width + PosX;

			pBackWoodLayer->m_pTiles[Tile].m_Index = Index;
			if (x < 4 && y < 4)
			{
				pExTileLayer->m_pTiles[Tile].m_Index = EXTILE_GENMAP;
				str_format(pExTileLayer->m_pExTiles[Tile].m_ExArgs, MAX_EXTENTED_STR,
					"i%c%i%ci%c%i%ci%c1", 0xff, GenMap.m_GenMapCount, 0xff, 0xff, GenMap.m_TransitionCount, 0xff, 0xff);
			}
		}
	}

	{
		int Tile = (Height - 2) * Width + ChunkFrom;
		pExTileLayer->m_pTiles[Tile].m_Index = EXTILE_MAPTRANSITION_TO;
		str_format(pExTileLayer->m_pExTiles[Tile].m_ExArgs, MAX_EXTENTED_STR, "i%c%i", 0xff, GenMap.m_TransitionCount);
	}

	if (CHUNK_SIZE > 5)
	{
		GenerateTerrain(GenMap, pLayer, GenMap.m_Eruption, ChunkFrom + 5, ChunkTo);
	}

	GenMap.m_GenMapCount++;
	GenMap.m_TransitionCount++;
	return 0;
}

int CGenerator::GenerateChestSmall(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining)
{
	CGenLayerGroup *pPreGroup = GetLayerGroup(GenMap, "Pre");
	if (pPreGroup == NULL)
		return -1;

	int Height = GetLayerHeight(pLayer, ChunkFrom);
	int Width = pLayer->m_Width;

	CGenLayerExTiles *pExTileLayer = static_cast<CGenLayerExTiles *>(GetTileLayer(pGroup, "Extension"));
	CGenLayerTiles *pChestLayer = UseTileLayer(GenMap, pPreGroup, "Chests", "pdl_chests", false);
	if (pChestLayer == NULL || pExTileLayer == NULL)
		return -1;

	for (int x = 0; x < 3; x++)
	{
		for (int y = 0; y < 2; y++)
		{
			int PosX = ChunkFrom + x;
			int PosY = Height + y - 2;
			int Index = 17 + x + y * 16;
			int Tile = PosY * Width + PosX;

			pChestLayer->m_pTiles[Tile].m_Index = Index;
			pExTileLayer->m_pTiles[Tile].m_Index = EXTILE_CHEST;
			str_format(pExTileLayer->m_pExTiles[Tile].m_ExArgs, MAX_EXTENTED_STR,
				"i%c%i%ci%c%i", 0xff, GenMap.m_ChestIDCount, 0xff, 0xff, RANDCONTENT_SMALL);
		}
	}

	{
		int Tile = (Height - 3) * Width + ChunkFrom;
		pExTileLayer->m_pTiles[Tile].m_Index = EXTILE_CHEST_DISPLAY;
		str_format(pExTileLayer->m_pExTiles[Tile].m_ExArgs, MAX_EXTENTED_STR,
			"i%c%i", 0xff, GenMap.m_ChestIDCount);
	}

	if (CHUNK_SIZE > 3)
	{
		GenerateTerrain(GenMap, pLayer, GenMap.m_Eruption, ChunkFrom + 5, ChunkTo);
	}

	GenMap.m_ChestIDCount--;
	return 0;
}

int CGenerator::GenerateChestMedium(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining)
{
	CGenLayerGroup *pPreGroup = GetLayerGroup(GenMap, "Pre");
	if (pPreGroup == NULL)
		return -1;

	int Height = GetLayerHeight(pLayer, ChunkFrom);
	int Width = pLayer->m_Width;

	CGenLayerExTiles *pExTileLayer = static_cast<CGenLayerExTiles *>(GetTileLayer(pGroup, "Extension"));
	CGenLayerTiles *pChestLayer = UseTileLayer(GenMap, pPreGroup, "Chests", "pdl_chests", false);
	if (pChestLayer == NULL || pExTileLayer == NULL)
		return -1;

	for (int x = 0; x < 3; x++)
	{
		for (int y = 0; y < 2; y++)
		{
			int PosX = ChunkFrom + x;
			int PosY = Height + y - 2;
			int Index = 21 + x + y * 16;
			int Tile = PosY * Width + PosX;

			pChestLayer->m_pTiles[Tile].m_Index = Index;
			pExTileLayer->m_pTiles[Tile].m_Index = EXTILE_CHEST;
			str_format(pExTileLayer->m_pExTiles[Tile].m_ExArgs, MAX_EXTENTED_STR,
				"i%c%i%ci%c%i", 0xff, GenMap.m_ChestIDCount, 0xff, 0xff, RANDCONTENT_MEDIUM);
		}
	}

	{
		int Tile = (Height - 3) * Width + ChunkFrom;
		pExTileLayer->m_pTiles[Tile].m_Index = EXTILE_CHEST_DISPLAY;
		str_format(pExTileLayer->m_pExTiles[Tile].m_ExArgs, MAX_EXTENTED_STR,
			"i%c%i", 0xff, GenMap.m_ChestIDCount);
	}

	if (CHUNK_SIZE > 3)
	{
		GenerateTerrain(GenMap, pLayer, GenMap.m_Eruption, ChunkFrom + 5, ChunkTo);
	}

	GenMap.m_ChestIDCount--;
	return 0;
}

bool CGenerator::LoadPNG(CGenImageInfo *pImg, char *pFilename, int StorageType)
{
	char aCompleteFilename[512];
	unsigned char *pBuffer;
	png_t Png; // ignore_convention

			   // open file for reading
	png_init(0, 0); // ignore_convention

	IOHANDLE File = Storage()->OpenFile(pFilename, IOFLAG_READ, StorageType, aCompleteFilename, sizeof(aCompleteFilename));
	if (File)
		io_close(File);
	else
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", pFilename);
		return false;
	}

	int Error = png_open_file(&Png, aCompleteFilename); // ignore_convention
	if (Error != PNG_NO_ERROR)
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", aCompleteFilename);
		if (Error != PNG_FILE_ERROR)
			png_close_file(&Png); // ignore_convention
		return false;
	}

	if (Png.depth != 8 || (Png.color_type != PNG_TRUECOLOR && Png.color_type != PNG_TRUECOLOR_ALPHA)) // ignore_convention
	{
		dbg_msg("game/png", "invalid format. filename='%s'", aCompleteFilename);
		png_close_file(&Png); // ignore_convention
		return false;
	}

	pBuffer = (unsigned char *)mem_alloc(Png.width * Png.height * Png.bpp, 1); // ignore_convention
	png_get_data(&Png, pBuffer); // ignore_convention
	png_close_file(&Png); // ignore_convention

	pImg->m_Width = Png.width; // ignore_convention
	pImg->m_Height = Png.height; // ignore_convention
	if (Png.color_type == PNG_TRUECOLOR) // ignore_convention
		pImg->m_Format = CGenImageInfo::FORMAT_RGB;
	else if (Png.color_type == PNG_TRUECOLOR_ALPHA) // ignore_convention
		pImg->m_Format = CGenImageInfo::FORMAT_RGBA;
	pImg->m_pData = pBuffer;
	return true;
}

CGenerator::CGenerator()
{
	EnvelopeTemplates();
	Doodads();
	MinimalisticShapes();
}

void CGenerator::SetStorage(IStorage *pStorage)
{
	m_pStorage = pStorage;
}

bool CGenerator::Generate(char *pName, CPrevMapInfo PrevMapInfo)
{
	CGeneratingMap NewMap;
	NewMap.m_PrevMapInfo = PrevMapInfo;
	str_format(NewMap.m_aFileName, sizeof(NewMap.m_aFileName), "maps/generated/%s.map", pName);
	NewMap.m_TransitionCount = 3;
	NewMap.m_GenMapCount = 2;
	NewMap.m_ChestIDCount = -1;

	SetMapInfo(NewMap);

	GenerateMap(NewMap);

	bool Done = Save(Storage(), &NewMap) == 1;
	CleanGeneratedMap(NewMap);
	return Done;
}

void CGenerator::GenerateMap(CGeneratingMap& GenMap)
{
	switch (GenMap.m_MapBiome)
	{
	case MAPBIOME_PLAIRIE: GeneratePlairie(GenMap); break;
	case MAPBIOME_POLAR: GeneratePolar(GenMap); break;
	case MAPBIOME_DESERT: GenerateDesert(GenMap); break;
	case MAPBIOME_FOREST: GenerateForest(GenMap); break;
	case MAPBIOME_GRASSLAND: GenerateGrassland(GenMap); break;
	case MAPBIOME_TUNDRA: GenerateTundra(GenMap); break;
	case MAPBIOME_WATERLAND: GenerateWaterland(GenMap); break;
	default: GeneratePlairie(GenMap); break;
	}
}

void CGenerator::GeneratePlairie(CGeneratingMap& GenMap)
{
	GeneratePara0(GenMap);
	GeneratePara10(GenMap);
	GeneratePlanets(GenMap);
	GenerateClouds(GenMap);

	if (GenMap.m_Eruption <= 3)
	{
		GeneratePara25Errupted(GenMap);
		GeneratePara30Errupted(GenMap);
		GeneratePara45Errupted(GenMap);
	}
	else
	{
		GeneratePara25(GenMap);
		GeneratePara30(GenMap);
	}

	GeneratePreGroup(GenMap);
	GenerateGameGroup(GenMap);
	GenerateMidGroup(GenMap);
	GenerateMainGroup(GenMap);
	GeneratePostGroup(GenMap);
	GenerateForeground(GenMap);

	GraphicalMap(GenMap);
}

void CGenerator::GeneratePolar(CGeneratingMap& GenMap)
{
	GeneratePara0(GenMap);
	GeneratePara10(GenMap);
	GeneratePlanets(GenMap);
	GenerateClouds(GenMap);
	GeneratePara25Polar(GenMap);
	GeneratePara30Polar(GenMap);
	GeneratePara45Polar(GenMap);
	GenerateSnow(GenMap, 75, 16, 20);
	GenerateSnow(GenMap, 80, 16, 24);
	GeneratePreGroup(GenMap);
	GenerateGameGroup(GenMap);
	GenerateMidGroupPolar(GenMap);
	GenerateMainGroup(GenMap);
	GeneratePostGroup(GenMap);
	GenerateSnow(GenMap, 120, 32, 64);
	GenerateForeground(GenMap);

	GraphicalMap(GenMap);
}

void CGenerator::GenerateDesert(CGeneratingMap& GenMap)
{
	GeneratePara0(GenMap);
	GeneratePara10(GenMap);
	GeneratePlanets(GenMap);
	GeneratePara25Desert(GenMap);
	GeneratePara30Desert(GenMap);
	GeneratePreGroup(GenMap);
	GenerateGameGroup(GenMap);
	GenerateMidGroup(GenMap);
	GenerateMainGroup(GenMap);
	GeneratePostGroup(GenMap);
	GenerateForeground(GenMap);

	GraphicalMap(GenMap);
}

void CGenerator::GenerateForest(CGeneratingMap& GenMap)
{
	GeneratePara0(GenMap);
	GeneratePara10(GenMap);
	GeneratePlanets(GenMap);
	GenerateClouds(GenMap);
	GeneratePara70Forest(GenMap);
	GeneratePreGroup(GenMap);
	GenerateGameGroup(GenMap);
	GenerateMidGroupForest(GenMap);
	GenerateMainGroup(GenMap);
	GeneratePostGroup(GenMap);
	GenerateForeground(GenMap);

	GraphicalMap(GenMap);
	GenerateForestVegetation(GenMap);
}

void CGenerator::GenerateGrassland(CGeneratingMap& GenMap)
{
	GeneratePara0(GenMap);
	GeneratePara10(GenMap);
	GeneratePlanets(GenMap);
	GenerateClouds(GenMap);
	GeneratePara25(GenMap);
	GeneratePara30(GenMap);
	GeneratePreGroupGrassland(GenMap);
	GenerateGameGroup(GenMap);
	GenerateMidGroup(GenMap);
	GenerateMainGroup(GenMap);
	GeneratePostGroupGrassland(GenMap);
	GenerateForeground(GenMap);

	GraphicalMap(GenMap);
}

void CGenerator::GenerateTundra(CGeneratingMap& GenMap)
{
	GeneratePara0(GenMap);
	GeneratePara10(GenMap);
	GeneratePlanets(GenMap);
	GenerateClouds(GenMap);
	GeneratePara25Tundra(GenMap);
	GeneratePara30Tundra(GenMap);
	GeneratePara45Polar(GenMap);
	GeneratePreGroup(GenMap);
	GenerateGameGroup(GenMap);
	GenerateMidGroup(GenMap);
	GenerateMainGroup(GenMap);
	GeneratePostGroup(GenMap);
	GenerateForeground(GenMap);

	GraphicalMap(GenMap);
}

void CGenerator::GenerateWaterland(CGeneratingMap& GenMap)
{
	GeneratePara0(GenMap);
	GeneratePara10(GenMap);
	GeneratePlanets(GenMap);
	GenerateClouds(GenMap);
	GeneratePara25(GenMap);
	GeneratePara30(GenMap);
	GeneratePreGroup(GenMap);
	GenerateGameGroup(GenMap);
	GenerateMidGroup(GenMap);
	GenerateMainGroup(GenMap);
	GeneratePostGroup(GenMap);
	GenerateForeground(GenMap);

	GraphicalMap(GenMap);
}

void CGenerator::SetMapInfo(CGeneratingMap& GenMap)
{
	GenMap.m_MapType = MAPTYPE_WILDNESS;//MAPTYPE_TOWN
	GenMap.m_Eruption = ClampEruption(GenMap.m_PrevMapInfo.m_Eruption + (rand() % 3 - 1));

	GenMap.m_Temperature = ClampTemp(GenMap.m_PrevMapInfo.m_Temperature + ((rand() % 11) - 5) * 6);
	GenMap.m_Moisture = ClampMoisture(GenMap.m_PrevMapInfo.m_Moisture + ((rand() % 11) - 5) * 6);
	GenMap.m_MapBiome = CalculateBiome(GenMap.m_Temperature, GenMap.m_Moisture);
}

void CGenerator::GeneratePara0(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 0;
	pGroup->m_ParallaxY = 0;
	str_copy(pGroup->m_aName, "Background", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	GenerateBackgroundColor(GenMap, pGroup);
	GenerateBackgroundAquarell(GenMap, pGroup);
	GenerateBackgroundSunColor(GenMap, pGroup);
}

void CGenerator::GenerateBackgroundColor(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerQuads *pQuadLayer = new CGenLayerQuads();
	str_copy(pQuadLayer->m_aName, "BackColor", sizeof(pQuadLayer->m_aName));
	pGroup->m_lLayers.add(pQuadLayer);

	CQuad *pQuad = pQuadLayer->NewQuad();
	pQuad->m_ColorEnv = UseEnvelope(GenMap, "BackColor");

	pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = -SCREEN_WIDTH;
	pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = SCREEN_WIDTH;
	pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = -SCREEN_HEIGHT;
	pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = SCREEN_HEIGHT;
	pQuad->m_aColors[0].r = pQuad->m_aColors[1].r = 94;
	pQuad->m_aColors[0].g = pQuad->m_aColors[1].g = 59;
	pQuad->m_aColors[0].b = pQuad->m_aColors[1].b = 141;
	pQuad->m_aColors[2].r = pQuad->m_aColors[3].r = 204;
	pQuad->m_aColors[2].g = pQuad->m_aColors[3].g = 232;
	pQuad->m_aColors[2].b = pQuad->m_aColors[3].b = 255;
}

void CGenerator::GenerateBackgroundAquarell(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerQuads *pQuadLayer = new CGenLayerQuads();
	str_copy(pQuadLayer->m_aName, "Aquarell", sizeof(pQuadLayer->m_aName));
	pQuadLayer->m_Image = UseImage(GenMap, "pdl_aquarell", false, false);
	pGroup->m_lLayers.add(pQuadLayer);

	CQuad *pQuad = pQuadLayer->NewQuad();
	pQuad->m_ColorEnv = UseEnvelope(GenMap, "Aquarell");

	pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = -SCREEN_WIDTH;
	pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = SCREEN_WIDTH;
	pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = -SCREEN_HEIGHT;
	pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = SCREEN_HEIGHT;
	pQuad->m_aColors[0].a = pQuad->m_aColors[1].a = pQuad->m_aColors[2].a = pQuad->m_aColors[3].a = 77;
}

void CGenerator::GenerateBackgroundSunColor(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerQuads *pQuadLayer = new CGenLayerQuads();
	str_copy(pQuadLayer->m_aName, "SunColor", sizeof(pQuadLayer->m_aName));
	pQuadLayer->m_Image = UseImage(GenMap, "pdl_shine", false, false);
	pGroup->m_lLayers.add(pQuadLayer);

	CQuad *pQuad = pQuadLayer->NewQuad();
	pQuad->m_ColorEnv = UseEnvelope(GenMap, "SunColor");
	
	pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = -831000;
	pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = 831000;
	pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = -188000;
	pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = 1123000;
	pQuad->m_aColors[0].a = pQuad->m_aColors[1].a = pQuad->m_aColors[2].a = pQuad->m_aColors[3].a = 120;
}

void CGenerator::GeneratePara10(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 10;
	pGroup->m_ParallaxY = 10;
	str_copy(pGroup->m_aName, "Stars", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	GenerateStars(GenMap, pGroup);
}

void CGenerator::GenerateStars(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerQuads *pQuadLayer = new CGenLayerQuads();
	str_copy(pQuadLayer->m_aName, "Stars", sizeof(pQuadLayer->m_aName));
	pQuadLayer->m_Image = UseImage(GenMap, "stars", true, false);
	pGroup->m_lLayers.add(pQuadLayer);

	float ParaX = pGroup->m_ParallaxX / 100.0f;
	float ParaY = pGroup->m_ParallaxX / 100.0f;
	const int MaxSize = 64 * 1024;
	const int MinSize = 16 * 1024;
	for (int x = -SCREEN_WIDTH; x < MAP_WIDTH * 32 * 1024 * ParaX + SCREEN_WIDTH; x += MaxSize)
	{
		for (int y = -SCREEN_HEIGHT; y < MAP_HEIGHT * 32 * 1024 * ParaY + SCREEN_HEIGHT; y += MaxSize)
		{
			if (rand() % 10)
				continue;

			vec2 Pos = vec2(x, y) + vec2((rand() % (int)(MaxSize*0.5)) - MaxSize, (rand() % (int)(MaxSize*0.5) - MaxSize));

			GenerateRandomStar(GenMap, pQuadLayer, Pos, MinSize, MaxSize);
		}
	}
}

void CGenerator::GenerateRandomStar(CGeneratingMap& GenMap, CGenLayerQuads *pLayer, vec2 Pos, int MinSize, int MaxSize)
{
	int Type = rand() % 2;
	int Size = rand() % (MaxSize - MinSize) + MinSize;
	float Rotation = (rand() % 360) * pi / 180.0f;
	GenerateStar(GenMap, pLayer, Pos, Size, Type, Rotation);
}

void CGenerator::GenerateStar(CGeneratingMap& GenMap, CGenLayerQuads *pLayer, vec2 Pos, int Size, int Type, float Rotation)
{
	CQuad *pQuad = pLayer->NewQuad();
	pQuad->m_ColorEnv = UseEnvelope(GenMap, "Stars");

	pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = Pos.x - Size*0.5f;
	pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = Pos.x + Size*0.5f;
	pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = Pos.y - Size*0.5f;
	pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = Pos.y + Size*0.5f;
	pQuad->m_aPoints[4].x = Pos.x;
	pQuad->m_aPoints[4].y = Pos.y;

	if (Type == 0)
	{
		pQuad->m_aTexcoords[0].x = pQuad->m_aTexcoords[2].x = 0;
		pQuad->m_aTexcoords[1].x = pQuad->m_aTexcoords[3].x = 512;
		pQuad->m_aTexcoords[0].y = pQuad->m_aTexcoords[1].y = 0;
		pQuad->m_aTexcoords[2].y = pQuad->m_aTexcoords[3].y = 1024;
	}
	else
	{
		pQuad->m_aTexcoords[0].x = pQuad->m_aTexcoords[2].x = 512;
		pQuad->m_aTexcoords[1].x = pQuad->m_aTexcoords[3].x = 1024;
		pQuad->m_aTexcoords[0].y = pQuad->m_aTexcoords[1].y = 0;
		pQuad->m_aTexcoords[2].y = pQuad->m_aTexcoords[3].y = 1024;
	}

	RotateQuad(pQuad, Rotation);
}

void CGenerator::GeneratePlanets(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 0;
	pGroup->m_ParallaxY = 0;
	str_copy(pGroup->m_aName, "Planets", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	GenerateMoon(GenMap, pGroup);
	GenerateSunshine(GenMap, pGroup);
	GenerateSun(GenMap, pGroup);
}

void CGenerator::GenerateMoon(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerQuads *pQuadLayer = new CGenLayerQuads();
	str_copy(pQuadLayer->m_aName, "Moon", sizeof(pQuadLayer->m_aName));
	pQuadLayer->m_Image = UseImage(GenMap, "moon", true, false);
	pGroup->m_lLayers.add(pQuadLayer);

	CQuad *pQuad = pQuadLayer->NewQuad();
	pQuad->m_PosEnv = UseEnvelope(GenMap, "PlanetRotation");

	pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = -196 * 1024;
	pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x =	195 * 1024;
	pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = -458 * 1024;
	pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = -65 * 1024;
	pQuad->m_aPoints[4].x = 0 * 1024;
	pQuad->m_aPoints[4].y = 393 * 1024;
}

void CGenerator::GenerateSunshine(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerQuads *pQuadLayer = new CGenLayerQuads();
	str_copy(pQuadLayer->m_aName, "Sun-Shine", sizeof(pQuadLayer->m_aName));
	pQuadLayer->m_Image = UseImage(GenMap, "pdl_shine", false, false);
	pGroup->m_lLayers.add(pQuadLayer);

	CQuad *pQuad = pQuadLayer->NewQuad();
	pQuad->m_PosEnv = UseEnvelope(GenMap, "PlanetRotation");
	pQuad->m_PosEnvOffset = 720000;

	pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = -524 * 1024;
	pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = 524 * 1024;
	pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = -786 * 1024;
	pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = 262 * 1024;
	pQuad->m_aPoints[4].x = 0 * 1024;
	pQuad->m_aPoints[4].y = 393 * 1024;
}

void CGenerator::GenerateSun(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerQuads *pQuadLayer = new CGenLayerQuads();
	str_copy(pQuadLayer->m_aName, "Sun", sizeof(pQuadLayer->m_aName));
	if(GenMap.m_MapBiome == MAPBIOME_DESERT)
		pQuadLayer->m_Image = UseImage(GenMap, "desert_sun", true, false);
	else
		pQuadLayer->m_Image = UseImage(GenMap, "sun", true, false);
	pGroup->m_lLayers.add(pQuadLayer);

	CQuad *pQuad = pQuadLayer->NewQuad();
	pQuad->m_PosEnv = UseEnvelope(GenMap, "PlanetRotation");
	pQuad->m_PosEnvOffset = 720000;

	pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = -262 * 1024;
	pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = 262 * 1024;
	pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = -524 * 1024;
	pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = 0 * 1024;
	pQuad->m_aPoints[4].x = 0 * 1024;
	pQuad->m_aPoints[4].y = 393 * 1024;
}

void CGenerator::GenerateClouds(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 20;
	pGroup->m_ParallaxY = 20;
	str_copy(pGroup->m_aName, "Clouds", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	CGenLayerQuads *pCloudLayer0 = new CGenLayerQuads();
	str_copy(pCloudLayer0->m_aName, "#0", sizeof(pCloudLayer0->m_aName));
	pCloudLayer0->m_Image = UseImage(GenMap, "bg_cloud1", true, false);
	pGroup->m_lLayers.add(pCloudLayer0);

	CGenLayerQuads *pCloudLayer1 = new CGenLayerQuads();
	str_copy(pCloudLayer1->m_aName, "#1", sizeof(pCloudLayer1->m_aName));
	pCloudLayer1->m_Image = UseImage(GenMap, "bg_cloud2", true, false);
	pGroup->m_lLayers.add(pCloudLayer1);

	CGenLayerQuads *pCloudLayer2 = new CGenLayerQuads();
	str_copy(pCloudLayer2->m_aName, "#2", sizeof(pCloudLayer2->m_aName));
	pCloudLayer2->m_Image = UseImage(GenMap, "bg_cloud3", true, false);
	pGroup->m_lLayers.add(pCloudLayer2);

	for (int x = -SCREEN_WIDTH; x < MAP_WIDTH * 32 * 1024 * 0.2 + SCREEN_WIDTH; x += 500 * 1024)
	{
		if (rand() % 4 == 0)
			continue;

		vec2 Size = vec2(0, 0);
		CGenLayerQuads *pLayer = NULL;
		int CloudType = rand() % 3;
		switch (CloudType)
		{
		case 0: {pLayer = pCloudLayer0; Size = vec2(434, 217) * 1024; } break;
		case 1: {pLayer = pCloudLayer1; Size = vec2(246, 123) * 1024; } break;
		case 2: {pLayer = pCloudLayer2; Size = vec2(330, 165) * 1024; } break;
		}

		GenerateRandomCloud(GenMap, pLayer, Size, x, MAP_HEIGHT * 32 * 0.2);
	}
}

void CGenerator::GenerateRandomCloud(CGeneratingMap& GenMap, CGenLayerQuads *pCloudLayer, vec2 Size, int PosX, int MaxHeight)
{
	float SizeFactor = (rand() % 20) / 20.0f + 1.0f;
	Size *= SizeFactor;
	int Height = (rand() % MaxHeight) * 1024;

	GenerateCloud(GenMap, pCloudLayer, Size, vec2(PosX, Height));
}

void CGenerator::GenerateCloud(CGeneratingMap& GenMap, CGenLayerQuads *pCloudLayer, vec2 Size, vec2 Pos)
{
	CQuad *pQuad = pCloudLayer->NewQuad();
	pQuad->m_ColorEnv = UseEnvelope(GenMap, "Clouds");

	pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = Pos.x - Size.x*0.5f;
	pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = Pos.x + Size.x*0.5f;
	pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = Pos.y - Size.y*0.5f;
	pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = Pos.y + Size.y*0.5f;
	pQuad->m_aPoints[4].x = Pos.x;
	pQuad->m_aPoints[4].y = Pos.y;
}

void CGenerator::GeneratePara25(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 25;
	pGroup->m_ParallaxY = 25;
	str_copy(pGroup->m_aName, "Para 25", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	int Image = UseImage(GenMap, "winter_mountains3", true, false);
	GenerateMountains(GenMap, pGroup, vec3(0, 168, 0), Image, 8000, -64);
}

void CGenerator::GeneratePara30(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 30;
	pGroup->m_ParallaxY = 30;
	str_copy(pGroup->m_aName, "Para 30", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	int Image = UseImage(GenMap, "winter_mountains3", true, false);
	GenerateMountains(GenMap, pGroup, vec3(0, 255, 0), Image, 5000, 64);
}

void CGenerator::GeneratePara25Errupted(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 25;
	pGroup->m_ParallaxY = 25;
	str_copy(pGroup->m_aName, "Para 25", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	int Image = UseImage(GenMap, "mountains", true, false);
	GenerateMountains(GenMap, pGroup, vec3(200, 200, 200), Image, 8000, -64);
}

void CGenerator::GeneratePara30Errupted(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 30;
	pGroup->m_ParallaxY = 30;
	str_copy(pGroup->m_aName, "Para 30", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	int Image = UseImage(GenMap, "mountains", true, false);
	GenerateMountains(GenMap, pGroup, vec3(255, 255, 255), Image, 5000, 64);
}

void CGenerator::GeneratePara45Errupted(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 45;
	pGroup->m_ParallaxY = 45;
	str_copy(pGroup->m_aName, "Para 45", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	int Image = UseImage(GenMap, "winter_mountains3", true, false);
	GenerateMountains(GenMap, pGroup, vec3(0, 255, 0), Image, 5000, 200);
}

void CGenerator::GeneratePara25Polar(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 25;
	pGroup->m_ParallaxY = 25;
	str_copy(pGroup->m_aName, "Para 25", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	int Image = UseImage(GenMap, "winter_mountains", true, false);
	GenerateMountains(GenMap, pGroup, vec3(200, 200, 200), Image, 8000, -64);
}

void CGenerator::GeneratePara30Polar(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 30;
	pGroup->m_ParallaxY = 30;
	str_copy(pGroup->m_aName, "Para 30", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	int Image = UseImage(GenMap, "winter_mountains", true, false);
	GenerateMountains(GenMap, pGroup, vec3(255, 255, 255), Image, 5000, 64);
}

void CGenerator::GeneratePara45Polar(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 45;
	pGroup->m_ParallaxY = 45;
	str_copy(pGroup->m_aName, "Para 45", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	int Image = UseImage(GenMap, "winter_mountains3", true, false);
	GenerateMountains(GenMap, pGroup, vec3(255, 255, 255), Image, 5000, 200);
}

void CGenerator::GeneratePara25Desert(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 25;
	pGroup->m_ParallaxY = 25;
	str_copy(pGroup->m_aName, "Para 25", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	int Image = UseImage(GenMap, "desert_mountains", true, false);
	GenerateMountains(GenMap, pGroup, vec3(255, 181, 69), Image, 8000, -64);
}

void CGenerator::GeneratePara30Desert(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 30;
	pGroup->m_ParallaxY = 30;
	str_copy(pGroup->m_aName, "Para 30", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	int Image = UseImage(GenMap, "desert_mountains", false, false);
	GenerateMountains(GenMap, pGroup, vec3(255, 239, 140), Image, 5000, 64);
}

void CGenerator::GeneratePara70Forest(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 70;
	pGroup->m_ParallaxY = 70;
	str_copy(pGroup->m_aName, "Para 70", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	int Image = UseImage(GenMap, "jungle_background", true, false);
	GenerateSequoias(GenMap, pGroup, vec3(0, 20, 0), Image, 10048, 200);
}

void CGenerator::GeneratePara25Tundra(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 25;
	pGroup->m_ParallaxY = 25;
	str_copy(pGroup->m_aName, "Para 25", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	int Image = UseImage(GenMap, "winter_mountains2", true, false);
	GenerateMountains(GenMap, pGroup, vec3(200, 200, 200), Image, 8000, -64);
}

void CGenerator::GeneratePara30Tundra(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = 30;
	pGroup->m_ParallaxY = 30;
	str_copy(pGroup->m_aName, "Para 30", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	int Image = UseImage(GenMap, "winter_mountains2", true, false);
	GenerateMountains(GenMap, pGroup, vec3(255, 255, 255), Image, 5000, 64);
}

void CGenerator::GenerateMountains(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, vec3 Color, int Image, int TexX, int ShiftY)
{
	CGenLayerQuads *pQuadLayer = new CGenLayerQuads();
	str_copy(pQuadLayer->m_aName, "Maountains", sizeof(pQuadLayer->m_aName));
	pQuadLayer->m_Image = Image;

	pGroup->m_lLayers.add(pQuadLayer);

	float ParaX = pGroup->m_ParallaxX / 100.0f;
	float ParaY = pGroup->m_ParallaxX / 100.0f;

	{
		const int Height = 550 * 1024;
		CQuad *pQuad = pQuadLayer->NewQuad();
		pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = -SCREEN_WIDTH;
		pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = MAP_WIDTH * 32 * 1024 * ParaX + SCREEN_WIDTH;
		pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = MAP_HEIGHT * 32 * 0.5 * 1024 * ParaY - Height*0.5f + ShiftY * 1024;
		pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = MAP_HEIGHT * 32 * 0.5 * 1024 * ParaY + Height*0.5f + ShiftY * 1024;
		pQuad->m_aPoints[4].x = MAP_WIDTH * 32 * 1024 * ParaX * 0.5f;
		pQuad->m_aPoints[4].y = MAP_HEIGHT * 0.5 * 1024;

		pQuad->m_aTexcoords[0].x = pQuad->m_aTexcoords[2].x = 0;
		pQuad->m_aTexcoords[1].x = pQuad->m_aTexcoords[3].x = TexX;
		pQuad->m_aTexcoords[0].y = pQuad->m_aTexcoords[1].y = 0;
		pQuad->m_aTexcoords[2].y = pQuad->m_aTexcoords[3].y = 1024;

		pQuad->m_aColors[0].r = pQuad->m_aColors[1].r = pQuad->m_aColors[2].r = pQuad->m_aColors[3].r = Color.r;
		pQuad->m_aColors[0].g = pQuad->m_aColors[1].g = pQuad->m_aColors[2].g = pQuad->m_aColors[3].g = Color.g;
		pQuad->m_aColors[0].b = pQuad->m_aColors[1].b = pQuad->m_aColors[2].b = pQuad->m_aColors[3].b = Color.b;
	}

	{
		const int Height = 550 * 1024;
		CQuad *pQuad = pQuadLayer->NewQuad();
		pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = -SCREEN_WIDTH;
		pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = MAP_WIDTH * 32 * 1024 * ParaX + SCREEN_WIDTH;
		pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = MAP_HEIGHT * 32 * 0.5 * 1024 * ParaY + Height*0.5f + ShiftY * 1024 - 32 * 1024;
		pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = MAP_HEIGHT * 32 * 1024 * ParaY + SCREEN_HEIGHT;
		pQuad->m_aPoints[4].x = MAP_WIDTH * 32 * 1024 * ParaX * 0.5f;
		pQuad->m_aPoints[4].y = MAP_HEIGHT * 32 * 0.5 * 1024 * ParaY + Height*0.5f + SCREEN_HEIGHT;

		pQuad->m_aTexcoords[0].x = pQuad->m_aTexcoords[2].x = -2646;
		pQuad->m_aTexcoords[1].x = pQuad->m_aTexcoords[3].x = 3396;
		pQuad->m_aTexcoords[0].y = pQuad->m_aTexcoords[1].y = 726;
		pQuad->m_aTexcoords[2].y = pQuad->m_aTexcoords[3].y = 732;

		pQuad->m_aColors[0].r = pQuad->m_aColors[1].r = pQuad->m_aColors[2].r = pQuad->m_aColors[3].r = Color.r;
		pQuad->m_aColors[0].g = pQuad->m_aColors[1].g = pQuad->m_aColors[2].g = pQuad->m_aColors[3].g = Color.g;
		pQuad->m_aColors[0].b = pQuad->m_aColors[1].b = pQuad->m_aColors[2].b = pQuad->m_aColors[3].b = Color.b;
	}
}

void CGenerator::GenerateSequoias(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, vec3 Color, int Image, int TexX, int ShiftY)
{
	CGenLayerQuads *pQuadLayer = new CGenLayerQuads();
	str_copy(pQuadLayer->m_aName, "Sequoias", sizeof(pQuadLayer->m_aName));
	pQuadLayer->m_Image = Image;

	pGroup->m_lLayers.add(pQuadLayer);

	float ParaX = pGroup->m_ParallaxX / 100.0f;
	float ParaY = pGroup->m_ParallaxX / 100.0f;

	{
		const int Height = 3818 * 1024;
		CQuad *pQuad = pQuadLayer->NewQuad();
		pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = -SCREEN_WIDTH;
		pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = MAP_WIDTH * 32 * 1024 * ParaX + SCREEN_WIDTH;
		pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = MAP_HEIGHT * 32 * 0.5 * 1024 * ParaY - Height*0.8f + ShiftY * 1024;
		pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = MAP_HEIGHT * 32 * 0.5 * 1024 * ParaY + Height*0.2f + ShiftY * 1024;
		pQuad->m_aPoints[4].x = MAP_WIDTH * 32 * 1024 * ParaX * 0.5f;
		pQuad->m_aPoints[4].y = MAP_HEIGHT * 0.5 * 1024;

		pQuad->m_aTexcoords[0].x = pQuad->m_aTexcoords[2].x = 0;
		pQuad->m_aTexcoords[1].x = pQuad->m_aTexcoords[3].x = TexX;
		pQuad->m_aTexcoords[0].y = pQuad->m_aTexcoords[1].y = 0;
		pQuad->m_aTexcoords[2].y = pQuad->m_aTexcoords[3].y = 1024;

		pQuad->m_aColors[0].r = pQuad->m_aColors[1].r = pQuad->m_aColors[2].r = pQuad->m_aColors[3].r = Color.r;
		pQuad->m_aColors[0].g = pQuad->m_aColors[1].g = pQuad->m_aColors[2].g = pQuad->m_aColors[3].g = Color.g;
		pQuad->m_aColors[0].b = pQuad->m_aColors[1].b = pQuad->m_aColors[2].b = pQuad->m_aColors[3].b = Color.b;
	}

	{
		const int Height = 3818 * 1024;
		CQuad *pQuad = pQuadLayer->NewQuad();
		pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = -SCREEN_WIDTH;
		pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = MAP_WIDTH * 32 * 1024 * ParaX + SCREEN_WIDTH;
		pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = MAP_HEIGHT * 32 * 0.5 * 1024 * ParaY + Height*0.2f + ShiftY * 1024 - 32 * 1024;
		pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = MAP_HEIGHT * 32 * 1024 * ParaY + SCREEN_HEIGHT;
		pQuad->m_aPoints[4].x = MAP_WIDTH * 32 * 1024 * ParaX * 0.5f;
		pQuad->m_aPoints[4].y = MAP_HEIGHT * 32 * 0.5 * 1024 * ParaY + Height*0.2f + SCREEN_HEIGHT;

		pQuad->m_aTexcoords[0].x = pQuad->m_aTexcoords[2].x = -2646;
		pQuad->m_aTexcoords[1].x = pQuad->m_aTexcoords[3].x = -3822;
		pQuad->m_aTexcoords[0].y = pQuad->m_aTexcoords[1].y = 1197;
		pQuad->m_aTexcoords[2].y = pQuad->m_aTexcoords[3].y = 1203;

		pQuad->m_aColors[0].r = pQuad->m_aColors[1].r = pQuad->m_aColors[2].r = pQuad->m_aColors[3].r = Color.r;
		pQuad->m_aColors[0].g = pQuad->m_aColors[1].g = pQuad->m_aColors[2].g = pQuad->m_aColors[3].g = Color.g;
		pQuad->m_aColors[0].b = pQuad->m_aColors[1].b = pQuad->m_aColors[2].b = pQuad->m_aColors[3].b = Color.b;
	}
}

void CGenerator::GeneratePreGroup(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	str_copy(pGroup->m_aName, "Pre", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	GenerateWoodBack(GenMap, pGroup);
	GenerateDoodadsBack(GenMap, pGroup);
}

void CGenerator::GeneratePreGroupGrassland(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	str_copy(pGroup->m_aName, "Pre", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	GenerateWoodBack(GenMap, pGroup);
}

void CGenerator::GenerateGameGroup(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();//automaticly gamegroup, when gamelayer is group
	str_copy(pGroup->m_aName, "Game", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	GenerateGameLayer(GenMap, pGroup);
	GenerateExGameLayer(GenMap, pGroup);
	if (GenMap.m_MapBiome == MAPBIOME_WATERLAND)
		GenerateCollisionMapWaterland(GenMap, pGroup);
	else
		GenerateCollisionMap(GenMap, pGroup);
}

void CGenerator::GenerateMidGroup(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	str_copy(pGroup->m_aName, "Mid", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	GenerateWaterTiles(GenMap, pGroup);
	GenerateWaterQuads(GenMap, pGroup);
}

void CGenerator::GenerateMidGroupPolar(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	str_copy(pGroup->m_aName, "Mid", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	GenerateIceLayer(GenMap, pGroup);
}

void CGenerator::GenerateMidGroupForest(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	str_copy(pGroup->m_aName, "Mid", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	GenerateWaterGreenTiles(GenMap, pGroup);
	GenerateWaterGreenQuads(GenMap, pGroup);
}

void CGenerator::GenerateMainGroup(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	str_copy(pGroup->m_aName, "Main", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	GenerateMainLayer(GenMap, pGroup);
}

void CGenerator::GeneratePostGroup(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	str_copy(pGroup->m_aName, "Post", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	GenerateDoodadsMinimalistic(GenMap, pGroup);
	GenerateDoodadsFront(GenMap, pGroup);
	GenerateMainTransitions(GenMap, pGroup);
}

void CGenerator::GeneratePostGroupGrassland(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	str_copy(pGroup->m_aName, "Post", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	GenerateDoodadsMinimalistic(GenMap, pGroup);
	GenerateMainTransitions(GenMap, pGroup);
}


void CGenerator::GraphicalMap(CGeneratingMap& GenMap)
{
	AutomapMainLayer(GenMap);

	//no more automap
	GenerateWoodLeft(GenMap);
	GenerateWoodRight(GenMap);
	SetMainTransitions(GenMap);

	GenerateVegetation(GenMap, GenMap.m_MapBiome == MAPBIOME_TUNDRA);
	if (GenMap.m_MapBiome == MAPBIOME_POLAR)
		GenerateRiverFrozen(GenMap);
	else if (GenMap.m_MapBiome == MAPBIOME_WATERLAND)
		GenerateRiverWaterWaterland(GenMap);
	else
		GenerateRiverWater(GenMap);
	GenerateGameLayerStuff(GenMap);
	GenerateLightSources(GenMap);
}

void CGenerator::GenerateGameLayer(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerTiles *pGameLayer = new CGenLayerTiles(MAP_WIDTH, MAP_HEIGHT);
	pGameLayer->m_LayerType = LAYERTILETYPE_GAME;
	str_copy(pGameLayer->m_aName, "Game", sizeof(pGameLayer->m_aName));
	pGroup->m_lLayers.add(pGameLayer);
}

void CGenerator::GenerateExGameLayer(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerExTiles *pExTileLayer = new CGenLayerExTiles(MAP_WIDTH, MAP_HEIGHT);
	pExTileLayer->m_LayerType = LAYERTILETYPE_EX;
	str_copy(pExTileLayer->m_aName, "Extension", sizeof(pExTileLayer->m_aName));
	pGroup->m_lLayers.add(pExTileLayer);
}

void CGenerator::GenerateWaterTiles(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerTiles *pTileLayer = new CGenLayerTiles(MAP_WIDTH, MAP_HEIGHT);
	//pTileLayer->m_Image = UseImage(GenMap, "pdl_water", false, false);
	pTileLayer->m_Color = CColor(0, 38, 225, 128);
	str_copy(pTileLayer->m_aName, "Water Tiles", sizeof(pTileLayer->m_aName));
	pGroup->m_lLayers.add(pTileLayer);
}

void CGenerator::GenerateWaterQuads(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerQuads *pQuadLayer = new CGenLayerQuads();
	pQuadLayer->m_Image = UseImage(GenMap, "pdl_water_quad", false, false);
	str_copy(pQuadLayer->m_aName, "Water Quads", sizeof(pQuadLayer->m_aName));
	pGroup->m_lLayers.add(pQuadLayer);
}

void CGenerator::GenerateIceLayer(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerTiles *pTileLayer = new CGenLayerTiles(MAP_WIDTH, MAP_HEIGHT);
	pTileLayer->m_Image = UseImage(GenMap, "pdl_ice_river", false, false);
	str_copy(pTileLayer->m_aName, "Ice", sizeof(pTileLayer->m_aName));
	pGroup->m_lLayers.add(pTileLayer);
}

void CGenerator::GenerateWaterGreenTiles(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerTiles *pTileLayer = new CGenLayerTiles(MAP_WIDTH, MAP_HEIGHT);
	//pTileLayer->m_Image = UseImage(GenMap, "pdl_water_green", false, false);
	pTileLayer->m_Color = CColor(51, 123, 55, 174);
	str_copy(pTileLayer->m_aName, "Water Tiles", sizeof(pTileLayer->m_aName));
	pGroup->m_lLayers.add(pTileLayer);
}

void CGenerator::GenerateWaterGreenQuads(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerQuads *pQuadLayer = new CGenLayerQuads();
	pQuadLayer->m_Image = UseImage(GenMap, "pdl_water_green_quad", false, false);
	str_copy(pQuadLayer->m_aName, "Water Quads", sizeof(pQuadLayer->m_aName));
	pGroup->m_lLayers.add(pQuadLayer);
}


void CGenerator::GenerateMainLayer(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerTiles *pTileLayer  = new CGenLayerTiles(MAP_WIDTH, MAP_HEIGHT);
	
	switch (GenMap.m_MapBiome)
	{
	case MAPBIOME_PLAIRIE: pTileLayer->m_Image = UseImage(GenMap, "grass_main", true, true); break;
	case MAPBIOME_POLAR: pTileLayer->m_Image = UseImage(GenMap, "winter_main", true, true); break;
	case MAPBIOME_DESERT: pTileLayer->m_Image = UseImage(GenMap, "desert_main", true, true); break;
	case MAPBIOME_FOREST: pTileLayer->m_Image = UseImage(GenMap, "grass_main", true, true); break;
	case MAPBIOME_GRASSLAND: pTileLayer->m_Image = UseImage(GenMap, "grass_main", true, true); break;
	case MAPBIOME_TUNDRA: pTileLayer->m_Image = UseImage(GenMap, "grass_main", true, true); break;
	default: pTileLayer->m_Image = UseImage(GenMap, "grass_main", true, true); break;
	}

	if (GenMap.m_MapBiome == MAPBIOME_FOREST)
		pTileLayer->m_Color = CColor(150, 200, 255, 255);
	else if (GenMap.m_MapBiome == MAPBIOME_GRASSLAND)
		pTileLayer->m_Color = CColor(230, 230, 0, 255);

	str_copy(pTileLayer->m_aName, "Main", sizeof(pTileLayer->m_aName));
	pGroup->m_lLayers.add(pTileLayer);
}

void CGenerator::AutomapMainLayer(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGameGroup = GetLayerGroup(GenMap, "Game");
	CGenLayerGroup *pMainGroup = GetLayerGroup(GenMap, "Main");
	if (pGameGroup == NULL || pMainGroup == NULL)
		return;

	CGenLayerTiles *pGameLayer = GetTileLayer(pGameGroup, "Game");
	CGenLayerTiles *pMainLayer = GetTileLayer(pMainGroup, "Main");
	if (pGameLayer == NULL || pMainLayer == NULL)
		return;

	for (int i = 0; i < pMainLayer->m_Width*pMainLayer->m_Height; i++)
	{
		if (pGameLayer->m_pTiles[i].m_Index == TILE_SOLID)
			pMainLayer->m_pTiles[i].m_Index = 1;
	}
	//mem_copy(pMainLayer->m_pTiles, pGameLayer->m_pTiles, pMainLayer->m_Width*pMainLayer->m_Height*sizeof(CTile));
	if (pMainLayer->m_Image != -1)
		GenMap.m_lImages[pMainLayer->m_Image]->m_AutoMapper.Proceed(pMainLayer, pGameLayer, 0);
}

void CGenerator::GenerateMainTransitions(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerTiles *pTileLayer = new CGenLayerTiles(MAP_WIDTH, MAP_HEIGHT);
	pTileLayer->m_Image = UseImage(GenMap, "pdl_tree", false, false);
	str_copy(pTileLayer->m_aName, "Tree", sizeof(pTileLayer->m_aName));
	pGroup->m_lLayers.add(pTileLayer);
}

void CGenerator::GenerateWoodBack(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerTiles *pTileLayer = new CGenLayerTiles(MAP_WIDTH, MAP_HEIGHT);
	pTileLayer->m_Image = UseImage(GenMap, "pdl_tree", false, false);
	str_copy(pTileLayer->m_aName, "Tree Back", sizeof(pTileLayer->m_aName));
	pGroup->m_lLayers.add(pTileLayer);
}

void CGenerator::GenerateWoodLeft(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pPostGroup = GetLayerGroup(GenMap, "Post");
	CGenLayerGroup *pGameGroup = GetLayerGroup(GenMap, "Game");
	CGenLayerGroup *pMainGroup = GetLayerGroup(GenMap, "Main");
	CGenLayerGroup *pPreGroup = GetLayerGroup(GenMap, "Pre");
	if (pPostGroup == NULL || pGameGroup == NULL || pMainGroup == NULL || pPreGroup == NULL)
		return;

	CGenLayerTiles *pTileLayer = GetTileLayer(pPostGroup, "Tree");
	CGenLayerTiles *pGameLayer = GetTileLayer(pGameGroup, "Game");
	CGenLayerExTiles *pExTileLayer = static_cast<CGenLayerExTiles *>(GetTileLayer(pGameGroup, "Extension"));
	CGenLayerTiles *pMainLayer = GetTileLayer(pMainGroup, "Main");
	CGenLayerTiles *pBackWoodLayer = GetTileLayer(pPreGroup, "Tree Back");
	if (pTileLayer == NULL || pGameLayer == NULL || pBackWoodLayer == NULL || pMainLayer == NULL || pExTileLayer == NULL)
		return;

	int Height = GetLayerHeight(pGameLayer, 0);
	for (int x = 0; x < NEXT_MAP_BORDER - 3; x++)
	{
		bool Last = x == (NEXT_MAP_BORDER - 4);
		int SpecialTile = 0;
		int Mirror = 0;
		if (Last == false && x != 0 && x != 29 && x != 30)
		{
			SpecialTile = rand() % 7;
			Mirror = rand() % 4;
		}

		for (int y = 0; y < 5; y++)
		{
			int PosY = y + Height - 4;
			int Tile = PosY * pTileLayer->m_Width + x;
			int TileY = y;

			if (Mirror == 2 || Mirror == 3)
				TileY = 4 - y;
			int Index = TileY * 16 + 3 + Last;
			if (SpecialTile == 3)
				Index = TileY * 16 + 1;
			else if (SpecialTile == 4)
				Index = TileY * 16 + 2;
			else if (SpecialTile == 5)
			{
				Index = TileY * 16 + 113;
				if (TileY == 0 && !(Mirror == 2 || Mirror == 3))
					pTileLayer->m_pTiles[Tile - pTileLayer->m_Width].m_Index = 97;
			}
			else if (SpecialTile == 6)
			{
				if (TileY > 0)
					Index = TileY * 16 + 114;
			}

			if (x == 29 && y == 0)
				Index = 114;
			if (x == 30 && y == 0)
				Index = 115;

			pTileLayer->m_pTiles[Tile].m_Index = Index;
			pTileLayer->m_pTiles[Tile].m_Flags = Mirror;
		}
	}

	for (int x = 0; x < 2; x++)
	{
		int PosX = x + NEXT_MAP_BORDER - 4;
		for (int y = 0; y < 5; y++)
		{
			int PosY = y + Height - 4;
			int Tile = PosY * pTileLayer->m_Width + PosX;
			pBackWoodLayer->m_pTiles[Tile].m_Index = y * 16 + 6 + x;
		}
	}

	pMainLayer->m_pTiles[Height * pTileLayer->m_Width + (NEXT_MAP_BORDER - 3)].m_Index = TILE_AIR;
	pMainLayer->m_pTiles[Height * pTileLayer->m_Width + (NEXT_MAP_BORDER - 4)].m_Index = TILE_AIR;
	pTileLayer->m_pTiles[Height * pTileLayer->m_Width + (NEXT_MAP_BORDER - 3)].m_Index = 73;

	for (int x = 0; x < NEXT_MAP_BORDER - 3; x++)
	{
		if (x != 29 && x != 30)
			pGameLayer->m_pTiles[(Height - 4) * pGameLayer->m_Width + x].m_Index = TILE_NOHOOK;
		else
			pGameLayer->m_pTiles[(Height - 4) * pGameLayer->m_Width + x].m_Index = TILE_PUSH_UP;

		pGameLayer->m_pTiles[(Height) * pGameLayer->m_Width + x].m_Index = TILE_NOHOOK;
	}
	
	for (int y = 0; y < Height - 4; y++)
	{
		pGameLayer->m_pTiles[y * pGameLayer->m_Width + NEXT_MAP_BORDER - 5].m_Index = TILE_MOVE_STOP;
		pGameLayer->m_pTiles[y * pGameLayer->m_Width + NEXT_MAP_BORDER - 6].m_Index = TILE_MOVE_STOP;
		pGameLayer->m_pTiles[y * pGameLayer->m_Width + NEXT_MAP_BORDER - 13].m_Index = TILE_MOVE_RIGHT;
		pGameLayer->m_pTiles[y * pGameLayer->m_Width].m_Index = TILE_NOHOOK;
	}

	for (int y = 0; y < 3; y++)
	{
		int PosY = Height - 3 + y;
		pGameLayer->m_pTiles[PosY * pGameLayer->m_Width + NEXT_MAP_BORDER - 4].m_Index = TILE_MOVE_STOP;
		pGameLayer->m_pTiles[PosY * pGameLayer->m_Width + NEXT_MAP_BORDER - 7].m_Index = TILE_MOVE_LEFT;
		pGameLayer->m_pTiles[PosY * pGameLayer->m_Width + 21].m_Index = TILE_MOVE_STOP;
		pGameLayer->m_pTiles[PosY * pGameLayer->m_Width + 22].m_Index = TILE_MOVE_STOP;
		pGameLayer->m_pTiles[PosY * pGameLayer->m_Width].m_Index = TILE_NOHOOK;

		pExTileLayer->m_pTiles[PosY * pGameLayer->m_Width + 23].m_Index = EXTILE_GENMAP;
		str_format(pExTileLayer->m_pExTiles[PosY * pGameLayer->m_Width + 23].m_ExArgs, MAX_EXTENTED_STR,
			"i%c%i%ci%c%i%ci%c0", 0xff, 0, 0xff, 0xff, 1, 0xff, 0xff);
	}

	{
		int Tile = (Height - 3) * pGameLayer->m_Width + 25;
		pGameLayer->m_pTiles[Tile].m_Index = TILE_MOVE_RIGHT;
		pExTileLayer->m_pTiles[Tile].m_Index = EXTILE_MAPTRANSITION_TO;
		str_format(pExTileLayer->m_pExTiles[Tile].m_ExArgs, MAX_EXTENTED_STR, "i%c%i", 0xff, 1);
	}
}

void CGenerator::GenerateWoodRight(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pPostGroup = GetLayerGroup(GenMap, "Post");
	CGenLayerGroup *pGameGroup = GetLayerGroup(GenMap, "Game");
	CGenLayerGroup *pMainGroup = GetLayerGroup(GenMap, "Main");
	CGenLayerGroup *pPreGroup = GetLayerGroup(GenMap, "Pre");
	if (pPostGroup == NULL || pGameGroup == NULL || pMainGroup == NULL || pPreGroup == NULL)
		return;

	CGenLayerTiles *pTileLayer = GetTileLayer(pPostGroup, "Tree");
	CGenLayerTiles *pGameLayer = GetTileLayer(pGameGroup, "Game");
	CGenLayerExTiles *pExTileLayer = static_cast<CGenLayerExTiles *>(GetTileLayer(pGameGroup, "Extension"));
	CGenLayerTiles *pMainLayer = GetTileLayer(pMainGroup, "Main");
	CGenLayerTiles *pBackWoodLayer = GetTileLayer(pPreGroup, "Tree Back");
	if (pTileLayer == NULL || pGameLayer == NULL || pBackWoodLayer == NULL || pMainLayer == NULL || pExTileLayer == NULL)
		return;

	int Width = pTileLayer->m_Width;
	int Height = GetLayerHeight(pGameLayer, Width-1);
	for (int x = Width - NEXT_MAP_BORDER + 3; x < Width; x++)
	{
		bool First = x == (Width - NEXT_MAP_BORDER + 3);
		int SpecialTile = 0;
		int Mirror = TILEFLAG_VFLIP;
		if (First == false && x != Width-1 && x != Width-30 && x != Width-31)
		{
			SpecialTile = rand() % 7;
			Mirror = rand() % 4;
		}

		for (int y = 0; y < 5; y++)
		{
			int PosY = y + Height - 4;
			int Tile = PosY * pTileLayer->m_Width + x;
			int TileY = y;

			if (Mirror == 2 || Mirror == 3)
				TileY = 4 - y;
			int Index = TileY * 16 + 3 + First;
			if (SpecialTile == 3)
				Index = TileY * 16 + 1;
			else if (SpecialTile == 4)
				Index = TileY * 16 + 2;
			else if (SpecialTile == 5)
			{
				Index = TileY * 16 + 113;
				if (TileY == 0 && !(Mirror == 2 || Mirror == 3))
					pTileLayer->m_pTiles[Tile - pTileLayer->m_Width].m_Index = 97;
			}
			else if (SpecialTile == 6)
			{
				if (TileY > 0)
					Index = TileY * 16 + 114;
			}

			if (x == Width-30 && y == 0)
				Index = 114;
			if (x == Width-31 && y == 0)
				Index = 115;

			pTileLayer->m_pTiles[Tile].m_Index = Index;
			pTileLayer->m_pTiles[Tile].m_Flags = Mirror;
		}
	}

	for (int x = 0; x < 2; x++)
	{
		int PosX = x + Width - NEXT_MAP_BORDER + 2;
		for (int y = 0; y < 5; y++)
		{
			int PosY = y + Height - 4;
			int Tile = PosY * pTileLayer->m_Width + PosX;
			pBackWoodLayer->m_pTiles[Tile].m_Index = y * 16 + 6 + x;
		}
	}

	pMainLayer->m_pTiles[Height * pTileLayer->m_Width + (Width - NEXT_MAP_BORDER + 2)].m_Index = TILE_AIR;
	pMainLayer->m_pTiles[Height * pTileLayer->m_Width + (Width - NEXT_MAP_BORDER + 3)].m_Index = TILE_AIR;
	pTileLayer->m_pTiles[Height * pTileLayer->m_Width + (Width - NEXT_MAP_BORDER + 2)].m_Index = 73;
	pTileLayer->m_pTiles[Height * pTileLayer->m_Width + (Width - NEXT_MAP_BORDER + 2)].m_Flags = TILEFLAG_VFLIP;

	for (int x = Width - NEXT_MAP_BORDER + 3; x < Width; x++)
	{
		if ( x != 0 && x != Width - 30 && x != Width - 31)
			pGameLayer->m_pTiles[(Height - 4) * pGameLayer->m_Width + x].m_Index = TILE_NOHOOK;
		else
			pGameLayer->m_pTiles[(Height - 4) * pGameLayer->m_Width + x].m_Index = TILE_PUSH_UP;

		pGameLayer->m_pTiles[(Height) * pGameLayer->m_Width + x].m_Index = TILE_NOHOOK;
	}

	for (int y = 0; y < Height - 4; y++)
	{
		pGameLayer->m_pTiles[y * pGameLayer->m_Width + (Width - NEXT_MAP_BORDER + 4)].m_Index = TILE_MOVE_STOP;
		pGameLayer->m_pTiles[y * pGameLayer->m_Width + (Width - NEXT_MAP_BORDER + 5)].m_Index = TILE_MOVE_STOP;
		pGameLayer->m_pTiles[y * pGameLayer->m_Width + (Width - NEXT_MAP_BORDER + 12)].m_Index = TILE_MOVE_LEFT;
		pGameLayer->m_pTiles[y * pGameLayer->m_Width + Width -1].m_Index = TILE_NOHOOK;
	}

	for (int y = 0; y < 3; y++)
	{
		int PosY = Height - 3 + y;
		pGameLayer->m_pTiles[PosY * pGameLayer->m_Width + Width - NEXT_MAP_BORDER + 3].m_Index = TILE_MOVE_STOP;
		pGameLayer->m_pTiles[PosY * pGameLayer->m_Width + Width - NEXT_MAP_BORDER + 6].m_Index = TILE_MOVE_RIGHT;
		pGameLayer->m_pTiles[PosY * pGameLayer->m_Width + Width - 22].m_Index = TILE_MOVE_STOP;
		pGameLayer->m_pTiles[PosY * pGameLayer->m_Width + Width - 23].m_Index = TILE_MOVE_STOP;
		pGameLayer->m_pTiles[PosY * pGameLayer->m_Width + Width - 1].m_Index = TILE_NOHOOK;
		pExTileLayer->m_pTiles[PosY * pGameLayer->m_Width + Width - 24].m_Index = EXTILE_GENMAP;
		str_format(pExTileLayer->m_pExTiles[PosY * pGameLayer->m_Width + Width - 24].m_ExArgs, MAX_EXTENTED_STR,
			"i%c%i%ci%c%i%ci%c0", 0xff, 1, 0xff, 0xff, 2, 0xff, 0xff);
	}

	{
		int Tile = (Height - 3) * pGameLayer->m_Width + Width - 26;
		pGameLayer->m_pTiles[Tile].m_Index = TILE_MOVE_LEFT;
		pExTileLayer->m_pTiles[Tile].m_Index = EXTILE_MAPTRANSITION_TO;
		str_format(pExTileLayer->m_pExTiles[Tile].m_ExArgs, MAX_EXTENTED_STR, "i%c%i", 0xff, 2);
	}
}

void CGenerator::GenerateDoodadsBack(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerTiles *pTileLayer = new CGenLayerTiles(MAP_WIDTH, MAP_HEIGHT);

	switch (GenMap.m_MapBiome)
	{
	case MAPBIOME_PLAIRIE: pTileLayer->m_Image = UseImage(GenMap, "grass_doodads", true, false); break;
	case MAPBIOME_POLAR: pTileLayer->m_Image = UseImage(GenMap, "winter_doodads", true, false); break;
	case MAPBIOME_DESERT: pTileLayer->m_Image = UseImage(GenMap, "desert_doodads", true, false); break;
	case MAPBIOME_FOREST: pTileLayer->m_Image = UseImage(GenMap, "jungle_doodads", true, false); break;
	case MAPBIOME_GRASSLAND: pTileLayer->m_Image = UseImage(GenMap, "", true, false); break;
	case MAPBIOME_TUNDRA: pTileLayer->m_Image = UseImage(GenMap, "grass_doodads", true, false); break;
	default: pTileLayer->m_Image = UseImage(GenMap, "grass_doodads", true, false); break;
	}
	
	str_copy(pTileLayer->m_aName, "dds_back", sizeof(pTileLayer->m_aName));
	pTileLayer->m_Color = CColor(200, 200, 200, 255);
	pGroup->m_lLayers.add(pTileLayer);
}

void CGenerator::GenerateDoodadsMinimalistic(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerTiles *pTileLayer = new CGenLayerTiles(MAP_WIDTH, MAP_HEIGHT);
	switch (GenMap.m_MapBiome)
	{
	case MAPBIOME_PLAIRIE: pTileLayer->m_Image = UseImage(GenMap, "grass_doodads", true, false); break;
	case MAPBIOME_POLAR: pTileLayer->m_Image = UseImage(GenMap, "winter_main", true, false); break;
	case MAPBIOME_DESERT: pTileLayer->m_Image = UseImage(GenMap, "desert_main", true, false);
	case MAPBIOME_FOREST: pTileLayer->m_Image = UseImage(GenMap, "jungle_doodads", true, false); break;
	case MAPBIOME_GRASSLAND: pTileLayer->m_Image = UseImage(GenMap, "grass_doodads", true, false); break;
	case MAPBIOME_TUNDRA: pTileLayer->m_Image = UseImage(GenMap, "grass_doodads", true, false); break;
	default: pTileLayer->m_Image = UseImage(GenMap, "grass_doodads", true, false); break;
	}
	str_copy(pTileLayer->m_aName, "dds_min", sizeof(pTileLayer->m_aName));
	pGroup->m_lLayers.add(pTileLayer);
}


void CGenerator::GenerateDoodadsFront(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerTiles *pTileLayer = new CGenLayerTiles(MAP_WIDTH, MAP_HEIGHT);
	switch (GenMap.m_MapBiome)
	{
	case MAPBIOME_PLAIRIE: pTileLayer->m_Image = UseImage(GenMap, "grass_doodads", true, false); break;
	case MAPBIOME_POLAR: pTileLayer->m_Image = UseImage(GenMap, "winter_doodads", true, false); break;
	case MAPBIOME_DESERT: pTileLayer->m_Image = UseImage(GenMap, "desert_doodads", true, false); break;
	case MAPBIOME_FOREST: pTileLayer->m_Image = UseImage(GenMap, "jungle_doodads", true, false); break;
	case MAPBIOME_GRASSLAND: pTileLayer->m_Image = UseImage(GenMap, "", true, false); break;
	case MAPBIOME_TUNDRA: pTileLayer->m_Image = UseImage(GenMap, "grass_doodads", true, false); break;
	default: pTileLayer->m_Image = UseImage(GenMap, "grass_doodads", true, false); break;
	}
	str_copy(pTileLayer->m_aName, "dds_front", sizeof(pTileLayer->m_aName));
	pGroup->m_lLayers.add(pTileLayer);
}

void CGenerator::GenerateVegetation(CGeneratingMap& GenMap, int Variation)
{
	CGenLayerGroup *pPreGroup = GetLayerGroup(GenMap, "Pre");
	CGenLayerGroup *pPostGroup = GetLayerGroup(GenMap, "Post");

	CGenLayerTiles *pBackLayer = NULL;
	CGenLayerTiles *pMinLayer = NULL;
	CGenLayerTiles *pFrontLayer = NULL;
	

	if (pPreGroup)
		pBackLayer = GetTileLayer(pPreGroup, "dds_back");

	if (pPostGroup)
	{
		pMinLayer = GetTileLayer(pPostGroup, "dds_min");
		pFrontLayer = GetTileLayer(pPostGroup, "dds_front");
	}

	if (pBackLayer)
		GenerateVegetation(GenMap, pPreGroup, pBackLayer, Variation);

	if (pMinLayer)
		GenerateVegetationMinimalistic(GenMap, pPostGroup, pMinLayer);

	if (pFrontLayer)
		GenerateVegetation(GenMap, pPostGroup, pFrontLayer, Variation);
}

void CGenerator::GenerateVegetation(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pTileLayer, int Variation)
{
	CGenLayerGroup *pGameGroup = GetLayerGroup(GenMap, "Game");
	if (pGameGroup == NULL)
		return;

	CGenLayerTiles *pGameLayer = GetTileLayer(pGameGroup, "Game");
	CDoodad *pDoodad = GetDoodad(GenMap, pTileLayer, Variation);

	if (pGameLayer == NULL || pDoodad == NULL)
		return;

	int Width = pTileLayer->m_Width;
	for (int x = NEXT_MAP_BORDER; x < Width - NEXT_MAP_BORDER; x++)
	{
		int Height = GetLayerHeight(pGameLayer, x) - 1;
		int Space = GetLayerSpace(pGameLayer, x, Height);

		if (pGameLayer->m_pTiles[Height * Width + x].m_Index == TILE_WATER)
			continue;

		if (Space > 2)
		{
			int SpaceUsed = GenerateVegetation(pTileLayer, pDoodad, x, Space, Height);
			x += SpaceUsed + 2;
		}
	}
}

int CGenerator::GenerateVegetation(CGenLayerTiles *pTileLayer, CDoodad *pDoodad, int PosX, int Space, int Height)
{
	int Width = pTileLayer->m_Width;
	int From = PosX + 1;
	int To = Space - 1;
	int NumShapes = 0;
	int RealSpace = Space - 2;

	if (RealSpace <= 0)
		return 0;

	RealSpace = (rand() % RealSpace) + 1;

	NumShapes = pDoodad->GetNumShapes(RealSpace);

	if (NumShapes <= 0)
		return GenerateVegetation(pTileLayer, pDoodad, PosX, Space - 1, Height);

	int ChosenShape = rand() % NumShapes;
	CDoodadShape *pShape = pDoodad->GetShape(RealSpace, ChosenShape);
	if (pShape == NULL)
		return GenerateVegetation(pTileLayer, pDoodad, PosX, Space - 1, Height);

	if (pShape->m_Index > 0 && pShape->m_Height > 0)
	{
		for (int x = 0; x < RealSpace; x++)
		{
			for (int y = 0; y < pShape->m_Height; y++)
			{
				int Index = pShape->m_Index + x + y * 16;
				int Tile = (Height - pShape->m_Height + y + 1) * Width + From + x;
				pTileLayer->m_pTiles[Tile].m_Index = Index;
			}
		}

		return RealSpace;
	}

	return 0;
}

void CGenerator::GenerateVegetationMinimalistic(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pTileLayer)
{
	CGenLayerGroup *pGameGroup = GetLayerGroup(GenMap, "Game");
	if (pGameGroup == NULL)
		return;

	CGenLayerTiles *pGameLayer = GetTileLayer(pGameGroup, "Game");
	CMinimalisticShape *pShape = GetMinimalisticShape(GenMap, pTileLayer);
	if (pGameLayer == NULL || pShape == NULL)
		return;

	int Width = pTileLayer->m_Width;
	for (int x = NEXT_MAP_BORDER; x < Width - NEXT_MAP_BORDER; x++)
	{
		int Height = GetLayerHeight(pGameLayer, x) - 1;
		int Space = GetLayerSpace(pGameLayer, x, Height);

		if (pGameLayer->m_pTiles[Height * Width + x].m_Index == TILE_WATER)
			continue;

		if (Space > 2 + pShape->m_Space * 2)
		{
			GenerateVegetationMinimalistic(pTileLayer, pShape, pGroup, pGameGroup, x, Space, Height);
			x += Space - 1;
		}
	}
}

void CGenerator::GenerateVegetationMinimalistic(CGenLayerTiles *pTileLayer, CMinimalisticShape *pShape, CGenLayerGroup *pGroup, CGenLayerGroup *pGameGroup, int PosX, int Space, int Height)
{
	CGenLayerTiles *pGameLayer = GetTileLayer(pGameGroup, "Game");
	if (pGameLayer == NULL)
		return;

	int Width = pTileLayer->m_Width;
	if (Space + PosX >= Width - NEXT_MAP_BORDER)
		Space -= NEXT_MAP_BORDER;

	for (int x = 0 + pShape->m_Space; x < Space - pShape->m_Space; x++)
	{
		int Tile = Height * Width + PosX + x;
		int Index = pShape->m_Tile;
		int Flags = rand()%2;

		if (x == 0 && pGameLayer->m_pTiles[Tile - 1].m_Index == TILE_AIR)
		{
			Index = pShape->m_Start;
			Flags = 0;
		}
		else if (x == Space - 1 && pGameLayer->m_pTiles[Tile + 1].m_Index == TILE_AIR)
		{
			Index = pShape->m_Start;
			Flags = 1;
		}

		pTileLayer->m_pTiles[Tile].m_Index = Index;
		pTileLayer->m_pTiles[Tile].m_Flags = Flags;
	}
}

int CGenerator::GenerateRiver(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, CGenLayerTiles *pLayer, int ChunkFrom, int ChunkTo, int Chunksremaining)
{
	int ExtraChunks = rand() % 6;
	int SinkingLevel = 2;//(rand() % 2) + 1;
	int Height = pLayer->m_Height;
	int Width = pLayer->m_Width;
	int WaterLevel = GetLayerHeight(pLayer, ChunkFrom) + 1;
	array<int> SinkingHeights;
	ChunkTo -= 2;
	ChunkFrom += 2;

	while (ExtraChunks > 0)
	{
		if (Chunksremaining < ExtraChunks)
			ExtraChunks--;
		else
			break;
	}

	if (ExtraChunks > 0)
	{
		GeneratePlain(GenMap, pLayer, ChunkTo, ChunkTo + 2 + ExtraChunks*CHUNK_SIZE);
		ChunkTo += ExtraChunks*CHUNK_SIZE;
	}

	
	for (int x = ChunkFrom; x < ChunkTo; x++)
	{
		int MidPoint = ChunkFrom + (ChunkTo - ChunkFrom) / 2;
		int HighestTile = GetLayerHeight(pLayer, x - 1);
		int FromHeight = HighestTile;

		if (x < ChunkFrom + 1 || x > ChunkTo - 2)
		{
			HighestTile = HighestTile + (x < MidPoint ? 2 : -2);
		}
		else if (x < MidPoint && x % SinkingLevel == 0)
		{
			HighestTile += rand() % 4;;

			if (HighestTile > Height - 2)
				HighestTile = Height - 2;

			SinkingHeights.add(HighestTile - FromHeight);
		}	
		else if (x > MidPoint && x % SinkingLevel == 0)
		{
			int ChosenSinkingHeight = rand() % SinkingHeights.size();
			int NewChange = SinkingHeights[ChosenSinkingHeight];
			SinkingHeights.remove_index(ChosenSinkingHeight);
			HighestTile -= NewChange;
		}

		

		for (int h = 0; h < Height; h++)
			pLayer->m_pTiles[h * Width + x].m_Index = h < HighestTile ? TILE_AIR : TILE_SOLID;

		//fill water
		for (int y = WaterLevel; y < Height; y++)
		{
			int h = y;
			if (h > Height - 2)
				h = Height - 2;

			if (pLayer->m_pTiles[h * Width + x].m_Index == TILE_AIR)
				pLayer->m_pTiles[h * Width + x].m_Index = TILE_WATER;
		}


	}

	return ExtraChunks;
}

void CGenerator::GenerateRiverWater(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGameGroup = GetLayerGroup(GenMap, "Game");
	CGenLayerGroup *pMidGroup = GetLayerGroup(GenMap, "Mid");
	if (pGameGroup == NULL || pMidGroup == NULL)
		return;

	CGenLayerTiles *pGameLayer = GetTileLayer(pGameGroup, "Game");
	CGenLayerTiles *pTilesWater = GetTileLayer(pMidGroup, "Water Tiles");
	CGenLayerQuads *pQuadsWater = GetQuadLayer(pMidGroup, "Water Quads");
	if (pGameLayer == NULL || pTilesWater == NULL || pQuadsWater == NULL)
		return;

	int Width = pGameLayer->m_Width;
	int Height = pGameLayer->m_Height;
	for (int x = 1; x < Width - 1; x++)
	{
		for (int y = 1; y < Height - 1; y++)
		{
			if (pTilesWater->m_pTiles[y * Width + x].m_Index == 1)
				continue;// already water

			if (pGameLayer->m_pTiles[y * Width + x].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[y * Width + x + 1].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[y * Width + x - 1].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[(y - 1 ) * Width + x].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[(y - 1) * Width + x + 1].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[(y - 1) * Width + x - 1].m_Index == TILE_WATER)
				pTilesWater->m_pTiles[y * Width + x].m_Index = 1;
		}
	}

	for (int x = 4; x < Width - 4; x++)
	{
		for (int y = 1; y < Height - 1; y++)
		{
			int Tile = y * Width + x;
			bool FoundWater = false;

			for (int i = -1; i <= 2; i++)
			{
				if (pGameLayer->m_pTiles[Tile + i].m_Index == TILE_WATER)
				{
					FoundWater = true;
					break;
				}
			}

			if (FoundWater)
				continue;

			for (int i = -1; i <= 2; i++)
			{
				if (pGameLayer->m_pTiles[Tile + Width + i].m_Index == TILE_WATER)
				{
					FoundWater = true;
					break;
				}
			}

			if(FoundWater)
			{
				
				CQuad *pQuad = pQuadsWater->NewQuad();
				pQuad->m_PosEnv = UseEnvelope(GenMap, "Water");

				pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = x * 32 * 1024;
				pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = (x + 1) * 32 * 1024;
				pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = y * 32 * 1024;
				pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = (y + 1) * 32 * 1024;
				pQuad->m_aPoints[4].x = x * 32 * 1024 + 16;
				pQuad->m_aPoints[4].y = y * 32 * 1024 + 16;

				pQuad->m_aTexcoords[0].x = pQuad->m_aTexcoords[2].x = 0;
				pQuad->m_aTexcoords[1].x = pQuad->m_aTexcoords[3].x = 1024;
				pQuad->m_aTexcoords[0].y = pQuad->m_aTexcoords[1].y = 26;
				pQuad->m_aTexcoords[2].y = pQuad->m_aTexcoords[3].y = 986;
			}

		}
	}
}

void CGenerator::GenerateRiverWaterWaterland(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGameGroup = GetLayerGroup(GenMap, "Game");
	CGenLayerGroup *pMidGroup = GetLayerGroup(GenMap, "Mid");
	if (pGameGroup == NULL || pMidGroup == NULL)
		return;

	CGenLayerTiles *pGameLayer = GetTileLayer(pGameGroup, "Game");
	CGenLayerTiles *pTilesWater = GetTileLayer(pMidGroup, "Water Tiles");
	CGenLayerQuads *pQuadsWater = GetQuadLayer(pMidGroup, "Water Quads");
	if (pGameLayer == NULL || pTilesWater == NULL || pQuadsWater == NULL)
		return;

	int Width = pGameLayer->m_Width;
	int Height = pGameLayer->m_Height;
	for (int x = 1; x < Width - 1; x++)
	{
		for (int y = 1; y < Height - 1; y++)
		{
			if (pTilesWater->m_pTiles[y * Width + x].m_Index == 1)
				continue;// already water

			if (pGameLayer->m_pTiles[y * Width + x].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[y * Width + x + 1].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[y * Width + x - 1].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[(y - 1) * Width + x].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[(y - 1) * Width + x + 1].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[(y - 1) * Width + x - 1].m_Index == TILE_WATER)
				pTilesWater->m_pTiles[y * Width + x].m_Index = 1;
		}
	}

	for (int x = 0; x < Width; x++)
	{
		int y = Height / 2;
		int Tile = y * Width + x;
		if (pGameLayer->m_pTiles[Tile].m_Index == TILE_SOLID &&
			pGameLayer->m_pTiles[Tile + 1].m_Index == TILE_SOLID &&
			pGameLayer->m_pTiles[Tile - 1].m_Index == TILE_SOLID &&
			pGameLayer->m_pTiles[Tile + 2].m_Index == TILE_SOLID)
			continue;

		CQuad *pQuad = pQuadsWater->NewQuad();
		pQuad->m_PosEnv = UseEnvelope(GenMap, "Water");

		pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = x * 32 * 1024;
		pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = (x + 1) * 32 * 1024;
		pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = y * 32 * 1024;
		pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = (y + 1) * 32 * 1024;
		pQuad->m_aPoints[4].x = x * 32 * 1024 + 16;
		pQuad->m_aPoints[4].y = y * 32 * 1024 + 16;

		pQuad->m_aTexcoords[0].x = pQuad->m_aTexcoords[2].x = 0;
		pQuad->m_aTexcoords[1].x = pQuad->m_aTexcoords[3].x = 1024;
		pQuad->m_aTexcoords[0].y = pQuad->m_aTexcoords[1].y = 26;
		pQuad->m_aTexcoords[2].y = pQuad->m_aTexcoords[3].y = 986;
	}
}

void CGenerator::GenerateRiverFrozen(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGameGroup = GetLayerGroup(GenMap, "Game");
	CGenLayerGroup *pMidGroup = GetLayerGroup(GenMap, "Mid");
	if (pGameGroup == NULL || pMidGroup == NULL)
		return;

	CGenLayerTiles *pGameLayer = GetTileLayer(pGameGroup, "Game");
	CGenLayerExTiles *pExTileLayer = static_cast<CGenLayerExTiles *>(GetTileLayer(pGameGroup, "Extension"));
	CGenLayerTiles *pTilesIce = GetTileLayer(pMidGroup, "Ice");
	if (pGameLayer == NULL || pTilesIce == NULL || pExTileLayer == NULL)
		return;

	int Width = pGameLayer->m_Width;
	int Height = pGameLayer->m_Height;

	for (int x = 1; x < Width - 1; x++)
	{
		for (int y = 1; y < Height - 1; y++)
		{
			int Tile = y * Width + x;

			if (pTilesIce->m_pTiles[Tile].m_Index != 0)
				continue;// already water

			if (pGameLayer->m_pTiles[y * Width + x].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[y * Width + x + 1].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[y * Width + x - 1].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[(y - 1) * Width + x].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[(y - 1) * Width + x + 1].m_Index == TILE_WATER ||
				pGameLayer->m_pTiles[(y - 1) * Width + x - 1].m_Index == TILE_WATER)
			{
				if (pTilesIce->m_pTiles[Tile - Width].m_Index == 0)
					pTilesIce->m_pTiles[Tile].m_Index = 2;
				else
				{
					if (rand() % 5 == 0)
						pTilesIce->m_pTiles[Tile].m_Index = 3 + rand() % 13;
					else
						pTilesIce->m_pTiles[Tile].m_Index = 1;
				}
			}
		}
	}

	for (int x = 1; x < Width - 1; x++)
	{
		for (int y = 1; y < Height - 1; y++)
		{
			int Tile = y * Width + x;

			if (pGameLayer->m_pTiles[Tile].m_Index != TILE_WATER)
				continue;

			pGameLayer->m_pTiles[Tile].m_Index = TILE_NOHOOK;
			pExTileLayer->m_pTiles[Tile].m_Index = EXTILE_ICE;
		}
	}
}

void CGenerator::GenerateSnow(CGeneratingMap& GenMap, int Para, int Skip, int Size)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	pGroup->m_ParallaxX = Para;
	pGroup->m_ParallaxY = Para;
	str_copy(pGroup->m_aName, "Snow", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	GenerateSnowFlakes(GenMap, pGroup, Skip, Size);

}

void CGenerator::GenerateSnowFlakes(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, int Skip, int Size)
{
	CGenLayerQuads *pQuadLayer = new CGenLayerQuads();
	str_copy(pQuadLayer->m_aName, "Flakes", sizeof(pQuadLayer->m_aName));
	pGroup->m_lLayers.add(pQuadLayer);

	float ParaX = pGroup->m_ParallaxX / 100.0f;
	const int MaxSize = 128 * 1024;
	for (int x = -SCREEN_WIDTH; x < MAP_WIDTH * 32 * 1024 * ParaX + SCREEN_WIDTH; x += Skip *1024)
	{
		vec2 Pos = vec2(x, -16*1024) + vec2((rand() % (int)(MaxSize*0.5)) - MaxSize, (rand() % (int)(MaxSize*0.5) - MaxSize));
		GenerateSnowFlake(GenMap, pQuadLayer, Pos, Size);
	}
}

void CGenerator::GenerateSnowFlake(CGeneratingMap& GenMap, CGenLayerQuads *pQuadLayer, vec2 Pos, int Size)
{
	long RandomNumber = rand() * (RAND_MAX + +1) + rand();

	CQuad *pQuad = pQuadLayer->NewQuad();
	pQuad->m_PosEnv = UseEnvelope(GenMap, "Snow");
	pQuad->m_PosEnvOffset = RandomNumber % 230000;
	pQuadLayer->m_Image = UseImage(GenMap, "snow", true, false);

	pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = Pos.x - Size * 1024;
	pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = Pos.x + Size * 1024;
	pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = Pos.y - Size * 1024;
	pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = Pos.y + Size * 1024;
	pQuad->m_aPoints[4].x = Pos.x;
	pQuad->m_aPoints[4].y = Pos.y - Size * 1024;
}

void CGenerator::GenerateForeground(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGroup = new CGenLayerGroup();
	str_copy(pGroup->m_aName, "Foreground", sizeof(pGroup->m_aName));
	GenMap.m_lGroups.add(pGroup);

	GenerateDarkening(GenMap, pGroup);
}

void CGenerator::GenerateDarkening(CGeneratingMap& GenMap, CGenLayerGroup *pGroup)
{
	CGenLayerTiles *pTileLayer = new CGenLayerTiles(MAP_WIDTH, MAP_HEIGHT);
	str_copy(pTileLayer->m_aName, "Darkening", sizeof(pTileLayer->m_aName));
	pTileLayer->m_ColorEnv = UseEnvelope(GenMap, "Darkening");
	pTileLayer->m_Color = CColor(0, 0, 0, 255);
	pGroup->m_lLayers.add(pTileLayer);

	for (int i = 0; i < pTileLayer->m_Width * pTileLayer->m_Height; i++)
		pTileLayer->m_pTiles[i].m_Index = 1;
}

void CGenerator::GenerateGameLayerStuff(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGameGroup = GetLayerGroup(GenMap, "Game");
	if (pGameGroup == NULL)
		return;

	CGenLayerTiles *pGameLayer = GetTileLayer(pGameGroup, "Game");
	if (pGameLayer == NULL)
		return;

	int Tile = (pGameLayer->m_Height / 2 - 2) * pGameLayer->m_Width + (NEXT_MAP_BORDER - 3);
	pGameLayer->m_pTiles[Tile].m_Index = ENTITY_OFFSET + ENTITY_SPAWN;
}

void CGenerator::GenerateLightSources(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pPreGroup = GetLayerGroup(GenMap, "Pre");
	CGenLayerGroup *pGameGroup = GetLayerGroup(GenMap, "Game");
	CGenLayerGroup *pForground = GetLayerGroup(GenMap, "Foreground");

	if (pPreGroup == NULL || pGameGroup == NULL || pForground == NULL)
		return;

	CGenLayerTiles *pGameLayer = GetTileLayer(pGameGroup, "Game");
	CGenLayerTiles *pDarkeningLayer = GetTileLayer(pForground, "Darkening");

	if (pGameLayer == NULL || pDarkeningLayer == NULL)
		return;

	int Width = pGameLayer->m_Width;
	for (int x = NEXT_MAP_BORDER; x < Width - NEXT_MAP_BORDER; x++)
	{
		int Height = GetLayerHeight(pGameLayer, x) - 1;
		int Space = GetLayerSpace(pGameLayer, x, Height);

		if (pGameLayer->m_pTiles[Height * Width + x].m_Index == TILE_WATER)
			continue;

		if (Space > 4)
		{
			if (rand() % 10)
			{
				x += 16;
				continue;
			}

			CGenLayerTiles *pLanternLayer = UseTileLayer(GenMap, pPreGroup, "Lanterns", "winter_doodads", true);
			CGenLayerQuads *pLightLayer = UseQuadLayer(GenMap, pForground, "Lightsrc", "pdl_lightsource");

			for (int ny = 0; ny < 4; ny++)
				for (int nx = 0; nx < 2; nx++)
				{
					pLanternLayer->m_pTiles[(Height - 3 + ny) * Width + x + nx].m_Index = ny * 16 + nx + (ny > 1 ? 206 : 142);
				}

			for (int nx = 0; nx < 16; nx++)
				for (int ny = 0; ny < 16; ny++)
				{
					int Tile = max(0, Height + ny - 10) * Width + max(0, x + nx - 7);
					pDarkeningLayer->m_pTiles[Tile].m_Index = 0;
				}

			{
				CQuad *pQuad = pLightLayer->NewQuad();
				pQuad->m_ColorEnv = UseEnvelope(GenMap, "Darkening");

				pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = (x - 7) * 32 * 1024;
				pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = (x + 9) * 32 * 1024;
				pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = (Height - 10) * 32 * 1024;
				pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = (Height + 6) * 32 * 1024;
			}

			x += 32;
		}
	}
}

void CGenerator::SetMainTransitions(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pGameGroup = GetLayerGroup(GenMap, "Game");
	if (pGameGroup == NULL)
		return;

	CGenLayerExTiles *pExTileLayer = static_cast<CGenLayerExTiles *>(GetTileLayer(pGameGroup, "Extension"));
	int ChosenTrans = rand() % (GenMap.m_GenMapCount);
	for (int x = 2; x < pExTileLayer->m_Width-2; x++)
	{
		for (int y = 2; y < pExTileLayer->m_Height-2; y++)
		{
			char aBuf[MAX_EXTENTED_STR];
			char *pBuf = aBuf;
			int Tile = y * pExTileLayer->m_Width + x;
			if (pExTileLayer->m_pTiles[Tile].m_Index != EXTILE_GENMAP)
				continue;

			str_copy(aBuf, pExTileLayer->m_pExTiles[Tile].m_ExArgs, sizeof(aBuf));
			int ID = GetMapInteger(&pBuf, "Generator", vec2(x, y));
			if (ID != ChosenTrans)
				continue;

			pExTileLayer->m_pTiles[Tile].m_Index = EXTILE_MAPTRANSITION_FROM;
			str_format(pExTileLayer->m_pExTiles[Tile].m_ExArgs, MAX_EXTENTED_STR,
				"s%c%s%ci%c%i%ci%c%i%ci%c0", 0xff, GenMap.m_PrevMapInfo.m_aName, 0xff, 0xff, GenMap.m_PrevMapInfo.m_TransitionID, 0xff, 0xff, ChosenTrans > 1 ? 1 : 0, 0xff, 0xff);

			for (int nx = -2; nx <= 2; nx++)
				if (pExTileLayer->m_pTiles[Tile + nx].m_Index == EXTILE_MAPTRANSITION_TO)
					str_format(pExTileLayer->m_pExTiles[Tile + nx].m_ExArgs, MAX_EXTENTED_STR, "i%c%i", 0xff, 0);

		}
	}
}

void CGenerator::GenerateForestVegetation(CGeneratingMap& GenMap)
{
	CGenLayerGroup *pPreGroup = GetLayerGroup(GenMap, "Pre");
	CGenLayerGroup *pPostGroup = GetLayerGroup(GenMap, "Post");

	if (pPreGroup)
	{
		CGenLayerTiles *pTileLayer = new CGenLayerTiles(MAP_WIDTH, MAP_HEIGHT);
		pTileLayer->m_Image = UseImage(GenMap, "grass_doodads", true, false);
		str_copy(pTileLayer->m_aName, "dds_extra", sizeof(pTileLayer->m_aName));
		pTileLayer->m_Color = CColor(200, 200, 200, 255);
		pPreGroup->m_lLayers.add(pTileLayer);

		GenerateVegetation(GenMap, pPreGroup, pTileLayer, 0);
	}

	if (pPostGroup)
	{
		CGenLayerTiles *pTileLayer = new CGenLayerTiles(MAP_WIDTH, MAP_HEIGHT);
		pTileLayer->m_Image = UseImage(GenMap, "grass_doodads", true, false);
		str_copy(pTileLayer->m_aName, "dds_extra", sizeof(pTileLayer->m_aName));
		pTileLayer->m_Color = CColor(150, 200, 255, 255);
		pPostGroup->m_lLayers.add(pTileLayer);

		GenerateVegetation(GenMap, pPostGroup, pTileLayer, 0);
	}
}

int CGenerator::UseImage(CGeneratingMap& GenMap, char *pName, bool External, bool Automapper)
{
	int Index = -1;
	CGeneratorImage *pImage = NULL;

	for (int i = 0; i < GenMap.m_lImages.size(); i++)
	{
		if (str_comp(GenMap.m_lImages[i]->m_aName, pName) == 0)
		{
			pImage = GenMap.m_lImages[i];
			Index = i;
			break;
		}
	}

	if (pImage == NULL)
	{
		pImage = new CGeneratorImage(Storage());
		if (External == false)
		{
			char aName[128];
			str_format(aName, sizeof(aName), "mapres/generate/%s.png", pName);
			if (LoadPNG(pImage, aName, 1) == 0)
			{
				dbg_msg("Generator", "Failed to load image %s", pName);
				return -1;
			}
		}

		pImage->m_External = External;
		str_copy(pImage->m_aName, pName, sizeof(pImage->m_aName));
		GenMap.m_lImages.add(pImage);
		Index = GenMap.m_lImages.size() - 1;
	}

	if (Automapper)
	{
		if (pImage->m_AutoMapper.IsLoaded() == false)
		{
			pImage->m_AutoMapper.Load(pName);
		}
	}

	return Index;
}

int CGenerator::UseEnvelope(CGeneratingMap& GenMap, char *pName)
{
	for (int i = 0; i < GenMap.m_lEnvelopes.size(); i++)
	{
		if (str_comp(GenMap.m_lEnvelopes[i]->m_aName, pName) == 0)
			return i;
	}

	int FoundIndex = -1;
	for (int i = 0; i < m_lEnvelopeTemplates.size(); i++)
	{
		if (str_comp(m_lEnvelopeTemplates[i]->m_aName, pName) == 0)
		{
			FoundIndex = i;
			break;
		}
	}

	if (FoundIndex == -1)
	{
		dbg_msg("Generator", "Envelope '%s' not found!", pName);
		return -1;
	}

	CGenEnvelope *pEnvelope = new CGenEnvelope(0);
	*pEnvelope = *m_lEnvelopeTemplates[FoundIndex];
	GenMap.m_lEnvelopes.add(pEnvelope);
	return GenMap.m_lEnvelopes.size() - 1;
}

void CGenerator::AddEnvelopeTemplate(CGenEnvelope *pEnvelope, char *pName)
{
	str_copy(pEnvelope->m_aName, pName, sizeof(pEnvelope->m_aName));
	m_lEnvelopeTemplates.add(pEnvelope);
}

void CGenerator::AddDoodad(CDoodad *pDoodads, char *pName, int Variation)
{
	str_copy(pDoodads->m_aName, pName, sizeof(pDoodads->m_aName));
	pDoodads->m_Variation = Variation;
	m_lDoodads.add(pDoodads);
}

void CGenerator::AddMinimalisticShape(CMinimalisticShape Shape, char *pName)
{
	str_copy(Shape.m_aName, pName, sizeof(Shape.m_aName));
	m_lMinimalisticShapes.add(Shape);
}

void CGenerator::CleanGeneratedMap(CGeneratingMap &GenMap)
{
	for (int i = 0; i < GenMap.m_lGroups.size(); i++)
	{
		CGenLayerGroup *pGroup = GenMap.m_lGroups[i];
		pGroup->m_lLayers.delete_all();
	}
	GenMap.m_lGroups.delete_all();
	GenMap.m_lEnvelopes.delete_all();
	GenMap.m_lImages.delete_all();
}

int CGenerator::GetLayerHeight(CGenLayerTiles *pLayer, int PosX)
{
	int Height = pLayer->m_Height - 1;
	for (int h = 0; h < pLayer->m_Height; h++)
	{
		int Tile = h * pLayer->m_Width + PosX;
		if (pLayer->m_pTiles[Tile].m_Index == TILE_SOLID || pLayer->m_pTiles[Tile].m_Index == TILE_NOHOOK)
		{
			Height = h;
			break;
		}
	}
	return Height;
}

int CGenerator::GetLayerSpace(CGenLayerTiles *pLayer, int PosX, int Height)
{
	int Width = pLayer->m_Width;
	int Space = 0;
	for (int x = PosX; x < Width - NEXT_MAP_BORDER; x++)
	{
		int Tile = Height * Width + x;
		if(pLayer->m_pTiles[Tile].m_Index == TILE_SOLID || pLayer->m_pTiles[Tile].m_Index == TILE_NOHOOK ||
			(pLayer->m_pTiles[Tile + Width].m_Index != TILE_SOLID && pLayer->m_pTiles[Tile + Width].m_Index != TILE_NOHOOK))
			break;

		Space++;
	}

	return Space;
}

CGenLayerGroup *CGenerator::GetLayerGroup(CGeneratingMap& GenMap, char *pName)
{
	for (int i = 0; i < GenMap.m_lGroups.size(); i++)
	{
		CGenLayerGroup *pGroup = GenMap.m_lGroups[i];

		if (str_comp(pGroup->m_aName, pName) == 0)
			return pGroup;
	}
	return NULL;
}

CGenLayerTiles *CGenerator::UseTileLayer(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, char *pName, char *pImage, bool External, char *pEnv, CColor Color)
{
	CGenLayerTiles *pTileLayer = GetTileLayer(pGroup, pName);
	if (pTileLayer)
		return pTileLayer;

	pTileLayer = new CGenLayerTiles(MAP_WIDTH, MAP_HEIGHT);
	str_copy(pTileLayer->m_aName, pName, sizeof(pTileLayer->m_aName));

	if(pImage[0])
		pTileLayer->m_Image = UseImage(GenMap, pImage, External, false);

	if(pEnv[0])
		pTileLayer->m_ColorEnv = UseEnvelope(GenMap, pEnv);

	pTileLayer->m_Color = Color;
	pGroup->m_lLayers.add(pTileLayer);
	return pTileLayer;
}

CGenLayerQuads *CGenerator::UseQuadLayer(CGeneratingMap& GenMap, CGenLayerGroup *pGroup, char *pName, char *pImage, bool External)
{
	CGenLayerQuads *pQuadLayer = GetQuadLayer(pGroup, pName);
	if (pQuadLayer)
		return pQuadLayer;

	pQuadLayer = new CGenLayerQuads();
	str_copy(pQuadLayer->m_aName, pName, sizeof(pQuadLayer->m_aName));

	if (pImage[0])
		pQuadLayer->m_Image = UseImage(GenMap, pImage, External, false);

	pGroup->m_lLayers.add(pQuadLayer);
	return pQuadLayer;
}

CGenLayerTiles *CGenerator::GetTileLayer(CGenLayerGroup *pGroup, char *pName)
{
	for (int i = 0; i < pGroup->m_lLayers.size(); i++)
	{
		CGenLayerTiles *pLayer = dynamic_cast<CGenLayerTiles *>(pGroup->m_lLayers[i]);
		if (pLayer == NULL)
			continue;

		if (str_comp(pLayer->m_aName, pName) == 0)
			return pLayer;
	}
	return NULL;
}

CGenLayerQuads *CGenerator::GetQuadLayer(CGenLayerGroup *pGroup, char *pName)
{
	for (int i = 0; i < pGroup->m_lLayers.size(); i++)
	{
		CGenLayerQuads *pLayer = dynamic_cast<CGenLayerQuads *>(pGroup->m_lLayers[i]);
		if (pLayer == NULL)
			continue;

		if (str_comp(pLayer->m_aName, pName) == 0)
			return pLayer;
	}
	return NULL;
}

CDoodad *CGenerator::GetDoodad(CGeneratingMap& GenMap, CGenLayerTiles *pTileLayer, int Variation)
{
	CDoodad *pDoodad = NULL;
	int Image = pTileLayer->m_Image;
	if (Image < 0 || Image >= GenMap.m_lImages.size())
		return NULL;

	for (int i = 0; i < m_lDoodads.size(); i++)
	{
		if (str_comp(m_lDoodads[i]->m_aName, GenMap.m_lImages[Image]->m_aName) == 0 && m_lDoodads[i]->m_Variation == Variation)
		{
			pDoodad = m_lDoodads[i];
			break;
		}
	}

	return pDoodad;
}

CMinimalisticShape *CGenerator::GetMinimalisticShape(CGeneratingMap& GenMap, CGenLayerTiles *pTileLayer)
{
	int Image = pTileLayer->m_Image;
	if (Image < 0 || Image >= GenMap.m_lImages.size())
		return NULL;

	for (int i = 0; i < m_lMinimalisticShapes.size(); i++)
	{
		if (str_comp(m_lMinimalisticShapes[i].m_aName, GenMap.m_lImages[Image]->m_aName) == 0)
			return &m_lMinimalisticShapes[i];
	}

	return NULL;
}