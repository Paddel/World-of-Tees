#pragma once

#include <engine/server/maploader.h>
#include <game/server/elements/textpopup.h>
#include <game/server/weapon.h>

enum
{
	ACCOUNT_MAX_NAME_LEN=16,
	ACCOUNT_MAX_PASS_LEN=16,

	MAX_BREETH=10,
	MAX_WEAPONS=5,
};

struct CAccountInfo
{
	char m_aLoginName[32];
	char m_aLoginPassword[32];
	int m_TicketLevel;
	int m_Health;
	int m_Level;
	int m_Money;
	int m_Experience;
	int m_DeathNum;
	int m_SkillPoints;

	//filled in right before saving
	vec2 m_CurrentPos;
	char m_aCurrentMap[MAX_MAP_NAME];
	char m_aWeaponString[WEAP_MAX_LEN];
};

struct CPlayerInfo//these information will be saved although the player changed the map
{
	CStatusEffect m_aStatusEffects[NUM_STATUSEFFECT];
	CWeapon *m_pWeapon[MAX_WEAPONS];
	vec2 m_TempSpawnPos;
	int m_SpectatorID;
	int m_Mana;
	bool m_LoggedIn;
	bool m_InvTarget;

	void Reset()
	{
		for(int i = 0; i < MAX_WEAPONS; i++)
			m_pWeapon[i] = NULL;

		mem_zero(&m_aStatusEffects, sizeof(m_aStatusEffects));

		m_TempSpawnPos = vec2(0, 0);
		m_SpectatorID = -1;
		m_Mana = 0;
		m_LoggedIn = false;
		m_InvTarget = false;
	}
};