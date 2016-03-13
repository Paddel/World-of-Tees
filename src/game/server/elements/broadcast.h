#pragma once

#include <base/system.h>

class CGameContext;
class IServer;

#define MAX_BROADCAST_STRING 512

class CBroadcast
{
private:
	CGameContext *m_pGameServer;
	IServer *m_pServer;
	int64 m_BroadcastTime;
	int m_Owner;
	char m_aLastBroadString[MAX_BROADCAST_STRING];

	void DoBrodcastStringDead(char *pStr, int ClientID);
	void DoBroadcastStringIngame(char *pStr, int ClientID);

	void BroadAddQuestionSkip(char *pStr, int ClientID);
	void BroadAddExpBar(char *pStr, int ClientID);
	void BroadAddHealth(char *pStr, int ClientID);
	void BroadAddMana(char *pStr, int ClientID);
	void BroadAddAmmo(char *pStr, int ClientID);
	void BroadAddExp(char *pStr, int ClientID);
	void BroadAddMoney(char *pStr, int ClientID);
	void BroadAddSkillPoints(char *pStr, int ClientID);
	void BroadAddBreeth(char *pStr, int ClientID);
	void BroadAddEnterMap(char *pStr, int ClientID);
	void BroadAddChest(char *pStr, int ClientID);
	void BroadAddShift(char *pStr, int ClientID);

	void BroadAddStr(char *pStr, char *pAdd);

public:
	CBroadcast(CGameContext *pGameServer, int Owner);

	void Send(int ClientID);

	CGameContext *GameServer() const {return m_pGameServer;}
	IServer *Server() const {return m_pServer;}
};