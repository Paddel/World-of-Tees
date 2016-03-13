#pragma once

#include <base/tl/algorithm.h>
#include <game/mapitems.h>

#include "auto_map.h"

struct CPrevMapInfo
{
	int m_TransitionID, m_Eruption, m_Temperature, m_Moisture;
	char m_aName[256];
	CPrevMapInfo(char *pName, int TransitionID, int Eruption, int Temperature, int Moisture)
		: m_TransitionID(TransitionID), m_Eruption(Eruption), m_Temperature(Temperature), m_Moisture(Moisture)
	{
		str_copy(m_aName, pName, sizeof(m_aName));
	}

	CPrevMapInfo() { CPrevMapInfo("", 0, 2, 0, 100); }
};

class CGenImageInfo
{
public:
	enum
	{
		FORMAT_AUTO = -1,
		FORMAT_RGB = 0,
		FORMAT_RGBA = 1,
		FORMAT_ALPHA = 2,
	};
	int m_Width;
	int m_Height;
	int m_Format;
	void *m_pData;
};

class CGenEnvelope
{
public:
	int m_Channels;
	array<CEnvPoint> m_lPoints;
	char m_aName[32];
	float m_Bottom, m_Top;
	bool m_Synchronized;

	CGenEnvelope(int Chan)
	{
		m_Channels = Chan;
		m_aName[0] = 0;
		m_Bottom = 0;
		m_Top = 0;
		m_Synchronized = true;
	}

	void Resort()
	{
		sort(m_lPoints.all());
		FindTopBottom(0xf);
	}

	void FindTopBottom(int ChannelMask)
	{
		m_Top = -1000000000.0f;
		m_Bottom = 1000000000.0f;
		for (int i = 0; i < m_lPoints.size(); i++)
		{
			for (int c = 0; c < m_Channels; c++)
			{
				if (ChannelMask&(1 << c))
				{
					float v = fx2f(m_lPoints[i].m_aValues[c]);
					if (v > m_Top) m_Top = v;
					if (v < m_Bottom) m_Bottom = v;
				}
			}
		}
	}

	void AddPoint(int Time, int v0, int v1 = 0, int v2 = 0, int v3 = 0)
	{
		CEnvPoint p;
		p.m_Time = Time;
		p.m_aValues[0] = v0;
		p.m_aValues[1] = v1;
		p.m_aValues[2] = v2;
		p.m_aValues[3] = v3;
		p.m_Curvetype = CURVETYPE_LINEAR;
		m_lPoints.add(p);
		Resort();
	}

	float EndTime()
	{
		if (m_lPoints.size())
			return m_lPoints[m_lPoints.size() - 1].m_Time*(1.0f / 1000.0f);
		return 0;
	}
};

class CGeneratorImage : public CGenImageInfo
{
public:

	CGeneratorImage(IStorage *pStorage)
		: m_AutoMapper(pStorage)
	{
		m_TexID = -1;
		m_aName[0] = 0;
		m_External = 0;
		m_Width = 0;
		m_Height = 0;
		m_pData = 0;
		m_Format = 0;
	}

	~CGeneratorImage();

	void AnalyseTileFlags();

	int m_TexID;
	int m_External;
	char m_aName[128];
	unsigned char m_aTileFlags[256];
	class CGenAutoMapper m_AutoMapper;
};


class CGenLayer
{
public:

	CGenLayer()
	{
		m_Type = LAYERTYPE_INVALID;
		str_copy(m_aName, "(invalid)", sizeof(m_aName));
		m_Visible = true;
		m_Readonly = false;
		m_SaveToMap = true;
		m_Flags = 0;
	}

	virtual ~CGenLayer()
	{
	}

	char m_aName[12];
	int m_Type;
	int m_Flags;

	bool m_Readonly;
	bool m_Visible;
	bool m_SaveToMap;
};

class CGenLayerQuads : public CGenLayer
{
public:
	CGenLayerQuads();
	~CGenLayerQuads();

	CQuad *NewQuad();

	int m_Image;
	array<CQuad> m_lQuads;
};

class CGenLayerTiles : public CGenLayer
{
public:
	CGenLayerTiles(int w, int h);
	~CGenLayerTiles();

	void PrepareForSave();

	int m_TexID;
	int m_LayerType;
	int m_Image;
	int m_Width;
	int m_Height;
	CColor m_Color;
	int m_ColorEnv;
	int m_ColorEnvOffset;
	CTile *m_pTiles;
};

class CGenLayerExTiles : public CGenLayerTiles
{
public:
	CGenLayerExTiles(int w, int h);
	~CGenLayerExTiles();

	virtual void PrepareForSave();

	CExTile *m_pExTiles;
};

class CGenLayerGroup
{
public:
	CGenLayerGroup();
	array<CGenLayer*> m_lLayers;

	int m_OffsetX;
	int m_OffsetY;

	int m_ParallaxX;
	int m_ParallaxY;

	int m_UseClipping;
	int m_ClipX;
	int m_ClipY;
	int m_ClipW;
	int m_ClipH;

	char m_aName[12];
};

struct CGeneratingMap
{
	CPrevMapInfo m_PrevMapInfo;
	char m_aFileName[64];
	int m_MapType;

	int m_Temperature;
	int m_Moisture;
	int m_Eruption;
	int m_MapBiome;

	int m_TransitionCount;
	int m_GenMapCount;
	int m_ChestIDCount;
	array<CGenLayerGroup*> m_lGroups;
	array<CGenEnvelope*> m_lEnvelopes;
	array<CGeneratorImage*> m_lImages;
};

struct CDoodadShape
{
	int m_Height;
	int m_Width;
	int m_Index;
};

class CDoodad
{
public:
	array<CDoodadShape> m_DoodadShapes;
	char m_aName[128];
	int m_Variation;

	int GetNumShapes(int Width);
	CDoodadShape *GetShape(int Width, int Chosen);
};

struct CMinimalisticShape
{
	char m_aName[128];
	int m_Start;
	int m_Tile;
	int m_Space;
};