#pragma once

#include <base/vmath.h>
#include <game/server/srvgamecore.h>

class CGameContext;
class CMap;

enum
{
	INTERFLAG_COL = 1,
	INTERFLAG_HOOK = 2,
};

class CPlayerItem
{
public:
	enum
	{
		TYPE_PLAYER = 0,
		TYPE_SELCHAR,
		TYPE_NPC,

		NUM_TYPES,
	};

private:
	CGameContext *m_pGameServer;
	int m_InterFlags;
	int m_Type;
	bool m_Inited;

	//CSrvCharacterCore m_Core;

public:
	CPlayerItem();

	CNetObj_PlayerInput m_Input;

	void Init(CGameContext *pGameServer, CMap *pMap, vec2 Pos, int Type, int Flags = 0);
	void UpdateCore(float Slow);
	void ResetInput();

	virtual vec2 GetPos() {return GetCore()->m_Pos;}
	int GetInterFlags() const {return m_InterFlags;}
	int GetType() const { return m_Type; }
	virtual CMap *GetMap() = 0;
	virtual bool Spectating() { return false; }
	virtual CSrvCharacterCore *GetCore() = 0;

	virtual void Tick() {}
	virtual void PostTick() {}
	virtual void Snap(int SnappingClient) {}

	CGameContext *GameServer() const {return m_pGameServer;}
};