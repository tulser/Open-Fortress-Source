//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: The TF Game rules object
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef TF_GAMERULES_H
#define TF_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif


#include "teamplayroundbased_gamerules.h"
#include "convar.h"
#include "gamevars_shared.h"
#include "GameEventListener.h"
#include "tf_gamestats_shared.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#include "trains.h"
#include "team_train_watcher.h"
#endif

#ifdef CLIENT_DLL
	
	#define CTFGameRules C_TFGameRules
	#define CTFGameRulesProxy C_TFGameRulesProxy
#else

	extern BOOL no_cease_fire_text;
	extern BOOL cease_fire;

	class CHealthKit;
	class CAmmoPack;
	class CWeaponSpawner;

#endif

extern ConVar	tf_avoidteammates;

// Mutator enums
enum
{
	NO_MUTATOR = 0,
	INSTAGIB = 1,
	INSTAGIB_NO_MELEE = 2,
	CLAN_ARENA = 3,
	UNHOLY_TRINITY = 4,
	ROCKET_ARENA = 5,
	GUN_GAME = 6,
};

class CTFGameRulesProxy : public CTeamplayRoundBasedRulesProxy
{
public:
	DECLARE_CLASS( CTFGameRulesProxy, CTeamplayRoundBasedRulesProxy );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
	void	InputSetRedTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputSetBlueTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputSetMercenaryTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputAddRedTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputAddBlueTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputAddMercenaryTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputSetRedTeamGoalString( inputdata_t &inputdata );
	void	InputSetBlueTeamGoalString( inputdata_t &inputdata );
	void	InputSetMercenaryTeamGoalString( inputdata_t &inputdata );
	void	InputSetRedTeamRole( inputdata_t &inputdata );
	void	InputSetBlueTeamRole( inputdata_t &inputdata );
	void	InputSetMercenaryTeamRole( inputdata_t &inputdata );
	void	InputSetRedKothClockActive( inputdata_t &inputdata) ;
	void	InputSetBlueKothClockActive( inputdata_t &inputdata );
	void	InputPlayVORed( inputdata_t &inputdata );
	void	InputPlayVOBlue( inputdata_t &inputdata );
	void	InputPlayVOMercenary( inputdata_t &inputdata );
	void	InputPlayVO( inputdata_t &inputdata );

	virtual void Activate();
	
	bool m_bUsesHL2Hull;
	bool m_bForce3DSkybox;
	bool m_bUsesMoney;
	
	COutputEvent m_OutputIsCTF;
	COutputEvent m_OutputIsCP;
	COutputEvent m_OutputIsDM;
	COutputEvent m_OutputIsTeamplay;
	COutputEvent m_OutputIsGunGame;
	
	void	FireCTFOutput(void) {m_OutputIsCTF.FireOutput(NULL,this);}
	void	FireCPOutput(void) {m_OutputIsCP.FireOutput(NULL,this);}
	void	FireDMOutput(void) {m_OutputIsDM.FireOutput(NULL,this);}
	void	FireTeamplayOutput(void) {m_OutputIsTeamplay.FireOutput(NULL,this);}
	void	FireGunGameOutput(void) {m_OutputIsGunGame.FireOutput(NULL,this);}
#endif
};

class CTFLogicDM : public CBaseEntity
{
public:
	DECLARE_CLASS(CTFLogicDM, CBaseEntity);
	virtual void	Spawn( void );
	
#ifdef GAME_DLL
	DECLARE_DATADESC();
	bool m_bIsTeamplay;
	bool m_bDontCountKills;
#endif
};

class CTFLogicGG : public CBaseEntity
{
public:
	DECLARE_CLASS(CTFLogicGG, CBaseEntity);
	CTFLogicGG();

	virtual void	Spawn(void);

#ifdef GAME_DLL
	DECLARE_DATADESC();
	string_t	m_iszWeaponName[50];
	bool		m_bListOnly;
	int			m_iMaxLevel = 0;
	int			m_iRequiredKills = 2;
#endif
};

class CTFLogicESC : public CBaseEntity
{
public:
	DECLARE_CLASS(CTFLogicESC,	CBaseEntity);
	virtual void	Spawn(void);

#ifdef GAME_DLL
	DECLARE_DATADESC();
	int m_nMaxHunted_red;
	int m_nMaxHunted_blu;
	int m_nHuntedScoreAdd_red;
	int m_nHuntedScoreAdd_blue;

	COutputEvent m_OnHuntedDeath;
#endif
};

class CTFLogicDOM : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFLogicDOM,	CBaseEntity );

	virtual void	Spawn (void );

#ifdef GAME_DLL
	DECLARE_DATADESC();

	void	DOMThink(void);

	int m_nDomScore_limit;
	int m_nDomScore_time;
	bool m_bDomScoreOnKill;
	bool m_bDomWinOnLimit;

	void	AddDomScore_red( int amount );
	void	AddDomScore_blue( int amount );
	void	SetDomScore_red( int amount );
	void	SetDomScore_blue( int amount );
	void	SetDomScoreLimit( int amount );

	void	InputAddDomScore_red( inputdata_t &inputdata );
	void	InputAddDomScore_blue( inputdata_t &inputdata );
	void	InputSetDomScore_red( inputdata_t &inputdata );
	void	InputSetDomScore_blue( inputdata_t &inputdata );
	void	InputSetDomScoreLimit( inputdata_t &inputdata );

	COutputEvent m_OnDomScoreHit_any;
	COutputEvent m_OnDomScoreHit_red;
	COutputEvent m_OnDomScoreHit_blue;
#endif
};

struct PlayerRoundScore_t
{
	int iPlayerIndex;	// player index
	int iRoundScore;	// how many points scored this round
	int	iTotalScore;	// total points scored across all rounds
};

#define MAX_TEAMGOAL_STRING		256

class CTFGameRules : public CTeamplayRoundBasedRules
{
public:
	DECLARE_CLASS( CTFGameRules, CTeamplayRoundBasedRules );

	CTFGameRules();

	// Damage Queries.
	virtual bool	Damage_IsTimeBased( int iDmgType );			// Damage types that are time-based.
	virtual bool	Damage_ShowOnHUD( int iDmgType );				// Damage types that have client HUD art.
	virtual bool	Damage_ShouldNotBleed( int iDmgType );			// Damage types that don't make the player bleed.
	// TEMP:
	virtual int		Damage_GetTimeBased( void );		
	virtual int		Damage_GetShowOnHud( void );
	virtual int		Damage_GetShouldNotBleed( void );

	int				GetFarthestOwnedControlPoint( int iTeam, bool bWithSpawnpoints );
	virtual bool	TeamMayCapturePoint( int iTeam, int iPointIndex );
	virtual bool	PlayerMayCapturePoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason = NULL, int iMaxReasonLength = 0 );
	virtual bool	PlayerMayBlockPoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason = NULL, int iMaxReasonLength = 0 );

	CTeamRoundTimer* GetInfectionRoundTimer( void ) { return m_hInfectionTimer.Get(); }
	CTeamRoundTimer* GetBlueKothRoundTimer( void ) { return m_hBlueKothTimer.Get(); }
	CTeamRoundTimer* GetRedKothRoundTimer( void ) { return m_hRedKothTimer.Get(); }
	
	static int		CalcPlayerScore( RoundStats_t *pRoundStats );

	bool			IsBirthday( void );

	virtual const unsigned char *GetEncryptionKey( void ) { return (unsigned char *)"Op3Nf0rt"; }

#ifdef GAME_DLL
public:
	// Override this to prevent removal of game specific entities that need to persist
	virtual bool	RoundCleanupShouldIgnore( CBaseEntity *pEnt );
	virtual bool	ShouldCreateEntity( const char *pszClassName );
	virtual void	CleanUpMap( void );

	virtual void	FrameUpdatePostEntityThink();

	// Called when a new round is being initialized
	virtual void	SetupOnRoundStart( void );

	virtual void	SetupMutator( void );
	
	virtual void	PassAllTracks( void );

	// Called when a new round is off and running
	virtual void	SetupOnRoundRunning( void );

	// Called before a new round is started (so the previous round can end)
	virtual void	PreviousRoundEnd( void );

	// Send the team scores down to the client
	virtual void	SendTeamScoresEvent( void ) { return; }

	// Send the end of round info displayed in the win panel
	virtual void	SendWinPanelInfo( void );

	// Setup spawn points for the current round before it starts
	virtual void	SetupSpawnPointsForRound( void );

	// Called when a round has entered stalemate mode (timer has run out)
	virtual void	SetupOnStalemateStart( void );
	virtual void	SetupOnStalemateEnd( void );

	void			RecalculateControlPointState( void );

	virtual void	HandleSwitchTeams( void );
	virtual void	HandleScrambleTeams( void );
	bool			CanChangeClassInStalemate( void );

	virtual void	SetRoundOverlayDetails( void );	
	virtual void	ShowRoundInfoPanel( CTFPlayer *pPlayer = NULL ); // NULL pPlayer means show the panel to everyone

	virtual bool	TimerMayExpire( void );

	void			SetInfectionRoundTimer(CTeamRoundTimer *pTimer) { m_hInfectionTimer.Set( pTimer ); }
	void			SetRedKothRoundTimer(CTeamRoundTimer *pTimer) { m_hRedKothTimer.Set( pTimer ); }
	void			SetBlueKothRoundTimer(CTeamRoundTimer *pTimer) { m_hBlueKothTimer.Set( pTimer ); }

	virtual void	Activate();

	virtual bool	AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info );

	void			SetTeamGoalString( int iTeam, const char *pszGoal );

	// Speaking, vcds, voice commands.
	virtual void	InitCustomResponseRulesDicts();
	virtual void	ShutdownCustomResponseRulesDicts();

	virtual bool	HasPassedMinRespawnTime( CBasePlayer *pPlayer );

	bool			ShouldScorePerRound( void );
	
	static int		PlayerRoundScoreSortFunc( const PlayerRoundScore_t *pRoundScore1, const PlayerRoundScore_t *pRoundScore2 );
protected:
	virtual void	InitTeams( void );

	virtual void	RoundRespawn( void );

	virtual void	InternalHandleTeamWin( int iWinningTeam );
	

	virtual void FillOutTeamplayRoundWinEvent( IGameEvent *event );

	virtual bool CanChangelevelBecauseOfTimeLimit( void );
	virtual bool CanGoToStalemate( void );
#endif // GAME_DLL

public:
	// Return the value of this player towards capturing a point
	virtual int		GetCaptureValueForPlayer( CBasePlayer *pPlayer );

	// Collision and Damage rules.
	virtual bool	ShouldCollide( int collisionGroup0, int collisionGroup1 );
	
	int GetTimeLeft( void );

	void BeginInfection( void );
	void SelectInfector( void );
	void FinishInfection( void );

	virtual int GetMaxHunted( int iTeamNumber );
	virtual int GetHuntedCount( int iTeamNumber );

	// Get the view vectors for this mod.
	virtual const CViewVectors *GetViewVectors() const;

	virtual void FireGameEvent( IGameEvent *event );

	virtual const wchar_t *GetLocalizedGameTypeName( void );

	virtual bool FlagsMayBeCapped( void );
	virtual bool WeaponSpawnersMayBeUsed( void );

	virtual bool	IsInKothMode( void ) { return m_bKOTH; }

	void	RunPlayerConditionThink ( void );

	const char *GetTeamGoalString( int iTeam );

	virtual bool	IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer ) { return true; }

	CNetworkVar( int, m_iCosmeticCount ); 

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes data tables able to access our private vars.

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	HandleOvertimeBegin();
	Vector			GetTeamGlowColor( int nTeam );

	bool			ShouldShowTeamGoal( void );

	const char *GetVideoFileForMap( bool bWithExtension = true );

#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes data tables able to access our private vars.
	
	virtual ~CTFGameRules();

	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	virtual void Think();
	virtual	void EntityLimitPrevention();

	bool CheckTimeLimit();
	bool CheckWinLimit();
	bool CheckMaxRounds( bool bAllowEnd = true );
	bool CheckCapsPerRound();
	void CheckDOMScores( int dom_score_red_old, int dom_score_blue_old );

	virtual bool FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info );

	// Spawing rules.
	CBaseEntity *GetPlayerSpawnSpot( CBasePlayer *pPlayer );
	bool IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers );
	bool IsSpawnPointValidNoClass( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers );

	virtual float FlItemRespawnTime( CItem *pItem );
	virtual Vector VecItemRespawnSpot( CItem *pItem );
	virtual QAngle VecItemRespawnAngles( CItem *pItem );

	virtual const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );
	void ClientSettingsChanged( CBasePlayer *pPlayer );
	void ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues );
	void GetTaggedConVarList( KeyValues *pCvarTagList );
	void ChangePlayerName( CTFPlayer *pPlayer, const char *pszNewName );

	virtual VoiceCommandMenuItem_t *VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem ); 

	bool IsInPreMatch() const;
	float GetPreMatchEndTime() const;	// Returns the time at which the prematch will be over.
	void GoToIntermission( void );

	virtual int GetAutoAimMode()	{ return AUTOAIM_NONE; }

	bool CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex );

	virtual const char *GetGameDescription( void ){ return "Open Fortress"; }

	// Sets up g_pPlayerResource.
	virtual void CreateStandardEntities();

	virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	virtual CBasePlayer *GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim );

	void CalcDominationAndRevenge( CTFPlayer *pAttacker, CTFPlayer *pVictim, bool bIsAssist, int *piDeathFlags );

	const char *GetKillingWeaponName( const CTakeDamageInfo &info, CTFPlayer *pVictim );
	CBasePlayer *GetAssister( CBasePlayer *pVictim, CBasePlayer *pScorer, CBaseEntity *pInflictor );
	CTFPlayer *GetRecentDamager( CTFPlayer *pVictim, int iDamager, float flMaxElapsed );

	virtual void ClientDisconnected( edict_t *pClient );

	virtual void  RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore );

	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );

	virtual bool  FlPlayerFallDeathDoesScreenFade( CBasePlayer *pl ) { return false; }

	virtual bool UseSuicidePenalty() { return false; }

	int		GetPreviousRoundWinners( void ) { return m_iPreviousRoundWinners; }

	void	SendHudNotification( IRecipientFilter &filter, HudNotification_t iType );
	void	SendHudNotification( IRecipientFilter &filter, const char *pszText, const char *pszIcon, int iTeam = TEAM_UNASSIGNED );

private:

	int DefaultFOV( void ) { return 75; }

#endif

private:

#ifdef GAME_DLL

	Vector2D	m_vecPlayerPositions[MAX_PLAYERS];

	CUtlVector<CHandle<CHealthKit> > m_hDisabledHealthKits;	
	CUtlVector<CHandle<CAmmoPack> >  m_hDisabledAmmoPack;	
	CUtlVector<CHandle<CWeaponSpawner> >  m_hDisabledWeaponSpawners;	
	
	char	m_szMostRecentCappers[MAX_PLAYERS+1];	// list of players who made most recent capture.  Stored as string so it can be passed in events.
	int		m_iNumCaps[TF_TEAM_COUNT];				// # of captures ever by each team during a round

	int SetCurrentRoundStateBitString();
	void SetMiniRoundBitMask( int iMask );
	int m_iPrevRoundState;	// bit string representing the state of the points at the start of the previous miniround
	int m_iCurrentRoundState;
	int m_iCurrentMiniRoundMask;
	float m_flTimerMayExpireAt;
	bool m_bStartedVote;
	
#endif
	CNetworkVar( int, m_nGameType ); // Type of game this map is (CTF, CP)
	CNetworkVar( int, m_nMutator ); // Type of game this map is (CTF, CP)
	CNetworkVar( int, m_nCurrFrags ); // Biggest frag count
	CNetworkVar( bool, m_bKOTH ); // is the gamemode KOTH right now?
	CNetworkVar( CHandle<CTeamRoundTimer>, m_hRedKothTimer );
	CNetworkVar( CHandle<CTeamRoundTimer>, m_hBlueKothTimer );
	CNetworkVar( CHandle<CTeamRoundTimer>, m_hInfectionTimer );
	CNetworkString( m_pszTeamGoalStringRed, MAX_TEAMGOAL_STRING );
	CNetworkString( m_pszTeamGoalStringBlue, MAX_TEAMGOAL_STRING );
	CNetworkString( m_pszTeamGoalStringMercenary, MAX_TEAMGOAL_STRING );
public:
	bool m_bControlSpawnsPerTeam[ MAX_TEAMS ][ MAX_CONTROL_POINTS ];
	int	 m_iPreviousRoundWinners;
	CNetworkVar( bool, m_bCapsInitialized );
	CNetworkVar( bool, m_bEscortOverride );
	CNetworkVar( int, m_nHuntedCount_red );
	CNetworkVar( int, m_nMaxHunted_red );
	CNetworkVar( int, m_nHuntedCount_blu );
	CNetworkVar( int, m_nMaxHunted_blu );
	CNetworkVar( int, m_nDomScore_limit );
	CNetworkVar( int, m_nDomScore_blue );
	CNetworkVar( int, m_nDomScore_red );

#ifdef GAME_DLL
	// List of active pipebombs
	typedef CHandle<CPathTrack>	TrackHandle;
	CUtlVector<TrackHandle>		m_hTracksToPass;

	typedef CHandle<CTFTeamSpawn>	SpawnHandle;
	CUtlVector<SpawnHandle>		m_hReEnableSpawns;
	bool	m_bDisableRedSpawns;
	bool	m_bDisableBluSpawns;
	virtual void	KeepTeamSpawns( int iTeamNumber );
	virtual void	DisableSpawns( int iTeamNumber );
	bool			bMultiweapons;
#endif
	CNetworkVar( bool, m_nbDontCountKills ); // Do we Count Kills?
	
	string_t 		m_iszWeaponName[50];

	int				m_iMaxLevel;
	bool			m_bListOnly;
	int				m_iRequiredKills;
	
	virtual bool	IsDMGamemode(void);
	virtual bool	IsTDMGamemode(void);
	virtual bool	IsDOMGamemode(void);
	virtual void	CheckTDM(void);
	virtual bool	IsTeamplay(void);
	virtual bool 	DontCountKills( void );
	virtual bool	IsGGGamemode(void);
	virtual bool	Is3WaveGamemode(void);
	virtual bool	IsArenaGamemode(void);
	virtual bool	IsESCGamemode(void);
	virtual bool	IsCoopGamemode(void);
	virtual bool	IsZSGamemode(void);
	virtual bool	IsInfGamemode(void);
	virtual bool	IsPayloadOverride(void);
	virtual bool	IsHL2(void);
	virtual bool	Force3DSkybox(void) { return m_bForce3DSkybox; }
	virtual bool	UsesMoney(void);
	virtual bool	UsesDMBuckets( void );
	void	FireGamemodeOutputs(void);
	int		m_iBirthdayMode;

	// Domination gamemode
	int m_nDomScore_time;
	bool m_bDomWinOnLimit;
	bool m_bDomRedThreshold;
	bool m_bDomBlueThreshold;
	bool m_bDomRedLeadThreshold;
	bool m_bDomBlueLeadThreshold;

	bool InGametype( int nGametype );
	void AddGametype( int nGametype );	
	void RemoveGametype( int nGametype );	


	int GetMutator( void );
	bool IsMutator( int nMutator );

#ifdef GAME_DLL
	void SetMutator( int nMutator );	
#endif
	
	CNetworkVar( bool, m_bUsesHL2Hull );
	CNetworkVar( bool, m_bForce3DSkybox );
	CNetworkVar( bool, m_bUsesMoney );
	CNetworkVar( bool, m_bIsTeamplay ); //Used to check if of_logic_dm has teamplay enabled
	CNetworkVar( bool, m_bIsTDM ); //Usualy we would just make this a bool function but since it requires a big loop 
								   // which would make it fairly unoptimized and the fact that its used in a lot of places
								   // we decided to make a bool thats set on the start of the round
								   // should we find a better solution this could get removed
	CNetworkVar( bool, m_bIsHL2 );
	
#ifdef GAME_DLL
	bool	NPC_ShouldDropGrenade( CBasePlayer *pRecipient );
	bool	NPC_ShouldDropHealth( CBasePlayer *pRecipient );
	void	NPC_DroppedHealth( void );
	void	NPC_DroppedGrenade( void );
	bool	MegaPhyscannonActive( void ) { return false;	}
	
	virtual bool IsAlyxInDarknessMode();
	virtual bool			ShouldBurningPropsEmitLight();

	virtual void			InitDefaultAIRelationships( void );
	virtual const char*		AIClassText(int classType);
#endif
	float	m_flLastHealthDropTime;
	float	m_flLastGrenadeDropTime;
};

//-----------------------------------------------------------------------------
// Gets us at the team fortress game rules
//-----------------------------------------------------------------------------

inline CTFGameRules* TFGameRules()
{
	return static_cast<CTFGameRules*>(g_pGameRules);
}

#ifdef GAME_DLL
	bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround );
#endif

#endif // TF_GAMERULES_H
