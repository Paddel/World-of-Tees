#pragma  once

class CGameContext;

class CDamageIndicatorHandler
{
private:
	int m_DamageTaken;
	int m_DamageIndCount;

public:
	CDamageIndicatorHandler();

	void Tick(CGameContext *pGameContext, vec2 Pos, CMap *pMap);
	void OnDeath(CGameContext *pGameContext, vec2 Pos, CMap *pMap);
	void TookDamage(int Damage);
};