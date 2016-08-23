
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/money.h>
#include <game/server/entities/experience.h>

#include "helper.h"

#define NPC_MOVE_FARWORD_PREDICT 5

static char *s_aHelpText[] = {"Welcome.\n\nOn the right side you can find a door.\nHammering it while you're\nstanding inside the door\nlet you leave this building.\nFeel free to explore the town.\n\n\nPress right mouse to leave to dialog."
, "Going outside at night\nis very dangerous.\nIf you die, you\nloose much money."
, "Hammering this glowing\nstone let you regenerate\nmuch health."
, "Next to you is a chest.\nYou can open it by\npressing 'fire'\nwhile you are stinding\ninside of the chest.\n\nIf you opened it\na Laer will appear\nunder the chest,\nshowing the cooldown\nof the chest.\nIf there is no laser\nanymore, you can open\nthe chest again."
, "You are about to enter\na boss-room. You won't\nbe able to go back."};
static int s_aHelpShift[] = { 9, 15, 15, 6, 16};//19-num lines

CHelper::CHelper(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos, int HelpingText)
	: CNpc(pGameServer, pMap, pNpcHolder, Pos, INTERFLAG_COL, NPC_HELPER), CTargetAble(&pGameServer->m_World, CTargetAble::TYPE_NPC_VILLIGER, pMap)
{
	m_SpawnPos = Pos;
	m_RandJump = false;
	m_HelpingText = HelpingText;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aChatHelp[i].m_Close = false;
		m_aChatHelp[i].m_ChatTime = 0;
	}
}


bool CHelper::IsEnemy(CTargetAble *pTarget)
{
	if(!pTarget)
		return false;

	int Type = pTarget->GetType();
	if(Type == CTargetAble::TYPE_CHAR)
		return true;
	return false;
}

void CHelper::NoTargetInput()
{
	if(abs(m_SpawnPos.x-GetPos().x) > 64)
	{
		m_Input.m_Direction = clamp((int)(m_SpawnPos.x-GetPos().x), -1, 1);
	}
	else
		m_Input.m_Direction = 0;

	if(IsGrounded())
		m_RandJump = false;
	
	if(m_Input.m_Direction != 0)
	{
		vec2 LookPos = GetPos()+vec2(m_Input.m_Direction*NPC_MOVE_FARWORD_PREDICT*32, 0);
		vec2 OutPos = vec2(0, 0);
		int Hit = Map()->Collision()->IntersectLine(GetPos(), LookPos, &OutPos, NULL);
		if(Hit == TILE_SOLID || Hit == TILE_NOHOOK)
		{
			if(!m_Input.m_Jump)
			{
				if(IsGrounded())//always jump when grounded
					m_Input.m_Jump = 1;
				else
				{
					if(m_Core.m_Vel.y >= 0 && m_RandJump == false)
					{
						m_RandJump = true;
						m_Input.m_Jump = rand()%2;
					}
				}
			}
			else
				m_Input.m_Jump = false;
		}

		if(m_Core.m_Colliding)
		{
			if(!m_Input.m_Jump)
				m_Input.m_Jump = true;
			else
				m_Input.m_Jump = false;
		}
	}
}

void CHelper::SetExtraCollision()
{
	Map()->Collision()->SetExtraCollision(TILE_NPC_COL);
}

void CHelper::Tick()
{
	SetExtraCollision();
	if(Map()->Collision()->TestBox(GetPos(), vec2(GetProximityRadius(), GetProximityRadius())))
		m_Core.m_Pos.y -= 4;

	SetInput();
	UpdateCore(1.0f);
	Map()->Collision()->ResetExtraCollision();

	if(m_EmoticonTime < Server()->Tick())
	{
		int Emote = rand()%3;
		if(Emote == 0)
			Emote = 2;
		else if(Emote == 1)
			Emote = 5;
		else
			Emote = 14;

		DoEmoticon(Emote);
		m_EmoticonTime = Server()->Tick() + (Server()->TickSpeed() / 1000.0f) *(float)( rand()%5000 + 2300);
	}

	char aBuf[256];

	if(m_HelpingText == 0)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(Server()->ClientIngame(i) == false)
				continue;

			CCharacter *pChr = GameServer()->GetPlayerChar(i);
			if(!pChr || pChr->Map() != Map())
				continue;

			float dist = distance(GetPos(), pChr->m_Pos);
			if(m_aChatHelp[i].m_Close == false)
			{
				if(dist < 115)
				{
					if(m_aChatHelp[i].m_ChatTime < Server()->Tick())
					{
						str_format(aBuf, sizeof(aBuf), "%s: If you need help, just hammer me!", Server()->ClientName(i));
						if(rand()%24 == 0)
							str_fcat(aBuf, sizeof(aBuf), " 8====D");
						else
							str_fcat(aBuf, sizeof(aBuf), " <3");

						int PlayerItemID = GameServer()->m_World.GetPlayerItemId(this, Map());

						CNetMsg_Sv_Chat Msg;
						Msg.m_Team = 0;
						Msg.m_ClientID = PlayerItemID;
						Msg.m_pMessage = aBuf;
						Server()->SendChaMsgBot(&Msg, MSGFLAG_VITAL, i);

						m_aChatHelp[i].m_ChatTime = Server()->Tick()+Server()->TickSpeed()*5.0f;
					}

					m_aChatHelp[i].m_Close = true;
				}
			}
			else
			{
				if(dist > 128)
				{
					m_aChatHelp[i].m_Close = false;
				}
			}
		}
	}
}

bool CHelper::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	if(From == -1 || Weapon != WEAP_HAMMER)
		return true;
	CPlayer *pPlayer = GameServer()->m_apPlayers[From];
	if(!pPlayer)
		return true;

	int NumTextes = sizeof(s_aHelpText)/sizeof(s_aHelpText[0]);
	int NumShifts = sizeof(s_aHelpShift)/sizeof(s_aHelpShift[0]);
	if(m_HelpingText >= NumTextes || m_HelpingText >= NumShifts)
		return true;

	pPlayer->TextPopup()->AddText(s_aHelpText[m_HelpingText], s_aHelpShift[m_HelpingText], NULL, NULL);

	return true;
}

void CHelper::Snap(int SnappingClient)
{
	CPlayer *pSnappingPlayer = GameServer()->m_apPlayers[SnappingClient];
	if(pSnappingPlayer->GetMap() != GetMap())
		return;

	int RealID = GameServer()->m_World.GetPlayerItemId(this, Map());
	int id = RealID;
	if (id == -1 || !Server()->Translate(id, SnappingClient)) return;
	//player

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, id, sizeof(CNetObj_ClientInfo)));
	if(!pClientInfo)
		return;

	char aName[128];
	str_format(aName, sizeof(aName), "%s", NpcName());

	StrToInts(&pClientInfo->m_Name0, 4, aName);
	StrToInts(&pClientInfo->m_Clan0, 3, "Npc");
	pClientInfo->m_Country = -1;
	StrToInts(&pClientInfo->m_Skin0, 6, SkinName());
	pClientInfo->m_UseCustomColor = SkinCostumColor();
	pClientInfo->m_ColorBody = SkinColorBody();
	pClientInfo->m_ColorFeet = SkinColorFeet();

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, id, sizeof(CNetObj_PlayerInfo)));
	if(!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = 0;
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_ClientID = id;
	pPlayerInfo->m_Score = 0;
	pPlayerInfo->m_Team = g_Config.m_SvNpcInScoreboard?0:1;

	//character
	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, id, sizeof(CNetObj_Character)));
	if(!pCharacter)
		return;

	pCharacter->m_Tick = 0;
	m_Core.Write(pCharacter);

	pCharacter->m_Emote = m_EmoteType;

	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;

	pCharacter->m_Weapon = Weapon();
	pCharacter->m_AttackTick = 0;

	pCharacter->m_PlayerFlags = 0;

	pCharacter->m_Emote = EMOTE_HAPPY;

	CCharacter *pChr = GameServer()->GetPlayerChar(SnappingClient);
	if(pChr)
	{
		vec2 Dir = pChr->m_Pos - GetPos();

		float a = 0;
		if(Dir.x == 0)
			a = atanf((float)Dir.y);
		else
			a = atanf((float)Dir.y/(float)Dir.x);

		if(Dir.x < 0)
			a = a+pi;

		pCharacter->m_Angle = (int)(a*256.0f);
	}
}

void CHelper::SubSpawn()
{
	GameServer()->CreatePlayerSpawn(GetPos(), Map());
}

void CHelper::OnDeath(int From, int Weapon, vec2 DeathVel)
{
	if(From != -1)
	{
		int PlayerItemIDKiller = GameServer()->m_World.GetPlayerItemId(GameServer()->m_apPlayers[From], Map());
		int PlayerItemIDThis = GameServer()->m_World.GetPlayerItemId(this, Map());
		CNetMsg_Sv_KillMsg Msg;
		Msg.m_Killer = PlayerItemIDKiller;
		Msg.m_Victim = PlayerItemIDThis;
		Msg.m_Weapon = Weapon;
		Msg.m_ModeSpecial = 0;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, From);

		int Amount = rand()%MoneyAddAmount()+MoneyAmount();
		if(rand()%4 == 0)
		{
			new CMoney(&GameServer()->m_World, Map(), GetPos(), DeathVel*0.7f, Amount/2, From);
			new CMoney(&GameServer()->m_World, Map(), GetPos(), DeathVel*1.5f, Amount/2, From);
		}
		else
		{
			new CMoney(&GameServer()->m_World, Map(), GetPos(), DeathVel, Amount, From);
		}

		new CExp(&GameServer()->m_World, Map(), GetPos(), From, Experience());
	}

	OnNpcDeath();
	GameServer()->m_World.RemovePlayerItem(this, Map());
	OnTargetDeath();
	GameServer()->CreateDeath(GetPos(), -1, Map());
}

void CHelper::SetInput()
{
	NoTargetInput();
}