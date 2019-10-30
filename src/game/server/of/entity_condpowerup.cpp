//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Deathmatch Powerup Spawner.
//
//=============================================================================//

#include "cbase.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "entity_condpowerup.h"
#include "of_dropped_powerup.h"
#include "tf_gamerules.h"

#include "tier0/memdbgon.h"

extern ConVar of_powerups;

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the powerupspawner
//-----------------------------------------------------------------------------


BEGIN_DATADESC( CCondPowerup )

// Inputs.
DEFINE_KEYFIELD( m_bCondition, FIELD_INTEGER, "condID" ),
DEFINE_KEYFIELD( m_bCondDuration, FIELD_FLOAT, "duration" ),
DEFINE_KEYFIELD( m_iszPowerupModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszPowerupModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),
DEFINE_KEYFIELD( m_bDisableShowOutline, FIELD_BOOLEAN, "disable_glow" ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CCondPowerup, DT_CondPowerup )
SendPropBool( SENDINFO( m_bDisableShowOutline ) ),
SendPropBool( SENDINFO( m_bRespawning) ),
SendPropBool( SENDINFO( bInitialDelay ) ),
SendPropTime( SENDINFO( m_flRespawnTick ) ),
SendPropTime( SENDINFO( fl_RespawnTime ) ),
SendPropTime( SENDINFO( fl_RespawnDelay ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( dm_powerup_spawner, CCondPowerup );

void CCondPowerup::Spawn( void )
{
	if ( TFGameRules()->IsMutator( INSTAGIB ) || TFGameRules()->IsMutator( INSTAGIB_NO_MELEE ) ||
		 !of_powerups.GetBool() )
		return;
	Precache();
	if (m_iszPowerupModel==MAKE_STRING( "" )) SetModel( STRING(m_iszPowerupModelOLD)  );
	else SetModel( STRING(m_iszPowerupModel) );
	
	SetTransmitState( FL_EDICT_ALWAYS );	// Used for the glow effect to always show up

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the powerupspawner
//-----------------------------------------------------------------------------
void CCondPowerup::Precache( void )
{
	if (m_iszPowerupModel==MAKE_STRING( "" )) PrecacheModel( STRING(m_iszPowerupModelOLD)  );
	else PrecacheModel( STRING(m_iszPowerupModel) );
	PrecacheScriptSound( STRING( m_iszPickupSound ) );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the powerupspawner
//-----------------------------------------------------------------------------
bool CCondPowerup::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;
	if ( ValidTouch( pPlayer ) )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( !pTFPlayer )
			return false;
		
		switch ( m_bCondition )
		{
			case TF_COND_STEALTHED:
				m_bCondition = TF_COND_INVIS_POWERUP;
				break;
			case TF_COND_CRITBOOSTED:
				m_bCondition = TF_COND_CRIT_POWERUP;
				break;
			case TF_COND_INVIS_POWERUP:
				m_bCondition = TF_COND_INVIS_POWERUP;
				break;
			case TF_COND_CRIT_POWERUP:
				m_bCondition = TF_COND_CRITBOOSTED;
				break;
		}		
		
		if ( pTFPlayer->m_Shared.InCond(m_bCondition) )
			return false;
		bSuccess = true;
		Vector vecPackOrigin;
		QAngle vecPackAngles;
		pTFPlayer->m_Shared.AddCond( m_bCondition , m_bCondDuration );
		CTFDroppedPowerup::Create( vecPackOrigin, vecPackAngles , pTFPlayer,STRING( m_iszPowerupModel ), m_bCondition, m_bCondDuration, 0 );  // The dropped powerup is spawned here, more explanation in its cpp file but basicaly we do this to preserve custom settings like its model on it
		if ( TeamplayRoundBasedRules() )
		{
			if ( strcmp( GetPowerupPickupLine(), "None" ) || strcmp( GetPowerupPickupLineSelf(), "None" ) )
				TeamplayRoundBasedRules()->BroadcastSoundFFA( pTFPlayer->entindex(), GetPowerupPickupLineSelf(), GetPowerupPickupLine() );
			
			if ( strcmp( GetPowerupPickupSound(), "None" ) )
				TeamplayRoundBasedRules()->BroadcastSound( TEAM_UNASSIGNED, GetPowerupPickupSound(), false );
			else
				TeamplayRoundBasedRules()->BroadcastSound( TEAM_UNASSIGNED, STRING( m_iszPickupSound ), false );
		}
		m_nRenderFX = kRenderFxDistort;
	}
	return bSuccess;
}

const char* CCondPowerup::GetPowerupPickupLineSelf( void )
{
	switch ( m_bCondition )
	{
		case TF_COND_INVULNERABLE:
		case TF_COND_SHIELD:
		return "ShieldTakenSelf";
		break;
	}
	return "None";
}

const char* CCondPowerup::GetPowerupPickupLine( void )
{
	switch ( m_bCondition )
	{
		case TF_COND_CRITBOOSTED:
		case TF_COND_CRIT_POWERUP:
		return "CritTaken";
		break;
	}
	return "None";
}

const char* CCondPowerup::GetPowerupPickupSound( void )
{
	switch ( m_bCondition )
	{
		case TF_COND_CRITBOOSTED:
		case TF_COND_CRIT_POWERUP:
		return "Powerup.Crit";
		break;
		case TF_COND_STEALTHED:
		case TF_COND_INVIS_POWERUP:
		return "Powerup.Invis";
		break;
		case TF_COND_BERSERK:
		return "Powerup.Berserk";
		break;
		case TF_COND_INVULNERABLE:
		case TF_COND_SHIELD:
		return "Powerup.Shield";
		break;
	}
	return "None";
}

CCondPowerup::CCondPowerup()
{
	m_flRespawnTick = 0.0f;
}

CBaseEntity* CCondPowerup::Respawn( void )
{
	CBaseEntity *ret = BaseClass::Respawn();
	m_nRenderFX = kRenderFxDistort;
	m_flRespawnTick = GetNextThink();
	return ret;
}

const char* CCondPowerup::GetPowerupRespawnLine( void )
{
	switch ( m_bCondition )
	{
		case TF_COND_CRITBOOSTED:
		case TF_COND_CRIT_POWERUP:
		return "Crits";
		break;
		case TF_COND_STEALTHED:
		case TF_COND_INVIS_POWERUP:
		return "Invisibility";
		break;
		case TF_COND_SHIELD:
		case TF_COND_INVULNERABLE:
		return "Shield";
		break;
	}
	return "None";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCondPowerup::Materialize( void )
{
	if ( !IsDisabled() )
	{
		// changing from invisible state to visible.
		m_nRenderFX = kRenderFxNone;
		RemoveEffects( EF_NODRAW );
	}
	m_OnRespawn.FireOutput( this, this );
	if ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->State_Get() != GR_STATE_PREROUND && strcmp(GetPowerupRespawnLine(), "None" ) )
		TeamplayRoundBasedRules()->BroadcastSound(TEAM_UNASSIGNED, GetPowerupRespawnLine() );
	else if ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->State_Get() != GR_STATE_PREROUND )
		TeamplayRoundBasedRules()->BroadcastSound(TEAM_UNASSIGNED, STRING ( m_iszSpawnSound ), false );
	m_bRespawning = false;
	bInitialDelay = false;
	SetTouch( &CItem::ItemTouch );
}