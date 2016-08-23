/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <base/stringseperation.h>
#include <engine/shared/config.h>
#include <game/balancing.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>
#include <game/server/entities/door.h>
#include <game/server/entities/weaponpickup.h>
#include <game/server/weapons/_include.h>

#include "character.h"
#include "laser.h"
#include "projectile.h"

#define BACKPORT_TIME 10

//input count
struct CInputCount
{
	int m_Presses;
	int m_Releases;
};

CInputCount CountInput(int Prev, int Cur)
{
	CInputCount c = {0, 0};
	Prev &= INPUT_STATE_MASK;
	Cur &= INPUT_STATE_MASK;
	int i = Prev;

	while(i != Cur)
	{
		i = (i+1)&INPUT_STATE_MASK;
		if(i&1)
			c.m_Presses++;
		else
			c.m_Releases++;
	}

	return c;
}


MACRO_ALLOC_POOL_ID_IMPL(CCharacter, MAX_CLIENTS)

// Character, "physical" player's part
CCharacter::CCharacter(CGameWorld *pWorld, CMap *pMap)
: CEntity(pWorld, CGameWorld::ENTTYPE_CHARACTER, pMap, 3), CTargetAble(pWorld, CTargetAble::TYPE_CHAR, pMap)
{
	m_ProximityRadius = ms_PhysSize;
	m_pHealth = 0;
	m_ManaRatio = 0;
	m_BackportTime = 0;
}

void CCharacter::Reset()
{
	Destroy();
}

bool CCharacter::Spawn(CPlayer *pPlayer, vec2 Pos)
{
	m_EmoteStop = -1;
	m_LastAction = -1;
	m_LastNoAmmoSound = -1;
	m_ActiveWeapon = 0;
	m_LastWeapon = 0;
	m_QueuedWeapon = -1;

	m_pPlayer = pPlayer;
	m_Pos = Pos;

	m_pCore = GetPlayer()->GetCore();
	Core()->m_Pos = m_Pos;
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = Core();

	m_ReckoningTick = 0;
	mem_zero(&m_SendCore, sizeof(m_SendCore));
	mem_zero(&m_ReckoningCore, sizeof(m_ReckoningCore));

	GameServer()->m_World.InsertEntity(this);
	m_Alive = true;

	GameServer()->m_pController->OnCharacterSpawn(this);

	m_pHealth = &GetPlayer()->AccountInfo()->m_Health;

	EmptyWeaponCheck();

	if(HasHealth() == false)
		Die(m_pPlayer->GetCID(), WEAPON_WORLD);
	
	SetStatusEffect(PlayerInfo()->m_aStatusEffects);

	m_ShownTuning = m_WantedShownTuning = *GameServer()->Tuning();

	return true;
}

void CCharacter::Destroy()
{
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	m_Alive = false;
}

void CCharacter::SetWeapon(int W)
{
	if(W == m_ActiveWeapon)
		return;

	m_LastWeapon = m_ActiveWeapon;
	m_QueuedWeapon = -1;
	m_ActiveWeapon = W;
	GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH, Map());

	if(m_ActiveWeapon < 0 || m_ActiveWeapon >= NUM_WEAPS)
		m_ActiveWeapon = 0;
}

bool CCharacter::IsGrounded()
{
	if(Map()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	if(Map()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	return false;
}

bool CCharacter::InBox()
{
	if(Map()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2))
		return true;
	if(Map()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2))
		return true;
	if(Map()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y-m_ProximityRadius/2))
		return true;
	if(Map()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y-m_ProximityRadius/2))
		return true;
	return false;
}

void CCharacter::DoWeaponSwitch()
{
	// make sure we can switch
	if(m_ReloadTimer != 0 || m_QueuedWeapon == -1 || InputFrozen(INPUT_WEAPONCHANGE))
		return;

	// switch Weapon
	SetWeapon(m_QueuedWeapon);
}

void CCharacter::HandleWeaponSwitch()
{
	int WantedWeapon = m_ActiveWeapon;
	if(m_QueuedWeapon != -1)
		WantedWeapon = m_QueuedWeapon;

	// select Weapon
	int Next = CountInput(m_LatestPrevInput.m_NextWeapon, m_LatestInput.m_NextWeapon).m_Presses;
	int Prev = CountInput(m_LatestPrevInput.m_PrevWeapon, m_LatestInput.m_PrevWeapon).m_Presses;

	if(Next < 128) // make sure we only try sane stuff
	{
		while(Next) // Next Weapon selection
		{
			WantedWeapon = (WantedWeapon+1)% MAX_WEAPONS;
			if(PlayerInfo()->m_pWeapon[WantedWeapon])
				Next--;
		}
	}

	if(Prev < 128) // make sure we only try sane stuff
	{
		while(Prev) // Prev Weapon selection
		{
			WantedWeapon = (WantedWeapon-1)<0? MAX_WEAPONS -1:WantedWeapon-1;
			if(PlayerInfo()->m_pWeapon[WantedWeapon])
				Prev--;
		}
	}

	// Direct Weapon selection
	if(m_LatestInput.m_WantedWeapon)
		WantedWeapon = m_Input.m_WantedWeapon-1;

	// check for insane values
	if(WantedWeapon >= 0 && WantedWeapon < MAX_WEAPONS && WantedWeapon != m_ActiveWeapon && PlayerInfo()->m_pWeapon[WantedWeapon])
		m_QueuedWeapon = WantedWeapon;

	DoWeaponSwitch();
}

void CCharacter::FireWeapon()
{
	if(m_ReloadTimer != 0 || InputFrozen(INPUT_FIRE))
		return;

	DoWeaponSwitch();
	CWeapon *pWeapon = PlayerInfo()->m_pWeapon[m_ActiveWeapon];
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	if(pWeapon == NULL)
		return;

	bool FullAuto = pWeapon->FullAuto();

	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(FullAuto && (m_LatestInput.m_Fire&1) && pWeapon->Ammo())
		WillFire = true;

	if(!WillFire)
		return;

	if (m_BackportTime != 0)
		m_BackportTime = 0;

	// check for ammo
	if(!pWeapon->Ammo())
	{
		// 125ms is a magical limit of how fast a human can click
		m_ReloadTimer = 125 * Server()->TickSpeed() / 1000;
		if(m_LastNoAmmoSound+Server()->TickSpeed() <= Server()->Tick())
		{
			GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO, Map());
			m_LastNoAmmoSound = Server()->Tick();
		}
		return;
	}

	vec2 ProjStartPos = m_Pos+Direction*m_ProximityRadius*0.75f;

	if (pWeapon->ShootAnim())
		m_ShownAttackTick = Server()->Tick();

	int PreMana = PlayerInfo()->m_Mana;
	int Cooldown = pWeapon->Fire(m_Pos, Direction, ProjStartPos, &PlayerInfo()->m_Mana);
	m_ReloadTimer = Cooldown * Server()->TickSpeed() / 1000;

	if(PreMana > PlayerInfo()->m_Mana)
		m_ManaRatio = 0.0f;

	m_AttackTick = Server()->Tick();
}

vec2 CCharacter::MousePos()
{
	return vec2(clamp(m_Input.m_TargetX, -900, 900), clamp(m_Input.m_TargetY, -900, 900));
}

void CCharacter::HandleWeapons()
{
	// check reload timer
	if(m_ReloadTimer)
	{
		m_ReloadTimer--;
		return;
	}

	// fire Weapon, if wanted
	FireWeapon();

	return;
}

void CCharacter::OnReplaceWeapon(void *pInfo, int ClientID)
{
	CWeapon *pWeapon = (CWeapon *)pInfo;
	CCharacter *pThis = pWeapon->GameServer()->GetPlayerChar(ClientID);
	if(pThis == NULL)
		return;

	pThis->GiveWeapon(pWeapon);
}

void CCharacter::OnNotReplaceWeapon(void *pInfo, int ClientID)
{
	CWeapon *pWeapon = (CWeapon *)pInfo;
	CCharacter *pThis = pWeapon->GameServer()->GetPlayerChar(ClientID);
	if(pThis == NULL)
		return;

	new CWeaponPickup(&pThis->GameServer()->m_World, pThis->Map(), pThis->m_Pos, vec2(0, 1), -1, pWeapon);
}

void CCharacter::ReplaceWeapon(int Index, CWeapon *pNew)
{
	CWeapon *pOld = NULL;
	char aBuf[512];

	if(Index < 0 || Index >= MAX_WEAPONS)
		return;

	pOld = PlayerInfo()->m_pWeapon[Index];

	str_copy(aBuf, "Replace Weapon:\n\n", sizeof(aBuf));
	str_fcat(aBuf, sizeof(aBuf), "Replace\n");
	str_fcat(aBuf, sizeof(aBuf), "Abort\n\n");
	str_fcat(aBuf, sizeof(aBuf), "Old:\n    Name: %s\n    Damage: %i\n    Type: %s\n", s_aWeaponNames[pOld->GetType()], pOld->Damage(), pOld->Ranged() ? "Ranged" : "Meele");
	if(pOld->MaxAmmo() > -1)
		str_fcat(aBuf, sizeof(aBuf), "    Ammo: %i/%i\n", pOld->Ammo(), pOld->MaxAmmo());
	if(pOld->ManaCosts() > 0)
		str_fcat(aBuf, sizeof(aBuf), "    Maxacosts: %i\n", pOld->ManaCosts());
	str_fcat(aBuf, sizeof(aBuf), "\n");
	str_fcat(aBuf, sizeof(aBuf), "New:\n    Name: %s\n    Damage: %i\n    Type: %s\n", s_aWeaponNames[pNew->GetType()], pNew->Damage(), pNew->Ranged() ? "Ranged" : "Meele");
	if (pNew->MaxAmmo() > -1)
		str_fcat(aBuf, sizeof(aBuf), "    Ammo: %i/%i\n", pNew->Ammo(), pNew->MaxAmmo());
	if (pNew->ManaCosts() > 0)
		str_fcat(aBuf, sizeof(aBuf), "    Maxacosts: %i\n", pNew->ManaCosts());
	str_fcat(aBuf, sizeof(aBuf), "\n");

	CDecision aDecision[MAX_DECISIONS];
	aDecision[0].m_Line = 2;
	aDecision[0].m_Func = OnReplaceWeapon;
	aDecision[1].m_Line = 3;
	aDecision[1].m_Func = OnNotReplaceWeapon;
	GetPlayer()->TextPopup()->AddText(aBuf, 2, pNew, aDecision);
}

bool CCharacter::PickupWeapon(CWeaponPickup *pWeaponPickup)
{
	CWeapon *pWeapon = pWeaponPickup->GetWeapon();
	int Weapon = pWeapon->GetType();

	//Type of weapon already in inventory | no space in inventory => ask for replace
	bool HasSpace = false;
	for(int i = 0; i < MAX_WEAPONS; i++)
	{
		if (PlayerInfo()->m_pWeapon[i] == NULL)
			HasSpace = true;

		if(PlayerInfo()->m_pWeapon[i] && PlayerInfo()->m_pWeapon[i]->GetType() == Weapon)
		{
			ReplaceWeapon(i, pWeapon);
			return true;
		}
	}

	if (HasSpace == false)
	{
		if (m_ActiveWeapon == WEAP_HAMMER)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCID(), "You have no more weaponslots and you cannot drop your hammer.");
			return false;
		}
		else
		{
			ReplaceWeapon(m_ActiveWeapon, pWeapon);
			return true;
		}
	}

	//nothing to ask
	GiveWeapon(pWeapon);
	return true;
}

bool CCharacter::GiveWeapon(CWeapon *pWeapon)
{
	pWeapon->SetOwner(this);
	int Weapon = pWeapon->GetType();

	//Type of weapon already in inventory => replace
	for(int i = 0; i < MAX_WEAPONS; i++)
	{
		if(PlayerInfo()->m_pWeapon[i] && PlayerInfo()->m_pWeapon[i]->GetType() == Weapon)
		{
			DropWeapon(i);
			PlayerInfo()->m_pWeapon[i] = pWeapon;
			m_ActiveWeapon = i;
			return true;
		}
	}

	int FreeSpace = -1;
	for(int i = 0; i < MAX_WEAPONS; i++)
	{
		if(PlayerInfo()->m_pWeapon[i] == NULL)
		{
			FreeSpace = i;
			break;
		}
	}

	if(FreeSpace != -1)
	{
		PlayerInfo()->m_pWeapon[FreeSpace] = pWeapon;
		m_ActiveWeapon = FreeSpace;
	}
	else
	{
		DropWeapon(m_ActiveWeapon);
		PlayerInfo()->m_pWeapon[m_ActiveWeapon] = pWeapon;
	}

	return true;
}

bool CCharacter::DropWeapon(int Weapon)
{
	CWeapon *pWeapon = PlayerInfo()->m_pWeapon[Weapon];
	if(Weapon < 0 || Weapon >= MAX_WEAPONS || pWeapon == NULL)
		return false;

	vec2 Vel = normalize(vec2((rand()%200-100)*0.01f, -1))*16.0f;
	pWeapon->SetOwner(this);
	new CWeaponPickup(&GameServer()->m_World, Map(), m_Pos, Vel, GetPlayer()->GetCID(), pWeapon);
	PlayerInfo()->m_pWeapon[Weapon] = NULL;
	return true;
}

void CCharacter::EmptyWeaponCheck()
{
	bool MeeleFound = false;
	for(int i = 0; i < MAX_WEAPONS; i++)
	{
		if(PlayerInfo()->m_pWeapon[i] == NULL || PlayerInfo()->m_pWeapon[i]->Ranged())
			continue;

		MeeleFound = true;
		break;
	}

	if(MeeleFound == false)
	{
		GiveWeapon( new CHammer(GameServer(), this, 25) );
	}
}

void CCharacter::SetEmote(int Emote, int Tick)
{
	m_EmoteType = Emote;
	m_EmoteStop = Tick;
}

void CCharacter::OnPredictedInput(CNetObj_PlayerInput *pNewInput)
{
	// check for changes
	if(mem_comp(&m_Input, pNewInput, sizeof(CNetObj_PlayerInput)) != 0)
		m_LastAction = Server()->Tick();

	// copy new input
	mem_copy(&m_Input, pNewInput, sizeof(m_Input));
	m_NumInputs++;

	// it is not allowed to aim in the center
	if(m_Input.m_TargetX == 0 && m_Input.m_TargetY == 0)
		m_Input.m_TargetY = -1;
}

void CCharacter::OnDirectInput(CNetObj_PlayerInput *pNewInput)
{
	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
	mem_copy(&m_LatestInput, pNewInput, sizeof(m_LatestInput));

	// it is not allowed to aim in the center
	if(m_LatestInput.m_TargetX == 0 && m_LatestInput.m_TargetY == 0)
		m_LatestInput.m_TargetY = -1;

	if(m_NumInputs > 2 && m_pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		HandleWeaponSwitch();
		FireWeapon();
	}

	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
}

void CCharacter::ResetInput()
{
	m_Input.m_Direction = 0;
	m_Input.m_Hook = 0;
	// simulate releasing the fire button
	if((m_Input.m_Fire&1) != 0)
		m_Input.m_Fire++;
	m_Input.m_Fire &= INPUT_STATE_MASK;
	m_Input.m_Jump = 0;
	m_LatestPrevInput = m_LatestInput = m_Input;
}

bool CCharacter::Touching(int Tile)
{
	if(Map()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f) == Tile ||
		Map()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f) == Tile ||
		Map()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f) == Tile ||
		Map()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f) == Tile)
		return true;

	return false;
}

bool CCharacter::OnWormhole()
{
	CMapTransitionFrom *pMapTransitionFrom = NULL;
	for(int i = 0; i < Map()->WorldSection()->m_pMapTransitions.size(); i++)
	{
		CMapTransition *pMapTransition = Map()->WorldSection()->m_pMapTransitions[i];
		if(!pMapTransition->m_From)
			continue;

		if(distance(pMapTransition->m_Pos, m_Pos) < 32)
		{
			pMapTransitionFrom = (CMapTransitionFrom *)pMapTransition;
			break;
		}
	}

	if(!pMapTransitionFrom)
		return false;

	if(pMapTransitionFrom->m_HammerNeeded)
	{
		m_OnHammerTransition = true;
		//if(m_Marker.Active())
			//return true;

		if(m_ActiveWeapon != WEAP_HAMMER || (m_Input.m_Fire&1) == 0 || (m_PrevInput.m_Fire&1) == 1)
			return true;

		if(pMapTransitionFrom->m_TicketLevel > GetPlayer()->AccountInfo()->m_TicketLevel)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "You need Ticket #%i to enter this area. You have Ticket #%i", pMapTransitionFrom->m_TicketLevel, GetPlayer()->AccountInfo()->m_TicketLevel);
			GameServer()->SendChatTarget(GetPlayer()->GetCID(), aBuf);
			return true;
		}
	}

	if(pMapTransitionFrom->Map() == NULL)
		return false;

	array<CMapTransition *> pMapTransitions;
	for(int i = 0; i < pMapTransitionFrom->Map()->WorldSection()->m_pMapTransitions.size(); i++)
	{
		CMapTransition *pMapTransition = pMapTransitionFrom->Map()->WorldSection()->m_pMapTransitions[i];

		if(pMapTransitionFrom->m_ID != pMapTransition->m_ID || pMapTransition->m_From)
			continue;

		pMapTransitions.add(pMapTransition);
	}

	CMapTransition *pMapTransitionTo = NULL;

	if(pMapTransitions.size())
		pMapTransitionTo = pMapTransitions[rand()%pMapTransitions.size()];

	if(!pMapTransitionTo)
		return false;

	GetPlayer()->m_pWantedMap = pMapTransitionFrom->Map();
	PlayerInfo()->m_TempSpawnPos = pMapTransitionTo->m_Pos;

	return true;
}

bool CCharacter::OnGenerateMap(int ID, int TransitionID, int HammerNeeded)
{
	if (HammerNeeded)
	{
		m_OnHammerTransition = true;
		if (m_ActiveWeapon != WEAP_HAMMER || (m_Input.m_Fire & 1) == 0 || (m_PrevInput.m_Fire & 1) == 1)
			return true;
	}

	if (OnWormhole())
		return true;

	char aMapName[256];
	str_format(aMapName, sizeof(aMapName), "gen_%i", Server()->Tick());

	GameServer()->SendChatTarget(-1, "Generating a new map. This might take a while.");
	GameServer()->m_World.m_Paused = 1;
	Server()->DoSnapshot();

	CMap *pGeneratedMap = Server()->m_MapLoader.Generate(Map(), aMapName, TransitionID);
	GameServer()->m_World.m_Paused = 0;
	GameServer()->SendChatTarget(-1, "Generating done!");
	if (pGeneratedMap == 0x0)
		return false;

	if (Map()->Layers()->m_ExTileLayerUsed)
	{
		CMapItemLayerTilemap *pExTileMap = Map()->Layers()->ExTileLayer();
		CTile *pExTiles = (CTile *)Map()->GetEngineMap()->GetData(pExTileMap->m_Data);
		CExTile *pExArgs = (CExTile *)Map()->GetEngineMap()->GetData(pExTileMap->m_ExData);

		for (int y = 0; y < pExTileMap->m_Height; y++)
		{
			for (int x = 0; x < pExTileMap->m_Width; x++)
			{
				int Index = pExTiles[y*pExTileMap->m_Width + x].m_Index;

				if (Index != EXTILE_GENMAP)
					continue;

				vec2 Pos(x*32.0f + 16.0f, y*32.0f + 16.0f);

				char aArgs[MAX_EXTENTED_STR];
				mem_copy(aArgs, pExArgs[y*pExTileMap->m_Width + x].m_ExArgs, sizeof(aArgs));
				char *pArgs = aArgs;
				int GenID = GameServer()->GetMapInteger(&pArgs, "Mapgeneration", m_Pos, Map());

				if (GenID != ID)
					continue;

				Map()->WorldSection()->m_pMapTransitions.add(new CMapTransitionFrom(0, Pos, pGeneratedMap, false, 0));
			}
		}
	}

	return OnWormhole();
}

void CCharacter::TileCheck()
{
	int Tile = Map()->Collision()->GetCollisionAt(m_Pos);

	if(Tile == TILE_WATER)
	{
		if(m_BreethTime < Server()->Tick())
		{
			if(m_Breeth > 0)
				m_Breeth--;
			else
				TakeDamage(vec2(0, 0), Character_MaxHealth(GetPlayer()->AccountInfo()->m_Level)*0.1f, -1, WEAPON_WORLD);

			m_BreethTime = Server()->Tick()+Server()->TickSpeed()*2.5f;
		}
	}
	else
	{
		m_Breeth = MAX_BREETH;
		m_BreethTime = Server()->Tick()+Server()->TickSpeed();
	}

	if(Tile == TILE_PUSH_RIGHT && m_LastTile != TILE_PUSH_RIGHT)
		Core()->m_Vel.x = 35;
	if(Tile == TILE_PUSH_LEFT && m_LastTile != TILE_PUSH_LEFT)
		Core()->m_Vel.x = -35;
	if(Tile == TILE_PUSH_UP && m_LastTile != TILE_PUSH_UP)
		Core()->m_Vel.y = -35;
	if(Tile == TILE_PUSH_DOWN && m_LastTile != TILE_PUSH_DOWN)
		Core()->m_Vel.y = 35;
	if(Tile == TILE_PUSH_STOP && m_LastTile != TILE_PUSH_STOP)
		Core()->m_Vel = vec2(0, 0);

	if(Tile == TILE_MOVE_LEFT && Core()->m_ForceDirection == 0)
		Core()->m_ForceDirection = -1;
	if(Tile == TILE_MOVE_STOP)
		Core()->m_ForceDirection = 0;
	if(Tile == TILE_MOVE_RIGHT && Core()->m_ForceDirection == 0)
		Core()->m_ForceDirection = 1;

	if(Tile == TILE_HOME)
		GetPlayer()->WantTeleportHome();

	m_LastTile = Tile;
}

void CCharacter::ExTileCheck()
{
	int Tile = Map()->Collision()->GetExTile(m_Pos.x, m_Pos.y);
	char aArgs[MAX_EXTENTED_STR];
	char *pArgs = aArgs;
	str_copy(aArgs, Map()->Collision()->GetExArgs(m_Pos.x, m_Pos.y), sizeof(aArgs));

	if(Tile == EXTILE_MAPTRANSITION_FROM)
	{
		if(!OnWormhole())
			if(m_LastExTile != EXTILE_MAPTRANSITION_FROM)
				GameServer()->SendChatTarget(GetPlayer()->GetCID(), "Worldsection will come soon.");
	}

	if (Tile == EXTILE_GENMAP && m_LastTile != EXTILE_GENMAP)
	{
		int ID = GameServer()->GetMapInteger(&pArgs, "Mapgeneration", m_Pos, Map());
		int TransitionID = GameServer()->GetMapInteger(&pArgs, "Mapgeneration", m_Pos, Map());
		int HammerNeeded = GameServer()->GetMapInteger(&pArgs, "Mapgeneration", m_Pos, Map());
		if (OnGenerateMap(ID, TransitionID, HammerNeeded) == false)
			GameServer()->SendChatTarget(GetPlayer()->GetCID(), "Mapgeneration failed.");
	}

	if(Tile == EXTILE_CHEST)
	{
		m_OnChest = true;
		int ID = GameServer()->GetMapInteger(&pArgs, "Chest", m_Pos, Map());
		if(m_Input.m_Fire&1 && m_ActiveWeapon == WEAP_HAMMER && Server()->Tick()-m_DamageTakenTick > Server()->TickSpeed()*3.0f)
			GameServer()->WantLootChest(GetPlayer()->GetCID(), ID);
	}

	if(Tile == EXTILE_DOOR_TRIGGER)
	{
		int ID = GameServer()->GetMapInteger(&pArgs, "Door", m_Pos, Map());
		Map()->WorldSection()->OnRemote(CRemote::REMOTETYPE_DOOR, ID);

		if(m_LastExTile != EXTILE_DOOR_TRIGGER)
			GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO, Map());
	}

	if(Tile == EXTILE_BOLDER_TRIGGER && m_LastExTile != EXTILE_BOLDER_TRIGGER)
	{
		int ID = GameServer()->GetMapInteger(&pArgs, "Door-Trigger", m_Pos, Map());
		Map()->WorldSection()->OnRemote(CRemote::REMOTETYPE_BOLDER, ID);
	}

	if(Tile == EXTILE_HEAL)
		m_OnHealTile = true;
	else
		m_OnHealTile = false;
	
	m_LastExTile = Tile;
}

void CCharacter::RpgTick()
{
	m_OnHammerTransition = false;
	m_OnChest = false;

	DoRegeneration();
	HandleStatusEffects();

	if(GetPlayer()->TextPopup()->Active())
	{
		if((m_Input.m_Fire&1) && (m_PrevInput.m_Fire&1) == 0)
			GetPlayer()->TextPopup()->OnLeftClick();

		if(m_Input.m_Hook && !m_PrevInput.m_Hook)
		{
			GetPlayer()->TextPopup()->OnRightClick();
			Core()->m_HookState = HOOK_RETRACTED;
		}

		if(m_Input.m_Jump && !m_PrevInput.m_Jump)
			GetPlayer()->TextPopup()->OnJump();

		if(m_Input.m_Direction == -1 && m_PrevInput.m_Direction != -1)
			GetPlayer()->TextPopup()->OnMoveLeft();

		if(m_Input.m_Direction == 1 && m_PrevInput.m_Direction != 1)
			GetPlayer()->TextPopup()->OnMoveRight();
	}

	if(Core()->m_ForceDirection)
	{
		Core()->m_Input.m_Direction = Core()->m_ForceDirection;

		if(Core()->m_Colliding && Core()->m_Input.m_Jump == 0)
			Core()->m_Input.m_Jump = 1;
		else
			Core()->m_Input.m_Jump = 0;
	}

	if (m_BackportTime != 0)
	{
		if (m_BackportTime < Server()->Tick())
			GetPlayer()->WantTeleportHome();

		if (m_Input.m_Direction != 0 || m_Input.m_Hook || m_Input.m_Jump)
			m_BackportTime = 0;
	}


	//take characters input
	if(InputFrozen(INPUT_MOVE))
		Core()->m_Input.m_Direction = 0;
	if(InputFrozen(INPUT_JUMP))
		Core()->m_Jumped = 1;
	if(InputFrozen(INPUT_HOOK))
		Core()->m_HookState = HOOK_RETRACTED;

	if(InBox() && HasHealth())
	{
		Core()->m_Pos.y -= 4;
		Core()->m_Pos.x += m_Input.m_Direction*4;
	}
}

void CCharacter::Tick()
{
	m_DamageIndHandler.Tick(GameServer(), m_Pos, Map());

	Core()->m_Input = m_Input;
	RpgTick();
	TileCheck();
	ExTileCheck();

	SetExtraCollision();

	Core()->Tick(true);

	Map()->Collision()->ResetExtraCollision();

	// handle death-tiles and leaving gamelayer
	if(round(m_Pos.x)/32 < -200 || round(m_Pos.x)/32 > Map()->Collision()->GetWidth()+200 || round(m_Pos.y)/32 > Map()->Collision()->GetHeight()+600)
	{
		GetPlayer()->m_pWantedMap = Server()->m_MapLoader.GetDefaultMap();
	}
	else if(round(m_Pos.y)/32 < -600)
	{
		if(Core()->m_Vel.y < 0)
			Core()->m_Vel.y = 0;
	}

	// handle Weapons
	HandleWeapons();

	if(HasHealth() == false)
	{
		if(IsGrounded() && 
			Map()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y) == 0 &&
			Map()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y) == 0)
		{
			Core()->m_Pos.y++;
		}

		if(m_RemoveCorpseTime < Server()->Tick() && m_Input.m_Fire%2 == 1)
			RemoveCorpse();
	}

	if (Core()->m_OnIce)
	{
		m_WantedShownTuning.m_GroundFriction = 0.96f;
		m_WantedShownTuning.m_GroundControlAccel = 0.2f;
	}
	else
	{
		m_WantedShownTuning.m_GroundFriction = GameServer()->Tuning()->m_GroundFriction;
		m_WantedShownTuning.m_GroundControlAccel = GameServer()->Tuning()->m_GroundControlAccel;
	}


	if (mem_comp(&m_ShownTuning, &m_WantedShownTuning, sizeof(CTuningParams)) != 0)
	{
		GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_WantedShownTuning);
		m_ShownTuning = m_WantedShownTuning;
	}

	// Previnput
	m_PrevInput = m_Input;
	return;
}

void CCharacter::SetExtraCollision()
{
	//Map()->Collision()->SetExtraCollision(TILE_ICE);
}

void CCharacter::TickDefered()
{
	// advance the dummy
	{
		CSrvWorldCore TempWorld;
		m_ReckoningCore.Init(&TempWorld, Map()->Collision(), GameServer(), Map());
		m_ReckoningCore.PredictTick(false);
		m_ReckoningCore.PredictMove();
		m_ReckoningCore.Quantize();
	}

	//lastsentcore
	vec2 StartPos = Core()->m_Pos;
	vec2 StartVel = Core()->m_Vel;
	bool StuckBefore = Map()->Collision()->TestBox(Core()->m_Pos, vec2(28.0f, 28.0f));

	SetExtraCollision();

	Core()->Move(GetSlow());
	bool StuckAfterMove = Map()->Collision()->TestBox(Core()->m_Pos, vec2(28.0f, 28.0f));
	Core()->Quantize();
	bool StuckAfterQuant = Map()->Collision()->TestBox(Core()->m_Pos, vec2(28.0f, 28.0f));
	m_Pos = Core()->m_Pos;

	Map()->Collision()->ResetExtraCollision();

	if(!StuckBefore && (StuckAfterMove || StuckAfterQuant))
	{
		// Hackish solution to get rid of strict-aliasing warning
		union
		{
			float f;
			unsigned u;
		}StartPosX, StartPosY, StartVelX, StartVelY;

		StartPosX.f = StartPos.x;
		StartPosY.f = StartPos.y;
		StartVelX.f = StartVel.x;
		StartVelY.f = StartVel.y;

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "STUCK!!! %d %d %d %f %f %f %f %x %x %x %x",
			StuckBefore,
			StuckAfterMove,
			StuckAfterQuant,
			StartPos.x, StartPos.y,
			StartVel.x, StartVel.y,
			StartPosX.u, StartPosY.u,
			StartVelX.u, StartVelY.u);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	int Events = Core()->m_TriggeredEvents;
	int Mask = CmaskAllExceptOne(m_pPlayer->GetCID());

	if(Events&COREEVENT_GROUND_JUMP) GameServer()->CreateSound(m_Pos, SOUND_PLAYER_JUMP, Map(), Mask);

	if(Events&COREEVENT_HOOK_ATTACH_PLAYER) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_PLAYER, Map(), CmaskAll());
	if(Events&COREEVENT_HOOK_ATTACH_GROUND) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_GROUND, Map(), Mask);
	if(Events&COREEVENT_HOOK_HIT_NOHOOK) GameServer()->CreateSound(m_Pos, SOUND_HOOK_NOATTACH, Map(), Mask);


	if(m_pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		m_Pos.x = m_Input.m_TargetX;
		m_Pos.y = m_Input.m_TargetY;
	}

	// update the m_SendCore if needed
	{
		CNetObj_Character Predicted;
		CNetObj_Character Current;
		mem_zero(&Predicted, sizeof(Predicted));
		mem_zero(&Current, sizeof(Current));
		m_ReckoningCore.Write(&Predicted);
		Core()->Write(&Current);

		// only allow dead reackoning for a top of 3 seconds
		if(m_ReckoningTick+Server()->TickSpeed()*3 < Server()->Tick() || mem_comp(&Predicted, &Current, sizeof(CNetObj_Character)) != 0)
		{
			m_ReckoningTick = Server()->Tick();
			m_SendCore = *Core();
			m_ReckoningCore = *Core();
		}
	}
}

void CCharacter::TickPaused()
{
	++m_ShownAttackTick;
	++m_AttackTick;
	++m_DamageTakenTick;
	++m_Ninja.m_ActivationTick;
	++m_ReckoningTick;
	if(m_LastAction != -1)
		++m_LastAction;
	if(m_EmoteStop > -1)
		++m_EmoteStop;
}

bool CCharacter::IncreaseHealth(int Amount)
{
	if(*m_pHealth >= Character_MaxHealth(GetPlayer()->AccountInfo()->m_Level))
		return false;
	*m_pHealth = clamp(*m_pHealth+Amount, 0, Character_MaxHealth(GetPlayer()->AccountInfo()->m_Level));
	return true;
}

bool CCharacter::IncreaseMana(int Amount)
{
	if(PlayerInfo()->m_Mana >= Character_MaxMana(GetPlayer()->AccountInfo()->m_Level))
		return false;

	PlayerInfo()->m_Mana = clamp(PlayerInfo()->m_Mana+Amount, 0, Character_MaxMana(GetPlayer()->AccountInfo()->m_Level));
	return true;
}

void CCharacter::RemoveCorpse()
{
	// this is for auto respawn after 3 secs
	GetPlayer()->m_DieTick = Server()->Tick();

	//GetPlayer()->ResetIDMap();
	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID(), Map());

	m_Alive = false;
	GameServer()->m_World.RemoveEntity(this);
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID(), Map());

	GetPlayer()->OnCharacterDead();
}

void CCharacter::Die(int Killer, int Weapon)
{
	GetPlayer()->Save();

	// we got to wait 0.5 secs before respawning
	//m_pPlayer->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
	int ModeSpecial = 1;//GameServer()->m_pController->OnCharacterDeath(this, GameServer()->m_apPlayers[Killer], Weapon);

	if(Killer != -1)
	{
		int PlayerItemID = GameServer()->m_World.GetPlayerItemId(GameServer()->m_apPlayers[Killer], Map());
		// send the kill message
		CNetMsg_Sv_KillMsg Msg;
		Msg.m_Killer = PlayerItemID;
		Msg.m_Victim = GameServer()->m_World.GetPlayerItemId(m_pPlayer, Map());
		Msg.m_Weapon = Weapon;
		Msg.m_ModeSpecial = ModeSpecial;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
	}
	else
	{
		int PlayerItemID = GameServer()->m_World.GetPlayerItemId(m_pPlayer, Map());
		CNetMsg_Sv_KillMsg Msg;
		Msg.m_Killer = PlayerItemID;
		Msg.m_Victim = PlayerItemID;
		Msg.m_Weapon = Weapon;
		Msg.m_ModeSpecial = ModeSpecial;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
	}

	// a nice sound
	GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE, Map());
	OnTargetDeath();

	m_DamageIndHandler.OnDeath(GameServer(), m_Pos, Map());
	m_RemoveCorpseTime = Server()->Tick()+Server()->TickSpeed()*15;
}

bool CCharacter::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	if(HasHealth() == false)
		return false;

	Core()->m_Vel += Force;
	m_RegenHealthTime = Server()->Tick()+Server()->TickSpeed()*15;

	m_BackportTime = 0;

	// m_pPlayer only inflicts half damage on self
	if(From == m_pPlayer->GetCID())
		Dmg = max(1, Dmg/2);

	m_DamageIndHandler.TookDamage(Dmg);

	if(Dmg)
		*m_pHealth -= Dmg;

	m_DamageTakenTick = Server()->Tick();

	// do damage Hit sound
	if(From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
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
	if(*m_pHealth <= 0)
	{
		Die(From, Weapon);

		// set attacker's face to happy (taunt!)
		if (From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
		{
			CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
			if (pChr)
			{
				pChr->m_EmoteType = EMOTE_HAPPY;
				pChr->m_EmoteStop = Server()->Tick() + Server()->TickSpeed();
			}
		}

		return false;
	}

	if (Dmg > 2)
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG, Map());
	else
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT, Map());

	m_EmoteType = EMOTE_PAIN;
	m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;

	return true;
}

void CCharacter::Snap(int SnappingClient)
{
	CWeapon *pCurWeapon = PlayerInfo()->m_pWeapon[m_ActiveWeapon];
	if(NetworkClipped(SnappingClient))
		return;

	int id = GetPlayer()->GetCID();
	if(!GameServer()->GameTranslate(id, SnappingClient)) return;

	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, id, sizeof(CNetObj_Character)));
	if(!pCharacter)
		return;

	// write down the Core()
	if(!m_ReckoningTick || GameServer()->m_World.m_Paused)
	{
		// no dead reckoning when paused because the client doesn't know
		// how far to perform the reckoning
		pCharacter->m_Tick = 0;
		Core()->Write(pCharacter);
	}
	else
	{
		pCharacter->m_Tick = m_ReckoningTick;
		m_SendCore.Write(pCharacter);
	}

	if (Core()->m_OnIce && Core()->m_Input.m_Direction == 0 && GetPlayer()->GetCID() != SnappingClient)
		pCharacter->m_VelX = 0;

	// set emote
	if (m_EmoteStop < Server()->Tick())
	{
		m_EmoteType = EMOTE_NORMAL;
		m_EmoteStop = -1;
	}

	if(pCharacter->m_HookedPlayer != -1)
	{
		if(Core()->m_BotHooked)
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

	pCharacter->m_Emote = m_EmoteType;

	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;

	pCharacter->m_Weapon = WEAP_HAMMER;
	if(pCurWeapon)
		pCharacter->m_Weapon = pCurWeapon->SnapWeapon();

	pCharacter->m_AttackTick = m_ShownAttackTick;

	pCharacter->m_Direction = Core()->m_Input.m_Direction;

	if(m_pPlayer->GetCID() == SnappingClient || SnappingClient == -1 ||
		(!g_Config.m_SvStrictSpectateMode && m_pPlayer->GetCID() == GameServer()->m_apPlayers[SnappingClient]->PlayerInfo()->m_SpectatorID))
	{
		pCharacter->m_Health = ((float)*m_pHealth/Character_MaxHealth(GetPlayer()->AccountInfo()->m_Level))*10;
		pCharacter->m_Armor = ((float)PlayerInfo()->m_Mana/Character_MaxMana(GetPlayer()->AccountInfo()->m_Level))*10;
		if(pCurWeapon &&  pCurWeapon->Ammo() > 0)
			pCharacter->m_AmmoCount = ((float)pCurWeapon->Ammo()/pCurWeapon->MaxAmmo())*10;
	}

	if(pCharacter->m_Emote == EMOTE_NORMAL)
	{
		if(250 - ((Server()->Tick() - m_LastAction)%(250)) < 5)
			pCharacter->m_Emote = EMOTE_BLINK;
	}

	pCharacter->m_PlayerFlags = GetPlayer()->m_PlayerFlags;

	if(pCurWeapon)
		pCurWeapon->SpecialWorldSnap(SnappingClient, m_Pos, normalize(MousePos()), m_AttackTick);

	if (m_BackportTime != 0)
	{
		float RestTime = m_BackportTime - Server()->Tick();
		float DistFac = RestTime / (Server()->TickSpeed() * BACKPORT_TIME);
		float Dist = 40.0f * DistFac + 23.0f;
		float t = Server()->Tick() / 20.0f;

		for (int i = 0; i < 3; i++)
		{
			float PosFac = i / 3.0f;
			vec2 Pos = m_Pos + vec2(cosf(2 * pi * PosFac + t), sinf(2 * pi * PosFac + t)) * Dist;
			RenderProjectile(m_aSnapIDs[i], Server()->Tick() - 10, Pos, vec2(0, 0), WEAP_HAMMER);
		}
	}
}

bool CCharacter::InputFrozen(int Input)
{
	if(GetPlayer()->TextPopup()->Active())
		return true;

	if(HasHealth() == false)
		return true;

	if(Core()->m_ForceDirection && Input != INPUT_MOVE && Input != INPUT_JUMP)
		return true;

	if(GetStun())
		return true;

	return false;
}

bool CCharacter::IsEnemy(CTargetAble *pTarget)
{
	if(!pTarget)
		return false;

	if(PlayerInfo()->m_InvTarget)
		return false;

	if (Core()->m_ForceDirection != 0)
		return false;

	int Type = pTarget->GetType();
	if(Type == CTargetAble::TYPE_NPC_ENEMY)
		return true;
	return false;
}

void CCharacter::SetHealth(int Health)
{
	*m_pHealth = clamp(Health, 0, Character_MaxHealth(GetPlayer()->AccountInfo()->m_Level));
}

bool CCharacter::HasHealth()
{
	return *m_pHealth > 0;
}

bool CCharacter::Revive()
{
	if(HasHealth())
		return false;

	AddTargetAble();

	*m_pHealth = Character_MaxHealth(GetPlayer()->AccountInfo()->m_Level)*0.5f;
	return true;
}

void CCharacter::DoRegeneration()
{
	if(HasHealth() && m_RegenHealthTime < Server()->Tick())
	{
		int MaxHealth = Character_MaxHealth(GetPlayer()->AccountInfo()->m_Level);
		if(*m_pHealth < MaxHealth)
		{
			if(m_OnHealTile)
				IncreaseHealth(MaxHealth / 15);
			else
				IncreaseHealth(1);
		}

		m_RegenHealthTime = Server()->Tick()+Server()->TickSpeed()*2.75f;
	}

	if(Character_MaxMana(GetPlayer()->AccountInfo()->m_Level) > 0 && m_RegenManaTime < Server()->Tick())
	{
		if(PlayerInfo()->m_Mana < Character_MaxMana(GetPlayer()->AccountInfo()->m_Level))
		{
			m_ManaRatio += 0.2f;
			IncreaseMana((int)m_ManaRatio);
		}

		m_RegenManaTime = Server()->Tick()+Server()->TickSpeed()*0.5f;
	}
}

CPlayerInfo *CCharacter::PlayerInfo()
{
	return GetPlayer()->PlayerInfo();
}

int CCharacter::MaxHealth()
{
	return Character_MaxHealth(GetPlayer()->AccountInfo()->m_Level);
}

void CCharacter::WantBackport()
{
	if (m_BackportTime != 0)
		return;

	m_BackportTime = Server()->Tick() + Server()->TickSpeed() * BACKPORT_TIME;
}

CWeapon *CCharacter::HasWeapon(int Type)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (PlayerInfo()->m_pWeapon[i] == NULL)
			continue;

		if (PlayerInfo()->m_pWeapon[i]->GetType() == Type)
			return PlayerInfo()->m_pWeapon[i];
	}
	return NULL;
}

CWeapon *CCharacter::CurrentWeapon()
{
	return PlayerInfo()->m_pWeapon[m_ActiveWeapon];
}