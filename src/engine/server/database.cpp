
#define DB_CONNECTION_BAD 0

#include <stdio.h>

#include <base/system.h>
#include <engine/server.h>

#if defined(CONF_FAMILY_WINDOWS)
#include "winsock2.h"
#endif

#include <mysql.h>

#include "database.h"

CDatabase::CDatabase()
{
	m_Connected = false;
	m_pServer = 0;
}

void CDatabase::Init(IServer *pServer, char *pAddr, char *pUserName, char *pPass, char *pSchema)
{
	m_pServer = pServer;
	dbg_msg("Database", "Creating connection.");
	m_Connected = InitConnection(pAddr, pUserName, pPass, pSchema);
	if(m_Connected == DB_CONNECTION_BAD)
		dbg_msg("Database", "Connecting failed: '%s'", mysql_error((MYSQL *)m_pConnection));
}


bool CDatabase::InitConnection(char *pAddr, char *pUserName, char *pPass, char *pSchema)
{
	m_pConnection = mysql_init(NULL);
 
    if (m_pConnection == NULL)
		return DB_CONNECTION_BAD;

    if (mysql_real_connect((MYSQL *)m_pConnection, pAddr, pUserName, pPass, pSchema, 0,NULL,0) == NULL)
        return DB_CONNECTION_BAD;

	//if(g_Config.m_Debug)
	dbg_msg("Database", "Connected");

	return true;
}

void CDatabase::CloseConnection()
{
	mysql_close((MYSQL *)m_pConnection);
}

int CDatabase::Query(char *command)
{
	MYSQL *pConnection = (MYSQL*)m_pConnection;
	if(!m_Connected)
	{
		//if(g_Config.m_Debug)
		dbg_msg("Database", "Not connected. Can't send command");
		return 1;
	}

	mysql_query(pConnection, command);//Main-cmd

	{//error
		int err = mysql_errno(pConnection);
		if(err)
		{
			dbg_msg("Database", "Query failed: '%s", mysql_error(pConnection));
			return err;
		}
	}

	m_pResult = mysql_store_result(pConnection);
	MYSQL_RES *pResult = (MYSQL_RES *)m_pResult;

	if(!pResult)//no Results. DONE
		return -1;

	int affected = mysql_num_fields((MYSQL_RES *)pResult);
	int cont = (int)((MYSQL_RES *)pResult)->row_count; // Bottleneck: Use SELECT *, COUNT(*)
	//cout << "number entries: " << cont << endl;
	
	if(cont == 0)
		return -1;

	//for(int i = 0; i < cont; i++)
	//{
	//	for (int x= 0; x < affected; x++)
	//	{
	//		//if(g_Config.m_Debug)
	//			dbg_msg("Database", "Value of %i: %s", i, field[x]);
	//	}
	//	
	//	field = mysql_fetch_row(result);
	//	affected = mysql_num_fields(result);
	//}
	return 0;
}

void CDatabase::GetResult(ResultFunction ResultCallback, void *pData)
{
	MYSQL_RES *pResult = (MYSQL_RES *)m_pResult;
	if(!pResult)
		return;

	MYSQL_ROW field = mysql_fetch_row(pResult);
	int affected = mysql_num_fields(pResult);
	int count = (int)pResult->row_count; // Bottleneck: Use SELECT *, COUNT(*)
	
	if(count == 0 || affected == 0)
		return;

	for(int i = 0; i < count; i++)
	{
		for(int x = 0; x < affected; x++)
			ResultCallback(x, GetDatabaseValue(field[x]), sizeof(field[x]), pData);

		field = mysql_fetch_row(pResult);
		affected = mysql_num_fields(pResult);
	}
	pResult = 0;
}

char *CDatabase::GetDatabaseValue(char *pStr)
{
	if(pStr == 0)
		return "";
	return pStr;
}
