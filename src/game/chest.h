#pragma once

#include <base/vmath.h>

class CMap;

typedef void (*ChestFunc)(void *pInfo, int Size, int CoolDown);
typedef void (*DropFunc)(void *pInfo, char *pType, int Num, char *pExtraInfo, int ClientID);

enum
{
	CHESTSIZE_SMALL=0,
	CHESTSIZE_MEDIUM,
	CHESTSIZE_BIG,
};

enum
{
	RANDCONTENT_SMALL,
	RANDCONTENT_MEDIUM,
};

class CChest
{
private:
	int m_SnapID;
	bool m_DisplaySet;
	CMap *m_pDisplayMap;
	vec2 m_DisplayPosition;
	int64 m_CooldownTime;
	int m_DisplaySize;

	int64 m_LootRatioDecreaseTime;
	int m_LootID;
	float m_LootRatio;

	int m_ContentID;
	int m_Cooldown;
	int m_Size;
	int m_RandNum;

public:
	CChest();
	CChest(int ContentID);

	void Drop(ChestFunc BaseFunc, DropFunc Func, void *pInfo, int ClientID = -1);
	void DropRand(ChestFunc BaseFunc, DropFunc Func, void *pInfo, int ClientID = -1);

	//Serverside
	float CooldownRatio();
	bool OnCooldown();
	void SetCooldown();
	void SetDisplayPosition(CMap *pMap, vec2 Pos, int DisplaySize, int SnapID);
	void SetContentID(int ContentID);
	bool GetDisplayPositions(vec2 *pDisplayPos, vec2 *pEndPos);
	bool GetDisplaySet() const { return m_DisplaySet; }
	int GetSnapID() const { return m_SnapID; }
	CMap *GetDisplayMap() const { return m_pDisplayMap; }
	int GetCooldown() const { return m_Cooldown; }
	int GetContentID() const { return m_ContentID; }

	void GetRandWeapon(char **pName, char **pInfo);

	void Tick();
	bool WantLoot(int ClientID);

	static void GetChestInfo(void *pInfo, int Size, int CoolDownTime);

	static void ChestInfoTemp(void *pInfo, int Size, int CoolDownTime) {}
	static void ChestDropTemp(void *pInfo, char *pType, int Num, char *pExtraInfo, int ClientID) {}
};

static CChest s_aChests[] = {
	CChest(0),
	CChest(1),
	CChest(2),
	CChest(1),
};