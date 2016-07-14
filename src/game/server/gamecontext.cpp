/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <time.h>
#include <base/math.h>
#include <base/stringseperation.h>
#include <engine/shared/config.h>
#include <engine/map.h>
#include <engine/console.h>
#include "gamecontext.h"
#include <game/version.h>
#include <game/collision.h>
#include <game/gamecore.h>
#include <game/chest.h>
#include <game/server/elements/questionhandler.h>
#include <game/server/npc/_include.h>
#include <game/server/entities/money.h>
#include <game/server/entities/door.h>
#include <game/server/entities/weaponpickup.h>
#include <game/server/entities/bolder.h>
#include <game/server/entities/weaponmount.h>

#define MAX_CHAT_STRING_LENGTH 127
#define ALLOW_TEAM_CHANGE 0
#define USE_REAL_TIME 0

enum
{
	RESET,
	NO_RESET
};

void CGameContext::Construct(int Resetting)
{
	m_Resetting = 0;
	m_pServer = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
		m_apPlayers[i] = 0;

	m_pController = 0;
	m_LockTeams = 0;

	m_IngameTime = 0;
	m_IngameTimeSpeed = 1;
	m_pDevMap = NULL;

	if(Resetting==NO_RESET) {}
}

CGameContext::CGameContext(int Resetting)
{
	Construct(Resetting);
}

CGameContext::CGameContext()
{
	Construct(NO_RESET);
}

CGameContext::~CGameContext()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		delete m_apPlayers[i];
}

void CGameContext::Clear()
{
	CTuningParams Tuning = m_Tuning;

	m_Resetting = true;
	this->~CGameContext();
	mem_zero(this, sizeof(*this));
	new (this) CGameContext(RESET);

	m_Tuning = Tuning;
}

class CCharacter *CGameContext::GetPlayerChar(int ClientID)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || !m_apPlayers[ClientID])
		return 0;
	return m_apPlayers[ClientID]->GetCharacter();
}

bool CGameContext::GameTranslate(int& ClientID, int TranslateID)
{
	if(!Server()->ClientIngame(ClientID) || !Server()->ClientIngame(TranslateID))
		return false;

	CPlayer *pPlayer = m_apPlayers[ClientID];

	if(!pPlayer || Server()->CurrentMap(ClientID) != Server()->CurrentMap(TranslateID))
		return false;

	ClientID = m_World.GetPlayerItemId(pPlayer, pPlayer->GetMap());
	if (ClientID == -1 || !Server()->Translate(ClientID, TranslateID)) return false;
	return true;
}

int CGameContext::GetMapInteger(char **pSrc, char *pCall, vec2 Pos, CMap *pMap)
{
	char *pInd = GetSepStr(0xff, pSrc);
	if(pInd[0] != 'i')
	{
		dbg_msg(pCall, "Indicator error '%s' at Pos %f, %f on Map %s", pInd, Pos.x, Pos.y, pMap->GetName());
		return -1;
	}

	return GetSepInt(0xff, pSrc);
}

char *CGameContext::GetMapString(char **pSrc, char *pCall, vec2 Pos, CMap *pMap)
{
	char *pInd = GetSepStr(0xff, pSrc);
	if(pInd[0] != 's')
	{
		dbg_msg(pCall, "Indicator error '%s' at Pos %f, %f on Map %s", pInd, Pos.x, Pos.y, pMap->GetName());
		return "";
	}
	return GetSepStr(0xff, pSrc);;
}

void CGameContext::CreateDamageInd(vec2 Pos, float Angle, int Amount, CMap *pMap)
{
	float a = 3 * 3.14159f / 2 + Angle;
	//float a = get_angle(dir);
	float s = a-pi/3;
	float e = a+pi/3;
	for(int i = 0; i < Amount; i++)
	{
		float f = mix(s, e, float(i+1)/float(Amount+2));
		CNetEvent_DamageInd *pEvent = (CNetEvent_DamageInd *)m_Events.Create(NETEVENTTYPE_DAMAGEIND, sizeof(CNetEvent_DamageInd), pMap);
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
			pEvent->m_Angle = (int)(f*256.0f);
		}
	}
}

void CGameContext::CreateHammerHit(vec2 Pos, CMap *pMap)
{
	// create the event
	CNetEvent_HammerHit *pEvent = (CNetEvent_HammerHit *)m_Events.Create(NETEVENTTYPE_HAMMERHIT, sizeof(CNetEvent_HammerHit), pMap);
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}
}


void CGameContext::CreateExplosion(vec2 Pos, int Owner, int Weapon, int Damage, CMap *pMap, CTargetAble *pOwner)
{
	// create the event
	CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)m_Events.Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion), pMap);
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}

	if (Damage > 0)
	{
		float Radius = 135.0f;
		float InnerRadius = 48.0f;

		// deal damage
		CWorldSection *pSection = pMap->WorldSection();
		for(int i = 0; i < pSection->m_lpTargetAbles.size(); i++)
		{
			CTargetAble *pTarget = pSection->m_lpTargetAbles[i];

			if (pTarget->IsEnemy(pOwner) == false)
				continue;

			if(pTarget->GetDistance(Pos) > Radius+pTarget->GetProximityRadius())
				continue;

			vec2 Diff = pTarget->GetClosestPos(Pos) - Pos;
			vec2 ForceDir(0,1);
			float l = length(Diff);
			if(l)
				ForceDir = normalize(Diff);
			l = 1-clamp((l-InnerRadius)/(Radius-InnerRadius), 0.0f, 1.0f);
			float Dmg = Damage * l;
			if((int)Dmg)
				pTarget->TakeDamage(ForceDir*(6*l)*2, (int)Dmg, Owner, Weapon);
		}
	}
}

/*
void create_smoke(vec2 Pos)
{
	// create the event
	EV_EXPLOSION *pEvent = (EV_EXPLOSION *)events.create(EVENT_SMOKE, sizeof(EV_EXPLOSION));
	if(pEvent)
	{
		pEvent->x = (int)Pos.x;
		pEvent->y = (int)Pos.y;
	}
}*/

void CGameContext::CreatePlayerSpawn(vec2 Pos, CMap *pMap)
{
	// create the event
	CNetEvent_Spawn *ev = (CNetEvent_Spawn *)m_Events.Create(NETEVENTTYPE_SPAWN, sizeof(CNetEvent_Spawn), pMap);
	if(ev)
	{
		ev->m_X = (int)Pos.x;
		ev->m_Y = (int)Pos.y;
	}
}

void CGameContext::CreateDeath(vec2 Pos, int ClientID, CMap *pMap)
{
	// create the event
	CNetEvent_Death *pEvent = (CNetEvent_Death *)m_Events.Create(NETEVENTTYPE_DEATH, sizeof(CNetEvent_Death), pMap);
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_ClientID = ClientID;
	}
}

void CGameContext::CreateSound(vec2 Pos, int Sound, CMap *pMap, int Mask)
{
	if (Sound < 0)
		return;

	// create a sound
	CNetEvent_SoundWorld *pEvent = (CNetEvent_SoundWorld *)m_Events.Create(NETEVENTTYPE_SOUNDWORLD, sizeof(CNetEvent_SoundWorld), pMap, Mask);
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_SoundID = Sound;
	}
}

void CGameContext::CreateSoundGlobal(int Sound, int Target)
{
	if (Sound < 0)
		return;

	CNetMsg_Sv_SoundGlobal Msg;
	Msg.m_SoundID = Sound;
	if(Target == -2)
		Server()->SendPackMsg(&Msg, MSGFLAG_NOSEND, -1);
	else
	{
		int Flag = MSGFLAG_VITAL;
		if(Target != -1)
			Flag |= MSGFLAG_NORECORD;
		Server()->SendPackMsg(&Msg, Flag, Target);
	}
}


void CGameContext::SendChatTarget(int To, const char *pText)
{
	CNetMsg_Sv_Chat Msg;
	Msg.m_Team = 0;
	Msg.m_ClientID = -1;
	Msg.m_pMessage = pText;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, To);
}

void CGameContext::ChatCommand(int ChatterClientID, char *pTxt)
{
	char *pCmd = GetSepStr(' ', &pTxt);
	pCmd += str_length("/");

	int ChatCommandID = -1;
	for(int i = 0; i < m_ChatCommands.m_AllCommands.size(); i++)
	{
		if(str_comp_nocase(pCmd, m_ChatCommands.m_AllCommands[i].m_aName) == 0)
		{
			ChatCommandID = i;
			break;
		}
	}

	if(ChatCommandID == -1)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Command \"%s\" not found. Write \"/cmdlist\" to see all available commands", pCmd);
		SendChatTarget(ChatterClientID, aBuf);
		//SendChatTarget(ChatterClientID, "Command not found. Write '/cmdlist' for all available Commands and /help for help");//<-avast sees dis as a virus?
		//SendChatTarget(ChatterClientID, "Command not found.");
		//SendChatTarget(ChatterClientID, "Write '/cmdlist' too see all available Commands.");
		return;
	}

	m_ChatCommands.m_AllCommands[ChatCommandID].m_Function(this, ChatterClientID, pTxt);
}

void CGameContext::SendChat(int ChatterClientID, int Team, const char *pText)
{
	if(pText[0] == '/')
	{
		char aCmdText[MAX_CHAT_STRING_LENGTH];
		strcpy(aCmdText, pText);
		ChatCommand(ChatterClientID, aCmdText);
		return;
	}


	char aBuf[256];
	if(ChatterClientID >= 0 && ChatterClientID < MAX_CLIENTS)
		str_format(aBuf, sizeof(aBuf), "%d:%d:%s: %s", ChatterClientID, Team, Server()->ClientName(ChatterClientID), pText);
	else
		str_format(aBuf, sizeof(aBuf), "*** %s", pText);
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, Team!=CHAT_ALL?"teamchat":"chat", aBuf);

	int PlayerItemID = -1;
	
	if(ChatterClientID >= 0 && ChatterClientID < MAX_CLIENTS)
	{
		CPlayer *pPlayer = m_apPlayers[ChatterClientID];
		PlayerItemID = m_World.GetPlayerItemId(m_apPlayers[ChatterClientID], pPlayer->GetMap());
	}

	if(Team == CHAT_ALL)
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 0;
		Msg.m_ClientID = PlayerItemID;
		Msg.m_pMessage = pText;

		if(PlayerItemID != -1)
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(!Server()->ClientIngame(i))
					continue;

				Msg.m_ClientID = PlayerItemID;
				Server()->SendPackMsgTranslate(&Msg, MSGFLAG_VITAL, i, ChatterClientID, Server()->CurrentMap(ChatterClientID) != Server()->CurrentMap(i));
			}
		}
		else//server messages can send unedited
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
	}
	else
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 1;
		Msg.m_ClientID = ChatterClientID;
		Msg.m_pMessage = pText;

		// pack one for the recording only
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NOSEND, -1);

		// send to the clients
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			//if(m_apPlayers[i] && m_apPlayers[i]->GetTeam() == Team)
				//Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NORECORD, i, PlayerItemID);
		}
	}
}

void CGameContext::SendEmoticon(int ClientID, int Emoticon)
{
	CPlayer *pPlayer = m_apPlayers[ClientID];
	if(pPlayer == NULL)
		return;

	int PlayerItemID = m_World.GetPlayerItemId(pPlayer, pPlayer->GetMap());
	CNetMsg_Sv_Emoticon Msg;
	Msg.m_ClientID = PlayerItemID;
	Msg.m_Emoticon = Emoticon;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!Server()->ClientIngame(i) || Server()->CurrentMap(ClientID) != Server()->CurrentMap(i))
			continue;

		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
	}
}

void CGameContext::SendWeaponPickup(int ClientID, int Weapon)
{
	CNetMsg_Sv_WeaponPickup Msg;
	Msg.m_Weapon = Weapon;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}


void CGameContext::SendBroadcast(const char *pText, int ClientID)
{
	CNetMsg_Sv_Broadcast Msg;
	Msg.m_pMessage = pText;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

int CGameContext::NetworkClipped(CMap *pOnMap, int SnappingClient, vec2 CheckPos)
{
	if(SnappingClient == -1)
		return 0;

	CMap *pMap = Server()->CurrentMap(SnappingClient);
	if(pMap != pOnMap)
		return 1;

	float dx = m_apPlayers[SnappingClient]->m_ViewPos.x-CheckPos.x;
	float dy = m_apPlayers[SnappingClient]->m_ViewPos.y-CheckPos.y;

	if(absolute(dx) > 1000.0f || absolute(dy) > 800.0f)
		return 1;

	if(distance(m_apPlayers[SnappingClient]->m_ViewPos, CheckPos) > 1100.0f)
		return 1;
	return 0;
}

void CGameContext::ChestDrop(void *pInfo, char *pType, int Num, char *pExtraInfo, int ClientID)
{
	CGameContext *pThis = (CGameContext *)pInfo;
	if(pThis->Server()->ClientIngame(ClientID) == false)
		return;

	CCharacter *pChr = pThis->GetPlayerChar(ClientID);
	if(pChr == NULL)
		return;

	char aArgs[MAX_EXTENTED_STR];
	if(pExtraInfo)
		mem_copy(aArgs, pExtraInfo, sizeof(aArgs));
	char *pArgs = aArgs;

	vec2 Vel = normalize(vec2((rand()%200-100)*0.01f, -1))*16.0f;
	if(str_comp_nocase(pType, "Money") == 0)
	{
		if(rand()%4 == 0)
		{
			new CMoney(&pThis->m_World, pChr->Map(), pChr->m_Pos, Vel*0.7f, Num/2, ClientID);
			new CMoney(&pThis->m_World, pChr->Map(), pChr->m_Pos, Vel*1.5f, Num/2, ClientID);
		}
		else
		{
			new CMoney(&pThis->m_World, pChr->Map(), pChr->m_Pos, Vel, Num, ClientID);
		}
	}
	else
	{
		int WeaponType = -1;
		for(int i = 0; i < NUM_WEAPS; i++)
		{
			if(str_comp_nocase(s_aWeaponNames[i], pType) == 0)
			{
				WeaponType = i;
				break;
			}
		}

		if(WeaponType == -1)
			return;//weapon name not found

		int Damage = 1;
		int Ammo = -1;
		char *pAttribute = GetSepStr(';', &pArgs);
		while(pAttribute[0])
		{
			const char *pVal = str_find(pAttribute, "=")+1;
			if(pVal)
			{
				char *pInd = GetSepStr('=', &pAttribute);
				if(str_comp(pInd, "dmg") == 0)
					Damage = atoi(pVal);
				if (str_comp(pInd, "ammo") == 0)
					Ammo = atoi(pVal);
			}
			pAttribute = GetSepStr(';', &pArgs);
		}

		CWeapon *pWeapon = CreateWeapon(WeaponType, pThis, NULL, Damage);
		if (Ammo != -1)
			pWeapon->SetAmmo(Ammo);

		if(pWeapon)
		{
			for(int i = 0; i < Num; i++)
			{
				new CWeaponPickup(&pThis->m_World, pChr->Map(), pChr->m_Pos, Vel, ClientID, pWeapon);

				Vel = normalize(vec2((rand()%200-100)*0.01f, -1))*16.0f;
			}
		}
	}
}

void CGameContext::WantLootChest(int ClientID, int ChestID)
{
	CChest *pChest = NULL;
	int Type = 0;

	if (ChestID >= 0)
	{
		int NumChests = sizeof(s_aChests) / sizeof(s_aChests[0]);
		if (ChestID >= 0 && ChestID < NumChests)
			pChest = &s_aChests[ChestID];
	}
	else
	{
		int RealChestID = -(ChestID + 1);
		CPlayer *pPlayer = m_apPlayers[ClientID];
		Type = 1;

		if (pPlayer && RealChestID >= 0 && RealChestID < MAX_CHESTS_PER_MAP)
			pChest = &pPlayer->GetMap()->WorldSection()->m_aChests[RealChestID];
	}

	if (pChest == NULL)
		return;

	if(pChest->WantLoot(ClientID))
	{
		if(Type == 0)
			pChest->Drop(NULL, ChestDrop, this, ClientID);
		else
			pChest->DropRand(NULL, ChestDrop, this, ClientID);

		pChest->SetCooldown();

		CCharacter *pChr = GetPlayerChar(ClientID);
		if(pChr)
		{
			int Mask = CmaskOne(ClientID);
			CreateSound(pChr->m_Pos, SOUND_CTF_GRAB_PL, pChr->Map(), Mask);
		}
	}
}

void CGameContext::CreateWeaponString(CWeapon *pWeapon, char *pStr, int Size)
{
	str_format(pStr, Size, "type=%s,dmg=%i", s_aWeaponShortcuts[pWeapon->GetType()], pWeapon->Damage());
	if(pWeapon->MaxAmmo() > 0)
		str_fcat(pStr, Size, ",ammo=%i", pWeapon->Ammo());
}

CWeapon *CGameContext::CreateWeaponByString(char *pStr)
{
	CWeapon *pWeapon;
	const char *pType = NULL;
	int Damage = 0;
	int Ammo = -1;

	char *pAttribute = GetSepStr(',', &pStr);
	while(pAttribute[0])
	{
		const char *pVal = str_find(pAttribute, "=")+1;
		if(pVal)
		{
			if(str_find(pAttribute, "type"))
				pType = pVal;
			else if(str_find(pAttribute, "dmg"))
				Damage = atoi(pVal);
			else if (str_find(pAttribute, "ammo"))
				Ammo = atoi(pVal);
		}

		pAttribute = GetSepStr(',', &pStr);
	}

	for(int i = 0; i < NUM_WEAPS; i++)
	{
		if(str_comp(pType, s_aWeaponShortcuts[i]) == 0)
		{
			pWeapon = CreateWeapon(i, this, NULL, Damage);
			if (Ammo > -1)
				pWeapon->SetAmmo(Ammo);
			break;
		}
	}

	return pWeapon;
}

bool CGameContext::TimeBetween(int HoursFrom, int MinutesFrom, int HoursTo, int MinutesTo)
{
	int TimeFrom = HoursFrom*60+MinutesFrom;
	int TimeTo = HoursTo*60+MinutesTo;
	int CurrentHours = GetIngameTime()/(60*50);
	int CurrentMinutes = GetIngameTime()/50-CurrentHours*60;
	int TimeCurrent = CurrentHours*60+CurrentMinutes;

	if(TimeTo < TimeFrom)
	{//the next day
		if(TimeFrom <= TimeCurrent && TimeCurrent < 24*60 || 0 <= TimeCurrent && TimeCurrent < TimeTo)
			return true;
		else
			return false;
	}
	else
	{//same day
		if(TimeFrom <= TimeCurrent && TimeCurrent < TimeTo)
			return true;
		else
			return false;
	}
}

void CGameContext::CheckPureTuning()
{
	// might not be created yet during start up
	if(!m_pController)
		return;

	if(	str_comp(m_pController->m_pGameType, "DM")==0 ||
		str_comp(m_pController->m_pGameType, "TDM")==0 ||
		str_comp(m_pController->m_pGameType, "CTF")==0)
	{
		CTuningParams p;
		if(mem_comp(&p, &m_Tuning, sizeof(p)) != 0)
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "resetting tuning due to pure server");
			m_Tuning = p;
		}
	}
}

void CGameContext::SendTuningParams(int ClientID)
{
	CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
	int *pParams = (int *)&m_Tuning;
	for(unsigned i = 0; i < sizeof(m_Tuning)/sizeof(int); i++)
		Msg.AddInt(pParams[i]);
	Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::SendTuningParams(int ClientID, CTuningParams Tunings)
{
	CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
	int *pParams = (int *)&Tunings;
	for (unsigned i = 0; i < sizeof(Tunings) / sizeof(int); i++)
		Msg.AddInt(pParams[i]);
	Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::SwapTeams()
{
	if(!m_pController->IsTeamplay())
		return;
	
	SendChat(-1, CGameContext::CHAT_ALL, "Teams were swapped");

	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(m_apPlayers[i] && m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			m_apPlayers[i]->SetTeam(m_apPlayers[i]->GetTeam()^1, false);
	}

	(void)m_pController->CheckTeamBalance();
}

void CGameContext::DoIngameTime()
{
	if(USE_REAL_TIME)
	{
		static time_t timeObj;
		time(&timeObj);
		tm *pTime = gmtime(&timeObj);

		m_IngameTime = pTime->tm_min*50 + (pTime->tm_hour==23?0:pTime->tm_hour+1)*60*50 + (pTime->tm_sec/60.0f)*50.0f;
	}
	else
	{
		m_IngameTime += m_IngameTimeSpeed;

		if(m_IngameTime >= 1440*50 && m_IngameTimeSpeed > 0)
			m_IngameTime = 0;

		if(m_IngameTime <= 0 && m_IngameTimeSpeed < 0)
			m_IngameTime = 1440*50;
	}
}

void CGameContext::OnTick()
{

	DoIngameTime();

	// copy tuning
	m_World.m_Core.m_Tuning = m_Tuning;
	m_World.Tick();

	//chests
	int NumChests = sizeof(s_aChests)/sizeof(s_aChests[0]);
	for(int i = 0; i < NumChests; i++)
		s_aChests[i].Tick();

	//if(world.paused) // make sure that the game object always updates
	m_pController->Tick();
}

// Server hooks
void CGameContext::OnClientDirectInput(int ClientID, void *pInput)
{
	if(!m_World.m_Paused)
		m_apPlayers[ClientID]->OnDirectInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientPredictedInput(int ClientID, void *pInput)
{
	if(!m_World.m_Paused)
		m_apPlayers[ClientID]->OnPredictedInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientEnter(int ClientID)
{
	//world.insert_entity(&players[client_id]);
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s", Server()->ClientName(ClientID), m_pController->GetTeamName(m_apPlayers[ClientID]->GetTeam()));
	SendChat(-1, CGameContext::CHAT_ALL, aBuf);

	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' team=%d", ClientID, Server()->ClientName(ClientID), m_apPlayers[ClientID]->GetTeam());
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	m_apPlayers[ClientID]->SetBeginingValues();
}

void CGameContext::OnClientConnected(int ClientID)
{
	// Check which team the player should be on
	const int StartTeam = 1;

	m_apPlayers[ClientID] = new(ClientID) CPlayer(this, ClientID, StartTeam);
	//players[client_id].init(client_id);
	//players[client_id].client_id = client_id;

	(void)m_pController->CheckTeamBalance();

#ifdef CONF_DEBUG
	if(g_Config.m_DbgDummies)
	{
		if(ClientID >= MAX_CLIENTS-g_Config.m_DbgDummies)
			return;
	}
#endif

	// send motd
	CNetMsg_Sv_Motd Msg;
	Msg.m_pMessage = g_Config.m_SvMotd;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::OnClientDrop(int ClientID, const char *pReason)
{
	m_apPlayers[ClientID]->OnDisconnect(pReason);
	delete m_apPlayers[ClientID];
	m_apPlayers[ClientID] = 0;

	(void)m_pController->CheckTeamBalance();

	// update spectator modes

	if(Server()->GetClientChangingMap(ClientID) == false)
	{// update spectator modes
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(m_apPlayers[i] && m_apPlayers[i]->PlayerInfo()->m_SpectatorID == ClientID)
				m_apPlayers[i]->PlayerInfo()->m_SpectatorID = SPEC_FREEVIEW;
		}
	}
}

void CGameContext::OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	void *pRawMsg = m_NetObjHandler.SecureUnpackMsg(MsgID, pUnpacker);
	CPlayer *pPlayer = m_apPlayers[ClientID];

	if(!pRawMsg)
	{
		if(g_Config.m_Debug)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "dropped weird message '%s' (%d), failed on '%s'", m_NetObjHandler.GetMsgName(MsgID), MsgID, m_NetObjHandler.FailedMsgOn());
			Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
		}
		return;
	}

	if(Server()->ClientIngame(ClientID))
	{
		if(MsgID == NETMSGTYPE_CL_SAY)
		{
			if(g_Config.m_SvSpamprotection && pPlayer->m_LastChat && pPlayer->m_LastChat+Server()->TickSpeed() > Server()->Tick())
				return;

			CNetMsg_Cl_Say *pMsg = (CNetMsg_Cl_Say *)pRawMsg;
			int Team = pMsg->m_Team ? pPlayer->GetTeam() : CGameContext::CHAT_ALL;
			
			// trim right and set maximum length to 128 utf8-characters
			int Length = 0;
			const char *p = pMsg->m_pMessage;
			const char *pEnd = 0;
			while(*p)
 			{
				const char *pStrOld = p;
				int Code = str_utf8_decode(&p);

				// check if unicode is not empty
				if(Code > 0x20 && Code != 0xA0 && Code != 0x034F && (Code < 0x2000 || Code > 0x200F) && (Code < 0x2028 || Code > 0x202F) &&
					(Code < 0x205F || Code > 0x2064) && (Code < 0x206A || Code > 0x206F) && (Code < 0xFE00 || Code > 0xFE0F) &&
					Code != 0xFEFF && (Code < 0xFFF9 || Code > 0xFFFC))
				{
					pEnd = 0;
				}
				else if(pEnd == 0)
					pEnd = pStrOld;

				if(++Length >= MAX_CHAT_STRING_LENGTH)
				{
					*(const_cast<char *>(p)) = 0;
					break;
				}
 			}
			if(pEnd != 0)
				*(const_cast<char *>(pEnd)) = 0;

			// drop empty and autocreated spam messages (more than 16 characters per second)
			if(Length == 0 || (g_Config.m_SvSpamprotection && pPlayer->m_LastChat && pPlayer->m_LastChat+Server()->TickSpeed()*((15+Length)/16) > Server()->Tick()))
				return;

			pPlayer->m_LastChat = Server()->Tick();

			SendChat(ClientID, Team, pMsg->m_pMessage);
		}
		else if(MsgID == NETMSGTYPE_CL_CALLVOTE)
		{
			/*if(g_Config.m_SvSpamprotection && pPlayer->m_LastVoteTry && pPlayer->m_LastVoteTry+Server()->TickSpeed()*3 > Server()->Tick())
				return;

			int64 Now = Server()->Tick();
			pPlayer->m_LastVoteTry = Now;

			{
				SendChatTarget(ClientID, "Voting system is disabled at the moment.");
				return;
			}

			if(pPlayer->GetTeam() == TEAM_SPECTATORS)
			{
				SendChatTarget(ClientID, "Spectators aren't allowed to start a vote.");
				return;
			}

			if(m_VoteCloseTime)
			{
				SendChatTarget(ClientID, "Wait for current vote to end before calling a new one.");
				return;
			}

			int Timeleft = pPlayer->m_LastVoteCall + Server()->TickSpeed()*60 - Now;
			if(pPlayer->m_LastVoteCall && Timeleft > 0)
			{
				char aChatmsg[512] = {0};
				str_format(aChatmsg, sizeof(aChatmsg), "You must wait %d seconds before making another vote", (Timeleft/Server()->TickSpeed())+1);
				SendChatTarget(ClientID, aChatmsg);
				return;
			}

			char aChatmsg[512] = {0};
			char aDesc[VOTE_DESC_LENGTH] = {0};
			char aCmd[VOTE_CMD_LENGTH] = {0};
			CNetMsg_Cl_CallVote *pMsg = (CNetMsg_Cl_CallVote *)pRawMsg;
			const char *pReason = pMsg->m_Reason[0] ? pMsg->m_Reason : "No reason given";

			if(str_comp_nocase(pMsg->m_Type, "option") == 0)
			{
				CVoteOptionServer *pOption = m_pVoteOptionFirst;
				while(pOption)
				{
					if(str_comp_nocase(pMsg->m_Value, pOption->m_aDescription) == 0)
					{
						str_format(aChatmsg, sizeof(aChatmsg), "'%s' called vote to change server option '%s' (%s)", Server()->ClientName(ClientID),
									pOption->m_aDescription, pReason);
						str_format(aDesc, sizeof(aDesc), "%s", pOption->m_aDescription);
						str_format(aCmd, sizeof(aCmd), "%s", pOption->m_aCommand);
						break;
					}

					pOption = pOption->m_pNext;
				}

				if(!pOption)
				{
					str_format(aChatmsg, sizeof(aChatmsg), "'%s' isn't an option on this server", pMsg->m_Value);
					SendChatTarget(ClientID, aChatmsg);
					return;
				}
			}
			else if(str_comp_nocase(pMsg->m_Type, "kick") == 0)
			{
				if(!g_Config.m_SvVoteKick)
				{
					SendChatTarget(ClientID, "Server does not allow voting to kick players");
					return;
				}

				if(g_Config.m_SvVoteKickMin)
				{
					int PlayerNum = 0;
					for(int i = 0; i < MAX_CLIENTS; ++i)
						if(m_apPlayers[i] && m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
							++PlayerNum;

					if(PlayerNum < g_Config.m_SvVoteKickMin)
					{
						str_format(aChatmsg, sizeof(aChatmsg), "Kick voting requires %d players on the server", g_Config.m_SvVoteKickMin);
						SendChatTarget(ClientID, aChatmsg);
						return;
					}
				}

				int KickID = str_toint(pMsg->m_Value);

				if (!Server()->ReverseTranslate(KickID, ClientID))
					return;

				CPlayerItem *pPlayerItem = pPlayer->GetMap()->WorldSection()->m_lpPlayerItems[KickID];
				if(!pPlayerItem)
					return;

				if(pPlayerItem->GetType() != CPlayerItem::TYPE_PLAYER)
				{
					SendChatTarget(ClientID, "You can't kick Bots.");
					return;
				}

				KickID = m_World.GetPlayerID(pPlayerItem);

				if(KickID < 0 || KickID >= MAX_CLIENTS || !m_apPlayers[KickID])
				{
					SendChatTarget(ClientID, "Invalid client id to kick");
					return;
				}

				if(KickID == ClientID)
				{
					SendChatTarget(ClientID, "You can't kick yourself");
					return;
				}
				if(Server()->IsAuthed(KickID))
				{
					SendChatTarget(ClientID, "You can't kick admins");
					char aBufKick[128];
					str_format(aBufKick, sizeof(aBufKick), "'%s' called for vote to kick you", Server()->ClientName(ClientID));
					SendChatTarget(KickID, aBufKick);
					return;
				}

				str_format(aChatmsg, sizeof(aChatmsg), "'%s' called for vote to kick '%s' (%s)", Server()->ClientName(ClientID), Server()->ClientName(KickID), pReason);
				str_format(aDesc, sizeof(aDesc), "Kick '%s'", Server()->ClientName(KickID));
				if (!g_Config.m_SvVoteKickBantime)
					str_format(aCmd, sizeof(aCmd), "kick %d Kicked by vote", KickID);
				else
				{
					char aAddrStr[NETADDR_MAXSTRSIZE] = {0};
					Server()->GetClientAddr(KickID, aAddrStr, sizeof(aAddrStr));
					str_format(aCmd, sizeof(aCmd), "ban %s %d Banned by vote", aAddrStr, g_Config.m_SvVoteKickBantime);
				}
			}
			else if(str_comp_nocase(pMsg->m_Type, "spectate") == 0)
			{
				if(!g_Config.m_SvVoteSpectate)
				{
					SendChatTarget(ClientID, "Server does not allow voting to move players to spectators");
					return;
				}

				int SpectateID = str_toint(pMsg->m_Value);

				if (!Server()->ReverseTranslate(SpectateID, ClientID))
					return;

				CPlayerItem *pPlayerItem = pPlayer->GetMap()->WorldSection()->m_lpPlayerItems[SpectateID];
				if(!pPlayerItem)
					return;

				if(pPlayerItem->GetType() != CPlayerItem::TYPE_PLAYER)
				{
					SendChatTarget(ClientID, "You can't move Bots to Spectator.");
					return;
				}

				SpectateID = m_World.GetPlayerID(pPlayerItem);

				if(SpectateID < 0 || SpectateID >= MAX_CLIENTS || !m_apPlayers[SpectateID] || m_apPlayers[SpectateID]->GetTeam() == TEAM_SPECTATORS)
				{
					SendChatTarget(ClientID, "Invalid client id to move");
					return;
				}
				if(SpectateID == ClientID)
				{
					SendChatTarget(ClientID, "You can't move yourself");
					return;
				}

				str_format(aChatmsg, sizeof(aChatmsg), "'%s' called for vote to move '%s' to spectators (%s)", Server()->ClientName(ClientID), Server()->ClientName(SpectateID), pReason);
				str_format(aDesc, sizeof(aDesc), "move '%s' to spectators", Server()->ClientName(SpectateID));
				str_format(aCmd, sizeof(aCmd), "set_team %d -1 %d", SpectateID, g_Config.m_SvVoteSpectateRejoindelay);
			}

			if(aCmd[0])
			{
				SendChat(-1, CGameContext::CHAT_ALL, aChatmsg);
				StartVote(aDesc, aCmd, pReason);
				pPlayer->m_Vote = 1;
				pPlayer->m_VotePos = m_VotePos = 1;
				m_VoteCreator = ClientID;
				pPlayer->m_LastVoteCall = Now;
			}*/
		}
		else if(MsgID == NETMSGTYPE_CL_VOTE)
		{
			CNetMsg_Cl_Vote *pMsg = (CNetMsg_Cl_Vote *)pRawMsg;
			if(pMsg->m_Vote == 1)
				pPlayer->QuestionHandler()->AnswerYes();
			else if(pMsg->m_Vote == -1)
				pPlayer->QuestionHandler()->AnswerNo();
			/*if(!m_VoteCloseTime)
				return;*/

			/*if(pPlayer->m_Vote == 0)
			{
				CNetMsg_Cl_Vote *pMsg = (CNetMsg_Cl_Vote *)pRawMsg;
				if(!pMsg->m_Vote)
					return;

				pPlayer->m_Vote = pMsg->m_Vote;
				pPlayer->m_VotePos = ++m_VotePos;
				m_VoteUpdate = true;
			}*/
		}
		else if(MsgID == NETMSGTYPE_CL_SETTEAM && !m_World.m_Paused)
		{
			CNetMsg_Cl_SetTeam *pMsg = (CNetMsg_Cl_SetTeam *)pRawMsg;

			if(ALLOW_TEAM_CHANGE == 0)
			{
				pPlayer->m_LastSetTeam = Server()->Tick();
				SendChatTarget(ClientID, "Changing teams is not allowed.");
				return;
			}

			if(pPlayer->GetTeam() == pMsg->m_Team || (g_Config.m_SvSpamprotection && pPlayer->m_LastSetTeam && pPlayer->m_LastSetTeam+Server()->TickSpeed()*3 > Server()->Tick()))
				return;

			if(pMsg->m_Team != TEAM_SPECTATORS && m_LockTeams)
			{
				pPlayer->m_LastSetTeam = Server()->Tick();
				SendBroadcast("Teams are locked", ClientID);
				return;
			}

			if(pPlayer->m_TeamChangeTick > Server()->Tick())
			{
				pPlayer->m_LastSetTeam = Server()->Tick();
				int TimeLeft = (pPlayer->m_TeamChangeTick - Server()->Tick())/Server()->TickSpeed();
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "Time to wait before changing team: %02d:%02d", TimeLeft/60, TimeLeft%60);
				SendBroadcast(aBuf, ClientID);
				return;
			}

			// Switch team on given client and kill/respawn him
			if(m_pController->CanJoinTeam(pMsg->m_Team, ClientID))
			{
				if(m_pController->CanChangeTeam(pPlayer, pMsg->m_Team))
				{
					pPlayer->m_LastSetTeam = Server()->Tick();
					pPlayer->SetTeam(pMsg->m_Team);
					(void)m_pController->CheckTeamBalance();
					pPlayer->m_TeamChangeTick = Server()->Tick();
				}
				else
					SendBroadcast("Teams must be balanced, please join other team", ClientID);
			}
			else
			{
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "Only %d active players are allowed", Server()->MaxClients()-g_Config.m_SvSpectatorSlots);
				SendBroadcast(aBuf, ClientID);
			}
		}
		else if(MsgID == NETMSGTYPE_CL_SETSPECTATORMODE && !m_World.m_Paused)
		{
			CNetMsg_Cl_SetSpectatorMode *pMsg = (CNetMsg_Cl_SetSpectatorMode *)pRawMsg;

			if(g_Config.m_SvSpamprotection && pPlayer->m_LastSetSpectatorMode && pPlayer->m_LastSetSpectatorMode+Server()->TickSpeed()*0.25f > Server()->Tick())
				return;

			if(Server()->IsAuthed(ClientID) == false)
				return;

			pPlayer->m_LastSetSpectatorMode = Server()->Tick();

			if(pMsg->m_SpectatorID != SPEC_FREEVIEW)
			{
				if (!Server()->ReverseTranslate(pMsg->m_SpectatorID, ClientID))
					return;

				CPlayerItem *pPlayerItem = pPlayer->GetMap()->WorldSection()->m_lpPlayerItems[pMsg->m_SpectatorID];
				if(!pPlayerItem)
					return;

				if(pPlayerItem->GetType() != CPlayerItem::TYPE_PLAYER)
				{
					SendChatTarget(ClientID, "You can't spectate Bots.");
					return;
				}

				pMsg->m_SpectatorID = m_World.GetPlayerID(pPlayerItem);
			}
			else
				return;

			if(pPlayer->GetTeam() != TEAM_SPECTATORS || pPlayer->PlayerInfo()->m_SpectatorID == pMsg->m_SpectatorID || ClientID == pMsg->m_SpectatorID)
				return;

			
			if(pMsg->m_SpectatorID != SPEC_FREEVIEW && (!m_apPlayers[pMsg->m_SpectatorID] || m_apPlayers[pMsg->m_SpectatorID]->GetTeam() == TEAM_SPECTATORS))
				SendChatTarget(ClientID, "Invalid spectator id used");
			else
				pPlayer->Spectate(pMsg->m_SpectatorID);
		}
		else if(MsgID == NETMSGTYPE_CL_CHANGEINFO)
		{
			if(g_Config.m_SvSpamprotection && pPlayer->m_LastChangeInfo && pPlayer->m_LastChangeInfo+Server()->TickSpeed()*5 > Server()->Tick())
				return;

			CNetMsg_Cl_ChangeInfo *pMsg = (CNetMsg_Cl_ChangeInfo *)pRawMsg;
			pPlayer->m_LastChangeInfo = Server()->Tick();

			// set infos
			char aOldName[MAX_NAME_LENGTH];
			str_copy(aOldName, Server()->ClientName(ClientID), sizeof(aOldName));
			Server()->SetClientName(ClientID, pMsg->m_pName);
			if(str_comp(aOldName, Server()->ClientName(ClientID)) != 0)
			{
				char aChatText[256];
				str_format(aChatText, sizeof(aChatText), "'%s' changed name to '%s'", aOldName, Server()->ClientName(ClientID));
				SendChat(-1, CGameContext::CHAT_ALL, aChatText);
			}
			Server()->SetClientClan(ClientID, pMsg->m_pClan);
			Server()->SetClientCountry(ClientID, pMsg->m_Country);
			str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));
			pPlayer->m_TeeInfos.m_UseCustomColor = pMsg->m_UseCustomColor;
			pPlayer->m_TeeInfos.m_ColorBody = pMsg->m_ColorBody;
			pPlayer->m_TeeInfos.m_ColorFeet = pMsg->m_ColorFeet;
			m_pController->OnPlayerInfoChange(pPlayer);
		}
		else if(MsgID == NETMSGTYPE_CL_EMOTICON && !m_World.m_Paused)
		{
			CNetMsg_Cl_Emoticon *pMsg = (CNetMsg_Cl_Emoticon *)pRawMsg;

			if(g_Config.m_SvSpamprotection && pPlayer->m_LastEmote && pPlayer->m_LastEmote+Server()->TickSpeed()*0.125f > Server()->Tick())
				return;

			pPlayer->m_LastEmote = Server()->Tick();

			SendEmoticon(ClientID, pMsg->m_Emoticon);

			CCharacter* pChr = pPlayer->GetCharacter();
			if(pChr)
			{
				switch(pMsg->m_Emoticon)
				{
				case EMOTICON_EXCLAMATION:
				case EMOTICON_GHOST:
				case EMOTICON_QUESTION:
				case EMOTICON_WTF:
						pChr->SetEmoteType(EMOTE_SURPRISE);
						break;
				case EMOTICON_DOTDOT:
				case EMOTICON_DROP:
				case EMOTICON_ZZZ:
						pChr->SetEmoteType(EMOTE_BLINK);
						break;
				case EMOTICON_EYES:
				case EMOTICON_HEARTS:
				case EMOTICON_MUSIC:
						pChr->SetEmoteType(EMOTE_HAPPY);
						break;
				case EMOTICON_OOP:
				case EMOTICON_SORRY:
				case EMOTICON_SUSHI:
						pChr->SetEmoteType(EMOTE_PAIN);
						break;
				case EMOTICON_DEVILTEE:
				case EMOTICON_SPLATTEE:
				case EMOTICON_ZOMG:
						pChr->SetEmoteType(EMOTE_ANGRY);
						break;
					default:
						pChr->SetEmoteType(EMOTE_NORMAL);
						break;
				}
				pChr->SetEmoteStop(Server()->Tick() + 2 * Server()->TickSpeed());
			}

		}
		else if(MsgID == NETMSGTYPE_CL_KILL && !m_World.m_Paused)
		{
			/*if(pPlayer->m_LastKill && pPlayer->m_LastKill+Server()->TickSpeed()*3 > Server()->Tick())
				return;

			pPlayer->m_LastKill = Server()->Tick();
			pPlayer->KillCharacter(WEAPON_SELF);*/
		}
	}
	else
	{
		if(MsgID == NETMSGTYPE_CL_STARTINFO)
		{
			if(pPlayer->m_IsReady)
				return;

			CNetMsg_Cl_StartInfo *pMsg = (CNetMsg_Cl_StartInfo *)pRawMsg;
			pPlayer->m_LastChangeInfo = Server()->Tick();

			// set start infos
			Server()->SetClientName(ClientID, pMsg->m_pName);
			Server()->SetClientClan(ClientID, pMsg->m_pClan);
			Server()->SetClientCountry(ClientID, pMsg->m_Country);
			str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));
			pPlayer->m_TeeInfos.m_UseCustomColor = pMsg->m_UseCustomColor;
			pPlayer->m_TeeInfos.m_ColorBody = pMsg->m_ColorBody;
			pPlayer->m_TeeInfos.m_ColorFeet = pMsg->m_ColorFeet;
			m_pController->OnPlayerInfoChange(pPlayer);

			// send vote options
			/*CNetMsg_Sv_VoteClearOptions ClearMsg;
			Server()->SendPackMsg(&ClearMsg, MSGFLAG_VITAL, ClientID);

			CNetMsg_Sv_VoteOptionListAdd OptionMsg;
			int NumOptions = 0;
			OptionMsg.m_pDescription0 = "";
			OptionMsg.m_pDescription1 = "";
			OptionMsg.m_pDescription2 = "";
			OptionMsg.m_pDescription3 = "";
			OptionMsg.m_pDescription4 = "";
			OptionMsg.m_pDescription5 = "";
			OptionMsg.m_pDescription6 = "";
			OptionMsg.m_pDescription7 = "";
			OptionMsg.m_pDescription8 = "";
			OptionMsg.m_pDescription9 = "";
			OptionMsg.m_pDescription10 = "";
			OptionMsg.m_pDescription11 = "";
			OptionMsg.m_pDescription12 = "";
			OptionMsg.m_pDescription13 = "";
			OptionMsg.m_pDescription14 = "";
			CVoteOptionServer *pCurrent = m_pVoteOptionFirst;
			while(pCurrent)
			{
				switch(NumOptions++)
				{
				case 0: OptionMsg.m_pDescription0 = pCurrent->m_aDescription; break;
				case 1: OptionMsg.m_pDescription1 = pCurrent->m_aDescription; break;
				case 2: OptionMsg.m_pDescription2 = pCurrent->m_aDescription; break;
				case 3: OptionMsg.m_pDescription3 = pCurrent->m_aDescription; break;
				case 4: OptionMsg.m_pDescription4 = pCurrent->m_aDescription; break;
				case 5: OptionMsg.m_pDescription5 = pCurrent->m_aDescription; break;
				case 6: OptionMsg.m_pDescription6 = pCurrent->m_aDescription; break;
				case 7: OptionMsg.m_pDescription7 = pCurrent->m_aDescription; break;
				case 8: OptionMsg.m_pDescription8 = pCurrent->m_aDescription; break;
				case 9: OptionMsg.m_pDescription9 = pCurrent->m_aDescription; break;
				case 10: OptionMsg.m_pDescription10 = pCurrent->m_aDescription; break;
				case 11: OptionMsg.m_pDescription11 = pCurrent->m_aDescription; break;
				case 12: OptionMsg.m_pDescription12 = pCurrent->m_aDescription; break;
				case 13: OptionMsg.m_pDescription13 = pCurrent->m_aDescription; break;
				case 14:
					{
						OptionMsg.m_pDescription14 = pCurrent->m_aDescription;
						OptionMsg.m_NumOptions = NumOptions;
						Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
						OptionMsg = CNetMsg_Sv_VoteOptionListAdd();
						NumOptions = 0;
						OptionMsg.m_pDescription1 = "";
						OptionMsg.m_pDescription2 = "";
						OptionMsg.m_pDescription3 = "";
						OptionMsg.m_pDescription4 = "";
						OptionMsg.m_pDescription5 = "";
						OptionMsg.m_pDescription6 = "";
						OptionMsg.m_pDescription7 = "";
						OptionMsg.m_pDescription8 = "";
						OptionMsg.m_pDescription9 = "";
						OptionMsg.m_pDescription10 = "";
						OptionMsg.m_pDescription11 = "";
						OptionMsg.m_pDescription12 = "";
						OptionMsg.m_pDescription13 = "";
						OptionMsg.m_pDescription14 = "";
					}
				}
				pCurrent = pCurrent->m_pNext;
			}
			if(NumOptions > 0)
			{
				OptionMsg.m_NumOptions = NumOptions;
				Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
			}*/

			// send tuning parameters to client
			SendTuningParams(ClientID);

			// client is ready to enter
			pPlayer->m_IsReady = true;
			CNetMsg_Sv_ReadyToEnter m;
			Server()->SendPackMsg(&m, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientID);
		}
	}
}

void CGameContext::ConSetIngameTime(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(USE_REAL_TIME)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "SvRealTime has to be deactivated.");
		return;
	}
	int NewTime = pResult->GetInteger(0)*60+pResult->GetInteger(1);
	pSelf->m_IngameTime = NewTime*50;
}

void CGameContext::ConSetIngameTimeSpeed(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(USE_REAL_TIME)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "SvRealTime has to be deactivated.");
		return;
	}
	int NewTime = pResult->GetInteger(0);
	pSelf->m_IngameTimeSpeed = NewTime;
}

void CGameContext::ConTuneParam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pParamName = pResult->GetString(0);
	float NewValue = pResult->GetFloat(1);

	if(pSelf->Tuning()->Set(pParamName, NewValue))
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "%s changed to %.2f", pParamName, NewValue);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
		pSelf->SetDefaultTunings();
		pSelf->SendTuningParams(-1);
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "No such tuning parameter");
}

void CGameContext::ConTuneReset(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CTuningParams TuningParams;
	*pSelf->Tuning() = TuningParams;
	pSelf->SetDefaultTunings();
	pSelf->SendTuningParams(-1);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "Tuning reset");
}

void CGameContext::ConTuneDump(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[256];
	for(int i = 0; i < pSelf->Tuning()->Num(); i++)
	{
		float v;
		pSelf->Tuning()->Get(i, &v);
		str_format(aBuf, sizeof(aBuf), "%s %.2f", pSelf->Tuning()->m_apNames[i], v);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
	}
}

void CGameContext::ConPause(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(pSelf->m_pController->IsGameOver())
		return;

	pSelf->m_World.m_Paused ^= 1;
}

void CGameContext::ConChangeMap(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pController->ChangeMap(pResult->NumArguments() ? pResult->GetString(0) : "");
}

void CGameContext::ConRestart(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pResult->NumArguments())
		pSelf->m_pController->DoWarmup(pResult->GetInteger(0));
	else
		pSelf->m_pController->StartRound();
}

void CGameContext::ConBroadcast(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendBroadcast(pResult->GetString(0), -1);
}

void CGameContext::ConSay(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, pResult->GetString(0));
}

void CGameContext::ConSetTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	int Team = clamp(pResult->GetInteger(1), -1, 1);
	int Delay = pResult->NumArguments()>2 ? pResult->GetInteger(2) : 0;
	if(!pSelf->m_apPlayers[ClientID])
		return;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "moved client %d to team %d", ClientID, Team);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	pSelf->m_apPlayers[ClientID]->m_TeamChangeTick = pSelf->Server()->Tick()+pSelf->Server()->TickSpeed()*Delay*60;
	pSelf->m_apPlayers[ClientID]->SetTeam(Team);
	(void)pSelf->m_pController->CheckTeamBalance();
}

void CGameContext::ConSetTeamAll(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Team = clamp(pResult->GetInteger(0), -1, 1);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "All players were moved to the %s", pSelf->m_pController->GetTeamName(Team));
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

	for(int i = 0; i < MAX_CLIENTS; ++i)
		if(pSelf->m_apPlayers[i])
			pSelf->m_apPlayers[i]->SetTeam(Team, false);

	(void)pSelf->m_pController->CheckTeamBalance();
}

void CGameContext::ConSwapTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SwapTeams();
}

void CGameContext::ConShuffleTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController->IsTeamplay())
		return;

	int CounterRed = 0;
	int CounterBlue = 0;
	int PlayerTeam = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
		if(pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			++PlayerTeam;
	PlayerTeam = (PlayerTeam+1)/2;
	
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, "Teams were shuffled");

	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
		{
			if(CounterRed == PlayerTeam)
				pSelf->m_apPlayers[i]->SetTeam(TEAM_BLUE, false);
			else if(CounterBlue == PlayerTeam)
				pSelf->m_apPlayers[i]->SetTeam(TEAM_RED, false);
			else
			{	
				if(rand() % 2)
				{
					pSelf->m_apPlayers[i]->SetTeam(TEAM_BLUE, false);
					++CounterBlue;
				}
				else
				{
					pSelf->m_apPlayers[i]->SetTeam(TEAM_RED, false);
					++CounterRed;
				}
			}
		}
	}

	(void)pSelf->m_pController->CheckTeamBalance();
}

void CGameContext::ConLockTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_LockTeams ^= 1;
	if(pSelf->m_LockTeams)
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "Teams were locked");
	else
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "Teams were unlocked");
}

void CGameContext::ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
	{
		CNetMsg_Sv_Motd Msg;
		Msg.m_pMessage = g_Config.m_SvMotd;
		CGameContext *pSelf = (CGameContext *)pUserData;
		for(int i = 0; i < MAX_CLIENTS; ++i)
			if(pSelf->m_apPlayers[i])
				pSelf->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
	}
}

void CGameContext::ConRevive(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = pResult->GetInteger(0);
	CCharacter *pChr = pSelf->GetPlayerChar(ClientID);

	if(pSelf->Server()->ClientIngame(ClientID) == false || !pChr)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "No player with this ClientID is online");
		return;
	}

	if(pChr->HasHealth())
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Player could not be revived, because he's still alive");
		return;
	}

	if(pChr->Revive() == false)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Player could not be revived.");
		return;
	}
}

void CGameContext::ConSpectate(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = pResult->GetInteger(0);
	int SpectatingID = pResult->GetInteger(1);
	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];

	if(ClientID == SpectatingID)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Dont spectate yourself");
		return;
	}

	if(pSelf->Server()->ClientIngame(ClientID) == false || !pPlayer)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Spectating client not online");
		return;
	}

	if(pSelf->Server()->ClientIngame(SpectatingID) == false && SpectatingID != -1)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Client to spectate not online");
		return;
	}

	pPlayer->Spectate(SpectatingID);
}

void CGameContext::ConDevPos(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = pResult->GetInteger(0);

	if(pSelf->m_pDevMap == NULL)
		return;

	if(pSelf->Server()->ClientIngame(ClientID) == false)
		return;
	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	if(!pPlayer)
		return;

	if(pPlayer->GetMap() == pSelf->m_pDevMap)
		pPlayer->GetCore()->m_Pos = pSelf->m_DevPos;
	else
	{
		pPlayer->m_pWantedMap = pSelf->m_pDevMap;
		pPlayer->PlayerInfo()->m_TempSpawnPos = pSelf->m_DevPos;
	}
}

void CGameContext::ConTele(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = pResult->GetInteger(0);
	int To = pResult->GetInteger(1);
	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	CPlayer *pToPlayer = pSelf->m_apPlayers[To];

	if(ClientID == To)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Dont teleport to yourself");
		return;
	}

	if(pSelf->Server()->ClientIngame(ClientID) == false || !pPlayer)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Teleporting client not online");
		return;
	}

	if(pSelf->Server()->ClientIngame(To) == false && !pToPlayer)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Client to teleport not online");
		return;
	}

	if(pPlayer->GetMap() == pToPlayer->GetMap())
		pPlayer->GetCore()->m_Pos = pToPlayer->GetCore()->m_Pos;
	else
	{
		pPlayer->m_pWantedMap = pToPlayer->GetMap();
		pPlayer->PlayerInfo()->m_TempSpawnPos = pToPlayer->GetCore()->m_Pos;
	}
}

void CGameContext::ConInvTarget(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = pResult->GetInteger(0);

	if(pSelf->Server()->ClientIngame(ClientID) == false)
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	if(!pPlayer)
		return;

	pPlayer->PlayerInfo()->m_InvTarget = !pPlayer->PlayerInfo()->m_InvTarget;

	if(pPlayer->PlayerInfo()->m_InvTarget)
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invtarget Activated.");
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invtarget Deactivated.");
}

void CGameContext::ConSpawnWeapon(IConsole::IResult *pResult, void *pUserData)
{
	char aBuf[256];
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = pResult->GetInteger(0);
	const char *pWeaponName = pResult->GetString(1);
	int Damage = pResult->GetInteger(2);
	CCharacter *pChr = pSelf->GetPlayerChar(ClientID);
	CWeapon *pWeapon = NULL;

	if(pSelf->Server()->ClientIngame(ClientID) == false || pChr == NULL)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Player not online.");
		return;
	}

	pWeapon = CreateWeapon(GetWeaponType(pWeaponName), pSelf, NULL, Damage);

	if(pWeapon == NULL)
	{
		str_format(aBuf, sizeof(aBuf), "Weapon %s not found.", pWeaponName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	vec2 Vel = normalize(vec2((rand()%200-100)*0.01f, -1))*16.0f;
	new CWeaponPickup(&pSelf->m_World, pChr->Map(), pChr->m_Pos, Vel, pChr->GetPlayer()->GetCID(), pWeapon);

	str_format(aBuf, sizeof(aBuf), "Weapon %s Spawned for %s.", pWeaponName, pSelf->Server()->ClientName(ClientID));
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

void CGameContext::OnConsoleInit()
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();

	Console()->Register("SetTime", "ii", CFGFLAG_SERVER, ConSetIngameTime, this, "Manipulate ingame time (in i:i).");
	Console()->Register("SetTimeSpeed", "i", CFGFLAG_SERVER, ConSetIngameTimeSpeed, this, "Defines how fast the time passes.");

	Console()->Register("tune", "si", CFGFLAG_SERVER, ConTuneParam, this, "Tune variable to value");
	Console()->Register("tune_reset", "", CFGFLAG_SERVER, ConTuneReset, this, "Reset tuning");
	Console()->Register("tune_dump", "", CFGFLAG_SERVER, ConTuneDump, this, "Dump tuning");

	Console()->Register("pause", "", CFGFLAG_SERVER, ConPause, this, "Pause/unpause game");
	Console()->Register("change_map", "?r", CFGFLAG_SERVER|CFGFLAG_STORE, ConChangeMap, this, "Change map");
	Console()->Register("restart", "?i", CFGFLAG_SERVER|CFGFLAG_STORE, ConRestart, this, "Restart in x seconds (0 = abort)");
	Console()->Register("broadcast", "r", CFGFLAG_SERVER, ConBroadcast, this, "Broadcast message");
	Console()->Register("say", "r", CFGFLAG_SERVER, ConSay, this, "Say in chat");
	Console()->Register("set_team", "ii?i", CFGFLAG_SERVER, ConSetTeam, this, "Set team of player to team");
	Console()->Register("set_team_all", "i", CFGFLAG_SERVER, ConSetTeamAll, this, "Set team of all players to team");
	Console()->Register("swap_teams", "", CFGFLAG_SERVER, ConSwapTeams, this, "Swap the current teams");
	Console()->Register("shuffle_teams", "", CFGFLAG_SERVER, ConShuffleTeams, this, "Shuffle the current teams");
	Console()->Register("lock_teams", "", CFGFLAG_SERVER, ConLockTeams, this, "Lock/unlock teams");

	Console()->Chain("sv_motd", ConchainSpecialMotdupdate, this);

	Console()->Register("revive", "i", CFGFLAG_SERVER, ConRevive, this, "Revive a Player");
	Console()->Register("spectate", "ii", CFGFLAG_SERVER, ConSpectate, this, "Force a Player to spectate another Player");
	Console()->Register("devpos", "i", CFGFLAG_SERVER, ConDevPos, this, "Only for Developer");
	Console()->Register("tele", "ii", CFGFLAG_SERVER, ConTele, this, "Teleport tees");
	Console()->Register("inv_target", "i", CFGFLAG_SERVER, ConInvTarget, this, "Tee is not a target anymore.");
	Console()->Register("spawn_weapon", "isi", CFGFLAG_SERVER, ConSpawnWeapon, this, "Spawn a weapon near a tee.");
}

void CGameContext::InitTile(int Index, vec2 Pos, CMap *pMap)
{
	if(Index == TILE_DEV_TEL)
	{
		m_pDevMap = pMap;
		m_DevPos = Pos;
	}

	pMap->WorldSection()->InitTile(Index, Pos);
}

void CGameContext::InitExTile(int Index, vec2 Pos, CMap *pMap, char *pArgs)
{
	if(Index == EXTILE_CHEST_DISPLAY)
	{
		int ChestID = GetMapInteger(&pArgs, "Chest-Display", Pos, pMap);
		CChest *pChest = NULL;

		if (ChestID >= 0)
		{
			int NumChests = sizeof(s_aChests) / sizeof(s_aChests[0]);
			if (ChestID >= 0 && ChestID < NumChests)
				pChest = &s_aChests[ChestID];
		}
		else
		{
			int RealChestID = -(ChestID + 1);

			if (RealChestID >= 0 && RealChestID < MAX_CHESTS_PER_MAP)
				pChest = &pMap->WorldSection()->m_aChests[RealChestID];
		}

		if(pChest)
			pChest->SetDisplayPosition(pMap, Pos-vec2(16, 0), 3*32, Server()->SnapNewID());
		else
			dbg_msg("Chest-Display", "Error at Pos %f, %f on Map %s", Pos.x, Pos.y, pMap->GetName());
	}
	else if(Index == EXTILE_DOOR)
	{
		int ID = GetMapInteger(&pArgs, "Door", Pos, pMap);
		int Type = GetMapInteger(&pArgs, "Door", Pos, pMap);
		int Time = GetMapInteger(&pArgs, "Door", Pos, pMap);
		new CDoor(&m_World, pMap, Pos, Type, ID, Time);
	}
	else if(Index == EXTILE_DOOR_TRIGGER)
	{
		int ID = GetMapInteger(&pArgs, "Door-Trigger", Pos, pMap);
		int Vis = GetMapInteger(&pArgs, "Door-Trigger", Pos, pMap);

		if(Vis)
			new CDoorTrigger(&m_World, pMap, Pos);
	}
	else if(Index == EXTILE_BOLDER)
	{
		int ID = GetMapInteger(&pArgs, "Bolder", Pos, pMap);
		int Damage = GetMapInteger(&pArgs, "Bolder", Pos, pMap);
		new CBolderSpawner(&m_World, pMap, Pos, Damage, ID);
	}
	else if(Index == EXTILE_WEAPON_MOUNT)
	{
		char *pType = GetMapString(&pArgs, "Weapon-Mount", Pos, pMap);
		int Damage = GetMapInteger(&pArgs, "Weapon-Mount", Pos, pMap);
		
		CWeapon *pWepaon = CreateWeapon(GetWeaponType(pType), this, NULL, Damage);
		if(pWepaon == NULL)
		{
			dbg_msg("Weapon-Mount", "Error at Pos %f, %f on Map %s", Pos.x, Pos.y, pMap->GetName());
			return;
		}

		new CWeaponMount(&m_World, pMap, Pos, pWepaon);
	}

	pMap->WorldSection()->InitExTile(Index, Pos, pArgs);
}

void CGameContext::InitMapTiles(CMap *pMap)
{
	CMapItemLayerTilemap *pTileMap = pMap->Layers()->GameLayer();
	CTile *pTiles = (CTile *)pMap->GetEngineMap()->GetData(pTileMap->m_Data);

	for(int y = 0; y < pTileMap->m_Height; y++)
	{
		for(int x = 0; x < pTileMap->m_Width; x++)
		{
			int Index = pTiles[y*pTileMap->m_Width+x].m_Index;

			if(!Index)
				continue;

			vec2 Pos(x*32.0f+16.0f, y*32.0f+16.0f);

			if(Index >= ENTITY_OFFSET)
				m_pController->OnEntity(Index-ENTITY_OFFSET, Pos, pMap);
			else
				InitTile(Index, Pos, pMap);
		}
	}

	if(pMap->Layers()->m_ExTileLayerUsed)
	{
		CMapItemLayerTilemap *pExTileMap = pMap->Layers()->ExTileLayer();
		CTile *pExTiles = (CTile *)pMap->GetEngineMap()->GetData(pExTileMap->m_Data);
		CExTile *pExArgs = (CExTile *)pMap->GetEngineMap()->GetData(pExTileMap->m_ExData);
		
		for(int y = 0; y < pExTileMap->m_Height; y++)
		{
			for(int x = 0; x < pExTileMap->m_Width; x++)
			{
				int Index = pExTiles[y*pExTileMap->m_Width+x].m_Index;

				if(!Index)
					continue;

				vec2 Pos(x*32.0f+16.0f, y*32.0f+16.0f);

				char aArgs[MAX_EXTENTED_STR];
				mem_copy(aArgs, pExArgs[y*pExTileMap->m_Width+x].m_ExArgs, sizeof(aArgs));

				InitExTile(Index, Pos, pMap, aArgs);
			}
		}
	}
}

void CGameContext::OnInit(/*class IKernel *pKernel*/)
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_World.SetGameServer(this);
	m_Events.SetGameServer(this);
	Server()->m_MapLoader.SetGameServer(this);

	m_ChatCommands.Init(this);

	SetDefaultTunings();

	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		Server()->SnapSetStaticsize(i, m_NetObjHandler.GetObjSize(i));


	m_pController = new CGameController(this);


#ifdef CONF_DEBUG
	if(g_Config.m_DbgDummies)
	{
		for(int i = 0; i < g_Config.m_DbgDummies ; i++)
		{
			OnClientConnected(MAX_CLIENTS-i-1);
		}
	}
#endif
}

void CGameContext::SetDefaultTunings()
{
	Tuning()->Set("gun_speed", 10);
	Tuning()->Set("gun_lifetime", 100);
	Tuning()->Set("shotgun_speed", 10);
	Tuning()->Set("shotgun_lifetime", 100);
	Tuning()->Set("grenade_speed", 10);
	Tuning()->Set("grenade_lifetime", 100);

	Tuning()->Set("laser_bounce_delay", 700);
}


void CGameContext::OnShutdown()
{
	delete m_pController;
	m_pController = 0;
	Clear();
}

void CGameContext::RenderChests(CChest *pFirst, int Num, int Size, int ClientID)
{
	for (int i = 0; i < Num; i++)
	{
		CChest *pChest = pFirst + Size * i;
		if (pChest == NULL)
			continue;

		if (pChest->GetDisplaySet() == false)
			continue;

		vec2 DisplayPos;
		vec2 DisplayEndPos;
		if (pChest->GetDisplayPositions(&DisplayPos, &DisplayEndPos) == false)
			continue;

		if (NetworkClipped(pChest->GetDisplayMap(), ClientID, DisplayPos))
			continue;

		if (pChest->GetSnapID() == -1)
			continue;

		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, pChest->GetSnapID(), sizeof(CNetObj_Laser)));
		if (!pObj)
			continue;

		pObj->m_X = (int)DisplayEndPos.x;
		pObj->m_Y = (int)DisplayEndPos.y;
		pObj->m_FromX = (int)DisplayPos.x;
		pObj->m_FromY = (int)DisplayPos.y;
		pObj->m_StartTick = Server()->Tick() - 1;
	}
}

void CGameContext::OnSnap(int ClientID)
{
	// add tuning to demo
	CTuningParams StandardTuning;
	if(ClientID == -1 && Server()->DemoRecorder_IsRecording() && mem_comp(&StandardTuning, &m_Tuning, sizeof(CTuningParams)) != 0)
	{
		CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
		int *pParams = (int *)&m_Tuning;
		for(unsigned i = 0; i < sizeof(m_Tuning)/sizeof(int); i++)
			Msg.AddInt(pParams[i]);
		Server()->SendMsg(&Msg, MSGFLAG_RECORD|MSGFLAG_NOSEND, ClientID);
	}

	m_World.Snap(ClientID);
	m_pController->Snap(ClientID);
	m_Events.Snap(ClientID);

	//chests;
	int NumChests = sizeof(s_aChests)/sizeof(s_aChests[0]);
	RenderChests(&s_aChests[0], NumChests, 1, ClientID);

	CPlayer *pPlayer = m_apPlayers[ClientID];
	if (pPlayer)
	{
		CWorldSection *pWorldSection = pPlayer->GetMap()->WorldSection();
		RenderChests(&pWorldSection->m_aChests[0], MAX_CHESTS_PER_MAP, 1, ClientID);
	}
}
void CGameContext::OnPreSnap() {}
void CGameContext::OnPostSnap()
{
	m_Events.Clear();
}

bool CGameContext::IsClientReady(int ClientID)
{
	return m_apPlayers[ClientID] && m_apPlayers[ClientID]->m_IsReady ? true : false;
}

bool CGameContext::IsClientPlayer(int ClientID)
{
	return m_apPlayers[ClientID] && m_apPlayers[ClientID]->GetTeam() == TEAM_SPECTATORS ? false : true;
}

const char *CGameContext::GameType() { return m_pController && m_pController->m_pGameType ? m_pController->m_pGameType : ""; }
const char *CGameContext::Version() { return GAME_VERSION; }
const char *CGameContext::NetVersion() { return GAME_NETVERSION; }

IGameServer *CreateGameServer() { return new CGameContext; }
