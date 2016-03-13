#pragma once

#include <base/tl/array.h>
#include <game/server/entities/character.h>

#define MAX_DECISIONS 10
#define MAX_TEXT_LENGTH 512

class CGameContext;
class IServer;

struct CDecision
{
	pClientFunc m_Func;
	int m_Line;

	CDecision()
	{
		m_Line = -1;
		m_Func = 0;
	}
};

class CPage
{
private:
	char m_aText[MAX_TEXT_LENGTH];
	int m_LineSkips;
	int m_NumDecisions;
	void *m_pInfo;

public:
	CPage(char *pText, int LineSkips, void *pInfo, CDecision aDecision[MAX_DECISIONS]);

	int m_CurrentDecision;
	CDecision m_aDecision[MAX_DECISIONS];
	
	int NumDecisions() { return m_NumDecisions; }
	void ResetDecisions() { m_NumDecisions = 0; }
	void *GetInfo() { return m_pInfo; }
	char *Text() { return m_aText; }
	int LineSkips() { return m_LineSkips; }
};

class CTextPopup
{
private:
	CGameContext *m_pGameServer;
	IServer *m_pServer;
	array <CPage *> m_pPages;
	int64 m_PopupTime;
	int64 m_AppearTime;
	int m_ClientID;
	int m_CurrentPage;

	void SendPopup();
	void CreatePopupText(char *pStr, int Size, CPage *pPage);

public:
	CTextPopup(CGameContext *pGameServer, int ClientID);

	void Tick();
	void AddText(char *pText, int LineSkips, void *pInfo, CDecision m_aDecision[MAX_DECISIONS]);
	void OnLeftClick();
	void OnRightClick();
	void OnJump();
	void OnMoveRight();
	void OnMoveLeft();
	void Clear();

	bool Active();

	CPage *CurrentPage();

	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }
};