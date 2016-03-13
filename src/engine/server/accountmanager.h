#pragma once

#include <engine/server/database.h>
#include <engine/server/maploader.h>

class IServer;

class CAccountManager : CDatabase
{
private:
	IServer *m_pServer;

	void AddQueryStr(char *pDest, char *pStr);
	void AddQueryInt(char *pDest, int Val);

	void FillPlayerInformation(char *pStr, CAccountInfo *pAccInfo);
	static void CheckPassword(int index, char *pResult, int pResultSize, void *pData);
	static void WriteAccountInfo(int index, char *pResult, int pResultSize, void *pData);

public:
	CAccountManager();

	void Save(CAccountInfo *pAccInfo);
	bool Load(CAccountInfo *pAccInfo);
	bool Init(IServer *pServer);

	bool Login(char *pStr, int Size, char *pRegisterName, char *pRegisterPassword, CAccountInfo *pAccInfo);
	bool Register(char *pStr, int Size, char *pRegisterName, char *pRegisterPassword, CAccountInfo *pAccInfo);

	IServer *Server() const { return m_pServer; }
};