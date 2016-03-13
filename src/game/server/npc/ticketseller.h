#pragma once

#include <game/server/npc.h>

class CGameContext;
class CMap;

class CTicketSeller : public CNpc, public CTargetAble
{
private:

protected:
	vec2 m_SpawnPos;
	bool m_RandJump;
	int m_Ticket;
	int m_Level;
	int m_Costs;

	struct
	{
		bool m_Close;
	} m_aSell[MAX_CLIENTS];

public:
	CTicketSeller(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos, int HelpingText);

	static void BuyTicketStatic(void *pInfo, int ClientID);
	void BuyTicket(int ClientID);

	int GetLevel() const { return m_Level; }
	int GetCosts() const { return m_Costs; }

	virtual void Tick();
	virtual void Snap(int SnappingClient);

	virtual void SetInput();
	virtual void NoTargetInput();

	virtual void SubSpawn();
	virtual void OnDeath(int From, int Weapon, vec2 DeathVel);
	virtual bool Alive() { return true; }

	//targetable
	virtual void ResetEnemy(CTargetAble *pTarget) {}
	virtual vec2 GetClosestPos(vec2 Pos) { return GetPos(); }
	virtual vec2 GetIntersectPos(vec2 Pos0, vec2 Pos1) { return closest_point_on_line(Pos0, Pos1, GetPos()); }
	virtual bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon);
	virtual int GetProximityRadius() { return 28; }
	virtual bool IsEnemy(CTargetAble *pTarget);
	virtual bool IsBuilding() { return false; }
	//virtual void Slow(float t, float SlowRate) { m_Core.Slow(t, SlowRate); }

	char *NpcName() { return "Ticket Seller"; }
	int MaxHealth() { return 1; }
	int MoneyAmount() { return 2000; }
	int MoneyAddAmount() { return 500; }
	int BaseDamage() { return 0; }
	int Experience() { return 500; }
	int Weapon() { return WEAPON_HAMMER; }
	
	//skin
	char *SkinName() { return "limekitty"; }
	int SkinCostumColor() { return 1; }
	int SkinColorBody() { return 5220274; }
	int SkinColorFeet() { return 102; }

	//overwriteable functions
	virtual void SetExtraCollision();
};