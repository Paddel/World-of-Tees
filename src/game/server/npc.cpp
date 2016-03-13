
#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/money.h>
#include <game/server/entities/experience.h>
#include <game/server/entities/weaponpickup.h>
#include <game/server/weapons/_include.h>
#include <game/server/npc/_include.h>

#include "npc.h"

#define NPC_MOVE_LOOK_UP_BLOCKS 12
#define NPC_MOVE_FARWORD_PREDICT 5

CNpc *CreateNpc(int Type, CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHoler, vec2 Pos)
{
	CNpc *pNpc = NULL;
	switch(Type)
	{
	case NPC_SKELETON: pNpc = new CSkeleton(pGameServer, pMap, pNpcHoler, Pos); break;
	case NPC_WILDTEE: pNpc = new CWildtee(pGameServer, pMap, pNpcHoler, Pos); break;
	case NPC_BANDIT: pNpc = new CBandit(pGameServer, pMap, pNpcHoler, Pos); break;
	case NPC_SHADOW: pNpc = new CShadow(pGameServer, pMap, pNpcHoler, Pos); break;
	case NPC_MUMMY: pNpc = new CMummy(pGameServer, pMap, pNpcHoler, Pos); break;
	case NPC_ZOMBIE: pNpc = new CZombie(pGameServer, pMap, pNpcHoler, Pos); break;
	case NPC_PHARAO: pNpc = new CPharao(pGameServer, pMap, pNpcHoler, Pos); break;

	}

	return pNpc;
}

CNpc::CNpc(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos, int InterFlags, int Type)
{
	m_pServer = pGameServer->Server();
	m_pMap = pMap;
	m_pNpcHolder = pNpcHolder;
	m_Type = Type;
	CPlayerItem::Init(pGameServer, Map(), Pos, CPlayerItem::TYPE_NPC, InterFlags);

	m_Owner = -1;
	m_Temporary = false;
}

void CNpc::Spawn()
{
	SubSpawn();

	if(m_pNpcHolder)
		m_pNpcHolder->OnNewNpc(this);
}

void CNpc::OnNpcDeath()
{
	int PlayerItemID = GameServer()->m_World.GetPlayerItemId(this, Map());

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CCharacter *pChr = GameServer()->GetPlayerChar(i);
		if(!pChr)
			continue;
		CSrvCharacterCore *pCore = pChr->Core();
		if(!pCore->m_BotHooked || pCore->m_HookedPlayer == -1)
			continue;

		if(pCore->m_HookedPlayer == PlayerItemID)
		{
			pCore->m_HookState = HOOK_RETRACTED;
			pCore->m_HookedPlayer = -1;
		}
	}

	if(m_pNpcHolder)
		m_pNpcHolder->OnRemoveNpc(this);
}

void CNpc::SetEmote(int Emote, int Tick)
{
	m_EmoteType = Emote;
	m_EmoteStop = Tick;
}


void CNpc::DoEmoticon(int Emoticon)
{
	int PlayerItemID = GameServer()->m_World.GetPlayerItemId(this, Map());
	CNetMsg_Sv_Emoticon Msg;
	Msg.m_ClientID = PlayerItemID;
	Msg.m_Emoticon = Emoticon;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!Server()->ClientIngame(i) || Map() != Server()->CurrentMap(i))
			continue;

		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
	}
}

bool CNpc::IsGrounded()
{
	if(Map()->Collision()->CheckPoint(GetPos().x+28/2, GetPos().y+28/2+5))
		return true;
	if(Map()->Collision()->CheckPoint(GetPos().x-28/2, GetPos().y+28/2+5))
		return true;
	return false;
}

CTargetAble *CNpc::FindClosestTarget(float Radius, CTargetAble *pAskinTarget)
{
	CTargetAble * pClosestTarget = NULL;
	float ClosestRange = Radius*Radius;

	for(int i = 0; i < Map()->WorldSection()->m_lpTargetAbles.size(); i++)
	{
		CTargetAble *pTarget = Map()->WorldSection()->m_lpTargetAbles[i];
		if (pTarget == pAskinTarget)
			continue;

		//if(FriendlyTarget && (pMap->m_GameMapFlags&MAPGAMEFLAG_CHARDMG) == 0 && pTarget->GetType() == CTargetAble::TYPE_CHAR)
			//continue;

		if(pAskinTarget && pTarget->IsEnemy(pAskinTarget) == false)
			continue;

		vec2 TargetPos = pTarget->GetClosestPos(GetPos());
		vec2 CheckPos = TargetPos-GetPos();

		if(abs(CheckPos.x) > Radius || abs(CheckPos.y) > Radius)//if one coordinate is over range
			continue;
		
		float len = CheckPos.x*CheckPos.x+CheckPos.y*CheckPos.y;
		if(len > ClosestRange)
			continue;

		//if(pTarget->GetType() == CTargetAble::TYPE_CHAR && ((CCharacter*)pTarget)->GetInvisible())
			//continue;

			if(Map()->Collision()->IntersectLine(GetPos(), TargetPos, NULL, NULL))
				continue;

		pClosestTarget = pTarget;
		ClosestRange = len;
	}

	//to get the length do sqrt(ClosestRange); :))
	
	return pClosestTarget;
}

CEnemyNpc::CEnemyNpc(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos, int Type)
	: CNpc(pGameServer, pMap, pNpcHolder, Pos, INTERFLAG_COL|INTERFLAG_HOOK, Type), CTargetAble(&pGameServer->m_World, CTargetAble::TYPE_NPC_ENEMY, pMap)
{
	m_pTarget = 0;
	m_AttackTick = 0;

	m_RandJump = false;

	for(int i = 0; i < NUM_TIMES; i++)
		m_aTimes[i] = 0;

	m_pWeapon = new CHammer(GameServer(), this, 0);
}

void CEnemyNpc::SetMoveDirection(int Direction, float Time)
{
	m_Input.m_Direction = Direction;
	m_aTimes[TIME_NOTAR_CHANGE_MOVEDIR] = Server()->Tick()+(Server()->TickSpeed()*Time);
}

bool CEnemyNpc::RandomDrop(float Chance)
{
	if(Chance >= 1.0f)
		return true;

	float Res = (rand()%1000)/1000.0f;
	if(Chance > Res)
		return true;

	return false;
}

int CEnemyNpc::AiFireDistance()
{
	int Weapon = m_pWeapon->GetType();

	switch(Weapon)
	{
	case WEAP_HAMMER: return 92;
	case WEAP_GUN: return 900;
	case WEAP_SHOTGUN: return 500;
	case WEAP_GRENADE: return 550;
	case WEAP_RIFLE: return 800;
	case WEAP_DAGGER: return 64;
	case WEAP_FIREROD: return 450;
	}

	return 48;
}

int CEnemyNpc::AiMoveDistance()
{
	int Weapon = m_pWeapon->GetType();

	switch(Weapon)
	{
	case WEAP_HAMMER: return 48;
	case WEAP_GUN: return 650;
	case WEAP_SHOTGUN: return 400;
	case WEAP_GRENADE: return 450;
	case WEAP_RIFLE: return 750;
	case WEAP_DAGGER: return 48;
	case WEAP_FIREROD: return 300;
	}

	return 48;
}

int CEnemyNpc::AiMoveTolerance()
{
	int Weapon = m_pWeapon->GetType();

	switch(Weapon)
	{
	case WEAP_HAMMER: return 8;
	case WEAP_GUN: return 50;
	case WEAP_SHOTGUN: return 40;
	case WEAP_GRENADE: return 80;
	case WEAP_RIFLE: return 30;
	case WEAP_DAGGER: return 4;
	case WEAP_FIREROD: return 32;
	}

	return 0;
}

vec2 CEnemyNpc::GetFireDirection(vec2 OwnPos, vec2 EnemyPos)
{
	int Weapon = m_pWeapon->GetType();
	
	if(Weapon == WEAP_HAMMER)
		return EnemyPos-OwnPos;
	else if(Weapon == WEAP_GUN)
	{
		float dist = distance(OwnPos, EnemyPos);
		if(dist > 500)
		{
			float a = GetAngle(normalize(EnemyPos-OwnPos));
			a += 0.085f*(EnemyPos.x>OwnPos.x?-1:1);
			return vec2(cosf(a), sinf(a));
		}
		else
			return normalize(EnemyPos-OwnPos);
	}
	else if(Weapon == WEAP_SHOTGUN)
		return EnemyPos-OwnPos;
	else if(Weapon == WEAP_GRENADE)
	{
		float dist = distance(OwnPos, EnemyPos);
		if(dist > 200)
		{
			float a = GetAngle(normalize(EnemyPos-OwnPos));
			a += 0.385f*(EnemyPos.x>OwnPos.x?-1:1);
			return vec2(cosf(a), sinf(a));
		}
		else
			return normalize(EnemyPos-OwnPos);
	}
	else if(Weapon == WEAP_RIFLE)
	{
		if(rand()%3 == 0)
		{//miss sometimes
			float a = GetAngle(normalize(EnemyPos-OwnPos));
			a += 0.185f*(rand()%2?-1:1);
			return vec2(cosf(a), sinf(a));
		}
		else
			return EnemyPos-OwnPos;
	}

	return EnemyPos-OwnPos;
}

bool CEnemyNpc::CheckImpasse(vec2 CurPos, vec2 ColPos)
{
	if(!Map()->Collision()->GetCollisionAt(CurPos.x, CurPos.y+32))
		return false;

	for(int i = 0; i < NPC_MOVE_LOOK_UP_BLOCKS; i++)
	{
		if(Map()->Collision()->GetCollisionAt(CurPos.x, CurPos.y-i*32))
			break;

		if(!Map()->Collision()->GetCollisionAt(ColPos.x, ColPos.y-i*32))
			return false;
	}

	return true;
}


void CEnemyNpc::SetExtraCollision()
{
	Map()->Collision()->SetExtraCollision(TILE_NPC_COL);
	Map()->Collision()->SetExtraCollision(TILE_NPC_ENEMY_COL);
}

void CEnemyNpc::CheckHealthRegeneration()
{
	if(m_pTarget || m_Health >= MaxHealth() || m_HealthRegTime >= Server()->Tick())
		return;

	m_Health += clamp(MaxHealth()/20, 1, MaxHealth()-m_Health);
	m_HealthRegTime = Server()->Tick()+Server()->TickSpeed()*2.5f;
}

void CEnemyNpc::Tick()
{
	SetExtraCollision();
	//if(Map()->Collision()->TestBox(GetPos(), vec2(GetProximityRadius(), GetProximityRadius())))
		//m_Core.m_Pos.y -= 4;

	SetInput();
	SubTick();
	UpdateCore(GetSlow());
	Map()->Collision()->ResetExtraCollision();

	CheckHealthRegeneration();
	HandleStatusEffects();

	m_DamageIndHandler.Tick(GameServer(), GetPos(), Map());
}

bool CEnemyNpc::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	m_Core.m_Vel += Force;

	if(Dmg)
		m_Health -= Dmg;

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

void CEnemyNpc::Snap(int SnappingClient)
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

	if(StatusEffectSkin(&pClientInfo->m_ColorBody, &pClientInfo->m_ColorFeet))
		pClientInfo->m_UseCustomColor = 1;

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

	pCharacter->m_Weapon = m_pWeapon->SnapWeapon();
	if(m_pWeapon && m_pWeapon->ShootAnim())
		pCharacter->m_AttackTick = m_AttackTick;

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

	pCharacter->m_PlayerFlags = 0;

	vec2 Direction = normalize(vec2(m_Input.m_TargetX, m_Input.m_TargetY));
	m_pWeapon->SpecialWorldSnap(SnappingClient, GetPos(), Direction, m_AttackTick);
}

void CEnemyNpc::SubSpawn()
{
	GameServer()->CreatePlayerSpawn(GetPos(), Map());
	m_Health = MaxHealth();
	m_pWeapon = CreateWeapon();
	m_Core.m_Speed = Speed();
	mem_zero(&m_aStatusEffects, sizeof(m_aStatusEffects));
	SetStatusEffect(m_aStatusEffects);
	SubOnSpawn();
}

void CEnemyNpc::OnDeath(int From, int Weapon, vec2 DeathVel)
{
	SubOnDeath(From, Weapon, DeathVel);

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

		m_pWeapon->SetOwner(NULL);
		if(WeaponDropChance() > 0.0f && RandomDrop(WeaponDropChance()))
			new CWeaponPickup(&GameServer()->m_World, Map(), GetPos(), DeathVel*0.5f, From, m_pWeapon);
	}

	OnNpcDeath();
	GameServer()->m_World.RemovePlayerItem(this, Map());
	OnTargetDeath();
	GameServer()->CreateDeath(GetPos(), -1, Map());

	m_DamageIndHandler.OnDeath(GameServer(), GetPos(), Map());
}

void CEnemyNpc::OnNotActive()
{
	m_Health = MaxHealth();
}

void CEnemyNpc::ResetEnemy(CTargetAble *pTarget)
{
	if(m_pTarget == pTarget)
	{
		m_pTarget = NULL;
		m_aTimes[TIME_TAR_COL_LOOSE] = 0;
		SubResetTarget();
	}
}

bool CEnemyNpc::IsEnemy(CTargetAble *pTarget)
{
	if(!pTarget)
		return false;

	int Type = pTarget->GetType();
	if(Type == CTargetAble::TYPE_CHAR ||
		Type == CTargetAble::TYPE_NPC_VILLIGER)
		return true;
	return false;
}

void CEnemyNpc::NoTargetInput()
{
	m_pTarget = FindClosestTarget(1024, this);

	vec2 CampPos = m_pNpcHolder?m_pNpcHolder->CampPos():vec2(0, 0);
	if(m_pNpcHolder && m_pNpcHolder->Camp() && abs(CampPos.x -  GetPos().x) > 320)
	{
		m_Input.m_Direction = clamp((int)(CampPos.x-GetPos().x), -1, 1);
		m_aTimes[TIME_NOTAR_CHANGE_MOVEDIR] = Server()->Tick()+(Server()->TickSpeed()/100.0f*(float)(rand()%100+50));
	}
	else
	{
		if(m_aTimes[TIME_NOTAR_CHANGE_MOVEDIR] < Server()->Tick())
		{
			m_Input.m_Direction = rand()%3-1;
			m_aTimes[TIME_NOTAR_CHANGE_MOVEDIR] = Server()->Tick()+(Server()->TickSpeed()/100.0f*(float)(rand()%400+200));

			m_Input.m_TargetX = m_Input.m_Direction;
			m_Input.m_TargetY = 0;
		}
	}

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
		if(Hit)
		{
			bool Stuck = CheckImpasse(GetPos(), OutPos);

			if(Stuck)
			{
				m_Input.m_Direction = -m_Input.m_Direction;
			}
			
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

	if(UseHook())
	{
		m_Input.m_Hook = 0;
	}
}

void CEnemyNpc::TargetInput()
{
	vec2 TargetPos = m_pTarget->GetClosestPos(GetPos());
	float dist = distance(GetPos(), TargetPos);

	//target
	vec2 FireDirection = GetFireDirection(GetPos(), TargetPos);
	m_Input.m_TargetX = FireDirection.x*128;
	m_Input.m_TargetY = FireDirection.y*128;

	//direction
	if(dist > AiMoveDistance()+AiMoveTolerance())
	{
		m_Input.m_Direction = clamp((int)(TargetPos.x-GetPos().x), -1, 1);
	}
	else if(dist < AiMoveDistance()-AiMoveTolerance())
	{
		m_Input.m_Direction = clamp((int)(-(TargetPos.x-GetPos().x)), -1, 1);
	}
	else
	{
		m_Input.m_Direction = 0;
	}

	//jump
	if(m_pWeapon->Ranged())
	{
		if(m_Input.m_Direction != 0)
		{
			vec2 LookPos = GetPos()+vec2(m_Input.m_Direction*NPC_MOVE_FARWORD_PREDICT*32, 0);
			vec2 OutPos = vec2(0, 0);
			int Hit = Map()->Collision()->IntersectLine(GetPos(), LookPos, &OutPos, NULL);
			if(Hit)
			{
				bool Stuck = CheckImpasse(GetPos(), OutPos);

				if(Stuck == false)
				{
					if(!m_Input.m_Jump)
					{
						if(IsGrounded())//always jump when grounded
							m_Input.m_Jump = 1;
						else
						{
							if(m_Core.m_Vel.y >= 0)
							{
								m_Input.m_Jump = 1;
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
					m_Input.m_Jump = true;
				else
					m_Input.m_Jump = false;
			}
		}
	}
	else if(dist > AiMoveDistance()+AiMoveTolerance())
	{
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

	//fire
	if(dist <= AiFireDistance() && GetStun() == false)
	{
		if(!Map()->Collision()->IntersectLine(GetPos(), TargetPos, NULL, NULL))
		{
			if(m_aTimes[TIME_TAR_ATTACK] < Server()->Tick())
			{
				vec2 Direction = normalize(vec2(m_Input.m_TargetX, m_Input.m_TargetY));
				vec2 ProjStartPos = GetPos()+Direction*GetProximityRadius()*0.75f;
				int Cooldown = m_pWeapon->Fire(GetPos(), Direction, ProjStartPos, NULL);
				m_aTimes[TIME_TAR_ATTACK] = Server()->Tick()+Server()->TickSpeed()/1000.0f*AttackSpeed();
				m_AttackTick = Server()->Tick();
			}
		}
	}
	else
		m_aTimes[TIME_TAR_ATTACK] = Server()->Tick()+Server()->TickSpeed()/1000.0f*AttackSpeed()*0.5f;

	//hook
	if(UseHook() && dist < 360)
	{
		m_Input.m_TargetX = TargetPos.x-GetPos().x;
		m_Input.m_TargetY = TargetPos.y-GetPos().y;

		if((m_Core.m_HookState != HOOK_FLYING && m_Core.m_HookState != HOOK_GRABBED) || (m_Core.m_HookState == HOOK_GRABBED && m_Core.m_HookedPlayer == -1))
		{
			if(m_Input.m_Hook == 1)
				m_Input.m_Hook = 0;
			else
				m_Input.m_Hook = 1;
		}
	}

	//loose target
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

	if(m_pTarget->IsEnemy(this) == false)
		ResetEnemy(m_pTarget);
}

void CEnemyNpc::SetInput()
{
	if(!m_pTarget)
		NoTargetInput();
	else
		TargetInput();

	if(GetStun())
	{
		m_Input.m_Direction = 0;
		m_Input.m_Jump = 0;
		m_Core.m_HookState = HOOK_RETRACTED;
	}
}

void CNpcHolder::KillTemporary()
{
	for(int i = 0; i < m_pNpcs.size(); i++)
	{
		CNpc *pNpc = m_pNpcs[i];
		if(pNpc->GetTemporary() == false)
			continue;

		pNpc->OnDeath(-1, WEAPON_WORLD, vec2(0, 0));
		i--;
	}
}

int CNpcHolder::GetTemporaryNum()
{
	int Num = 0;
	for(int i = 0; i < m_pNpcs.size(); i++)
	{
		CNpc *pNpc = m_pNpcs[i];
		if(pNpc->GetTemporary() == false)
			continue;
		Num++;
	}
	return Num;
}

int CNpcHolder::GetTempPlayerSpawnNum(int ClientID)
{
	int Num = 0;
	for(int i = 0; i < m_pNpcs.size(); i++)
	{
		CNpc *pNpc = m_pNpcs[i];
		if(pNpc->GetTemporary() == false || pNpc->GetOwner() != ClientID)
			continue;
		Num++;
	}
	return Num;
}

int CNpcHolder::GetNum()
{
	return m_pNpcs.size();
}

void CNpcHolder::OnNewNpc(CNpc *pNpc)
{
	for(int i = 0; i < m_pNpcs.size(); i++)
	{
		if(m_pNpcs[i] == pNpc)
			return;
	}

	m_pNpcs.add(pNpc);
}

void CNpcHolder::OnRemoveNpc(CNpc *pNpc)
{
	for(int i = 0; i < m_pNpcs.size(); i++)
	{
		if(m_pNpcs[i] == pNpc)
		{
			m_pNpcs.remove_index(i);
			return;
		}
	}
}