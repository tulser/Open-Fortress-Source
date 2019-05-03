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
// Weapon SMG tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFSMG, DT_WeaponSMG )

BEGIN_NETWORK_TABLE( CTFSMG, DT_WeaponSMG )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSMG )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_smg, CTFSMG );
PRECACHE_WEAPON_REGISTER( tf_weapon_smg );

IMPLEMENT_NETWORKCLASS_ALIASED( TFSMG_Mercenary, DT_WeaponSMG_Mercenary )

BEGIN_NETWORK_TABLE( CTFSMG_Mercenary, DT_WeaponSMG_Mercenary )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSMG_Mercenary )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_smg_mercenary, CTFSMG_Mercenary );
PRECACHE_WEAPON_REGISTER( tf_weapon_smg_mercenary );

IMPLEMENT_NETWORKCLASS_ALIASED( TFTommyGun, DT_WeaponTommyGun )

BEGIN_NETWORK_TABLE( CTFTommyGun, DT_WeaponTommyGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFTommyGun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_tommygun, CTFTommyGun );
PRECACHE_WEAPON_REGISTER( tf_weapon_tommygun );

IMPLEMENT_NETWORKCLASS_ALIASED( TFAssaultRifle, DT_WeaponAssaultRifle )

BEGIN_NETWORK_TABLE( CTFAssaultRifle, DT_WeaponAssaultRifle )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFAssaultRifle )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_assaultrifle, CTFAssaultRifle );
PRECACHE_WEAPON_REGISTER( tf_weapon_assaultrifle );


// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFSMG )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon SMG functions.
//


//-----------------------------------------------------------------------------
// Purpose: Primary fire button attack
//-----------------------------------------------------------------------------
void CTFAssaultRifle::BasePrimaryAttack(void)
{
	// If my clip is empty (and I use clips) start reload
	if ( UsesClipsForAmmo1() && !m_iClip1 ) 
	{
		Reload();
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
	info.m_iShots = 0;
	float fireRate = GetFireRate();

	while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		// MUST call sound before removing a round from the clip of a CMachineGun
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		info.m_iShots++;
		if ( !fireRate )
			break;
	}

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
	
	if (!m_bInBurst)
		WeaponSound(SINGLE);

	info.m_iShots = 1;
	m_iClip1 -= info.m_iShots;

	pPlayer->FireBullets( info );

	//Add our view kick in
	AddViewKick();
}


int howmanytimes = 0;

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CTFAssaultRifle::BurstThink( void )
{
	BasePrimaryAttack();

	WeaponSound(SINGLE); howmanytimes++; DevMsg("ARBURST: Played shoot sound %i times!\n", howmanytimes);

	m_iBurstSize--;

	if( m_iBurstSize == 0 )
	{
		// The burst is over!
		SetThink(NULL);

		// idle immediately to stop the firing animation
		SetWeaponIdleTime( gpGlobals->curtime );
		m_bInBurst = false;
		howmanytimes = 0;
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.05f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CTFAssaultRifle::PrimaryAttack(void)
{
	if (m_bFireOnEmpty)
		return;

	m_bInBurst = true;
	m_iBurstSize = GetBurstSize();
		
	// Call the think function directly so that the first round gets fired immediately.
	CTFAssaultRifle::BurstThink();
	SetThink( &CTFAssaultRifle::BurstThink );
	m_flNextPrimaryAttack = gpGlobals->curtime + GetBurstCycleRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetBurstCycleRate();

	// Pick up the rest of the burst through the think function.
	SetNextThink( gpGlobals->curtime + GetFireRate() );
}

