#pragma once

#include <base/vmath.h>

class CGameContext;
class IServer;
class CTargetAble;
class CWeapon;

enum
{
	WEAP_HAMMER=0,
	WEAP_GUN,
	WEAP_SHOTGUN,
	WEAP_GRENADE,
	WEAP_RIFLE,
	WEAP_DAGGER,
	WEAP_FIREROD,
	WEAP_BOOMERANG,
	WEAP_SWOSHROD,
	WEAP_SHURIKEN,
	NUM_WEAPS,
};

static char *s_aWeaponShortcuts[NUM_WEAPS] = {
	"HM",
	"GN",
	"SG",
	"GR",
	"RF",
	"DG",
	"FR",
	"BG",
	"SR",
	"SK",
};

static char *s_aWeaponNames[NUM_WEAPS] = {
	"Hammer",
	"Gun",
	"Shotgun",
	"Grenade",
	"Rifle",
	"Dagger",
	"Firerod",
	"Boomerang",
	"Swoshrod",
	"Shuriken",
};

enum
{
	WEAP_MAX_LEN=128,//changing this also requires a change in database
};

static int GetWeaponType(const char *pName)
{
	for(int i = 0; i < NUM_WEAPS; i++)
	{
		if(str_comp_nocase(s_aWeaponNames[i], pName) == 0)
			return i;
	}
	return -1;
};

CWeapon *CreateWeapon(int Weapon, CGameContext *pGameServer, CTargetAble *pOwner, int Damage);

class CWeapon
{
private:
	CGameContext *m_pGameServer;
	IServer *m_pServer;
	CTargetAble *m_pOwner;
	int m_Type;
	int m_Ammo;
	int m_Damage;
	int m_MaxAmmo;

public:
	CWeapon(CGameContext *pGameServer, CTargetAble *pOwner, int Type, int Damage, int MaxAmmo);

	int Fire(vec2 PlayerPos, vec2 Direction, vec2 ProjPos, int *pMana);

	int GetType() const { return m_Type; }
	int Ammo() const { return m_Ammo; }
	int Damage() const { return m_Damage; }
	int MaxAmmo() const { return m_MaxAmmo; }

	void SetAmmo(int Ammo) { m_Ammo = clamp(Ammo, -1, m_MaxAmmo); }
	void AddAmmo(int Amount) { m_Ammo = clamp(m_Ammo + Amount, -1, m_MaxAmmo); }

	virtual void Destroy() {}

	virtual int FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos) = 0;//returns cooldown
	virtual bool SpecialWorldSnap(int SnappingClient, vec2 Pos, vec2 Direction, int Attacktick) = 0;
	
	virtual int SnapWeapon() = 0;
	virtual int ManaCosts() = 0;
	virtual int AmmoCosts() = 0;
	virtual bool FullAuto() = 0;
	virtual bool Ranged() = 0;
	virtual bool ShootAnim() = 0;
	virtual bool StackAble() = 0;

	void RenderLaser(int ID, int64 Tick, vec2 From, vec2 To);
	void RenderProjectile(int ID, int64 Tick, vec2 Pos, vec2 Vel, int Type);
	void RenderPickup(int ID, vec2 Pos, int Type, int SubType);
	void RenderFlag(vec2 Pos, int Team);

	void SetOwner(CTargetAble *pOwner) { m_pOwner = pOwner; }

	CGameContext *GameServer() const { return m_pGameServer; }
	CTargetAble *Owner() const { return m_pOwner; }
	IServer *Server() const { return m_pServer; }
};