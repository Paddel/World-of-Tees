
#include <stdio.h>

#include <base/system.h>
#include <engine/server.h>
#include <game/server/playerinfo.h>

#include <string.h>

#include "accountmanager.h"

#define QUERY_MAX_STR_LEN 512
#define SCHEMA_NAME "wot"

CAccountManager::CAccountManager() : CDatabase()
{
}

bool CAccountManager::Init(IServer *pServer)
{
	m_pServer = pServer;
	CDatabase::Init(pServer, "78.47.53.206", "taschenrechner", "hades", SCHEMA_NAME);

	return GetConnected();
}


void CAccountManager::Save(CAccountInfo *pAccInfo)
{
	char aQuery[QUERY_MAX_STR_LEN];//use UPDATE
	mem_zero(&aQuery, sizeof(aQuery));

	str_copy(aQuery, "REPLACE INTO ", sizeof(aQuery));
	FillPlayerInformation(aQuery, pAccInfo);

	Query(aQuery);
}

void CAccountManager::FillPlayerInformation(char *pStr, CAccountInfo *pAccInfo)
{
	strcat(pStr, "player(name, password, money, level, experience, health, map, posx, posy, deathnum, skillpoints, weapons, ticketlevel) VALUES (");
	AddQueryStr(pStr, pAccInfo->m_aLoginName);
	strcat(pStr, ", ");
	AddQueryStr(pStr, pAccInfo->m_aLoginPassword);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pAccInfo->m_Money);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pAccInfo->m_Level);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pAccInfo->m_Experience);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pAccInfo->m_Health);
	strcat(pStr, ", ");
	AddQueryStr(pStr, pAccInfo->m_aCurrentMap);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pAccInfo->m_CurrentPos.x);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pAccInfo->m_CurrentPos.y);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pAccInfo->m_DeathNum);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pAccInfo->m_SkillPoints);
	strcat(pStr, ", ");
	AddQueryStr(pStr, pAccInfo->m_aWeaponString);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pAccInfo->m_TicketLevel);
	strcat(pStr, ");");
}

bool CAccountManager::Load(CAccountInfo *pAccInfo)
{
	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s.player WHERE player.name=\"%s\"", SCHEMA_NAME, pAccInfo->m_aLoginName);
	int res = Query(aQuery);
	
	if(res == -1)
		return false;

	GetResult(&WriteAccountInfo, pAccInfo);
	return true;
}

void CAccountManager::WriteAccountInfo(int index, char *pResult, int pResultSize, void *pData)
{
	CAccountInfo *pAccInfo = (CAccountInfo *)pData;
	switch(index)
	{
		case 0: str_copy(pAccInfo->m_aLoginName, pResult, ACCOUNT_MAX_NAME_LEN); break;
		case 1: str_copy(pAccInfo->m_aLoginPassword, pResult, ACCOUNT_MAX_PASS_LEN); break;
		case 2: pAccInfo->m_Money = atoi(pResult); break;
		case 3: pAccInfo->m_Level = atoi(pResult); break;
		case 4: pAccInfo->m_Experience = atoi(pResult); break;
		case 5: pAccInfo->m_Health = atoi(pResult); break;
		case 6: str_copy(pAccInfo->m_aCurrentMap, pResult, MAX_MAP_NAME); break;
		case 7: pAccInfo->m_CurrentPos.x = atoi(pResult); break;
		case 8: pAccInfo->m_CurrentPos.y = atoi(pResult); break;
		case 9: pAccInfo->m_DeathNum = atoi(pResult); break;
		case 10: pAccInfo->m_SkillPoints = atoi(pResult); break;
		case 11: str_copy(pAccInfo->m_aWeaponString, pResult, WEAP_MAX_LEN); break;
		case 12: pAccInfo->m_TicketLevel = atoi(pResult); break;

		//case 9999: pAccInfo->m_Money = atoi(pResult); break;
	}
}

bool PasswordConfirmed;
bool CAccountManager::Login(char *pStr, int Size, char *pRegisterName, char *pRegisterPassword, CAccountInfo *pAccInfo)
{
	for(int i = 0; i < str_length(pRegisterName); i++)
	{
		char c = pRegisterName[i];
		if(((c >= 48 & c <= 57)|(c >= 65 & c <= 90)|(c >= 97 & c <= 122)) == 0)
		{
			str_format(pStr, Size, "Registration failed: Your name contains invalid characters.");
			return false;
		}
	}
	for(int i = 0; i < str_length(pRegisterPassword); i++)
	{
		char c = pRegisterPassword[i];
		if(((c >= 48 & c <= 57)|(c >= 65 & c <= 90)|(c >= 97 & c <= 122)) == 0)
		{
			str_format(pStr, Size, "Registration failed: Your password contains invalid characters.");
			return false;
		}
	}

	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT password FROM %s.player WHERE player.name=\"%s\"", SCHEMA_NAME, pRegisterName);
	int res = Query(aQuery);
	PasswordConfirmed = false;

	if(res == -1)
	{
		str_format(pStr, Size, "Login failed: Account does not exist.");
		return false;
	}

	GetResult(&CheckPassword, pRegisterPassword);
	if(PasswordConfirmed == false)
	{
		str_format(pStr, Size, "Login failed: Password is not correct.");
		return false;
	}

	str_copy(pAccInfo->m_aLoginName, pRegisterName, sizeof(pAccInfo->m_aLoginName));
	str_copy(pAccInfo->m_aLoginPassword, pRegisterPassword, sizeof(pAccInfo->m_aLoginPassword));

	if(!Load(pAccInfo))
	{
		str_format(pStr, Size, "Login failed: Could not load account data.");
		return false;
	}
	
	return true;
}

void CAccountManager::CheckPassword(int index, char *pResult, int pResultSize, void *pData)
{
	if(index == 0 && str_comp((char *)pData, pResult) == 0)
		PasswordConfirmed = true;
	else
		PasswordConfirmed = false;
}


bool CAccountManager::Register(char *pStr, int Size, char *pRegisterName, char *pRegisterPassword, CAccountInfo *pAccInfo)
{
	if(!pRegisterName[0])
	{
		str_format(pStr, Size, "Registration failed: Your name cant be empty.");
		return false;
	}
	if(str_length(pRegisterName) >= ACCOUNT_MAX_NAME_LEN)
	{
		str_format(pStr, Size, "Registration failed: Namelength of 16 characters reached.");
		return false;
	}
	if(!pRegisterPassword[0])
	{
		str_format(pStr, Size, "Registration failed: Your password cant be empty.");
		return false;
	}
	if(str_length(pRegisterPassword) >= ACCOUNT_MAX_PASS_LEN)
	{
		str_format(pStr, Size, "Registration failed: Passwordlength of 16 characters reached.");
		return false;
	}
	for(int i = 0; i < str_length(pRegisterName); i++)
	{
		char c = pRegisterName[i];
		if(((c >= 48 & c <= 57)|(c >= 65 & c <= 90)|(c >= 97 & c <= 122)) == 0)
		{
			str_format(pStr, Size, "Registration failed: Your name contains invalid characters.");
			return false;
		}
	}
	for(int i = 0; i < str_length(pRegisterPassword); i++)
	{
		char c = pRegisterPassword[i];
		if(((c >= 48 & c <= 57)|(c >= 65 & c <= 90)|(c >= 97 & c <= 122)) == 0)
		{
			str_format(pStr, Size, "Registration failed: Your password contains invalid characters.");
			return false;
		}
	}


	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s.player WHERE player.name=\"%s\"", SCHEMA_NAME, pRegisterName);
	int res = Query(aQuery);

	if(res != -1)
	{
		str_format(pStr, Size, "Registration failed: Account Already exists.");
		return false;
	}

	str_copy(pAccInfo->m_aLoginName, pRegisterName, sizeof(pAccInfo->m_aLoginName));
	str_copy(pAccInfo->m_aLoginPassword, pRegisterPassword, sizeof(pAccInfo->m_aLoginPassword));

	mem_zero(&aQuery, sizeof(aQuery));
	str_copy(aQuery, "INSERT INTO ", sizeof(aQuery));
	FillPlayerInformation(aQuery, pAccInfo);

	res = Query(aQuery);


	if(res > 0)
	{
		str_format(pStr, Size, "Registration failed: Unkown error (%i). Please report this as a bug with the number of the error you register informations and a comment. Thank you.", res);
		return false;
	}

	return true;
}

void CAccountManager::AddQueryStr(char *pDest, char *pStr)
{
	strcat(pDest, "\"");
	strcat(pDest, pStr);
	strcat(pDest, "\"");
}

void CAccountManager::AddQueryInt(char *pDest, int Val)
{
	char aBuf[128];
	sprintf(aBuf, "%d", Val);
	strcat(pDest, "\"");
	strcat(pDest, aBuf);
	strcat(pDest, "\"");
}