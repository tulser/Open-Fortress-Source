//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

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
	pDroppedPowerup->SetParent(pOwner);
	pDroppedPowerup->FollowEntity( pOwner, false );
	pDroppedPowerup->PowerupDuration=PowerupDuration;
	if ( OriginalPowerupDuration == 0 )
	{
		pDroppedPowerup->OriginalPowerupDuration=PowerupDuration;
	}
	else
		pDroppedPowerup->OriginalPowerupDuration=OriginalPowerupDuration;
	pDroppedPowerup->PowerupID=PowerupID;
	pDroppedPowerup->AddEffects( EF_NODRAW );
	pDroppedPowerup->m_flRespawnTick = gpGlobals->curtime + PowerupDuration;
	pDroppedPowerup->SetNextThink( gpGlobals->curtime );
	pDroppedPowerup->SetThink( &CTFDroppedPowerup::FlyThink );
	pDroppedPowerup->SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + PowerupDuration , "DieContext" );
	return pDroppedPowerup;
}

void CTFDroppedPowerup::SetInitialVelocity( Vector &vecVelocity )
{ 
	m_vecInitialVelocity = vecVelocity;
}

void CTFDroppedPowerup::FlyThink( void )
{
	CTFPlayer *pTFPlayer = ToTFPlayer ( GetOwnerEntity() );
	if (pTFPlayer && !pTFPlayer->IsAlive() )
	{
		SetParent(NULL);
		FollowEntity( NULL );
		SetAbsAngles( QAngle( 0, 0, 0 ) );
		RemoveEffects( EF_NODRAW );
		OnGround = true;
		m_bAllowOwnerPickup = true;
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.01f );
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
	//Don't let the person who threw this ammo pick it up until it hits the ground.
	//This way we can throw ammo to people, but not touch it as soon as we throw it ourselves
	if( GetOwnerEntity() == pOther && m_bAllowOwnerPickup == false )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	Assert( pPlayer );

	bool bSuccess = true;
	if ( pTFPlayer->m_Shared.InCond(PowerupID) )
		return;
	if ( bSuccess )
	{
		DevMsg("Powerup time is %d \n", (PowerupDuration - gpGlobals->curtime - m_flCreationTime));
		CSingleUserRecipientFilter filter( pTFPlayer );
		EmitSound( filter, entindex(), "AmmoPack.Touch" );
		pTFPlayer->m_Shared.AddCond( PowerupID , PowerupDuration - ( gpGlobals->curtime - GetCreationTime()) );
		Vector vecPackOrigin;
		QAngle vecPackAngles;
		CTFDroppedPowerup::Create( vecPackOrigin, vecPackAngles , pTFPlayer, STRING( GetModelName() ), PowerupID, PowerupDuration - ( gpGlobals->curtime - GetCreationTime()), OriginalPowerupDuration );
		UTIL_Remove( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTFDroppedPowerup::PhysicsSolidMaskForEntity( void ) const
{ 
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_DEBRIS;
}