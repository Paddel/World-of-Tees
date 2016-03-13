
#include <game/server/gamecontext.h>
#include <engine/server/maploader.h>

#include "damageindhandler.h"

CDamageIndicatorHandler::CDamageIndicatorHandler()
{
	m_DamageTaken = 0;
	m_DamageIndCount = 0;
}

void CDamageIndicatorHandler::Tick(CGameContext *pGameContext, vec2 Pos, CMap *pMap)
{
	if(m_DamageIndCount <= 0)
		return;

	m_DamageTaken++;

	// create healthmod indicator
	// make sure that the damage indicators doesn't group together
	pGameContext->CreateDamageInd(Pos, m_DamageTaken*0.25f, 1, pMap);

	m_DamageIndCount--;
}

void CDamageIndicatorHandler::OnDeath(CGameContext *pGameContext, vec2 Pos, CMap *pMap)
{
	if(m_DamageIndCount <= 0)
		return;

	pGameContext->CreateDamageInd(Pos, m_DamageTaken*0.25f, m_DamageIndCount, pMap);
}

void CDamageIndicatorHandler::TookDamage(int Damage)
{
	if(m_DamageIndCount == 0)
		m_DamageTaken = 0;
	m_DamageIndCount += max(1, (int)(Damage/10));
}