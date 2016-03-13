/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <engine/server/maploader.h>
#include <game/mapitems.h>
#include <game/server/gamecontext.h>

#include "srvgamecore.h"

void CSrvCharacterCore::Init(CSrvWorldCore *pWorld, CCollision *pCollision, CGameContext *pGameServer, CMap *pMap)
{
	m_pWorld = pWorld;
	m_pCollision = pCollision;
	m_pGameServer = pGameServer;
	m_pMap = pMap;
	m_Speed = m_pWorld->m_Tuning.m_GroundControlSpeed;
}

void CSrvCharacterCore::Reset()
{
	m_Pos = vec2(0,0);
	m_Vel = vec2(0,0);
	m_HookPos = vec2(0,0);
	m_HookDir = vec2(0,0);
	m_HookTick = 0;
	m_HookState = HOOK_IDLE;
	m_HookedPlayer = -1;
	m_Jumped = 0;
	m_TriggeredEvents = 0;
	m_BotHooked = false;
	m_ForceDirection = 0;
}

void CSrvCharacterCore::Tick(bool UseInput, bool UseCol)
{
	float PhysSize = 28.0f;
	m_TriggeredEvents = 0;

	m_Colliding = false;

	if(m_pCollision->GetCollisionAt(m_Pos) == TILE_WATER)
		m_InWater = true;
	else
		m_InWater = false;

	if (m_pCollision->GetExTile(m_Pos.x, m_Pos.y + PhysSize / 2 + 5) == EXTILE_ICE)//m_pCollision->GetCollisionAt(m_Pos.x, m_Pos.y + PhysSize / 2 + 5)
		m_OnIce = true;
	else
		m_OnIce = false;

	// get ground state
	bool Grounded = false;
	if(m_pCollision->CheckPoint(m_Pos.x+PhysSize/2, m_Pos.y+PhysSize/2+5))
		Grounded = true;
	if(m_pCollision->CheckPoint(m_Pos.x-PhysSize/2, m_Pos.y+PhysSize/2+5))
		Grounded = true;

	vec2 TargetDirection = normalize(vec2(m_Input.m_TargetX, m_Input.m_TargetY));

	if(!m_InWater)
		m_Vel.y += m_pWorld->m_Tuning.m_Gravity;
	else
		m_Vel.y += m_pWorld->m_Tuning.m_Gravity*0.27f;

	float MaxSpeed = Grounded ? m_Speed : m_Speed*0.5f;
	float Accel = Grounded ? m_pWorld->m_Tuning.m_GroundControlAccel : m_pWorld->m_Tuning.m_AirControlAccel;
	float Friction = Grounded ? m_pWorld->m_Tuning.m_GroundFriction : m_pWorld->m_Tuning.m_AirFriction;

	if (m_OnIce)
	{
		Friction = 0.96f;
		Accel = 0.2f;
	}

	// handle input
	if(UseInput)
	{
		m_Direction = m_Input.m_Direction;

		// setup angle
		float a = 0;
		if(m_Input.m_TargetX == 0)
			a = atanf((float)m_Input.m_TargetY);
		else
			a = atanf((float)m_Input.m_TargetY/(float)m_Input.m_TargetX);

		if(m_Input.m_TargetX < 0)
			a = a+pi;

		m_Angle = (int)(a*256.0f);

		// handle jump
		if(m_Input.m_Jump)
		{
			if(!(m_Jumped&1))
			{
				if(Grounded)
				{
					m_TriggeredEvents |= COREEVENT_GROUND_JUMP;
					m_Vel.y = -m_pWorld->m_Tuning.m_GroundJumpImpulse;
					m_Jumped |= 1;
				}
				else if(!(m_Jumped&2))
				{
					m_TriggeredEvents |= COREEVENT_AIR_JUMP;
					m_Vel.y = -m_pWorld->m_Tuning.m_AirJumpImpulse;
					m_Jumped |= 3;
				}
			}
		}
		else
		{
			m_Jumped &= ~1;

			if(m_InWater && !Grounded)
				m_Jumped = 0;
		}

		// handle hook
		if(m_Input.m_Hook)
		{
			if(m_HookState == HOOK_IDLE)
			{
				m_HookState = HOOK_FLYING;
				m_HookPos = m_Pos+TargetDirection*PhysSize*1.5f;
				m_HookDir = TargetDirection;
				m_HookedPlayer = -1;
				m_HookTick = 0;
				m_TriggeredEvents |= COREEVENT_HOOK_LAUNCH;
			}
		}
		else
		{
			m_HookedPlayer = -1;
			m_HookState = HOOK_IDLE;
			m_HookPos = m_Pos;
		}
	}

	// add the speed modification according to players wanted direction
	if(m_Direction < 0)
		m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Vel.x, -Accel);
	if(m_Direction > 0)
		m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Vel.x, Accel);
	if(m_Direction == 0)
		m_Vel.x *= Friction;

	// handle jumping
	// 1 bit = to keep track if a jump has been made on this input
	// 2 bit = to keep track if a air-jump has been made
	if(Grounded)
		m_Jumped &= ~2;

	// do hook
	if(m_HookState == HOOK_IDLE)
	{
		m_HookedPlayer = -1;
		m_HookState = HOOK_IDLE;
		m_HookPos = m_Pos;
	}
	else if(m_HookState >= HOOK_RETRACT_START && m_HookState < HOOK_RETRACT_END)
	{
		m_HookState++;
	}
	else if(m_HookState == HOOK_RETRACT_END)
	{
		m_HookState = HOOK_RETRACTED;
		m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
		m_HookState = HOOK_RETRACTED;
	}
	else if(m_HookState == HOOK_FLYING)
	{
		vec2 NewPos = m_HookPos+m_HookDir*m_pWorld->m_Tuning.m_HookFireSpeed;
		if(distance(m_Pos, NewPos) > m_pWorld->m_Tuning.m_HookLength)
		{
			m_HookState = HOOK_RETRACT_START;
			NewPos = m_Pos + normalize(NewPos-m_Pos) * m_pWorld->m_Tuning.m_HookLength;
		}

		// make sure that the hook doesn't go though the ground
		bool GoingToHitGround = false;
		bool GoingToRetract = false;
		int Hit = m_pCollision->IntersectLine(m_HookPos, NewPos, &NewPos, 0);
		if(Hit)
		{
			if(Hit == TILE_NOHOOK)
				GoingToRetract = true;
			else
				GoingToHitGround = true;
		}

		// Check against other players first
		if(m_pWorld && m_pWorld->m_Tuning.m_PlayerHooking)
		{
			float Distance = 0.0f;
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				CSrvCharacterCore *pCharCore = m_pWorld->m_apCharacters[i];
				if(!pCharCore || pCharCore == this)
					continue;

				if(pCharCore->m_pMap != Map())
					continue;

				vec2 ClosestPoint = closest_point_on_line(m_HookPos, NewPos, pCharCore->m_Pos);
				if(distance(pCharCore->m_Pos, ClosestPoint) < PhysSize+2.0f)
				{
					if (m_HookedPlayer == -1 || distance(m_HookPos, pCharCore->m_Pos) < Distance)
					{
						m_TriggeredEvents |= COREEVENT_HOOK_ATTACH_PLAYER;
						m_HookState = HOOK_GRABBED;
						m_HookedPlayer = i;
						m_BotHooked = false;
						Distance = distance(m_HookPos, pCharCore->m_Pos);
					}
				}
			}

			CWorldSection *pSection = m_pMap->WorldSection();
			for(int i = 0; i < pSection->m_lpPlayerItems.size(); i++)
			{
				CPlayerItem *pPlayerItem = pSection->m_lpPlayerItems[i];
				if(!pPlayerItem)
					continue;

				if(pPlayerItem->GetCore() == this)
					continue;

				if(!(int)(pPlayerItem->GetInterFlags()&INTERFLAG_HOOK))
					continue;

				vec2 ClosestPoint = closest_point_on_line(m_HookPos, NewPos, pPlayerItem->GetPos());
				if(distance(pPlayerItem->GetPos(), ClosestPoint) < PhysSize+2.0f)
				{
					if (m_HookedPlayer == -1 || distance(m_HookPos, pPlayerItem->GetPos()) < Distance)
					{
						m_TriggeredEvents |= COREEVENT_HOOK_ATTACH_PLAYER;
						m_HookState = HOOK_GRABBED;
						m_HookedPlayer = i;
						m_BotHooked = true;
						Distance = distance(m_HookPos, pPlayerItem->GetPos());
					}
				}
			}
		}

		if(m_HookState == HOOK_FLYING)
		{
			// check against ground
			if(GoingToHitGround)
			{
				m_TriggeredEvents |= COREEVENT_HOOK_ATTACH_GROUND;
				m_HookState = HOOK_GRABBED;

				if(Hit != TILE_SOLID)//client doesnt predict => no sound
					m_pGameServer->CreateSound(NewPos, SOUND_HOOK_ATTACH_GROUND, Map());
			}
			else if(GoingToRetract)
			{
				m_TriggeredEvents |= COREEVENT_HOOK_HIT_NOHOOK;
				m_HookState = HOOK_RETRACT_START;
			}

			m_HookPos = NewPos;
		}
	}

	if(m_HookState == HOOK_GRABBED)
	{
		if(m_HookedPlayer != -1)
		{
			if(m_BotHooked)
			{
				CWorldSection *pSection = m_pMap->WorldSection();
				CPlayerItem *pPlayerItem = pSection->m_lpPlayerItems[m_HookedPlayer];
				if(pPlayerItem && pPlayerItem->GetCore()->m_ForceDirection == false)
					m_HookPos = pPlayerItem->GetPos();
				else
				{
					// release hook
					m_HookedPlayer = -1;
					m_HookState = HOOK_RETRACTED;
					m_HookPos = m_Pos;
					m_BotHooked = false;
				}
			}
			else
			{
				CSrvCharacterCore *pCharCore = m_pWorld->m_apCharacters[m_HookedPlayer];
				if(pCharCore && pCharCore->m_ForceDirection == false)
					m_HookPos = pCharCore->m_Pos;
				else
				{
					// release hook
					m_HookedPlayer = -1;
					m_HookState = HOOK_RETRACTED;
					m_HookPos = m_Pos;
					m_BotHooked = false;
				}
			}

			// keep players hooked for a max of 1.5sec
			//if(Server()->Tick() > hook_tick+(Server()->TickSpeed()*3)/2)
				//release_hooked();
		}

		// don't do this hook rutine when we are hook to a player
		if(m_HookedPlayer == -1 && distance(m_HookPos, m_Pos) > 46.0f)
		{
			vec2 HookVel = normalize(m_HookPos-m_Pos)*m_pWorld->m_Tuning.m_HookDragAccel;
			// the hook as more power to drag you up then down.
			// this makes it easier to get on top of an platform
			if(HookVel.y > 0)
				HookVel.y *= 0.3f;

			// the hook will boost it's power if the player wants to move
			// in that direction. otherwise it will dampen everything abit
			if((HookVel.x < 0 && m_Direction < 0) || (HookVel.x > 0 && m_Direction > 0))
				HookVel.x *= 0.95f;
			else
				HookVel.x *= 0.75f;

			vec2 NewVel = m_Vel+HookVel;

			// check if we are under the legal limit for the hook
			if(length(NewVel) < m_pWorld->m_Tuning.m_HookDragSpeed || length(NewVel) < length(m_Vel))
				m_Vel = NewVel; // no problem. apply

		}

		// release hook (max hook time is 1.25
		m_HookTick++;
		if(m_HookedPlayer != -1)
		{
			CWorldSection *pSection = m_pMap->WorldSection();
			if(m_HookTick > SERVER_TICK_SPEED+SERVER_TICK_SPEED/5 || (m_BotHooked?!pSection->m_lpPlayerItems[m_HookedPlayer] : !m_pWorld->m_apCharacters[m_HookedPlayer]))
			{
				m_HookedPlayer = -1;
				m_HookState = HOOK_RETRACTED;
				m_HookPos = m_Pos;
				m_BotHooked = false;
			}
		}
	}

	if(m_pWorld)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			CSrvCharacterCore *pCharCore = m_pWorld->m_apCharacters[i];
			if(!pCharCore)
				continue;

			//player *p = (player*)ent;
			if(pCharCore == this) // || !(p->flags&FLAG_ALIVE)
				continue; // make sure that we don't nudge our self

			if(pCharCore->m_pMap != Map())
				continue;

			// handle player <-> player collision
			float Distance = distance(m_Pos, pCharCore->m_Pos);
			vec2 Dir = normalize(m_Pos - pCharCore->m_Pos);
			if(m_pWorld->m_Tuning.m_PlayerCollision && Distance < PhysSize*1.25f && Distance > 0.0f && UseCol)
			{
				float a = (PhysSize*1.45f - Distance);
				float Velocity = 0.5f;

				// make sure that we don't add excess force by checking the
				// direction against the current velocity. if not zero.
				if (length(m_Vel) > 0.0001)
					Velocity = 1-(dot(normalize(m_Vel), Dir)+1)/2;

				m_Vel += Dir*a*(Velocity*0.75f);
				m_Vel *= 0.85f;
				m_Colliding = true;

				if(Distance == 0)
				{
					m_Vel = vec2(850, 0);
					pCharCore->m_Vel = vec2(-850, 0);
				}
			}

			// handle hook influence
			if(m_HookedPlayer == i && m_pWorld->m_Tuning.m_PlayerHooking && m_BotHooked == false)
			{
				if(Distance > PhysSize*1.50f) // TODO: fix tweakable variable
				{
					float Accel = m_pWorld->m_Tuning.m_HookDragAccel * (Distance/m_pWorld->m_Tuning.m_HookLength);
					float DragSpeed = m_pWorld->m_Tuning.m_HookDragSpeed;

					// add force to the hooked player
					pCharCore->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.x, Accel*Dir.x*1.5f);
					pCharCore->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.y, Accel*Dir.y*1.5f);

					// add a little bit force to the guy who has the grip
					m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.x, -Accel*Dir.x*0.25f);
					m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.y, -Accel*Dir.y*0.25f);
				}
			}
		}

		CWorldSection *pSection = m_pMap->WorldSection();
		for(int i = 0; i < pSection->m_lpPlayerItems.size(); i++)
		{
			CPlayerItem *pPlayerItem = pSection->m_lpPlayerItems[i];
			if(!pPlayerItem)
				continue;

			if(pPlayerItem->GetCore() == this)
				continue;

			static const int MinDist = PhysSize*1.25f;
			//if(abs(pPlayerItem->GetPos().x - m_Pos.x) > MinDist || abs(pPlayerItem->GetPos().y - m_Pos.y) > MinDist)//if one coordinate is over range
			//	continue;

			// handle player <-> player collision
			float Distance = distance(m_Pos, pPlayerItem->GetPos());
			vec2 Dir = normalize(m_Pos - pPlayerItem->GetPos());

			if((int)(pPlayerItem->GetInterFlags()&INTERFLAG_COL))
			{
				if(m_pWorld->m_Tuning.m_PlayerCollision && Distance < MinDist && UseCol)
				{
					float a = (PhysSize*1.45f - Distance);
					float Velocity = 0.5f;

					// make sure that we don't add excess force by checking the
					// direction against the current velocity. if not zero.
					if (length(m_Vel) > 0.0001)
						Velocity = 1-(dot(normalize(m_Vel), Dir)+1)/2;

					m_Vel += Dir*a*(Velocity*0.75f);
					m_Vel *= 0.85f;
					m_Colliding = true;

					if(Distance == 0)
					{
						m_Vel = vec2(850, 0);
						pPlayerItem->GetCore()->m_Vel = vec2(-850, 0);
					}
				}
			}

			if((int)(pPlayerItem->GetInterFlags()&INTERFLAG_HOOK))
			{
				// handle hook influence
				if(m_HookedPlayer == i && m_pWorld->m_Tuning.m_PlayerHooking && m_BotHooked == true)
				{
					if(Distance > PhysSize*1.50f) // TODO: fix tweakable variable
					{
						float Accel = m_pWorld->m_Tuning.m_HookDragAccel * (Distance/m_pWorld->m_Tuning.m_HookLength);
						float DragSpeed = m_pWorld->m_Tuning.m_HookDragSpeed;

						// add force to the hooked player
						pPlayerItem->GetCore()->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pPlayerItem->GetCore()->m_Vel.x, Accel*Dir.x*1.5f);
						pPlayerItem->GetCore()->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pPlayerItem->GetCore()->m_Vel.y, Accel*Dir.y*1.5f);

						// add a little bit force to the guy who has the grip
						m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.x, -Accel*Dir.x*0.25f);
						m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.y, -Accel*Dir.y*0.25f);
					}
				}
			}
		}

	}

	// clamp the velocity to something sane
	if(length(m_Vel) > 6000)
		m_Vel = normalize(m_Vel) * 6000;
}

void CSrvCharacterCore::Move(float Slow)
{
	float RampValue = VelocityRamp(length(m_Vel)*50, m_pWorld->m_Tuning.m_VelrampStart, m_pWorld->m_Tuning.m_VelrampRange, m_pWorld->m_Tuning.m_VelrampCurvature);

	m_Vel.x = m_Vel.x*RampValue;

	if(Slow < 1.0f)
		m_Vel.x = clamp(m_Vel.x, m_Speed*Slow*-1, m_Speed*Slow);

	vec2 NewPos = m_Pos;
	m_pCollision->MoveBox(&NewPos, &m_Vel, vec2(28.0f, 28.0f), 0);

	m_Vel.x = m_Vel.x*(1.0f/RampValue);

	if(m_pWorld && m_pWorld->m_Tuning.m_PlayerCollision)
	{
		// check player collision
		float Distance = distance(m_Pos, NewPos);
		int End = Distance+1;
		vec2 LastPos = m_Pos;

		for(int i = 0; i < End; i++)
		{
			float a = i/Distance;
			vec2 Pos = mix(m_Pos, NewPos, a);

			CWorldSection *pSection = m_pMap->WorldSection();
			for(int i = 0; i < pSection->m_lpPlayerItems.size(); i++)
			{
				CPlayerItem *pPlayerItem = pSection->m_lpPlayerItems[i];
				if(!pPlayerItem)
					continue;

				if(pPlayerItem->GetCore() == this)
					continue;

				if(!(int)(pPlayerItem->GetInterFlags()&INTERFLAG_COL))
					continue;

				static const int MinDist = 28.0f;
				if(abs(pPlayerItem->GetPos().x - Pos.x) > MinDist || abs(pPlayerItem->GetPos().y - Pos.y) > MinDist)//if one coordinate is over range
					continue;

				float D = distance(Pos, pPlayerItem->GetPos());
				if(D < 28.0f && D > 0.0f)
				{
					if(a > 0.0f)
						m_Pos = LastPos;
					else if(distance(NewPos, pPlayerItem->GetPos()) > D)
						m_Pos = NewPos;
					return;
				}
			}

			for(int p = 0; p < MAX_CLIENTS; p++)
			{
				CSrvCharacterCore *pCharCore = m_pWorld->m_apCharacters[p];
				if(!pCharCore || pCharCore == this)
					continue;

				if(pCharCore->m_pMap != Map())
					continue;

				float D = distance(Pos, pCharCore->m_Pos);
				if(D < 28.0f && D > 0.0f)
				{
					if(a > 0.0f)
						m_Pos = LastPos;
					else if(distance(NewPos, pCharCore->m_Pos) > D)
						m_Pos = NewPos;
					return;
				}
			}

			LastPos = Pos;
		}
	}

	m_Pos = NewPos;
}

void CSrvCharacterCore::Write(CNetObj_CharacterCore *pObjCore)
{
	pObjCore->m_X = round(m_Pos.x);
	pObjCore->m_Y = round(m_Pos.y);

	pObjCore->m_VelX = round(m_Vel.x*256.0f);
	pObjCore->m_VelY = round(m_Vel.y*256.0f);
	pObjCore->m_HookState = m_HookState;
	pObjCore->m_HookTick = m_HookTick;
	pObjCore->m_HookX = round(m_HookPos.x);
	pObjCore->m_HookY = round(m_HookPos.y);
	pObjCore->m_HookDx = round(m_HookDir.x*256.0f);
	pObjCore->m_HookDy = round(m_HookDir.y*256.0f);
	pObjCore->m_HookedPlayer = m_HookedPlayer;
	pObjCore->m_Jumped = m_Jumped;
	pObjCore->m_Direction = m_Direction;
	pObjCore->m_Angle = m_Angle;
}

void CSrvCharacterCore::Read(const CNetObj_CharacterCore *pObjCore)
{
	m_Pos.x = pObjCore->m_X;
	m_Pos.y = pObjCore->m_Y;
	m_Vel.x = pObjCore->m_VelX/256.0f;
	m_Vel.y = pObjCore->m_VelY/256.0f;
	m_HookState = pObjCore->m_HookState;
	m_HookTick = pObjCore->m_HookTick;
	m_HookPos.x = pObjCore->m_HookX;
	m_HookPos.y = pObjCore->m_HookY;
	m_HookDir.x = pObjCore->m_HookDx/256.0f;
	m_HookDir.y = pObjCore->m_HookDy/256.0f;
	m_HookedPlayer = pObjCore->m_HookedPlayer;
	m_Jumped = pObjCore->m_Jumped;
	m_Direction = pObjCore->m_Direction;
	m_Angle = pObjCore->m_Angle;
}

void CSrvCharacterCore::Quantize()
{
	CNetObj_CharacterCore Core;
	Write(&Core);
	Read(&Core);
}

void CSrvCharacterCore::PredictTick(bool UseInput, bool UseCol)  //this function simulates who the vinalla client would predict everything - no input needed
{
	float PhysSize = 28.0f;
	m_TriggeredEvents = 0;

	// get ground state
	bool Grounded = false;
	if(m_pCollision->CheckPoint(m_Pos.x+PhysSize/2, m_Pos.y+PhysSize/2+5))
		Grounded = true;
	if(m_pCollision->CheckPoint(m_Pos.x-PhysSize/2, m_Pos.y+PhysSize/2+5))
		Grounded = true;

	vec2 TargetDirection = normalize(vec2(m_Input.m_TargetX, m_Input.m_TargetY));

	m_Vel.y += m_pWorld->m_Tuning.m_Gravity;

	float MaxSpeed = Grounded ? m_pWorld->m_Tuning.m_GroundControlSpeed : m_pWorld->m_Tuning.m_AirControlSpeed;
	float Accel = Grounded ? m_pWorld->m_Tuning.m_GroundControlAccel : m_pWorld->m_Tuning.m_AirControlAccel;
	float Friction = Grounded ? m_pWorld->m_Tuning.m_GroundFriction : m_pWorld->m_Tuning.m_AirFriction;

	// add the speed modification according to players wanted direction
	if(m_Direction < 0)
		m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Vel.x, -Accel);
	if(m_Direction > 0)
		m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Vel.x, Accel);
	if(m_Direction == 0)
		m_Vel.x *= Friction;

	// handle jumping
	// 1 bit = to keep track if a jump has been made on this input
	// 2 bit = to keep track if a air-jump has been made
	if(Grounded)
		m_Jumped &= ~2;

	// do hook
	if(m_HookState == HOOK_IDLE)
	{
		m_HookedPlayer = -1;
		m_HookState = HOOK_IDLE;
		m_HookPos = m_Pos;
	}
	else if(m_HookState >= HOOK_RETRACT_START && m_HookState < HOOK_RETRACT_END)
	{
		m_HookState++;
	}
	else if(m_HookState == HOOK_RETRACT_END)
	{
		m_HookState = HOOK_RETRACTED;
		m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
		m_HookState = HOOK_RETRACTED;
	}
	else if(m_HookState == HOOK_FLYING)
	{
		vec2 NewPos = m_HookPos+m_HookDir*m_pWorld->m_Tuning.m_HookFireSpeed;
		if(distance(m_Pos, NewPos) > m_pWorld->m_Tuning.m_HookLength)
		{
			m_HookState = HOOK_RETRACT_START;
			NewPos = m_Pos + normalize(NewPos-m_Pos) * m_pWorld->m_Tuning.m_HookLength;
		}

		// make sure that the hook doesn't go though the ground
		bool GoingToHitGround = false;
		bool GoingToRetract = false;
		int Hit = m_pCollision->IntersectLine(m_HookPos, NewPos, &NewPos, 0);
		if(Hit)
		{
			if(Hit == TILE_NOHOOK)
				GoingToRetract = true;
			else
				GoingToHitGround = true;
		}

		// Check against other players first
		if(m_pWorld && m_pWorld->m_Tuning.m_PlayerHooking)
		{
			float Distance = 0.0f;
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				CSrvCharacterCore *pCharCore = m_pWorld->m_apCharacters[i];
				if(!pCharCore || pCharCore == this)
					continue;

				if(pCharCore->m_pMap != Map())
					continue;

				vec2 ClosestPoint = closest_point_on_line(m_HookPos, NewPos, pCharCore->m_Pos);
				if(distance(pCharCore->m_Pos, ClosestPoint) < PhysSize+2.0f)
				{
					if (m_HookedPlayer == -1 || distance(m_HookPos, pCharCore->m_Pos) < Distance)
					{
						m_TriggeredEvents |= COREEVENT_HOOK_ATTACH_PLAYER;
						m_HookState = HOOK_GRABBED;
						m_HookedPlayer = i;
						m_BotHooked = false;
						Distance = distance(m_HookPos, pCharCore->m_Pos);
					}
				}
			}

			CWorldSection *pSection = m_pMap->WorldSection();
			for(int i = 0; i < pSection->m_lpPlayerItems.size(); i++)
			{
				CPlayerItem *pPlayerItem = pSection->m_lpPlayerItems[i];
				if(!pPlayerItem)
					continue;

				if(pPlayerItem->GetCore() == this)
					continue;

				if(!(int)(pPlayerItem->GetInterFlags()&INTERFLAG_HOOK))
					continue;

				vec2 ClosestPoint = closest_point_on_line(m_HookPos, NewPos, pPlayerItem->GetPos());
				if(distance(pPlayerItem->GetPos(), ClosestPoint) < PhysSize+2.0f)
				{
					if (m_HookedPlayer == -1 || distance(m_HookPos, pPlayerItem->GetPos()) < Distance)
					{
						m_TriggeredEvents |= COREEVENT_HOOK_ATTACH_PLAYER;
						m_HookState = HOOK_GRABBED;
						m_HookedPlayer = i;
						m_BotHooked = true;
						Distance = distance(m_HookPos, pPlayerItem->GetPos());
					}
				}
			}
		}

		if(m_HookState == HOOK_FLYING)
		{
			// check against ground
			if(GoingToHitGround)
			{
				m_TriggeredEvents |= COREEVENT_HOOK_ATTACH_GROUND;
				m_HookState = HOOK_GRABBED;
			}
			else if(GoingToRetract)
			{
				m_TriggeredEvents |= COREEVENT_HOOK_HIT_NOHOOK;
				m_HookState = HOOK_RETRACT_START;
			}

			m_HookPos = NewPos;
		}
	}

	if(m_HookState == HOOK_GRABBED)
	{
		if(m_HookedPlayer != -1)
		{
			if(m_BotHooked)
			{
				CWorldSection *pSection = m_pMap->WorldSection();
				CPlayerItem *pSelChar = pSection->m_lpPlayerItems[m_HookedPlayer];
				if(pSelChar)
					m_HookPos = pSelChar->GetPos();
				else
				{
					// release hook
					m_HookedPlayer = -1;
					m_HookState = HOOK_RETRACTED;
					m_HookPos = m_Pos;
					m_BotHooked = false;
				}
			}
			else
			{
				CSrvCharacterCore *pCharCore = m_pWorld->m_apCharacters[m_HookedPlayer];
				if(pCharCore)
					m_HookPos = pCharCore->m_Pos;
				else
				{
					// release hook
					m_HookedPlayer = -1;
					m_HookState = HOOK_RETRACTED;
					m_HookPos = m_Pos;
					m_BotHooked = false;
				}
			}

			// keep players hooked for a max of 1.5sec
			//if(Server()->Tick() > hook_tick+(Server()->TickSpeed()*3)/2)
				//release_hooked();
		}

		// don't do this hook rutine when we are hook to a player
		if(m_HookedPlayer == -1 && distance(m_HookPos, m_Pos) > 46.0f)
		{
			vec2 HookVel = normalize(m_HookPos-m_Pos)*m_pWorld->m_Tuning.m_HookDragAccel;
			// the hook as more power to drag you up then down.
			// this makes it easier to get on top of an platform
			if(HookVel.y > 0)
				HookVel.y *= 0.3f;

			// the hook will boost it's power if the player wants to move
			// in that direction. otherwise it will dampen everything abit
			if((HookVel.x < 0 && m_Direction < 0) || (HookVel.x > 0 && m_Direction > 0))
				HookVel.x *= 0.95f;
			else
				HookVel.x *= 0.75f;

			vec2 NewVel = m_Vel+HookVel;

			// check if we are under the legal limit for the hook
			if(length(NewVel) < m_pWorld->m_Tuning.m_HookDragSpeed || length(NewVel) < length(m_Vel))
				m_Vel = NewVel; // no problem. apply

		}

		// release hook (max hook time is 1.25
		m_HookTick++;
		if(m_HookedPlayer != -1)
		{
			CWorldSection *pSection = m_pMap->WorldSection();
			if(m_HookTick > SERVER_TICK_SPEED+SERVER_TICK_SPEED/5 || (m_BotHooked?!pSection->m_lpPlayerItems[m_HookedPlayer] : !m_pWorld->m_apCharacters[m_HookedPlayer]))
			{
				m_HookedPlayer = -1;
				m_HookState = HOOK_RETRACTED;
				m_HookPos = m_Pos;
				m_BotHooked = false;
			}
		}
	}

	if(m_pWorld)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			CSrvCharacterCore *pCharCore = m_pWorld->m_apCharacters[i];
			if(!pCharCore)
				continue;

			//player *p = (player*)ent;
			if(pCharCore == this) // || !(p->flags&FLAG_ALIVE)
				continue; // make sure that we don't nudge our self

			if(pCharCore->m_pMap != Map())
				continue;

			// handle player <-> player collision
			float Distance = distance(m_Pos, pCharCore->m_Pos);
			vec2 Dir = normalize(m_Pos - pCharCore->m_Pos);
			if(m_pWorld->m_Tuning.m_PlayerCollision && Distance < PhysSize*1.25f && Distance > 0.0f && UseCol)
			{
				float a = (PhysSize*1.45f - Distance);
				float Velocity = 0.5f;

				// make sure that we don't add excess force by checking the
				// direction against the current velocity. if not zero.
				if (length(m_Vel) > 0.0001)
					Velocity = 1-(dot(normalize(m_Vel), Dir)+1)/2;

				m_Vel += Dir*a*(Velocity*0.75f);
				m_Vel *= 0.85f;
			}

			// handle hook influence
			if(m_HookedPlayer == i && m_pWorld->m_Tuning.m_PlayerHooking && m_BotHooked == false)
			{
				if(Distance > PhysSize*1.50f) // TODO: fix tweakable variable
				{
					float Accel = m_pWorld->m_Tuning.m_HookDragAccel * (Distance/m_pWorld->m_Tuning.m_HookLength);
					float DragSpeed = m_pWorld->m_Tuning.m_HookDragSpeed;

					// add force to the hooked player
					pCharCore->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.x, Accel*Dir.x*1.5f);
					pCharCore->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.y, Accel*Dir.y*1.5f);

					// add a little bit force to the guy who has the grip
					m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.x, -Accel*Dir.x*0.25f);
					m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.y, -Accel*Dir.y*0.25f);
				}
			}
		}
	}

	// clamp the velocity to something sane
	if(length(m_Vel) > 6000)
		m_Vel = normalize(m_Vel) * 6000;
}

void CSrvCharacterCore::PredictMove()  //this function simulates who the vinalla client would predict everything
{
	float RampValue = VelocityRamp(length(m_Vel)*50, m_pWorld->m_Tuning.m_VelrampStart, m_pWorld->m_Tuning.m_VelrampRange, m_pWorld->m_Tuning.m_VelrampCurvature);

	m_Vel.x = m_Vel.x*RampValue;

	vec2 NewPos = m_Pos;
	m_pCollision->MoveBoxPredict(&NewPos, &m_Vel, vec2(28.0f, 28.0f), 0);

	m_Vel.x = m_Vel.x*(1.0f/RampValue);

	if(m_pWorld && m_pWorld->m_Tuning.m_PlayerCollision)
	{
		// check player collision
		float Distance = distance(m_Pos, NewPos);
		int End = Distance+1;
		vec2 LastPos = m_Pos;

		for(int i = 0; i < End; i++)
		{
			float a = i/Distance;
			vec2 Pos = mix(m_Pos, NewPos, a);

			for(int p = 0; p < MAX_CLIENTS; p++)
			{
				CSrvCharacterCore *pCharCore = m_pWorld->m_apCharacters[p];
				if(!pCharCore || pCharCore == this)
					continue;

				if(pCharCore->m_pMap != Map())
					continue;

				float D = distance(Pos, pCharCore->m_Pos);
				if(D < 28.0f && D > 0.0f)
				{
					if(a > 0.0f)
						m_Pos = LastPos;
					else if(distance(NewPos, pCharCore->m_Pos) > D)
						m_Pos = NewPos;
					return;
				}
			}

			LastPos = Pos;
		}
	}

	m_Pos = NewPos;
}