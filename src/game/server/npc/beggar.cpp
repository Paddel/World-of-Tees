
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/money.h>
#include <game/server/entities/experience.h>

#include "beggar.h"

#define NPC_MOVE_FARWORD_PREDICT 5

CBeggar::CBeggar(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos)
	: CNpc(pGameServer, pMap, pNpcHolder, Pos, INTERFLAG_COL, NPC_BEGGAR)
{
	m_SpawnPos = Pos;
	m_RandJump = false;
}

void CBeggar::NoTargetInput()
{
	if(m_MoveEyeTime < Server()->Tick())
	{
		m_Input.m_TargetX = rand()%(900*2)-900;
		m_Input.m_TargetY = rand()%(900*2)-900;
		m_MoveEyeTime = Server()->Tick()+(Server()->TickSpeed()/100.0f*(float)(rand()%400+200));
	}

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
	}

	if(m_Core.m_Colliding)
	{
		if(!m_Input.m_Jump)
		{
			m_Input.m_Jump = true;

			SetEmote(EMOTE_SURPRISE, Server()->Tick() + Server()->TickSpeed());
			if(rand()%3 == 0)
			{
				int RandEmoticon = rand()%4;
				int Emoticon = -1;
				switch(RandEmoticon)
				{
				case 0: Emoticon = EMOTICON_GHOST; break;
				case 1: Emoticon = EMOTICON_WTF; break;
				case 2: Emoticon = EMOTICON_QUESTION; break;
				case 3: Emoticon = EMOTICON_EXCLAMATION; break;
				}

				if(Emoticon != -1)
					DoEmoticon(Emoticon);
			}

			m_Input.m_TargetX = rand()%(900*2)-900;
			m_Input.m_TargetY = rand()%(900*2)-900;
			m_MoveEyeTime = Server()->Tick()+(Server()->TickSpeed()/100.0f*(float)(rand()%600+300));

		}
		else
			m_Input.m_Jump = false;
	}
}

void CBeggar::SetExtraCollision()
{
	Map()->Collision()->SetExtraCollision(TILE_NPC_COL);
}

void CBeggar::Tick()
{
	SetExtraCollision();
	if(Map()->Collision()->TestBox(GetPos(), vec2(28, 28)))
		m_Core.m_Pos.y -= 4;

	SetInput();
	UpdateCore(1.0f);
	Map()->Collision()->ResetExtraCollision();
}

void CBeggar::Snap(int SnappingClient)
{
	CPlayer *pSnappingPlayer = GameServer()->m_apPlayers[SnappingClient];
	if(pSnappingPlayer->GetMap() != GetMap())
		return;

	int RealID = GameServer()->m_World.GetPlayerItemId(this, Map());
	int id = RealID;
	if (id == -1 || !Server()->Translate(id, SnappingClient)) return;
	// set emote
	if (m_EmoteStop < Server()->Tick())
	{
		m_EmoteType = EMOTE_NORMAL;
		m_EmoteStop = -1;
	}

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

	pCharacter->m_Emote = m_EmoteType;
}

void CBeggar::SubSpawn()
{
	GameServer()->CreatePlayerSpawn(GetPos(), Map());
}

void CBeggar::OnDeath(int From, int Weapon, vec2 DeathVel)
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
	GameServer()->CreateDeath(GetPos(), -1, Map());
}

void CBeggar::SetInput()
{
	NoTargetInput();
}