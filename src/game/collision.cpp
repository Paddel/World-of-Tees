/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <math.h>
#include <engine/map.h>
#include <engine/kernel.h>

#include <game/mapitems.h>
#include <game/layers.h>
#include <game/collision.h>

CCollision::CCollision()
{
	m_pTiles.clear();
	m_pExTiles.clear();
	m_pExArgs.clear();
	m_Width = 0;
	m_Height = 0;
	m_ExWidth = 0;
	m_ExHeight = 0;
	m_pLayers = 0;
}

void CCollision::Init(class CLayers *pLayers)
{
	m_pTiles.clear();
	m_pExTiles.clear();
	m_pExArgs.clear();

	m_pLayers = pLayers;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	CTile *pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));
	for(int i = 0; i < m_Width*m_Height; i++)
		m_pTiles.add(pTiles[i]);

	if(m_pLayers->m_ExTileLayerUsed)
	{
		m_ExWidth = m_pLayers->ExTileLayer()->m_Width;
		m_ExHeight = m_pLayers->ExTileLayer()->m_Height;
		CTile *pExTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->ExTileLayer()->m_Data));
		CExTile *pExArgs = static_cast<CExTile *>(m_pLayers->Map()->GetData(m_pLayers->ExTileLayer()->m_ExData));
		for(int i = 0; i < m_ExWidth*m_ExHeight; i++)
		{
			m_pExTiles.add(pExTiles[i]);
			m_pExArgs.add(pExArgs[i]);
		}
	}

	ResetExtraCollision();
}

int CCollision::GetTile(int x, int y)
{
	int Nx = clamp(x/32, 0, m_Width-1);
	int Ny = clamp(y/32, 0, m_Height-1);

	return m_pTiles[Ny*m_Width+Nx].m_Index > 128 ? 0 : m_pTiles[Ny*m_Width+Nx].m_Index;
}

int CCollision::GetExTile(int x, int y)
{
	if(!m_pLayers->m_ExTileLayerUsed)
		return EXTILE_NONE;

	vec2 Pos = vec2(x/32, y/32);
	if(Pos.x < 0 || Pos.y < 0 || Pos.x > m_ExWidth || Pos.y > m_ExHeight)
		return EXTILE_NONE;

	return m_pExTiles[Pos.y*m_ExWidth+Pos.x].m_Index;
}

const char *CCollision::GetExArgs(int x, int y)
{
	if(!m_pLayers->m_ExTileLayerUsed)
		return "";

	vec2 Pos = vec2(x/32, y/32);
	if(Pos.x < 0 || Pos.y < 0 || Pos.x > m_ExWidth || Pos.y > m_ExHeight)
		return "";

	int Tile = Pos.y*m_ExWidth+Pos.x;

	if(m_pExTiles[Tile].m_Index == EXTILE_NONE)
		return "";

	return m_pExArgs[Tile].m_ExArgs;
}

bool CCollision::IsTileSolid(int x, int y)
{
	int Index = GetTile(x, y);

	for(int i = 0; i < m_lExtraCollisions.size(); i++)
	{
		if(Index == m_lExtraCollisions[i])
			return true;
	}

	return Index == TILE_SOLID | Index == TILE_NOHOOK;
}

// TODO: rewrite this smarter!
int CCollision::IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision)
{
	float Distance = distance(Pos0, Pos1);
	int End(Distance+1);
	vec2 Last = Pos0;

	for(int i = 0; i < End; i++)
	{
		float a = i/Distance;
		vec2 Pos = mix(Pos0, Pos1, a);
		if(CheckPoint(Pos.x, Pos.y))
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return GetCollisionAt(Pos.x, Pos.y);
		}
		Last = Pos;
	}
	if(pOutCollision)
		*pOutCollision = Pos1;
	if(pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

int CCollision::IntersectBox(vec2 Pos0, vec2 Pos1, int Size, vec2 *pOutCollision, vec2 *pOutBeforeCollision)
{
	float Distance = distance(Pos0, Pos1);
	int End(Distance+1);
	vec2 Last = Pos0;

	for(int i = 0; i < End; i++)
	{
		float a = i/Distance;
		vec2 Pos = mix(Pos0, Pos1, a);
		if(TestBox(Pos, vec2(Size, Size)))
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return GetCollisionAt(Pos.x, Pos.y);
		}
		Last = Pos;
	}
	if(pOutCollision)
		*pOutCollision = Pos1;
	if(pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

// TODO: OPT: rewrite this smarter!
void CCollision::MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces)
{
	if(pBounces)
		*pBounces = 0;

	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;
	if(CheckPoint(Pos + Vel))
	{
		int Affected = 0;
		if(CheckPoint(Pos.x + Vel.x, Pos.y))
		{
			pInoutVel->x *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(CheckPoint(Pos.x, Pos.y + Vel.y))
		{
			pInoutVel->y *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(Affected == 0)
		{
			pInoutVel->x *= -Elasticity;
			pInoutVel->y *= -Elasticity;
		}
	}
	else
	{
		*pInoutPos = Pos + Vel;
	}
}

bool CCollision::TestBox(vec2 Pos, vec2 Size)
{
	Size *= 0.5f;
	if(CheckPoint(Pos.x-Size.x, Pos.y-Size.y))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y-Size.y))
		return true;
	if(CheckPoint(Pos.x-Size.x, Pos.y+Size.y))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y+Size.y))
		return true;
	return false;
}

bool CCollision::MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity, CColLaser *pNotThis)
{
	bool HitWall = false;
	// do the move
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;

	float Distance = length(Vel);
	int Max = (int)Distance;

	if(Distance > 0.00001f)
	{
		//vec2 old_pos = pos;
		float Fraction = 1.0f/(float)(Max+1);
		for(int i = 0; i <= Max; i++)
		{
			//float amount = i/(float)max;
			//if(max == 0)
				//amount = 0;

			vec2 NewPos = Pos + Vel*Fraction; // TODO: this row is not nice

			for(int i = 0; i < m_pColLasers.size(); i++)
			{
				CColLaser *pLaser = m_pColLasers[i];

				if(pNotThis == pLaser || pLaser->Active() == false)
					continue;

				vec2 CheckPos = pLaser->From()-NewPos;
				if(abs(CheckPos.x) > COLLLASER_DIST || abs(CheckPos.y) > COLLLASER_DIST)//if one coordinate is over range
					continue;

				float dist = distance(pLaser->From(), NewPos);
				if(dist > COLLLASER_DIST)
					continue;

				vec2 ClosestPoint = closest_point_on_line(pLaser->From(), pLaser->To(), NewPos);
				float Boxdist = distance(ClosestPoint, NewPos);
				float MaxDist = (Size.x+Size.y)*0.5f;
				if(Boxdist < MaxDist)
				{
					Vel += normalize(NewPos-ClosestPoint)*(MaxDist-Boxdist);
					NewPos = Pos + Vel*Fraction;
					HitWall = true;
				}

			}

			if(TestBox(vec2(NewPos.x, NewPos.y), Size))
			{
				int Hits = 0;

				if(TestBox(vec2(Pos.x, NewPos.y), Size))
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					Hits++;
					HitWall = true;
				}

				if(TestBox(vec2(NewPos.x, Pos.y), Size))
				{
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
					Hits++;
					HitWall = true;
				}

				// neither of the tests got a collision.
				// this is a real _corner case_!
				if(Hits == 0)
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
				}
			}

			Pos = NewPos;
		}
	}

	//water
	if(GetCollisionAt(Pos) == TILE_WATER)
		Vel = vec2(Vel.x*0.6f, Vel.y*0.9f);

	*pInoutPos = Pos;
	*pInoutVel = Vel;
	return HitWall;
}

void CCollision::MoveBoxPredict(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity)
{
	// do the move
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;

	float Distance = length(Vel);
	int Max = (int)Distance;

	if(Distance > 0.00001f)
	{
		//vec2 old_pos = pos;
		float Fraction = 1.0f/(float)(Max+1);
		for(int i = 0; i <= Max; i++)
		{
			//float amount = i/(float)max;
			//if(max == 0)
				//amount = 0;

			vec2 NewPos = Pos + Vel*Fraction; // TODO: this row is not nice

			if(TestBox(vec2(NewPos.x, NewPos.y), Size))
			{
				int Hits = 0;

				if(TestBox(vec2(Pos.x, NewPos.y), Size))
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					Hits++;
				}

				if(TestBox(vec2(NewPos.x, Pos.y), Size))
				{
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
					Hits++;
				}

				// neither of the tests got a collision.
				// this is a real _corner case_!
				if(Hits == 0)
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
				}
			}

			Pos = NewPos;
		}
	}

	//water
	if(GetCollisionAt(Pos) == TILE_WATER)
		Vel = vec2(Vel.x*0.6f, Vel.y*0.9f);

	*pInoutPos = Pos;
	*pInoutVel = Vel;
}


void CCollision::AddColLaser(CColLaser *pLaser)
{
	for(int i = 0; i < m_pColLasers.size(); i++)
	{
		if(m_pColLasers[i] == pLaser)
			return;
	}

	m_pColLasers.add(pLaser);
}

void CCollision::RemoveColLaser(CColLaser *pLaser)
{
	for(int i = 0; i < m_pColLasers.size(); i++)
	{
		if(m_pColLasers[i] == pLaser)
		{
			m_pColLasers.remove_index(i);
			i--;
		}
	}
}