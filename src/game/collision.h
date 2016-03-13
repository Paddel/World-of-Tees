/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>
#include <base/math.h>
#include <base/tl/array.h>
#include <game/mapitems.h>
#include <game/collaser.h>

#define COLLLASER_DIST 1024

class CCollision
{
	array<CTile> m_pTiles;
	int m_Width;
	int m_Height;

	array<CTile> m_pExTiles;
	array<CExTile> m_pExArgs;
	int m_ExWidth;
	int m_ExHeight;
	array<int> m_lExtraCollisions;
	class CLayers *m_pLayers;
	array<CColLaser *> m_pColLasers;

	bool IsTileSolid(int x, int y);
	int GetTile(int x, int y);

public:

	CCollision();
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y) { return IsTileSolid(round(x), round(y)); }
	bool CheckPoint(vec2 Pos) { return CheckPoint(Pos.x, Pos.y); }
	int GetCollisionAt(float x, float y) { return GetTile(round(x), round(y)); }
	int GetCollisionAt(vec2 Pos) { return GetCollisionAt(Pos.x, Pos.y); }
	int GetWidth() { return m_Width; };
	int GetHeight() { return m_Height; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	int IntersectBox(vec2 Pos0, vec2 Pos1, int Size, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces);
	bool MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity, CColLaser *pNotThis = NULL);
	void MoveBoxPredict(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity);
	bool TestBox(vec2 Pos, vec2 Size);

	void SetExtraCollision(int ExtraCollision) { m_lExtraCollisions.add(ExtraCollision); }
	void ResetExtraCollision() { m_lExtraCollisions.clear(); }

	int GetExTile(int x, int y);
	const char *GetExArgs(int x, int y);

	void AddColLaser(CColLaser *pLaser);
	void RemoveColLaser(CColLaser *pLaser);
};

#endif
