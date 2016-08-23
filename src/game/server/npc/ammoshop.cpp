
#include <engine/shared/config.h>
#include <game/balancing.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/money.h>
#include <game/server/entities/experience.h>
#include <game/server/elements/questionhandler.h>

#include "ammoshop.h"

#define NPC_MOVE_FARWORD_PREDICT 5

static char MoneyChar[] = {-30, -120, -121, 0};

CAmmoShop::CAmmoShop(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos)
	: CNpc(pGameServer, pMap, pNpcHolder, Pos, INTERFLAG_COL, NPC_AMMOSHOP)
{
	m_SpawnPos = Pos;
	m_RandJump = false;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aSell[i].m_Close = false;
	}
}

void CAmmoShop::BuyAmmo(int ClientID)
{
	char aBuf[256];
	char aReason[64];
	CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
	if(!pChr || pChr->Map() != Map())
		return;

	CPlayer *pPlayer = pChr->GetPlayer();
	CWeapon *pWeapon = pChr->CurrentWeapon();
	if(!pWeapon)
		return;

	int PlayerItemID = GameServer()->m_World.GetPlayerItemId(this, Map());
	int Costs = pWeapon->AmmoCosts();
	bool CanBuy = true;

	if(pPlayer->AccountInfo()->m_Money < Costs)
	{
		str_format(aBuf, sizeof(aBuf), "%s: You need %i%s to buy this, but you have %i%s.", GameServer()->Server()->ClientName(ClientID), Costs, MoneyChar, pPlayer->AccountInfo()->m_Money, MoneyChar);
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 0;
		Msg.m_ClientID = PlayerItemID;
		Msg.m_pMessage = aBuf;
		Server()->SendChaMsgBot(&Msg, MSGFLAG_VITAL, ClientID);
		CanBuy = false;
	}

	if(!CanBuy)
		return;

	pPlayer->AccountInfo()->m_Money -= Costs;
	pWeapon->AddAmmo(5);

	str_format(aBuf, sizeof(aBuf), "%s", s_aWeaponNames[pWeapon->GetType()]);
	str_format(aReason, sizeof(aReason), "%i%s", pWeapon->AmmoCosts(), MoneyChar);
	pPlayer->QuestionHandler()->Ask(this, CQuestionHandler::TYPE_SELL, BuyAmmoStatic, NULL, aBuf, aReason);
}

void CAmmoShop::BuyAmmoStatic(void *pInfo, int ClientID)
{
	CAmmoShop *pThis = (CAmmoShop *)pInfo;
	pThis->BuyAmmo(ClientID);
}

void CAmmoShop::NoTargetInput()
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

void CAmmoShop::SetExtraCollision()
{
	Map()->Collision()->SetExtraCollision(TILE_NPC_COL);
}

void CAmmoShop::Tick()
{
	SetExtraCollision();

	SetInput();
	UpdateCore(1.0f);
	Map()->Collision()->ResetExtraCollision();

	char aBuf[64];
	char aReason[64];

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(Server()->ClientIngame(i) == false)
			continue;

		CCharacter *pChr = GameServer()->GetPlayerChar(i);
		if(!pChr || pChr->Map() != Map())
			continue;

		float dist = distance(GetPos(), pChr->m_Pos);
		if(m_aSell[i].m_Close == false)
		{
			if(dist < 115)
			{
				CPlayer *pPlayer = GameServer()->m_apPlayers[i];
				CWeapon *pWeapon = pChr->CurrentWeapon();
				if(pPlayer && pWeapon)
				{
					if(pWeapon->MaxAmmo() == -1)
					{
						str_format(aBuf, sizeof(aBuf), "%s", s_aWeaponNames[pWeapon->GetType()]);
						str_format(aReason, sizeof(aReason), "Has no ammo");
						pPlayer->QuestionHandler()->Ask(this, CQuestionHandler::TYPE_SELL, NULL, NULL, aBuf, aReason);
					}
					else
					{
						str_format(aBuf, sizeof(aBuf), "%s", s_aWeaponNames[pWeapon->GetType()]);
						str_format(aReason, sizeof(aReason), "%i%s", pWeapon->AmmoCosts(), MoneyChar);
						pPlayer->QuestionHandler()->Ask(this, CQuestionHandler::TYPE_SELL, BuyAmmoStatic, NULL, aBuf, aReason);
					}

					if(m_aSell[i].m_ChatTime < Server()->Tick())
					{
						str_format(aBuf, sizeof(aBuf), "%s: Buy new ammo for your weapon.", GameServer()->Server()->ClientName(i));
						int PlayerItemID = GameServer()->m_World.GetPlayerItemId(this, Map());
						CNetMsg_Sv_Chat Msg;
						Msg.m_Team = 0;
						Msg.m_ClientID = PlayerItemID;
						Msg.m_pMessage = aBuf;
						Server()->SendChaMsgBot(&Msg, MSGFLAG_VITAL, i);

						m_aSell[i].m_ChatTime = Server()->Tick() + Server()->TickSpeed() * 60;
					}

					m_aSell[i].m_WeaponType = pWeapon->GetType();
				}
				m_aSell[i].m_Close = true;
			}
		}
		else
		{
			CWeapon *pWeapon = pChr->CurrentWeapon();
			bool NewWeapon = false;
			if(pWeapon && m_aSell[i].m_WeaponType != pWeapon->GetType())
				NewWeapon = true;

			if(dist > 128 || NewWeapon)
			{
				CPlayer *pPlayer = GameServer()->m_apPlayers[i];
				if(pPlayer)
				{
					pPlayer->QuestionHandler()->EndQuestion(CQuestionHandler::TYPE_SELL);
				}
				m_aSell[i].m_Close = false;
			}
		}
	}
}

void CAmmoShop::Snap(int SnappingClient)
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

void CAmmoShop::SubSpawn()
{
	GameServer()->CreatePlayerSpawn(GetPos(), Map());
}

void CAmmoShop::OnDeath(int From, int Weapon, vec2 DeathVel)
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

void CAmmoShop::SetInput()
{
	NoTargetInput();
}