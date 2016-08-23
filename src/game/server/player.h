/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

// this include should perhaps be removed   //Paddel says no :c 
#include "entities/character.h"
#include "gamecontext.h"
#include "playeritem.h"
#include "playerinfo.h"

class CQuestionHandler;
class CBroadcast;

// player object
class CPlayer : public CPlayerItem
{
	MACRO_ALLOC_POOL_ID()

public:
	CPlayer(CGameContext *pGameServer, int ClientID, int Team);
	~CPlayer();

	void Init(int CID);

	void TryRespawn();
	void SetTeam(int Team, bool DoChatMsg=true);
	int GetTeam() const { return m_Team; };
	int GetCID() const { return m_ClientID; };

	virtual void Tick();
	virtual void PostTick();
	virtual void Snap(int SnappingClient);
	virtual CMap *GetMap() { return m_pMap; }
	virtual bool Spectating() { return m_Team == TEAM_SPECTATORS; }
	virtual CSrvCharacterCore *GetCore() { return &m_Core; }

	void OnDirectInput(CNetObj_PlayerInput *NewInput);
	void OnPredictedInput(CNetObj_PlayerInput *NewInput);
	void OnDisconnect(const char *pReason);

	void KillCharacter(int Weapon = WEAPON_GAME);
	void OnCharacterDead();
	CCharacter *GetCharacter();

	void WantTeleportHome();

	void LoadWeaponString();
	void SetBeginingValues();
	bool FillSavingInfos(bool IllegalLogout);
	bool Register(char *pFailMsg, int Size, char *pName, char *pPassword);
	bool Login(char *pFailMsg, int Size, char *pName, char *pPassword);
	bool Save();
	bool Logout();

	bool EarnMoney(int Money);
	void GainExp(int Amount);
	void LevelUp();
	void Heal(float percent);

	void Spectate(int SpectatingID);

	CMap *GetHomeMap();

	CMap *m_pWantedMap;

	//---------------------------------------------------------
	// this is used for snapping so we know how we can clip the view for the player
	vec2 m_ViewPos;

	// states if the client is chatting, accessing a menu etc.
	int m_PlayerFlags;

	// used for snapping to just update latency if the scoreboard is active
	int m_aActLatency[MAX_CLIENTS];

	bool m_IsReady;

	//
	virtual vec2 GetPos() { return m_ViewPos; }

	//
	int m_Vote;
	int m_VotePos;
	//
	int m_LastVoteCall;
	int m_LastVoteTry;
	int m_LastChat;
	int m_LastSetTeam;
	int m_LastSetSpectatorMode;
	int m_LastChangeInfo;
	int m_LastEmote;
	int m_LastKill;

	// TODO: clean this up
	struct
	{
		char m_SkinName[64];
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;
	} m_TeeInfos;

	int m_RespawnTick;//remove dis
	int m_DieTick;
	int m_Score;
	int m_ScoreStartTick;
	bool m_ForceBalanced;
	int m_LastActionTick;
	int m_TeamChangeTick;
	struct
	{
		int m_TargetX;
		int m_TargetY;
	} m_LatestActivity;

	// network latency calculations
	struct
	{
		int m_Accum;
		int m_AccumMin;
		int m_AccumMax;
		int m_Avg;
		int m_Min;
		int m_Max;
	} m_Latency;

	CAccountInfo *AccountInfo() const { return m_pAccountInfo; }
	CPlayerInfo *PlayerInfo() const { return m_pPlayerInfo; }
	CTextPopup *TextPopup() const { return m_pTextPopup; }
	CQuestionHandler *QuestionHandler() const { return m_pQuestionHandler; }
	CBroadcast *Broadcast() const { return m_pBroadcast; }

private:
	CCharacter *m_pCharacter;
	CGameContext *m_pGameServer;

	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const;

	bool m_WantTeleportHome;
	void TeleportHome();

	//
	bool m_Spawning;
	int m_ClientID;
	int m_Team;

	CMap *m_pMap;
	CAccountInfo *m_pAccountInfo;
	CPlayerInfo *m_pPlayerInfo;
	CTextPopup *m_pTextPopup;
	CQuestionHandler *m_pQuestionHandler;
	CBroadcast *m_pBroadcast;

	CSrvCharacterCore m_Core;
};

#endif
