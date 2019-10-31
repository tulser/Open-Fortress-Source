//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose: The TF Game rules 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "ammodef.h"
#include "KeyValues.h"
#include "tf_weaponbase.h"
#include "time.h"
#include "tf_shareddefs.h"
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include "tier3/tier3.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "gameeventdefs.h"

#ifdef CLIENT_DLL
	#include <game/client/iviewport.h>
	#include "c_tf_player.h"
	#include "c_tf_objective_resource.h"
#else
	#include "basemultiplayerplayer.h"
	#include "voice_gamemgr.h"
	#include "items.h"
	#include "team.h"
	#include "tf_bot_temp.h"
	#include "tf_player.h"
	#include "tf_team.h"
	#include "player_resource.h"
	#include "entity_tfstart.h"
	#include "filesystem.h"
	#include "tf_obj.h"
	#include "tf_objective_resource.h"
	#include "tf_player_resource.h"
	#include "team_control_point_master.h"
	#include "entity_roundwin.h"
	#include "playerclass_info_parse.h"
	#include "team_train_watcher.h"
	#include "entity_roundwin.h"
	#include "coordsize.h"
	#include "entity_healthkit.h"
	#include "tf_gamestats.h"
	#include "entity_capture_flag.h"
	#include "entity_weapon_spawner.h"
	#include "tf_player_resource.h"
	#include "tf_obj_sentrygun.h"
	#include "tier0/icommandline.h"
	#include "activitylist.h"
	#include "AI_ResponseSystem.h"
	#include "hl2orange.spa.h"
	#include "hltvdirector.h"
	#include "globalstate.h"
    #include "igameevents.h"
	#include "trains.h"
	#include "pathtrack.h"
	#include "entitylist.h"
	#include "trigger_area_capture.h"
	
	#include "ai_basenpc.h"
	#include "ai_dynamiclink.h"
	#include "nav_mesh.h"
	#include "vote_controller.h"
	#include "tf_voteissues.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define ITEM_RESPAWN_TIME	10.0f

enum
{
	BIRTHDAY_RECALCULATE,
	BIRTHDAY_OFF,
	BIRTHDAY_ON,
};

static int g_TauntCamAchievements[] = 
{
	0,		// TF_CLASS_UNDEFINED

	0,		// TF_CLASS_SCOUT,	
	0,		// TF_CLASS_SNIPER,
	0,		// TF_CLASS_SOLDIER,
	0,		// TF_CLASS_DEMOMAN,
	0,		// TF_CLASS_MEDIC,
	0,		// TF_CLASS_HEAVYWEAPONS,
	0,		// TF_CLASS_PYRO,
	0,		// TF_CLASS_SPY,
	0,		// TF_CLASS_ENGINEER,

	0,		// TF_CLASS_MERCENARY,
	0,		// TF_CLASS_CIVILIAN,
	0,		// TF_CLASS_JUGGERNAUT,
	0,		// TF_CLASS_COUNT_ALL,
};

string_t m_iszDefaultWeaponName[50] =
{
	MAKE_STRING("tf_weapon_rocketlauncher_dm"),
	MAKE_STRING("tf_weapon_grenadelauncher_mercenary"),
	MAKE_STRING("tf_weapon_railgun"),
	MAKE_STRING("tf_weapon_gatlinggun"),
	MAKE_STRING("tf_weapon_flamethrower"),
	MAKE_STRING("tf_weapon_smg_mercenary"),
	MAKE_STRING("tf_weapon_supershotgun"),
	MAKE_STRING("tf_weapon_revolver_mercenary"),
	MAKE_STRING("tf_weapon_shotgun_mercenary"),
	MAKE_STRING("tf_weapon_tommygun"),
	MAKE_STRING("tf_weapon_nailgun"),
	MAKE_STRING("tf_weapon_pistol_akimbo"),
	MAKE_STRING("tf_weapon_pistol_mercenary"),	
	MAKE_STRING("tf_weapon_knife")	
};

extern ConVar mp_capstyle;
extern ConVar sv_turbophysics;
extern ConVar of_bunnyhop;
extern ConVar of_crouchjump;
extern ConVar of_forceclass;
extern ConVar of_resistance;
extern ConVar mp_disable_respawn_times;
extern ConVar fraglimit;
extern ConVar of_bunnyhop_max_speed_factor;
extern ConVar tf_maxspeed;
extern ConVar sv_airaccelerate;
extern ConVar of_knockback_all;
extern ConVar of_knockback_bullets;
extern ConVar of_knockback_melee;
extern ConVar of_knockback_explosives;
extern ConVar teamplay;
extern ConVar of_gravitygun;

ConVar tf_caplinear						( "tf_caplinear", "1", FCVAR_REPLICATED, "If set to 1, teams must capture control points linearly." );
ConVar tf_stalematechangeclasstime		( "tf_stalematechangeclasstime", "20", FCVAR_REPLICATED, "Amount of time that players are allowed to change class in stalemates." );
ConVar tf_birthday						( "tf_birthday", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );

// Open Fortress Convars
ConVar of_arena						( "of_arena", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles Arena mode." );
ConVar of_infection					( "of_infection", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles Infection mode." );
ConVar of_coop						( "of_coop", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles Coop mode." );
ConVar of_threewave					( "of_threewave", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles Threewave." );

ConVar of_allow_allclass_pickups 	( "of_allow_allclass_pickups", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Non-Mercenary Classes can pickup dropped weapons.");
ConVar of_allow_allclass_spawners 	( "of_allow_allclass_spawners", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Non-Mercenary Classes can pickup weapons from spawners.");
ConVar of_rocketjump_multiplier		( "of_rocketjump_multiplier", "3", FCVAR_NOTIFY | FCVAR_REPLICATED, "How much blast jumps should push you further than when you blast enemies." );
ConVar of_selfdamage				( "of_selfdamage", "-1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Weather or not you should deal self damage with explosives.",true, -1, true, 1  );
ConVar of_allow_special_classes		( "of_allow_special_classes", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allow Special classes outside of their respective modes.");
ConVar of_payload_override			( "of_payload_override", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Turn on Escort instead of Payload.");

// Not implemented.
// ConVar of_ggweaponlist		( "of_ggweaponlist", "cfg/gg_weaponlist_default.txt" );
ConVar of_mutator			( "of_mutator", "0", FCVAR_NOTIFY | FCVAR_REPLICATED,
							"Defines the gamemode mutators to be used.\n List of mutators:\n 0 : Disabled\n 1 : Instagib(Railgun + Crowbar)\n 2 : Instagib(Railgun)\n 3 : Clan Arena\n 4 : Unholy Trinity\n 5 : Rocket Arena\n 6 : Gun Game",
							true, 0, true, 6 );

/*	List of mutators:
	0: Disabled
	1: Instagib (Railgun + Crowbar)
	2: Instagib (Railgun)
	3: Clan Arena
	4: Unholy Trinity
	5: Rocket Arena
	6: Gun Game
	7: Randomizer
*/

/*	Individual gamemode mutators, deprecated by the convar above.
	ConVar of_instagib			( "of_instagib", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Toggles Instagib.", true, 0, true, 2 );
	ConVar of_clanarena		( "of_clanarena", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles Clan Arena mutators.", true, 0, true, 2 );
	ConVar of_gungame			( "of_gungame", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles Gun Game mode." );
*/

ConVar of_usehl2hull		( "of_usehl2hull", "-1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Use HL2 collision hull." );
ConVar of_multiweapons		( "of_multiweapons", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles the Quake-like multi weapon system." );
ConVar of_weaponspawners	( "of_weaponspawners", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles weapon spawners." );
ConVar of_powerups			( "of_powerups", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles powerups." );

#ifdef GAME_DLL
// TF overrides the default value of the convars below.
ConVar mp_waitingforplayers_time( "mp_waitingforplayers_time", "30", FCVAR_GAMEDLL, "Length in seconds to wait for players." );
ConVar tf_gravetalk( "tf_gravetalk", "1", FCVAR_NOTIFY, "Allows living players to hear dead players using text/voice chat." );
ConVar tf_spectalk( "tf_spectalk", "1", FCVAR_NOTIFY, "Allows living players to hear spectators using text chat." );

// Infection stuff
ConVar of_infection_preparetime		 ( "of_infection_preparetime", "20", FCVAR_GAMEDLL, "How many seconds survivors have to prepare before the Infection." );
ConVar of_infection_roundtime		 ( "of_infection_roundtime", "300", FCVAR_GAMEDLL, "How many seconds survivors need to... survive for after the Infection." );
ConVar of_infection_zombie_threshold ( "of_infection_zombie_threshold", "6", FCVAR_GAMEDLL, "For every n humans, this many zombies are selected when the Infection starts." );
#endif

ConVar of_retromode ( "of_retromode", "-1", FCVAR_REPLICATED | FCVAR_NOTIFY, \
					"Sets the Retro mode type, which turns on TFC classes and mechanics such as armor.\n-1 = Default to map settings\n 0 = Force off\n 1 = Force on\n 2 = Force on for Blu only\n 3 = Force on for Red only" );

ConVar of_grenades	( "of_grenades", "-1", FCVAR_REPLICATED | FCVAR_NOTIFY, \
					"Enables grenades.\n-1 = Depends on Retro mode\n 0 = Forced off\n 1 = Forced on (frags only)\n 2 = Forced on (class-based grenades)" );

ConVar of_navmesh_spawns( "of_navmesh_spawns", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Select random spawns using the navigation mesh on Deathmatch mode" );

#ifdef GAME_DLL
//listner class creates a listener for the mEvent and returns the mEvent as true
class CMyListener : public IGameEventListener2
{
	CMyListener()
	{
		// add myself as client-side listener for this event
		gameeventmanager->AddListener(this, "teamplay_round_win", false);
	}
	//this is the event checker
	void FireGameEvent(IGameEvent* mEvent)
	{
	}
};
void ValidateCapturesPerRound( IConVar *pConVar, const char *oldValue, float flOldValue )
{
	ConVarRef var( pConVar );

	if ( var.GetInt() <= 0 )
	{
		// reset the flag captures being played in the current round
		int nTeamCount = TFTeamMgr()->GetTeamCount();
		for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( !pTeam )
				continue;

			pTeam->SetFlagCaptures( 0 );
		}
	}
}
#endif	

ConVar tf_flag_caps_per_round( "tf_flag_caps_per_round", "3", FCVAR_REPLICATED, "Number of flag captures per round on CTF maps. Set to 0 to disable.", true, 0, true, 9
#ifdef GAME_DLL
							  , ValidateCapturesPerRound
#endif
							  );


/**
 * Player hull & eye position for standing, ducking, etc.  This version has a taller
 * player height, but goldsrc-compatible collision bounds.
 */
static CViewVectors g_TFViewVectors(
	Vector( 0, 0, 72 ),		//VEC_VIEW (m_vView) eye position
							
	Vector(-24, -24, 0 ),	//VEC_HULL_MIN (m_vHullMin) hull min
	Vector( 24,  24, 82 ),	//VEC_HULL_MAX (m_vHullMax) hull max
												
	Vector(-24, -24, 0 ),	//VEC_DUCK_HULL_MIN (m_vDuckHullMin) duck hull min
	Vector( 24,  24, 55 ),	//VEC_DUCK_HULL_MAX	(m_vDuckHullMax) duck hull max
	Vector( 0, 0, 45 ),		//VEC_DUCK_VIEW		(m_vDuckView) duck view
												
	Vector( -10, -10, -10 ),	//VEC_OBS_HULL_MIN	(m_vObsHullMin) observer hull min
	Vector(  10,  10,  10 ),	//VEC_OBS_HULL_MAX	(m_vObsHullMax) observer hull max
												
	Vector( 0, 0, 14 )		//VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight) dead view height
);

static CViewVectors g_HLViewVectors(
	Vector( 0, 0, 64 ),			//VEC_VIEW (m_vView)
								
	Vector(-16, -16, 0 ),		//VEC_HULL_MIN (m_vHullMin)
	Vector( 16,  16,  72 ),		//VEC_HULL_MAX (m_vHullMax)
													
	Vector(-16, -16, 0 ),		//VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16,  36 ),		//VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 28 ),			//VEC_DUCK_VIEW		(m_vDuckView)
													
	Vector(-10, -10, -10 ),		//VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),		//VEC_OBS_HULL_MAX	(m_vObsHullMax)
													
	Vector( 0, 0, 14 )			//VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)
);					

const CViewVectors *CTFGameRules::GetViewVectors() const
{

	if ( m_bUsesHL2Hull && of_usehl2hull.GetInt() < 0 || of_usehl2hull.GetInt() > 0 )
		return &g_HLViewVectors;

	return &g_TFViewVectors;
}

REGISTER_GAMERULES_CLASS( CTFGameRules );

BEGIN_NETWORK_TABLE_NOBASE( CTFGameRules, DT_TFGameRules )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_nGameType ) ),
	RecvPropInt( RECVINFO( m_nMutator ) ),
	RecvPropInt( RECVINFO( m_iCosmeticCount ) ),
	RecvPropInt( RECVINFO( m_nCurrFrags ) ),
	RecvPropInt( RECVINFO( m_nHuntedCount_red ) ),
	RecvPropInt( RECVINFO( m_nMaxHunted_red ) ),
	RecvPropInt( RECVINFO( m_nHuntedCount_blu ) ),
	RecvPropInt( RECVINFO( m_nMaxHunted_blu ) ),	
	RecvPropInt( RECVINFO( m_nDomScore_limit ) ),	
	RecvPropInt( RECVINFO( m_nDomScore_blue ) ),	
	RecvPropInt( RECVINFO( m_nDomScore_red ) ),	
	RecvPropString( RECVINFO( m_pszTeamGoalStringRed ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringBlue ) ),
	RecvPropString( RECVINFO(m_pszTeamGoalStringMercenary)),
	RecvPropBool( RECVINFO( m_bEscortOverride ) ),
	RecvPropBool( RECVINFO( m_bCapsInitialized ) ),
	RecvPropBool( RECVINFO( m_bIsTeamplay ) ),
	RecvPropBool( RECVINFO( m_bIsTDM ) ),
	RecvPropBool( RECVINFO( m_bIsHL2 ) ),
	RecvPropBool( RECVINFO( m_nbDontCountKills ) ),
	RecvPropBool( RECVINFO( m_bUsesHL2Hull ) ),
	RecvPropBool( RECVINFO( m_bForce3DSkybox ) ),
	RecvPropBool( RECVINFO( m_bUsesMoney ) ),
	RecvPropBool( RECVINFO( m_bKOTH ) ),
	RecvPropEHandle( RECVINFO( m_hRedKothTimer ) ), 
	RecvPropEHandle( RECVINFO( m_hBlueKothTimer ) ),
	RecvPropEHandle( RECVINFO( m_hInfectionTimer ) ),
#else

	SendPropInt( SENDINFO( m_nGameType ), TF_GAMETYPE_LAST, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nMutator ), 3, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_iCosmeticCount ) ),
	SendPropInt( SENDINFO( m_nCurrFrags ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nHuntedCount_red ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nMaxHunted_red ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nHuntedCount_blu ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nMaxHunted_blu ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDomScore_limit ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDomScore_blue ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDomScore_red ), 7, SPROP_UNSIGNED ),
	SendPropString( SENDINFO( m_pszTeamGoalStringRed ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringBlue ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringMercenary ) ),
	SendPropBool( SENDINFO( m_bCapsInitialized ) ),
	SendPropBool( SENDINFO( m_bEscortOverride ) ),
	SendPropBool( SENDINFO( m_bIsTeamplay ) ),
	SendPropBool( SENDINFO( m_bIsTDM ) ),
	SendPropBool( SENDINFO( m_bIsHL2 ) ),
	SendPropBool( SENDINFO( m_nbDontCountKills ) ),
	SendPropBool( SENDINFO( m_bUsesHL2Hull ) ),
	SendPropBool( SENDINFO( m_bForce3DSkybox ) ),
	SendPropBool( SENDINFO( m_bUsesMoney ) ),
	SendPropBool( SENDINFO( m_bKOTH ) ),
	SendPropEHandle( SENDINFO( m_hRedKothTimer ) ), 
	SendPropEHandle( SENDINFO( m_hBlueKothTimer ) ),
	SendPropEHandle( SENDINFO( m_hInfectionTimer ) )
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_gamerules, CTFGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( TFGameRulesProxy, DT_TFGameRulesProxy )

#ifdef CLIENT_DLL
	void RecvProxy_TFGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CTFGameRules *pRules = TFGameRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CTFGameRulesProxy, DT_TFGameRulesProxy )
		RecvPropDataTable( "tf_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_TFGameRules ), RecvProxy_TFGameRules )
	END_RECV_TABLE()
#else
	void *SendProxy_TFGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CTFGameRules *pRules = TFGameRules();
		Assert( pRules );
		pRecipients->SetAllRecipients();
		return pRules;
	}

	BEGIN_SEND_TABLE( CTFGameRulesProxy, DT_TFGameRulesProxy )
		SendPropDataTable( "tf_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_TFGameRules ), SendProxy_TFGameRules )
	END_SEND_TABLE()
#endif

#ifdef GAME_DLL
BEGIN_DATADESC( CTFGameRulesProxy )
	//Keyfields
	DEFINE_KEYFIELD( m_bUsesHL2Hull , FIELD_BOOLEAN, "UsesHL2Hull"),
	DEFINE_KEYFIELD( m_bForce3DSkybox , FIELD_BOOLEAN, "Force3DSkybox"),
	DEFINE_KEYFIELD( m_bUsesMoney , FIELD_BOOLEAN, "UsesMoney"),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetRedTeamRespawnWaveTime", InputSetRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetBlueTeamRespawnWaveTime", InputSetBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMercenaryTeamRespawnWaveTime", InputSetMercenaryTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddRedTeamRespawnWaveTime", InputAddRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddBlueTeamRespawnWaveTime", InputAddBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddMercenaryTeamRespawnWaveTime", InputAddMercenaryTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRedTeamGoalString", InputSetRedTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMercenaryTeamGoalString", InputSetMercenaryTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetBlueTeamGoalString", InputSetBlueTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedTeamRole", InputSetRedTeamRole ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBlueTeamRole", InputSetBlueTeamRole ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMercenaryTeamRole", InputSetMercenaryTeamRole ),
// - InputSetRedKothClockActive (Offset 0) (Input)(0 Bytes) - SetRedKothClockActive
// - InputSetBlueKothClockActive (Offset 0) (Input)(0 Bytes) - SetBlueKothClockActive
// - InputPlayVORed (Offset 0) (Input)(0 Bytes) - PlayVORed
// - InputPlayVOBlue (Offset 0) (Input)(0 Bytes) - PlayVOBlue
// - InputPlayVO (Offset 0) (Input)(0 Bytes) - PlayVO
	DEFINE_INPUTFUNC( FIELD_VOID, "SetRedKothClockActive", InputSetRedKothClockActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetBlueKothClockActive", InputSetBlueKothClockActive ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVORed", InputPlayVORed ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVOBlue", InputPlayVOBlue ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVOMercenary", InputPlayVOMercenary ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVO", InputPlayVO ),
	
	DEFINE_OUTPUT( m_OutputIsCTF,	"IsCTF" ),
	DEFINE_OUTPUT( m_OutputIsCP,	"IsCP" ),
	DEFINE_OUTPUT( m_OutputIsDM,	"IsDM" ),
	DEFINE_OUTPUT( m_OutputIsTeamplay,	"IsTeamplay" ),
	DEFINE_OUTPUT( m_OutputIsGunGame,	"IsGunGame" ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CTFGameRulesProxy::InputPlayVORed( inputdata_t &inputdata )
{
	if ( TFGameRules() )
		TFGameRules()->BroadcastSound( TF_TEAM_RED, inputdata.value.String() );
}

void CTFGameRulesProxy::InputPlayVOBlue( inputdata_t &inputdata )
{
	if ( TFGameRules() )
		TFGameRules()->BroadcastSound( TF_TEAM_BLUE, inputdata.value.String() );
}

void CTFGameRulesProxy::InputPlayVOMercenary( inputdata_t &inputdata )
{
	if ( TFGameRules() )
		TFGameRules()->BroadcastSound( TF_TEAM_MERCENARY, inputdata.value.String() );
}

void CTFGameRulesProxy::InputPlayVO( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->BroadcastSound( TF_TEAM_RED, inputdata.value.String() );
		TFGameRules()->BroadcastSound( TF_TEAM_BLUE, inputdata.value.String() );
		TFGameRules()->BroadcastSound( TF_TEAM_MERCENARY, inputdata.value.String() );
		TFGameRules()->BroadcastSound( TEAM_SPECTATOR, inputdata.value.String() );
		TFGameRules()->BroadcastSound( TEAM_UNASSIGNED, inputdata.value.String() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CTFGameRulesProxy::InputSetRedKothClockActive(inputdata_t &inputdata)
{
	if ( TFGameRules() && TFGameRules()->GetRedKothRoundTimer() )
	{
		TFGameRules()->GetRedKothRoundTimer()->InputEnable( inputdata );

		if ( TFGameRules()->GetBlueKothRoundTimer() )
		{
			TFGameRules()->GetBlueKothRoundTimer()->InputDisable( inputdata );
		}
	}
}

void CTFGameRulesProxy::InputSetBlueKothClockActive(inputdata_t &inputdata)
{
	if ( TFGameRules() && TFGameRules()->GetBlueKothRoundTimer() )
	{
		TFGameRules()->GetBlueKothRoundTimer()->InputEnable( inputdata );

		if ( TFGameRules()->GetRedKothRoundTimer() )
		{
			TFGameRules()->GetRedKothRoundTimer()->InputDisable( inputdata );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_RED, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}

void CTFGameRulesProxy::InputSetMercenaryTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddRedTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_RED, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddBlueTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}

void CTFGameRulesProxy::InputAddMercenaryTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_MERCENARY, inputdata.value.Float() );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_RED, inputdata.value.String() );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_BLUE, inputdata.value.String() );
}

void CTFGameRulesProxy::InputSetMercenaryTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_MERCENARY, inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_RED );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

void CTFGameRulesProxy::InputSetMercenaryTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_MERCENARY );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::Activate()
{
	TFGameRules()->Activate();
	TFGameRules()->m_bUsesHL2Hull = m_bUsesHL2Hull;
	TFGameRules()->m_bForce3DSkybox = m_bForce3DSkybox;
	TFGameRules()->m_bUsesMoney = m_bUsesMoney;

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// DM Logic 
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS(of_logic_dm, CTFLogicDM);

BEGIN_DATADESC( CTFLogicDM )
	//Keyfields
	DEFINE_KEYFIELD( m_bIsTeamplay, FIELD_BOOLEAN, "IsTeamplay"),
	DEFINE_KEYFIELD( m_bDontCountKills, FIELD_BOOLEAN, "DontCountKills"),
END_DATADESC()

void CTFLogicDM::Spawn(void)
{
	TFGameRules()->m_bIsTeamplay = m_bIsTeamplay;
	TFGameRules()->m_nbDontCountKills = m_bDontCountKills;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// GG Logic 
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS(of_logic_gg, CTFLogicGG);

BEGIN_DATADESC( CTFLogicGG )
	//Keyfields
	DEFINE_KEYFIELD( m_bListOnly, FIELD_BOOLEAN, "ListOnly"),
	DEFINE_KEYFIELD( m_iRequiredKills, FIELD_INTEGER, "RequiredKills"),
	
	DEFINE_KEYFIELD(m_iszWeaponName[0],  FIELD_STRING, "WeaponName01"),
	DEFINE_KEYFIELD(m_iszWeaponName[1],  FIELD_STRING, "WeaponName02"),
	DEFINE_KEYFIELD(m_iszWeaponName[2],  FIELD_STRING, "WeaponName03"),
	DEFINE_KEYFIELD(m_iszWeaponName[3],  FIELD_STRING, "WeaponName04"),
	DEFINE_KEYFIELD(m_iszWeaponName[4],  FIELD_STRING, "WeaponName05"),
	DEFINE_KEYFIELD(m_iszWeaponName[5],  FIELD_STRING, "WeaponName06"),
	DEFINE_KEYFIELD(m_iszWeaponName[6],  FIELD_STRING, "WeaponName07"),
	DEFINE_KEYFIELD(m_iszWeaponName[7],  FIELD_STRING, "WeaponName08"),
	DEFINE_KEYFIELD(m_iszWeaponName[8],  FIELD_STRING, "WeaponName09"),
	DEFINE_KEYFIELD(m_iszWeaponName[9],  FIELD_STRING, "WeaponName10"),
	DEFINE_KEYFIELD(m_iszWeaponName[10], FIELD_STRING, "WeaponName11"),
	DEFINE_KEYFIELD(m_iszWeaponName[11], FIELD_STRING, "WeaponName12"),
	DEFINE_KEYFIELD(m_iszWeaponName[12], FIELD_STRING, "WeaponName13"),
	DEFINE_KEYFIELD(m_iszWeaponName[13], FIELD_STRING, "WeaponName14"),
	DEFINE_KEYFIELD(m_iszWeaponName[14], FIELD_STRING, "WeaponName15"),
	DEFINE_KEYFIELD(m_iszWeaponName[15], FIELD_STRING, "WeaponName16"),
	DEFINE_KEYFIELD(m_iszWeaponName[16], FIELD_STRING, "WeaponName17"),
	DEFINE_KEYFIELD(m_iszWeaponName[17], FIELD_STRING, "WeaponName18"),
	DEFINE_KEYFIELD(m_iszWeaponName[18], FIELD_STRING, "WeaponName19"),
	DEFINE_KEYFIELD(m_iszWeaponName[19], FIELD_STRING, "WeaponName20"),
	DEFINE_KEYFIELD(m_iszWeaponName[20], FIELD_STRING, "WeaponName21"),
	DEFINE_KEYFIELD(m_iszWeaponName[21], FIELD_STRING, "WeaponName22"),
	DEFINE_KEYFIELD(m_iszWeaponName[22], FIELD_STRING, "WeaponName23"),
	DEFINE_KEYFIELD(m_iszWeaponName[23], FIELD_STRING, "WeaponName24"),
	DEFINE_KEYFIELD(m_iszWeaponName[24], FIELD_STRING, "WeaponName25"),
	DEFINE_KEYFIELD(m_iszWeaponName[25], FIELD_STRING, "WeaponName26"),
	DEFINE_KEYFIELD(m_iszWeaponName[26], FIELD_STRING, "WeaponName27"),
	DEFINE_KEYFIELD(m_iszWeaponName[27], FIELD_STRING, "WeaponName28"),
	DEFINE_KEYFIELD(m_iszWeaponName[28], FIELD_STRING, "WeaponName29"),
	DEFINE_KEYFIELD(m_iszWeaponName[29], FIELD_STRING, "WeaponName30"),
	DEFINE_KEYFIELD(m_iszWeaponName[30], FIELD_STRING, "WeaponName31"),
	DEFINE_KEYFIELD(m_iszWeaponName[31], FIELD_STRING, "WeaponName32"),
	DEFINE_KEYFIELD(m_iszWeaponName[32], FIELD_STRING, "WeaponName33"),
	DEFINE_KEYFIELD(m_iszWeaponName[33], FIELD_STRING, "WeaponName34"),
	DEFINE_KEYFIELD(m_iszWeaponName[34], FIELD_STRING, "WeaponName35"),
	DEFINE_KEYFIELD(m_iszWeaponName[35], FIELD_STRING, "WeaponName36"),
	DEFINE_KEYFIELD(m_iszWeaponName[36], FIELD_STRING, "WeaponName37"),
	DEFINE_KEYFIELD(m_iszWeaponName[37], FIELD_STRING, "WeaponName38"),
	DEFINE_KEYFIELD(m_iszWeaponName[38], FIELD_STRING, "WeaponName39"),
	DEFINE_KEYFIELD(m_iszWeaponName[39], FIELD_STRING, "WeaponName40"),
	DEFINE_KEYFIELD(m_iszWeaponName[40], FIELD_STRING, "WeaponName41"),	
	DEFINE_KEYFIELD(m_iszWeaponName[41], FIELD_STRING, "WeaponName42"),
	DEFINE_KEYFIELD(m_iszWeaponName[42], FIELD_STRING, "WeaponName43"),
	DEFINE_KEYFIELD(m_iszWeaponName[43], FIELD_STRING, "WeaponName44"),
	DEFINE_KEYFIELD(m_iszWeaponName[44], FIELD_STRING, "WeaponName45"),
	DEFINE_KEYFIELD(m_iszWeaponName[45], FIELD_STRING, "WeaponName46"),
	DEFINE_KEYFIELD(m_iszWeaponName[46], FIELD_STRING, "WeaponName47"),
	DEFINE_KEYFIELD(m_iszWeaponName[47], FIELD_STRING, "WeaponName48"),
	DEFINE_KEYFIELD(m_iszWeaponName[48], FIELD_STRING, "WeaponName49"),
	DEFINE_KEYFIELD(m_iszWeaponName[49], FIELD_STRING, "WeaponName50"),
END_DATADESC()

CTFLogicGG::CTFLogicGG()
{
	for ( int i = 0; i < 50; i++ )
	{
		m_iszWeaponName[i] = MAKE_STRING( "NULL" );
		TFGameRules()->m_iszWeaponName[i] = MAKE_STRING( "" );
	}
}

void CTFLogicGG::Spawn( void )
{
	int y = 0;
	m_iMaxLevel = 0;
	for ( int i = 0; i < 50; i++ )
	{
		if ( m_iszWeaponName[i] != MAKE_STRING( "NULL" ) )
		{
			m_iMaxLevel++;
			TFGameRules()->m_iszWeaponName[y] = m_iszWeaponName[i];
			y++;
		}
	}

	TFGameRules()->m_iMaxLevel = m_iMaxLevel;
	TFGameRules()->m_bListOnly = m_bListOnly;
	TFGameRules()->m_iRequiredKills = m_iRequiredKills;

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// TDM Logic 
//-----------------------------------------------------------------------------
class CTFLogicTDM : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFLogicTDM, CBaseEntity );
	void	Spawn( void );
};

void CTFLogicTDM::Spawn( void )
{
	BaseClass::Spawn();
}

LINK_ENTITY_TO_CLASS( of_logic_tdm, CTFLogicTDM);

//-----------------------------------------------------------------------------
// Civ Escort Logic 
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( of_logic_esc, CTFLogicESC );

BEGIN_DATADESC( CTFLogicESC )
	//Keyfields
	DEFINE_KEYFIELD( m_nMaxHunted_red, FIELD_INTEGER, "MaxRedHunted" ),
	DEFINE_KEYFIELD( m_nMaxHunted_blu, FIELD_INTEGER, "MaxBluHunted" ),

	DEFINE_OUTPUT( m_OnHuntedDeath, "OnHuntedDeath" ),
END_DATADESC()

void CTFLogicESC::Spawn(void)
{
	if ( TFGameRules() )
	{
		TFGameRules()->m_nMaxHunted_red = m_nMaxHunted_red;
		TFGameRules()->m_nMaxHunted_blu = m_nMaxHunted_blu;
	}	

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Domination Logic (this can be combined with escort)
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( of_logic_dom, CTFLogicDOM );

BEGIN_DATADESC( CTFLogicDOM )
	//Keyfields
	DEFINE_KEYFIELD( m_nDomScore_limit, FIELD_INTEGER, "DomScoreLimit" ),
	DEFINE_KEYFIELD( m_nDomScore_time, FIELD_INTEGER, "DomScoreTime" ),
	DEFINE_KEYFIELD( m_bDomWinOnLimit, FIELD_BOOLEAN, "DomWinOnLimit" ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddDomScoreRed", InputAddDomScore_red ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddDomScoreBlue", InputAddDomScore_blue ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDomScoreRed", InputSetDomScore_red ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDomScoreBlue", InputSetDomScore_blue ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDomScoreLimit", InputSetDomScoreLimit ),

	DEFINE_OUTPUT( m_OnDomScoreHit_any, "OnScoreLimitHitAny" ),
	DEFINE_OUTPUT( m_OnDomScoreHit_red, "OnScoreLimitHitRed" ),
	DEFINE_OUTPUT( m_OnDomScoreHit_blue, "OnScoreLimitHitBlue" ),

	DEFINE_THINKFUNC( DOMThink ),
END_DATADESC()

void CTFLogicDOM::Spawn( void )
{
	if ( TFGameRules() )
	{
		TFGameRules()->m_nDomScore_limit = m_nDomScore_limit;
		TFGameRules()->m_nDomScore_time = m_nDomScore_time;
		TFGameRules()->m_bDomWinOnLimit = m_bDomWinOnLimit;
	}

	BaseClass::Spawn();

	SetThink( &CTFLogicDOM::DOMThink );
	SetNextThink( gpGlobals->curtime );
}

void CTFLogicDOM::DOMThink( void )
{
	if ( TFGameRules() && TFGameRules()->IsInWaitingForPlayers() )
	{
		SetNextThink( gpGlobals->curtime + m_nDomScore_time );
		return;
	}

	int nOwnedRed = 0;
	int nOwnedBlue = 0;

	// this sucks!
	CTeamControlPoint *pTeamControlPoint = (CTeamControlPoint *)gEntList.FindEntityByClassname( NULL, "team_control_point" );

	// find the score value of each of team-owned control points (usually just 1)
	while ( pTeamControlPoint )
	{
		if ( pTeamControlPoint->GetTeamNumber() == TF_TEAM_RED )
		{
			nOwnedRed = nOwnedRed + pTeamControlPoint->GetDomScoreAmount();
		}
		else if ( pTeamControlPoint->GetTeamNumber() == TF_TEAM_BLUE )
		{
			nOwnedBlue = nOwnedBlue + pTeamControlPoint->GetDomScoreAmount();
		}

		pTeamControlPoint = (CTeamControlPoint *)gEntList.FindEntityByClassname( pTeamControlPoint, "team_control_point" );
	}

	// add the total score we got from all counted and owned control points
	AddDomScore_red( nOwnedRed );
	AddDomScore_blue( nOwnedBlue );

	SetNextThink( gpGlobals->curtime + m_nDomScore_time );
}

void CTFLogicDOM::AddDomScore_red( int amount )
{
	if ( amount == 0 )
		return;

	int dom_score_red_old = TFGameRules()->m_nDomScore_red;
	int dom_score_blue_old = TFGameRules()->m_nDomScore_blue;

	// is the resulting score gonna be over the limit? clamp it
	if ( ( dom_score_red_old + amount ) > TFGameRules()->m_nDomScore_limit )
		TFGameRules()->m_nDomScore_red = TFGameRules()->m_nDomScore_limit;
	else
		TFGameRules()->m_nDomScore_red = TFGameRules()->m_nDomScore_red + amount;

	TFGameRules()->CheckDOMScores( dom_score_red_old, dom_score_blue_old );
}

void CTFLogicDOM::AddDomScore_blue( int amount )
{
	if ( amount == 0 )
		return;

	int dom_score_red_old = TFGameRules()->m_nDomScore_red;
	int dom_score_blue_old = TFGameRules()->m_nDomScore_blue;

	// is the resulting score gonna be over the limit? clamp it
	if ( ( dom_score_blue_old + amount ) > TFGameRules()->m_nDomScore_limit )
		TFGameRules()->m_nDomScore_blue = TFGameRules()->m_nDomScore_limit;
	else
		TFGameRules()->m_nDomScore_blue = TFGameRules()->m_nDomScore_blue + amount;

	TFGameRules()->CheckDOMScores( dom_score_red_old, dom_score_blue_old );
}

void CTFLogicDOM::SetDomScore_red( int amount )
{
	if ( amount == TFGameRules()->m_nDomScore_red )
		return;

	int dom_score_red_old = TFGameRules()->m_nDomScore_red;
	int dom_score_blue_old = TFGameRules()->m_nDomScore_blue;

	// is the resulting score gonna be over the limit? clamp it
	if ( amount > TFGameRules()->m_nDomScore_limit )
		TFGameRules()->m_nDomScore_red = TFGameRules()->m_nDomScore_limit;
	else
		TFGameRules()->m_nDomScore_red = amount;

	TFGameRules()->CheckDOMScores( dom_score_red_old, dom_score_blue_old );
}

void CTFLogicDOM::SetDomScore_blue( int amount )
{
	if ( amount == TFGameRules()->m_nDomScore_blue )
		return;

	int dom_score_red_old = TFGameRules()->m_nDomScore_red;
	int dom_score_blue_old = TFGameRules()->m_nDomScore_blue;

	// is the resulting score gonna be over the limit? clamp it
	if ( amount > TFGameRules()->m_nDomScore_limit )
		TFGameRules()->m_nDomScore_blue = TFGameRules()->m_nDomScore_limit;
	else
		TFGameRules()->m_nDomScore_blue = amount;

	TFGameRules()->CheckDOMScores( dom_score_red_old, dom_score_blue_old );
}

void CTFLogicDOM::SetDomScoreLimit( int amount )
{
	if ( amount == 0 )
		return;

	int dom_score_red_old = TFGameRules()->m_nDomScore_red;
	int dom_score_blue_old = TFGameRules()->m_nDomScore_blue;

	// clamp any scores if the limit becomes lower
	if ( TFGameRules()->m_nDomScore_red > amount )
		TFGameRules()->m_nDomScore_red = amount;
	else if ( TFGameRules()->m_nDomScore_blue > amount )
		TFGameRules()->m_nDomScore_blue = amount;

	TFGameRules()->m_nDomScore_limit = amount;

	TFGameRules()->CheckDOMScores( dom_score_red_old, dom_score_blue_old );
}

void CTFLogicDOM::InputAddDomScore_red( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		AddDomScore_red( inputdata.value.Int() );
	}
}

void CTFLogicDOM::InputAddDomScore_blue( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		AddDomScore_blue( inputdata.value.Int() );
	}
}

void CTFLogicDOM::InputSetDomScore_red( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		SetDomScore_red( inputdata.value.Int() );
	}
}

void CTFLogicDOM::InputSetDomScore_blue( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		SetDomScore_blue( inputdata.value.Int() );
	}
}

void CTFLogicDOM::InputSetDomScoreLimit( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		SetDomScoreLimit( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Coop Logic 
//-----------------------------------------------------------------------------

class CTFLogicCoop : public CBaseEntity
{
public:
	DECLARE_CLASS(CTFLogicCoop, CBaseEntity);
	void	Spawn(void);
};

LINK_ENTITY_TO_CLASS(of_logic_coop, CTFLogicCoop);

void CTFLogicCoop::Spawn(void)
{
	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Infection Logic 
//-----------------------------------------------------------------------------

class CTFLogicInf : public CBaseEntity
{
public:
	DECLARE_CLASS(CTFLogicInf, CBaseEntity);
	void	Spawn(void);
};

LINK_ENTITY_TO_CLASS(of_logic_inf, CTFLogicInf);

void CTFLogicInf::Spawn(void)
{
	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Arena Logic 
//-----------------------------------------------------------------------------

// inheriting from pointentity, not baseentity as the event inputs are required
class CTFLogicArena : public CPointEntity
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTFLogicArena, CPointEntity );

	void	Spawn( void );

	CTFLogicArena();

	float		m_iCapEnableDelay;

	bool	m_bCapUnlocked;
	void	OnCapEnabled( void );

	//awful
	virtual void	InputRoundActivate( inputdata_t &inputdata );

	COutputEvent m_ArenaRoundStart;
	COutputEvent m_OnCapEnabled;
};

BEGIN_DATADESC( CTFLogicArena )
	DEFINE_KEYFIELD( m_iCapEnableDelay, FIELD_FLOAT, "CapEnableDelay" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),

	DEFINE_OUTPUT( m_ArenaRoundStart, "OnArenaRoundStart" ),
	DEFINE_OUTPUT( m_OnCapEnabled, "OnCapEnabled" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS(tf_logic_arena, CTFLogicArena);

CTFLogicArena::CTFLogicArena()
{
	m_iCapEnableDelay = 60;

	m_bCapUnlocked = false;
}

void CTFLogicArena::Spawn(void)
{
	BaseClass::Spawn();

	if ( m_iCapEnableDelay == 0 )
		m_bCapUnlocked = true;
}

void CTFLogicArena::OnCapEnabled( void )
{
	if ( !m_bCapUnlocked )
	{
		m_bCapUnlocked = true; 
		// activator??
		m_OnCapEnabled.FireOutput( this, this );
	}
}

void CTFLogicArena::InputRoundActivate( inputdata_t &inputdata )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( !pMaster )
		return;

	for ( int i = 0; i < pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		// lock control point
		pPoint->SetLocked( true );
		pPoint->SetUnlockTime( m_iCapEnableDelay );
	}
}

//-----------------------------------------------------------------------------
// KOTH Logic 
//-----------------------------------------------------------------------------

// https://raw.githubusercontent.com/powerlord/tf2-data/master/datamaps.txt
/*
CBaseEntity - tf_logic_koth
- m_nTimerInitialLength (Offset 856) (Save|Key)(4 Bytes) - timer_length
- m_nTimeToUnlockPoint (Offset 860) (Save|Key)(4 Bytes) - unlock_point
- InputRoundSpawn (Offset 0) (Input)(0 Bytes) - RoundSpawn
- InputRoundActivate (Offset 0) (Input)(0 Bytes) - RoundActivate
- InputSetRedTimer (Offset 0) (Input)(0 Bytes) - SetRedTimer
- InputSetBlueTimer (Offset 0) (Input)(0 Bytes) - SetBlueTimer
- InputAddRedTimer (Offset 0) (Input)(0 Bytes) - AddRedTimer
- InputAddBlueTimer (Offset 0) (Input)(0 Bytes) - AddBlueTimer
*/

class CTFLogicKOTH : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFLogicKOTH, CBaseEntity );
	DECLARE_DATADESC();

	CTFLogicKOTH();

	// called at setup
	virtual void	InputRoundSpawn( inputdata_t &inputdata );

	virtual void	InputSetBlueTimer( inputdata_t &inputdata );
	virtual void	InputSetRedTimer( inputdata_t &inputdata );
	virtual void	InputAddBlueTimer( inputdata_t &inputdata );
	virtual void	InputAddRedTimer( inputdata_t &inputdata );

private:
	int m_nTimerInitialLength;
	int m_nTimeToUnlockPoint;

};

BEGIN_DATADESC( CTFLogicKOTH )

	DEFINE_KEYFIELD( m_nTimerInitialLength, FIELD_INTEGER, "timer_length" ),
	DEFINE_KEYFIELD( m_nTimeToUnlockPoint, FIELD_INTEGER, "unlock_point" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBlueTimer", InputSetBlueTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedTimer", InputSetRedTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddBlueTimer", InputAddBlueTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddRedTimer", InputAddRedTimer ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_koth, CTFLogicKOTH );

CTFLogicKOTH::CTFLogicKOTH()
{
	m_nTimerInitialLength = 180;
	m_nTimeToUnlockPoint = 30;
}

void CTFLogicKOTH::InputRoundSpawn(inputdata_t &inputdata)
{
	if ( TFGameRules() )
	{
		variant_t sVariant;

		sVariant.SetInt( TFGameRules()->GetTimeLeft() );

		TFGameRules()->SetBlueKothRoundTimer( ( CTeamRoundTimer* )CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );

		if ( TFGameRules()->GetBlueKothRoundTimer() )
		{
			TFGameRules()->GetBlueKothRoundTimer()->SetName( MAKE_STRING( "zz_blue_koth_timer" ) );

			TFGameRules()->GetBlueKothRoundTimer()->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			TFGameRules()->GetBlueKothRoundTimer()->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );

			TFGameRules()->GetBlueKothRoundTimer()->SetTimeRemaining( m_nTimerInitialLength ); 

			TFGameRules()->GetBlueKothRoundTimer()->SetShowInHud( false );

			TFGameRules()->GetBlueKothRoundTimer()->ChangeTeam( TF_TEAM_BLUE );
		}

		TFGameRules()->SetRedKothRoundTimer( ( CTeamRoundTimer* )CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );

		if ( TFGameRules()->GetRedKothRoundTimer() )
		{
			TFGameRules()->GetRedKothRoundTimer()->SetName( MAKE_STRING( "zz_red_koth_timer" ) );

			TFGameRules()->GetRedKothRoundTimer()->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			TFGameRules()->GetRedKothRoundTimer()->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );

			TFGameRules()->GetRedKothRoundTimer()->SetTimeRemaining( m_nTimerInitialLength );

			TFGameRules()->GetRedKothRoundTimer()->SetShowInHud( false );

			TFGameRules()->GetRedKothRoundTimer()->ChangeTeam( TF_TEAM_RED );
		}
	}
}

void CTFLogicKOTH::InputAddBlueTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetBlueKothRoundTimer() )
	{
		TFGameRules()->GetBlueKothRoundTimer()->AddTimerSeconds( inputdata.value.Int() );
	}
}

void CTFLogicKOTH::InputAddRedTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetRedKothRoundTimer() )
	{
		TFGameRules()->GetRedKothRoundTimer()->AddTimerSeconds( inputdata.value.Int() );
	}
}

void CTFLogicKOTH::InputSetBlueTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetBlueKothRoundTimer() )
	{
		TFGameRules()->GetBlueKothRoundTimer()->SetTimeRemaining( inputdata.value.Int() );
	}
}

void CTFLogicKOTH::InputSetRedTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetRedKothRoundTimer() )
	{
		TFGameRules()->GetRedKothRoundTimer()->SetTimeRemaining( inputdata.value.Int() );
	}
}

#endif

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

// (We clamp ammo ourselves elsewhere).
ConVar ammo_max( "ammo_max", "5000", FCVAR_REPLICATED );

ConVar  physcannon_mega_enabled( "physcannon_mega_enabled", "0", FCVAR_CHEAT | FCVAR_REPLICATED );

// Controls the application of the robus radius damage model.
ConVar	sv_robust_explosions( "sv_robust_explosions","1", FCVAR_REPLICATED );

// Damage scale for damage inflicted by the player on each skill level.
ConVar	sk_dmg_inflict_scale1( "sk_dmg_inflict_scale1", "1.50", FCVAR_REPLICATED );
ConVar	sk_dmg_inflict_scale2( "sk_dmg_inflict_scale2", "1.00", FCVAR_REPLICATED );
ConVar	sk_dmg_inflict_scale3( "sk_dmg_inflict_scale3", "0.75", FCVAR_REPLICATED );

// Damage scale for damage taken by the player on each skill level.
ConVar	sk_dmg_take_scale1( "sk_dmg_take_scale1", "0.50", FCVAR_REPLICATED );
ConVar	sk_dmg_take_scale2( "sk_dmg_take_scale2", "1.00", FCVAR_REPLICATED );
#ifdef HL2_EPISODIC
	ConVar	sk_dmg_take_scale3( "sk_dmg_take_scale3", "2.0", FCVAR_REPLICATED );
#else
	ConVar	sk_dmg_take_scale3( "sk_dmg_take_scale3", "1.50", FCVAR_REPLICATED );
#endif//HL2_EPISODIC

ConVar	sk_allow_autoaim( "sk_allow_autoaim", "1", FCVAR_REPLICATED | FCVAR_ARCHIVE_XBOX );

// Autoaim scale
ConVar	sk_autoaim_scale1( "sk_autoaim_scale1", "1.0", FCVAR_REPLICATED );
ConVar	sk_autoaim_scale2( "sk_autoaim_scale2", "1.0", FCVAR_REPLICATED );
//ConVar	sk_autoaim_scale3( "sk_autoaim_scale3", "0.0", FCVAR_REPLICATED ); NOT CURRENTLY OFFERED ON SKILL 3

// Quantity scale for ammo received by the player.
ConVar	sk_ammo_qty_scale1 ( "sk_ammo_qty_scale1", "1.20", FCVAR_REPLICATED );
ConVar	sk_ammo_qty_scale2 ( "sk_ammo_qty_scale2", "1.00", FCVAR_REPLICATED );
ConVar	sk_ammo_qty_scale3 ( "sk_ammo_qty_scale3", "0.60", FCVAR_REPLICATED );

ConVar	sk_plr_health_drop_time		( "sk_plr_health_drop_time", "30", FCVAR_REPLICATED );
ConVar	sk_plr_grenade_drop_time	( "sk_plr_grenade_drop_time", "30", FCVAR_REPLICATED );

ConVar	sk_plr_dmg_ar2			( "sk_plr_dmg_ar2","0", FCVAR_REPLICATED );
ConVar	sk_npc_dmg_ar2			( "sk_npc_dmg_ar2","0", FCVAR_REPLICATED);
ConVar	sk_max_ar2				( "sk_max_ar2","0", FCVAR_REPLICATED);
ConVar	sk_max_ar2_altfire		( "sk_max_ar2_altfire","0", FCVAR_REPLICATED);

ConVar	sk_plr_dmg_alyxgun		( "sk_plr_dmg_alyxgun","0", FCVAR_REPLICATED );
ConVar	sk_npc_dmg_alyxgun		( "sk_npc_dmg_alyxgun","0", FCVAR_REPLICATED);
ConVar	sk_max_alyxgun			( "sk_max_alyxgun","0", FCVAR_REPLICATED);

ConVar	sk_plr_dmg_pistol		( "sk_plr_dmg_pistol","0", FCVAR_REPLICATED );
ConVar	sk_npc_dmg_pistol		( "sk_npc_dmg_pistol","0", FCVAR_REPLICATED);
ConVar	sk_max_pistol			( "sk_max_pistol","0", FCVAR_REPLICATED);

ConVar	sk_plr_dmg_smg1			( "sk_plr_dmg_smg1","0", FCVAR_REPLICATED );
ConVar	sk_npc_dmg_smg1			( "sk_npc_dmg_smg1","0", FCVAR_REPLICATED);
ConVar	sk_max_smg1				( "sk_max_smg1","0", FCVAR_REPLICATED);

// FIXME: remove these
//ConVar	sk_plr_dmg_flare_round	( "sk_plr_dmg_flare_round","0", FCVAR_REPLICATED);
//ConVar	sk_npc_dmg_flare_round	( "sk_npc_dmg_flare_round","0", FCVAR_REPLICATED);
//ConVar	sk_max_flare_round		( "sk_max_flare_round","0", FCVAR_REPLICATED);

ConVar	sk_plr_dmg_buckshot		( "sk_plr_dmg_buckshot","0", FCVAR_REPLICATED);	
ConVar	sk_npc_dmg_buckshot		( "sk_npc_dmg_buckshot","0", FCVAR_REPLICATED);
ConVar	sk_max_buckshot			( "sk_max_buckshot","0", FCVAR_REPLICATED);
ConVar	sk_plr_num_shotgun_pellets( "sk_plr_num_shotgun_pellets","7", FCVAR_REPLICATED);

ConVar	sk_plr_dmg_rpg_round	( "sk_plr_dmg_rpg_round","0", FCVAR_REPLICATED);
ConVar	sk_npc_dmg_rpg_round	( "sk_npc_dmg_rpg_round","0", FCVAR_REPLICATED);
ConVar	sk_max_rpg_round		( "sk_max_rpg_round","0", FCVAR_REPLICATED);

ConVar	sk_plr_dmg_sniper_round	( "sk_plr_dmg_sniper_round","0", FCVAR_REPLICATED);	
ConVar	sk_npc_dmg_sniper_round	( "sk_npc_dmg_sniper_round","0", FCVAR_REPLICATED);
ConVar	sk_max_sniper_round		( "sk_max_sniper_round","0", FCVAR_REPLICATED);

//ConVar	sk_max_slam				( "sk_max_slam","0", FCVAR_REPLICATED);
//ConVar	sk_max_tripwire			( "sk_max_tripwire","0", FCVAR_REPLICATED);

//ConVar	sk_plr_dmg_molotov		( "sk_plr_dmg_molotov","0", FCVAR_REPLICATED);
//ConVar	sk_npc_dmg_molotov		( "sk_npc_dmg_molotov","0", FCVAR_REPLICATED);
//ConVar	sk_max_molotov			( "sk_max_molotov","0", FCVAR_REPLICATED);

ConVar	sk_plr_dmg_grenade		( "sk_plr_dmg_grenade","0", FCVAR_REPLICATED);
ConVar	sk_npc_dmg_grenade		( "sk_npc_dmg_grenade","0", FCVAR_REPLICATED);
ConVar	sk_max_grenade			( "sk_max_grenade","0", FCVAR_REPLICATED);

#ifdef HL2_EPISODIC
ConVar	sk_max_hopwire			( "sk_max_hopwire", "3", FCVAR_REPLICATED);
ConVar	sk_max_striderbuster	( "sk_max_striderbuster", "3", FCVAR_REPLICATED);
#endif

//ConVar sk_plr_dmg_brickbat	( "sk_plr_dmg_brickbat","0", FCVAR_REPLICATED);
//ConVar sk_npc_dmg_brickbat	( "sk_npc_dmg_brickbat","0", FCVAR_REPLICATED);
//ConVar sk_max_brickbat		( "sk_max_brickbat","0", FCVAR_REPLICATED);

ConVar	sk_plr_dmg_smg1_grenade	( "sk_plr_dmg_smg1_grenade","0", FCVAR_REPLICATED);
ConVar	sk_npc_dmg_smg1_grenade	( "sk_npc_dmg_smg1_grenade","0", FCVAR_REPLICATED);
ConVar	sk_max_smg1_grenade		( "sk_max_smg1_grenade","0", FCVAR_REPLICATED );

ConVar	sk_plr_dmg_357			( "sk_plr_dmg_357", "0", FCVAR_REPLICATED );
ConVar	sk_npc_dmg_357			( "sk_npc_dmg_357", "0", FCVAR_REPLICATED );
ConVar	sk_max_357				( "sk_max_357", "0", FCVAR_REPLICATED );

ConVar	sk_plr_dmg_crossbow		( "sk_plr_dmg_crossbow", "0", FCVAR_REPLICATED );
ConVar	sk_npc_dmg_crossbow		( "sk_npc_dmg_crossbow", "0", FCVAR_REPLICATED );
ConVar	sk_max_crossbow			( "sk_max_crossbow", "0", FCVAR_REPLICATED );

ConVar	sk_dmg_sniper_penetrate_plr( "sk_dmg_sniper_penetrate_plr","0", FCVAR_REPLICATED);
ConVar	sk_dmg_sniper_penetrate_npc( "sk_dmg_sniper_penetrate_npc","0", FCVAR_REPLICATED);

ConVar	sk_plr_dmg_airboat		( "sk_plr_dmg_airboat", "0", FCVAR_REPLICATED );
ConVar	sk_npc_dmg_airboat		( "sk_npc_dmg_airboat", "0", FCVAR_REPLICATED );

ConVar	sk_max_gauss_round		( "sk_max_gauss_round", "0", FCVAR_REPLICATED );

// Gunship & Dropship cannons
ConVar	sk_npc_dmg_gunship			( "sk_npc_dmg_gunship", "0", FCVAR_REPLICATED );
ConVar	sk_npc_dmg_gunship_to_plr	( "sk_npc_dmg_gunship_to_plr", "0", FCVAR_REPLICATED );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_IsTimeBased( int iDmgType )
{
	// Damage types that are time-based.
	return ( ( iDmgType & ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_ShowOnHUD( int iDmgType )
{
	// Damage types that have client HUD art.
	return ( ( iDmgType & ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK ) ) != 0 );
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_ShouldNotBleed( int iDmgType )
{
	// Should always bleed currently.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::Damage_GetTimeBased( void )
{
	int iDamage = ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::Damage_GetShowOnHud( void )
{
	int iDamage = ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFGameRules::Damage_GetShouldNotBleed( void )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGameRules::CTFGameRules()
{
#ifdef GAME_DLL
	// Create teams.
	TFTeamMgr()->Init();

	ResetMapTime();

	// Create the team managers
//	for ( int i = 0; i < ARRAYSIZE( teamnames ); i++ )
//	{
//		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "tf_team" ));
//		pTeam->Init( sTeamNames[i], i );
//
//		g_Teams.AddToTail( pTeam );
//	}

	// Reset Domination stuff
	m_nDomScore_red = 0;
	m_nDomScore_blue = 0;
	m_bDomRedThreshold = false;
	m_bDomBlueThreshold = false;
	m_bDomRedLeadThreshold = false;
	m_bDomBlueLeadThreshold = false;
	m_bStartedVote = false;

	m_flIntermissionEndTime = 0.0f;
	m_flNextPeriodicThink = 0.0f;

	if ( !Q_strncmp(STRING(gpGlobals->mapname), "d1_", 3) || !Q_strncmp(STRING(gpGlobals->mapname), "d2_", 3) || !Q_strncmp(STRING(gpGlobals->mapname), "d3_", 3) )
		m_bIsHL2 = true;

	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_capture_blocked" );	
	ListenForGameEvent( "teamplay_point_unlocked" );
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_flag_event" );
	
	Q_memset( m_vecPlayerPositions,0, sizeof(m_vecPlayerPositions) );

	m_iPrevRoundState = -1;
	m_iCurrentRoundState = -1;
	m_iCurrentMiniRoundMask = 0;
	m_flTimerMayExpireAt = -1.0f;

	// Lets execute a map specific cfg file
	// ** execute this after server.cfg!
	char szCommand[32];
	Q_snprintf( szCommand, sizeof( szCommand ), "exec %s.cfg\n", STRING( gpGlobals->mapname ) );
	engine->ServerCommand( szCommand );

#else // GAME_DLL

	ListenForGameEvent( "game_newmap" );
	
#endif

	// Initialize the game type
//	m_nGameType.Set( TF_GAMETYPE_UNDEFINED );

	// Initialize the classes here.
	InitPlayerClasses();

	// Set turbo physics on.  Do it here for now.
	sv_turbophysics.SetValue( 0 );

	// Initialize the team manager here, etc...

	// If you hit these asserts its because you added or removed a weapon type 
	// and didn't also add or remove the weapon name or damage type from the
	// arrays defined in tf_shareddefs.cpp
	Assert( g_aWeaponDamageTypes[TF_WEAPON_COUNT] == TF_DMG_SENTINEL_VALUE );
	Assert( FStrEq( g_aWeaponNames[TF_WEAPON_COUNT], "TF_WEAPON_COUNT" ) );	

	m_iPreviousRoundWinners = TEAM_UNASSIGNED;
	m_iBirthdayMode = BIRTHDAY_RECALCULATE;

	m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
	m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';
	m_pszTeamGoalStringMercenary.GetForModify()[0] = '\0';

	m_flLastHealthDropTime = 0.0f;
	m_flLastGrenadeDropTime = 0.0f;	
	
	for( int i = 0; i<50 ; i++ )
	{
		m_iszWeaponName[i] = m_iszDefaultWeaponName[i];
	}
	m_iMaxLevel = 14;
	m_iRequiredKills = 2;
}

bool CTFGameRules::InGametype( int nGametype )
{	
	Assert( nGametype >= 0 && nGametype < TF_GAMETYPE_LAST );
	return ( ( m_nGameType & (1<<nGametype) ) != 0 );
}

void CTFGameRules::AddGametype( int nGametype )
{
	Assert( nGametype >= 0 && nGametype < TF_GAMETYPE_LAST );
	m_nGameType |= (1<<nGametype);
}

void CTFGameRules::RemoveGametype( int nGametype )
{
	Assert( nGametype >= 0 && nGametype < TF_GAMETYPE_LAST );
	m_nGameType &= ~(1<<nGametype);
}

bool CTFGameRules::IsMutator( int nMutator )
{
	return ( m_nMutator == nMutator );
}

int CTFGameRules::GetMutator( void )
{
	return m_nMutator;
}

#ifdef GAME_DLL
void CTFGameRules::SetMutator( int nMutator )
{
	m_nMutator = nMutator;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::FlagsMayBeCapped( void )
{
	if ( State_Get() != GR_STATE_TEAM_WIN )
		return true;

	return false;
}

bool CTFGameRules::WeaponSpawnersMayBeUsed( void )
{
	if ( State_Get() != GR_STATE_TEAM_WIN )
		return true;

	return false;
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Determines whether we should allow mp_timelimit to trigger a map change
//-----------------------------------------------------------------------------
bool CTFGameRules::CanChangelevelBecauseOfTimeLimit( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	// we only want to deny a map change triggered by mp_timelimit if we're not forcing a map reset,
	// we're playing mini-rounds, and the master says we need to play all of them before changing (for maps like Dustbowl)
	if ( !m_bForceMapReset && pMaster && pMaster->PlayingMiniRounds() && pMaster->ShouldPlayAllControlPointRounds() )
	{
		if ( pMaster->NumPlayableControlPointRounds() )
		{
			return false;
		}
	}

	return true;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanGoToStalemate( void )
{
	// In CTF, don't go to stalemate if one of the flags isn't at home
	if ( m_nGameType == TF_GAMETYPE_CTF )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*> ( gEntList.FindEntityByClassname( NULL, "item_teamflag" ) );
		while( pFlag )
		{
			if ( pFlag->IsDropped() || pFlag->IsStolen() )
				return false;

			pFlag = dynamic_cast<CCaptureFlag*> ( gEntList.FindEntityByClassname( pFlag, "item_teamflag" ) );
		}

		// check that one team hasn't won by capping
		if ( CheckCapsPerRound() )
			return false;
	}

	return BaseClass::CanGoToStalemate();
}

// Classnames of entities that are preserved across round restarts
// Updated to modern TF2
static const char *s_PreserveEnts[] =
{
	"tf_gamerules",
	"tf_team_manager",
	"tf_player_manager",
	"tf_team",
	"tf_objective_resource",
	"keyframe_rope",
	"move_rope",
	"tf_",
	"tf_logic_training",
	"tf_logic_training_mode",
	"tf_powerup_bottle",
	"tf_mann_vs_machine_stats",
	"tf_viewmodel"
	"tf_wearable",
	"tf_wearable_demoshield",
	"tf_wearable_robot_arm",
	"tf_wearable_vm",
	"tf_logic_bonusround",
	"vote_controller",
	"monster_resource",
	"tf_logic_medieval",
	"tf_logic_cp_timer",
	"tf_logic_tower_defense",
	"func_upgradestation",
	"entity_rocket",
	"entity_carrier",
	"entity_sign",
	"entity_saucer",
	"tf_halloween_gift_pickup",
	"tf_logic_competitive",
	"tf_wearable_razorback",
	"", // END Marker
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Activate()
{
	SetupMutator();

	ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server global gamemode config file\n");
	engine->ServerCommand("exec config_default_global.cfg\n");
	engine->ServerExecute();

	m_iBirthdayMode = BIRTHDAY_RECALCULATE;

	m_nCurrFrags.Set(0);
	m_bDisableRedSpawns = false;
	m_bDisableBluSpawns = false;
//	m_nGameType.Set(TF_GAMETYPE_UNDEFINED);
	CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*> (gEntList.FindEntityByClassname(NULL, "item_teamflag"));
	if (pFlag)
	{
		AddGametype(TF_GAMETYPE_CTF);
	}
	CTeamTrainWatcher *pTrain = dynamic_cast<CTeamTrainWatcher*> (gEntList.FindEntityByClassname(NULL, "team_train_watcher"));

	if (pTrain)
	{
		AddGametype(TF_GAMETYPE_PAYLOAD);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server Payload gamemode config file\n");
		engine->ServerCommand("exec config_default_pl.cfg \n");
		engine->ServerExecute();
	}
	
	if (g_hControlPointMasters.Count())
	{
		AddGametype(TF_GAMETYPE_CP);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server CP gamemode config file\n");
		engine->ServerCommand("exec config_default_cp.cfg \n");
		engine->ServerExecute();
	}

	if (gEntList.FindEntityByClassname(NULL, "tf_logic_arena") || !Q_strncmp(STRING(gpGlobals->mapname), "arena_", 6) || of_arena.GetBool() )
	{
		AddGametype(TF_GAMETYPE_ARENA);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server Arena gamemode config file\n");
		engine->ServerCommand("exec config_default_arena.cfg \n");
		engine->ServerExecute();
	}

	if (gEntList.FindEntityByClassname(NULL, "tf_logic_koth") || !Q_strncmp(STRING(gpGlobals->mapname), "koth_", 5) )
	{
		AddGametype(TF_GAMETYPE_CP);
		m_bKOTH = true;
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server KOTH gamemode config file\n");
		engine->ServerCommand("exec config_default_koth.cfg \n");
		engine->ServerExecute();
	}

	if (gEntList.FindEntityByClassname(NULL, "of_logic_dm") || !Q_strncmp(STRING(gpGlobals->mapname), "dm_", 3) )
	{
		AddGametype(TF_GAMETYPE_DM);
		if ( ((( teamplay.GetInt() < 0 || gEntList.FindEntityByClassname(NULL, "of_logic_tdm")) && m_bIsTeamplay ) || teamplay.GetInt() > 0 )  )
		{
			AddGametype(TF_GAMETYPE_TDM);
			ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server TDM gamemode config file\n");
			engine->ServerCommand("exec config_default_tdm.cfg \n");
			engine->ServerExecute();
		}
		else 
		{
			ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server DM gamemode config file\n");
			engine->ServerCommand("exec config_default_dm.cfg \n");
			engine->ServerExecute();
		}
	}
	
	if ( ( gEntList.FindEntityByClassname(NULL, "of_logic_gg") && !m_bListOnly ) || !Q_strncmp(STRING(gpGlobals->mapname), "gg_", 3) || TFGameRules()->IsMutator( GUN_GAME ) )
	{
		AddGametype(TF_GAMETYPE_GG);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server GG gamemode config file\n");
		engine->ServerCommand("exec config_default_gg.cfg \n");
		engine->ServerExecute();
	}	
	
	if ( ( gEntList.FindEntityByClassname(NULL, "of_logic_3wave") && !m_bListOnly ) || of_threewave.GetInt() == 1 )
	{
		AddGametype(TF_GAMETYPE_3WAVE);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server Threewave gamemode config file\n");
		engine->ServerCommand("exec config_default_3wave.cfg \n");
		engine->ServerExecute();
	}
	
	if (gEntList.FindEntityByClassname(NULL, "of_logic_esc") || !Q_strncmp(STRING(gpGlobals->mapname), "esc_", 4) || ( of_payload_override.GetBool() && InGametype( TF_GAMETYPE_PAYLOAD ) ) )
	{
		AddGametype(TF_GAMETYPE_ESC);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server Escort gamemode config file\n");
		engine->ServerCommand("exec config_default_esc.cfg \n");
		engine->ServerExecute();
		
		if ( of_payload_override.GetBool() && !( gEntList.FindEntityByClassname(NULL, "of_logic_esc") || !Q_strncmp(STRING(gpGlobals->mapname), "esc_", 4) ) ) // We're replacing payload with Escort
		{
			m_bEscortOverride = true;
		}
	}

	if (gEntList.FindEntityByClassname(NULL, "of_logic_dom") || !Q_strncmp(STRING(gpGlobals->mapname), "dom_", 4) )
	{
		// no Domination gamemode is set in Escort
		if ( !IsESCGamemode() )
			AddGametype( TF_GAMETYPE_DOM );

		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server Domination gamemode config file\n");
		engine->ServerCommand("exec config_default_dom.cfg \n");
		engine->ServerExecute();
	}

	// this is for a future zombie survival gamemode
	if (gEntList.FindEntityByClassname(NULL, "of_logic_coop") || !Q_strncmp(STRING(gpGlobals->mapname), "zm_", 3) || of_coop.GetBool() )
	{
		AddGametype(TF_GAMETYPE_COOP);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server Coop gamemode config file\n");
		engine->ServerCommand("exec config_default_coop.cfg \n");
		engine->ServerExecute();
	}
	
	// HL2 Deathmatch
	if ( IsHL2() )
	{
		AddGametype(TF_GAMETYPE_COOP);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server HL2 gamemode config file\n");
		engine->ServerCommand("exec config_default_hl2.cfg \n");
		engine->ServerExecute();

		AddGametype(TF_GAMETYPE_DM);
	}

	// Infection
	if ( gEntList.FindEntityByClassname(NULL, "of_logic_inf") || !Q_strncmp(STRING(gpGlobals->mapname), "inf_", 4) || of_infection.GetBool() )
	{
		// incompatible gametypes with infection!
		if ( InGametype( TF_GAMETYPE_CTF ) )
			RemoveGametype( TF_GAMETYPE_CTF );
		if ( InGametype( TF_GAMETYPE_CP ) )
			RemoveGametype( TF_GAMETYPE_CP );
		if ( InGametype( TF_GAMETYPE_DM ) )
			RemoveGametype( TF_GAMETYPE_DM );
		if ( InGametype( TF_GAMETYPE_TDM ) )
			RemoveGametype( TF_GAMETYPE_TDM );
		if ( InGametype( TF_GAMETYPE_ESC ) )
			RemoveGametype( TF_GAMETYPE_ESC );
		if ( InGametype( TF_GAMETYPE_COOP ) )
			RemoveGametype( TF_GAMETYPE_COOP );
		if ( InGametype( TF_GAMETYPE_DOM ) )
			RemoveGametype( TF_GAMETYPE_DOM );
		if ( InGametype( TF_GAMETYPE_DOM ) )
			RemoveGametype( TF_GAMETYPE_DOM );
		if ( InGametype( TF_GAMETYPE_PAYLOAD ) )
			RemoveGametype( TF_GAMETYPE_PAYLOAD );

		AddGametype(TF_GAMETYPE_INF);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server Infection gamemode config file\n");
		engine->ServerCommand("exec config_default_inf.cfg \n");
		engine->ServerExecute();
	}
	
	CheckTDM();
}

#endif

void CTFGameRules::CheckTDM( void )
{
	for( int i = TF_GAMETYPE_UNDEFINED + 1; i < TF_GAMETYPE_LAST; i++ )
	{
		if ( i == TF_GAMETYPE_DM || i == TF_GAMETYPE_TDM )
			continue;
		if ( InGametype ( i ) )
		{
			m_bIsTDM = false;
			return;
		}
	}
	m_bIsTDM = InGametype( TF_GAMETYPE_DM ) && InGametype( TF_GAMETYPE_TDM );
}

bool CTFGameRules::IsDMGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_DM ); 
}

bool CTFGameRules::IsTDMGamemode( void )
{ 
	return m_bIsTDM;
}

bool CTFGameRules::IsDOMGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_DOM ); 
}

bool CTFGameRules::IsTeamplay( void )
{ 
	return InGametype( TF_GAMETYPE_TDM );
}

bool CTFGameRules::DontCountKills( void )
{ 
	return m_nbDontCountKills || IsGGGamemode(); 
}

bool CTFGameRules::IsGGGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_GG );
}

bool CTFGameRules::Is3WaveGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_3WAVE );
}

bool CTFGameRules::IsArenaGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_ARENA );
}

bool CTFGameRules::IsESCGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_ESC );
}

bool CTFGameRules::IsCoopGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_COOP );
}

bool CTFGameRules::IsZSGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_ZS );
}

bool CTFGameRules::IsInfGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_INF );
}

bool CTFGameRules::IsPayloadOverride( void )
{ 
	return m_bEscortOverride;
}

bool CTFGameRules::IsHL2( void )
{ 
	return m_bIsHL2;
}

bool CTFGameRules::UsesMoney( void )
{ 
	return m_bUsesMoney;
}
#ifdef GAME_DLL

void CTFGameRules::FireGamemodeOutputs()
{
#ifdef GAME_DLL
	CTFGameRulesProxy *pProxy = dynamic_cast<CTFGameRulesProxy*> (gEntList.FindEntityByClassname(NULL, "tf_gamerules"));
	if ( pProxy )
	{
		if ( InGametype( TF_GAMETYPE_CTF ) )
				pProxy->FireCTFOutput();
		if ( InGametype( TF_GAMETYPE_CP ) )
				pProxy->FireCPOutput();
		if ( InGametype( TF_GAMETYPE_DM ) )
			pProxy->FireDMOutput();
		if ( IsTeamplay() )
			pProxy->FireTeamplayOutput();
		if ( InGametype( TF_GAMETYPE_GG ) )
			pProxy->FireGunGameOutput();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	bool bRetVal = true;

	if ( ( State_Get() == GR_STATE_TEAM_WIN ) && pVictim )
	{
		if ( pVictim->GetTeamNumber() == GetWinningTeam() )
		{
			CBaseTrigger *pTrigger = dynamic_cast< CBaseTrigger *>( info.GetInflictor() );

			// we don't want players on the winning team to be
			// hurt by team-specific trigger_hurt entities during the bonus time
			if ( pTrigger && pTrigger->UsesFilter() )
			{
				bRetVal = false;
			}
		}
	}

	return bRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetTeamGoalString( int iTeam, const char *pszGoal )
{
	if ( iTeam == TF_TEAM_RED )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringRed.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringRed.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
	else if ( iTeam == TF_TEAM_BLUE )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringBlue.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringBlue.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
	else if ( iTeam == TF_TEAM_MERCENARY )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringMercenary.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringMercenary.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringMercenary.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::RoundCleanupShouldIgnore( CBaseEntity *pEnt )
{
	if ( FindInList( s_PreserveEnts, pEnt->GetClassname() ) )
		return true;

	//There has got to be a better way of doing this.
	if ( Q_strstr( pEnt->GetClassname(), "tf_weapon_" ) )
		return true;

	return BaseClass::RoundCleanupShouldIgnore( pEnt );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldCreateEntity( const char *pszClassName )
{
	if ( FindInList( s_PreserveEnts, pszClassName ) )
		return false;

	return BaseClass::ShouldCreateEntity( pszClassName );
}

void CTFGameRules::CleanUpMap( void )
{
	BaseClass::CleanUpMap();

	if ( HLTVDirector() )
	{
		HLTVDirector()->BuildCameraList();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------ob
void CTFGameRules::RecalculateControlPointState( void )
{
	Assert( ObjectiveResource() );

	if ( !g_hControlPointMasters.Count() )
		return;

	if ( g_pObjectiveResource && g_pObjectiveResource->PlayingMiniRounds() )
		return;

	for ( int iTeam = LAST_SHARED_TEAM+1; iTeam < GetNumberOfTeams(); iTeam++ )
	{
		int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, true );
		if ( iFarthestPoint == -1 )
			continue;

		// Now enable all spawn points for that spawn point
		CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		while( pSpot )
		{
			CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn*>(pSpot);
			if ( pTFSpawn->GetControlPoint() )
			{
				if ( pTFSpawn->GetTeamNumber() == iTeam )
				{
					if ( pTFSpawn->GetControlPoint()->GetPointIndex() == iFarthestPoint )
					{
						pTFSpawn->SetDisabled( false );
					}
					else
					{
						pTFSpawn->SetDisabled( true );
					}
				}
			}

			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is being initialized
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnRoundStart( void )
{
	SetupMutator();

	// fixes a dumb crash, see ai_pathfinder.cpp line 607
	CAI_DynamicLink::gm_bInitialized = false;
	CAI_DynamicLink::InitDynamicLinks();

	FireGamemodeOutputs();

	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		ObjectiveResource()->SetBaseCP( -1, i );
	}

	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_iNumCaps[i] = 0;
	}

	// Let all entities know that a new round is starting
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundSpawn", NULL, NULL, emptyVariant, 0 );

		pEnt = gEntList.NextEnt( pEnt );
	}

	// All entities have been spawned, now activate them
	pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundActivate", NULL, NULL, emptyVariant, 0 );

		pEnt = gEntList.NextEnt( pEnt );
	}

	if ( g_pObjectiveResource && !g_pObjectiveResource->PlayingMiniRounds() )
	{
		// Find all the control points with associated spawnpoints
		memset( m_bControlSpawnsPerTeam, 0, sizeof(bool) * MAX_TEAMS * MAX_CONTROL_POINTS );
		CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		while( pSpot )
		{
			CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn*>(pSpot);
			if ( pTFSpawn->GetControlPoint() )
			{
				m_bControlSpawnsPerTeam[ pTFSpawn->GetTeamNumber() ][ pTFSpawn->GetControlPoint()->GetPointIndex() ] = true;
				pTFSpawn->SetDisabled( true );
			}

			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
		}

		RecalculateControlPointState();

		SetRoundOverlayDetails();
	}

	m_nDomScore_limit = 100;
	m_nDomScore_time = 3;
	m_nDomScore_red = 0;
	m_nDomScore_blue = 0;
	m_bDomWinOnLimit = true;
	m_bDomRedThreshold = false;
	m_bDomBlueThreshold = false;
	m_bDomRedLeadThreshold = false;
	m_bDomBlueLeadThreshold = false;
	
	if ( IsArenaGamemode() )
	{
		m_flStalemateStartTime = gpGlobals->curtime;
	}
#ifdef GAME_DLL
	m_szMostRecentCappers[0] = 0;
#endif

	if ( TFGameRules()->IsInfGamemode() )
	{
		// put everyone into RED again
		CTFPlayer *pPlayer = NULL;
		int i;

		for ( i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( pPlayer && pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			{
				pPlayer->m_Shared.SetZombie( false );
				pPlayer->ChangeTeam( TF_TEAM_RED, true );
				if ( of_forceclass.GetBool() == 1 ) 
					pPlayer->SetDesiredPlayerClassIndex( TF_CLASS_MERCENARY );
			}
		}

		// remove the timer
		if ( GetInfectionRoundTimer() )
			UTIL_Remove( GetInfectionRoundTimer() );

		// HORRIBLE HORRIBLE HACK TO FIX SPAWN ROOMS ON 2FORT
		if ( !Q_strncmp( STRING( gpGlobals->mapname), "ctf_2fort", 9 ) )
		{
			CBaseEntity *pEntity = NULL;
			while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "func_door" ) ) != NULL )
			{
				UTIL_Remove( pEntity );
			}
		}

		// no more visualizers, or respawn rooms, or regenerate lockers
		CBaseEntity *pEntity = NULL;
		while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "func_respawnroomvisualizer" ) ) != NULL )
			UTIL_Remove( pEntity );
		while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "func_respawnroom" ) ) != NULL )
			UTIL_Remove( pEntity );
		while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "func_regenerate" ) ) != NULL )
			UTIL_Remove( pEntity );

		if ( !IsInWaitingForPlayers() )
		{
			// wow
			variant_t sVariant;
			sVariant.SetInt( 0 );

			// create the timer
			TFGameRules()->SetInfectionRoundTimer( ( CTeamRoundTimer* )CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );

			if ( TFGameRules()->GetInfectionRoundTimer() )
			{
				TFGameRules()->GetInfectionRoundTimer()->SetName( MAKE_STRING( "zz_infection_timer" ) );
				TFGameRules()->GetInfectionRoundTimer()->SetInfectionBeginning( true );
				TFGameRules()->GetInfectionRoundTimer()->SetTimeRemaining( of_infection_preparetime.GetInt() ); 
				TFGameRules()->GetInfectionRoundTimer()->SetShowInHud( true );
				TFGameRules()->GetInfectionRoundTimer()->ChangeTeam( TF_TEAM_RED );

				TFGameRules()->GetInfectionRoundTimer()->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
				TFGameRules()->GetInfectionRoundTimer()->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
			}

			// all teams
			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				BroadcastSound( i, "InfectionMusic.Warmup", false );
			}
		}
	}
}

// Ran at the start of every round, simply setups and performs anything necessary for mutators
void CTFGameRules::SetupMutator( void )
{
	SetMutator ( of_mutator.GetInt() );

	if ( TFGameRules()->IsMutator( NO_MUTATOR ) )
	{
		// gungame shouldnt be a gametype...
		if ( InGametype(TF_GAMETYPE_GG) )
			RemoveGametype(TF_GAMETYPE_GG );

		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server DISABLED mutator config file\n");
		engine->ServerCommand("exec config_default_mutator_disabled.cfg \n");
		engine->ServerExecute();
	}	
	else if ( TFGameRules()->IsMutator( INSTAGIB ) )
	{
		// gungame shouldnt be a gametype...
		if ( InGametype(TF_GAMETYPE_GG) )
			RemoveGametype(TF_GAMETYPE_GG );

		ConColorMsg(Color(123, 176, 130, 255), "[TFGameRules] Executing server Instagib mutator config file\n");
		engine->ServerCommand("exec config_default_mutator_instagib.cfg \n");
		engine->ServerExecute();
	}	
	else if ( TFGameRules()->IsMutator( INSTAGIB_NO_MELEE ) )
	{
		// gungame shouldnt be a gametype...
		if ( InGametype(TF_GAMETYPE_GG) )
			RemoveGametype(TF_GAMETYPE_GG );

		ConColorMsg(Color(123, 176, 130, 255), "[TFGameRules] Executing server Instagib (no melee) mutator config file\n");
		engine->ServerCommand("exec config_default_mutator_instagibnomelee.cfg \n");
		engine->ServerExecute();
	}	
	else if ( TFGameRules()->IsMutator( CLAN_ARENA ) )
	{
		// gungame shouldnt be a gametype...
		if ( InGametype(TF_GAMETYPE_GG) )
			RemoveGametype(TF_GAMETYPE_GG );

		ConColorMsg(Color(123, 176, 130, 255), "[TFGameRules] Executing server Clan Arena mutator config file\n");
		engine->ServerCommand("exec config_default_mutator_clanarena.cfg \n");
		engine->ServerExecute();
	}	
	else if ( TFGameRules()->IsMutator( UNHOLY_TRINITY ) )
	{
		// gungame shouldnt be a gametype...
		if ( InGametype(TF_GAMETYPE_GG) )
			RemoveGametype(TF_GAMETYPE_GG );

		ConColorMsg(Color(123, 176, 130, 255), "[TFGameRules] Executing server Unholy Trinity mutator config file\n");
		engine->ServerCommand("exec config_default_mutator_unholytrinity.cfg \n");
		engine->ServerExecute();
	}	
	else if ( TFGameRules()->IsMutator( ROCKET_ARENA ) )
	{
		// gungame shouldnt be a gametype...
		if ( InGametype(TF_GAMETYPE_GG) )
			RemoveGametype(TF_GAMETYPE_GG );

		ConColorMsg(Color(123, 176, 130, 255), "[TFGameRules] Executing server Rocket Arena mutator config file\n");
		engine->ServerCommand("exec config_default_mutator_rocketarena.cfg \n");
		engine->ServerExecute();
	}	
	else if ( TFGameRules()->IsMutator( GUN_GAME ) )
	{
		// gungame shouldnt be a gametype...
		if ( !InGametype(TF_GAMETYPE_GG) )
			AddGametype(TF_GAMETYPE_GG );

		ConColorMsg(Color(123, 176, 130, 255), "[TFGameRules] Executing server Gun Game mutator config file\n");
		engine->ServerCommand("exec config_default_mutator_gungame.cfg \n");
		engine->ServerExecute();
	}
	else if ( TFGameRules()->IsMutator( RANDOMIZER ) )
	{
		// gungame shouldnt be a gametype...
		if ( InGametype(TF_GAMETYPE_GG) )
			RemoveGametype(TF_GAMETYPE_GG );

		ConColorMsg(Color(123, 176, 130, 255), "[TFGameRules] Executing server Randomizer mutator config file\n");
		engine->ServerCommand("exec config_default_mutator_randomizer.cfg \n");
		engine->ServerExecute();
	}
}

void CTFGameRules::PassAllTracks( void )
{
	// At here we trigger all but the last 2 Track Trains
	// We do this because some maps have doors that only open when the train passes
	// or completley block off areas
	for ( int i = 0; i < m_hTracksToPass.Count(); i++ )
	{
		if( m_hTracksToPass[i] ) 
			m_hTracksToPass[i]->Pass( NULL ); 
	}
	for ( int i = 0; i < m_hTracksToPass.Count(); i++ )
	{
		if( m_hTracksToPass[i] ) 
			m_hTracksToPass.Remove(i);
	}
	if ( m_bDisableRedSpawns )
		DisableSpawns( TF_TEAM_RED );
	if ( m_bDisableBluSpawns )
		DisableSpawns( TF_TEAM_BLUE );
}

void CTFGameRules::KeepTeamSpawns( int iTeamNumber )
{
	switch ( iTeamNumber )
	{
		case TF_TEAM_RED:
			m_bDisableRedSpawns = true;
		break;
		case TF_TEAM_BLUE:
			m_bDisableBluSpawns = true;
		break;
	}
}

void CTFGameRules::DisableSpawns( int iTeamNumber )
{
	const char *pEntClassName = "info_player_teamspawn";
	CBaseEntity *pSpot;
	// Get an initial spawn point.
	pSpot = gEntList.FindEntityByClassname( NULL, pEntClassName );
	if ( !pSpot )
	{
		// Sometimes the first spot can be NULL????
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}
	// First we try to find a spawn point that is fully clear. If that fails,
	// we look for a spawnpoint that's clear except for another players. We
	// don't collide with our team members, so we should be fine.
	CBaseEntity *pFirstSpot = pSpot;
	do{
		if ( pSpot )
		{
			CTFTeamSpawn *pCTFSpawn = dynamic_cast<CTFTeamSpawn*>( pSpot );
			// don't run validity checks on a info_player_start, as spawns in singleplayer maps should always be valid
			if ( !FStrEq( STRING( pSpot->m_iClassname ), "info_player_start" ) )
			{
				bool bSpotCreated = false;
				// Check to see if this is a valid team spawn (player is on this team, etc.).
				if( pSpot->GetTeamNumber() == iTeamNumber )
				{
					// Check for a bad spawn entity.
					if ( pSpot->GetAbsOrigin() != Vector( 0, 0, 0 ) )
					{
						bool bKill = true;
						for ( int i = 0; i < m_hReEnableSpawns.Count(); i++ )
						{
							if ( m_hReEnableSpawns[i] == pCTFSpawn )
							{
								bKill = false;
							}
						}
						if ( bKill )
						{
							// Found a valid spawn point.
							pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
							bSpotCreated = true;
							inputdata_t Temp;
							UTIL_Remove( pCTFSpawn );//->InputDisable( Temp );
						}
					}
				}
				if ( !bSpotCreated )
					pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
				if ( pSpot != pFirstSpot )
				{
					continue;
				}
				else
					break;
			}
		}
		else
			break;
	}
		// Continue until a valid spawn point is found or we hit the start.
	while ( pSpot != pFirstSpot );
} 

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is off and running
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnRoundRunning( void )
{
	CTFLogicArena *pArena = dynamic_cast< CTFLogicArena* > ( gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );

	if ( pArena )
		pArena->m_ArenaRoundStart.FireOutput( NULL, pArena );

	if ( IsArenaGamemode() )
		m_flStalemateStartTime = gpGlobals->curtime - tf_stalematechangeclasstime.GetFloat();

	// Let out control point masters know that the round has started
	for ( int i = 0; i < g_hControlPointMasters.Count(); i++ )
	{
		variant_t emptyVariant;
		if ( g_hControlPointMasters[i] )
		{
			g_hControlPointMasters[i]->AcceptInput( "RoundStart", NULL, NULL, emptyVariant, 0 );
		}
	}

	// Reset player speeds after preround lock
	CTFPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		pPlayer->TeamFortress_SetSpeed();
		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_ROUND_START );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called before a new round is started (so the previous round can end)
//-----------------------------------------------------------------------------
void CTFGameRules::PreviousRoundEnd( void )
{
	// before we enter a new round, fire the "end output" for the previous round
	if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
	{
		g_hControlPointMasters[0]->FireRoundEndOutput();
	}

	m_iPreviousRoundWinners = GetWinningTeam();
}

//-----------------------------------------------------------------------------
// Purpose: Called when a round has entered stalemate mode (timer has run out)
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnStalemateStart( void )
{
	// Remove everyone's objects
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->TeamFortress_RemoveEverythingFromWorld();
		}
	}

	// Respawn all the players
	RespawnPlayers( true );

	// exception for arena
	if ( InGametype( TF_GAMETYPE_ARENA ) )
	{
		CTFLogicArena *pArena = dynamic_cast<CTFLogicArena *>( gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );
		if ( pArena )
		{
			pArena->m_ArenaRoundStart.FireOutput( NULL, pArena );

			IGameEvent *event = gameeventmanager->CreateEvent( "arena_round_start" );

			if ( event )
				gameeventmanager->FireEvent( event );

			// all teams
			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				BroadcastSound( i, "Ambient.Siren", false );
				BroadcastSound( i, "AM_RoundStartRandom" );
			}

			m_flStalemateStartTime = gpGlobals->curtime;
		}
	}

	// Disable all the active health packs in the world
	m_hDisabledHealthKits.Purge();
	CHealthKit *pHealthPack = gEntList.NextEntByClass( (CHealthKit *)NULL );
	while ( pHealthPack )
	{
		if ( !pHealthPack->IsDisabled() )
		{
			pHealthPack->SetDisabled( true );
			m_hDisabledHealthKits.AddToTail( pHealthPack );
		}
		pHealthPack = gEntList.NextEntByClass( pHealthPack );
	}

	CTFPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_SUDDENDEATH_START );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnStalemateEnd( void )
{
	// Reenable all the health packs we disabled
	for ( int i = 0; i < m_hDisabledHealthKits.Count(); i++ )
	{
		if ( m_hDisabledHealthKits[i] )
		{
			m_hDisabledHealthKits[i]->SetDisabled( false );
		}
	}

	m_hDisabledHealthKits.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InitTeams( void )
{
	BaseClass::InitTeams();

	// clear the player class data
	ResetFilePlayerClassInfoDatabase();
}


ConVar tf_fixedup_damage_radius ( "tf_fixedup_damage_radius", "1", FCVAR_CHEAT );
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			&vecSrcIn - 
//			flRadius - 
//			iClassIgnore - 
//			*pEntityIgnore - 
//-----------------------------------------------------------------------------
void CTFGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	const int MASK_RADIUS_DAMAGE = MASK_SHOT&(~CONTENTS_HITBOX);
	CBaseEntity *pEntity = NULL;
	trace_t		tr;
	float		falloff;
	Vector		vecSpot;

	Vector vecSrc = vecSrcIn;

	if ( info.GetDamageType() & DMG_RADIUS_MAX )
		falloff = 0.0;
	else if ( info.GetDamageType() & DMG_HALF_FALLOFF )
		falloff = 0.5;
	else if ( flRadius )
		falloff = info.GetDamage() / flRadius;
	else
		falloff = 1.0;

	CBaseEntity *pInflictor = info.GetInflictor();
	
//	float flHalfRadiusSqr = Square( flRadius / 2.0f );

	// iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( vecSrc, flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		// This value is used to scale damage when the explosion is blocked by some other object.
		float flBlockedDamagePercent = 0.0f;

		if ( pEntity == pEntityIgnore )
			continue;

		if ( pEntity->m_takedamage == DAMAGE_NO )
			continue;

		// UNDONE: this should check a damage mask, not an ignore
		if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
		{// houndeyes don't hurt other houndeyes with their attack
			continue;
		}

		// Check that the explosion can 'see' this entity.
		vecSpot = pEntity->BodyTarget( vecSrc, false );
		CTraceFilterIgnorePlayers filter( info.GetInflictor(), COLLISION_GROUP_PROJECTILE );
		UTIL_TraceLine( vecSrc, vecSpot, MASK_RADIUS_DAMAGE, &filter, &tr );

		if ( tr.fraction != 1.0 && tr.m_pEnt != pEntity )
			continue;

		// Adjust the damage - apply falloff.
		float flAdjustedDamage = 0.0f;
		float flNonSelfDamage = 0.0f;

		float flDistanceToEntity;

		bool bInstantKill = false;
		// Rockets store the ent they hit as the enemy and have already
		// dealt full damage to them by this time
		if ( pInflictor && ( pEntity == pInflictor->GetEnemy() ) )
		{
			// Full damage, we hit this entity directly
			flDistanceToEntity = 0;
			
			if ( TFGameRules()->IsMutator( ROCKET_ARENA ) && !TFGameRules()->IsGGGamemode() )
				bInstantKill = true;
		}
		else if ( pEntity->IsPlayer() || pEntity->IsNPC() )
		{
			// Use whichever is closer, absorigin or worldspacecenter
			float flToWorldSpaceCenter = ( vecSrc - pEntity->WorldSpaceCenter() ).Length();
			float flToOrigin = ( vecSrc - pEntity->GetAbsOrigin() ).Length();

			flDistanceToEntity = min( flToWorldSpaceCenter, flToOrigin );
		}
		else
		{
			flDistanceToEntity = ( vecSrc - tr.endpos ).Length();
		}

		if ( tf_fixedup_damage_radius.GetBool() )
		{
			flAdjustedDamage = RemapValClamped(flDistanceToEntity, 0, flRadius, info.GetDamage(), info.GetDamage() * falloff);
		}
		else
		{
			flAdjustedDamage = flDistanceToEntity * falloff;
			flAdjustedDamage = info.GetDamage() - flAdjustedDamage;
		}

		// Take a little less damage from yourself
		if ( pEntity == info.GetAttacker() )
		{
			flNonSelfDamage = flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent);
			CTFPlayer *pSelf = ToTFPlayer(pEntity);
			if ( pSelf && pSelf->m_Shared.InCond( TF_COND_SHIELD ) )
				flNonSelfDamage /= ( of_resistance.GetFloat() * 2.0f );
			if ( pSelf && pSelf->m_Shared.InCondUber() )
				flNonSelfDamage = 0;
			else
			{
				switch ( of_selfdamage.GetInt() )
				{
					case -1:
						switch ( TFGameRules()->GetMutator() )
						{	
						case CLAN_ARENA:
						case UNHOLY_TRINITY:
						case ROCKET_ARENA:
								flAdjustedDamage = 0.0f;
								break;
							default:
								flAdjustedDamage = flAdjustedDamage * 0.75f;
								break;
						}
						break;
					case 0:
						flAdjustedDamage = flAdjustedDamage * 0.0f;
						break;
					default:
					case 1:
						flAdjustedDamage = flAdjustedDamage * 0.75f;
						break;
				}
			}
		}
		/*
		if ( flAdjustedDamage <= 0 && !flNonSelfDamage )
			continue;
		*/
		// the explosion can 'see' this entity, so hurt them!
		if (tr.startsolid)
		{
			// if we're stuck inside them, fixup the position and distance
			tr.endpos = vecSrc;
			tr.fraction = 0.0;
		}
		
		CTakeDamageInfo adjustedInfo = info;
		//Msg("%s: Blocked damage: %f percent (in:%f  out:%f)\n", pEntity->GetClassname(), flBlockedDamagePercent * 100, flAdjustedDamage, flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );
		adjustedInfo.SetDamage( flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );

		// Now make a consideration for skill level!
		if( info.GetAttacker() && info.GetAttacker()->IsPlayer() && pEntity->IsNPC() )
		{
			// An explosion set off by the player is harming an NPC. Adjust damage accordingly.
			adjustedInfo.AdjustPlayerDamageInflictedForSkillLevel();
		}

		Vector dir = vecSpot - vecSrc;
		VectorNormalize(dir);

		// If we don't have a damage force, manufacture one
		if (adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin)
		{
			float SelfDamage = adjustedInfo.GetDamage();
			adjustedInfo.SetDamage ( flNonSelfDamage );
			CalculateExplosiveDamageForce(&adjustedInfo, dir, vecSrc);
			adjustedInfo.SetDamage ( SelfDamage );
		}
		else
		{
			// Assume the force passed in is the maximum force. Decay it based on falloff.
			float flForce = adjustedInfo.GetDamageForce().Length() * falloff;
			adjustedInfo.SetDamageForce(dir * flForce);
			adjustedInfo.SetDamagePosition(vecSrc);
		}
		
		if ( bInstantKill )
			adjustedInfo.SetDamage( pEntity->GetHealth() / falloff );

		if ( tr.fraction != 1.0 && pEntity == tr.m_pEnt)
		{
			ClearMultiDamage( );
			pEntity->DispatchTraceAttack( adjustedInfo, dir, &tr );
			if ( flNonSelfDamage )
				ApplyMultiSelfDamage( flNonSelfDamage * of_rocketjump_multiplier.GetFloat() );
			else
				ApplyMultiDamage();
		}
		else
		{
			if ( flNonSelfDamage )
				pEntity->TakeSelfDamage( adjustedInfo, flNonSelfDamage * of_rocketjump_multiplier.GetFloat() );
			else
				pEntity->TakeDamage( adjustedInfo/*, flNonSelfDamage, flNonSelfDamage */);
		}

		// Now hit all triggers along the way that respond to damage... 
		pEntity->TraceAttackToTriggers( adjustedInfo, vecSrc, tr.endpos, dir );
	}
}

	// --------------------------------------------------------------------------------------------------- //
	// Voice helper
	// --------------------------------------------------------------------------------------------------- //

	class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
	{
	public:
		virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
		{
			// Dead players can only be heard by other dead team mates but only if a match is in progress
			if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN && TFGameRules()->State_Get() != GR_STATE_GAME_OVER ) 
			{
				if ( pTalker->IsAlive() == false )
				{
					if ( pListener->IsAlive() == false || tf_gravetalk.GetBool() )
						return ( pListener->InSameTeam( pTalker ) );

					return false;
				}
			}

			return ( pListener->InSameTeam( pTalker ) );
		}
	};
	CVoiceGameMgrHelper g_VoiceGameMgrHelper;
	IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

	// Load the objects.txt file.
	class CObjectsFileLoad : public CAutoGameSystem
	{
	public:
		virtual bool Init()
		{
			LoadObjectInfos( filesystem );
			return true;
		}
	} g_ObjectsFileLoad;

	// --------------------------------------------------------------------------------------------------- //
	// Globals.
	// --------------------------------------------------------------------------------------------------- //
/*
	// NOTE: the indices here must match TEAM_UNASSIGNED, TEAM_SPECTATOR, TF_TEAM_RED, TF_TEAM_BLUE, etc.
	char *sTeamNames[] =
	{
		"Unassigned",
		"Spectator",
		"Red",
		"Blue"
	};
*/
	// --------------------------------------------------------------------------------------------------- //
	// Global helper functions.
	// --------------------------------------------------------------------------------------------------- //
	
	// World.cpp calls this but we don't use it in TF.
	void InitBodyQue()
	{
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	CTFGameRules::~CTFGameRules()
	{
		// Note, don't delete each team since they are in the gEntList and will 
		// automatically be deleted from there, instead.
		TFTeamMgr()->Shutdown();
		ShutdownCustomResponseRulesDicts();
	}

	//-----------------------------------------------------------------------------
	// Purpose: TF2 Specific Client Commands
	// Input  :
	// Output :
	//-----------------------------------------------------------------------------
	bool CTFGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEdict );

		gameeventmanager->LoadEventsFromFile("resource/ModEvents.res");
		const char *pcmd = args[0];
		if ( FStrEq( pcmd, "objcmd" ) )
		{
			if ( args.ArgC() < 3 )
				return true;

			int entindex = atoi( args[1] );
			edict_t* pEdict = INDEXENT(entindex);
			if ( pEdict )
			{
				CBaseEntity* pBaseEntity = GetContainingEntity(pEdict);
				CBaseObject* pObject = dynamic_cast<CBaseObject*>(pBaseEntity);

				if ( pObject )
				{
					// We have to be relatively close to the object too...

					// BUG! Some commands need to get sent without the player being near the object, 
					// eg delayed dismantle commands. Come up with a better way to ensure players aren't
					// entering these commands in the console.

					//float flDistSq = pObject->GetAbsOrigin().DistToSqr( pPlayer->GetAbsOrigin() );
					//if (flDistSq <= (MAX_OBJECT_SCREEN_INPUT_DISTANCE * MAX_OBJECT_SCREEN_INPUT_DISTANCE))
					{
						// Strip off the 1st two arguments and make a new argument string
						CCommand objectArgs( args.ArgC() - 2, &args.ArgV()[2]);
						pObject->ClientCommand( pPlayer, objectArgs );
					}
				}
			}

			return true;
		}

		// Handle some player commands here as they relate more directly to gamerules state
		if ( FStrEq( pcmd, "nextmap" ) )
		{
			if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
			{
				char szNextMap[32];

				if ( nextlevel.GetString() && *nextlevel.GetString() && engine->IsMapValid( nextlevel.GetString() ) )
				{
					Q_strncpy( szNextMap, nextlevel.GetString(), sizeof( szNextMap ) );
				}
				else
				{
					GetNextLevelName( szNextMap, sizeof( szNextMap ) );
				}

				ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_nextmap", szNextMap);

				pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1;
			}

			return true;
		}
		else if ( FStrEq( pcmd, "timeleft" ) )
		{	
			if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
			{
				if ( mp_timelimit.GetInt() > 0 )
				{
					int iTimeLeft = GetTimeLeft();

					char szMinutes[5];
					char szSeconds[3];

					if ( iTimeLeft <= 0 )
					{
						Q_snprintf( szMinutes, sizeof(szMinutes), "0" );
						Q_snprintf( szSeconds, sizeof(szSeconds), "00" );
					}
					else
					{
						Q_snprintf( szMinutes, sizeof(szMinutes), "%d", iTimeLeft / 60 );
						Q_snprintf( szSeconds, sizeof(szSeconds), "%02d", iTimeLeft % 60 );
					}				

					ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_timeleft", szMinutes, szSeconds );
				}
				else
				{
					ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_timeleft_nolimit" );
				}

				pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1;
			}
			return true;
		}
		else if( pPlayer->ClientCommand( args ) )
		{
            return true;
		}

		return BaseClass::ClientCommand( pEdict, args );
	}
	// Add the ability to ignore the world trace
	void CTFGameRules::Think()
	{
		if ( !g_fGameOver )
		{
			if ( gpGlobals->curtime > m_flNextPeriodicThink )
			{
				if ( State_Get() != GR_STATE_TEAM_WIN )
				{
					if ( CheckCapsPerRound() )
						return;
				}
			}

			// there is less than 60 seconds left of time, start voting for next map
			if ( !IsDMGamemode() && mp_timelimit.GetInt() > 0 && GetTimeLeft() <= 60 && !m_bStartedVote && !TFGameRules()->IsInWaitingForPlayers() )
			{
				DevMsg( "VoteController: Timeleft is less than 60 seconds, begin nextlevel voting... \n" );
				m_bStartedVote = true;
				//engine->ServerCommand( "callvote nextlevel" );
				char szEmptyDetails[MAX_VOTE_DETAILS_LENGTH];
				szEmptyDetails[0] = '\0';
				g_voteController->CreateVote( DEDICATED_SERVER, "nextlevel", szEmptyDetails );
			}

			if ( IsDMGamemode() && CountActivePlayers() > 0 && !DontCountKills() )
			{
				int iFragLimit = fraglimit.GetInt();
				if ( IsGGGamemode() )
						iFragLimit = (m_iMaxLevel * m_iRequiredKills) - m_iRequiredKills+1;
				if ( iFragLimit > 0 ) 
				{
					if ( IsTDMGamemode() )
					{
						if ( m_nCurrFrags < TFTeamMgr()->GetTeam(TF_TEAM_RED)->GetScore() )
						{
							m_nCurrFrags = TFTeamMgr()->GetTeam(TF_TEAM_RED)->GetScore();
							FireTargets( "game_fragincrease", TFTeamMgr()->GetTeam(TF_TEAM_RED), TFTeamMgr()->GetTeam(TF_TEAM_RED), USE_TOGGLE, 0 );
						}
						else if ( m_nCurrFrags < TFTeamMgr()->GetTeam(TF_TEAM_BLUE)->GetScore() )
						{
							m_nCurrFrags = TFTeamMgr()->GetTeam(TF_TEAM_BLUE)->GetScore();
							FireTargets( "game_fragincrease", TFTeamMgr()->GetTeam(TF_TEAM_BLUE), TFTeamMgr()->GetTeam(TF_TEAM_BLUE), USE_TOGGLE, 0 );
						}						
						if ( TFTeamMgr()->GetTeam(TF_TEAM_RED)->GetScore() >= iFragLimit || TFTeamMgr()->GetTeam(TF_TEAM_BLUE)->GetScore() >= iFragLimit )
						{
							SendTeamScoresEvent();
							GoToIntermission();
						}
						// one of our teams is at 80% of the fragcount, start voting for next map
						if ( ( TFTeamMgr()->GetTeam(TF_TEAM_RED)->GetScore() >= ( (float)iFragLimit * 0.8 ) ||
							( TFTeamMgr()->GetTeam(TF_TEAM_BLUE)->GetScore() >= ( (float)iFragLimit * 0.8 ) ) ) 
							&& !m_bStartedVote && !TFGameRules()->IsInWaitingForPlayers() )
						{
							DevMsg( "VoteController: Team fraglimit is 80%, begin nextlevel voting... \n" );
							m_bStartedVote = true;
							//engine->ServerCommand( "callvote nextlevel" );
							char szEmptyDetails[MAX_VOTE_DETAILS_LENGTH];
							szEmptyDetails[0] = '\0';
							g_voteController->CreateVote( DEDICATED_SERVER, "nextlevel", szEmptyDetails );
						}
					}
					else
					{
						// check if any player is over the frag limit
						for ( int i = 1; i <= gpGlobals->maxClients; i++ )
						{
							CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

							if ( pPlayer )
							{
								if ( m_nCurrFrags < pPlayer->FragCount() )
								{
									m_nCurrFrags = pPlayer->FragCount();
									FireTargets( "game_fragincrease", pPlayer, pPlayer, USE_TOGGLE, 0 );
								}

								if ( pPlayer->FragCount() >= iFragLimit )
								{
									SetWinningTeam( TF_TEAM_MERCENARY, WINREASON_POINTLIMIT, true, true, false);
								}

								// one of our players is at 80% of the fragcount, start voting for next map
								if ( pPlayer->FragCount() >= ( (float)iFragLimit * 0.8 ) && !m_bStartedVote && !TFGameRules()->IsInWaitingForPlayers() )
								{
									DevMsg( "VoteController: Player fraglimit is 80%, begin nextlevel voting... \n" );
									m_bStartedVote = true;
									//engine->ServerCommand( "callvote nextlevel" );
									char szEmptyDetails[MAX_VOTE_DETAILS_LENGTH];
									szEmptyDetails[0] = '\0';
									g_voteController->CreateVote( DEDICATED_SERVER, "nextlevel", szEmptyDetails );
								}
							}					
						}
					}
				}
			}
			
			if ( IsGGGamemode() && CountActivePlayers() > 0 )
			{
				// check if any player is over the frag limit
				// and creates a game event for achievement
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					CTFPlayer *pPlayer =( CTFPlayer *) UTIL_PlayerByIndex(i);				
					if (pPlayer && pPlayer->GGLevel() == m_iMaxLevel )
					{
						GoToIntermission();
					}
				}
			}
			
			if ( IsArenaGamemode() )
			{
				if ( CountActivePlayers() > 1 && State_Get() == GR_STATE_RND_RUNNING )
				{
					int iDeadTeam = TEAM_UNASSIGNED;
					int iAliveTeam = TEAM_UNASSIGNED;

					// If a team is fully killed, the other team has won
					for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
					{
						CTeam *pTeam = GetGlobalTeam(i);
						Assert( pTeam );
						if ( !pTeam )
							continue;
						int iPlayers = pTeam->GetNumPlayers();
						if ( iPlayers )
						{
							int bFoundLiveOne = 0;
							for ( int player = 0; player < iPlayers; player++ )
							{
								if ( pTeam->GetPlayer(player) && pTeam->GetPlayer(player)->IsAlive())
								{
									bFoundLiveOne++;
									if ( pTeam->GetTeamNumber() != TF_TEAM_MERCENARY )
										break;
								}
							}
							if ( pTeam->GetTeamNumber() == TF_TEAM_MERCENARY  )
							{
								if ( bFoundLiveOne <= 1 )
								{
									iAliveTeam = i;
									iDeadTeam = i;
									break;
								}
								else
									iAliveTeam = i;
							}
							else
							{
								if ( bFoundLiveOne > 0 )
								{
									iAliveTeam = i;
								}
								else
								{
									iDeadTeam = i;
								}
							}							
						}
					}

					if ( iDeadTeam && iAliveTeam )
					{					
						SetWinningTeam( iAliveTeam, WINREASON_OPPONENTS_DEAD, m_bForceMapReset );
					}
				}
			}
			
			if ( IsESCGamemode() )
			{
				// If a team is fully killed, the other team has won
				for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
				{
					CTeam *pTeam = GetGlobalTeam(i);
					Assert( pTeam );
							int iPlayers = pTeam->GetNumPlayers();
					if ( iPlayers )
					{
						int bFoundCivs = 0;
						for ( int player = 0; player < iPlayers; player++ )
						{
							if ( pTeam->GetPlayer(player) && pTeam->GetPlayer(player)->IsConnected() )
							{
								CTFPlayer *pCiv = ToTFPlayer ( pTeam->GetPlayer(player) );
								if ( pCiv && pCiv->IsPlayerClass( TF_CLASS_CIVILIAN ) )
								{
									bFoundCivs++;
									if ( bFoundCivs > GetMaxHunted( i ) && !of_allow_special_classes.GetBool() )
									{
										pCiv->HandleCommand_JoinClass( "random", true );
										pCiv->AllowInstantSpawn();
										pCiv->ForceRespawn();											
										bFoundCivs--;
									}
								}
							}
						}
						if ( GetMaxHunted( i ) <= 0 )
							continue;
						int HuntedTracker = GetMaxHunted( i );
						if( iPlayers < GetMaxHunted( i ) )
							HuntedTracker = iPlayers;
						while( bFoundCivs < HuntedTracker )
						{
							int iPlayerIndex;
							iPlayerIndex = random->RandomInt( 0, iPlayers );
							if( pTeam->GetPlayer(iPlayerIndex) && pTeam->GetPlayer(iPlayerIndex)->IsConnected() )
							{
								CTFPlayer *pCiv = ToTFPlayer( pTeam->GetPlayer(iPlayerIndex) );
								if ( pCiv && !pCiv->IsPlayerClass( TF_CLASS_CIVILIAN ) )
								{
									pCiv->SetDesiredPlayerClassIndex( TF_CLASS_CIVILIAN );
									pCiv->AllowInstantSpawn();
									pCiv->ForceRespawn();								
									bFoundCivs++;
								}
							}
						}
					}
				}
			}
			
			if (m_hReEnableSpawns.Count())
			{
				for ( int i = 0; i < m_hReEnableSpawns.Count(); i++ )
				{
					if ( m_hReEnableSpawns[i] )
						{
							inputdata_t Temp; 
							m_hReEnableSpawns[i]->InputEnable( Temp );
						}
				}
			}
		} // Game playerdie
		// Play( MineOddity );
		BaseClass::Think();
	}
	// entity limit measures, if we are above 1950 then start clearing out entities 
	// this really only happens with >24 players on large maps such as tc_hydro	
	void CTFGameRules::EntityLimitPrevention()
	{
		if ( engine->GetEntityCount() > 1950 )
		{
			Warning("Entity count exceeded 1950, removing unnecessary entities...");

			CBaseEntity *pEntity = NULL;
			while ((pEntity = gEntList.FindEntityByClassname(pEntity, "spotlight_end")) != NULL)
			{
				UTIL_Remove(pEntity);
			}
			while ((pEntity = gEntList.FindEntityByClassname(pEntity, "beam")) != NULL)
			{
				UTIL_Remove(pEntity);
			}
			while ((pEntity = gEntList.FindEntityByClassname(pEntity, "point_spotlight")) != NULL)
			{
				UTIL_Remove(pEntity);
			}

			// if the server manages to somehow get more than 2000 entities after the previous killing, take desperate measures and kill more visual elements
			if ( engine->GetEntityCount() > 2000 )
			{
				Warning("Entity count exceeded 2000!!!!! Removing more visual entities...");

				while ((pEntity = gEntList.FindEntityByClassname(pEntity, "env_lightglow")) != NULL)
				{
					UTIL_Remove(pEntity);
				}
				while ((pEntity = gEntList.FindEntityByClassname(pEntity, "env_sprite")) != NULL)
				{
					UTIL_Remove(pEntity);
				}
				while ((pEntity = gEntList.FindEntityByClassname(pEntity, "move_rope")) != NULL)
				{
					UTIL_Remove(pEntity);
				}
				while ((pEntity = gEntList.FindEntityByClassname(pEntity, "keyframe_rope")) != NULL)
				{
					UTIL_Remove(pEntity);
				}
			}
		}
	}	
	
	//Runs think for all player's conditions
	//Need to do this here instead of the player so players that crash still run their important thinks
	void CTFGameRules::RunPlayerConditionThink ( void )
	{
		for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( pPlayer )
			{
				pPlayer->m_Shared.ConditionGameRulesThink();
			}
		}
	}

	void CTFGameRules::FrameUpdatePostEntityThink()
	{
		BaseClass::FrameUpdatePostEntityThink();

		RunPlayerConditionThink();
	}

	bool CTFGameRules::CheckCapsPerRound()
	{
		if ( tf_flag_caps_per_round.GetInt() > 0 )
		{
			int iMaxCaps = -1;
			CTFTeam *pMaxTeam = NULL;

			// check to see if any team has won a "round"
			int nTeamCount = TFTeamMgr()->GetTeamCount();
			for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
			{
				CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
				if ( !pTeam )
					continue;

				// we might have more than one team over the caps limit (if the server op lowered the limit)
				// so loop through to see who has the most among teams over the limit
				if ( pTeam->GetFlagCaptures() >= tf_flag_caps_per_round.GetInt() )
				{
					if ( pTeam->GetFlagCaptures() > iMaxCaps )
					{
						iMaxCaps = pTeam->GetFlagCaptures();
						pMaxTeam = pTeam;
					}
				}
			}

			if ( iMaxCaps != -1 && pMaxTeam != NULL )
			{
				SetWinningTeam( pMaxTeam->GetTeamNumber(), WINREASON_FLAG_CAPTURE_LIMIT );
				return true;
			}
		}

		return false;
	}

	// ran everytime the score in DOM is changed
	void CTFGameRules::CheckDOMScores( int dom_score_red_old, int dom_score_blue_old )
	{
		// is red team's score higher than the score limit?
		if ( m_nDomScore_red >= m_nDomScore_limit )
		{
			// yes? send appropiate outputs to dom logic entity
			CTFLogicDOM *pDom = dynamic_cast< CTFLogicDOM* > ( gEntList.FindEntityByClassname( NULL, "of_logic_dom" ) );
			if ( pDom )
			{
				pDom->m_OnDomScoreHit_any.FireOutput( NULL, pDom );
				pDom->m_OnDomScoreHit_red.FireOutput( NULL, pDom );
			}

			// if win on score limit is enabled in the dom logic entity, set our winner
			if ( m_bDomWinOnLimit )
			{
				SetWinningTeam( TF_TEAM_RED, 0, true, true );
			}
			
			return;
		}
		// is blue team's score higher than the score limit?
		else if ( m_nDomScore_blue >= m_nDomScore_limit )
		{
			// yes? send appropiate outputs to dom logic entity
			CTFLogicDOM *pDom = dynamic_cast< CTFLogicDOM* > ( gEntList.FindEntityByClassname( NULL, "of_logic_dom" ) );
			if ( pDom )
			{
				pDom->m_OnDomScoreHit_any.FireOutput( NULL, pDom );
				pDom->m_OnDomScoreHit_blue.FireOutput( NULL, pDom );
			}

			// if win on score limit is enabled in the dom logic entity, set our winner
			if ( m_bDomWinOnLimit )
			{
				SetWinningTeam( TF_TEAM_BLUE, 0, true, true );
			}

			return;
		}

		//
		// announcer dialog
		//

		// don't create announcer dialog in Escort if domination is enabled there too
		if ( IsESCGamemode() )
			return;

		// if red's score is 90% or more of the score limit, then play appropiate dialog
		// however we don't want this dialog to play everytime! therefore the threshold bool is checked
		// if we somehow end up with *less* score than last time and its less than 90%, then we want to allow the dialog to play again
		if ( ( m_nDomScore_red >= ( m_nDomScore_limit * 90 / 100 ) ) && !m_bDomRedThreshold )
		{
			m_bDomRedThreshold = true;

			BroadcastSound( TF_TEAM_RED, "DOM_FriendlyClose" );
			BroadcastSound( TF_TEAM_BLUE, "DOM_EnemyClose" );

			return;
		}
		else if ( ( ( m_nDomScore_red < dom_score_red_old ) && m_nDomScore_red < ( m_nDomScore_limit * 90 / 100 ) ) && m_bDomRedThreshold )
		{
			m_bDomRedThreshold = false;
		}

		// if blue's score is 90% or more of the score limit, then play appropiate dialog
		// however we don't want this dialog to play everytime! therefore the threshold bool is checked
		// if we somehow end up with *less* score than last time and its less than 90%, then we want to allow the dialog to play again
		if ( ( m_nDomScore_blue >= ( m_nDomScore_limit * 90 / 100 ) ) && !m_bDomBlueThreshold )
		{
			m_bDomBlueThreshold = true;

			BroadcastSound( TF_TEAM_BLUE, "DOM_FriendlyClose" );
			BroadcastSound( TF_TEAM_RED, "DOM_EnemyClose" );

			return;
		}
		else if ( ( ( m_nDomScore_blue < dom_score_blue_old ) && m_nDomScore_blue < ( m_nDomScore_limit * 90 / 100 ) ) && m_bDomBlueThreshold )
		{
			m_bDomBlueThreshold = false;
		}

		// is red's score higher than blue's old score? we are in the lead, play announcer!
		// however we don't want this dialog to play everytime! therefore the threshold bool is checked
		if ( ( m_nDomScore_red > dom_score_blue_old ) && !m_bDomRedLeadThreshold )
		{
			m_bDomBlueLeadThreshold = false;
			m_bDomRedLeadThreshold = true;

			BroadcastSound( TF_TEAM_RED, "DOM_FriendlyLead" );
			BroadcastSound( TF_TEAM_BLUE, "DOM_EnemyLead" );

			return;
		}

		// is blue's score higher than blue's old score? we are in the lead, play announcer!
		// however we don't want this dialog to play everytime! therefore the threshold bool is checked
		if ( ( m_nDomScore_blue > dom_score_red_old ) && !m_bDomBlueLeadThreshold )
		{
			m_bDomRedLeadThreshold = false;
			m_bDomBlueLeadThreshold = true;

			BroadcastSound( TF_TEAM_BLUE, "DOM_FriendlyLead" );
			BroadcastSound( TF_TEAM_RED, "DOM_EnemyLead" );

			return;
		}

		// play countdown dialog if we are within 10 points of reaching the limit
		if ( m_nDomScore_red > dom_score_red_old )
		{
			int counter;

			counter = m_nDomScore_limit - m_nDomScore_red;

			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				switch ( counter )
				{
				case 1:
					BroadcastSound( i, "RoundEnds1seconds" );
					break;
				case 2:
					BroadcastSound( i, "RoundEnds2seconds" );
					break;
				case 3:
					BroadcastSound( i, "RoundEnds3seconds" );
					break;
				case 4:
					BroadcastSound( i, "RoundEnds4seconds" );
					break;
				case 5:
					BroadcastSound( i, "RoundEnds5seconds" );
					break;
				case 6:
					BroadcastSound( i, "RoundEnds6seconds" );
					break;
				case 7:
					BroadcastSound( i, "RoundEnds7seconds" );
					break;
				case 8:
					BroadcastSound( i, "RoundEnds8seconds" );
					break;
				case 9:
					BroadcastSound( i, "RoundEnds9seconds" );
					break;
				default:
					break;
				}
			}
		}
		else if ( m_nDomScore_blue > dom_score_blue_old )
		{
			int counter;

			counter = m_nDomScore_limit - m_nDomScore_blue;

			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				switch ( counter )
				{
				case 1:
					BroadcastSound( i, "RoundEnds1seconds" );
					break;
				case 2:
					BroadcastSound( i, "RoundEnds2seconds" );
					break;
				case 3:
					BroadcastSound( i, "RoundEnds3seconds" );
					break;
				case 4:
					BroadcastSound( i, "RoundEnds4seconds" );
					break;
				case 5:
					BroadcastSound( i, "RoundEnds5seconds" );
					break;
				case 6:
					BroadcastSound( i, "RoundEnds6seconds" );
					break;
				case 7:
					BroadcastSound( i, "RoundEnds7seconds" );
					break;
				case 8:
					BroadcastSound( i, "RoundEnds8seconds" );
					break;
				case 9:
					BroadcastSound( i, "RoundEnds9seconds" );
					break;
				default:
					break;
				}
			}
		}
	}

	bool CTFGameRules::CheckWinLimit()
	{
		if ( mp_winlimit.GetInt() != 0 )
		{
			bool bWinner = false;

			if ( TFTeamMgr()->GetTeam( TF_TEAM_BLUE )->GetScore() >= mp_winlimit.GetInt() )
			{
				UTIL_LogPrintf( "Team \"BLUE\" triggered \"Intermission_Win_Limit\"\n" );
				bWinner = true;
			}
			else if ( TFTeamMgr()->GetTeam( TF_TEAM_RED )->GetScore() >= mp_winlimit.GetInt() )
			{
				UTIL_LogPrintf( "Team \"RED\" triggered \"Intermission_Win_Limit\"\n" );
				bWinner = true;
			}
			else if ( TFTeamMgr()->GetTeam( TF_TEAM_MERCENARY )->GetScore() >= mp_winlimit.GetInt() )
			{
				UTIL_LogPrintf( "Team \"Mercenary\" triggered \"Intermission_Win_Limit\"\n" );
				bWinner = true;
			}
			
			if ( bWinner )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "tf_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Win Limit" );
					gameeventmanager->FireEvent( event );
				}

				GoToIntermission();
				return true;
			}

			bool bVote = false;

			// 1 win left? allow voting
			if ( TFTeamMgr()->GetTeam( TF_TEAM_BLUE )->GetScore() == ( mp_winlimit.GetInt() - 1 ) )
			{
				bVote = true;
			}
			else if ( TFTeamMgr()->GetTeam( TF_TEAM_RED )->GetScore() == ( mp_winlimit.GetInt() - 1 ) )
			{
				bVote = true;
			}
			else if ( TFTeamMgr()->GetTeam( TF_TEAM_MERCENARY )->GetScore() == ( mp_winlimit.GetInt() - 1 ) )
			{
				bVote = true;
			}

			if ( bVote && !m_bStartedVote )
			{
				DevMsg( "VoteController: One round remaining until winlimit, begin nextlevel voting... \n" );
				m_bStartedVote = true;
				//engine->ServerCommand( "callvote nextlevel" );
				char szEmptyDetails[MAX_VOTE_DETAILS_LENGTH];
				szEmptyDetails[0] = '\0';
				g_voteController->CreateVote( DEDICATED_SERVER, "nextlevel", szEmptyDetails );
			}
		}

		return false;
	}

	bool CTFGameRules::CheckMaxRounds( bool bAllowEnd /*= true*/ )
	{
		if ( mp_maxrounds.GetInt() > 0 && IsInPreMatch() == false )
		{
			if ( m_nRoundsPlayed >= mp_maxrounds.GetInt() )
			{
				if ( bAllowEnd )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
					if ( event )
					{
						event->SetString( "reason", "Reached Round Limit" );
						gameeventmanager->FireEvent( event );
					}

					GoToIntermission();
				}
				return true;
			}

			if ( m_nRoundsPlayed == ( mp_maxrounds.GetInt() - 1  ) && !m_bStartedVote )
			{
				DevMsg( "VoteController: One round remaining until maxrounds, begin nextlevel voting... \n" );
				m_bStartedVote = true;
				//engine->ServerCommand( "callvote nextlevel" );
				char szEmptyDetails[MAX_VOTE_DETAILS_LENGTH];
				szEmptyDetails[0] = '\0';
				g_voteController->CreateVote( DEDICATED_SERVER, "nextlevel", szEmptyDetails );
			}
		}

		return false;
	}

	bool CTFGameRules::IsInPreMatch() const
	{
		// TFTODO    return (cb_prematch_time > gpGlobals->time)
		return false;
	}

	float CTFGameRules::GetPreMatchEndTime() const
	{
		//TFTODO: implement this.
		return gpGlobals->curtime;
	}

	void CTFGameRules::GoToIntermission( void )
	{
		CTF_GameStats.Event_GameEnd();

		BaseClass::GoToIntermission();
	}

	bool CTFGameRules::FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info)
	{
		// guard against NULL pointers if players disconnect
		if ( !pPlayer || !pAttacker )
			return false;

		// if pAttacker is an object, we can only do damage if pPlayer is our builder
		if ( pAttacker->IsBaseObject() )
		{
			CBaseObject *pObj = ( CBaseObject *)pAttacker;

			if ( pObj->GetBuilder() == pPlayer || pPlayer->GetTeamNumber() != pObj->GetTeamNumber() )
			{
				// Builder and enemies
				return true;
			}
			else
			{
				// Teammates of the builder
				return false;
			}
		}

		return BaseClass::FPlayerCanTakeDamage(pPlayer, pAttacker, info);
	}

Vector DropToGround( 
	CBaseEntity *pMainEnt, 
	const Vector &vPos, 
	const Vector &vMins, 
	const Vector &vMaxs )
{
	trace_t trace;
	UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
	return trace.endpos;
}


void TestSpawnPointType( const char *pEntClassName )
{
	// Find the next spawn spot.
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, pEntClassName );

	while( pSpot )
	{
		// trace a box here
		Vector vTestMins = pSpot->GetAbsOrigin() + VEC_HULL_MIN;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + VEC_HULL_MAX;

		if ( UTIL_IsSpaceEmpty( pSpot, vTestMins, vTestMaxs ) )
		{
			// the successful spawn point's location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 0, 255, 0, 100, 60 );

			// drop down to ground
			// removed this causes issues with info player start on some maps
			//Vector GroundPos = DropToGround( NULL, pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

			// the location the player will spawn at
			//NDebugOverlay::Box( GroundPos, VEC_HULL_MIN, VEC_HULL_MAX, 0, 0, 255, 100, 60 );

			// draw the spawn angles
			QAngle spotAngles = pSpot->GetLocalAngles();
			Vector vecForward;
			AngleVectors( spotAngles, &vecForward );
			NDebugOverlay::HorzArrow( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin() + vecForward * 32, 10, 255, 0, 0, 255, true, 60 );
		}
		else
		{
			// failed spawn point location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 255, 0, 0, 100, 60 );
		}

		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}
}

// -------------------------------------------------------------------------------- //

void TestSpawns()
{
	TestSpawnPointType( "info_player_teamspawn" );
}
ConCommand cc_TestSpawns( "map_showspawnpoints", TestSpawns, "Dev - test the spawn points, draws for 60 seconds", FCVAR_CHEAT );

// -------------------------------------------------------------------------------- //

void cc_ShowRespawnTimes()
{
	CTFGameRules *pRules = TFGameRules();
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );

	if ( pRules && pPlayer )
	{
		float flRedMin = ( pRules->m_TeamRespawnWaveTimes[TF_TEAM_RED] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_RED] : mp_respawnwavetime.GetFloat() );
		float flRedScalar = pRules->GetRespawnTimeScalar( TF_TEAM_RED );
		float flNextRedRespawn = pRules->GetNextRespawnWave( TF_TEAM_RED, NULL ) - gpGlobals->curtime;

		float flBlueMin = ( pRules->m_TeamRespawnWaveTimes[TF_TEAM_BLUE] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_BLUE] : mp_respawnwavetime.GetFloat() );
		float flBlueScalar = pRules->GetRespawnTimeScalar( TF_TEAM_BLUE );
		float flNextBlueRespawn = pRules->GetNextRespawnWave( TF_TEAM_BLUE, NULL ) - gpGlobals->curtime;

		float flMercenaryMin = ( pRules->m_TeamRespawnWaveTimes[TF_TEAM_MERCENARY] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_MERCENARY] : mp_respawnwavetime.GetFloat() );
		float flMercenaryScalar = pRules->GetRespawnTimeScalar( TF_TEAM_MERCENARY );
		float flNextMercenaryRespawn = pRules->GetNextRespawnWave( TF_TEAM_MERCENARY, NULL ) - gpGlobals->curtime;	
	
		char tempRed[128];
		Q_snprintf( tempRed, sizeof( tempRed ),   "Red:  Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flRedMin, flRedScalar, flNextRedRespawn );

		char tempBlue[128];
		Q_snprintf( tempBlue, sizeof( tempBlue ), "Blue: Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flBlueMin, flBlueScalar, flNextBlueRespawn );
		
		char tempMercenary[128];
		Q_snprintf( tempMercenary, sizeof( tempMercenary ), "Mercenary: Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flMercenaryMin, flMercenaryScalar, flNextMercenaryRespawn );

		ClientPrint( pPlayer, HUD_PRINTTALK, tempRed );
		ClientPrint( pPlayer, HUD_PRINTTALK, tempBlue );
		ClientPrint( pPlayer, HUD_PRINTTALK, tempMercenary );
	}
}

ConCommand mp_showrespawntimes( "mp_showrespawntimes", cc_ShowRespawnTimes, "Show the min respawn times for the teams" );

// -------------------------------------------------------------------------------- //

CBaseEntity *CTFGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	if ( of_navmesh_spawns.GetBool() && IsDMGamemode() )
	{
		if ( TheNavMesh->IsLoaded() )
		{
			int iCount = TheNavAreas.Count();
			int iArea = RandomInt( 0, iCount - 1 );
			int iPatience;
			for (iPatience = iCount; iPatience; iPatience--)
			{
				if ( TheNavAreas[iArea]->IsFlat() )
				{
					for ( int iAreaPatience = 4; iAreaPatience; iAreaPatience-- )
					{
						Vector vSpawn = TheNavAreas[iArea]->GetRandomPoint() + Vector( 0, 0, 32 );
						trace_t tracehull;
						UTIL_TraceHull( vSpawn, vSpawn, VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &tracehull );
						if ( !tracehull.DidHit() )
						{
							//Take the angles pointing to the furthest wall, forward, backwards, right or left
							QAngle qEyeAngles(0, 0, 0);
							float curdistance = 0;
							for ( int iLine = 1; iLine <= 4; iLine++ )
							{
								trace_t traceline;
								Vector vDir;
								QAngle qDirAngles( 0, 90 * iLine, 0 );
								AngleVectors( qDirAngles, &vDir );
								UTIL_TraceLine( vSpawn, vSpawn + vDir * MAX_COORD_RANGE, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &traceline );
								if ( traceline.DidHit() )
								{
									Vector vRelative = vSpawn - traceline.endpos;
									float distance = VectorNormalize( vRelative );
									if ( distance > curdistance )
									{
										curdistance = distance;
										qEyeAngles = qDirAngles;
									}
								}
							}
							// SUCCESS
							pPlayer->SetLocalOrigin( vSpawn );
							pPlayer->SetAbsVelocity( vec3_origin );
							pPlayer->SetLocalAngles( qEyeAngles );
							pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
							pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
							pPlayer->SnapEyeAngles( qEyeAngles );

							return NULL;
						}
					}
				}
				if ( ++iArea >= iCount ) iArea -= iCount;
			}
			DevMsg( "No valid nav_area found in nav_mesh for of_navmesh_spawns, nav mesh has %d areas\n", iCount );
		}
		else
		{
			DevMsg( "No navigation mesh avalaible with of_navmesh_spawns enabled\n" );
		}
	}

	// get valid spawn point
	CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();

	if ( pSpawnSpot )
	{
		// drop down to ground
		Vector GroundPos = DropToGround( pPlayer, pSpawnSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

		// Move the player to the place it said.
		pPlayer->SetLocalOrigin( pSpawnSpot->GetAbsOrigin() + Vector(0,0,1) );
		pPlayer->SetAbsVelocity( vec3_origin );
		pPlayer->SetLocalAngles( pSpawnSpot->GetLocalAngles() );
		pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
		pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
		pPlayer->SnapEyeAngles( pSpawnSpot->GetLocalAngles() );
	}

	return pSpawnSpot;
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the player is on the correct team and whether or
//          not the spawn point is available.
//-----------------------------------------------------------------------------
bool CTFGameRules::IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers )
{
    // Check the team if not in deathmatch
	if ( IsHL2() || ( TFGameRules()->IsInfGamemode() && pPlayer->GetTeamNumber() == TF_TEAM_BLUE ) )
	{
	}
	else if ( pSpot->GetTeamNumber() != pPlayer->GetTeamNumber() )
	{
		// wow...
		// some fools assigned teampoints to spectator team so i have to check this too
		if ( pSpot->GetTeamNumber() == TEAM_UNASSIGNED || pSpot->GetTeamNumber() == TF_TEAM_MERCENARY && TFGameRules()->IsTeamplay() )
		{
		}
		else
		{
			// Hack: DM maps are supported in infection, but the spawnpoints have no teams assigned
			// Therefore just allow every spawnpoint if the map is prefixed with dm_ ...
			if ( ( TFGameRules()->IsInfGamemode() && !Q_strncmp( STRING( gpGlobals->mapname), "dm_", 3 ) ) )
			{
			}
			else
				return false;
		}
	}

	if ( !pSpot->IsTriggered( pPlayer ) )
		return false;

	CTFTeamSpawn *pCTFSpawn = dynamic_cast<CTFTeamSpawn*>( pSpot );
	if ( pCTFSpawn )
	{
		if ( pCTFSpawn->IsDisabled() )
			return false;

		// live tf2 uses spawnpoints for the comp end screen, which are given the unassigned team (and unassigned spawnpoints are regarded as valid here)
		// therefore, avoid spawnpoints that are flagged as Loser or Winner for the comp end screen
		if ( pCTFSpawn->GetMatchSummary() >= 1 )
			return false;

		
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		// check if this spawnpoint allows our class to spawn here
		if ( pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
		{
			// You shouldn't be looking for a spawnpoint when class is undefined
			Warning( "CTFGameRules::IsSpawnPointValid: Player has undefined class" );
			return false;
		}
		else if ( pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SCOUT && !pCTFSpawn->AllowScout() )
			return false;
		else if ( pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SNIPER && !pCTFSpawn->AllowSniper() )
			return false;
		else if ( pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SOLDIER && !pCTFSpawn->AllowSoldier() )
			return false;
		else if ( pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_DEMOMAN && !pCTFSpawn->AllowDemoman() )
			return false;
		else if ( pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_MEDIC && !pCTFSpawn->AllowMedic() )
			return false;
		else if ( pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_HEAVYWEAPONS && !pCTFSpawn->AllowHeavyweapons() )
			return false;
		else if ( pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_PYRO && !pCTFSpawn->AllowPyro() )
			return false;
		else if ( pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY && !pCTFSpawn->AllowSpy() )
			return false;
		else if ( pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_ENGINEER && !pCTFSpawn->AllowEngineer() )
			return false;
		//else if ( pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY && !pCTFSpawn->AllowMercenary() )
		//	return false;
		else if ( pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_CIVILIAN && !pCTFSpawn->AllowCivilian() )
		{
			// official tf2 maps will have spawnflags missing for Civilian, therefore I do a cheeky solution
			// get every single spawnpoint and check if any of them has a Civilian spawnflag
			// if there is, then its likely a custom map for our mod and therefore we can just keep looping spawnpoints until we hit that one
			// otherwise, force ourselves to spawn here
			CBaseEntity *pEntity = NULL;

			bool bValidSpawn = false;

			while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "info_player_teamspawn" ) ) != NULL )
			{
				if ( TFGameRules()->IsSpawnPointValidNoClass( pEntity, pPlayer, true ) )
					continue;

				if ( pEntity->HasSpawnFlags( SF_CLASS_CIVILIAN ) )
					bValidSpawn = true;
			}

			if ( bValidSpawn )
				return false;
		}
		else if ( pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_JUGGERNAUT && !pCTFSpawn->AllowJuggernaut() )
		{
			// official tf2 maps will have spawnflags missing for Juggernaut, therefore I do a cheeky solution
			// get every single spawnpoint and check if any of them has a Juggernaut spawnflag
			// if there is, then its likely a custom map for our mod and therefore we can just keep looping spawnpoints until we hit that one
			// otherwise, force ourselves to spawn here
			// Find all entities of the correct name and try and sit where they're at
			CBaseEntity *pEntity = NULL;

			bool bValidSpawn = false;

			while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "info_player_teamspawn" ) ) != NULL )
			{
				if ( TFGameRules()->IsSpawnPointValidNoClass( pEntity, pPlayer, true ) )
					continue;

				if ( pEntity->HasSpawnFlags( SF_CLASS_JUGGERNAUT ) )
					bValidSpawn = true;
			}

			if ( bValidSpawn )
				return false;
		}	
	}

	Vector mins = GetViewVectors()->m_vHullMin;
	Vector maxs = GetViewVectors()->m_vHullMax;

	if ( !bIgnorePlayers )
	{
		Vector vTestMins = pSpot->GetAbsOrigin() + mins;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + maxs;
		return UTIL_IsSpaceEmpty( pPlayer, vTestMins, vTestMaxs );
	}

	trace_t trace;
	UTIL_TraceHull( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin(), mins, maxs, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
	return ( trace.fraction == 1 && trace.allsolid != 1 && (trace.startsolid != 1) );
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the player is on the correct team and whether or
//          not the spawn point is available.
//          This time however, without checking if the class spawnflags match
//-----------------------------------------------------------------------------
bool CTFGameRules::IsSpawnPointValidNoClass( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers )
{
    // Check the team if not in deathmatch
	if ( IsHL2() )
	{
	}
	else if ( pSpot->GetTeamNumber() != pPlayer->GetTeamNumber() )
	{
		// wow...
		// some fools assigned teampoints to spectator team so i have to check this too
		if ( pSpot->GetTeamNumber() == TEAM_UNASSIGNED || pSpot->GetTeamNumber() == TF_TEAM_MERCENARY && TFGameRules()->IsTeamplay() )
		{
		}
		else
		{
			// Hack: DM maps are supported in infection, but the spawnpoints have no teams assigned
			// Therefore just allow every spawnpoint if the map is prefixed with dm_ ...
			if ( ( TFGameRules()->IsInfGamemode() && !Q_strncmp( STRING( gpGlobals->mapname), "dm_", 3 ) ) )
			{
			}
			else
				return false;
		}
	}

	if ( !pSpot->IsTriggered( pPlayer ) )
		return false;

	CTFTeamSpawn *pCTFSpawn = dynamic_cast<CTFTeamSpawn*>( pSpot );
	if ( pCTFSpawn )
	{
		if ( pCTFSpawn->IsDisabled() )
			return false;

		// live tf2 uses spawnpoints for the comp end screen, which are given the unassigned team (and unassigned spawnpoints are regarded as valid here)
		// therefore, avoid spawnpoints that are flagged as Loser or Winner for the comp end screen
		if ( pCTFSpawn->GetMatchSummary() >= 1 )
			return false;
	}

	Vector mins = GetViewVectors()->m_vHullMin;
	Vector maxs = GetViewVectors()->m_vHullMax;

	if ( !bIgnorePlayers )
	{
		Vector vTestMins = pSpot->GetAbsOrigin() + mins;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + maxs;
		return UTIL_IsSpaceEmpty( pPlayer, vTestMins, vTestMaxs );
	}

	trace_t trace;
	UTIL_TraceHull( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin(), mins, maxs, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
	return ( trace.fraction == 1 && trace.allsolid != 1 && (trace.startsolid != 1) );
}

Vector CTFGameRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->GetOriginalSpawnOrigin();
}

QAngle CTFGameRules::VecItemRespawnAngles( CItem *pItem )
{
	return pItem->GetOriginalSpawnAngles();
}

float CTFGameRules::FlItemRespawnTime( CItem *pItem )
{
	return ITEM_RESPAWN_TIME;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( !pPlayer )  // dedicated server output
	{
		return NULL;
	}

	const char *pszFormat = NULL;

	// team only
	if ( bTeamOnly == true )
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_Spec";
		}
		else
		{
			if ( pPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_Team_Dead";
			}
			else
			{
				const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
				if ( chatLocation && *chatLocation )
				{
					pszFormat = "TF_Chat_Team_Loc";
				}
				else
				{
					pszFormat = "TF_Chat_Team";
				}
			}
		}
	}
	// everyone
	else
	{	
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_AllSpec";	
		}
		else
		{
			if ( pPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_AllDead";
			}
			else
			{
				pszFormat = "TF_Chat_All";	
			}
		}
	}

	return pszFormat;
}

VoiceCommandMenuItem_t *CTFGameRules::VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem )
{
	VoiceCommandMenuItem_t *pItem = BaseClass::VoiceCommand( pPlayer, iMenu, iItem );

	if ( pItem )
	{
		int iActivity = ActivityList_IndexForName( pItem->m_szGestureActivity );

		if ( iActivity != ACT_INVALID )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

			if ( pTFPlayer )
			{
				pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_VOICE_COMMAND_GESTURE, iActivity );
			}
		}
	}

	return pItem;
}

//-----------------------------------------------------------------------------
// Purpose: Actually change a player's name.  
//-----------------------------------------------------------------------------
void CTFGameRules::ChangePlayerName( CTFPlayer *pPlayer, const char *pszNewName )
{
	const char *pszOldName = pPlayer->GetPlayerName();

	CReliableBroadcastRecipientFilter filter;
	UTIL_SayText2Filter( filter, pPlayer, false, "#TF_Name_Change", pszOldName, pszNewName );

	IGameEvent * event = gameeventmanager->CreateEvent( "player_changename" );
	if ( event )
	{
		event->SetInt( "userid", pPlayer->GetUserID() );
		event->SetString( "oldname", pszOldName );
		event->SetString( "newname", pszNewName );
		gameeventmanager->FireEvent( event );
	}

	pPlayer->SetPlayerName( pszNewName );

	pPlayer->m_flNextNameChangeTime = gpGlobals->curtime + 10.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	const char *pszName = engine->GetClientConVarValue( pPlayer->entindex(), "name" );

	const char *pszOldName = pPlayer->GetPlayerName();

	CTFPlayer *pTFPlayer = (CTFPlayer*)pPlayer;

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && Q_strncmp( pszOldName, pszName, MAX_PLAYER_NAME_LENGTH-1 ) )		
	{
		if ( pTFPlayer->m_flNextNameChangeTime < gpGlobals->curtime )
		{
			ChangePlayerName( pTFPlayer, pszName );
		}
		else
		{
			// no change allowed, force engine to use old name again
			engine->ClientCommand( pPlayer->edict(), "name \"%s\"", pszOldName );

			// tell client that he hit the name change time limit
			ClientPrint( pTFPlayer, HUD_PRINTTALK, "#Name_change_limit_exceeded" );
		}
	}

	// keep track of their hud_classautokill value
	int nClassAutoKill = Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "hud_classautokill" ) );
	pTFPlayer->SetHudClassAutoKill( nClassAutoKill > 0 ? true : false );

	// keep track of their tf_medigun_autoheal value
	pTFPlayer->SetMedigunAutoHeal( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "tf_medigun_autoheal" ) ) > 0 );
	
	// keep track of their of_autoreload value
	pTFPlayer->SetAutoReload( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "of_autoreload" ) ) > 0 );
	
	// keep track of their cl_autorezoom value
	pTFPlayer->SetAutoRezoom( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "cl_autorezoom" ) ) > 0 );

	const char *pszFov = engine->GetClientConVarValue( pPlayer->entindex(), "fov_desired" );
	int iFov = atoi(pszFov);
	iFov = clamp( iFov, 50, 130 );
	pTFPlayer->SetDefaultFOV( iFov );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGameRules::ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues )
{
	BaseClass::ClientCommandKeyValues( pEntity, pKeyValues );

	CTFPlayer *pPlayer = ToTFPlayer( CBaseEntity::Instance( pEntity ) );
	if ( !pPlayer )
		return;

	if ( FStrEq( pKeyValues->GetName(), "FreezeCamTaunt" ) )
	{
		int iAchieverIndex = pKeyValues->GetInt( "achiever" );
		CTFPlayer *pAchiever = ToTFPlayer( UTIL_PlayerByIndex( iAchieverIndex ) );

		if ( pAchiever && pAchiever != pPlayer )
		{
			int iClass = pAchiever->GetPlayerClass()->GetClassIndex();

			if ( g_TauntCamAchievements[iClass] != 0 )
			{
				pAchiever->AwardAchievement( g_TauntCamAchievements[iClass] );
			}
		}
	}
}

void CTFGameRules::GetTaggedConVarList( KeyValues *pCvarTagList )
{
	BaseClass::GetTaggedConVarList( pCvarTagList );
	
		// of_mutators
	KeyValues *pKeyValues = new KeyValues( "of_mutator" );
	pKeyValues->SetString( "convar", "of_mutator" );
	pKeyValues->SetString( "tag", "mutator" );

	pCvarTagList->AddSubKey( pKeyValues );

	/*
		// of_instagib
	KeyValues *pKeyValues = new KeyValues( "of_instagib" );
	pKeyValues->SetString( "convar", "of_instagib" );
	pKeyValues->SetString( "tag", "instagib" );

	pCvarTagList->AddSubKey( pKeyValues );

		// of_clanarena
	pKeyValues = new KeyValues( "of_clanarena" );
	pKeyValues->SetString( "convar", "of_clanarena" );
	pKeyValues->SetString( "tag", "clanarena" );

	pCvarTagList->AddSubKey( pKeyValues );

		// of_gungame
	pKeyValues = new KeyValues( "of_gungame" );
	pKeyValues->SetString( "convar", "of_gungame" );
	pKeyValues->SetString( "tag", "gungame" );

	pCvarTagList->AddSubKey( pKeyValues );
	*/
	
		// of_infiniteammo
	pKeyValues = new KeyValues( "of_infiniteammo" );
	pKeyValues->SetString( "convar", "of_infiniteammo" );
	pKeyValues->SetString( "tag", "infiniteammo" );

	pCvarTagList->AddSubKey( pKeyValues );
	
		// of_noreload
	pKeyValues = new KeyValues( "of_noreload" );
	pKeyValues->SetString( "convar", "of_noreload" );
	pKeyValues->SetString( "tag", "noreload" );

	pCvarTagList->AddSubKey( pKeyValues );
	
		// of_allow_special_teams 
	pKeyValues = new KeyValues( "of_allow_special_teams" );
	pKeyValues->SetString( "convar", "of_allow_special_teams" );
	pKeyValues->SetString( "tag", "specialteams" );

	pCvarTagList->AddSubKey( pKeyValues );
	
		// of_forceclass 
	pKeyValues = new KeyValues( "of_forceclass" );
	pKeyValues->SetString( "convar", "of_forceclass" );
	pKeyValues->SetString( "tag", "allclassesallowed" );

	pCvarTagList->AddSubKey( pKeyValues );	
	
		// mp_disable_respawn_times  
	pKeyValues = new KeyValues( "mp_disable_respawn_times" );
	pKeyValues->SetString( "convar", "mp_disable_respawn_times" );
	pKeyValues->SetString( "tag", "norespawntimes" );

	pCvarTagList->AddSubKey( pKeyValues );	
	
		// tf_weapon_criticals   
	pKeyValues = new KeyValues( "tf_weapon_criticals" );
	pKeyValues->SetString( "convar", "tf_weapon_criticals" );
	pKeyValues->SetString( "tag", "critsenabled" );

	pCvarTagList->AddSubKey( pKeyValues );		
	
		// tf_damage_disablespread   
	pKeyValues = new KeyValues( "tf_damage_disablespread" );
	pKeyValues->SetString( "convar", "tf_damage_disablespread" );
	pKeyValues->SetString( "tag", "nodamagespread" );

	pCvarTagList->AddSubKey( pKeyValues );	
	
		// tf_use_fixed_weaponspreads   
	pKeyValues = new KeyValues( "tf_use_fixed_weaponspreads" );
	pKeyValues->SetString( "convar", "tf_use_fixed_weaponspreads" );
	pKeyValues->SetString( "tag", "norandomspread" );

	pCvarTagList->AddSubKey( pKeyValues );		

		// tf_birthday    
	pKeyValues = new KeyValues( "tf_birthday" );
	pKeyValues->SetString( "convar", "tf_birthday" );
	pKeyValues->SetString( "tag", "birthday" );

	pCvarTagList->AddSubKey( pKeyValues );	
	
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified player can carry any more of the ammo type
//-----------------------------------------------------------------------------
bool CTFGameRules::CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex )
{
	if ( iAmmoIndex > -1 )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

		if ( pTFPlayer )
		{
			// Get the player class data - contains ammo counts for this class.
			TFPlayerClassData_t *pData = pTFPlayer->GetPlayerClass()->GetData();
			if ( pData )
			{
				// Get the max carrying capacity for this ammo
				int iMaxCarry = pData->m_aAmmoMax[iAmmoIndex];

				// Does the player have room for more of this type of ammo?
				if ( pTFPlayer->GetAmmoCount( iAmmoIndex ) < iMaxCarry )
				{
					return true;
				}
			}
		}
		// fixme experimental: this should fix npc ammo but im not sure
		else
		{
			return BaseClass::CanHaveAmmo( pPlayer, iAmmoIndex );
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBaseMultiplayerPlayer *pScorer = ToBaseMultiplayerPlayer( GetDeathScorer( pKiller, pInflictor, pVictim ) );
	CBaseEntity *pAssister = NULL;
	CBaseObject *pObject = NULL;

	// if inflictor or killer is a base object, tell them that they got a kill
	// ( depends if a sentry rocket got the kill, sentry may be inflictor or killer )
	if ( pInflictor )
	{
		if ( pInflictor->IsBaseObject() )
		{
			pObject = dynamic_cast<CBaseObject *>( pInflictor );
		}
		else 
		{
			CBaseEntity *pInflictorOwner = pInflictor->GetOwnerEntity();
			if ( pInflictorOwner && pInflictorOwner->IsBaseObject() )
			{
				pObject = dynamic_cast<CBaseObject *>( pInflictorOwner );
			}
		}
		
	}
	else if( pKiller && pKiller->IsBaseObject() )
	{
		pObject = dynamic_cast<CBaseObject *>( pKiller );
	}

	if ( pObject )
	{
		pObject->IncrementKills();
		pInflictor = pObject;

		if ( pObject->ObjectType() == OBJ_SENTRYGUN )
		{
			CTFPlayer *pOwner = pObject->GetOwner();
			if ( pOwner )
			{
				int iKills = pObject->GetKills();

				// keep track of max kills per a single sentry gun in the player object
				if ( pOwner->GetMaxSentryKills() < iKills )
				{
					pOwner->SetMaxSentryKills( iKills );
					CTF_GameStats.Event_MaxSentryKills( pOwner, iKills );
				}

				// if we just got 10 kills with one sentry, tell the owner's client, which will award achievement if it doesn't have it already
				if ( iKills == 10 )
				{
					pOwner->AwardAchievement( ACHIEVEMENT_TF_GET_TURRETKILLS );
				}
			}
		}
	}

	// if not killed by  suicide or killed by world, see if the scorer had an assister, and if so give the assister credit
	if ( ( pVictim != pScorer ) && pKiller )
	{
		pAssister = GetAssister( pVictim, pScorer, pInflictor );
	}	

	//find the area the player is in and see if his death causes a block
	CTriggerAreaCapture *pArea = dynamic_cast<CTriggerAreaCapture *>(gEntList.FindEntityByClassname( NULL, "trigger_capture_area" ) );
	while( pArea )
	{
		if ( pArea->CheckIfDeathCausesBlock( ToBaseMultiplayerPlayer(pVictim), pScorer ) )
			break;

		pArea = dynamic_cast<CTriggerAreaCapture *>( gEntList.FindEntityByClassname( pArea, "trigger_capture_area" ) );
	}

	CTFPlayer *pTFPlayerAssister = ToTFPlayer( pAssister );

	// determine if this kill affected a nemesis relationship
	int iDeathFlags = 0;
	CTFPlayer *pTFPlayerVictim = ToTFPlayer( pVictim );
	CTFPlayer *pTFPlayerScorer = ToTFPlayer( pScorer );
	if ( pScorer )
	{	
		CalcDominationAndRevenge( pTFPlayerScorer, pTFPlayerVictim, false, &iDeathFlags );

		if ( pTFPlayerAssister )
		{
			CalcDominationAndRevenge( pTFPlayerAssister, pTFPlayerVictim, true, &iDeathFlags );
		}

		if ( IsTeamplay() && IsTDMGamemode() && pTFPlayerScorer->IsEnemy(pTFPlayerVictim) && !DontCountKills() )
		{
			TFTeamMgr()->AddTeamScore( pTFPlayerScorer->GetTeamNumber(), 1 );
		}
	}
	pTFPlayerVictim->SetDeathFlags( iDeathFlags );	

	if ( pTFPlayerAssister )
	{
		CTF_GameStats.Event_AssistKill( pTFPlayerAssister, pVictim );
	}

	CBaseObject *pObjectAssister = dynamic_cast<CBaseObject *>( pInflictor );

	if ( pTFPlayerAssister && pObjectAssister )
	{
		pObjectAssister->IncrementAssists();
	}

	BaseClass::PlayerKilled( pVictim, info );
}

//-----------------------------------------------------------------------------
// Purpose: Determines if attacker and victim have gotten domination or revenge
//-----------------------------------------------------------------------------
void CTFGameRules::CalcDominationAndRevenge( CTFPlayer *pAttacker, CTFPlayer *pVictim, bool bIsAssist, int *piDeathFlags )
{
	PlayerStats_t *pStatsVictim = CTF_GameStats.FindPlayerStats( pVictim );

	// calculate # of unanswered kills between killer & victim - add 1 to include current kill
	int iKillsUnanswered = pStatsVictim->statsKills.iNumKilledByUnanswered[pAttacker->entindex()] + 1;		
	if ( TF_KILLS_DOMINATION == iKillsUnanswered )
	{			
		// this is the Nth unanswered kill between killer and victim, killer is now dominating victim
		*piDeathFlags |= ( bIsAssist ? TF_DEATH_ASSISTER_DOMINATION : TF_DEATH_DOMINATION );
		// set victim to be dominated by killer
		pAttacker->m_Shared.SetPlayerDominated( pVictim, true );
		// record stats
		CTF_GameStats.Event_PlayerDominatedOther( pAttacker );
	}
	else if ( pVictim->m_Shared.IsPlayerDominated( pAttacker->entindex() ) )
	{
		// the killer killed someone who was dominating him, gains revenge
		*piDeathFlags |= ( bIsAssist ? TF_DEATH_ASSISTER_REVENGE : TF_DEATH_REVENGE );
		// set victim to no longer be dominating the killer
		pVictim->m_Shared.SetPlayerDominated( pAttacker, false );
		// record stats
		CTF_GameStats.Event_PlayerRevenge( pAttacker );
	}

}

//-----------------------------------------------------------------------------
// Purpose: create some proxy entities that we use for transmitting data */
//-----------------------------------------------------------------------------
void CTFGameRules::CreateStandardEntities()
{
	// Create the player resource
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "tf_player_manager", vec3_origin, vec3_angle );

	// Create the objective resource
	g_pObjectiveResource = (CTFObjectiveResource *)CBaseEntity::Create( "tf_objective_resource", vec3_origin, vec3_angle );

	Assert( g_pObjectiveResource );

	// Create the entity that will send our data to the client.
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "tf_gamerules" );

	if ( !pEnt )
	{
		pEnt = CBaseEntity::Create( "tf_gamerules", vec3_origin, vec3_angle );
		pEnt->SetName( AllocPooledString( "tf_gamerules" ) );
	}

	
	CBaseEntity::Create( "vote_controller", vec3_origin, vec3_angle );

	new CKickIssue();
	CRestartGameIssue *pRestartVote = new CRestartGameIssue();
	if ( pRestartVote )
		pRestartVote->Init();
	new CChangeLevelIssue();
	new CChangeMutatorIssue();
	new CNextLevelIssue();
	CScrambleTeams *pScrambleVote = new CScrambleTeams();
	if ( pScrambleVote )
		pScrambleVote->Init();
}

//-----------------------------------------------------------------------------
// Purpose: determine the class name of the weapon that got a kill
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetKillingWeaponName( const CTakeDamageInfo &info, CTFPlayer *pVictim )
{
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = TFGameRules()->GetDeathScorer( pKiller, pInflictor, pVictim );

	const char *killer_weapon_name = "world";

	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING )
	{
		// special-case burning damage, since persistent burning damage may happen after attacker has switched weapons
		killer_weapon_name = "tf_weapon_flamethrower";
	}
	else if ( pScorer && pInflictor && ( pInflictor == pScorer ) )
	{
		// If the inflictor is the killer,  then it must be their current weapon doing the damage
		if ( pScorer->GetActiveWeapon() )
		{
			killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname(); 
		}
	}
	else if ( pInflictor )
	{
		killer_weapon_name = STRING( pInflictor->m_iClassname );
	}
	int proj = Q_strlen( "tf_projectile_" );
	if ( strncmp( killer_weapon_name, "tf_projectile_", proj ) == 0 )
	{
		CTFGrenadePipebombProjectile *pGrenade = dynamic_cast<CTFGrenadePipebombProjectile *>(pInflictor);
		if ( pGrenade ) 
		{
			if ( pGrenade->GetLauncher() )
				killer_weapon_name = pGrenade->GetLauncher()->GetClassname();
			else 
				killer_weapon_name = pGrenade->GetClassname();
		}
	}	
	// strip certain prefixes from inflictor's classname
	const char *prefix[] = { "tf_weapon_grenade_", "tf_weapon_", "NPC_", "func_" };
	for ( int i = 0; i< ARRAYSIZE( prefix ); i++ )
	{
		// if prefix matches, advance the string pointer past the prefix
		int len = Q_strlen( prefix[i] );
		if ( strncmp( killer_weapon_name, prefix[i], len ) == 0 )
		{
			killer_weapon_name += len;
			break;
		}
	}

	// look out for sentry rocket as weapon and map it to sentry gun, so we get the sentry death icon
	if ( 0 == Q_strcmp( killer_weapon_name, "tf_projectile_sentryrocket" ) )
	{
		killer_weapon_name = "obj_sentrygun";
	}
	return killer_weapon_name;
}

//-----------------------------------------------------------------------------
// Purpose: returns the player who assisted in the kill, or NULL if no assister
//-----------------------------------------------------------------------------
CBasePlayer *CTFGameRules::GetAssister( CBasePlayer *pVictim, CBasePlayer *pScorer, CBaseEntity *pInflictor )
{
	CTFPlayer *pTFScorer = ToTFPlayer( pScorer );
	CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
	if ( pTFScorer && pTFVictim )
	{
		// if victim killed himself, don't award an assist to anyone else, even if there was a recent damager
		if ( pTFScorer == pTFVictim )
			return NULL;

		// If a player is healing the scorer, give that player credit for the assist
		CTFPlayer *pHealer = ToTFPlayer( static_cast<CBaseEntity *>( pTFScorer->m_Shared.GetFirstHealer() ) );
		// Must be a medic to receive a healing assist, otherwise engineers get credit for assists from dispensers doing healing.
		// Also don't give an assist for healing if the inflictor was a sentry gun, otherwise medics healing engineers get assists for the engineer's sentry kills.
		if ( pHealer && ( TF_CLASS_MEDIC == pHealer->GetPlayerClass()->GetClassIndex() ) && ( NULL == dynamic_cast<CObjectSentrygun *>( pInflictor ) ) )
		{
			return pHealer;
		}

		// See who has damaged the victim 2nd most recently (most recent is the killer), and if that is within a certain time window.
		// If so, give that player an assist.  (Only 1 assist granted, to single other most recent damager.)
		CTFPlayer *pRecentDamager = GetRecentDamager( pTFVictim, 1, TF_TIME_ASSIST_KILL );
		if ( pRecentDamager && ( pRecentDamager != pScorer ) )
			return pRecentDamager;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns specifed recent damager, if there is one who has done damage
//			within the specified time period.  iDamager=0 returns the most recent
//			damager, iDamager=1 returns the next most recent damager.
//-----------------------------------------------------------------------------
CTFPlayer *CTFGameRules::GetRecentDamager( CTFPlayer *pVictim, int iDamager, float flMaxElapsed )
{
	Assert( iDamager < MAX_DAMAGER_HISTORY );

	DamagerHistory_t &damagerHistory = pVictim->GetDamagerHistory( iDamager );
	if ( ( NULL != damagerHistory.hDamager ) && ( gpGlobals->curtime - damagerHistory.flTimeDamage <= flMaxElapsed ) )
	{
		CTFPlayer *pRecentDamager = ToTFPlayer( damagerHistory.hDamager );
		if ( pRecentDamager )
			return pRecentDamager;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns who should be awarded the kill
//-----------------------------------------------------------------------------
CBasePlayer *CTFGameRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim )
{
	if ( ( pKiller == pVictim ) && ( pKiller == pInflictor ) )
	{
		// If this was an explicit suicide, see if there was a damager within a certain time window.  If so, award this as a kill to the damager.
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim )
		{
			CTFPlayer *pRecentDamager = GetRecentDamager( pTFVictim, 0, TF_TIME_SUICIDE_KILL_CREDIT );
			if ( pRecentDamager )
				return pRecentDamager;
		}
	}

	return BaseClass::GetDeathScorer( pKiller, pInflictor, pVictim );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//			*pKiller - 
//			*pInflictor - 
//-----------------------------------------------------------------------------
void CTFGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	int killer_ID = 0;

	// Find the killer & the scorer
	CTFPlayer *pTFPlayerVictim = ToTFPlayer( pVictim );
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor, pVictim );
	CTFPlayer *pAssister = ToTFPlayer( GetAssister( pVictim, pScorer, pInflictor ) );

	// Work out what killed the player, and send a message to all clients about it
	const char *killer_weapon_name = GetKillingWeaponName( info, pTFPlayerVictim );

	if ( pScorer )	// Is the killer a client?
	{
		killer_ID = pScorer->GetUserID();
	}

	IGameEvent * event = gameeventmanager->CreateEvent( "player_death" );

	if ( event )
	{
		event->SetInt( "userid", pVictim->GetUserID() );
		event->SetInt( "attacker", killer_ID );
		event->SetInt( "assister", pAssister ? pAssister->GetUserID() : -1 );
		event->SetString( "weapon", killer_weapon_name );
		event->SetInt( "damagebits", info.GetDamageType() );
		event->SetInt( "customkill", info.GetDamageCustom() );
		event->SetInt( "priority", 7 );	// HLTV event priority, not transmitted
		if ( pTFPlayerVictim->GetDeathFlags() & TF_DEATH_DOMINATION )
		{
			event->SetInt( "dominated", 1 );
		}
		if ( pTFPlayerVictim->GetDeathFlags() & TF_DEATH_ASSISTER_DOMINATION )
		{
			event->SetInt( "assister_dominated", 1 );
		}
		if ( pTFPlayerVictim->GetDeathFlags() & TF_DEATH_REVENGE )
		{
			event->SetInt( "revenge", 1 );
		}
		if ( pTFPlayerVictim->GetDeathFlags() & TF_DEATH_ASSISTER_REVENGE )
		{
			event->SetInt( "assister_revenge", 1 );
		}

		gameeventmanager->FireEvent( event );
	}		
}

void CTFGameRules::ClientDisconnected( edict_t *pClient )
{
	// clean up anything they left behind
	CTFPlayer *pPlayer = ToTFPlayer( GetContainingEntity( pClient ) );
	if ( pPlayer )
	{
		pPlayer->TeamFortress_ClientDisconnected();
	}

	// are any of the spies disguising as this player?
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTemp && pTemp != pPlayer )
		{
			if ( pTemp->m_Shared.GetDisguiseTarget() == pPlayer )
			{
				// choose someone else...
				pTemp->m_Shared.FindDisguiseTarget();
			}
		}
	}

	BaseClass::ClientDisconnected( pClient );
}

// Falling damage stuff.
#define TF_PLAYER_MAX_SAFE_FALL_SPEED	650		

float CTFGameRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	if ( pPlayer->m_Local.m_flFallVelocity > TF_PLAYER_MAX_SAFE_FALL_SPEED )
	{
		// Old TFC damage formula
		float flFallDamage = 5 * (pPlayer->m_Local.m_flFallVelocity / 300);

		// Fall damage needs to scale according to the player's max health, or
		// it's always going to be much more dangerous to weaker classes than larger.
		float flRatio = (float)pPlayer->GetMaxHealth() / 100.0;
		flFallDamage *= flRatio;

		flFallDamage *= random->RandomFloat( 0.8, 1.2 );

		return flFallDamage;
	}

	// Fall caused no damage
	return 0;
}

void CTFGameRules::SendWinPanelInfo( void )
{
	IGameEvent *winEvent = gameeventmanager->CreateEvent( "teamplay_win_panel" );

	if ( winEvent )
	{
		int iBlueScore = GetGlobalTeam( TF_TEAM_BLUE )->GetScore();
		int iRedScore = GetGlobalTeam( TF_TEAM_RED )->GetScore();
		int iMercenaryScore = GetGlobalTeam( TF_TEAM_MERCENARY )->GetScore();
		int iBlueScorePrev = iBlueScore;
		int iRedScorePrev = iRedScore;
		int iMercenaryScorePrev = iMercenaryScore;

		bool bRoundComplete = m_bForceMapReset || ( IsGameUnderTimeLimit() && ( GetTimeLeft() <= 0 ) );

		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		bool bScoringPerCapture = ( pMaster ) ? ( pMaster->ShouldScorePerCapture() ) : false;

		if ( bRoundComplete && !bScoringPerCapture )
		{
			// if this is a complete round, calc team scores prior to this win
			switch ( m_iWinningTeam )
			{
			case TF_TEAM_BLUE:
				iBlueScorePrev = ( iBlueScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iBlueScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
				break;
			case TF_TEAM_RED:
				iRedScorePrev = ( iRedScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iRedScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
				break;
			case TEAM_UNASSIGNED:
				break;	// stalemate; nothing to do
			case TF_TEAM_MERCENARY:
				iMercenaryScorePrev = ( iMercenaryScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iMercenaryScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
				break;
			}
		}
			
		winEvent->SetInt( "panel_style", WINPANEL_BASIC );
		winEvent->SetInt( "winning_team", m_iWinningTeam );
		winEvent->SetInt( "winreason", m_iWinReason );
		winEvent->SetString( "cappers",  ( m_iWinReason == WINREASON_ALL_POINTS_CAPTURED || m_iWinReason == WINREASON_FLAG_CAPTURE_LIMIT ) ?
			m_szMostRecentCappers : "" );
		winEvent->SetInt( "flagcaplimit", tf_flag_caps_per_round.GetInt() );
		winEvent->SetInt( "blue_score", iBlueScore );
		winEvent->SetInt( "red_score", iRedScore );
		winEvent->SetInt( "blue_score_prev", iBlueScorePrev );
		winEvent->SetInt( "red_score_prev", iRedScorePrev );
		winEvent->SetInt( "round_complete", bRoundComplete );

		CTFPlayerResource *pResource = dynamic_cast< CTFPlayerResource * >( g_pPlayerResource );
		if ( !pResource )
			return;
 
		// determine the 3 players on winning team who scored the most points this round

		// build a vector of players & round scores
		CUtlVector<PlayerRoundScore_t> vecPlayerScore;
		int iPlayerIndex;
		for( iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( !pTFPlayer || !pTFPlayer->IsConnected() )
				continue;
			// filter out spectators and, if not stalemate, all players not on winning team
			int iPlayerTeam = pTFPlayer->GetTeamNumber();
			if ( ( iPlayerTeam < FIRST_GAME_TEAM ) || ( m_iWinningTeam != TEAM_UNASSIGNED && ( m_iWinningTeam != iPlayerTeam ) ) )
				continue;

			int iRoundScore = 0, iTotalScore = 0;
			PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( pTFPlayer );
			if ( pStats )
			{
				iRoundScore = CalcPlayerScore( &pStats->statsCurrentRound );
				iTotalScore = CalcPlayerScore( &pStats->statsAccumulated );
			}
			PlayerRoundScore_t &playerRoundScore = vecPlayerScore[vecPlayerScore.AddToTail()];
			playerRoundScore.iPlayerIndex = iPlayerIndex;
			playerRoundScore.iRoundScore = iRoundScore;
			playerRoundScore.iTotalScore = iTotalScore;
		}
		// sort the players by round score
		vecPlayerScore.Sort( PlayerRoundScoreSortFunc );

		// set the top (up to) 3 players by round score in the event data
		int numPlayers = min( 3, vecPlayerScore.Count() );
		for ( int i = 0; i < numPlayers; i++ )
		{
			// only include players who have non-zero points this round; if we get to a player with 0 round points, stop
			if ( 0 == vecPlayerScore[i].iRoundScore )
				break;

			// set the player index and their round score in the event
			char szPlayerIndexVal[64]="", szPlayerScoreVal[64]="";
			Q_snprintf( szPlayerIndexVal, ARRAYSIZE( szPlayerIndexVal ), "player_%d", i+ 1 );
			Q_snprintf( szPlayerScoreVal, ARRAYSIZE( szPlayerScoreVal ), "player_%d_points", i+ 1 );
			winEvent->SetInt( szPlayerIndexVal, vecPlayerScore[i].iPlayerIndex );
			winEvent->SetInt( szPlayerScoreVal, vecPlayerScore[i].iRoundScore );				
		}

		if ( !bRoundComplete && ( TEAM_UNASSIGNED != m_iWinningTeam ) )
		{
			// if this was not a full round ending, include how many mini-rounds remain for winning team to win
			if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
			{
				winEvent->SetInt( "rounds_remaining", g_hControlPointMasters[0]->CalcNumRoundsRemaining( m_iWinningTeam ) );
			}
		}

		// Send the event
		gameeventmanager->FireEvent( winEvent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sorts players by round score
//-----------------------------------------------------------------------------
int CTFGameRules::PlayerRoundScoreSortFunc( const PlayerRoundScore_t *pRoundScore1, const PlayerRoundScore_t *pRoundScore2 )
{
	// sort first by round score	
	if ( pRoundScore1->iRoundScore != pRoundScore2->iRoundScore )
		return pRoundScore2->iRoundScore - pRoundScore1->iRoundScore;

	// if round scores are the same, sort next by total score
	if ( pRoundScore1->iTotalScore != pRoundScore2->iTotalScore )
		return pRoundScore2->iTotalScore - pRoundScore1->iTotalScore;

	// if scores are the same, sort next by player index so we get deterministic sorting
	return ( pRoundScore2->iPlayerIndex - pRoundScore1->iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the teamplay_round_win event is about to be sent, gives
//			this method a chance to add more data to it
//-----------------------------------------------------------------------------
void CTFGameRules::FillOutTeamplayRoundWinEvent( IGameEvent *event )
{
	// determine the losing team
	int iLosingTeam;

	switch( event->GetInt( "team" ) )
	{
	case TF_TEAM_RED:
		iLosingTeam = TF_TEAM_BLUE + TF_TEAM_MERCENARY;
		break;
	case TF_TEAM_BLUE:
		iLosingTeam = TF_TEAM_RED + TF_TEAM_MERCENARY;
		break;
	case TF_TEAM_MERCENARY:
		iLosingTeam = TF_TEAM_RED + TF_TEAM_BLUE;
	break;
	case TEAM_UNASSIGNED:
	default:
		iLosingTeam = TEAM_UNASSIGNED;
		break;
	}

	// set the number of caps that team got any time during the round
	event->SetInt( "losing_team_num_caps", m_iNumCaps[iLosingTeam] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetupSpawnPointsForRound( void )
{
	if ( !g_hControlPointMasters.Count() || !g_hControlPointMasters[0] || !g_hControlPointMasters[0]->PlayingMiniRounds() )
		return;

	CTeamControlPointRound *pCurrentRound = g_hControlPointMasters[0]->GetCurrentRound();
	if ( !pCurrentRound )
	{
		return;
	}

	// loop through the spawn points in the map and find which ones are associated with this round or the control points in this round
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
	while( pSpot )
	{
		CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn*>(pSpot);

		if ( pTFSpawn )
		{
			CHandle<CTeamControlPoint> hControlPoint = pTFSpawn->GetControlPoint();
			CHandle<CTeamControlPointRound> hRoundBlue = pTFSpawn->GetRoundBlueSpawn();
			CHandle<CTeamControlPointRound> hRoundRed = pTFSpawn->GetRoundRedSpawn();
			CHandle<CTeamControlPointRound> hRoundMercenary = pTFSpawn->GetRoundMercenarySpawn();
			
			if ( hControlPoint && pCurrentRound->IsControlPointInRound( hControlPoint ) )
			{
				// this spawn is associated with one of our control points
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( hControlPoint->GetOwner() );
			}
			else if ( hRoundBlue && ( hRoundBlue == pCurrentRound ) )
			{
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( TF_TEAM_BLUE );
			}
			else if ( hRoundRed && ( hRoundRed == pCurrentRound ) )
			{
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( TF_TEAM_RED );
			}
			else if ( hRoundMercenary && ( hRoundMercenary == pCurrentRound ) )
			{
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( TF_TEAM_MERCENARY );
			}
			else
			{
				// this spawn isn't associated with this round or the control points in this round
				pTFSpawn->SetDisabled( true );
			}
		}

		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
	}
}


int CTFGameRules::SetCurrentRoundStateBitString( void )
{
	m_iPrevRoundState = m_iCurrentRoundState;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	if ( !pMaster )
	{
		return 0;
	}

	int iState = 0;

	for ( int i=0; i<pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		if ( pPoint->GetOwner() == TF_TEAM_BLUE )
		{
			// Set index to 1 for the point being owned by blue
			iState |= ( 1<<i );
		}
	}

	m_iCurrentRoundState = iState;

	return iState;
}


void CTFGameRules::SetMiniRoundBitMask( int iMask )
{
	m_iCurrentMiniRoundMask = iMask;
}

//-----------------------------------------------------------------------------
// Purpose: NULL pPlayer means show the panel to everyone
//-----------------------------------------------------------------------------
void CTFGameRules::ShowRoundInfoPanel( CTFPlayer *pPlayer /* = NULL */ )
{
	KeyValues *data = new KeyValues( "data" );

	if ( m_iCurrentRoundState < 0 )
	{
		// Haven't set up the round state yet
		return;
	}

	// if prev and cur are equal, we are starting from a fresh round
	if ( m_iPrevRoundState >= 0 && pPlayer == NULL )	// we have data about a previous state
	{
		data->SetInt( "prev", m_iPrevRoundState );
	}
	else
	{
		// don't send a delta if this is just to one player, they are joining mid-round
		data->SetInt( "prev", m_iCurrentRoundState );	
	}

	data->SetInt( "cur", m_iCurrentRoundState );

	// get bitmask representing the current miniround
	data->SetInt( "round", m_iCurrentMiniRoundMask );

	if ( pPlayer )
	{
		pPlayer->ShowViewPortPanel( PANEL_ROUNDINFO, true, data );
	}
	else
	{
		for ( int i = 1;  i <= MAX_PLAYERS; i++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( pTFPlayer && pTFPlayer->IsReadyToPlay() )
			{
				pTFPlayer->ShowViewPortPanel( PANEL_ROUNDINFO, true, data );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::TimerMayExpire( void )
{
	// Prevent timers expiring while control points are contested
	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	for ( int iPoint = 0; iPoint < iNumControlPoints; iPoint++ )
	{
		if ( ObjectiveResource()->GetCappingTeam( iPoint ) )
		{
			// HACK: Fix for some maps adding time to the clock 0.05s after CP is capped.
			m_flTimerMayExpireAt = gpGlobals->curtime + 0.1f;
			return false;
		}
	}

	if ( m_flTimerMayExpireAt >= gpGlobals->curtime )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RoundRespawn( void )
{
	// remove any buildings, grenades, rockets, etc. the player put into the world
	for ( int i = 1;  i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			pPlayer->TeamFortress_RemoveEverythingFromWorld();
		}
	}

	// reset the flag captures
	int nTeamCount = TFTeamMgr()->GetTeamCount();
	for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
		if ( !pTeam )
			continue;

		pTeam->SetFlagCaptures( 0 );
	}

	CTF_GameStats.ResetRoundStats();

	BaseClass::RoundRespawn();

	// ** AFTER WE'VE BEEN THROUGH THE ROUND RESPAWN, SHOW THE ROUNDINFO PANEL
	if ( !IsInWaitingForPlayers() )
	{
		ShowRoundInfoPanel();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InternalHandleTeamWin( int iWinningTeam )
{
	// remove any spies' disguises and make them visible (for the losing team only)
	// and set the speed for both teams (winners get a boost and losers have reduced speed)
	for ( int i = 1;  i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			if ( pPlayer->GetTeamNumber() > LAST_SHARED_TEAM )
			{
				if ( pPlayer->GetTeamNumber() != iWinningTeam )
				{
					pPlayer->RemoveInvisibility();
//					pPlayer->RemoveDisguise();

					if ( pPlayer->HasTheFlag() )
					{
						pPlayer->DropFlag();
					}
				}

				pPlayer->TeamFortress_SetSpeed();
			}
		}
	}

	// disable any sentry guns the losing team has built
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "obj_sentrygun" ) ) != NULL )
	{
		CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun *>( pEnt );
		if ( pSentry )
		{
			if ( pSentry->GetTeamNumber() != iWinningTeam )
			{
				pSentry->SetDisabled( true );
			}
		}
	}

	if ( m_bForceMapReset )
	{
		m_iPrevRoundState = -1;
		m_iCurrentRoundState = -1;
		m_iCurrentMiniRoundMask = 0;
	}
}

// sort function for the list of players that we're going to use to scramble the teams
int ScramblePlayersSort( CTFPlayer* const *p1, CTFPlayer* const *p2 )
{
	CTFPlayerResource *pResource = dynamic_cast< CTFPlayerResource * >( g_pPlayerResource );

	if ( pResource )
	{
		// check the priority
		if ( pResource->GetTotalScore( (*p2)->entindex() ) > pResource->GetTotalScore( (*p1)->entindex() ) )
		{
			return 1;
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleScrambleTeams( void )
{
	int i = 0;
	CTFPlayer *pTFPlayer = NULL;
	CUtlVector<CTFPlayer *> pListPlayers;

	// add all the players (that are on blue or red) to our temp list
	for ( i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTFPlayer && ( pTFPlayer->GetTeamNumber() >= TF_TEAM_RED ) )
		{
			pListPlayers.AddToHead( pTFPlayer );
		}
	}

	// sort the list
	pListPlayers.Sort( ScramblePlayersSort );

	// loop through and put everyone on Spectator to clear the teams (or the autoteam step won't work correctly)
	for ( i = 0 ; i < pListPlayers.Count() ; i++ )
	{
		pTFPlayer = pListPlayers[i];

		if ( pTFPlayer )
		{
			pTFPlayer->ForceChangeTeam( TEAM_SPECTATOR );
		}
	}

	// loop through and auto team everyone
	for ( i = 0 ; i < pListPlayers.Count() ; i++ )
	{
		pTFPlayer = pListPlayers[i];

		if ( pTFPlayer )
		{
			pTFPlayer->ForceChangeTeam( TF_TEAM_AUTOASSIGN );
		}
	}
	
	ResetTeamsRoundWinTracking();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleSwitchTeams( void )
{
	int i = 0;

	// respawn the players
	for ( i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->TeamFortress_RemoveEverythingFromWorld();

			// Ignore players who aren't on an active team
			if ( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE && pPlayer->GetTeamNumber() != TF_TEAM_MERCENARY)
			{
				continue;
			}

			if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_BLUE );
			}
			else if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_RED );
			}
		}
	}

	// switch the team scores
	CTFTeam *pRedTeam = GetGlobalTFTeam( TF_TEAM_RED );
	CTFTeam *pBlueTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	if ( pRedTeam && pBlueTeam )
	{
		int nRed = pRedTeam->GetScore();
		int nBlue = pBlueTeam->GetScore();

		pRedTeam->SetScore( nBlue );
		pBlueTeam->SetScore( nRed );
	}
}

bool CTFGameRules::CanChangeClassInStalemate( void ) 
{ 
	return (gpGlobals->curtime < (m_flStalemateStartTime + tf_stalematechangeclasstime.GetFloat())); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetRoundOverlayDetails( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	if ( pMaster && pMaster->PlayingMiniRounds() )
	{
		CTeamControlPointRound *pRound = pMaster->GetCurrentRound();

		if ( pRound )
		{
			CHandle<CTeamControlPoint> pRedPoint = pRound->GetPointOwnedBy( TF_TEAM_RED );
			CHandle<CTeamControlPoint> pBluePoint = pRound->GetPointOwnedBy( TF_TEAM_BLUE );

			// do we have opposing points in this round?
			if ( pRedPoint && pBluePoint )
			{
				int iMiniRoundMask = ( 1<<pBluePoint->GetPointIndex() ) | ( 1<<pRedPoint->GetPointIndex() );
				SetMiniRoundBitMask( iMiniRoundMask );
			}
			else
			{
				SetMiniRoundBitMask( 0 );
			}

			SetCurrentRoundStateBitString();
		}
	}

	BaseClass::SetRoundOverlayDetails();
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether a team should score for each captured point
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldScorePerRound( void )
{ 
	bool bRetVal = true;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster && pMaster->ShouldScorePerCapture() )
	{
		bRetVal = false;
	}

	return bRetVal;
}

#endif  // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetFarthestOwnedControlPoint( int iTeam, bool bWithSpawnpoints )
{
	int iOwnedEnd = ObjectiveResource()->GetBaseControlPointForTeam( iTeam );
	if ( iOwnedEnd == -1 )
		return -1;

	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	int iWalk = 1;
	int iEnemyEnd = iNumControlPoints-1;
	if ( iOwnedEnd != 0 )
	{
		iWalk = -1;
		iEnemyEnd = 0;
	}

	// Walk towards the other side, and find the farthest owned point that has spawn points
	int iFarthestPoint = iOwnedEnd;
	for ( int iPoint = iOwnedEnd; iPoint != iEnemyEnd; iPoint += iWalk )
	{
		// If we've hit a point we don't own, we're done
		if ( ObjectiveResource()->GetOwningTeam( iPoint ) != iTeam )
			break;

		if ( bWithSpawnpoints && !m_bControlSpawnsPerTeam[iTeam][iPoint] )
			continue;

		iFarthestPoint = iPoint;
	}

	return iFarthestPoint;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::TeamMayCapturePoint( int iTeam, int iPointIndex ) 
{ 
	// point capturing allowed?
	if ( !PointsMayBeCaptured() )
		return false;

	// locked?
	if ( ObjectiveResource()->GetCPLocked( iPointIndex ) )
		return false;

	// don't run further logic in domination
	if ( !tf_caplinear.GetBool() || IsDOMGamemode() )
		return true; 

	// Any previous points necessary?
	int iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint( iPointIndex, iTeam, 0 );

	// Points set to require themselves are always cappable 
	if ( iPointNeeded == iPointIndex )
		return true;

	// No required points specified? Require all previous points.
	if ( iPointNeeded == -1 )
	{
		if ( !ObjectiveResource()->PlayingMiniRounds() )
		{
			// No custom previous point, team must own all previous points
			int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, false );
			return (abs(iFarthestPoint - iPointIndex) <= 1);
		}
		else
		{
			// No custom previous point, team must own all previous points in the current mini-round
			//tagES TFTODO: need to figure out a good algorithm for this
			return true;
		}
	}

	// Loop through each previous point and see if the team owns it
	for ( int iPrevPoint = 0; iPrevPoint < MAX_PREVIOUS_POINTS; iPrevPoint++ )
	{
		int iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint( iPointIndex, iTeam, iPrevPoint );
		if ( iPointNeeded != -1 )
		{
			if ( ObjectiveResource()->GetOwningTeam( iPointNeeded ) != iTeam )
				return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerMayCapturePoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason /* = NULL */, int iMaxReasonLength /* = 0 */ )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( !pTFPlayer )
	{
		return false;
	}

	// Disguised and invisible spies cannot capture points
	if ( pTFPlayer->m_Shared.InCondInvis() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_stealthed" );
		}
		return false;
	}

	if ( pTFPlayer->m_Shared.InCondUber() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}
		return false;
	}

 	if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_disguised" );
		}
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerMayBlockPoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason, int iMaxReasonLength )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return false;

	// Invuln players can block points
	if ( pTFPlayer->m_Shared.InCondUber() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Calculates score for player
//-----------------------------------------------------------------------------
int CTFGameRules::CalcPlayerScore( RoundStats_t *pRoundStats )
{
	int iScore =	( pRoundStats->m_iStat[TFSTAT_KILLS] * TF_SCORE_KILL ) + 
					( pRoundStats->m_iStat[TFSTAT_CAPTURES] * TF_SCORE_CAPTURE ) + 
					( pRoundStats->m_iStat[TFSTAT_DEFENSES] * TF_SCORE_DEFEND ) + 
					( pRoundStats->m_iStat[TFSTAT_BUILDINGSDESTROYED] * TF_SCORE_DESTROY_BUILDING ) + 
					( pRoundStats->m_iStat[TFSTAT_HEADSHOTS] * TF_SCORE_HEADSHOT ) + 
					( pRoundStats->m_iStat[TFSTAT_BACKSTABS] * TF_SCORE_BACKSTAB ) + 
					( pRoundStats->m_iStat[TFSTAT_HEALING] / TF_SCORE_HEAL_HEALTHUNITS_PER_POINT ) +  
					( pRoundStats->m_iStat[TFSTAT_KILLASSISTS] / TF_SCORE_KILL_ASSISTS_PER_POINT ) + 
					( pRoundStats->m_iStat[TFSTAT_TELEPORTS] / TF_SCORE_TELEPORTS_PER_POINT ) +
					( pRoundStats->m_iStat[TFSTAT_INVULNS] / TF_SCORE_INVULN ) +
					( pRoundStats->m_iStat[TFSTAT_REVENGE] / TF_SCORE_REVENGE );
	if ( TFGameRules()->IsDMGamemode() && !TFGameRules()->DontCountKills() )
		iScore = ( pRoundStats->m_iStat[TFSTAT_KILLS] * TF_SCORE_KILL );
	return max( iScore, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::IsBirthday( void )
{
	if ( IsX360() )
		return false;

	if ( m_iBirthdayMode == BIRTHDAY_RECALCULATE )
	{
		m_iBirthdayMode = BIRTHDAY_OFF;
		if ( tf_birthday.GetBool() )
		{
			m_iBirthdayMode = BIRTHDAY_ON;
		}
		else
		{
			time_t ltime = time(0);
			const time_t *ptime = &ltime;
			struct tm *today = localtime( ptime );
			if ( today )
			{
				if ( today->tm_mon == 7 && today->tm_mday == 24 )
				{
					m_iBirthdayMode = BIRTHDAY_ON;
				}
			}
		}
	}

	return ( m_iBirthdayMode == BIRTHDAY_ON );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	// The smaller number is always first
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		int tmp = collisionGroup0;
		collisionGroup0 = collisionGroup1;
		collisionGroup1 = tmp;
	}
	
	//Don't stand on COLLISION_GROUP_WEAPONs
	if( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	// Don't stand on projectiles
	if( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_PROJECTILE )
	{
		return false;
	}

	// Rockets need to collide with players when they hit, but
	// be ignored by player movement checks
	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return true;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_WEAPON ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;
		
	// rockets do not collide with each other (tf_point_weapon_mimic)
	if ( ( collisionGroup0 == TFCOLLISION_GROUP_ROCKETS ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;

	// Grenades don't collide with players. They handle collision while flying around manually.
	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && 
		( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) && 
		( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
		return false;

	// Respawn rooms only collide with players
	if ( collisionGroup1 == TFCOLLISION_GROUP_RESPAWNROOMS )
		return ( collisionGroup0 == COLLISION_GROUP_PLAYER ) || ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT );
	
/*	if ( collisionGroup0 == COLLISION_GROUP_PLAYER )
	{
		// Players don't collide with objects or other players
		if ( collisionGroup1 == COLLISION_GROUP_PLAYER  )
			 return false;
 	}

	if ( collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		// This is only for probing, so it better not be on both sides!!!
		Assert( collisionGroup0 != COLLISION_GROUP_PLAYER_MOVEMENT );

		// No collide with players any more
		// Nor with objects or grenades
		switch ( collisionGroup0 )
		{
		default:
			break;
		case COLLISION_GROUP_PLAYER:
			return false;
		}
	}
*/
	// don't want caltrops and other grenades colliding with each other
	// caltops getting stuck on other caltrops, etc.)
	if ( ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES ) && 
		 ( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
	{
		return false;
	}


	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
	{
		return false;
	}

	if ( collisionGroup0 == COLLISION_GROUP_PLAYER &&
		collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
	{
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}

//-----------------------------------------------------------------------------
// Purpose: Return the value of this player towards capturing a point
//-----------------------------------------------------------------------------
int	CTFGameRules::GetCaptureValueForPlayer( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer->GetPlayerClass()->GetCapNumber() > 1)
	{
		if ( mp_capstyle.GetInt() == 1 )
		{
			// Scouts count for 2 people in timebased capping.
			return pTFPlayer->GetPlayerClass()->GetCapNumber();
		}
		else
		{
			// Scouts can cap all points on their own.
			return pTFPlayer->GetPlayerClass()->GetCapNumber() * 5;
		}
	}

	return BaseClass::GetCaptureValueForPlayer( pPlayer );
}

bool CTFGameRules::UsesDMBuckets()
{
	return ( of_multiweapons.GetBool() && IsDMGamemode() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetTimeLeft( void )
{
	float flTimeLimit = mp_timelimit.GetInt() * 60;

	Assert( flTimeLimit > 0 && "Should not call this function when !IsGameUnderTimeLimit" );

	float flMapChangeTime = m_flMapResetTime + flTimeLimit;

	return ( (int)(flMapChangeTime - gpGlobals->curtime) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetMaxHunted( int iTeamNumber )
{
	switch ( iTeamNumber )
	{
		case TF_TEAM_RED:
			return m_nMaxHunted_red;
			break;
		case TF_TEAM_BLUE:
			return m_nMaxHunted_blu;
			break;
		default:
			return -1;
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetHuntedCount( int iTeamNumber )
{
	switch ( iTeamNumber )
	{
		case TF_TEAM_RED:
			return m_nHuntedCount_red;
			break;
		case TF_TEAM_BLUE:
			return m_nHuntedCount_blu;
			break;
		default:
			return -2;
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if ( !Q_strcmp( eventName, "teamplay_point_captured" ) )
	{
#ifdef GAME_DLL
		RecalculateControlPointState();

		// keep track of how many times each team caps
		int iTeam = event->GetInt( "team" );
		Assert( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT );
		m_iNumCaps[iTeam]++;

		// award a capture to all capping players
		const char *cappers = event->GetString( "cappers" );

		Q_strncpy( m_szMostRecentCappers, cappers, ARRAYSIZE( m_szMostRecentCappers ) );	
		for ( int i =0; i < Q_strlen( cappers ); i++ )
		{
			int iPlayerIndex = (int) cappers[i];
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( pPlayer )
			{
				CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );				
			}
		}
#endif
	}
	else if ( !Q_strcmp( eventName, "teamplay_capture_blocked" ) )
	{
#ifdef GAME_DLL
		int iPlayerIndex = event->GetInt( "blocker" );
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		CTF_GameStats.Event_PlayerDefendedPoint( pPlayer );
#endif
	}	
	else if ( !Q_strcmp( eventName, "teamplay_round_win" ) )
	{
#ifdef GAME_DLL
		int iWinningTeam = event->GetInt( "team" );
		bool bFullRound = event->GetBool( "full_round" );
		float flRoundTime = event->GetFloat( "round_time" );
		bool bWasSuddenDeath = event->GetBool( "was_sudden_death" );
		CTF_GameStats.Event_RoundEnd( iWinningTeam, bFullRound, flRoundTime, bWasSuddenDeath );
#endif
	}
	else if ( !Q_strcmp( eventName, "teamplay_point_unlocked" ) )
	{
#ifdef GAME_DLL
		// if this is an unlock event and we're in arena, fire OnCapEnabled		
		CTFLogicArena *pArena = dynamic_cast<CTFLogicArena *>( gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );

		if ( pArena )
			pArena->m_OnCapEnabled.FireOutput( NULL, pArena );
#endif
	}
	else if ( !Q_strcmp( eventName, "teamplay_flag_event" ) )
	{
#ifdef GAME_DLL
		// if this is a capture event, remember the player who made the capture		
		int iEventType = event->GetInt( "eventtype" );
		if ( TF_FLAGEVENT_CAPTURE == iEventType )
		{
			int iPlayerIndex = event->GetInt( "player" );
			m_szMostRecentCappers[0] = iPlayerIndex;
			m_szMostRecentCappers[1] = 0;
		}
#endif
	}
	else if ( !Q_strcmp( eventName, "teamplay_point_unlocked" ) )
	{
#ifdef GAME_DLL
		// unlock my point	
		CTFLogicArena *pArena = dynamic_cast< CTFLogicArena * >(gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );
		if ( pArena )
			pArena->OnCapEnabled();
#endif
	}
#ifdef CLIENT_DLL
	else if ( !Q_strcmp( eventName, "game_newmap" ) )
	{
		m_iBirthdayMode = BIRTHDAY_RECALCULATE;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Init ammo definitions
//-----------------------------------------------------------------------------

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			1	

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;
		
		// Start at 1 here and skip the dummy ammo type to make CAmmoDef use the same indices
		// as our #defines.
		for ( int i=1; i < TF_AMMO_COUNT; i++ )
		{
			def.AddAmmoType( g_aAmmoNames[i], DMG_BULLET, TRACER_LINE, 0, 0, "ammo_max", 2400, 10, 14 );
			Assert( def.Index( g_aAmmoNames[i] ) == i );
		}

		//HACKHACKHACKHACK AAAAAAAAAA
#define DMG_SNIPER DMG_CRITICAL

		

		//HL2 AMMO TYPES FOR BACKWARDS COMPAT
		def.AddAmmoType("AR2",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	"sk_plr_dmg_ar2",			"sk_npc_dmg_ar2",			"sk_max_ar2",			BULLET_IMPULSE(200, 1225), 0 );
		def.AddAmmoType("AlyxGun",			DMG_BULLET,					TRACER_LINE,			"sk_plr_dmg_alyxgun",		"sk_npc_dmg_alyxgun",		"sk_max_alyxgun",		BULLET_IMPULSE(200, 1225), 0 );
		def.AddAmmoType("Pistol",			DMG_BULLET,					TRACER_LINE_AND_WHIZ,	"sk_plr_dmg_pistol",		"sk_npc_dmg_pistol",		"sk_max_pistol",		BULLET_IMPULSE(200, 1225), 0 );
		def.AddAmmoType("SMG1",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	"sk_plr_dmg_smg1",			"sk_npc_dmg_smg1",			"sk_max_smg1",			BULLET_IMPULSE(200, 1225), 0 );
		def.AddAmmoType("357",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	"sk_plr_dmg_357",			"sk_npc_dmg_357",			"sk_max_357",			BULLET_IMPULSE(800, 5000), 0 );
		def.AddAmmoType("XBowBolt",			DMG_BULLET,					TRACER_LINE,			"sk_plr_dmg_crossbow",		"sk_npc_dmg_crossbow",		"sk_max_crossbow",		BULLET_IMPULSE(800, 8000), 0 );

		def.AddAmmoType("Buckshot",			DMG_BULLET | DMG_BUCKSHOT,	TRACER_LINE,			"sk_plr_dmg_buckshot",		"sk_npc_dmg_buckshot",		"sk_max_buckshot",		BULLET_IMPULSE(400, 1200), 0 );
		def.AddAmmoType("RPG_Round",		DMG_BURN,					TRACER_NONE,			"sk_plr_dmg_rpg_round",		"sk_npc_dmg_rpg_round",		"sk_max_rpg_round",		0, 0 );
		def.AddAmmoType("SMG1_Grenade",		DMG_BURN,					TRACER_NONE,			"sk_plr_dmg_smg1_grenade",	"sk_npc_dmg_smg1_grenade",	"sk_max_smg1_grenade",	0, 0 );
		def.AddAmmoType("SniperRound",		DMG_BULLET | DMG_SNIPER,	TRACER_NONE,			"sk_plr_dmg_sniper_round",	"sk_npc_dmg_sniper_round",	"sk_max_sniper_round",	BULLET_IMPULSE(650, 6000), 0 );
		def.AddAmmoType("SniperPenetratedRound", DMG_BULLET | DMG_SNIPER, TRACER_NONE,			"sk_dmg_sniper_penetrate_plr", "sk_dmg_sniper_penetrate_npc", "sk_max_sniper_round", BULLET_IMPULSE(150, 6000), 0 );
		def.AddAmmoType("Grenade",			DMG_BURN,					TRACER_NONE,			"sk_plr_dmg_grenade",		"sk_npc_dmg_grenade",		"sk_max_grenade",		0, 0);
		def.AddAmmoType("Thumper",			DMG_SONIC,					TRACER_NONE,			10, 10, 2, 0, 0 );
		def.AddAmmoType("Gravity",			DMG_CLUB,					TRACER_NONE,			0,	0, 8, 0, 0 );
//		def.AddAmmoType("Extinguisher",		DMG_BURN,					TRACER_NONE,			0,	0, 100, 0, 0 );
		def.AddAmmoType("Battery",			DMG_CLUB,					TRACER_NONE,			NULL, NULL, NULL, 0, 0 );
		def.AddAmmoType("GaussEnergy",		DMG_SHOCK,					TRACER_NONE,			"sk_jeep_gauss_damage",		"sk_jeep_gauss_damage", "sk_max_gauss_round", BULLET_IMPULSE(650, 8000), 0 ); // hit like a 10kg weight at 400 in/s
		def.AddAmmoType("CombineCannon",	DMG_BULLET,					TRACER_LINE,			"sk_npc_dmg_gunship_to_plr", "sk_npc_dmg_gunship", NULL, 1.5 * 750 * 12, 0 ); // hit like a 1.5kg weight at 750 ft/s
		def.AddAmmoType("AirboatGun",		DMG_AIRBOAT,				TRACER_LINE,			"sk_plr_dmg_airboat",		"sk_npc_dmg_airboat",		NULL,					BULLET_IMPULSE(10, 600), 0 );

		//=====================================================================
		// STRIDER MINIGUN DAMAGE - Pull up a chair and I'll tell you a tale.
		//
		// When we shipped Half-Life 2 in 2004, we were unaware of a bug in
		// CAmmoDef::NPCDamage() which was returning the MaxCarry field of
		// an ammotype as the amount of damage that should be done to a NPC
		// by that type of ammo. Thankfully, the bug only affected Ammo Types 
		// that DO NOT use ConVars to specify their parameters. As you can see,
		// all of the important ammotypes use ConVars, so the effect of the bug
		// was limited. The Strider Minigun was affected, though.
		//
		// According to my perforce Archeology, we intended to ship the Strider
		// Minigun ammo type to do 15 points of damage per shot, and we did. 
		// To achieve this we, unaware of the bug, set the Strider Minigun ammo 
		// type to have a maxcarry of 15, since our observation was that the 
		// number that was there before (8) was indeed the amount of damage being
		// done to NPC's at the time. So we changed the field that was incorrectly
		// being used as the NPC Damage field.
		//
		// The bug was fixed during Episode 1's development. The result of the 
		// bug fix was that the Strider was reduced to doing 5 points of damage
		// to NPC's, since 5 is the value that was being assigned as NPC damage
		// even though the code was returning 15 up to that point.
		//
		// Now as we go to ship Orange Box, we discover that the Striders in 
		// Half-Life 2 are hugely ineffective against citizens, causing big
		// problems in maps 12 and 13. 
		//
		// In order to restore balance to HL2 without upsetting the delicate 
		// balance of ep2_outland_12, I have chosen to build Episodic binaries
		// with 5 as the Strider->NPC damage, since that's the value that has
		// been in place for all of Episode 2's development. Half-Life 2 will
		// build with 15 as the Strider->NPC damage, which is how HL2 shipped
		// originally, only this time the 15 is located in the correct field
		// now that the AmmoDef code is behaving correctly.
		//
		//=====================================================================
#ifdef HL2_EPISODIC
		def.AddAmmoType("StriderMinigun",	DMG_BULLET,					TRACER_LINE,			5, 5, 15, 1.0 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED ); // hit like a 1.0kg weight at 750 ft/s
#else
		def.AddAmmoType("StriderMinigun",	DMG_BULLET,					TRACER_LINE,			5, 15,15, 1.0 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED ); // hit like a 1.0kg weight at 750 ft/s
#endif//HL2_EPISODIC

		def.AddAmmoType("StriderMinigunDirect",	DMG_BULLET,				TRACER_LINE,			2, 2, 15, 1.0 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED ); // hit like a 1.0kg weight at 750 ft/s
		def.AddAmmoType("HelicopterGun",	DMG_BULLET,					TRACER_LINE_AND_WHIZ,	"sk_npc_dmg_helicopter_to_plr", "sk_npc_dmg_helicopter",	"sk_max_smg1",	BULLET_IMPULSE(400, 1225), AMMO_FORCE_DROP_IF_CARRIED | AMMO_INTERPRET_PLRDAMAGE_AS_DAMAGE_TO_PLAYER );
		def.AddAmmoType("AR2AltFire",		DMG_DISSOLVE,				TRACER_NONE,			0, 0, "sk_max_ar2_altfire", 0, 0 );
		def.AddAmmoType("Grenade",			DMG_BURN,					TRACER_NONE,			"sk_plr_dmg_grenade",		"sk_npc_dmg_grenade",		"sk_max_grenade",		0, 0);
#ifdef HL2_EPISODIC
		def.AddAmmoType("Hopwire",			DMG_BLAST,					TRACER_NONE,			"sk_plr_dmg_grenade",		"sk_npc_dmg_grenade",		"sk_max_hopwire",		0, 0);
		def.AddAmmoType("CombineHeavyCannon",	DMG_BULLET,				TRACER_LINE,			40,	40, NULL, 10 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED ); // hit like a 10 kg weight at 750 ft/s
		def.AddAmmoType("ammo_proto1",			DMG_BULLET,				TRACER_LINE,			0, 0, 10, 0, 0 );
#endif // HL2_EPISODIC

	}

	return &def;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetTeamGoalString( int iTeam )
{
	if ( iTeam == TF_TEAM_RED )
		return m_pszTeamGoalStringRed.Get();
	if ( iTeam == TF_TEAM_BLUE )
		return m_pszTeamGoalStringBlue.Get();
	if ( iTeam == TF_TEAM_MERCENARY )
		return m_pszTeamGoalStringMercenary.Get();
	return NULL;
}

const wchar_t *CTFGameRules::GetLocalizedGameTypeName( void )
{
	wchar_t *GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_UNDEFINED]);
	if ( TFGameRules()->InGametype( TF_GAMETYPE_GG ) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_GG]);
	else if ( TFGameRules()->InGametype( TF_GAMETYPE_DM ) )
	{
		if ( TFGameRules()->InGametype( TF_GAMETYPE_TDM ) )
			GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_TDM]);
		else
			GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_DM]);
	}
	if ( TFGameRules()->InGametype( TF_GAMETYPE_CP ) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_CP]);
	if ( TFGameRules()->InGametype( TF_GAMETYPE_CTF ) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_CTF]);
	if ( TFGameRules()->InGametype( TF_GAMETYPE_ARENA ) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_ARENA]);
	if ( TFGameRules()->InGametype( TF_GAMETYPE_ESC ) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_ESC]);
	if ( TFGameRules()->InGametype(TF_GAMETYPE_PAYLOAD) && !TFGameRules()->m_bEscortOverride )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_PAYLOAD]);
	if ( TFGameRules()->InGametype( TF_GAMETYPE_COOP) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_COOP]);
	if ( TFGameRules()->InGametype( TF_GAMETYPE_DOM) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_DOM]);
	if ( TFGameRules()->InGametype( TF_GAMETYPE_INF) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_INF]);
	return GameType;
}

#ifdef GAME_DLL

	Vector MaybeDropToGround( 
							CBaseEntity *pMainEnt, 
							bool bDropToGround, 
							const Vector &vPos, 
							const Vector &vMins, 
							const Vector &vMaxs )
	{
		if ( bDropToGround )
		{
			trace_t trace;
			UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
			return trace.endpos;
		}
		else
		{
			return vPos;
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: This function can be used to find a valid placement location for an entity.
	//			Given an origin to start looking from and a minimum radius to place the entity at,
	//			it will sweep out a circle around vOrigin and try to find a valid spot (on the ground)
	//			where mins and maxs will fit.
	// Input  : *pMainEnt - Entity to place
	//			&vOrigin - Point to search around
	//			fRadius - Radius to search within
	//			nTries - Number of tries to attempt
	//			&mins - mins of the Entity
	//			&maxs - maxs of the Entity
	//			&outPos - Return point
	// Output : Returns true and fills in outPos if it found a spot.
	//-----------------------------------------------------------------------------
	bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround )
	{
		// This function moves the box out in each dimension in each step trying to find empty space like this:
		//
		//											  X  
		//							   X			  X  
		// Step 1:   X     Step 2:    XXX   Step 3: XXXXX
		//							   X 			  X  
		//											  X  
		//

		Vector mins, maxs;
		pMainEnt->CollisionProp()->WorldSpaceAABB( &mins, &maxs );
		mins -= pMainEnt->GetAbsOrigin();
		maxs -= pMainEnt->GetAbsOrigin();

		// Put some padding on their bbox.
		float flPadSize = 5;
		Vector vTestMins = mins - Vector( flPadSize, flPadSize, flPadSize );
		Vector vTestMaxs = maxs + Vector( flPadSize, flPadSize, flPadSize );

		// First test the starting origin.
		if ( UTIL_IsSpaceEmpty( pMainEnt, vOrigin + vTestMins, vOrigin + vTestMaxs ) )
		{
			outPos = MaybeDropToGround( pMainEnt, bDropToGround, vOrigin, vTestMins, vTestMaxs );
			return true;
		}

		Vector vDims = vTestMaxs - vTestMins;


		// Keep branching out until we get too far.
		int iCurIteration = 0;
		int nMaxIterations = 15;

		int offset = 0;
		do
		{
			for ( int iDim=0; iDim < 3; iDim++ )
			{
				float flCurOffset = offset * vDims[iDim];

				for ( int iSign=0; iSign < 2; iSign++ )
				{
					Vector vBase = vOrigin;
					vBase[iDim] += (iSign*2-1) * flCurOffset;

					if ( UTIL_IsSpaceEmpty( pMainEnt, vBase + vTestMins, vBase + vTestMaxs ) )
					{
						// Ensure that there is a clear line of sight from the spawnpoint entity to the actual spawn point.
						// (Useful for keeping things from spawning behind walls near a spawn point)
						trace_t tr;
						UTIL_TraceLine( vOrigin, vBase, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &tr );

						if ( tr.fraction != 1.0 )
						{
							continue;
						}

						outPos = MaybeDropToGround( pMainEnt, bDropToGround, vBase, vTestMins, vTestMaxs );
						return true;
					}
				}
			}

			++offset;
		} while ( iCurIteration++ < nMaxIterations );

		//	Warning( "EntityPlacementTest for ent %d:%s failed!\n", pMainEnt->entindex(), pMainEnt->GetClassname() );
		return false;
	}

#else // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( State_Get() == GR_STATE_STARTGAME )
	{
		m_iBirthdayMode = BIRTHDAY_RECALCULATE;
	}
}

void CTFGameRules::HandleOvertimeBegin()
{
	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pTFPlayer )
	{
		pTFPlayer->EmitSound( "Game.Overtime" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldShowTeamGoal( void )
{
	if ( State_Get() == GR_STATE_PREROUND || State_Get() == GR_STATE_RND_RUNNING || InSetup() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CTFGameRules::GetTeamGlowColor( int nTeam )
{
	float r, g, b;
	switch (nTeam)
	{
	case TF_TEAM_BLUE:
		r = 0.49f; g = 0.66f; b = 0.77f;
		break;

	case TF_TEAM_RED:
		r = 0.74f; g = 0.23f; b = 0.23f;
		break;

	case TF_TEAM_MERCENARY: //The team used in dm should use the color of the player for glow color
		r = of_color_r.GetFloat();
		g = of_color_g.GetFloat();
		b = of_color_b.GetFloat();
		if ( r < TF_GLOW_COLOR_CLAMP && g < TF_GLOW_COLOR_CLAMP && b < TF_GLOW_COLOR_CLAMP )
		{
			float maxi = max(max(r, g), b);
			maxi = TF_GLOW_COLOR_CLAMP - maxi;
			r += maxi;
			g += maxi;
			b += maxi;
		}
		r = r / 255.0f;
		g = g / 255.0f;
		b = b / 255.0f;	
		break;

	default:
		r = 0.76f; g = 0.76f; b = 0.76f;
		break;
	}

	return Vector(r, g, b);
}

#endif

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ShutdownCustomResponseRulesDicts()
{
	DestroyCustomResponseSystems();

	if ( m_ResponseRules.Count() != 0 )
	{
		int nRuleCount = m_ResponseRules.Count();
		for ( int iRule = 0; iRule < nRuleCount; ++iRule )
		{
			m_ResponseRules[iRule].m_ResponseSystems.Purge();
		}
		m_ResponseRules.Purge();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InitCustomResponseRulesDicts()
{
	MEM_ALLOC_CREDIT();

	// Clear if necessary.
	ShutdownCustomResponseRulesDicts();

	// Initialize the response rules for TF.
	m_ResponseRules.AddMultipleToTail( TF_CLASS_COUNT_ALL );

	char szName[512];
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		m_ResponseRules[iClass].m_ResponseSystems.AddMultipleToTail( MP_TF_CONCEPT_COUNT );

		for ( int iConcept = 0; iConcept < MP_TF_CONCEPT_COUNT; ++iConcept )
		{
			AI_CriteriaSet criteriaSet;
			criteriaSet.AppendCriteria( "playerclass", g_aPlayerClassNames_NonLocalized[iClass] );
			criteriaSet.AppendCriteria( "Concept", g_pszMPConcepts[iConcept] );

			// 1 point for player class and 1 point for concept.
			float flCriteriaScore = 2.0f;

			// Name.
			V_snprintf( szName, sizeof( szName ), "%s_%s\n", g_aPlayerClassNames_NonLocalized[iClass], g_pszMPConcepts[iConcept] );
			m_ResponseRules[iClass].m_ResponseSystems[iConcept] = BuildCustomResponseSystemGivenCriteria( "scripts/talker/response_rules.txt", szName, criteriaSet, flCriteriaScore );
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SendHudNotification( IRecipientFilter &filter, HudNotification_t iType )
{
	UserMessageBegin( filter, "HudNotify" );
		WRITE_BYTE( iType );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SendHudNotification( IRecipientFilter &filter, const char *pszText, const char *pszIcon, int iTeam /*= TEAM_UNASSIGNED*/ )
{
	UserMessageBegin( filter, "HudNotifyCustom" );
		WRITE_STRING( pszText );
		WRITE_STRING( pszIcon );
		WRITE_BYTE( iTeam );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: Is the player past the required delays for spawning
//-----------------------------------------------------------------------------
bool CTFGameRules::HasPassedMinRespawnTime( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( pTFPlayer && pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
		return true;

	float flMinSpawnTime = GetMinTimeWhenPlayerMaySpawn( pPlayer ); 

	return ( gpGlobals->curtime > flMinSpawnTime );
}


#endif


#ifdef CLIENT_DLL
const char *CTFGameRules::GetVideoFileForMap( bool bWithExtension /*= true*/ )
{
	char mapname[MAX_MAP_NAME];

	Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname ) );
	Q_strlower( mapname );

#ifdef _X360
	// need to remove the .360 extension on the end of the map name
	char *pExt = Q_stristr( mapname, ".360" );
	if ( pExt )
	{
		*pExt = '\0';
	}
#endif

	static char strFullpath[MAX_PATH];
	Q_strncpy( strFullpath, "media/", MAX_PATH );	// Assume we must play out of the media directory
	Q_strncat( strFullpath, mapname, MAX_PATH );

	if ( bWithExtension )
	{
		Q_strncat( strFullpath, ".bik", MAX_PATH );		// Assume we're a .bik extension type
	}

	return strFullpath;
}
#endif

#ifdef GAME_DLL

void CTFGameRules::BeginInfection( void )
{
	TFGameRules()->SelectInfector();

	// all teams
	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		BroadcastSound( i, "InfectionMusic.Begin", false );
	}

	// remove the timer
	if ( GetInfectionRoundTimer() )
		UTIL_Remove( GetInfectionRoundTimer() );

	// wow
	variant_t sVariant;
	sVariant.SetInt( 0 );

	// create the timer
	TFGameRules()->SetInfectionRoundTimer( ( CTeamRoundTimer* )CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );

	if ( TFGameRules()->GetInfectionRoundTimer() )
	{
		TFGameRules()->GetInfectionRoundTimer()->SetName( MAKE_STRING( "zz_infection_timer" ) );
		TFGameRules()->GetInfectionRoundTimer()->SetInfectionBeginning( false );
		TFGameRules()->GetInfectionRoundTimer()->SetTimeRemaining( of_infection_roundtime.GetInt() ); 
		TFGameRules()->GetInfectionRoundTimer()->SetShowInHud( true );
		TFGameRules()->GetInfectionRoundTimer()->ChangeTeam( TF_TEAM_RED );

		TFGameRules()->GetInfectionRoundTimer()->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
		TFGameRules()->GetInfectionRoundTimer()->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
	}
}

void CTFGameRules::SelectInfector( void )
{
	// Find random player(s) to infect
	// This is based on the amount of players (default ratio: 1 zombie for every 6 humans, rounded up)
	CBasePlayer *pPlayer = NULL;
	int i;

	int playercount = 0;
	int targets = 0;
	int candidates = 0;

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer )
		{
			playercount++;

			if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
				targets++;
		}
	}

	if ( targets <= 0 )
	{
		Msg( "TFGameRulesInfection: Infection found no players to zombify!\n" );
		if ( GetInfectionRoundTimer() )
			UTIL_Remove( GetInfectionRoundTimer() );
		TFGameRules()->SetInWaitingForPlayers( true );
		return;
	}

	if ( targets > 1 )
	{
		int threshold = of_infection_zombie_threshold.GetInt();

		if ( threshold <= 0 )
			threshold = 6;

		candidates = ceil( (float)targets / threshold );

		if ( candidates <= 0 )
		{
			Msg( "TFGameRulesInfection: Candidates for Infection is 0!\n" );
			if ( GetInfectionRoundTimer() )
				UTIL_Remove( GetInfectionRoundTimer() );
			TFGameRules()->SetInWaitingForPlayers( true );
			return;
		}
	}
	else
	{
		Msg( "TFGameRulesInfection: Needs more than 1 player on RED to begin.\n" );
		if ( GetInfectionRoundTimer() )
			UTIL_Remove( GetInfectionRoundTimer() );
		TFGameRules()->SetInWaitingForPlayers( true );
		return;
	}

	do
	{
		int index = RandomInt( 0, playercount - 1 );

		pPlayer = UTIL_PlayerByIndex( index );

		if ( pPlayer && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
		{
			candidates--;

			CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

			if ( pTFPlayer )
			{
				pTFPlayer->CommitSuicide( true, true );
				pTFPlayer->ChangeTeam( TF_TEAM_BLUE, false );
			}
		}
	} 
	while ( candidates > 0 );
}

void CTFGameRules::FinishInfection( void )
{
	SetWinningTeam( TF_TEAM_RED, WINREASON_COOP_FAIL, false );
}

	//-----------------------------------------------------------------------------
	// Purpose: Whether or not the NPC should drop a health vial
	// Output : Returns true on success, false on failure.
	//-----------------------------------------------------------------------------
	bool CTFGameRules::NPC_ShouldDropHealth( CBasePlayer *pRecipient )
	{
		// Can only do this every so often
		if ( m_flLastHealthDropTime > gpGlobals->curtime )
			return false;

		//Try to throw dynamic health
		float healthPerc = ( (float) pRecipient->m_iHealth / (float) pRecipient->m_iMaxHealth );

		if ( random->RandomFloat( 0.0f, 1.0f ) > healthPerc*1.5f )
			return true;

		return false;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Whether or not the NPC should drop a health vial
	// Output : Returns true on success, false on failure.
	//-----------------------------------------------------------------------------
	bool CTFGameRules::NPC_ShouldDropGrenade( CBasePlayer *pRecipient )
	{
		// Can only do this every so often
		if ( m_flLastGrenadeDropTime > gpGlobals->curtime )
			return false;
		
		int grenadeIndex = GetAmmoDef()->Index( "grenade" );
		int numGrenades = pRecipient->GetAmmoCount( grenadeIndex );

		// If we're not maxed out on grenades and we've randomly okay'd it
		if ( ( numGrenades < GetAmmoDef()->MaxCarry( grenadeIndex ) ) && ( random->RandomInt( 0, 2 ) == 0 ) )
			return true;

		return false;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Update the drop counter for health
	//-----------------------------------------------------------------------------
	void CTFGameRules::NPC_DroppedHealth( void )
	{
		m_flLastHealthDropTime = gpGlobals->curtime + sk_plr_health_drop_time.GetFloat();
	}

	//-----------------------------------------------------------------------------
	// Purpose: Update the drop counter for grenades
	//-----------------------------------------------------------------------------
	void CTFGameRules::NPC_DroppedGrenade( void )
	{
		m_flLastGrenadeDropTime = gpGlobals->curtime + sk_plr_grenade_drop_time.GetFloat();
	}


#ifdef HL2_EPISODIC
	ConVar  alyx_darkness_force("alyx_darkness_force", "0", FCVAR_CHEAT | FCVAR_REPLICATED);
#endif // HL2_EPISODIC

//-----------------------------------------------------------------------------
// Returns whether or not Alyx cares about light levels in order to see.
//-----------------------------------------------------------------------------
bool CTFGameRules::IsAlyxInDarknessMode()
{
#ifdef HL2_EPISODIC
	if ( alyx_darkness_force.GetBool() )
		return true;

	return ( GlobalEntity_GetState( "ep_alyx_darknessmode" ) == GLOBAL_ON );
#else
	return false;
#endif // HL2_EPISODIC
}


//-----------------------------------------------------------------------------
// This takes the long way around to see if a prop should emit a DLIGHT when it
// ignites, to avoid having Alyx-related code in props.cpp.
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldBurningPropsEmitLight()
{
#ifdef HL2_EPISODIC
	return IsAlyxInDarknessMode();
#else
	return false;
#endif // HL2_EPISODIC
}



	//------------------------------------------------------------------------------
	// Purpose : Initialize all default class relationships
	// Input   :
	// Output  :
	//------------------------------------------------------------------------------
	void CTFGameRules::InitDefaultAIRelationships( void )
	{
		int i, j;

		//  Allocate memory for default relationships
		CBaseCombatCharacter::AllocateDefaultRelationships();

		// --------------------------------------------------------------
		// First initialize table so we can report missing relationships
		// --------------------------------------------------------------
		for (i=0;i<NUM_AI_CLASSES;i++)
		{
			for (j=0;j<NUM_AI_CLASSES;j++)
			{
				// By default all relationships are neutral of priority zero
				CBaseCombatCharacter::SetDefaultRelationship( (Class_T)i, (Class_T)j, D_NU, 0 );
			}
		}

		// ------------------------------------------------------------
		//	> CLASS_ANTLION
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_COMBINE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_MANHACK,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_METROPOLICE,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_MILITARY,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_SCANNER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_STALKER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PROTOSNIPER,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_ANTLION,			D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_BARNACLE
		//
		//  In this case, the relationship D_HT indicates which characters
		//  the barnacle will try to eat.
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_BARNACLE,			D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_BULLSQUID,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_COMBINE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_MANHACK,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_METROPOLICE,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_MILITARY,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_STALKER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_EARTH_FAUNA,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_BULLSEYE
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PLAYER,			D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_ANTLION,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_BULLSQUID,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_CITIZEN_REBEL,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_COMBINE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_CONSCRIPT,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_HEADCRAB,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_HOUNDEYE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_METROPOLICE,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_MILITARY,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_VORTIGAUNT,		D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_ZOMBIE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PLAYER_ALLY,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_HACKED_ROLLERMINE,D_NU, 0);

		// ------------------------------------------------------------
		//	> CLASS_BULLSQUID
		// ------------------------------------------------------------
		/*
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BARNACLE,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BULLSEYE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BULLSQUID,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HEADCRAB,			D_HT, 1);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HOUNDEYE,			D_HT, 1);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MANHACK,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_METROPOLICE,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MILITARY,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_STALKER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HACKED_ROLLERMINE,D_HT, 0);
		*/
		// ------------------------------------------------------------
		//	> CLASS_CITIZEN_PASSIVE
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PLAYER,			D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_BARNACLE,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_BULLSQUID,		D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_CITIZEN_REBEL,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_COMBINE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_COMBINE_HUNTER,	D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_CONSCRIPT,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_HEADCRAB,			D_FR, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_HOUNDEYE,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_MANHACK,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_METROPOLICE,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_MILITARY,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_MISSILE,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_VORTIGAUNT,		D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_ZOMBIE,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PLAYER_ALLY,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_HACKED_ROLLERMINE,D_NU, 0);

		// ------------------------------------------------------------
		//	> CLASS_CITIZEN_REBEL
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PLAYER,			D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_BARNACLE,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_BULLSQUID,		D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_CITIZEN_REBEL,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_COMBINE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_COMBINE_HUNTER,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_CONSCRIPT,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_MANHACK,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_METROPOLICE,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_MILITARY,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_MISSILE,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_SCANNER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_STALKER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_VORTIGAUNT,		D_LI, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PLAYER_ALLY,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_HACKED_ROLLERMINE,D_NU, 0);

		// ------------------------------------------------------------
		//	> CLASS_COMBINE
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_BARNACLE,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_COMBINE,			D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_COMBINE_GUNSHIP,	D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_COMBINE_HUNTER,	D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_METROPOLICE,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_MILITARY,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_COMBINE_GUNSHIP
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_COMBINE,			D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_COMBINE_GUNSHIP,	D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_COMBINE_HUNTER,	D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_HEADCRAB,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_METROPOLICE,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_MILITARY,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_MISSILE,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_COMBINE_HUNTER
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,	CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_COMBINE,			D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_COMBINE_GUNSHIP,	D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_COMBINE_HUNTER,	D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,	CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_METROPOLICE,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_MILITARY,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_CONSCRIPT
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PLAYER,			D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_BARNACLE,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_CITIZEN_REBEL,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_COMBINE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_CONSCRIPT,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_MANHACK,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_METROPOLICE,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_MILITARY,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_SCANNER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_STALKER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_VORTIGAUNT,		D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PLAYER_ALLY,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_HACKED_ROLLERMINE,D_NU, 0);
		
		// ------------------------------------------------------------
		//	> CLASS_FLARE
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PLAYER,			D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_ANTLION,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_BULLSQUID,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_CITIZEN_REBEL,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_COMBINE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_CONSCRIPT,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_HEADCRAB,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_HOUNDEYE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_METROPOLICE,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_MILITARY,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_VORTIGAUNT,		D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_ZOMBIE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PLAYER_ALLY,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_HACKED_ROLLERMINE,D_NU, 0);

		// ------------------------------------------------------------
		//	> CLASS_HEADCRAB
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_BULLSQUID,		D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_COMBINE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_HEADCRAB,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_HOUNDEYE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_METROPOLICE,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_MILITARY,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_ZOMBIE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_HACKED_ROLLERMINE,D_FR, 0);

		// ------------------------------------------------------------
		//	> CLASS_HOUNDEYE
		// ------------------------------------------------------------
		/*
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BULLSQUID,		D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HEADCRAB,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HOUNDEYE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_METROPOLICE,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MILITARY,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_ZOMBIE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);
		*/

		// ------------------------------------------------------------
		//	> CLASS_MANHACK
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_COMBINE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_HEADCRAB,			D_HT,-1);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_HOUNDEYE,			D_HT,-1);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_METROPOLICE,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_MILITARY,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_METROPOLICE
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_COMBINE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_COMBINE_HUNTER,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_METROPOLICE,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_MILITARY,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_MILITARY
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_COMBINE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_METROPOLICE,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_MILITARY,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_MISSILE
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_COMBINE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_METROPOLICE,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_MILITARY,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_NONE
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PLAYER,			D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_ANTLION,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_BULLSEYE,			D_NU, 0);	
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_BULLSQUID,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_CITIZEN_REBEL,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_COMBINE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_COMBINE_HUNTER,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_CONSCRIPT,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_HEADCRAB,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_HOUNDEYE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_METROPOLICE,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_MILITARY,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_VORTIGAUNT,		D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_ZOMBIE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PLAYER_ALLY,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_HACKED_ROLLERMINE,D_NU, 0);

		// ------------------------------------------------------------
		//	> CLASS_PLAYER
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER,			D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_BARNACLE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_BULLSEYE,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_CITIZEN_PASSIVE,	D_LI, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_CITIZEN_REBEL,	D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_COMBINE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_COMBINE_GUNSHIP,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_CONSCRIPT,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_MANHACK,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_METROPOLICE,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_MILITARY,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_SCANNER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_STALKER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_VORTIGAUNT,		D_LI, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PROTOSNIPER,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER_ALLY,		D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_HACKED_ROLLERMINE,D_LI, 0);

		// ------------------------------------------------------------
		//	> CLASS_PLAYER_ALLY
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PLAYER,			D_LI, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_BARNACLE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_CITIZEN_REBEL,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_COMBINE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_CONSCRIPT,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_HEADCRAB,			D_FR, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_MANHACK,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_METROPOLICE,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_MILITARY,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_SCANNER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_STALKER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_VORTIGAUNT,		D_LI, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_ZOMBIE,			D_FR, 1);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PROTOSNIPER,		D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PLAYER_ALLY,		D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_HACKED_ROLLERMINE,D_LI, 0);

		// ------------------------------------------------------------
		//	> CLASS_PLAYER_ALLY_VITAL
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER,			D_LI, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_BARNACLE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_CITIZEN_REBEL,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_COMBINE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_COMBINE_HUNTER,	D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_CONSCRIPT,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_MANHACK,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_METROPOLICE,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_MILITARY,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_SCANNER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_STALKER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_VORTIGAUNT,		D_LI, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PROTOSNIPER,		D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER_ALLY,		D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_HACKED_ROLLERMINE,D_LI, 0);

		// ------------------------------------------------------------
		//	> CLASS_SCANNER
		// ------------------------------------------------------------	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_BULLSQUID,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_COMBINE,			D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_COMBINE_GUNSHIP,	D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_COMBINE_HUNTER,	D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_HEADCRAB,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_HOUNDEYE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_MANHACK,			D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_METROPOLICE,		D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_MILITARY,			D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_SCANNER,			D_LI, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_STALKER,			D_LI, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_ZOMBIE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PROTOSNIPER,		D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_STALKER
		// ------------------------------------------------------------	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_COMBINE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_HEADCRAB,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_HOUNDEYE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_METROPOLICE,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_MILITARY,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_ZOMBIE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_VORTIGAUNT
		// ------------------------------------------------------------	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PLAYER,			D_LI, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_BARNACLE,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_CITIZEN_PASSIVE,	D_LI, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_CITIZEN_REBEL,	D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_COMBINE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_COMBINE_HUNTER,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_CONSCRIPT,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_MANHACK,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_METROPOLICE,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_MILITARY,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_SCANNER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_STALKER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_VORTIGAUNT,		D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PLAYER_ALLY,		D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_HACKED_ROLLERMINE,D_LI, 0);

		// ------------------------------------------------------------
		//	> CLASS_ZOMBIE
		// ------------------------------------------------------------	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_BULLSQUID,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_COMBINE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_HEADCRAB,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_HOUNDEYE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_MANHACK,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_METROPOLICE,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_MILITARY,			D_FR, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_ZOMBIE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_PROTOSNIPER
		// ------------------------------------------------------------	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PLAYER,			D_HT, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_BULLSQUID,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_COMBINE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_HOUNDEYE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_METROPOLICE,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_MILITARY,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_MISSILE,			D_NU, 5);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_STALKER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_EARTH_FAUNA
		//
		// Hates pretty much everything equally except other earth fauna.
		// This will make the critter choose the nearest thing as its enemy.
		// ------------------------------------------------------------	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_NONE,				D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PLAYER,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_COMBINE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_COMBINE_GUNSHIP,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_CONSCRIPT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_FLARE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_MANHACK,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_METROPOLICE,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_MILITARY,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_MISSILE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_SCANNER,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_STALKER,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_VORTIGAUNT,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_ZOMBIE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PROTOSNIPER,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_HACKED_ROLLERMINE,D_NU, 0);

		// ------------------------------------------------------------
		//	> CLASS_HACKED_ROLLERMINE
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PLAYER,			D_LI, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_ANTLION,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_BARNACLE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_BULLSEYE,			D_NU, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_BULLSQUID,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_CITIZEN_REBEL,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_COMBINE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_CONSCRIPT,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_FLARE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_HEADCRAB,			D_HT, 0);
		//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_HOUNDEYE,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_MANHACK,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_METROPOLICE,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_MILITARY,			D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_MISSILE,			D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_SCANNER,			D_NU, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_STALKER,			D_HT, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_VORTIGAUNT,		D_LI, 0);		
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_ZOMBIE,			D_HT, 1);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_EARTH_FAUNA,		D_HT, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PLAYER_ALLY,		D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_HACKED_ROLLERMINE,D_LI, 0);
	}


	//------------------------------------------------------------------------------
	// Purpose : Return classify text for classify type
	// Input   :
	// Output  :
	//------------------------------------------------------------------------------
	const char* CTFGameRules::AIClassText(int classType)
	{
		switch (classType)
		{
			case CLASS_NONE:			return "CLASS_NONE";
			case CLASS_PLAYER:			return "CLASS_PLAYER";
			case CLASS_ANTLION:			return "CLASS_ANTLION";
			case CLASS_BARNACLE:		return "CLASS_BARNACLE";
			case CLASS_BULLSEYE:		return "CLASS_BULLSEYE";
			//case CLASS_BULLSQUID:		return "CLASS_BULLSQUID";	
			case CLASS_CITIZEN_PASSIVE: return "CLASS_CITIZEN_PASSIVE";		
			case CLASS_CITIZEN_REBEL:	return "CLASS_CITIZEN_REBEL";
			case CLASS_COMBINE:			return "CLASS_COMBINE";
			case CLASS_COMBINE_GUNSHIP:	return "CLASS_COMBINE_GUNSHIP";
			case CLASS_COMBINE_HUNTER:	return "CLASS_COMBINE_HUNTER";
			case CLASS_CONSCRIPT:		return "CLASS_CONSCRIPT";
			case CLASS_HEADCRAB:		return "CLASS_HEADCRAB";
			//case CLASS_HOUNDEYE:		return "CLASS_HOUNDEYE";
			case CLASS_MANHACK:			return "CLASS_MANHACK";
			case CLASS_METROPOLICE:		return "CLASS_METROPOLICE";
			case CLASS_MILITARY:		return "CLASS_MILITARY";	
			case CLASS_SCANNER:			return "CLASS_SCANNER";		
			case CLASS_STALKER:			return "CLASS_STALKER";		
			case CLASS_VORTIGAUNT:		return "CLASS_VORTIGAUNT";
			case CLASS_ZOMBIE:			return "CLASS_ZOMBIE";
			case CLASS_PROTOSNIPER:		return "CLASS_PROTOSNIPER";
			case CLASS_MISSILE:			return "CLASS_MISSILE";
			case CLASS_FLARE:			return "CLASS_FLARE";
			case CLASS_EARTH_FAUNA:		return "CLASS_EARTH_FAUNA";

			default:					return "MISSING CLASS in ClassifyText()";
		}
	}

#endif
