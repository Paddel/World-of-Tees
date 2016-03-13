
#include <game/server/gamecontext.h>
#include <game/server/targetable.h>
#include <game/server/entities/laser.h>

#include "rifle.h"

CRifle::CRifle(CGameContext *pGameServer, CTargetAble *pOwner, int Damage)
	: CWeapon(pGameServer, pOwner, WEAP_RIFLE, Damage, 300)
{
}

int CRifle::FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos)
{
	GameServer()->CreateSound(PlayerPos, SOUND_RIFLE_FIRE, Owner()->TargetMap());

	new CLaser(&GameServer()->m_World, PlayerPos, Direction, GameServer()->Tuning()->m_LaserReach, Owner(), Owner()->TargetMap(), Damage());

	return g_pData->m_Weapons.m_aId[WEAPON_RIFLE].m_Firedelay;
}