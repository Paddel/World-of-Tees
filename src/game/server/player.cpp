/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <base/stringseperation.h>
#include <engine/shared/config.h>
#include <game/balancing.h>
#include <game/server/elements/broadcast.h>
#include <game/server/elements/questionhandler.h>
#include "player.h"


MACRO_ALLOC_POOL_ID_IMPL(CPlayer, MAX_CLIENTS)

IServer *CPlayer::Server() const { return m_pGameServer->Server(); }

CPlayer::CPlayer(CGameContext *pGameServer, int ClientID, int Team)
{
	m_pGameServer = pGameServer;
	m_DieTick = Server()->Tick();
	m_ScoreStartTick = Server()->Tick();
	m_pCharacter = 0;
	m_ClientID = ClientID;
	m_LastActionTick = Server()->Tick();
	m_TeamChangeTick = Server()->Tick();

	m_pWantedMap = 0;
	m_WantTeleportHome = false;

	int* idMap = Server()->GetIdMap(ClientID);
	for(int i = 1;i < VANILLA_MAX_CLIENTS;i++)
	{
	    idMap[i] = -1;
	}
	idMap[0] = ClientID;

	m_pMap = Server()->CurrentMap(m_ClientID);
	m_pAccountInfo = Server()->GetClientAccountInfo(m_ClientID);
	m_pPlayerInfo = Server()->GetClientPlayerInfo(m_ClientID);
	CPlayerItem::Init(pGameServer, m_pMap, vec2(0, 0), CPlayerItem::TYPE_PLAYER);

	m_Team = PlayerInfo()->m_SpectatorID == -1? 0: TEAM_SPECTATORS;

	m_pTextPopup = new CTextPopup(GameServer(), ClientID);
	m_pQuestionHandler = new CQuestionHandler(GameServer(), ClientID);
	m_pBroadcast = new CBroadcast(GameServer(), ClientID);

	GetMap()->OnClientEnterMap(ClientID);
}

CPlayer::~CPlayer()
{
	GameServer()->m_World.RemovePlayerItem(this, m_pMap);
	delete m_pCharacter;
	m_pCharacter = 0;
}

void CPlayer::Tick()
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID))
		return;

	TextPopup()->Tick();
	QuestionHandler()->Tick();
	if(GetTeam() == TEAM_SPECTATORS && PlayerInfo()->m_SpectatorID != -1)
		Broadcast()->Send(PlayerInfo()->m_SpectatorID);
	else
		Broadcast()->Send(m_ClientID);

	//Server()->SetClientScore(m_ClientID, m_Score);

	// do latency stuff
	{
		IServer::CClientInfo Info;
		if(Server()->GetClientInfo(m_ClientID, &Info))
		{
			m_Latency.m_Accum += Info.m_Latency;
			m_Latency.m_AccumMax = max(m_Latency.m_AccumMax, Info.m_Latency);
			m_Latency.m_AccumMin = min(m_Latency.m_AccumMin, Info.m_Latency);
		}
		// each second
		if(Server()->Tick()%Server()->TickSpeed() == 0)
		{
			m_Latency.m_Avg = m_Latency.m_Accum/Server()->TickSpeed();
			m_Latency.m_Max = m_Latency.m_AccumMax;
			m_Latency.m_Min = m_Latency.m_AccumMin;
			m_Latency.m_Accum = 0;
			m_Latency.m_AccumMin = 1000;
			m_Latency.m_AccumMax = 0;
		}
	}

	if(!GameServer()->m_World.m_Paused)
	{
		if(!m_pCharacter && m_Team == TEAM_SPECTATORS && PlayerInfo()->m_SpectatorID == SPEC_FREEVIEW)
			m_ViewPos -= vec2(clamp(m_ViewPos.x-m_LatestActivity.m_TargetX, -500.0f, 500.0f), clamp(m_ViewPos.y-m_LatestActivity.m_TargetY, -400.0f, 400.0f));

		if(m_pCharacter)
		{
			if(m_pCharacter->IsAlive())
			{
				m_ViewPos = m_pCharacter->m_Pos;
			}
			else
			{
				delete m_pCharacter;
				m_pCharacter = 0;
			}
		}
		else
			TryRespawn();
	}
	else
	{
		++m_DieTick;
		++m_ScoreStartTick;
		++m_LastActionTick;
		++m_TeamChangeTick;
 	}

	if(m_Team == TEAM_SPECTATORS && PlayerInfo()->m_SpectatorID != SPEC_FREEVIEW && GameServer()->m_apPlayers[PlayerInfo()->m_SpectatorID])
	{
		if(GetMap() != GameServer()->m_apPlayers[PlayerInfo()->m_SpectatorID]->GetMap())
			m_pWantedMap = GameServer()->m_apPlayers[PlayerInfo()->m_SpectatorID]->GetMap();
	}
}

void CPlayer::PostTick()
{
	// update latency value
	if(m_PlayerFlags&PLAYERFLAG_SCOREBOARD)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				m_aActLatency[i] = GameServer()->m_apPlayers[i]->m_Latency.m_Min;
		}
	}

	// update view pos for spectators
	if(m_Team == TEAM_SPECTATORS && PlayerInfo()->m_SpectatorID != SPEC_FREEVIEW && GameServer()->m_apPlayers[PlayerInfo()->m_SpectatorID])
		m_ViewPos = GameServer()->m_apPlayers[PlayerInfo()->m_SpectatorID]->m_ViewPos;

	if (m_WantTeleportHome)
	{
		TeleportHome();
		m_WantTeleportHome = false;
	}

	if(m_pWantedMap)
	{
		if(GetMap() != m_pWantedMap)
		{
			if(!Server()->PlayerToMap(m_ClientID, m_pWantedMap))
				GameServer()->SendChatTarget(m_ClientID, "Worldsection will come soon.");
		}

		m_pWantedMap = 0;
	}
}

void CPlayer::Snap(int SnappingClient)
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID) || !Server()->ClientIngame(SnappingClient))
		return;
	
	int id = m_ClientID;
	if(!GameServer()->GameTranslate(id, SnappingClient)) return;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, id, sizeof(CNetObj_ClientInfo)));
	if(!pClientInfo)
		return;


	StrToInts(&pClientInfo->m_Name0, 4, Server()->ClientName(m_ClientID));
	StrToInts(&pClientInfo->m_Clan0, 3, Server()->ClientClan(m_ClientID));
	pClientInfo->m_Country = Server()->ClientCountry(m_ClientID);

	StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
	pClientInfo->m_UseCustomColor = m_TeeInfos.m_UseCustomColor;
	pClientInfo->m_ColorBody = m_TeeInfos.m_ColorBody;
	pClientInfo->m_ColorFeet = m_TeeInfos.m_ColorFeet;
	
	if(GetCharacter() && GetCharacter()->StatusEffectSkin(&pClientInfo->m_ColorBody, &pClientInfo->m_ColorFeet))
		pClientInfo->m_UseCustomColor = 1;

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, id, sizeof(CNetObj_PlayerInfo)));
	if(!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = SnappingClient == -1 ? m_Latency.m_Min : GameServer()->m_apPlayers[SnappingClient]->m_aActLatency[m_ClientID];
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_ClientID = id;
	pPlayerInfo->m_Score = AccountInfo()->m_Level;
	pPlayerInfo->m_Team = m_Team;

	if(m_ClientID == SnappingClient)
		pPlayerInfo->m_Local = 1;

	if(m_ClientID == SnappingClient && m_Team == TEAM_SPECTATORS)
	{
		CNetObj_SpectatorInfo *pSpectatorInfo = static_cast<CNetObj_SpectatorInfo *>(Server()->SnapNewItem(NETOBJTYPE_SPECTATORINFO, m_ClientID, sizeof(CNetObj_SpectatorInfo)));
		if(!pSpectatorInfo)
			return;

		int SnappingSpectatorID = PlayerInfo()->m_SpectatorID;
		if(SnappingSpectatorID != -1)
		{
			GameServer()->GameTranslate(SnappingSpectatorID, m_ClientID);
		}
		pSpectatorInfo->m_SpectatorID = SnappingSpectatorID;
		pSpectatorInfo->m_X = m_ViewPos.x;
		pSpectatorInfo->m_Y = m_ViewPos.y;
	}
}

void CPlayer::OnDisconnect(const char *pReason)
{
	KillCharacter();

	if(Server()->ClientIngame(m_ClientID))
	{
		char aBuf[512];
		if(pReason && *pReason)
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game (%s)", Server()->ClientName(m_ClientID), pReason);
		else
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game", Server()->ClientName(m_ClientID));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", m_ClientID, Server()->ClientName(m_ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
	}
}

void CPlayer::OnPredictedInput(CNetObj_PlayerInput *NewInput)
{
	// skip the input if chat is active
	if((m_PlayerFlags&PLAYERFLAG_CHATTING) && (NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING))
		return;

	if(m_pCharacter)
		m_pCharacter->OnPredictedInput(NewInput);
}

void CPlayer::OnDirectInput(CNetObj_PlayerInput *NewInput)
{
	if(NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING)
	{
		// skip the input if chat is active
		if(m_PlayerFlags&PLAYERFLAG_CHATTING)
			return;

		// reset input
		if(m_pCharacter)
			m_pCharacter->ResetInput();

		m_PlayerFlags = NewInput->m_PlayerFlags;
 		return;
	}

	m_PlayerFlags = NewInput->m_PlayerFlags;

	if(m_pCharacter)
		m_pCharacter->OnDirectInput(NewInput);

	// check for activity
	if(NewInput->m_Direction || m_LatestActivity.m_TargetX != NewInput->m_TargetX ||
		m_LatestActivity.m_TargetY != NewInput->m_TargetY || NewInput->m_Jump ||
		NewInput->m_Fire&1 || NewInput->m_Hook)
	{
		m_LatestActivity.m_TargetX = NewInput->m_TargetX;
		m_LatestActivity.m_TargetY = NewInput->m_TargetY;
		m_LastActionTick = Server()->Tick();
	}
}

CCharacter *CPlayer::GetCharacter()
{
	if(m_pCharacter && m_pCharacter->IsAlive())
		return m_pCharacter;
	return 0;
}

void CPlayer::KillCharacter(int Weapon)
{
	if(m_pCharacter)
	{
		m_pCharacter->Die(m_ClientID, Weapon);
		delete m_pCharacter;
		m_pCharacter = 0;
	}
}

void CPlayer::OnCharacterDead()
{
	AccountInfo()->m_DeathNum++;

	AccountInfo()->m_Health = Character_MaxHealth(AccountInfo()->m_Level)*0.5f;
	AccountInfo()->m_Money *= 0.5f;
	AccountInfo()->m_Experience *= 0.6f;

	WantTeleportHome();
}

void CPlayer::SetTeam(int Team, bool DoChatMsg)
{
	// clamp the team
	Team = GameServer()->m_pController->ClampTeam(Team);
	if(m_Team == Team)
		return;

	char aBuf[512];
	if(DoChatMsg)
	{
		str_format(aBuf, sizeof(aBuf), "'%s' joined the %s", Server()->ClientName(m_ClientID), GameServer()->m_pController->GetTeamName(Team));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	}

	KillCharacter();

	m_Team = Team;
	m_LastActionTick = Server()->Tick();
	PlayerInfo()->m_SpectatorID = SPEC_FREEVIEW;
	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", m_ClientID, Server()->ClientName(m_ClientID), m_Team);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	GameServer()->m_pController->OnPlayerInfoChange(GameServer()->m_apPlayers[m_ClientID]);

	if(Team == TEAM_SPECTATORS)
	{
		// update spectator modes
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->PlayerInfo()->m_SpectatorID == m_ClientID)
				GameServer()->m_apPlayers[i]->PlayerInfo()->m_SpectatorID = SPEC_FREEVIEW;
		}
	}
}

void CPlayer::TryRespawn()
{
	vec2 SpawnPos;

	vec2 TempSpawn = PlayerInfo()->m_TempSpawnPos;
	if(TempSpawn.x && TempSpawn.y)
	{
		SpawnPos = TempSpawn;
		PlayerInfo()->m_TempSpawnPos = vec2(0, 0);
	}
	//else if(m_pAccInfo->m_SpawnPos.x && m_pAccInfo->m_SpawnPos.y)
		//SpawnPos = m_pAccInfo->m_SpawnPos;
	else
	{
		if(!GameServer()->m_pController->CanSpawn(m_Team, &SpawnPos, m_pMap))
			return;
	}

	m_Spawning = false;
	m_pCharacter = new(m_ClientID) CCharacter(&GameServer()->m_World, m_pMap);
	m_pCharacter->Spawn(this, SpawnPos);
	GameServer()->CreatePlayerSpawn(SpawnPos, m_pMap);
}

void CPlayer::WantTeleportHome()
{
	m_WantTeleportHome = true;
}

void CPlayer::TeleportHome()
{
	CMap *pMap = Server()->m_MapLoader.GetDefaultMap();

	if(pMap == GetMap())
		KillCharacter();
	else
		m_pWantedMap = pMap;
}

void CPlayer::LoadWeaponString()
{
	if(GetCharacter() == NULL)
		return;

	char *pWeaponStr = AccountInfo()->m_aWeaponString;

	for(int i = 0; i < MAX_WEAPONS; i++)
		PlayerInfo()->m_pWeapon[i] = NULL;

	char *pAttribute = GetSepStr(';', &pWeaponStr);
	while(pAttribute[0])
	{
		CWeapon *pWeapon = GameServer()->CreateWeaponByString(pAttribute);
		if(pWeapon)
			GetCharacter()->GiveWeapon(pWeapon);

		pAttribute = GetSepStr(';', &pWeaponStr);
	}

	GetCharacter()->EmptyWeaponCheck();
}

void CPlayer::SetBeginingValues()
{
	AccountInfo()->m_Level = 1;
	AccountInfo()->m_TicketLevel = 1;
	AccountInfo()->m_Experience = 0;
	AccountInfo()->m_Money = 0;
	AccountInfo()->m_DeathNum = 0;
	AccountInfo()->m_SkillPoints = 0;

	AccountInfo()->m_Health = Character_MaxHealth(AccountInfo()->m_Level);
}

bool CPlayer::FillSavingInfos()
{
	//if(GetCharacter() == NULL)
		//return false;//should not happen!

	AccountInfo()->m_CurrentPos = GetCore()->m_Pos;
	str_copy(AccountInfo()->m_aCurrentMap, GetMap()->GetName(), sizeof(AccountInfo()->m_aCurrentMap));

	mem_zero(AccountInfo()->m_aWeaponString, sizeof(AccountInfo()->m_aWeaponString));
	for(int i = 0; i < MAX_WEAPONS; i++)
	{
		CWeapon *pWeapon = PlayerInfo()->m_pWeapon[i];
		char aBuf[256];
		if(pWeapon == NULL)
			continue;

		GameServer()->CreateWeaponString(pWeapon, aBuf, sizeof(aBuf));
		str_fcat(AccountInfo()->m_aWeaponString, sizeof(AccountInfo()->m_aWeaponString), "%s", AccountInfo()->m_aWeaponString[0]?";":"");
		str_fcat(AccountInfo()->m_aWeaponString, sizeof(AccountInfo()->m_aWeaponString), "%s", aBuf);
	}

	return true;
}

bool CPlayer::Register(char *pFailMsg, int Size, char *pName, char *pPassword)
{
	if(PlayerInfo()->m_LoggedIn)
		return false;

	if(FillSavingInfos() == false)
		return false;

	if(Server()->m_AccountManager.Register(pFailMsg, Size, pName, pPassword, AccountInfo()) == false)
		return false;

	PlayerInfo()->m_LoggedIn = true;
	return true;
}

bool CPlayer::Login(char *pFailMsg, int Size, char *pName, char *pPassword)
{
	if(PlayerInfo()->m_LoggedIn || GetMap()->GetMapType() != MAPTYPE_TOWN || GetCharacter() == NULL)
		return false;

	if(Server()->m_AccountManager.Login(pFailMsg, Size, pName, pPassword, AccountInfo()) == false)
		return false;//smth failed

	CMap *pMap = Server()->m_MapLoader.GetMap(AccountInfo()->m_aCurrentMap);
	if(!pMap)
		pMap = Server()->m_MapLoader.GetDefaultMap();

	PlayerInfo()->m_LoggedIn = true;

	LoadWeaponString();

	if(pMap == GetMap())
		GetCharacter()->Core()->m_Pos = AccountInfo()->m_CurrentPos;
	else
	{
		PlayerInfo()->m_TempSpawnPos = AccountInfo()->m_CurrentPos;
		Server()->PlayerToMap(m_ClientID, pMap);
	}
	return true;
}

bool CPlayer::Save()
{
	if(!PlayerInfo()->m_LoggedIn || GetMap()->GetMapType() != MAPTYPE_TOWN)
		return false;

	if(Server()->GetClientChangingMap(m_ClientID))
	{
		dbg_msg("Saving", "tried to save while changing map");
		return false;
	}

	if(FillSavingInfos() == false)
		return false;

	Server()->m_AccountManager.Save(AccountInfo());
	return true;
}

bool CPlayer::Logout()
{
	if(!PlayerInfo()->m_LoggedIn || GetMap()->GetMapType() != MAPTYPE_TOWN)
		return false;

	Save();
	PlayerInfo()->m_LoggedIn = false;
	mem_zero(AccountInfo(), sizeof(CAccountInfo));
	PlayerInfo()->Reset();
	SetBeginingValues();

	TeleportHome();
	return true;
}

bool CPlayer::EarnMoney(int Money)
{
	AccountInfo()->m_Money += Money;
	return true;
}

void CPlayer::GainExp(int Amount)
{
	AccountInfo()->m_Experience += Amount;
	int NeededExp = Character_NeededExperience(AccountInfo()->m_Level);
	if(AccountInfo()->m_Experience > NeededExp)
	{//level up
		int Rest = AccountInfo()->m_Experience-NeededExp;
		LevelUp();
		AccountInfo()->m_Experience = 0;
		GainExp(Rest);
	}
}

void CPlayer::LevelUp()
{
	AccountInfo()->m_Level++;
	AccountInfo()->m_SkillPoints++;
	if(GetCharacter())
	{
		GameServer()->CreateSound(GetCharacter()->m_Pos, SOUND_CTF_CAPTURE, GetCharacter()->Map());
		for(float i = 0.0f; i < 2*pi; i+=0.3f)
			GameServer()->CreateDamageInd(GetCharacter()->m_Pos, i, 1, GetCharacter()->Map());

		AccountInfo()->m_Health = Character_MaxHealth(AccountInfo()->m_Level);
	}
}

void CPlayer::Heal(float percent)
{
	int MaxHP = Character_MaxHealth(AccountInfo()->m_Level);
	int Healing = MaxHP*percent;
	AccountInfo()->m_Health = clamp(AccountInfo()->m_Health + Healing, 0, MaxHP);
}

void CPlayer::Spectate(int SpectatingID)
{
	if(PlayerInfo()->m_SpectatorID == -1 && SpectatingID != -1)
	{//started
		SetTeam(TEAM_SPECTATORS, false);
	}
	else if(PlayerInfo()->m_SpectatorID != -1 && SpectatingID == -1)
	{//stopped
		SetTeam(1, false);
	}

	PlayerInfo()->m_SpectatorID = SpectatingID;
}