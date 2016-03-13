
#include <game/balancing.h>
#include <game/server/gamecontext.h>
#include <game/server/targetable.h>
#include <game/server/entities/character.h>
#include <game/server/entities/firetile.h>

#include "firerod.h"


CFireRod::CFireRod(CGameContext *pGameServer, CTargetAble *pOwner, int Damage)
	: CWeapon(pGameServer, pOwner, WEAP_FIREROD, Damage, -1)
{
	int NumSnapIDs = sizeof(m_aSnapIDs)/sizeof(m_aSnapIDs[0]);
	for(int i = 0; i < NumSnapIDs; i++)
		m_aSnapIDs[i] = pGameServer->Server()->SnapNewID();

	m_Cooldown = 0;
	m_ManaCooldown = 0;
}

void CFireRod::Destroy()
{
	int NumSnapIDs = sizeof(m_aSnapIDs)/sizeof(m_aSnapIDs[0]);
	for(int i = 0; i < NumSnapIDs; i++)
		GameServer()->Server()->SnapFreeID(m_aSnapIDs[i]);
}

int CFireRod::FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos)
{
	if(m_Cooldown > Server()->Tick())
		return 0;

	vec2 To = PlayerPos+Direction*105;

	if(m_ManaCooldown < Server()->Tick())
		m_ManaCooldown = Server()->Tick() + Server()->TickSpeed()*0.5f;

	if(Owner()->TargetMap()->Collision()->IntersectLine(PlayerPos, To, NULL, NULL))
		return 0;

	GameServer()->CreateSound(PlayerPos, SOUND_CHAT_CLIENT, Owner()->TargetMap());

	float Angle = GetAngle(Direction);
	Angle += (((rand()%21)-10)/10.0f)*0.5f;
	vec2 ProjDir = vec2(cosf(Angle), sin(Angle));

	new CFireTile(&GameServer()->m_World, Owner(), To, ProjDir, Damage(), WEAPON_HAMMER, Owner()->TargetMap());
	m_Cooldown = Server()->Tick()+Server()->TickSpeed()/1000.0f * 50.0f;

	return 0;
}

bool CFireRod::SpecialWorldSnap(int SnappingClient, vec2 Pos, vec2 Direction, int Attacktick)
{
	static const float s_WeaponShiftTime = Server()->TickSpeed()*0.1f;

	float WeaponShift = 0.0f;
	if(Attacktick + s_WeaponShiftTime > Server()->Tick())
		WeaponShift = (Attacktick + s_WeaponShiftTime-Server()->Tick())/s_WeaponShiftTime;

	vec2 From = Pos+Direction*(-42 + 20 * WeaponShift);
	vec2 To = Pos+Direction*(72 + 20 * WeaponShift);
	RenderLaser(m_aSnapIDs[0], Server()->Tick(), To, From);

	//rotate 90
	vec2 NewDir = Direction;
	float Buf = NewDir.x*-1;
	NewDir.x = NewDir.y;
	NewDir.y = Buf;
	RenderLaser(m_aSnapIDs[1], Server()->Tick()-13, To+NewDir*24, To+NewDir*(-24));
	RenderLaser(m_aSnapIDs[2], Server()->Tick(), To+NewDir*24, To+NewDir*24);
	RenderProjectile(m_aSnapIDs[3], Server()->Tick()-10, To+Direction*20, vec2(0, 0), WEAPON_GRENADE);
	return true;
}

int CFireRod::ManaCosts()
{
	if(m_ManaCooldown < Server()->Tick())
		return 1;

	return 0;
}