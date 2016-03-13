
#include <base/stringseperation.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>

#include "puzzletee.h"


CPuzzleTee::CPuzzleTee(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos, int PuzzleType, int ID)
	: CNpc(pGameServer, pMap, pNpcHolder, Pos, INTERFLAG_COL|INTERFLAG_HOOK, NPC_PUZZLETEE)
{
	m_ID = ID;
	m_PuzzleType = PuzzleType;
	m_SpawnPos = Pos;
}

int CPuzzleTee::SkinColorBody()
{
	if(m_PuzzleType == 0)
	{
		if(m_ID == 0)
			return 10616576;
		else if(m_ID == 1)
			return 3276544;
		else if(m_ID == 2)
			return 6422272;
		else if(m_ID == 3)
			return 65280;
	}
	return 0;
}

void CPuzzleTee::SetExtraCollision()
{
	Map()->Collision()->SetExtraCollision(TILE_NPC_COL);
}

void CPuzzleTee::Tick()
{
	SetExtraCollision();

	if(Map()->Collision()->TestBox(GetPos(), vec2(28, 28)))
		m_Core.m_Pos.y -= 4;

	UpdateCore(1.0f);
	Map()->Collision()->ResetExtraCollision();

}

void CPuzzleTee::Snap(int SnappingClient)
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

	StrToInts(&pClientInfo->m_Name0, 4, "?");
	StrToInts(&pClientInfo->m_Clan0, 3, "Npc");
	pClientInfo->m_Country = -1;
	StrToInts(&pClientInfo->m_Skin0, 6, "default");
	pClientInfo->m_UseCustomColor = 1;
	pClientInfo->m_ColorBody = SkinColorBody();
	pClientInfo->m_ColorFeet = 0;

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

	pCharacter->m_Weapon = WEAPON_HAMMER;
	pCharacter->m_AttackTick = 0;

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

void CPuzzleTee::SubSpawn()
{
	GameServer()->CreatePlayerSpawn(GetPos(), Map());
	
}

void CPuzzleTee::OnNotActive()
{
	Reset();
}

void CPuzzleTee::Reset()
{
	m_Core.m_Pos = m_SpawnPos;
}

int CPuzzleTee::IsRight()
{
	if(m_PuzzleType == 0)
	{
		int Tile = Map()->Collision()->GetExTile(GetPos().x, GetPos().y);
		char aArgs[MAX_EXTENTED_STR];
		char *pArgs = aArgs;
		str_copy(aArgs, Map()->Collision()->GetExArgs(GetPos().x, GetPos().y), sizeof(aArgs));

		if(Tile == EXTILE_PUZZLE)
		{
			if(GetSepStr(0xff, &pArgs)[0] == 's')
			{
				char *pType = GetSepStr(0xff, &pArgs);
				if(str_comp_nocase(pType, "Pyramide") == 0)
				{
					if(GetSepStr(0xff, &pArgs)[0] == 'i')
					{
						int ID = GetSepInt(0xff, &pArgs);
						if(m_ID == 0 && ID == 4)
							return 1;
						else if(m_ID == 1 && ID == 6)
							return 1;
						else if(m_ID == 2 && ID == 5)
							return 1;
						else if(m_ID == 3 && ID == 7)
							return 1;
						else if(ID > 3)
							return 0;
					}
				}
			}
		}
	}

	return -1;
}