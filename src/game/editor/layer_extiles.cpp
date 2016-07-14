/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <engine/client.h>

#include "editor.h"


CLayerExTiles::CLayerExTiles(int w, int h)
: CLayerTiles(w, h)
{
	str_copy(m_aName, "Extension", sizeof(m_aName));
	m_LayerType = LAYERTILETYPE_EX;

	m_pExTiles = new CExTile[m_Width*m_Height];
	mem_zero(m_pExTiles, m_Width*m_Height*sizeof(CExTile));

	//delete [] m_pTiles;
}

CLayerExTiles::~CLayerExTiles()
{
	int Type = GetTileType();
	if(Type != Type);//?
		return;

	delete [] m_pExTiles;
}

void CLayerExTiles::PrepareForSave()
{
	for(int y = 0; y < m_Height; y++)
		for(int x = 0; x < m_Width; x++)
			m_pTiles[y*m_Width+x].m_Flags &= TILEFLAG_VFLIP|TILEFLAG_HFLIP|TILEFLAG_ROTATE;

	if(m_Image != -1 && m_Color.a == 255)
	{
		for(int y = 0; y < m_Height; y++)
			for(int x = 0; x < m_Width; x++)
				m_pTiles[y*m_Width+x].m_Flags |= m_pEditor->m_Map.m_lImages[m_Image]->m_aTileFlags[m_pTiles[y*m_Width+x].m_Index];
	}
}

void CLayerExTiles::Render()
{
	if(m_Image >= 0 && m_Image < m_pEditor->m_Map.m_lImages.size())
		m_TexID = m_pEditor->m_Map.m_lImages[m_Image]->m_TexID;

	Graphics()->TextureSet(m_TexID);
	vec4 Color = vec4(m_Color.r/255.0f, m_Color.g/255.0f, m_Color.b/255.0f, m_Color.a/255.0f);
	Graphics()->BlendNone();
	m_pEditor->RenderTools()->RenderTilemap(m_pTiles, m_Width, m_Height, 32.0f, Color, LAYERRENDERFLAG_OPAQUE,
												m_pEditor->EnvelopeEval, m_pEditor, m_ColorEnv, m_ColorEnvOffset);
	Graphics()->BlendNormal();
	m_pEditor->RenderTools()->RenderTilemap(m_pTiles, m_Width, m_Height, 32.0f, Color, LAYERRENDERFLAG_TRANSPARENT,
												m_pEditor->EnvelopeEval, m_pEditor, m_ColorEnv, m_ColorEnvOffset);
}


int CLayerExTiles::BrushGrab(CLayerGroup *pBrush, CUIRect Rect)
{
	RECTi r;
	Convert(Rect, &r);
	Clamp(&r);

	if(!r.w || !r.h)
		return 0;

	// create new layers
	CLayerExTiles *pGrabbed = new CLayerExTiles(r.w, r.h);
	pGrabbed->m_pEditor = m_pEditor;
	pGrabbed->m_TexID = m_TexID;
	pGrabbed->m_Image = m_Image;
	pGrabbed->m_LayerType = m_LayerType;
	pBrush->AddLayer(pGrabbed);

	// copy the tiles
	for(int y = 0; y < r.h; y++)
		for(int x = 0; x < r.w; x++)
		{
			pGrabbed->m_pTiles[y*pGrabbed->m_Width+x] = m_pTiles[(r.y+y)*m_Width+(r.x+x)];
			pGrabbed->m_pExTiles[y*pGrabbed->m_Width+x] = pGrabbed->GetTileType() == LAYERTILETYPE_EX? m_pExTiles[(r.y+y)*m_Width+(r.x+x)] : CExTile();
		}

	return 1;
}

void CLayerExTiles::FillSelection(bool Empty, CLayer *pBrush, CUIRect Rect)
{
	if(m_Readonly)
		return;

	Snap(&Rect);

	int sx = ConvertX(Rect.x);
	int sy = ConvertY(Rect.y);
	int w = ConvertX(Rect.w);
	int h = ConvertY(Rect.h);

	CLayerExTiles *pLt = static_cast<CLayerExTiles*>(pBrush);

	for(int y = 0; y < h; y++)
	{
		for(int x = 0; x < w; x++)
		{
			int fx = x+sx;
			int fy = y+sy;

			if(fx < 0 || fx >= m_Width || fy < 0 || fy >= m_Height)
				continue;

			if(Empty)
				m_pTiles[fy*m_Width+fx].m_Index = 1;
			else
			{
				m_pTiles[fy*m_Width+fx] = pLt->m_pTiles[(y*pLt->m_Width + x%pLt->m_Width) % (pLt->m_Width*pLt->m_Height)];
				m_pExTiles[fy*m_Width+fx] = pLt->GetTileType() == LAYERTILETYPE_EX? pLt->m_pExTiles[(y*pLt->m_Width + x%pLt->m_Width) % (pLt->m_Width*pLt->m_Height)] : CExTile();
			}
		}
	}
	m_pEditor->m_Map.m_Modified = true;
}

void CLayerExTiles::BrushDraw(CLayer *pBrush, float wx, float wy)
{
	if(m_Readonly)
		return;

	//
	CLayerExTiles *l = (CLayerExTiles *)pBrush;
	int sx = ConvertX(wx);
	int sy = ConvertY(wy);

	for(int y = 0; y < l->m_Height; y++)
		for(int x = 0; x < l->m_Width; x++)
		{
			int fx = x+sx;
			int fy = y+sy;
			if(fx<0 || fx >= m_Width || fy < 0 || fy >= m_Height)
				continue;

			m_pTiles[fy*m_Width+fx] = l->m_pTiles[y*l->m_Width+x];

			if(pBrush->GetTileType() == LAYERTILETYPE_EX)
				m_pExTiles[fy*m_Width+fx] = l->m_pExTiles[y*l->m_Width+x];
			else
			{
				CExTile NewExTile = CExTile();

				int Tile = fy*m_Width+fx;
				if(Tile == -1)//outa map
					return;

				int Index = m_pEditor->m_Map.m_pExLayer->GetIndex(Tile);

				int ExtInpID = -1;
				for(int i = 0; i < m_pEditor->m_lpExtendInput.size(); i++)
				{
					if(m_pEditor->m_lpExtendInput[i]->m_Index == Index)
					{
						ExtInpID = i;
						break;
					}
				}

				if(ExtInpID != -1)
				{
					CExtendInput *pExtInp = m_pEditor->m_lpExtendInput[ExtInpID];
					int Size = pExtInp->m_lpInputArgs.size();
					for(int i = 0; i < Size; i++)
					{
						char Indicator = 0;
						char aVal[MAX_EXTENTED_STR];
						if(pExtInp->m_lpInputArgs[i]->m_Type == EXTINPTYPE_STR)
						{
							Indicator = 's';
							str_format(aVal, sizeof(aVal), "%s", ((CExtendInputStr *)pExtInp->m_lpInputArgs[i])->m_aVal);
						}
						else if(pExtInp->m_lpInputArgs[i]->m_Type == EXTINPTYPE_INT)
						{
							Indicator = 'i';
							str_format(aVal, sizeof(aVal), "%i", ((CExtendInputInt *)pExtInp->m_lpInputArgs[i])->m_Val);
						}


						str_fcat(NewExTile.m_ExArgs, sizeof(NewExTile.m_ExArgs), "%c%c%s%s", Indicator, 0xff, aVal, i==Size-1?"":"\xff");
					}
				}

				m_pExTiles[fy*m_Width+fx] = NewExTile;
				
			}
		}
	m_pEditor->m_Map.m_Modified = true;
}

void CLayerExTiles::BrushFlipX()
{
	for(int y = 0; y < m_Height; y++)
		for(int x = 0; x < m_Width/2; x++)
		{
			CTile Tmp = m_pTiles[y*m_Width+x];
			m_pTiles[y*m_Width+x] = m_pTiles[y*m_Width+m_Width-1-x];
			m_pTiles[y*m_Width+m_Width-1-x] = Tmp;
			
			//if(GetTileType() == LAYERTILETYPE_EX)
			{
				CExTile ExTmp = m_pExTiles[y*m_Width+x];
				m_pExTiles[y*m_Width+x] = m_pExTiles[y*m_Width+m_Width-1-x];
				m_pExTiles[y*m_Width+m_Width-1-x] = ExTmp;
			}
		}

}

void CLayerExTiles::BrushFlipY()
{
	for(int y = 0; y < m_Height/2; y++)
		for(int x = 0; x < m_Width; x++)
		{
			CTile Tmp = m_pTiles[y*m_Width+x];
			m_pTiles[y*m_Width+x] = m_pTiles[(m_Height-1-y)*m_Width+x];
			m_pTiles[(m_Height-1-y)*m_Width+x] = Tmp;

			//if(GetTileType() == LAYERTILETYPE_EX)
			{
				CExTile ExTmp = m_pExTiles[y*m_Width+x];
				m_pExTiles[y*m_Width+x] = m_pExTiles[(m_Height-1-y)*m_Width+x];
				m_pExTiles[(m_Height-1-y)*m_Width+x] = ExTmp;
			}
		}
}

void CLayerExTiles::BrushRotate(float Amount)
{
	int Rotation = (round(360.0f*Amount/(pi*2))/90)%4;	// 0=0°, 1=90°, 2=180°, 3=270°
	if(Rotation < 0)
		Rotation +=4;

	if(Rotation == 1 || Rotation == 3)
	{
		// 90° rotation
		CTile *pTempData = new CTile[m_Width*m_Height];
		CExTile *pExTempData = new CExTile[m_Width*m_Height];
		mem_copy(pTempData, m_pTiles, m_Width*m_Height*sizeof(CTile));
		mem_copy(pExTempData, m_pExTiles, m_Width*m_Height*sizeof(CExTile));
		CTile *pDst = m_pTiles;
		CExTile *pExDst = m_pExTiles;
		for(int x = 0; x < m_Width; ++x)
			for(int y = m_Height-1; y >= 0; --y, ++pDst, ++pExDst)
			{
				*pDst = pTempData[y*m_Width+x];
				*pExDst = pExTempData[y*m_Width+x];
			}

		int Temp = m_Width;
		m_Width = m_Height;
		m_Height = Temp;
		delete[] pTempData;
		delete[] pExTempData;
	}

	if(Rotation == 2 || Rotation == 3)
	{
		BrushFlipX();
		BrushFlipY();
	}
}

void CLayerExTiles::Resize(int NewW, int NewH)
{
	CTile *pNewData = new CTile[NewW*NewH];
	CExTile *pNewExData = new CExTile[NewW*NewH];
	mem_zero(pNewData, NewW*NewH*sizeof(CTile));
	mem_zero(pNewExData, NewW*NewH*sizeof(CExTile));

	// copy old data
	for(int y = 0; y < min(NewH, m_Height); y++)
	{
		mem_copy(&pNewData[y*NewW], &m_pTiles[y*m_Width], min(m_Width, NewW)*sizeof(CTile));
		mem_copy(&pNewExData[y*NewW], &m_pExTiles[y*m_Width], min(m_Width, NewW)*sizeof(CExTile));
	}

	// replace old
	delete [] m_pTiles;
	delete [] m_pExTiles;
	m_pTiles = pNewData;
	m_pExTiles = pNewExData;
	m_Width = NewW;
	m_Height = NewH;
}

void CLayerExTiles::Shift(int Direction)
{
	switch(Direction)
	{
	case 1:
		{
			// left
			for(int y = 0; y < m_Height; ++y)
			{
				mem_move(&m_pTiles[y*m_Width], &m_pTiles[y*m_Width+1], (m_Width-1)*sizeof(CTile));
				mem_move(&m_pExTiles[y*m_Width], &m_pExTiles[y*m_Width+1], (m_Width-1)*sizeof(CExTile));
			}
		}
		break;
	case 2:
		{
			// right
			for(int y = 0; y < m_Height; ++y)
			{
				mem_move(&m_pTiles[y*m_Width+1], &m_pTiles[y*m_Width], (m_Width-1)*sizeof(CTile));
				mem_move(&m_pExTiles[y*m_Width+1], &m_pExTiles[y*m_Width], (m_Width-1)*sizeof(CExTile));
			}
		}
		break;
	case 4:
		{
			// up
			for(int y = 0; y < m_Height-1; ++y)
			{
				mem_copy(&m_pTiles[y*m_Width], &m_pTiles[(y+1)*m_Width], m_Width*sizeof(CTile));
				mem_copy(&m_pExTiles[y*m_Width], &m_pExTiles[(y+1)*m_Width], m_Width*sizeof(CExTile));
			}
		}
		break;
	case 8:
		{
			// down
			for(int y = m_Height-1; y > 0; --y)
			{
				mem_copy(&m_pTiles[y*m_Width], &m_pTiles[(y-1)*m_Width], m_Width*sizeof(CTile));
				mem_copy(&m_pExTiles[y*m_Width], &m_pExTiles[(y-1)*m_Width], m_Width*sizeof(CExTile));
			}
		}
	}
}

void CLayerExTiles::ShowInfo()
{
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	Graphics()->TextureSet(m_pEditor->Client()->GetDebugFont());
	Graphics()->QuadsBegin();

	int StartY = max(0, (int)(ScreenY0/32.0f)-1);
	int StartX = max(0, (int)(ScreenX0/32.0f)-1);
	int EndY = min((int)(ScreenY1/32.0f)+1, m_Height);
	int EndX = min((int)(ScreenX1/32.0f)+1, m_Width);

	for(int y = StartY; y < EndY; y++)
		for(int x = StartX; x < EndX; x++)
		{
			int c = x + y*m_Width;
			if(m_pTiles[c].m_Index)
			{
				char aBuf[64];
				str_format(aBuf, sizeof(aBuf), "%i %s", m_pTiles[c].m_Index, m_pExTiles[c].m_ExArgs);

				m_pEditor->Graphics()->QuadsText(x*32, y*32, 16.0f, aBuf);

				char aFlags[4] = {	m_pTiles[c].m_Flags&TILEFLAG_VFLIP ? 'V' : ' ',
									m_pTiles[c].m_Flags&TILEFLAG_HFLIP ? 'H' : ' ',
									m_pTiles[c].m_Flags&TILEFLAG_ROTATE? 'R' : ' ',
									0};
				m_pEditor->Graphics()->QuadsText(x*32, y*32+16, 16.0f, aFlags);
			}
			x += m_pTiles[c].m_Skip;
		}

	Graphics()->QuadsEnd();
	Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
}

int CLayerExTiles::RenderProperties(CUIRect *pToolBox)
{
	int r = CLayerTiles::RenderProperties(pToolBox);
	m_Image = -1;
	return r;
}