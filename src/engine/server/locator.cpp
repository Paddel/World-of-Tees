
#include <base/system.h>
#include <base/stringseperation.h>
#include <engine/shared/config.h>
#include <engine/shared/network.h>
#include <game/server/playerinfo.h>

#include <windows.h>

#include "locator.h"

static NETSOCKET invalid_socket = {NETTYPE_INVALID, -1, -1};

CLocator::CLocator(IServer *pServer)
{
	m_pServer = pServer;
}

void CLocator::Tick()
{
	for(int i = 0; i < m_pRequests.size(); i++)
	{
		static const int BufferSize = 4024;
		char aBuffer[BufferSize];
		CRequest *pRequest = &m_pRequests[i];

		int Received = net_tcp_recv(pRequest->m_Sock, aBuffer, BufferSize - 1);
		if(Received <= 0)
			continue;
		aBuffer[Received + 1] = 0;
		if(pRequest->m_CallbackFunc)
			pRequest->m_CallbackFunc(Server(), pRequest->m_aIndicator, aBuffer, pRequest->m_pResObj);
		//dbg_msg("recv", "Received %i\n'%s'", Received, aBuffer);
		
		m_pRequests.remove_index(i);
	}
}

bool HttpRequest(CRequest *pRequest, char *pMethod, const char * Host, const char * Path)
{
	NETSOCKET Socket = invalid_socket;
	NETADDR HostAddress;
	char aNetBuff[1024];
	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(&HostAddress, aAddrStr, sizeof(aAddrStr), 80);

	if(net_host_lookup(Host, &HostAddress, NETTYPE_IPV4) != 0)
	{
		dbg_msg("HTTP-Request", "Error running host lookup");
		return false;
	}

	//Connect
	int socketID = create_http_socket();
	if(socketID < 0)
	{
		dbg_msg("HTTP-Request", "Error creating socket");
		return false;
	}

	Socket.type = NETTYPE_IPV4;
	Socket.ipv4sock = socketID;
	HostAddress.port = 80;
	
	if(net_tcp_connect(Socket, &HostAddress) != 0)
	{
		net_tcp_close(Socket);
		dbg_msg("HTTP-Request","Error connecting to host");
		return false;
	}

	net_set_non_blocking(Socket);

	str_format(aNetBuff, sizeof(aNetBuff), "%s %s HTTP/1.0\nHost: %s\n\n", pMethod, Path, Host);
	net_tcp_send(Socket, aNetBuff, str_length(aNetBuff));
	pRequest->m_Sock = Socket;
	return true;
}

char * CLocator::EscapeStr(const char * From)
{
    unsigned long Len = str_length(From);
    unsigned long DestLen = Len * 4;
    char * Result = new char[DestLen + 1];
    memset(Result, 0, DestLen + 1);
        
    unsigned long Char;
    const char * Text = From;
    char * ToText = Result;
    unsigned long i;
    for (i = 0; i < Len; i++)
    {
		if ((From[i] >= 'a' && From[i] <= 'z') || (From[i] >= 'A' && From[i] <= 'Z') || (From[i] >= '0' && From[i] <= '9'))
			*(ToText++) = From[i];
		else
		{
			*(ToText++) = '%';
			str_format(ToText, 3, "%02x", ((unsigned int)From[i])%0x100);
			ToText += 2;
		}
    }
        
    return Result;
}


void CLocator::LocateIP(HttpRequestCallback Callback, char *pAddr, class CLocation *pLocation)//TODO:move this in a thread maybe.. bottleneck
{
	const int RequestSize = 4096;

	char *pEscapedStr = EscapeStr(pAddr);

	char *pRequest = new char[RequestSize];

	if(g_Config.m_SvAllowLocation == 0)
		return;

	delete [] pEscapedStr;

	str_format(pRequest, RequestSize, "/json/%s", pAddr);
	
	CRequest m_Request;
	bool Reached = HttpRequest(&m_Request, "GET", "google.com", "");
	if (!Reached)
	{
		delete [] pRequest;
		return;
	}

	str_copy(m_Request.m_aIndicator, pAddr, sizeof(m_Request.m_aIndicator));
	m_Request.m_CallbackFunc = Callback;
	m_Request.m_pResObj = pLocation;

	m_pRequests.add(m_Request);
}

void CLocator::InsertLocation(CLocation *pLocation, char *pRequestResult)
{
	char *pJsonStr = (char *)str_find_nocase(pRequestResult, "{\"ip");
	// no location, no to return
	if (!pJsonStr)
		return;

	pJsonStr += str_length("{\"");

	
	while(pJsonStr)
	{
		char *pName = GetSepStr('\"', &pJsonStr);
		if(!pName || !pJsonStr)
			break;
		char *pInd = GetSepStr('\"', &pJsonStr);
		if(!pInd || pInd[0] != ':' || !pJsonStr)
			break;
		char *pVal = GetSepStr('\"', &pJsonStr);
		if(!pVal || !pJsonStr)
			break;
		char *pEndChar = GetSepStr('\"', &pJsonStr);
		if(!pEndChar)
			break;

		if(str_comp(pName, "ip") == 0)
			continue;//ip not needed
		else if(str_comp(pName, "country_code") == 0)
			str_copy(pLocation->m_aCountryCode, pVal, sizeof(pLocation->m_aCountryCode));
		else if(str_comp(pName, "country_name") == 0)
		str_copy(pLocation->m_aCountryName, pVal, sizeof(pLocation->m_aCountryName));
		else if(str_comp(pName, "region_code") == 0)
			str_copy(pLocation->m_aRegionCode, pVal, sizeof(pLocation->m_aRegionCode));
		else if(str_comp(pName, "region_name") == 0)
			str_copy(pLocation->m_aRegion, pVal, sizeof(pLocation->m_aRegion));
		else if(str_comp(pName, "city") == 0)
			str_copy(pLocation->m_aCity, pVal, sizeof(pLocation->m_aCity));
		else if(str_comp(pName, "zip_code") == 0)
			str_copy(pLocation->m_aZipCode, pVal, sizeof(pLocation->m_aZipCode));
		else if(str_comp(pName, "latitude") == 0)
			str_format(pLocation->m_aCoords, sizeof(pLocation->m_aCoords), "%s%s %s%s",pName, pInd, pVal, pEndChar);
		else if(str_comp(pName, "metro_code") == 0)
			str_copy(pLocation->m_aMetroCode, pVal, sizeof(pLocation->m_aMetroCode));
		else if(str_comp(pName, "area_code") == 0)
			str_copy(pLocation->m_aAreaCode, pVal, sizeof(pLocation->m_aAreaCode));
		else if(str_comp(pName, "time_zone") == 0)
			str_copy(pLocation->m_aTimeZone, pVal, sizeof(pLocation->m_aTimeZone));
	}
}