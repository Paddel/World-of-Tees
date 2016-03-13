
#include <game/server/gamecontext.h>
#include <game/server/targetable.h>
#include <game/server/entities/projectile.h>

#include "shotgun.h"

CShotgun::CShotgun(CGameContext *pGameServer, CTargetAble *pOwner, int Damage)
	: CWeapon(pGameServer, pOwner, WEAP_SHOTGUN, Damage, 400)
{
}

int CShotgun::FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos)
{
	int PlayerID = GameServer()->m_World.GetPlayerID(Owner());
	int ShotSpread = 2;

	GameServer()->CreateSound(PlayerPos, SOUND_SHOTGUN_FIRE, Owner()->TargetMap());

	CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
	Msg.AddInt(ShotSpread*2+1);

	for(int i = -ShotSpread; i <= ShotSpread; ++i)
	{
		float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
		float a = GetAngle(Direction);
		a += Spreading[i+2];
		float v = 1-(absolute(i)/(float)ShotSpread);
		float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
		CProjectile *pProj = new CProjectile(&GameServer()->m_World, Owner(), WEAPON_SHOTGUN,
			ProjPos,
			vec2(cosf(a), sinf(a))*Speed,
			(int)(Server()->TickSpeed()*0.20f),
			Damage(), 0, 10, -1, WEAPON_SHOTGUN, Owner()->TargetMap());

		if(PlayerID != -1)
		{
			// pack the Projectile and send it to the client Directly
			CNetObj_Projectile p;
			pProj->FillInfo(&p);

			for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
				Msg.AddInt(((int *)&p)[i]);
		}
	}

	if(PlayerID != -1)
		Server()->SendMsg(&Msg, 0,PlayerID);

	return g_pData->m_Weapons.m_aId[WEAPON_SHOTGUN].m_Firedelay;
}

bool CShotgun::FullAuto()
{
	return true;
}