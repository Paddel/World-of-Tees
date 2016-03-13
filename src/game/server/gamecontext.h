/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMECONTEXT_H
#define GAME_SERVER_GAMECONTEXT_H

#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/memheap.h>

#include <game/layers.h>
#include <game/voting.h>

#include <game/server/elements/chatcommands.h>
#include <game/server/elements/eventhandler.h>

#include "gamecontroller.h"
#include "gameworld.h"
#include "player.h"

/*
	Tick
		Game Context (CGameContext::tick)
			Game World (GAMEWORLD::tick)
				Reset world if requested (GAMEWORLD::reset)
				All entities in the world (ENTITY::tick)
				All entities in the world (ENTITY::tick_defered)
				Remove entities marked for deletion (GAMEWORLD::remove_entities)
			Game Controller (GAMECONTROLLER::tick)
			All players (CPlayer::tick)


	Snap
		Game Context (CGameContext::snap)
			Game World (GAMEWORLD::snap)
				All entities in the world (ENTITY::snap)
			Game Controller (GAMECONTROLLER::snap)
			Events handler (EVENT_HANDLER::snap)
			All players (CPlayer::snap)

*/
class CGameContext : public IGameServer
{
	IServer *m_pServer;
	class IConsole *m_pConsole;
	//CCollision m_Collision;
	CNetObjHandler m_NetObjHandler;
	CTuningParams m_Tuning;

	static void ConSetIngameTime(IConsole::IResult *pResult, void *pUserData);
	static void ConSetIngameTimeSpeed(IConsole::IResult *pResult, void *pUserData);

	static void ConTuneParam(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneReset(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneDump(IConsole::IResult *pResult, void *pUserData);
	static void ConPause(IConsole::IResult *pResult, void *pUserData);
	static void ConChangeMap(IConsole::IResult *pResult, void *pUserData);
	static void ConRestart(IConsole::IResult *pResult, void *pUserData);
	static void ConBroadcast(IConsole::IResult *pResult, void *pUserData);
	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeamAll(IConsole::IResult *pResult, void *pUserData);
	static void ConSwapTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConShuffleTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConLockTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	static void ConRevive(IConsole::IResult *pResult, void *pUserData);
	static void ConSpectate(IConsole::IResult *pResult, void *pUserData);
	static void ConDevPos(IConsole::IResult *pResult, void *pUserData);
	static void ConTele(IConsole::IResult *pResult, void *pUserData);
	static void ConInvTarget(IConsole::IResult *pResult, void *pUserData);
	static void ConSpawnWeapon(IConsole::IResult *pResult, void *pUserData);

	CGameContext(int Resetting);
	void Construct(int Resetting);

	bool m_Resetting;

	int m_IngameTimeSpeed;
	int m_IngameTime;

public:
	IServer *Server() const { return m_pServer; }
	class IConsole *Console() { return m_pConsole; }
	//CCollision *Collision() { return &m_Collision; }
	CTuningParams *Tuning() { return &m_Tuning; }

	CGameContext();
	~CGameContext();

	void Clear();

	void DoIngameTime();
	int GetIngameTime() const { return m_IngameTime; }

	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];

	CGameController *m_pController;
	CGameWorld m_World;

	CMap *m_pDevMap;
	vec2 m_DevPos;

	CChatCommands m_ChatCommands;
	void ChatCommand(int ChatterClientID, char *pTxt);

	//-
	void InitTile(int Index, vec2 Pos, CMap *pMap);

	// helper functions
	class CCharacter *GetPlayerChar(int ClientID);
	bool GameTranslate(int& ClientID, int TranslateID);
	int GetMapInteger(char **pSrc, char *pCall, vec2 Pos, CMap *pMap);
	char *GetMapString(char **pSrc, char *pCall, vec2 Pos, CMap *pMap);

	int m_LockTeams;

	bool TimeBetween(int HoursFrom, int MinutesFrom, int HoursTo, int MinutesTo);

	// helper functions
	void CreateDamageInd(vec2 Pos, float AngleMod, int Amount, CMap *pMap);
	void CreateExplosion(vec2 Pos, int Owner, int Weapon, int Damage, CMap *pMap, CTargetAble *pOwner);
	void CreateHammerHit(vec2 Pos, CMap *pMap);
	void CreatePlayerSpawn(vec2 Pos, CMap *pMap);
	void CreateDeath(vec2 Pos, int Who, CMap *pMap);
	void CreateSound(vec2 Pos, int Sound, CMap *pMap, int Mask=-1);
	void CreateSoundGlobal(int Sound, int Target=-1);


	enum
	{
		CHAT_ALL=-2,
		CHAT_SPEC=-1,
		CHAT_RED=0,
		CHAT_BLUE=1
	};

	// network
	void SendChatTarget(int To, const char *pText);
	void SendChat(int ClientID, int Team, const char *pText);
	void SendEmoticon(int ClientID, int Emoticon);
	void SendWeaponPickup(int ClientID, int Weapon);
	void SendBroadcast(const char *pText, int ClientID);

	int NetworkClipped(CMap *pOnMap, int SnappingClient, vec2 CheckPos);
	static void ChestDrop(void *pInfo, char *pType, int Num, char *pExtraInfo, int ClientID);
	void WantLootChest(int ClientID, int ChestID);

	void CreateWeaponString(CWeapon *pWeapon, char *pStr, int Size);
	CWeapon *CreateWeaponByString(char *pStr);

	//
	void CheckPureTuning();
	void SendTuningParams(int ClientID);
	void SendTuningParams(int ClientID, CTuningParams Tunings);

	//
	void SwapTeams();

	void SetDefaultTunings();

	// engine events
	virtual void OnInit();
	virtual void OnConsoleInit();
	virtual void OnShutdown();

	virtual void OnTick();
	virtual void OnPreSnap();
	virtual void OnSnap(int ClientID);
	virtual void OnPostSnap();

	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID);

	virtual void OnClientConnected(int ClientID);
	virtual void OnClientEnter(int ClientID);
	virtual void OnClientDrop(int ClientID, const char *pReason);
	virtual void OnClientDirectInput(int ClientID, void *pInput);
	virtual void OnClientPredictedInput(int ClientID, void *pInput);

	virtual bool IsClientReady(int ClientID);
	virtual bool IsClientPlayer(int ClientID);

	virtual const char *GameType();
	virtual const char *Version();
	virtual const char *NetVersion();

	void InitExTile(int Index, vec2 Pos, CMap *pMap, char *pArgs);
	virtual void InitMapTiles(CMap *pMap);

	void RenderChests(CChest *pFirst, int Num, int Size, int ClientID);
};

inline int CmaskAll() { return -1; }
inline int CmaskOne(int ClientID) { return 1<<ClientID; }
inline int CmaskAllExceptOne(int ClientID) { return 0x7fffffff^CmaskOne(ClientID); }
inline bool CmaskIsSet(int Mask, int ClientID) { return (Mask&CmaskOne(ClientID)) != 0; }
#endif
