//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_smg.h"
#include "in_buttons.h"
#include "tf_weaponbase.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"

// Server specific.
#else
#include "tf_player.h"
#include "util.h"
#endif

//=============================================================================
//
// SMG
//
//=============================================================================
IMPLEMENT_NETWORKCLASS_ALIASED( TFSMG, DT_WeaponSMG )

BEGIN_NETWORK_TABLE( CTFSMG, DT_WeaponSMG )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSMG )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_smg, CTFSMG );
PRECACHE_WEAPON_REGISTER( tf_weapon_smg );

//=============================================================================
//
// Merc SMG
//
//=============================================================================
IMPLEMENT_NETWORKCLASS_ALIASED( TFSMG_Mercenary, DT_WeaponSMG_Mercenary )

BEGIN_NETWORK_TABLE( CTFSMG_Mercenary, DT_WeaponSMG_Mercenary )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSMG_Mercenary )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_smg_mercenary, CTFSMG_Mercenary );
PRECACHE_WEAPON_REGISTER( tf_weapon_smg_mercenary );

//=============================================================================
//
// Tommy Gun
//
//=============================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( TFTommyGun, DT_WeaponTommyGun )

BEGIN_NETWORK_TABLE( CTFTommyGun, DT_WeaponTommyGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFTommyGun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_tommygun, CTFTommyGun );
PRECACHE_WEAPON_REGISTER( tf_weapon_tommygun );


//=============================================================================
//
// AR
//
//=============================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( TFAssaultRifle, DT_WeaponAssaultRifle )

BEGIN_NETWORK_TABLE( CTFAssaultRifle, DT_WeaponAssaultRifle )
#if defined( CLIENT_DLL )
	RecvPropInt( RECVINFO( m_iShotsDue ) ),
	RecvPropFloat( RECVINFO(m_flNextShotTime ) ),
#else
	SendPropInt( SENDINFO( m_iShotsDue ), 4, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropFloat( SENDINFO( m_flNextShotTime ), 0, SPROP_CHANGES_OFTEN ),
#endif
END_NETWORK_TABLE()

#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CTFAssaultRifle )

	DEFINE_FIELD(m_iShotsDue, FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextShotTime, FIELD_FLOAT ),

END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_assaultrifle, CTFAssaultRifle );
PRECACHE_WEAPON_REGISTER( tf_weapon_assaultrifle );


// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFSMG )
END_DATADESC()
#endif




//=============================================================================
//
// Assault Rifle functions.
//
//=============================================================================



//-----------------------------------------------------------------------------
// Purpose: Primary fire button attack
//-----------------------------------------------------------------------------
void CTFAssaultRifle::Shoot(void)
{
	// If my clip is empty (and I use clips) start reload
	if ( UsesClipsForAmmo1() && !m_iClip1 ) 
	{
		m_iShotsDue = 0;
		return;
	}

	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
	{
		return;
	}

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( GetPrimaryAttackActivity() );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	FireBulletsInfo_t info;
	info.m_vecSrc	 = pPlayer->Weapon_ShootPosition( );
	
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;

#if !defined( CLIENT_DLL )
	// Fire the bullets
	info.m_vecSpread = pPlayer->GetAttackSpread( this );
#else
	//!!!HACKHACK - what does the client want this function for? 
	info.m_vecSpread = GetActiveWeapon()->GetBulletSpread();
#endif // CLIENT_DLL

	info.m_iShots = 1;
	m_iClip1 -= info.m_iShots;

	pPlayer->FireBullets( info );

	//Add our view kick in
	AddViewKick();
}

bool CTFAssaultRifle::Reload( void )
{
	if ( InBurst( ) )
		return false;

	return BaseClass::Reload( );
}

void CTFAssaultRifle::ItemPostFrame( void )
{
	if ( InBurst( ) && m_flNextShotTime < gpGlobals->curtime )
		BurstFire( );

	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner( ) );
	if ( !pOwner )
		return;

	if ( pOwner->IsAlive( ) && ( pOwner->m_nButtons & IN_ATTACK ) && m_flNextPrimaryAttack < gpGlobals->curtime )
	{
		BeginBurstFire( );
	}

	BaseClass::ItemPostFrame( );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CTFAssaultRifle::BurstFire( void )
{
	if ( m_iClip1 == 0 )
	{
		m_iShotsDue = 0;
		return;
	}
	Shoot( );
	WeaponSound( SINGLE );
	m_iShotsDue--;
	m_flNextShotTime = gpGlobals->curtime + ofd_weapon_assaultrifle_bursttime.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CTFAssaultRifle::BeginBurstFire(void)
{
	if (m_bFireOnEmpty || InBurst())
		return;

	m_iShotsDue = ofd_weapon_assaultrifle_burstshots.GetInt();

	m_flNextPrimaryAttack = gpGlobals->curtime + GetBurstTotalTime() + ofd_weapon_assaultrifle_time_between_bursts.GetFloat();
}

