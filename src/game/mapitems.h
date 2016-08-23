/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_MAPITEMS_H
#define GAME_MAPITEMS_H

#define MAX_EXTENTED_STR 48

#include <base/math.h>

enum
{
	MAPTYPE_WILDNESS = 0,
	MAPTYPE_TOWN,
	MAPTYPE_DUNGEON,
	MAPTYPE_BOSSROOM,
	NUM_MAPTYPES,

	MAPBIOME_PLAIRIE = 0,
	MAPBIOME_TUNDRA,
	MAPBIOME_POLAR,
	MAPBIOME_DESERT,
	MAPBIOME_GRASSLAND,
	MAPBIOME_WATERLAND,
	MAPBIOME_FOREST,
	NUM_MAPBIOMES,
};

static char *s_aMapTypeNames[NUM_MAPTYPES] = {
	"Wildness",
	"Town",
	"Dungeon",
	"Bossroom",
};

static char *s_aMapBiomNames[NUM_MAPBIOMES] = {
	"Plairie",
	"Tundra",
	"Polar",
	"Desert",
	"Grassland",
	"Waterland",
	"Jungle",
};

// layer types
enum
{
	LAYERTYPE_INVALID=0,
	LAYERTYPE_GAME,
	LAYERTYPE_TILES,
	LAYERTYPE_QUADS,
	LAYERTYPE_HIDDEN,

	MAPITEMTYPE_VERSION=0,
	MAPITEMTYPE_INFO,
	MAPITEMTYPE_IMAGE,
	MAPITEMTYPE_ENVELOPE,
	MAPITEMTYPE_GROUP,
	MAPITEMTYPE_LAYER,
	MAPITEMTYPE_ENVPOINTS,


	CURVETYPE_STEP=0,
	CURVETYPE_LINEAR,
	CURVETYPE_SLOW,
	CURVETYPE_FAST,
	CURVETYPE_SMOOTH,
	NUM_CURVETYPES,

	// game layer tiles
	ENTITY_NULL=0,
	ENTITY_SPAWN,
	ENTITY_SPAWN_RED,
	ENTITY_SPAWN_BLUE,
	ENTITY_FLAGSTAND_RED,
	ENTITY_FLAGSTAND_BLUE,
	ENTITY_ARMOR_1,
	ENTITY_HEALTH_1,
	ENTITY_WEAPON_SHOTGUN,
	ENTITY_WEAPON_GRENADE,
	ENTITY_POWERUP_NINJA,
	ENTITY_WEAPON_RIFLE,
	NUM_ENTITIES,

	TILE_AIR=0,
	TILE_SOLID,
	TILE_DEATH,
	TILE_NOHOOK,
	TILE_WATER,
	TILE_NPC_COL,
	TILE_PUSH_RIGHT,
	TILE_PUSH_LEFT,
	TILE_PUSH_UP,
	TILE_PUSH_DOWN,
	TILE_PUSH_STOP,
	TILE_MOVE_LEFT,
	TILE_MOVE_STOP,
	TILE_MOVE_RIGHT,
	TILE_NPC_SPAWN_GUARD,
	TILE_NPC_SPAWN_BEGGAR,
	TILE_DEV_TEL,
	TILE_NPC_ENEMY_COL,
	TILE_HOME,
	TILE_NPC_SPAWN_SHOP,

	EXTILE_NONE = 0,
	EXTILE_MAPTRANSITION_FROM,
	EXTILE_MAPTRANSITION_TO,
	EXTILE_NPC_HELPER,
	EXTILE_HEAL,
	EXTILE_NPC_SPAWNER,
	EXTILE_CHEST,
	EXTILE_CHEST_DISPLAY,
	EXTILE_DOOR,
	EXTILE_DOOR_TRIGGER,
	EXTILE_PUZZLE,
	EXTILE_NPC_TICKETSELLER,
	EXTILE_BOLDER,
	EXTILE_BOLDER_TRIGGER,
	EXTILE_BOSS_SPAWNER,
	EXTILE_WEAPON_MOUNT,
	EXTILE_GENMAP,
	EXTILE_ICE,

	TILEFLAG_VFLIP=1,
	TILEFLAG_HFLIP=2,
	TILEFLAG_OPAQUE=4,
	TILEFLAG_ROTATE=8,

	LAYERFLAG_DETAIL=1,

	LAYERTILETYPE_NONE = 0,
	LAYERTILETYPE_GAME = 1,
	LAYERTILETYPE_EX = 3,

	ENTITY_OFFSET=255-16*4,
};

struct CPoint
{
	int x, y; // 22.10 fixed point
};

struct CColor
{
	int r, g, b, a;
	CColor() { CColor(255, 255, 255, 255); }
	CColor(int nR, int nG, int nB, int nA)
		: r(nR), g(nG), b(nB), a(nA) {}
};

struct CQuad
{
	CPoint m_aPoints[5];
	CColor m_aColors[4];
	CPoint m_aTexcoords[4];

	int m_PosEnv;
	int m_PosEnvOffset;

	int m_ColorEnv;
	int m_ColorEnvOffset;
};

class CTile
{
public:
	unsigned char m_Index;
	unsigned char m_Flags;
	unsigned char m_Skip;
	unsigned char m_Reserved;
};

class CExTile
{
public:
	char m_ExArgs[MAX_EXTENTED_STR];
};

struct CMapItemInfo
{
	int m_Version;
	int m_Author;
	int m_MapVersion;
	int m_Credits;
	int m_License;
} ;

struct CMapItemInfoEx : CMapItemInfo
{
	char m_MapType;
	char m_Temperature;
	unsigned char m_Moisture;
	char m_Eruption;
	char m_TicketLevel;
};

struct CMapItemImage
{
	int m_Version;
	int m_Width;
	int m_Height;
	int m_External;
	int m_ImageName;
	int m_ImageData;
} ;

struct CMapItemGroup_v1
{
	int m_Version;
	int m_OffsetX;
	int m_OffsetY;
	int m_ParallaxX;
	int m_ParallaxY;

	int m_StartLayer;
	int m_NumLayers;
} ;


struct CMapItemGroup : public CMapItemGroup_v1
{
	enum { CURRENT_VERSION=3 };

	int m_UseClipping;
	int m_ClipX;
	int m_ClipY;
	int m_ClipW;
	int m_ClipH;

	int m_aName[3];
} ;

struct CMapItemLayer
{
	int m_Version;
	int m_Type;
	int m_Flags;
} ;

struct CMapItemLayerTilemap
{
	CMapItemLayer m_Layer;
	int m_Version;

	int m_Width;
	int m_Height;
	int m_LayerType;

	CColor m_Color;
	int m_ColorEnv;
	int m_ColorEnvOffset;

	int m_Image;
	int m_Data;
	int m_ExData;

	int m_ExDataSize;

	int m_aName[3];
} ;

struct CMapItemLayerQuads
{
	CMapItemLayer m_Layer;
	int m_Version;

	int m_NumQuads;
	int m_Data;
	int m_Image;

	int m_aName[3];
} ;

struct CMapItemVersion
{
	int m_Version;
} ;

struct CEnvPoint
{
	int m_Time; // in ms
	int m_Curvetype;
	int m_aValues[4]; // 1-4 depending on envelope (22.10 fixed point)

	bool operator<(const CEnvPoint &Other) { return m_Time < Other.m_Time; }
} ;

struct CMapItemEnvelope_v1
{
	int m_Version;
	int m_Channels;
	int m_StartPoint;
	int m_NumPoints;
	int m_aName[8];
} ;

struct CMapItemEnvelope : public CMapItemEnvelope_v1
{
	enum { CURRENT_VERSION=2 };
	int m_Synchronized;
};

inline int CalculateBiome(int Temperature, int Moisture)
{
	if (Temperature < -82)
		return MAPBIOME_POLAR;
	else if (Temperature < -65)
		return MAPBIOME_TUNDRA;
	else if (Temperature <= 65)
		return MAPBIOME_PLAIRIE;
	else
	{//warm stuff
		if (Moisture < 65)
			return MAPBIOME_DESERT;
		else if (Moisture <= 100)
			return MAPBIOME_GRASSLAND;
		else if (Moisture <= 135)
			return MAPBIOME_FOREST;
		else
			return MAPBIOME_WATERLAND;
	}

	return MAPBIOME_PLAIRIE;
}

inline int ClampTemp(int Temp)
{
	return clamp(Temp, -100, 100);
}

inline int ClampMoisture(int Moisture)
{
	return clamp(Moisture, 0, 200);
}

inline int ClampEruption(int Eruption)
{
	return clamp(Eruption, 2, 8);
}

#endif
