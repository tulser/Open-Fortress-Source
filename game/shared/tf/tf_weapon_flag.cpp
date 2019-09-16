//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_flag.h"
#include "decals.h"
#include "tf_fx_shared.h"
#include "effect_dispatch_data.h"
#include "takedamageinfo.h"
#include "tf_projectile_nail.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
	#include "c_tf_player.h"
	#include "c_te_effect_dispatch.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_team.h"
#include "func_bomb_target.h"
#include "tf_gamestats.h"
#include "tf_player.h"
#include "tf_fx.h"
#include "te_effect_dispatch.h"

#include "tf_projectile_rocket.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "te.h"
#include "of_projectile_tripmine.h"
#endif

//=============================================================================
//
// Weapon Flag tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFFlag, DT_TFWeaponFlag )

BEGIN_NETWORK_TABLE( CTFFlag, DT_TFWeaponFlag )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bInPlantZone ) ),
	RecvPropBool( RECVINFO( m_bPlanting ) ),
	RecvPropTime( RECVINFO( m_flPlantStart ) ),
// Server specific.
#else
	SendPropBool( SENDINFO( m_bInPlantZone ) ),
	SendPropBool( SENDINFO( m_bPlanting ) ),
	SendPropTime( SENDINFO( m_flPlantStart ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFFlag )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bInPlantZone, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bPlanting, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
#if defined( CLIENT_DLL )
	DEFINE_FIELD(m_bInPlantZone, FIELD_INTEGER ),
	DEFINE_FIELD(m_bPlanting, FIELD_INTEGER ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_c4, CTFFlag );
PRECACHE_WEAPON_REGISTER( tf_weapon_c4 );

// Server specific.

BEGIN_DATADESC( CTFFlag )
DEFINE_FIELD( m_bInPlantZone, FIELD_BOOLEAN ),
DEFINE_FIELD( m_bPlanting, FIELD_BOOLEAN ),
END_DATADESC()

//=============================================================================
//
// Weapon Flag functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFFlag::CTFFlag()
{
	m_bPlanting = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFFlag::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
#ifdef GAME_DLL
        TFTeamMgr()->PlayerCenterPrint( ToTFPlayer( GetOwner() ), "#TF_Flag_AltFireToDrop" );
#endif
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlag::PrimaryAttack( void )
{	
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;
	
	if ( !m_bInPlantZone )
		return;
	
	if ( m_bPlanting = false || gpGlobals->curtime < m_flPlantStart + 5.0f )
		return;

#ifndef CLIENT_DLL

	pPlayer->SpeakWeaponFire();

	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	
#ifdef GAME_DLL
	pPlayer->DropCurrentWeapon();
#endif		
	pPlayer->SwitchToNextBestWeapon( this );


	// Set the idle animation times based on the sequence duration, so that we play full fire animations
	// that last longer than the refire rate may allow.
}	

void CTFFlag::ItemPostFrame( void )
{
	

	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner( ) );
	if ( !pOwner )
		return;
#ifdef GAME_DLL	
	m_bInPlantZone = InBombTargetZone(pOwner->GetAbsOrigin());
#endif	

	if ( m_bInPlantZone )
	{
		if ( m_bPlanting = false && pOwner->m_nButtons & IN_ATTACK )
		{
			m_bPlanting = true;
			m_flPlantStart = gpGlobals->curtime;
			SendWeaponAnim( ACT_VM_HITCENTER );
			SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
		}
		else if ( m_bPlanting = true && !( pOwner->m_nButtons & IN_ATTACK ) )
		{
			m_bPlanting = false;
			m_flPlantStart = gpGlobals->curtime;
		}
		if ( m_bPlanting = true && gpGlobals->curtime >= m_flPlantStart + 5.0f )
		{
			PrimaryAttack();
			m_bPlanting = false;
			m_flPlantStart = gpGlobals->curtime;
		}
	}
	BaseClass::ItemPostFrame();	
	
}


