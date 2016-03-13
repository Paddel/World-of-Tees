
#include <game/server/gameworld.h>
#include <game/server/gamecontext.h>

#include "targetable.h"

CTargetAble::CTargetAble(CGameWorld *pWorld, int Type, CMap *pMap)
{
	m_pWorld = pWorld;
	m_pServer = pWorld->Server();
	m_Type = Type;
	m_pTargetMap = pMap;
	m_pStatusEffect = NULL;
	m_ShownStatusEffectTime = 0;
	m_FireDamageTime = 0;

	AddTargetAble();
}

void CTargetAble::AddTargetAble()
{
	CWorldSection *pSection = m_pTargetMap->WorldSection();
	for(int i = 0; i < pSection->m_lpTargetAbles.size(); i++)
	{
		if(pSection->m_lpTargetAbles[i] == this)
			return;
	}

	m_pWorld->AddTargetAble(this, m_pTargetMap);
}

void CTargetAble::ResetEnemies()
{
	CWorldSection *pSection = m_pTargetMap->WorldSection();
	for(int i = 0; i < pSection->m_lpTargetAbles.size(); i++)
	{
		CTargetAble *pTarget = pSection->m_lpTargetAbles[i];

		if(pTarget == this)
			continue;

		pTarget->ResetEnemy(this);
	}
}

void CTargetAble::OnTargetDeath()
{
	ResetEnemies();
	m_pWorld->RemoveTargetAble(this, m_pTargetMap);
}

float CTargetAble::GetSlow()
{
	float Slow = 1.0f;
	if(m_pStatusEffect)
	{
		if(m_pStatusEffect[STATUSEFFECT_ICE].m_EndTime > m_pServer->Tick())
		{
			Slow *= 0.6f;
		}
	}

	return Slow;
}

bool CTargetAble::GetStun()
{
	return m_pStatusEffect && m_pStatusEffect[STATUSEFFECT_STUN].m_EndTime > m_pServer->Tick();
}

void CTargetAble::SetStatusEffect(CStatusEffect *pStatusEffect)
{
	m_pStatusEffect = pStatusEffect;
}

void CTargetAble::HandleStatusEffects()
{
	if(m_pStatusEffect == NULL)
		return;

	if(m_pStatusEffect[STATUSEFFECT_FIRE].m_EndTime > m_pServer->Tick())
	{
		if(m_FireDamageTime < m_pServer->Tick())
		{
			int Damage = MaxHealth() * 0.05f;
			TakeDamage(vec2(0, 0), Damage, m_pStatusEffect[STATUSEFFECT_FIRE].m_From, WEAPON_WORLD);
			m_FireDamageTime = m_pServer->Tick() + m_pServer->TickSpeed();
		}
	}

	

	//if there is more than 1 statuseffect, rotate between them
	if(m_ShownStatusEffectTime < m_pServer->Tick())
	{
		int NextStatusEffect = m_ShownStatusEffect+1;
		m_ShownStatusEffect = -1;
		for(int i = 0; i < NUM_STATUSEFFECT; i++)
		{
			int Check = (NextStatusEffect+i)%NUM_STATUSEFFECT;
			if(m_pStatusEffect[Check].m_EndTime > m_pServer->Tick())
			{
				m_ShownStatusEffect = Check;
				m_ShownStatusEffectTime = m_pServer->Tick() + m_pServer->TickSpeed()*0.3f;
				break;
			}
		}
	}
}

bool CTargetAble::StatusEffectSkin(int *pColorBody, int *pColorFeet)
{
	if(m_pStatusEffect == NULL)
		return false;

	for(int i = 0; i < NUM_STATUSEFFECT; i++)
	{
		if(m_pStatusEffect[i].m_EndTime > m_pServer->Tick() && m_ShownStatusEffect == i)
		{
			*pColorBody = s_aStatusEffectColors[i];
			*pColorFeet = s_aStatusEffectColors[i];
			return true;
		}
	}

	return false;
}

void CTargetAble::NewStatusEffect(int StatusEffect, int From)
{
	if(m_pStatusEffect == NULL || StatusEffect < 0 || StatusEffect >= NUM_STATUSEFFECT)
		return;

	m_pStatusEffect[StatusEffect].m_From = From;
	m_pStatusEffect[StatusEffect].m_EndTime = m_pServer->Tick() + m_pServer->TickSpeed()*5.0f;
}