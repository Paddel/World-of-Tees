
#include <engine/server.h>
#include <game/balancing.h>
#include <game/server/gamecontext.h>
#include <game/server/weapon.h>
#include <game/server/elements/questionhandler.h>

#include "broadcast.h"

static char ExpTopLeft[] = { -30, -107, -108};
static char ExpTopMidFull[] = { -30, -107, -90};
static char ExpTopMidEmpty[] = { -30, -107, -112};
static char ExpTopRight[] = { -30, -107, -105};

static char ExpBotLeft[] = { -30, -107, -102};
static char ExpBotMidFull[] = { -30, -107, -87};
static char ExpBotMidEmpty[] = { -30, -107, -112};
static char ExpBotRight[] = { -30, -107, -99};

static char MoneyChar[] = {-30, -120, -121 };

CBroadcast::CBroadcast(CGameContext *pGameServer, int Owner)
{
	m_pGameServer = pGameServer;
	m_pServer = pGameServer->Server();
	m_Owner = Owner;

	m_BroadcastTime = 0;
	mem_zero(&m_aLastBroadString, sizeof(m_aLastBroadString));
}

void CBroadcast::Send(int ClientID)
{
	CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
	char aBroadString[MAX_BROADCAST_STRING];
	mem_zero(&aBroadString, sizeof(aBroadString));

	if(pChr && pChr->HasHealth() == false)
		DoBrodcastStringDead(aBroadString, ClientID);
	else
		DoBroadcastStringIngame(aBroadString, ClientID);

	if(aBroadString[0] && (m_BroadcastTime < Server()->Tick() || str_comp(m_aLastBroadString, aBroadString) != 0))
	{
		GameServer()->SendBroadcast(aBroadString, m_Owner);
		m_BroadcastTime = Server()->Tick()+Server()->TickSpeed()*4;
		str_copy(m_aLastBroadString, aBroadString, sizeof(m_aLastBroadString));
	}
}

void CBroadcast::DoBrodcastStringDead(char *pStr, int ClientID)
{
	char aDeadStr[64];
	CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
	if(!pChr)
		return;

	if(pChr->GetRemoveCorpseTime() > Server()->Tick())
	{
		int RemainingTime = (pChr->GetRemoveCorpseTime()-Server()->Tick())/Server()->TickSpeed()+1;
		str_format(aDeadStr, sizeof(aDeadStr), "Wait %i seconds to restart.", RemainingTime);
	}
	else
		str_format(aDeadStr, sizeof(aDeadStr), "Fire to restart.");

	BroadAddStr(pStr, aDeadStr);
}

void CBroadcast::DoBroadcastStringIngame(char *pStr, int ClientID)
{
	BroadAddQuestionSkip(pStr, ClientID);
	BroadAddExpBar(pStr, ClientID);
	BroadAddHealth(pStr, ClientID);
	BroadAddMana(pStr, ClientID);
	BroadAddAmmo(pStr, ClientID);
	BroadAddExp(pStr, ClientID);
	BroadAddMoney(pStr, ClientID);
	BroadAddSkillPoints(pStr, ClientID);
	BroadAddBreeth(pStr, ClientID);
	BroadAddEnterMap(pStr, ClientID);
	BroadAddChest(pStr, ClientID);

	BroadAddShift(pStr, ClientID);
}

void CBroadcast::BroadAddQuestionSkip(char *pStr, int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return;

	if(pPlayer->QuestionHandler()->GetQuestionNum() > 0)
	{
		for(int i = 0; i < 5; i++)
			BroadAddStr(pStr, " ");
	}
}

void CBroadcast::BroadAddExpBar(char *pStr, int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return;
	int MaxNum = 15;
	float a = (float)pPlayer->AccountInfo()->m_Experience/Character_NeededExperience(pPlayer->AccountInfo()->m_Level);
	int num = a*MaxNum;
	char aBarTop[64];
	char aBarBot[64];

	//top
	str_format(aBarTop, sizeof(aBarTop), "%s", ExpTopLeft);
	for(int i = 0; i < num; i++)
		str_format(aBarTop, sizeof(aBarTop), "%s%s", aBarTop, ExpTopMidFull);
	for(int i = 0; i < MaxNum-num; i++)
		str_format(aBarTop, sizeof(aBarTop), "%s%s", aBarTop, ExpTopMidEmpty);
	str_format(aBarTop, sizeof(aBarTop), "%s%s", aBarTop, ExpTopRight);

	//bot
	str_format(aBarBot, sizeof(aBarBot), "%s", ExpBotLeft);
	for(int i = 0; i < num; i++)
		str_format(aBarBot, sizeof(aBarBot), "%s%s", aBarBot, ExpBotMidFull);
	for(int i = 0; i < MaxNum-num; i++)
		str_format(aBarBot, sizeof(aBarBot), "%s%s", aBarBot, ExpBotMidEmpty);
	str_format(aBarBot, sizeof(aBarBot), "%s%s", aBarBot, ExpBotRight);

	BroadAddStr(pStr, aBarTop);
	BroadAddStr(pStr, aBarBot);
}

void CBroadcast::BroadAddHealth(char *pStr, int ClientID)
{
	CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
	char aHealthStr[32];
	if(!pChr)
		return;

	str_format(aHealthStr, sizeof(aHealthStr), "HP: %i/%i", pChr->GetHealth(), Character_MaxHealth(pChr->GetPlayer()->AccountInfo()->m_Level));
	BroadAddStr(pStr, aHealthStr);
}

void CBroadcast::BroadAddMana(char *pStr, int ClientID)
{
	CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
	char aManaStr[32];
	if(!pChr || Character_MaxMana(pChr->GetPlayer()->AccountInfo()->m_Level) <= 0)
		return;

	str_format(aManaStr, sizeof(aManaStr), "Mana: %i/%i", pChr->PlayerInfo()->m_Mana, Character_MaxMana(pChr->GetPlayer()->AccountInfo()->m_Level));
	BroadAddStr(pStr, aManaStr);
}

void CBroadcast::BroadAddMoney(char *pStr, int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	char aMoneyStr[32];
	if(!pPlayer)
		return;

	str_format(aMoneyStr, sizeof(aMoneyStr), "Money: %i%s", pPlayer->AccountInfo()->m_Money, MoneyChar);
	BroadAddStr(pStr, aMoneyStr);
}

void CBroadcast::BroadAddAmmo(char *pStr, int ClientID)
{
	CWeapon *pWeapon = NULL;
	CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
	char aAmmoStr[32];
	if(!pChr)
		return;

	pWeapon = pChr->PlayerInfo()->m_pWeapon[pChr->ActiveWeapon()];

	if(pWeapon == false || pWeapon->MaxAmmo() <= 0)
		return;

	str_format(aAmmoStr, sizeof(aAmmoStr), "Ammo: %i/%i", pWeapon->Ammo(), pWeapon->MaxAmmo());
	BroadAddStr(pStr, aAmmoStr);
}

void CBroadcast::BroadAddExp(char *pStr, int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	char aExpStr[32];
	if(!pPlayer)
		return;

	str_format(aExpStr, sizeof(aExpStr), "Exp: %i/%i", pPlayer->AccountInfo()->m_Experience, Character_NeededExperience(pPlayer->AccountInfo()->m_Level));
	BroadAddStr(pStr, aExpStr);
}

void CBroadcast::BroadAddSkillPoints(char *pStr, int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	char aSPStr[32];
	if(!pPlayer || pPlayer->AccountInfo()->m_SkillPoints <= 0)
		return;

	str_format(aSPStr, sizeof(aSPStr), "SP: %i", pPlayer->AccountInfo()->m_SkillPoints);
	BroadAddStr(pStr, aSPStr);
}

void CBroadcast::BroadAddBreeth(char *pStr, int ClientID)
{
	CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
	char aBreethStr[32];
	mem_zero(&aBreethStr, sizeof(aBreethStr));
	if(!pChr || !pChr->Core()->m_InWater)
		return;

	for(int i = 0; i < pChr->GetBreeth(); i++)
		str_format(aBreethStr, sizeof(aBreethStr), "%s%s", aBreethStr[0]?aBreethStr:"", "o");
	for(int i = 0; i < MAX_BREETH-pChr->GetBreeth(); i++)
		str_format(aBreethStr, sizeof(aBreethStr), "%s%s", aBreethStr[0]?aBreethStr:"", "x");

	BroadAddStr(pStr, aBreethStr);
}

void CBroadcast::BroadAddEnterMap(char *pStr, int ClientID)
{
	CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
	if(!pChr || pChr->OnHammerTransition() == false)
		return;

	BroadAddStr(pStr, "Hammer to enter");
}

void CBroadcast::BroadAddChest(char *pStr, int ClientID)
{
	CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
	if (!pChr || pChr->OnChest() == false)
		return;

	BroadAddStr(pStr, "Hammer to open");
}

void CBroadcast::BroadAddShift(char *pStr, int ClientID)
{
	BroadAddStr(pStr, \
		"                                                                                                                                                                               "\
		);
}

void CBroadcast::BroadAddStr(char *pStr, char *pAdd)
{
	str_format(pStr, MAX_BROADCAST_STRING, "%s%s%s", pStr[0]?pStr:"", pStr[0]?"\n":"", pAdd);
}