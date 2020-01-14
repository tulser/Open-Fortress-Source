//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Dropped Cond Powerup
//
//=============================================================================//


// You're in for a bumpy ride

#include "cbase.h"
#include "of_dropped_powerup.h"
#include "tf_shareddefs.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "explode.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//----------------------------------------------

// Network table.
IMPLEMENT_SERVERCLASS_ST( CTFDroppedPowerup, DT_DroppedPowerup )
	SendPropVector( SENDINFO( m_vecInitialVelocity ), -1, SPROP_NOSCALE ),
	SendPropTime( SENDINFO( m_flRespawnTick ) ),
	SendPropTime( SENDINFO( PowerupDuration ) ),
	SendPropTime( SENDINFO( OriginalPowerupDuration ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CTFDroppedPowerup )
	DEFINE_THINKFUNC( FlyThink ),
	DEFINE_ENTITYFUNC( PackTouch ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( tf_dropped_powerup, CTFDroppedPowerup );

PRECACHE_REGISTER( tf_dropped_powerup );

void CTFDroppedPowerup::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );
	BaseClass::Spawn();

	SetNextThink( gpGlobals->curtime + 0.75f );
	SetThink( &CTFDroppedPowerup::FlyThink );

	SetTouch( &CTFDroppedPowerup::PackTouch );

	m_flCreationTime = gpGlobals->curtime;

	// no pickup until flythink
	m_bAllowOwnerPickup = false;

	// Die in 30 seconds

	if ( IsX360() )
	{
		RemoveEffects( EF_ITEM_BLINK );
	}
}

void CTFDroppedPowerup::Precache( void )
{
}

CTFDroppedPowerup *CTFDroppedPowerup::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszModelName, int PowerupID, float PowerupDuration, float OriginalPowerupDuration )
{
	CTFDroppedPowerup *pDroppedPowerup = static_cast<CTFDroppedPowerup*>( CBaseAnimating::CreateNoSpawn( "tf_dropped_powerup", vecOrigin, vecAngles, pOwner ) );
	if ( pDroppedPowerup )
	{
		pDroppedPowerup->SetModelName( AllocPooledString( pszModelName ) );
		DispatchSpawn( pDroppedPowerup );
	}
	
	pDroppedPowerup->SetParent(pOwner);					// Attach the dropped powerup
	pDroppedPowerup->FollowEntity( pOwner, false );		// to the player that picked up the powerup
	pDroppedPowerup->PowerupDuration = PowerupDuration; // Set its duration to the duration the caller ( either a Powerup spawner or another dropped powerup )
	
	if ( OriginalPowerupDuration == 0 )					// If the Original duration isn't set
		pDroppedPowerup->OriginalPowerupDuration=PowerupDuration;	// Set it to the Powerup duration
	else
		pDroppedPowerup->OriginalPowerupDuration=OriginalPowerupDuration; // Otherwise set it to the callers original duration         We do this so that the circle timer always shows how long the normal powerup lasts, even if it was dropped and used multiple times
	pDroppedPowerup->PowerupID=PowerupID;		// Set the condition ID
	pDroppedPowerup->AddEffects( EF_NODRAW );	// Make it invisible, since its initialy called when its still not dropped
	pDroppedPowerup->m_flRespawnTick = gpGlobals->curtime + PowerupDuration; // This sets when its gonna despawn, used for the circular timer
	pDroppedPowerup->SetNextThink( gpGlobals->curtime ); // Set the next think to happen imidiatley
	pDroppedPowerup->SetThink( &CTFDroppedPowerup::FlyThink ); 
	pDroppedPowerup->SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + PowerupDuration , "DieContext" ); // Set its death time to whenever the powerup would have ran out
	return pDroppedPowerup;
}

void CTFDroppedPowerup::SetInitialVelocity( Vector &vecVelocity ) // Obsolete, because the entity doesn't have physics, left it in since we may use it later on
{ 
	m_vecInitialVelocity = vecVelocity;
}

void CTFDroppedPowerup::FlyThink( void )
{
	CTFPlayer *pTFPlayer = ToTFPlayer ( GetOwnerEntity() ); // Get the player who currently posseses this Powerup
	if ( pTFPlayer && !pTFPlayer->IsAlive() )	// If they're dead
	{
		SetParent(NULL);	// Unparent us
		FollowEntity( NULL ); // And stop following them
		SetAbsAngles( QAngle( 0, 0, 0 ) ); // Set this to stand upwards like a real man
		RemoveEffects( EF_NODRAW ); // Make it visible
		OnGround = true;	// Allow people to pick it up
		m_bAllowOwnerPickup = true; // Allow the now dead owner to pick it up
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.01f ); // Otherwise restart this think
		SetThink( &CTFDroppedPowerup::FlyThink );
	}
}

void CTFDroppedPowerup::PackTouch( CBaseEntity *pOther )
{
	Assert( pOther );

	if( !pOther->IsPlayer() )
		return;

	if( !pOther->IsAlive() )
		return;	
	if ( !OnGround )
		return;
	//Don't let the person who got this powerup pick it up until it hits the ground and its owner is dead
	//This way the powerup wont get picked up by the person who already has it picked up and wont be able to be picked up by walking into its owner
	if( GetOwnerEntity() == pOther && m_bAllowOwnerPickup == false )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );

	CTFPlayer *pTFPlayer = NULL;

	if ( pPlayer )
		pTFPlayer = ToTFPlayer( pPlayer );

	if ( !pTFPlayer )
		return;

	if ( pTFPlayer->m_Shared.IsZombie() )
		return;

	bool bSuccess = true;

	// oh boy...
	// HACK: can't update all the maps with the old conditions, therefore this has to be remapped!
	switch ( PowerupID )
	{
		case TF_COND_STEALTHED:
			PowerupID = TF_COND_INVIS_POWERUP;
			break;
		case TF_COND_CRITBOOSTED:
			PowerupID = TF_COND_CRIT_POWERUP;
			break;
		case 12:
			PowerupID = TF_COND_SPAWNPROTECT;
			break;
		case 13:
			PowerupID = TF_COND_SHIELD_CHARGE;
			break;
		case 14:
			PowerupID = TF_COND_BERSERK;
			break;
		case 15:
			PowerupID = TF_COND_SHIELD;
			break;
	}
	
	if ( pTFPlayer->m_Shared.InCond(PowerupID) )  // If we already have this condition, dont pick it up
		return;
	if ( bSuccess )			// If we picked it up
	{
		CSingleUserRecipientFilter filter( pTFPlayer );		// Filter the sound to the person who picked this up
		EmitSound( filter, entindex(), "AmmoPack.Touch" );	// Play the pickup sound
		pTFPlayer->m_Shared.AddCond( PowerupID , PowerupDuration - ( gpGlobals->curtime - GetCreationTime()) ); // Give them the condition
		int iRandom = random->RandomInt( 0, 1 );
		pTFPlayer->SpeakConceptIfAllowed( ( iRandom == 1 ) ? MP_CONCEPT_PLAYER_SPELL_PICKUP_RARE : MP_CONCEPT_PLAYER_SPELL_PICKUP_COMMON );
		Vector vecPackOrigin;	// These are only used so that the create function compiles,
		QAngle vecPackAngles;	// if we are sure we wont use physics on this we can remove it from the function
		CTFDroppedPowerup::Create( vecPackOrigin, vecPackAngles , pTFPlayer, STRING( GetModelName() ), PowerupID, PowerupDuration - ( gpGlobals->curtime - GetCreationTime()), OriginalPowerupDuration );
		UTIL_Remove( this ); // Remove the dropped powerup
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTFDroppedPowerup::PhysicsSolidMaskForEntity( void ) const
{ 
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_DEBRIS;
}