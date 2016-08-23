/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_CHARACTER_H
#define GAME_SERVER_ENTITIES_CHARACTER_H

#include <game/server/entity.h>
#include <game/generated/server_data.h>
#include <game/generated/protocol.h>
#include <game/server/targetable.h>
#include <game/server/elements/damageindhandler.h>

#include <game/gamecore.h>

typedef void (*pClientFunc)(void *pInfo, int ClientID); 

class CWeapon;

enum
{
	WEAPON_GAME = -3, // team switching etc
	WEAPON_SELF = -2, // console kill command
	WEAPON_WORLD = -1, // death tiles etc
};

class CCharacter : public CEntity, public CTargetAble
{
	MACRO_ALLOC_POOL_ID()

public:
	//character's size
	static const int ms_PhysSize = 28;

	CCharacter(CGameWorld *pWorld, CMap *pMap);

	virtual void Reset();
	virtual void Destroy();
	virtual void Tick();
	virtual void TickDefered();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	bool IsGrounded();
	bool OnWormhole();
	bool OnGenerateMap(int ID, int TransitionID, int HammerNeeded);

	void SetWeapon(int W);
	void HandleWeaponSwitch();
	void DoWeaponSwitch();

	int GetHealth() const { return *m_pHealth; }
	bool OnHammerTransition() const { return m_OnHammerTransition; }
	bool OnChest() const { return m_OnChest; }
	int GetBreeth() const { return m_Breeth; }

	void SetHealth(int Health);
	bool HasHealth();
	bool Revive();

	int64 GetRemoveCorpseTime() const { return m_RemoveCorpseTime; }

	void HandleWeapons();

	void OnPredictedInput(CNetObj_PlayerInput *pNewInput);
	void OnDirectInput(CNetObj_PlayerInput *pNewInput);
	void ResetInput();
	void FireWeapon();

	void Die(int Killer, int Weapon);

	bool Spawn(class CPlayer *pPlayer, vec2 Pos);

	bool IncreaseHealth(int Amount);
	bool IncreaseMana(int Amount);

	static void OnReplaceWeapon(void *pInfo, int ClientID);
	static void OnNotReplaceWeapon(void *pInfo, int ClientID);
	void ReplaceWeapon(int Index, CWeapon *pNew);
	bool PickupWeapon(class CWeaponPickup *pWeaponPickup);
	bool GiveWeapon(CWeapon *pWeapon);
	bool DropWeapon(int Weapon);
	void EmptyWeaponCheck();

	void SetEmote(int Emote, int Tick);

	bool IsAlive() const { return m_Alive; }
	class CPlayer *GetPlayer() { return m_pPlayer; }

	bool Touching(int Tile);
	void TileCheck();
	void ExTileCheck();

	void SetExtraCollision();

	void WantBackport();
	
	CWeapon *HasWeapon(int Type);
	CWeapon *CurrentWeapon();

	void SetEmoteType(int EmoteType) { m_EmoteType = EmoteType; };
	void SetEmoteStop(int EmoteStop) { m_EmoteStop = EmoteStop; };

	vec2 MousePos();
	class CPlayerInfo *PlayerInfo();

	CSrvCharacterCore *Core() { return m_pCore; }
	int ActiveWeapon() const { return m_ActiveWeapon; }

	//targetable
	virtual void ResetEnemy() {}
	virtual CMap *GetMap() { return Map(); }
	virtual vec2 GetClosestPos(vec2 Pos) { return m_Pos; }
	virtual vec2 GetIntersectPos(vec2 Pos0, vec2 Pos1) { return closest_point_on_line(Pos0, Pos1, m_Pos); }
	virtual bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon);
	virtual int GetProximityRadius() { return ms_PhysSize; }
	virtual bool IsEnemy(CTargetAble *pTarget);
	virtual bool IsBuilding() { return false; }
	virtual int MaxHealth();

private:
	// player controlling this character
	class CPlayer *m_pPlayer;

	CDamageIndicatorHandler m_DamageIndHandler;
	CTuningParams m_ShownTuning;
	CTuningParams m_WantedShownTuning;

	bool m_Alive;

	void RpgTick();

	int m_ActiveWeapon;
	int m_LastWeapon;
	int m_QueuedWeapon;

	int m_ReloadTimer;
	int m_AttackTick;
	int m_ShownAttackTick;

	int m_EmoteType;
	int m_EmoteStop;

	// last tick that the player took any action ie some input
	int m_LastAction;
	int m_LastNoAmmoSound;

	// these are non-heldback inputs
	CNetObj_PlayerInput m_LatestPrevInput;
	CNetObj_PlayerInput m_LatestInput;

	// input
	CNetObj_PlayerInput m_PrevInput;
	CNetObj_PlayerInput m_Input;
	int m_NumInputs;
	int m_Jumped;

	int m_DamageTakenTick;

	int *m_pHealth;

	int m_Breeth;
	int64 m_BreethTime;

	// ninja
	struct
	{
		vec2 m_ActivationDir;
		int m_ActivationTick;
		int m_CurrentMoveTime;
		int m_OldVelAmount;
	} m_Ninja;

	// the player core for the physics
	CSrvCharacterCore *m_pCore;

	// info for dead reckoning
	int m_ReckoningTick; // tick that we are performing dead reckoning From
	CSrvCharacterCore m_SendCore; // core that we should send
	CSrvCharacterCore m_ReckoningCore; // the dead reckoning core

	bool InBox();

	bool m_OnHammerTransition;
	bool m_OnChest;

	bool InputFrozen(int Input);
	enum
	{
		INPUT_FIRE=0,
		INPUT_MOVE,
		INPUT_JUMP,
		INPUT_HOOK,
		INPUT_WEAPONCHANGE,
	};

	void RemoveCorpse();
	int64 m_RemoveCorpseTime;

	int m_LastTile;
	int m_LastExTile;

	int64 m_RegenHealthTime;
	int64 m_RegenManaTime;
	float m_ManaRatio;
	void DoRegeneration();

	int64 m_BackportTime;

	bool m_OnHealTile;
};

#endif
