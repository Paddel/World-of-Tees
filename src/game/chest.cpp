
#include <base/math.h>
#include <base/system.h>
#include <game/generated/protocol.h>

#include "chest.h."

CChest::CChest()
{
	m_ContentID = -1;
	m_CooldownTime = 0;
	m_Size = 0;
	m_Cooldown = 0;
	m_DisplaySet = false;
	m_pDisplayMap = NULL;
	m_DisplayPosition = vec2(0, 0);
	m_SnapID = -1;

	m_LootID = -1;
	m_LootRatio = 0.0f;
	m_LootRatioDecreaseTime = 0;
	m_RandNum = rand();
}

CChest::CChest(int ContentID)
{
	m_ContentID = ContentID;
	m_CooldownTime = 0;
	m_Size = 0;
	m_Cooldown = 0;
	m_DisplaySet = false;
	m_pDisplayMap = NULL;
	m_DisplayPosition = vec2(0, 0);
	m_SnapID = -1;

	m_LootID = -1;
	m_LootRatio = 0.0f;
	m_LootRatioDecreaseTime = 0;
	m_RandNum = rand();

	Drop(GetChestInfo, NULL, this);
}

void CChest::Drop(ChestFunc BaseFunc, DropFunc Func, void *pInfo, int ClientID)
{
	if(BaseFunc == NULL)
		BaseFunc = ChestInfoTemp;
	if(Func == NULL)
		Func = ChestDropTemp;

	switch(m_ContentID)
	{
	case 0:
		{
			BaseFunc(pInfo, CHESTSIZE_SMALL, 600);
			Func(pInfo, "Money", 200, 0x0, ClientID);

		} break;
	case 1:
		{
			BaseFunc(pInfo, CHESTSIZE_SMALL, 1);
			Func(pInfo, "Shuriken", 1, "dmg=4;ammo=30", ClientID);
			Func(pInfo, "Money", 42, 0x0, ClientID);
		} break;
	case 2:
		{
			BaseFunc(pInfo, CHESTSIZE_SMALL, 120);
			Func(pInfo, "Money", 30, 0x0, ClientID);
		} break;
	default :
		{
			dbg_msg("Chests", "Content-ID %i not found", m_ContentID);
		};
	}
}

void CChest::DropRand(ChestFunc BaseFunc, DropFunc Func, void *pInfo, int ClientID)
{
	if (BaseFunc == NULL)
		BaseFunc = ChestInfoTemp;
	if (Func == NULL)
		Func = ChestDropTemp;

	switch (m_ContentID)
	{
	case RANDCONTENT_SMALL:
	{
		int Money = m_RandNum % 50 + 170;
		int NumShuriken = m_RandNum % 20 + 20;
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "dmg=14;ammo=%i", NumShuriken);
		BaseFunc(pInfo, CHESTSIZE_SMALL, 600);
		Func(pInfo, "Money", Money, 0x0, ClientID);
		
		if(rand()%5 != 0)
			Func(pInfo, "Shuriken", 1, aBuf, ClientID);

		if (m_RandNum % 3 == 0)
		{
			char *pName = NULL;
			char *pWeapInfo = NULL;
			GetRandWeapon(&pName, &pWeapInfo);
			if (pName != 0x0 && pWeapInfo != 0x0)
				Func(pInfo, pName, 1, pWeapInfo, ClientID);
		}
	}break;
	case RANDCONTENT_MEDIUM:
	{
		int Money = m_RandNum % 200 + 300;
		BaseFunc(pInfo, CHESTSIZE_SMALL, 1800);
		Func(pInfo, "Money", Money, 0x0, ClientID);
		if (m_RandNum % 2 == 0)
		{
			char *pName = NULL;
			char *pWeapInfo = NULL;
			GetRandWeapon(&pName, &pWeapInfo);
			if (pName != 0x0 && pWeapInfo != 0x0)
				Func(pInfo, pName, 1, pWeapInfo, ClientID);
		}
	} break;
	default:
	{
		dbg_msg("Chests", "Random Content-ID %i not found", m_ContentID);
	};
	}
}

float CChest::CooldownRatio()
{
	if(OnCooldown() == false)
		return 1.0;

	float RemainingTime = (m_CooldownTime-time_get()) / (float)time_freq();
	float Ratio = RemainingTime/(float)m_Cooldown;
	return Ratio;
}

bool CChest::OnCooldown()
{
	return m_CooldownTime >= time_get();
}

void CChest::SetCooldown()
{
	if(OnCooldown())
		return;

	m_CooldownTime = time_get()+time_freq()*m_Cooldown;
}

void CChest::SetDisplayPosition(CMap *pMap, vec2 Pos, int DisplaySize, int SnapID)
{
	if(m_DisplaySet)
		dbg_msg("Chest", "Display already set. ContentID=%i", m_ContentID);

	m_DisplaySet = true;
	m_DisplayPosition = Pos;
	m_pDisplayMap = pMap;
	m_DisplaySize = DisplaySize;
	m_SnapID = SnapID;
}

void CChest::SetContentID(int ContentID)
{
	m_ContentID = ContentID;
	DropRand(GetChestInfo, NULL, this);
}

bool CChest::GetDisplayPositions(vec2 *pDisplayPos, vec2 *pEndPos)
{
	if(OnCooldown())
	{
		vec2 EndPos = m_DisplayPosition + vec2(1, 0) * (float)(m_DisplaySize * CooldownRatio());

		*pDisplayPos = m_DisplayPosition;
		*pEndPos = EndPos;
		return true;
	}
	else if(m_LootRatio > 0.0f)
	{
		vec2 EndPos = m_DisplayPosition + vec2(1, 0) * (float)(m_DisplaySize * m_LootRatio);

		*pDisplayPos = m_DisplayPosition;
		*pEndPos = EndPos;
		return true;
	}

	return false;
}

void CChest::GetRandWeapon(char **pName, char **pInfo)
{
	if (m_ContentID == RANDCONTENT_SMALL)
	{
		int ChosenWeapon = m_RandNum % 4;
		switch (ChosenWeapon)
		{
		case 0: { *pName = "Gun"; *pInfo = "dmg=7"; } break;
		case 1: { *pName = "Hammer"; *pInfo = "dmg=32"; } break;
		case 2: { *pName = "Boomerang"; *pInfo = "dmg=12"; } break;
		case 3: { *pName = "Dagger"; *pInfo = "dmg=42"; } break;
		}
	}
	else if (m_ContentID == RANDCONTENT_MEDIUM)
	{
		int ChosenWeapon = m_RandNum % 7;
		switch (ChosenWeapon)
		{
		case 0: { *pName = "Gun"; *pInfo = "dmg=15"; } break;
		case 1: { *pName = "Hammer"; *pInfo = "dmg=53"; } break;
		case 2: { *pName = "Boomerang"; *pInfo = "dmg=25"; } break;
		case 3: { *pName = "Dagger"; *pInfo = "dmg=69"; } break;
		case 4: { *pName = "Shotgun"; *pInfo = "dmg=11"; } break;
		case 5: { *pName = "Rifle"; *pInfo = "dmg=98"; } break;
		case 6: { *pName = "Grenade"; *pInfo = "dmg=39"; } break;
		}
	}
}

void CChest::Tick()
{
	if(m_LootRatio > 0.0f && m_LootRatioDecreaseTime < time_get())
	{
		m_LootRatio = clamp(m_LootRatio-0.1f, 0.0f, 1.0f);

		if(m_LootRatio == 0.0f)
			m_LootID = -1;

		m_LootRatioDecreaseTime = time_get()+time_freq()*0.2f;
	}
}

bool CChest::WantLoot(int ClientID)
{
	if(OnCooldown())
		return false;

	if(m_LootID != -1 && m_LootID !=  ClientID)
		return false;//only allow the first who was on the chest to loot

	m_LootID = ClientID;
	m_LootRatio = clamp(m_LootRatio+0.005f, 0.0f, 1.0f);
	m_LootRatioDecreaseTime = time_get()+time_freq()*1.0f;
	return m_LootRatio == 1.0f;
}

void CChest::GetChestInfo(void *pInfo, int Size, int CoolDown)
{
	CChest *pThis = (CChest *)pInfo;

	pThis->m_Size = Size;
	pThis->m_Cooldown = CoolDown;
}