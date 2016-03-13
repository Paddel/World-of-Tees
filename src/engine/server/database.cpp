
#define DB_CONNECTION_BAD 0

#include <base/system.h>
#include <engine/server.h>

#include <WinSock.h>
#include <mysql.h>

#include "database.h"

MYSQL_ROW field;
MYSQL *conn;
MYSQL_RES *result;

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
		dbg_msg("Database", "Connecting failed: '%s'", mysql_error(conn));
}


bool CDatabase::InitConnection(char *pAddr, char *pUserName, char *pPass, char *pSchema)
{
	conn = mysql_init(NULL);
 
    if (conn == NULL)
		return DB_CONNECTION_BAD;

    if (mysql_real_connect(conn, pAddr, pUserName, pPass, pSchema, 0,NULL,0) == NULL)
        return DB_CONNECTION_BAD;

	//if(g_Config.m_Debug)
	dbg_msg("Database", "Connected");

	return true;
}

void CDatabase::CloseConnection()
{
	mysql_close(conn);
}

int CDatabase::Query(char *command)
{
	if(!m_Connected)
	{
		//if(g_Config.m_Debug)
		dbg_msg("Database", "Not connected. Can't send command");
		return 1;
	}

	mysql_query(conn, command);//Main-cmd

	{//error
		int err = mysql_errno(conn);
		if(err)
		{
			dbg_msg("Database", "Query failed: '%s", mysql_error(conn));
			return err;
		}
	}

	result = mysql_store_result(conn);

	if(!result)//no Results. DONE
		return -1;

	int affected = mysql_num_fields(result);
	int cont = (int)result->row_count; // Bottleneck: Use SELECT *, COUNT(*)
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
	if(!result)
		return;

	field = mysql_fetch_row(result);
	int affected = mysql_num_fields(result);
	int count = (int)result->row_count; // Bottleneck: Use SELECT *, COUNT(*)
	
	if(count == 0 || affected == 0)
		return;

	for(int i = 0; i < count; i++)
	{
		for(int x = 0; x < affected; x++)
			ResultCallback(x, GetDatabaseValue(field[x]), sizeof(field[x]), pData);

		field = mysql_fetch_row(result);
		affected = mysql_num_fields(result);
	}
	result = 0;
}

char *CDatabase::GetDatabaseValue(char *pStr)
{
	if(pStr == 0)
		return "";
	return pStr;
}