#pragma once

#include <base/tl/array.h>
#include <game/server/entities/character.h>

class CGameContext;
class IServer;

class CQuestion
{
private:
	void *m_pOwner;
	IServer *m_pServer;
	char m_aText[128];
	char m_aReason[64];
	int m_Time;
	int m_Type;

public:
	CQuestion(void *pOwner, int Type, pClientFunc pYesFunc, pClientFunc pNoFunc, pClientFunc pNoAnswerFunc, int Time, char *pText, char *pReason);

	pClientFunc m_pYesFunc;
	pClientFunc m_pNoFunc;
	pClientFunc m_pNoAnswerFunc;

	void *GetOwner() const { return m_pOwner; }
	int GetTime() const { return m_Time; }
	int GetType() const { return m_Type; }
	char *GetText() { return m_aText; }
	char *GetReason() { return m_aReason; }

	void ReduceTime() { m_Time--; }
};

class CQuestionHandler
{
public:
	enum
	{
		TYPE_SELL=0,
		TYPE_WEAPONPICKUP,
		NUM_TYPES,
	};
private:
	array <CQuestion *> m_pQuestions;
	CGameContext *m_pGameServer;
	IServer *m_pServer;
	int64 m_TimeCounter;
	int m_Owner;

public:
	CQuestionHandler(CGameContext *pGameServer, int Owner);

	void Ask(void *pOwner, int Type, pClientFunc pYesFunc, pClientFunc pNoFunc, char *pText, char *pReason);
	void Ask(void *pOwner, int Type, pClientFunc pYesFunc, pClientFunc pNoFunc, pClientFunc pNoAnswerFunc, int Time, char *pText, char *pReason);

	void AnswerYes();
	void AnswerNo();
	void CloseQuestion();
	void Update();
	void EndQuestion(int Type);

	void Tick();

	int GetQuestionNum() { return m_pQuestions.size(); }

	CGameContext *GameServer() const {return m_pGameServer;}
	IServer *Server() const {return m_pServer;}

	int m_Last;
};