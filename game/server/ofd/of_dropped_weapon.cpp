//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "of_dropped_weapon.h"
#include "tf_shareddefs.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "explode.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//----------------------------------------------

extern ConVar ofd_multiweapons;
extern ConVar ofd_allow_allclass_pickups;

// Network table.
IMPLEMENT_SERVERCLASS_ST( CTFDroppedWeapon, DT_DroppedWeapon )
	SendPropVector( SENDINFO( m_vecInitialVelocity ), -1, SPROP_NOSCALE ),
END_SEND_TABLE()

BEGIN_DATADESC( CTFDroppedWeapon )
	DEFINE_THINKFUNC( FlyThink ),
	DEFINE_ENTITYFUNC( PackTouch ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( tf_dropped_weapon, CTFDroppedWeapon );

PRECACHE_REGISTER( tf_dropped_weapon );

void CTFDroppedWeapon::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );
	BaseClass::Spawn();

	SetNextThink( gpGlobals->curtime + 0.75f );
	SetThink( &CTFDroppedWeapon::FlyThink );

	SetTouch( &CTFDroppedWeapon::PackTouch );

	m_flCreationTime = gpGlobals->curtime;

	// no pickup until flythink
	m_bAllowOwnerPickup = false;

	// Die in 30 seconds
	SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + 30, "DieContext" );

	if ( IsX360() )
	{
		RemoveEffects( EF_ITEM_BLINK );
	}
}

void CTFDroppedWeapon::Precache( void )
{
}

CTFDroppedWeapon *CTFDroppedWeapon::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszModelName )
{
	CTFDroppedWeapon *pDroppedWeapon = static_cast<CTFDroppedWeapon*>( CBaseAnimating::CreateNoSpawn( "tf_dropped_weapon", vecOrigin, vecAngles, pOwner ) );
	if ( pDroppedWeapon )
	{
		pDroppedWeapon->SetModelName( AllocPooledString( pszModelName ) );
		DispatchSpawn( pDroppedWeapon );
	}

	return pDroppedWeapon;
}

void CTFDroppedWeapon::SetInitialVelocity( Vector &vecVelocity )
{ 
	m_vecInitialVelocity = vecVelocity;
}

void CTFDroppedWeapon::FlyThink( void )
{
	m_bAllowOwnerPickup = true;
}

void CTFDroppedWeapon::PackTouch( CBaseEntity *pOther )
{
	Assert( pOther );

	if( !pOther->IsPlayer() )
		return;

	if( !pOther->IsAlive() )
		return;	
	if ( TFGameRules() && TFGameRules()->IsGGGamemode() )
		return;
	//Don't let the person who threw this ammo pick it up until it hits the ground.
	//This way we can throw ammo to people, but not touch it as soon as we throw it ourselves
	if( GetOwnerEntity() == pOther && m_bAllowOwnerPickup == false )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if( !pTFPlayer->GetPlayerClass()->IsClass( TF_CLASS_MERCENARY) && !ofd_allow_allclass_pickups.GetBool() )
		return;
	Assert( pPlayer );

	bool bSuccess = true;
	if ( WeaponID == TF_WEAPON_PISTOL_MERCENARY )
		WeaponID = TF_WEAPON_PISTOL_AKIMBO;
	const char *pszWeaponName = WeaponIdToClassname( WeaponID );
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)pPlayer->GiveNamedItem( pszWeaponName );
	for ( int iWeapon = 0; iWeapon < TF_WEAPON_COUNT; ++iWeapon )
	{		
		CTFWeaponBase *pCarriedWeapon = (CTFWeaponBase *)pPlayer->GetWeapon( iWeapon );
		if ( pCarriedWeapon == pWeapon ) 
		{
			bSuccess=false;
		}
		if ( TFGameRules() && !TFGameRules()->UsesDMBuckets() )
		{
			bSuccess=false;
			if ( pCarriedWeapon && pWeapon && pCarriedWeapon->GetSlot() == pWeapon->GetSlot() && pCarriedWeapon != pWeapon && pTFPlayer->m_nButtons & IN_USE )
			{
				pTFPlayer->DropWeapon( pCarriedWeapon );
				if ( pCarriedWeapon )
				{
					pTFPlayer->Weapon_Detach( pCarriedWeapon );
					UTIL_Remove( pCarriedWeapon );
					pCarriedWeapon = NULL;				
				}
				CSingleUserRecipientFilter filter( pTFPlayer );
				EmitSound( filter, entindex(), "AmmoPack.Touch" );
				CTFWeaponBase *pGivenWeapon =(CTFWeaponBase *)pPlayer->GiveNamedItem( pszWeaponName );
				pGivenWeapon->GiveTo( pPlayer );		
				UTIL_Remove( this );
				if ( pWeapon )
				{
					pTFPlayer->Weapon_Detach( pWeapon );
					UTIL_Remove( pWeapon );
					pWeapon = NULL;
				}
				return;
			}
		}
		
	}
	if ( bSuccess )
	{
		CSingleUserRecipientFilter filter( pTFPlayer );
		EmitSound( filter, entindex(), "AmmoPack.Touch" );
		CTFWeaponBase *pGivenWeapon =(CTFWeaponBase *)pPlayer->GiveNamedItem( pszWeaponName );
		pGivenWeapon->GiveTo( pPlayer );		
		UTIL_Remove( this );
	}
	if ( pWeapon )
	{
		pTFPlayer->Weapon_Detach( pWeapon );
		UTIL_Remove( pWeapon );
		pWeapon = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTFDroppedWeapon::PhysicsSolidMaskForEntity( void ) const
{ 
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_DEBRIS;
}