#pragma once

#include <game/server/npc.h>

class CGameContext;
class CMap;

class CPuzzleTee : public CNpc
{
private:

protected:
	vec2 m_SpawnPos;
	int m_PuzzleType;
	int m_ID;

	int SkinColorBody();

public:
	CPuzzleTee(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos, int PuzzleType, int ID);

	void Reset();

	int GetPuzzleType() const { return m_PuzzleType; }
	int IsRight();

	virtual void Tick();
	virtual void Snap(int SnappingClient);

	virtual void SubSpawn();
	virtual bool Alive() { return true; }

	virtual void OnNotActive();

	//overwriteable functions
	virtual void SetExtraCollision();
};