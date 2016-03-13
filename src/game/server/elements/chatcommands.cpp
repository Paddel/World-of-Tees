
#include <base/stringseperation.h>

#include <game/server/gamecontext.h>

#include "chatcommands.h"

void CChatCommands::AliveErr(class CGameContext *pGameServer, int ClientID)
{
	pGameServer->SendChatTarget(ClientID, "You have to be alive to use this command.");
}

void CChatCommands::ArgError(class CGameContext *pGameServer, int ClientID)
{
	pGameServer->SendChatTarget(ClientID, "The written parameters weren't correct.");
}

void CChatCommands::AuthError(class CGameContext *pGameServer, int ClientID)
{
	pGameServer->SendChatTarget(ClientID, "You have to be an admin to use this command.");
}

void CChatCommands::Test(class CGameContext *pGameServer, int ClientID, char *pArgs)
{
	if(!pArgs)
	{
		ArgError(pGameServer, ClientID);
		return;
	}

	char *pTxt = GetSepStr(' ', &pArgs);

	if(pArgs)
	{
		ArgError(pGameServer, ClientID);
		return;
	}

	pGameServer->SendChatTarget(ClientID, pTxt);
}

void CChatCommands::Register(class CGameContext *pGameServer, int ClientID, char *pArgs)
{
	char *pName = GetSepStr(' ', &pArgs);
	char *pPassword = GetSepStr(' ', &pArgs);

	CPlayer *pPlayer = pGameServer->m_apPlayers[ClientID];

	if(!pPlayer)
		return;

	if(pArgs)
	{
		ArgError(pGameServer, ClientID);
		return;
	}

	if(pPlayer->PlayerInfo()->m_LoggedIn)
	{
		pGameServer->SendChatTarget(ClientID, "You are already logged in.");
		return;
	}

	char aFailMessage[128];
	if(pPlayer->Register(aFailMessage, sizeof(aFailMessage), pName, pPassword))
		pGameServer->SendChatTarget(ClientID, "Registration succesfull!");
	else
		pGameServer->SendChatTarget(ClientID, aFailMessage);
}

void CChatCommands::Login(class CGameContext *pGameServer, int ClientID, char *pArgs)
{
	char *pName = GetSepStr(' ', &pArgs);
	char *pPassword = GetSepStr(' ', &pArgs);
	CPlayer *pPlayer = pGameServer->m_apPlayers[ClientID];

	if(!pPlayer)
		return;

	if(pArgs)
	{
		ArgError(pGameServer, ClientID);
		return;
	}

	if(pPlayer->PlayerInfo()->m_LoggedIn)
	{
		pGameServer->SendChatTarget(ClientID, "You are already logged in.");
		return;
	}

	if(pPlayer->GetMap()->GetMapType() != MAPTYPE_TOWN)
	{
		pGameServer->SendChatTarget(ClientID, "You have to be in a town to login.");
		return;
	}

	char aFailMessage[128];
	if(pPlayer->Login(aFailMessage, sizeof(aFailMessage), pName, pPassword))
		pGameServer->SendChatTarget(ClientID, "Login succesfull!");
	else
		pGameServer->SendChatTarget(ClientID, aFailMessage);
}

void CChatCommands::Save(class CGameContext *pGameServer, int ClientID, char *pArgs)
{
	CPlayer *pPlayer = pGameServer->m_apPlayers[ClientID];

	if(!pPlayer)
		return;

	if(pPlayer->GetMap()->GetMapType() != MAPTYPE_TOWN)
	{
		pGameServer->SendChatTarget(ClientID, "You have to be in a town to save.");
		return;
	}

	if(!pPlayer->PlayerInfo()->m_LoggedIn)
	{
		pGameServer->SendChatTarget(ClientID, "You have to be logged in.");
		return;
	}

	if(pPlayer->Save())
		pGameServer->SendChatTarget(ClientID, "Saving was succesfull!");
	else
		pGameServer->SendChatTarget(ClientID, "Account could not be saved!");
}

void CChatCommands::Logout(class CGameContext *pGameServer, int ClientID, char *pArgs)
{
	CPlayer *pPlayer = pGameServer->m_apPlayers[ClientID];

	if(!pPlayer)
		return;

	if(pPlayer->GetMap()->GetMapType() != MAPTYPE_TOWN)
	{
		pGameServer->SendChatTarget(ClientID, "You have to be in a town to logout.");
		return;
	}

	if(!pPlayer->PlayerInfo()->m_LoggedIn)
	{
		pGameServer->SendChatTarget(ClientID, "You have to be logged in.");
		return;
	}

	pPlayer->Logout();
	pGameServer->SendChatTarget(ClientID, "Logged out succesfull!");
}


void CChatCommands::CmdList(class CGameContext *pGameServer, int ClientID, char *pArgs)
{
	CChatCommands *pCmds = &pGameServer->m_ChatCommands;

	char aCmdList[1024];
	strcpy(aCmdList, "Commands: ");
	for(int i = 0; i < pCmds->m_AllCommands.size(); i++)
	{
		if(pCmds->m_AllCommands[i].m_Visible == false)
			continue;

		str_format(aCmdList, sizeof(aCmdList), "%s%s%s", aCmdList[0]?aCmdList:"", pCmds->m_AllCommands[i].m_aName, (i!=pCmds->m_AllCommands.size()-1)?", ":"");
	}

	pGameServer->SendChatTarget(ClientID, aCmdList);
}

void CChatCommands::ResetPlayers(class CGameContext *pGameServer, int ClientID, char *pArgs)
{
	CPlayer *pPlayer = pGameServer->m_apPlayers[ClientID];
	if(!pPlayer)
		return;

	//pPlayer->ResetIDMap();
	pGameServer->SendChatTarget(ClientID, "Use this command if there are invisible tees near you.");
}

void CChatCommands::Whisper(class CGameContext *pGameServer, int ClientID, char *pArgs)
{
	if(!pArgs)
	{
		ArgError(pGameServer, ClientID);
		return;
	}

	char *pTargetName = GetSepStr(' ', &pArgs);
	int TargetID = -1;
	int NumTargets = 0;

	if(!pTargetName)
		return;


	while(pArgs)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(pGameServer->Server()->ClientIngame(i) == false || ClientID == i)
				continue;

			const char *pClientName = pGameServer->Server()->ClientName(i);
			if(str_find_nocase(pClientName, pTargetName))
			{
				TargetID = i;
				NumTargets++;
			}
		}
		if(NumTargets || str_length(pTargetName) >= 16)
			break;

		str_format(pTargetName, 16, "%s %s", pTargetName, GetSepStr(' ', &pArgs));
	}

	if(pGameServer->Server()->ClientIngame(TargetID) == false)
	{
		pGameServer->SendChatTarget(ClientID, "Player not found.");
		return;
	}

	if(NumTargets != 1)
	{
		pGameServer->SendChatTarget(ClientID, "Too many possible chattargets found.");
		return;
	}

	char aBuf[1024];
	str_format(aBuf, sizeof(aBuf), "%s => %s: %s", pGameServer->Server()->ClientName(ClientID), pGameServer->Server()->ClientName(TargetID), pArgs);
	pGameServer->SendChatTarget(TargetID, aBuf);
	pGameServer->SendChatTarget(ClientID, aBuf);

	//pGameServer->SendChatAuthed(aBuf);
}

void CChatCommands::Home(class CGameContext *pGameServer, int ClientID, char *pArgs)
{
	int MapType;
	CCharacter *pChr;

	if (pArgs)
	{
		ArgError(pGameServer, ClientID);
		return;
	}

	pChr = pGameServer->GetPlayerChar(ClientID);
	if (pChr == NULL)
		return;

	MapType = pChr->Map()->GetMapType();
	if (MapType != MAPTYPE_WILDNESS && MapType != MAPTYPE_TOWN)
	{
		pGameServer->SendChatTarget(ClientID, "You can't teleport back here.");
		return;
	}

	pChr->WantBackport();
}

void CChatCommands::Credits(class CGameContext *pGameServer, int ClientID, char *pArgs)
{
}

void CChatCommands::Utilization(class CGameContext *pGameServer, int ClientID, char *pArgs)
{
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "%i server-ticks/second, %i game-ticks/second.", pGameServer->Server()->GetServerTicks(), pGameServer->Server()->GetGameTicks());
	pGameServer->SendChatTarget(ClientID, aBuf);
}

void CChatCommands::MapInfo(class CGameContext *pGameServer, int ClientID, char *pArgs)
{
	char aBuf[64];
	CCharacter *pChr = pGameServer->GetPlayerChar(ClientID);
	if (pChr == NULL)
	{
		AliveErr(pGameServer, ClientID);
		return;
	}

	str_format(aBuf, sizeof(aBuf), "(Temperature: %i, Moisture %i, Eruption %i) => Biome: %s",
		pChr->Map()->GetMapTemperature(), pChr->Map()->GetMapMoisture(), pChr->Map()->GetMapEruption(), s_aMapBiomNames[pChr->Map()->GetMapBiome()]);
	pGameServer->SendChatTarget(ClientID, aBuf);
}

void CChatCommands::Init(class CGameContext *pGameServer)
{
	m_AllCommands.add(CCmd("Register", &Register, true));
	m_AllCommands.add(CCmd("Login", &Login, true));
	m_AllCommands.add(CCmd("Save", &Save, true));
	m_AllCommands.add(CCmd("Logout", &Logout, true));
	m_AllCommands.add(CCmd("ResetPlayers", &ResetPlayers, true));
	m_AllCommands.add(CCmd("Whisper", &Whisper, true));
	m_AllCommands.add(CCmd("Home", &Home, true));

	m_AllCommands.add(CCmd("w", &Whisper, false));
	m_AllCommands.add(CCmd("u", &Utilization, false));
	m_AllCommands.add(CCmd("Credits", &Credits, false));
	m_AllCommands.add(CCmd("Test", &Test, false));
	m_AllCommands.add(CCmd("CmdList", &CmdList, false));
	m_AllCommands.add(CCmd("MapInfo", &MapInfo, false));
}