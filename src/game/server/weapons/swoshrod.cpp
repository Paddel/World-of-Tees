
#include <game/balancing.h>
#include <game/server/gamecontext.h>
#include <game/server/targetable.h>
#include <game/server/entities/character.h>

#include "swoshrod.h"


CSwoshRod::CSwoshRod(CGameContext *pGameServer, CTargetAble *pOwner, int Damage)
	: CWeapon(pGameServer, pOwner, WEAP_SWOSHROD, Damage, -1)
{
	int NumSnapIDs = sizeof(m_aSnapIDs)/sizeof(m_aSnapIDs[0]);
	for(int i = 0; i < NumSnapIDs; i++)
		m_aSnapIDs[i] = pGameServer->Server()->SnapNewID();
}

void CSwoshRod::Destroy()
{
	int NumSnapIDs = sizeof(m_aSnapIDs)/sizeof(m_aSnapIDs[0]);
	for(int i = 0; i < NumSnapIDs; i++)
		GameServer()->Server()->SnapFreeID(m_aSnapIDs[i]);
}

int CSwoshRod::FireSub(vec2 PlayerPos, vec2 Direction, vec2 ProjPos)
{
	int OwnerID = GameServer()->m_World.GetPlayerID(Owner());
	CCharacter *pChr = GameServer()->GetPlayerChar(OwnerID);
	if (pChr)
		pChr->Core()->m_Vel = normalize(pChr->MousePos()) * Damage();
	
	return 0;
}

bool CSwoshRod::SpecialWorldSnap(int SnappingClient, vec2 Pos, vec2 Direction, int Attacktick)
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
	RenderProjectile(m_aSnapIDs[3], Server()->Tick()-10, To+Direction*20, vec2(0, 0), WEAPON_GUN);
	return true;
}