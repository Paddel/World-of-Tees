#pragma once

#include <base/vmath.h>
#include <game/server/playeritem.h>
#include <game/server/targetable.h>
#include <game/server/elements/damageindhandler.h>
#include <game/server/weapon.h>

class CGameContext;
class IServer;
class CNpcSpawner;
class CMap;
class CNpcHolder;
class CNpc;

//static const char *s_pSkinNames[] = {
//	"bluekitty",
//	"bluestripe",
//	"brownbear",
//	"cammo",
//	"cammostripes",
//	"coala",
//	"limekitty",
//	"pinky",
//	"redbopp",
//	"redstripe",
//	"saddo",
//	"toptri",
//	"twinbop",
//	"twintri",
//	"warpaint"
//};

enum
{
	NPC_BEGGAR = 0,
	NPC_GUARD,
	NPC_HELPER,
	NPC_PUZZLETEE,
	NPC_AMMOSHOP,

	NPC_BANDIT,
	NPC_MUMMY,
	NPC_NOMAD,
	NPC_PHARAO,
	NPC_SHADOW,
	NPC_SKELETON,
	NPC_WILDTEE,
	NPC_ZOMBIE,
	NPC_SKELETON_SMALL,
	NPC_WILDTEE_SMALL,
	NPC_ZOMBIE_SMALL,

	NUM_NPCS,
};

static const char *s_aNpcNames[NUM_NPCS] = {
	"beggar",
	"guard",
	"helper",
	"helper",

	"bandit",
	"mummy",
	"nomad",
	"pharao",
	"shadow",
	"skeleton",
	"wildtee",
	"zombie",
};

static int GetNpcType(const char *pName)
{
	for(int i = 0; i < NUM_NPCS; i++)
	{
		if(str_comp_nocase(s_aNpcNames[i], pName) == 0)
			return i;
	}
	return -1;
};

CNpc *CreateNpc(int Type, CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHoler, vec2 Pos);

class CNpc : public CPlayerItem
{
protected:
	IServer *m_pServer;
	CMap *m_pMap;
	CNpcHolder *m_pNpcHolder;
	CSrvCharacterCore m_Core;
	bool m_Temporary;
	int m_Owner;
	int m_Type;

	int m_EmoteType;
	int m_EmoteStop;

	IServer *Server() const {return m_pServer;}
	CMap *Map() { return m_pMap; }
	
	CTargetAble *FindClosestTarget(float Radius, CTargetAble *pAskinTarget);

public:
	CNpc(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos, int InterFlags, int Type);

	void Spawn();
	void OnNpcDeath();
	void DoEmoticon(int Emoticon);
	void SetEmote(int Emote, int Tick);

	bool IsGrounded();

	int GetType() const { return m_Type; }
	bool GetTemporary() const { return m_Temporary; }
	void SetTemporary(bool Temp) { m_Temporary = Temp; }
	int GetOwner() const { return m_Owner; }
	void SetOwner(int Owner) { m_Owner = Owner; }

	virtual void Tick() = 0;
	virtual void Snap(int SnappingClient) = 0;

	virtual bool Alive() = 0;

	virtual void SubSpawn() {};
	virtual void OnDeath(int From, int Weapon, vec2 DeathVel) {};

	virtual void OnNotActive() {}
	virtual CMap *GetMap() { return Map(); }
	virtual CSrvCharacterCore *GetCore() { return &m_Core; }
};

class CEnemyNpc : public CNpc, public CTargetAble
{
private:

protected:
	CStatusEffect m_aStatusEffects[NUM_STATUSEFFECT];
	CDamageIndicatorHandler m_DamageIndHandler;
	CTargetAble *m_pTarget;
	CWeapon *m_pWeapon;
	int64 m_HealthRegTime;
	int m_Health;
	int m_AttackTick;

	bool CheckImpasse(vec2 CurPos, vec2 ColPos);
	void CheckHealthRegeneration();

	int AiFireDistance();
	int AiMoveDistance();
	int AiMoveTolerance();
	vec2 GetFireDirection(vec2 OwnPos, vec2 EnemyPos);

	enum
	{
		TIME_NOTAR_CHANGE_MOVEDIR=0,
		TIME_NOTAR_JUMP,
		TIME_NOTAR_NEW_EYE_DIR,
		TIME_TAR_COL_CHECK,
		TIME_TAR_COL_LOOSE,
		TIME_TAR_ATTACK,

		NUM_TIMES,
	};

	int64 m_aTimes[NUM_TIMES];
	bool m_RandJump;


public:
	CEnemyNpc(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos, int Type);

	void SetMoveDirection(int Direction, float Time);
	bool RandomDrop(float Chance);

	virtual void Tick();
	virtual void Snap(int SnappingClient);

	virtual void SetInput();
	virtual void NoTargetInput();
	virtual void TargetInput();

	virtual void SubSpawn();
	virtual void OnDeath(int From, int Weapon, vec2 DeathVel);
	virtual bool Alive() { return m_Health > 0; }

	virtual void OnNotActive();

	//targetable
	virtual void ResetEnemy(CTargetAble *pTarget);
	virtual vec2 GetClosestPos(vec2 Pos) { return GetPos(); }
	virtual vec2 GetIntersectPos(vec2 Pos0, vec2 Pos1) { return closest_point_on_line(Pos0, Pos1, GetPos()); }
	virtual bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon);
	virtual int GetProximityRadius() { return 28; }
	virtual bool IsEnemy(CTargetAble *pTarget);
	virtual bool IsBuilding() { return false; }
	virtual int MaxHealth() = 0;
	//virtual void Slow(float t, float SlowRate) { m_Core.Slow(t, SlowRate); }

	virtual CWeapon *CreateWeapon() = 0;
	virtual char *NpcName() = 0;
	virtual int MoneyAmount() = 0;
	virtual int MoneyAddAmount() = 0;
	virtual int BaseDamage() = 0;
	virtual int Experience() = 0;
	virtual int AttackSpeed() = 0;
	virtual float WeaponDropChance() = 0;
	
	//skin
	virtual char *SkinName() = 0;
	virtual int SkinCostumColor() = 0;
	virtual int SkinColorBody() = 0;
	virtual int SkinColorFeet() = 0;

	virtual bool UseHook() = 0;
	virtual float Speed() { return 10; }

	//sub functions
	virtual void SubTick() {}
	virtual void SubResetTarget() {}
	virtual void SubOnSpawn() {}
	virtual void SubOnDeath(int From, int Weapon, vec2 DeathVel) {}

	//overwriteable functions
	virtual void SetExtraCollision();
};

class CNpcHolder
{
protected:
	array<CNpc *> m_pNpcs;

public:

	void KillTemporary();
	int GetTemporaryNum();
	int GetTempPlayerSpawnNum(int ClientID);
	int GetNum();

	void OnNewNpc(CNpc *pNpc);
	void OnRemoveNpc(CNpc *pNpc);

	virtual bool Camp() = 0;
	virtual vec2 CampPos() = 0;
};