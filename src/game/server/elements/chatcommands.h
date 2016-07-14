#pragma once

#include <string.h>

#include <base/tl/array.h>

typedef void (*pCmdFunc)(class CGameContext *pGameServer, int ClientID, char *pArgs); 

struct CCmd
{
	bool m_Visible;
	char m_aName[256];
	pCmdFunc m_Function;
	CCmd(){}
	CCmd(char *pName, pCmdFunc pFunc, bool Visible) : m_Function(pFunc), m_Visible(Visible)
	{
		str_copy(m_aName, pName, sizeof(m_aName));
		//str_format(m_aName, sizeof(m_aName), "%c%s", '/', pName);
	}
};

class CChatCommands
{
private:
	static void AliveErr(class CGameContext *pGameServer, int ClientID);
	static void ArgError(class CGameContext *pGameServer, int ClientID);
	static void AuthError(class CGameContext *pGameServer, int ClientID);

	static void Test(class CGameContext *pGameServer, int ClientID, char *pArgs);
	static void Register(class CGameContext *pGameServer, int ClientID, char *pArgs);
	static void Login(class CGameContext *pGameServer, int ClientID, char *pArgs);
	static void Save(class CGameContext *pGameServer, int ClientID, char *pArgs);
	static void Logout(class CGameContext *pGameServer, int ClientID, char *pArgs);
	static void CmdList(class CGameContext *pGameServer, int ClientID, char *pArgs);
	static void ResetPlayers(class CGameContext *pGameServer, int ClientID, char *pArgs);
	static void Whisper(class CGameContext *pGameServer, int ClientID, char *pArgs);
	static void Credits(class CGameContext *pGameServer, int ClientID, char *pArgs);
	static void Utilization(class CGameContext *pGameServer, int ClientID, char *pArgs);
	static void MapInfo(class CGameContext *pGameServer, int ClientID, char *pArgs);
	static void Home(class CGameContext *pGameServer, int ClientID, char *pArgs);

public:
	array<CCmd>m_AllCommands;

	void Init(class CGameContext *pGameServer);
};