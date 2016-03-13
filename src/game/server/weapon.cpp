
#include <game/server/gamecontext.h>
#include <game/server/weapons/_include.h>

#include "weapon.h"

CWeapon *CreateWeapon(int Weapon, CGameContext *pGameServer, CTargetAble *pOwner, int Damage)
{
	switch(Weapon)
	{
	case WEAP_HAMMER: return new CHammer(pGameServer, pOwner, Damage);
	case WEAP_GUN: return new CGun(pGameServer, pOwner, Damage);
	case WEAP_SHOTGUN: return new CShotgun(pGameServer, pOwner, Damage);
	case WEAP_GRENADE: return new CGrenade(pGameServer, pOwner, Damage);
	case WEAP_RIFLE: return new CRifle(pGameServer, pOwner, Damage);
	case WEAP_DAGGER: return new CDagger(pGameServer, pOwner, Damage);
	case WEAP_FIREROD: return new CFireRod(pGameServer, pOwner, Damage);
	case WEAP_BOOMERANG: return new CBoomerang(pGameServer, pOwner, Damage);
	case WEAP_SWOSHROD: return new CSwoshRod(pGameServer, pOwner, Damage);
	case WEAP_SHURIKEN: return new CShuriken(pGameServer, pOwner, Damage);
	}

	return NULL;
}

CWeapon::CWeapon(CGameContext *pGameServer, CTargetAble *pOwner, int Type, int Damage, int MaxAmmo)
{
	m_pGameServer = pGameServer;
	m_pServer = pGameServer->Server();
	m_pOwner = pOwner;
	m_Type = Type;
	m_Ammo = MaxAmmo;
	m_Damage = Damage;
	m_MaxAmmo = MaxAmmo;
}

int CWeapon::Fire(vec2 PlayerPos, vec2 Direction, vec2 ProjPos, int *pMana)
{
	//ammo
	if(MaxAmmo() != -1)
		m_Ammo = clamp(m_Ammo -1, 0, MaxAmmo());
	
	//mana
	if(pMana)
	{
		int Costs = ManaCosts();
		if(*pMana < Costs)
			return 125;

		*pMana -= Costs;
	}

	return FireSub(PlayerPos, Direction, ProjPos);
}

void CWeapon::RenderLaser(int ID, int64 Tick, vec2 From, vec2 To)
{
	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)To.x;
	pObj->m_Y = (int)To.y;
	pObj->m_FromX = (int)From.x;
	pObj->m_FromY = (int)From.y;
	pObj->m_StartTick = Tick;
}

void CWeapon::RenderProjectile(int ID, int64 Tick, vec2 Pos, vec2 Vel, int Type)
{

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, ID, sizeof(CNetObj_Projectile)));
	if(!pProj)
		return;

	pProj->m_X = (int)Pos.x;
	pProj->m_Y = (int)Pos.y;
	pProj->m_VelX = (int)(Vel.x);
	pProj->m_VelY = (int)(Vel.y);
	pProj->m_StartTick = Tick;
	pProj->m_Type = Type;
}

void CWeapon::RenderPickup(int ID, vec2 Pos, int Type, int SubType)
{
	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)Pos.x;
	pP->m_Y = (int)Pos.y;
	pP->m_Type = Type;
	pP->m_Subtype = SubType;
}

void CWeapon::RenderFlag(vec2 Pos, int Team)
{
	CNetObj_Flag *pFlag = (CNetObj_Flag *)Server()->SnapNewItem(NETOBJTYPE_FLAG, Team, sizeof(CNetObj_Flag));
	if(!pFlag)
		return;

	pFlag->m_X = (int)Pos.x;
	pFlag->m_Y = (int)Pos.y;
	pFlag->m_Team = Team;
}