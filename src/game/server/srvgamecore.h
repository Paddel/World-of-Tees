#pragma once

#include <base/system.h>
#include <base/math.h>

#include <math.h>
#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>
#include <game/collision.h>
#include <game/gamecore.h>

class CGameContext;
class CMap;

class CSrvWorldCore
{
public:
	CSrvWorldCore()
	{
		mem_zero(m_apCharacters, sizeof(m_apCharacters));
	}

	CTuningParams m_Tuning;
	class CSrvCharacterCore *m_apCharacters[MAX_CLIENTS];
};

class CSrvCharacterCore
{
	CSrvWorldCore *m_pWorld;
	CCollision *m_pCollision;
	CGameContext *m_pGameServer;
	CMap *m_pMap;

public:
	CMap *Map() const { return m_pMap; }

	vec2 m_Pos;
	vec2 m_Vel;

	vec2 m_HookPos;
	vec2 m_HookDir;
	int m_HookTick;
	int m_HookState;
	int m_HookedPlayer;
	bool m_BotHooked;

	bool m_Colliding;
	bool m_InWater;
	bool m_OnIce;

	int m_ForceDirection;

	int m_Jumped;

	float m_Speed;

	int m_Direction;
	int m_Angle;
	CNetObj_PlayerInput m_Input;

	int m_TriggeredEvents;

	void Init(CSrvWorldCore *pWorld, CCollision *pCollision, CGameContext *pGameServer, CMap *pMap);
	void Reset();
	void Tick(bool UseInput, bool UseCol = true);
	void Move(float Slow);

	void Read(const CNetObj_CharacterCore *pObjCore);
	void Write(CNetObj_CharacterCore *pObjCore);
	void Quantize();

	void PredictMove();
	void PredictTick(bool UseInput, bool UseCol = true);
};