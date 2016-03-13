#pragma once

typedef void (*HttpRequestCallback)(void *pServerAddr, char *pIndicator, char *pResult, void *pResObj); 

struct CRequest
{
	HttpRequestCallback m_CallbackFunc;
	NETSOCKET m_Sock;
	char m_aIndicator[128];
	void *m_pResObj;
};

struct CLocation
{
	char m_aCountryCode[4];
	char m_aCountryName[64];
	char m_aRegionCode[4];
	char m_aRegion[64];
	char m_aCity[64];
	char m_aZipCode[64];
	char m_aCoords[64];
	char m_aMetroCode[4];
	char m_aAreaCode[4];
	char m_aTimeZone[64];
};

class CLocator
{
private:
	array<CRequest> m_pRequests;
	IServer *m_pServer;

	char *EscapeStr(const char * From);

public:
	CLocator(IServer *pServer);

	void Tick();
	void InsertLocation(CLocation *pLocation, char *pRequestResult);
	void LocateIP(HttpRequestCallback Callback, char *pAddr, class CLocation *pLocation);

	IServer *Server() const { return m_pServer; }
};

