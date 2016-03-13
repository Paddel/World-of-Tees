
#include "modules.h"

CGeneratorImage::~CGeneratorImage()
{
	//m_pEditor->Graphics()->UnloadTexture(m_TexID);
	if (m_pData)
	{
		mem_free(m_pData);
		m_pData = 0;
	}
}

void CGeneratorImage::AnalyseTileFlags()
{
	mem_zero(m_aTileFlags, sizeof(m_aTileFlags));

	if (m_External)
		return;

	int tw = m_Width / 16; // tilesizes
	int th = m_Height / 16;
	if (tw == th)
	{
		unsigned char *pPixelData = (unsigned char *)m_pData;

		int TileID = 0;
		for (int ty = 0; ty < 16; ty++)
			for (int tx = 0; tx < 16; tx++, TileID++)
			{
				bool Opaque = true;
				for (int x = 0; x < tw; x++)
					for (int y = 0; y < th; y++)
					{
						int p = (ty*tw + y)*m_Width + tx*tw + x;
						if (pPixelData[p * 4 + 3] < 250)
						{
							Opaque = false;
							break;
						}
					}

				if (Opaque)
					m_aTileFlags[TileID] |= TILEFLAG_OPAQUE;
			}
	}
}

CGenLayerQuads::CGenLayerQuads()
{
	m_Type = LAYERTYPE_QUADS;
	str_copy(m_aName, "Quads", sizeof(m_aName));
	m_Image = -1;
}

CGenLayerQuads::~CGenLayerQuads()
{

}

CQuad *CGenLayerQuads::NewQuad()
{
	CQuad *q = &m_lQuads[m_lQuads.add(CQuad())];

	q->m_PosEnv = -1;
	q->m_ColorEnv = -1;
	q->m_PosEnvOffset = 0;
	q->m_ColorEnvOffset = 0;
	int x = 0, y = 0;
	q->m_aPoints[0].x = x;
	q->m_aPoints[0].y = y;
	q->m_aPoints[1].x = x + 64;
	q->m_aPoints[1].y = y;
	q->m_aPoints[2].x = x;
	q->m_aPoints[2].y = y + 64;
	q->m_aPoints[3].x = x + 64;
	q->m_aPoints[3].y = y + 64;

	q->m_aPoints[4].x = x + 32; // pivot
	q->m_aPoints[4].y = y + 32;

	for (int i = 0; i < 5; i++)
	{
		q->m_aPoints[i].x <<= 10;
		q->m_aPoints[i].y <<= 10;
	}


	q->m_aTexcoords[0].x = 0;
	q->m_aTexcoords[0].y = 0;

	q->m_aTexcoords[1].x = 1 << 10;
	q->m_aTexcoords[1].y = 0;

	q->m_aTexcoords[2].x = 0;
	q->m_aTexcoords[2].y = 1 << 10;

	q->m_aTexcoords[3].x = 1 << 10;
	q->m_aTexcoords[3].y = 1 << 10;

	q->m_aColors[0].r = 255; q->m_aColors[0].g = 255; q->m_aColors[0].b = 255; q->m_aColors[0].a = 255;
	q->m_aColors[1].r = 255; q->m_aColors[1].g = 255; q->m_aColors[1].b = 255; q->m_aColors[1].a = 255;
	q->m_aColors[2].r = 255; q->m_aColors[2].g = 255; q->m_aColors[2].b = 255; q->m_aColors[2].a = 255;
	q->m_aColors[3].r = 255; q->m_aColors[3].g = 255; q->m_aColors[3].b = 255; q->m_aColors[3].a = 255;

	return q;
}

CGenLayerTiles::CGenLayerTiles(int w, int h)
{
	m_Type = LAYERTYPE_TILES;
	str_copy(m_aName, "Tiles", sizeof(m_aName));
	m_Width = w;
	m_Height = h;
	m_Image = -1;
	m_TexID = -1;
	m_LayerType = LAYERTILETYPE_NONE;
	m_Color.r = 255;
	m_Color.g = 255;
	m_Color.b = 255;
	m_Color.a = 255;
	m_ColorEnv = -1;
	m_ColorEnvOffset = 0;

	m_pTiles = new CTile[m_Width*m_Height];
	mem_zero(m_pTiles, m_Width*m_Height*sizeof(CTile));
}

CGenLayerTiles::~CGenLayerTiles()
{
	delete[] m_pTiles;
}

void CGenLayerTiles::PrepareForSave()
{
	for (int y = 0; y < m_Height; y++)
		for (int x = 0; x < m_Width; x++)
			m_pTiles[y*m_Width + x].m_Flags &= TILEFLAG_VFLIP | TILEFLAG_HFLIP | TILEFLAG_ROTATE;

	/*if (m_Image != -1 && m_Color.a == 255)
	{
	for (int y = 0; y < m_Height; y++)
	for (int x = 0; x < m_Width; x++)
	m_pTiles[y*m_Width + x].m_Flags |= m_pEditor->m_Map.m_lImages[m_Image]->m_aTileFlags[m_pTiles[y*m_Width + x].m_Index];
	}*/
}

CGenLayerExTiles::CGenLayerExTiles(int w, int h)
	: CGenLayerTiles(w, h)
{
	str_copy(m_aName, "Extension", sizeof(m_aName));
	m_LayerType = LAYERTILETYPE_EX;

	m_pExTiles = new CExTile[m_Width*m_Height];
	mem_zero(m_pExTiles, m_Width*m_Height*sizeof(CExTile));

	//delete [] m_pTiles;
}

CGenLayerExTiles::~CGenLayerExTiles()
{
	delete[] m_pExTiles;
}

void CGenLayerExTiles::PrepareForSave()
{
	for (int y = 0; y < m_Height; y++)
		for (int x = 0; x < m_Width; x++)
			m_pTiles[y*m_Width + x].m_Flags &= TILEFLAG_VFLIP | TILEFLAG_HFLIP | TILEFLAG_ROTATE;

	/*if (m_Image != -1 && m_Color.a == 255)
	{
	for (int y = 0; y < m_Height; y++)
	for (int x = 0; x < m_Width; x++)
	m_pTiles[y*m_Width + x].m_Flags |= m_pEditor->m_Map.m_lImages[m_Image]->m_aTileFlags[m_pTiles[y*m_Width + x].m_Index];
	}*/
}

CGenLayerGroup::CGenLayerGroup()
{
	m_OffsetX = 0;
	m_OffsetY = 0;

	m_ParallaxX = 100;
	m_ParallaxY = 100;

	m_UseClipping = 0;
	m_ClipX = 0;
	m_ClipY = 0;
	m_ClipW = 0;
	m_ClipH = 0;

	mem_zero(&m_aName, sizeof(m_aName));
}

int CDoodad::GetNumShapes(int Width)
{
	int Num = 0;
	for (int i = 0; i < m_DoodadShapes.size(); i++)
	{
		if (m_DoodadShapes[i].m_Width != Width)
			continue;

		Num++;
	}
	return Num;
}

CDoodadShape *CDoodad::GetShape(int Width, int Chosen)
{
	CDoodadShape *pShape = NULL;

	for (int i = 0; i < m_DoodadShapes.size(); i++)
	{
		if (m_DoodadShapes[i].m_Width != Width)
			continue;

		if (Chosen == 0)
		{
			pShape = &m_DoodadShapes[i];
			break;
		}
		else
			Chosen--;
	}

	return pShape;
}