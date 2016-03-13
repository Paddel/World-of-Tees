
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/money.h>
#include <game/server/entities/experience.h>

#include "guard.h"

#define NPC_MOVE_FARWORD_PREDICT 5

CGuard::CGuard(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos)
	: CNpc(pGameServer, pMap, pNpcHolder, Pos, INTERFLAG_COL, NPC_GUARD), CTargetAble(&pGameServer->m_World, CTargetAble::TYPE_NPC_VILLIGER, pMap)
{
	m_pTarget = 0;
	m_AttackTick = 0;
	m_SpawnPos = Pos;

	m_RandJump = false;

	for(int i = 0; i < NUM_TIMES; i++)
		m_aTimes[i] = 0;
}


bool CGuard::IsEnemy(CTargetAble *pTarget)
{
	if(!pTarget)
		return false;

	int Type = pTarget->GetType();
	if(Type == CTargetAble::TYPE_NPC_ENEMY)
		return true;
	return false;
}

void CGuard::NoTargetInput()
{
	m_pTarget = FindClosestTarget(1024, this);

	if(abs(m_SpawnPos.x-GetPos().x) > 64)
	{
		m_Input.m_Direction = clamp((int)(m_SpawnPos.x-GetPos().x), -1, 1);
	}
	else
		m_Input.m_Direction = 0;

	if(IsGrounded())
		m_RandJump = false;
	
	if(m_aTimes[TIME_NOTAR_NEW_EYE_DIR] < Server()->Tick())
	{
		m_Input.m_TargetX = rand()%(900*2)-900;
		m_Input.m_TargetY = rand()%(900*2)-900;
		m_aTimes[TIME_NOTAR_NEW_EYE_DIR] = Server()->Tick()+(Server()->TickSpeed()/100.0f*(float)(rand()%400+200));
	}

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

	m_Input.m_Hook = 0;
}

void CGuard::SetExtraCollision()
{
	Map()->Collision()->SetExtraCollision(TILE_NPC_COL);
}

void CGuard::CheckHealthRegeneration()
{
	if(m_pTarget || m_Health >= MaxHealth() || m_HealthRegTime >= Server()->Tick())
		return;

	m_Health += clamp(MaxHealth()/20, 1, MaxHealth()-m_Health);
	m_HealthRegTime = Server()->Tick()+Server()->TickSpeed()*2.5f;
}

void CGuard::Tick()
{
	SetExtraCollision();
	if(Map()->Collision()->TestBox(GetPos(), vec2(GetProximityRadius(), GetProximityRadius())))
		m_Core.m_Pos.y -= 4;

	SetInput();
	UpdateCore(1.0f);
	Map()->Collision()->ResetExtraCollision();

	CheckHealthRegeneration();

	m_DamageIndHandler.Tick(GameServer(), GetPos(), Map());
}

bool CGuard::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	m_Core.m_Vel += Force;

	if(Dmg)
		m_Health = clamp(m_Health-Dmg, 1, MaxHealth());

	m_DamageIndHandler.TookDamage(Dmg);

	// do damage Hit sound
	if(From >= 0 && GameServer()->m_apPlayers[From])
	{
		int Mask = CmaskOne(From);
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->PlayerInfo()->m_SpectatorID == From)
				Mask |= CmaskOne(i);
		}
		GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Map(), Mask);
	}

	// check for death
	if(!Alive())
	{
		OnDeath(From, Weapon, Force);

		// set attacker's face to happy (taunt!)
		if (From >= 0 && GameServer()->m_apPlayers[From])
		{
			CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
			if(pChr)
				pChr->SetEmote(EMOTE_HAPPY, Server()->Tick() + Server()->TickSpeed());
		}

		return false;
	}

	if (Dmg > 2)
		GameServer()->CreateSound(GetPos(), SOUND_PLAYER_PAIN_LONG, Map());
	else
		GameServer()->CreateSound(GetPos(), SOUND_PLAYER_PAIN_SHORT, Map());

	m_EmoteType = EMOTE_PAIN;
	m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;

	return true;
}

void CGuard::Snap(int SnappingClient)
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
	int LifeRate = (m_Health/(float)MaxHealth())*100.0f;
	str_format(aName, sizeof(aName), "%s", NpcName());
	if(LifeRate < 100)
		str_format(aName, sizeof(aName), "%s [%i%%]", aName, LifeRate);

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
	pCharacter->m_AttackTick = m_AttackTick;

	pCharacter->m_PlayerFlags = 0;

	if(pCharacter->m_HookedPlayer != -1)
	{
		if(m_Core.m_BotHooked)
		{
			int PlayerItemID = pCharacter->m_HookedPlayer;
			if (Server()->Translate(PlayerItemID, SnappingClient))
				pCharacter->m_HookedPlayer = PlayerItemID;
			else
				pCharacter->m_HookedPlayer = -1;
		}
		else
		{
			int PlayerItemID = GameServer()->m_World.GetPlayerItemId(GameServer()->m_apPlayers[pCharacter->m_HookedPlayer], Map());
			if (Server()->Translate(PlayerItemID, SnappingClient))
				pCharacter->m_HookedPlayer = PlayerItemID;
			else
				pCharacter->m_HookedPlayer = -1;
		}
	}
}

void CGuard::SubSpawn()
{
	GameServer()->CreatePlayerSpawn(GetPos(), Map());
	m_Health = MaxHealth();
}

void CGuard::OnDeath(int From, int Weapon, vec2 DeathVel)
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

	m_DamageIndHandler.OnDeath(GameServer(), GetPos(), Map());
}

void CGuard::OnNotActive()
{
	m_Health = MaxHealth();
}

bool CGuard::DealDamage(CTargetAble *pTarget, vec2 Force, int Damage, int Weapon)
{
	return pTarget->TakeDamage(Force, Damage, -1, Weapon);
}

void CGuard::ResetEnemy(CTargetAble *pTarget)
{
	if(m_pTarget == pTarget)
	{
		m_pTarget = NULL;
		m_aTimes[TIME_TAR_COL_LOOSE] = 0;
	}
}

void CGuard::TargetInput()
{
	vec2 TargetPos = m_pTarget->GetClosestPos(GetPos());
	float dist = distance(GetPos(), TargetPos);

	m_Input.m_TargetX = TargetPos.x-GetPos().x;
	m_Input.m_TargetY = TargetPos.y-GetPos().y;

	if(dist >= 48)
	{
		m_Input.m_Direction = TargetPos.x-GetPos().x;

		if(!m_Input.m_Jump)
		{
			if(TargetPos.y+16 < GetPos().y)
			{
				if(IsGrounded())//always jump when grounded
					m_Input.m_Jump = 1;
				else if(m_Core.m_Vel.y >= 0)
					m_Input.m_Jump = 1;
			}
		}
		else
			m_Input.m_Jump = 0;
	}
	else
	{
		m_Input.m_Direction = 0;
	}

	if(dist <= 128)
	{
		if(!Map()->Collision()->IntersectLine(GetPos(), TargetPos, NULL, NULL))
		{
			if(m_aTimes[TIME_TAR_ATTACK] < Server()->Tick())
			{
				GameServer()->CreateHammerHit((TargetPos+GetPos())*0.5f, Map());
				DealDamage(m_pTarget, vec2(0, 0), BaseDamage(), WEAPON_HAMMER);
				m_AttackTick = Server()->Tick();
				m_aTimes[TIME_TAR_ATTACK] = Server()->Tick()+Server()->TickSpeed()/1000.0f*AttackSpeed();
			}
		}
	}
	else
		m_aTimes[TIME_TAR_ATTACK] = Server()->Tick()+Server()->TickSpeed()/1000.0f*AttackSpeed();

	//hook
	if(dist < 360)
	{
		if((m_Core.m_HookState != HOOK_FLYING && m_Core.m_HookState != HOOK_GRABBED) || (m_Core.m_HookState == HOOK_GRABBED && m_Core.m_HookedPlayer == -1))
		{
			if(m_Input.m_Hook == 1)
				m_Input.m_Hook = 0;
			else
				m_Input.m_Hook = 1;
		}
	}

	if(m_aTimes[TIME_TAR_COL_CHECK] < Server()->Tick())
	{

		int Hit = Map()->Collision()->IntersectLine(GetPos(), TargetPos, NULL, NULL);
		if(Hit)
		{
			m_aTimes[TIME_TAR_COL_LOOSE]++;
		}
		else
			m_aTimes[TIME_TAR_COL_LOOSE] = 0;

		if(m_aTimes[TIME_TAR_COL_LOOSE] >= 10)
			ResetEnemy(m_pTarget);

		m_aTimes[TIME_TAR_COL_CHECK] = Server()->Tick()+Server()->TickSpeed();
	}
}

void CGuard::SetInput()
{
	if(!m_pTarget)
		NoTargetInput();
	else
		TargetInput();
}