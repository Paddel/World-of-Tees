#pragma once

typedef void (*ResultFunction)(int index, char *pResult, int pResultSize, void *pData);

class IServer;

class CDatabase
{
private:
	void *m_pConnection;
	void *m_pResult;
	bool m_Connected;
	IServer *m_pServer;

	bool InitConnection(char *pAddr, char *pUserName, char *pPass, char *pSchema);
	void CloseConnection();

	char *GetDatabaseValue(char *pStr);

public:
	CDatabase();

	void Init(IServer *pServer, char *pAddr, char *pUserName, char *pPass, char *pTable);

	int Query(char *command);
	void GetResult(ResultFunction ResultCallback, void *pData);

	bool GetConnected() const { return m_Connected; }

	void WriteAccInfo(class CAccountInfo *pAccInfo);

	IServer *Server() const { return m_pServer; }
};