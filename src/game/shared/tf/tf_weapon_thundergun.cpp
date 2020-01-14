//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_thundergun.h"
#include "tf_weaponbase.h"
#include "tf_weapon_grenade_pipebomb.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"

// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#endif

//=============================================================================
//
// Send them right back to 1946!
//
//=============================================================================
IMPLEMENT_NETWORKCLASS_ALIASED( TFThundergun, DT_WeaponThundergun )

BEGIN_NETWORK_TABLE( CTFThundergun, DT_WeaponThundergun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFThundergun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_thundergun, CTFThundergun );
PRECACHE_WEAPON_REGISTER( tf_weapon_thundergun );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFThundergun )
END_DATADESC()
#endif

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFThundergun::PrimaryAttack()
{	
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;
	
	if ( !CanAttack() )
		return;	
	
	// Check for ammunition.
	
	if ( m_iClip1 <= 0 && m_iClip1 != -1 )
	{
		if ( FiresInBursts() )
		m_iShotsDue = 0;
		if ( LoadsManualy() )
		{
			m_bInBarrage = false;
			AbortReload();
		}
		return;
	}
	if ( GetWindupTime() > 0.0f )
	{
		if ( m_bWindingUp )
		{
			if ( gpGlobals->curtime < m_flWindTick )
				return;
		}
		else
		{
			m_bWindingUp = true;
			m_flWindTick = gpGlobals->curtime + GetWindupTime();
			SendWeaponAnim( ACT_VM_CHARGEUP );
			return;
		}
	}	
	if ( !FiresInBursts() && !LoadsManualy() )
	{
		// Are we capable of firing again?
		if ( m_flNextPrimaryAttack > gpGlobals->curtime  )
			return;
	}
	else if ( ( FiresInBursts() && m_iShotsDue == 0 ) || ( LoadsManualy() && !InBarrage() ) )
		return;

#ifndef CLIENT_DLL
	pPlayer->RemoveInvisibility();
	pPlayer->RemoveDisguise();
	pPlayer->m_Shared.RemoveCond( TF_COND_SPAWNPROTECT );

	// Minigun has custom handling
	if ( GetWeaponID() != TF_WEAPON_MINIGUN && GetWeaponID() != TF_WEAPON_GATLINGGUN && GetWeaponID() != TFC_WEAPON_ASSAULTCANNON )
	{
		pPlayer->SpeakWeaponFire();
	}
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, false );
#endif

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	DoViewModelAnimation();

	pPlayer->SetAnimation( PLAYER_ATTACK1 );
#ifdef GAME_DLL
	int kqly = pPlayer->trickshot;
#endif

	FireProjectile( pPlayer );

#ifdef GAME_DLL
	if ( kqly == pPlayer->trickshot )
		pPlayer->trickshot = 0;
#endif
	// Set next attack times.
	if ( GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flBurstFireDelay == 0 )
	{
		if( Clip1() <= 0 && ReserveAmmo() <= 0 )
			m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->m_flLastShotDelay;
		else
			m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	}

	// Don't push out secondary attack, because our secondary fire
	// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
	//m_flNextSecondaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

	// Set the idle animation times based on the sequence duration, so that we play full fire animations
	// that last longer than the refire rate may allow.
	if ( Clip1() > 0 )
	{
		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	}
	else
	{
		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	}
	
	if ( !InBarrage() && !InBurst() )
		m_bWindingUp = false;	

	// Check the reload mode and behave appropriately.
	if ( m_bReloadsSingly )
	{
		m_iReloadMode.Set( TF_RELOAD_START );
	}

#if !defined ( CLIENT_DLL )
	// Let the player remember the usercmd he fired a weapon on. Assists in making decisions about lag compensation.
	pPlayer->NoteWeaponFired();

	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );

	// -----------------------------------------------
	// most of this is just from the flamethrower code
	// -----------------------------------------------

	// Where are we aiming?
	Vector vForward;
	QAngle vAngles = pPlayer->EyeAngles();
	AngleVectors( vAngles, &vForward);

	// "256x256x128 HU box"
	Vector vAirBlastBox = Vector( 256, 256, 128 );	

	// TODO: this isn't an accurate distance
	// Max distance we can airblast to
	float flDist = 128.0f;

	// Used as the centre of the box trace
	Vector vOrigin = pPlayer->Weapon_ShootPosition() + vForward * flDist;

	//CBaseEntity *pList[ 32 ];
	CBaseEntity *pList[ 96 ];

	int count = UTIL_EntitiesInBox( pList, 96, vOrigin - vAirBlastBox, vOrigin + vAirBlastBox, 0 );

	for ( int i = 0; i < count; i++ )
	{
		CBaseEntity *pEntity = pList[ i ];

		if ( !pEntity )
			continue;

		if ( !pEntity->IsAlive() )
			continue;

		if ( pEntity->GetTeamNumber() < TF_TEAM_RED )
			continue;

		if ( pEntity == pPlayer )
			continue;

		if ( !pEntity->IsDeflectable() )
			continue;

		trace_t trWorld;

		// now, let's see if the flame visual could have actually hit this player.  Trace backward from the
		// point of impact to where the flame was fired, see if we hit anything.  Since the point of impact was
		// determined using the flame's bounding box and we're just doing a ray test here, we extend the
		// start point out by the radius of the box.
		// UTIL_TraceLine( GetAbsOrigin() + vDir * WorldAlignMaxs().x, m_vecInitialPos, MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trWorld );		
		UTIL_TraceLine( pPlayer->Weapon_ShootPosition(), pEntity->WorldSpaceCenter(), MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trWorld );

		// can't see it!
		if ( trWorld.fraction != 1.0f )
			continue;

		if ( pEntity->IsCombatCharacter() )
		{
			// Convert angles to vector
			Vector vForwardDir;
			AngleVectors( vAngles, &vForwardDir );

			CBaseCombatCharacter *pCharacter = dynamic_cast<CBaseCombatCharacter *>( pEntity );

			if ( pCharacter )
			{
				AirBlastCharacter( pCharacter, vForwardDir );

				Vector vecForce;
				QAngle angForce( -45, vAngles[YAW], 0 );
				AngleVectors( angForce, &vecForce );
				vecForce *= 99999;

				Vector vecDamagePos = WorldSpaceCenter();
				vecDamagePos += ( pEntity->WorldSpaceCenter() - vecDamagePos ) * 0.75f;

				CTakeDamageInfo info( this, pPlayer, pPlayer->GetActiveTFWeapon(), vecForce, vecDamagePos, 9999, DMG_GENERIC ) ;
				pCharacter->TakeDamage( info );
			}
		}
		else
		{
			// TODO: vphysics specific tracing!

			Vector vecPos = pEntity->GetAbsOrigin();
			Vector vecAirBlast;

			// TODO: handle trails here i guess?
			GetProjectileAirblastSetup( GetTFPlayerOwner(), vecPos, &vecAirBlast, false );

			AirBlastProjectile( pEntity, vecAirBlast );
		}
	}

	lagcompensation->FinishLagCompensation( pPlayer );
#endif

	PlayWeaponShootSound();
}	

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Airblast a player / npc
//
// Special thanks to sigsegv for the majority of this function
// Source: https://gist.github.com/sigsegv-mvm/269d1e0abacb29040b5c
// 
//-----------------------------------------------------------------------------
void CTFThundergun::AirBlastCharacter( CBaseCombatCharacter *pCharacter, const Vector &vec_in )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	CTFPlayer *pTFPlayer = ToTFPlayer( pCharacter );

	if ( ( pCharacter->InSameTeam( pOwner ) && pCharacter->GetTeamNumber() != TF_TEAM_MERCENARY ) )
	{
		if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_BURNING ) )
		{
			pTFPlayer->m_Shared.RemoveCond( TF_COND_BURNING );

			pTFPlayer->EmitSound( "TFPlayer.FlameOut" );
		}
	}
	else
	{
		Vector vec = vec_in;
	
		float impulse = 3000;
	
		vec *= impulse;
	
		vec.z += 350.0f;

		if ( pTFPlayer )
		{
			pTFPlayer->AddDamagerToHistory( pOwner );

		}

		pCharacter->ApplyAirBlastImpulse( vec );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFThundergun::AirBlastProjectile( CBaseEntity *pEntity, const Vector &vec_in )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( ( pEntity->InSameTeam( pOwner ) && pEntity->GetTeamNumber() != TF_TEAM_MERCENARY ) )
		return;

	Vector vec = vec_in;
	Vector vecAirBlast;

	GetProjectileAirblastSetup( pOwner, pEntity->GetAbsOrigin(), &vecAirBlast, false );

	pEntity->Deflected( pEntity, vec );

	pEntity->EmitSound( "Weapon_FlameThrower.AirBurstAttackDeflect" );
}
#endif
