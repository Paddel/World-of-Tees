#pragma once

#include <game/server/npc.h>

class CGameContext;
class CMap;

class CAmmoShop : public CNpc
{
private:

protected:
	vec2 m_SpawnPos;
	bool m_RandJump;

	struct
	{
		int m_WeaponType;
		bool m_Close;
		int64 m_ChatTime;
	} m_aSell[MAX_CLIENTS];

public:
	CAmmoShop(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos);

	static void BuyAmmoStatic(void *pInfo, int ClientID);
	void BuyAmmo(int ClientID);

	virtual void Tick();
	virtual void Snap(int SnappingClient);

	virtual void SetInput();
	virtual void NoTargetInput();

	virtual void SubSpawn();
	virtual void OnDeath(int From, int Weapon, vec2 DeathVel);
	virtual bool Alive() { return true; }

	char *NpcName() { return "Ammo Seller"; }
	int MaxHealth() { return 1; }
	int MoneyAmount() { return 2000; }
	int MoneyAddAmount() { return 500; }
	int BaseDamage() { return 0; }
	int Experience() { return 500; }
	int Weapon() { return WEAPON_HAMMER; }
	
	//skin
	char *SkinName() { return "coala"; }
	int SkinCostumColor() { return 1; }
	int SkinColorBody() { return 344109; }
	int SkinColorFeet() { return 2083840; }

	//overwriteable functions
	virtual void SetExtraCollision();
};