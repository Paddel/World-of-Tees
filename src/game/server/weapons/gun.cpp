
#include <game/server/gamecontext.h>
#include <game/server/targetable.h>
#include <game/server/entities/projectile.h>

#include "gun.h"

CGun::CGun(CGameContext *pGameServer, CTargetAble *pOwner, int Damage)
	: CWeapon(pGameServer, pOwner, WEAP_GUN, Damage, 700)
{
}

int CGun::FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos)
{
	int PlayerID = GameServer()->m_World.GetPlayerID(Owner());
	GameServer()->CreateSound(PlayerPos, SOUND_GUN_FIRE, Owner()->TargetMap());

	CProjectile *pProj = new CProjectile(&GameServer()->m_World, Owner(), WEAPON_GUN,
		ProjPos,
		Direction,
		(int)(Server()->TickSpeed()*2.0f),
		Damage(), 0, 2, -1, WEAPON_GUN, Owner()->TargetMap());

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

	return g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Firedelay;
}