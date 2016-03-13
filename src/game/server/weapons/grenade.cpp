
#include <game/server/gamecontext.h>
#include <game/server/targetable.h>
#include <game/server/entities/projectile.h>

#include "grenade.h"

CGrenade::CGrenade(CGameContext *pGameServer, CTargetAble *pOwner, int Damage)
	: CWeapon(pGameServer, pOwner, WEAP_GRENADE, Damage, 400)
{
}

int CGrenade::FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos)
{
	int PlayerID = GameServer()->m_World.GetPlayerID(Owner());
	GameServer()->CreateSound(PlayerPos, SOUND_GRENADE_FIRE, Owner()->TargetMap());

	CProjectile *pProj = new CProjectile(&GameServer()->m_World, Owner(), WEAPON_GRENADE,
		ProjPos,
		Direction,
		(int)(Server()->TickSpeed()*2.0f),
		Damage(), true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE, Owner()->TargetMap());

	if(PlayerID != -1)
	{
		// pack the Projectile and send it to the client Directly
		CNetObj_Projectile p;
		pProj->FillInfo(&p);

		CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
		Msg.AddInt(1);
		for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
			Msg.AddInt(((int *)&p)[i]);
		Server()->SendMsg(&Msg, 0, PlayerID);
	}

	return g_pData->m_Weapons.m_aId[WEAPON_GRENADE].m_Firedelay;
}

bool CGrenade::FullAuto()
{
	return true;
}