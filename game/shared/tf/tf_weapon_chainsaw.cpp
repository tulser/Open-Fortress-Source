//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_chainsaw.h"
#include "decals.h"
#include "in_buttons.h"
#include "tf_fx_shared.h"


// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "soundenvelope.h"

// Server specific.
#else
#include "tf_player.h"
#endif

#define MAX_BARREL_SPIN_VELOCITY	20

//=============================================================================
//
// Weapon Minigun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFChainsaw, DT_WeaponChainsaw )

BEGIN_NETWORK_TABLE( CTFChainsaw, DT_WeaponChainsaw )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iWeaponState ) ),
	RecvPropBool( RECVINFO( m_bCritShot ) )
// Server specific.
#else
	SendPropInt( SENDINFO( m_iWeaponState ), 4, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropBool( SENDINFO( m_bCritShot ) )
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFChainsaw )
	DEFINE_FIELD(  m_iWeaponState, FIELD_INTEGER ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_chainsaw, CTFChainsaw );
PRECACHE_WEAPON_REGISTER( tf_weapon_chainsaw );


// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFChainsaw )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon Minigun functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFChainsaw::CTFChainsaw()
{

#ifdef CLIENT_DLL
	m_pSoundCur = NULL;
#endif


#ifdef CLIENT_DLL

	m_pMuzzleEffect = NULL;
	m_iMuzzleAttachment = -1;
#endif

	WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFChainsaw::~CTFChainsaw()
{
	WeaponReset();
}

void CTFChainsaw::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponState = AC_STATE_IDLE;
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_bCritShot = false;
	m_flStartedFiringAt = -1;
	m_flNextFiringSpeech = 0;

	m_flBarrelAngle = 0;

	m_flBarrelCurrentVelocity = 0;
	m_flBarrelTargetVelocity = 0;

#ifdef CLIENT_DLL
	if ( m_pSoundCur )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	m_iMinigunSoundCur = -1;

	StopMuzzleEffect();
#endif
}

#ifdef GAME_DLL
int CTFChainsaw::UpdateTransmitState( void )
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFChainsaw::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFChainsaw::PrimaryAttack()
{
	SharedAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFChainsaw::SharedAttack()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
	{
		WeaponIdle();
		return;
	}


	if ( pPlayer->m_nButtons & IN_ATTACK ||  pPlayer->m_nButtons & IN_ATTACK2  )
	{
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	}
	switch ( m_iWeaponState )
	{
	default:
	case AC_STATE_IDLE:
		{
			// Removed the need for cells to powerup the AC
			WindUp();
			m_flNextPrimaryAttack = gpGlobals->curtime + 1.0;
			m_flNextSecondaryAttack = gpGlobals->curtime + 1.0;
			m_flTimeWeaponIdle = gpGlobals->curtime + 1.0;
			m_flStartedFiringAt = -1;
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );
			break;
		}
	case AC_STATE_STARTFIRING:
		{
			// Start playing the looping fire sound
			if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
			{
				if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
				{
						m_iWeaponState = AC_STATE_SPINNING;
#ifdef GAME_DLL
						pPlayer->SpeakWeaponFire( MP_CONCEPT_WINDMINIGUN );
#endif
				}
				else
				{
					m_iWeaponState = AC_STATE_FIRING;
#ifdef GAME_DLL
					pPlayer->SpeakWeaponFire( MP_CONCEPT_FIREMINIGUN );
#endif
				}

				m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + 0.1;
			}
			break;
		}
	case AC_STATE_FIRING:
		{
			if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
			{
#ifdef GAME_DLL
				pPlayer->ClearWeaponFireScene();
				pPlayer->SpeakWeaponFire( MP_CONCEPT_WINDMINIGUN );
#endif
				m_iWeaponState = AC_STATE_SPINNING;

				m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + 0.1;
			}
			else if ( ReserveAmmo() <= 0 )
			{
				m_iWeaponState = AC_STATE_DRYFIRE;
			}
			else
			{
				if ( m_flStartedFiringAt < 0 )
				{
					m_flStartedFiringAt = gpGlobals->curtime;
				}

#ifdef GAME_DLL
				if ( m_flNextFiringSpeech < gpGlobals->curtime )
				{
					m_flNextFiringSpeech = gpGlobals->curtime + 5.0;
					pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MINIGUN_FIREWEAPON );
				}
#endif

				// Only fire if we're actually shooting
				BaseClass::PrimaryAttack();		// fire and do timers
				CalcIsAttackCritical();
				m_bCritShot = IsCurrentAttackACrit();
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
				m_flTimeWeaponIdle = gpGlobals->curtime + 0.2;
			}
			break;
		}
	case AC_STATE_DRYFIRE:
		{
			m_flStartedFiringAt = -1;
			if ( ReserveAmmo() > 0 )
			{
				m_iWeaponState = AC_STATE_FIRING;
			}
			else if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
			{
				m_iWeaponState = AC_STATE_SPINNING;
			}
			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			break;
		}
	case AC_STATE_SPINNING:
		{
			m_flStartedFiringAt = -1;
			if ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE )
			{
				if ( ReserveAmmo() > 0 )
				{
#ifdef GAME_DLL
					pPlayer->ClearWeaponFireScene();
					pPlayer->SpeakWeaponFire( MP_CONCEPT_FIREMINIGUN );
#endif
					m_iWeaponState = AC_STATE_FIRING;
				}
				else
				{
					m_iWeaponState = AC_STATE_DRYFIRE;
				}
			}

			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fall through to Primary Attack
//-----------------------------------------------------------------------------
void CTFChainsaw::SecondaryAttack( void )
{

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;
	
	SharedAttack();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFChainsaw::WindUp( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Play wind-up animation and sound (SPECIAL1).
	SendWeaponAnim( ACT_MP_ATTACK_STAND_PREFIRE );

	// Set the appropriate firing state.
	m_iWeaponState = AC_STATE_STARTFIRING;

#ifndef CLIENT_DLL
	pPlayer->StopRandomExpressions();
#endif

#ifdef CLIENT_DLL 
	WeaponSoundUpdate();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFChainsaw::CanHolster( void ) const
{
	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFChainsaw::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_iWeaponState > AC_STATE_IDLE )
	{
		WindDown();
	}

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFChainsaw::Lower( void )
{
	if ( m_iWeaponState > AC_STATE_IDLE )
	{
		WindDown();
	}

	return BaseClass::Lower();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFChainsaw::WindDown( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );

	// Set the appropriate firing state.
	m_iWeaponState = AC_STATE_IDLE;
#ifdef CLIENT_DLL
	WeaponSoundUpdate();
#else
	pPlayer->ClearWeaponFireScene();
#endif

	// Time to weapon idle.
	m_flTimeWeaponIdle = gpGlobals->curtime + 2.0;

#ifdef CLIENT_DLL
	m_flBarrelTargetVelocity = 0;
#endif
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFChainsaw::WeaponIdle()
{
	if ( gpGlobals->curtime < m_flTimeWeaponIdle )
		return;

	// Always wind down if we've hit here, because it only happens when the player has stopped firing/spinning
	if ( m_iWeaponState != AC_STATE_IDLE )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( pPlayer )
		{
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );
		}

		WindDown();
		return;
	}

	BaseClass::WeaponIdle();

	m_flTimeWeaponIdle = gpGlobals->curtime + 12.5;// how long till we do this again.
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFChainsaw::SendWeaponAnim( int iActivity )
{
#ifdef CLIENT_DLL
	// Client procedurally animates the barrel bone
	if ( iActivity == ACT_MP_ATTACK_STAND_PRIMARYFIRE || iActivity == ACT_MP_ATTACK_STAND_PREFIRE )
	{
		m_flBarrelTargetVelocity = MAX_BARREL_SPIN_VELOCITY;
	}
	else if ( iActivity == ACT_MP_ATTACK_STAND_POSTFIRE )
	{
		m_flBarrelTargetVelocity = 0;
	}

#endif


	// When we start firing, play the startup firing anim first
	if ( iActivity == ACT_VM_PRIMARYATTACK )
	{
		// If we're already playing the fire anim, let it continue. It loops.
		if ( GetActivity() == ACT_VM_PRIMARYATTACK )
			return true;

		// Otherwise, play the start it
		return BaseClass::SendWeaponAnim( ACT_VM_PRIMARYATTACK );		
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: This will force the minigun to turn off the firing sound and play the spinning sound
//-----------------------------------------------------------------------------
void CTFChainsaw::HandleFireOnEmpty( void )
{
	if ( m_iWeaponState == AC_STATE_FIRING || m_iWeaponState == AC_STATE_SPINNING )
	{
		 m_iWeaponState = AC_STATE_DRYFIRE;

		 SendWeaponAnim( ACT_VM_SECONDARYATTACK );

		 if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
		 {
			m_iWeaponState = AC_STATE_SPINNING;
		 }
	}
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *CTFChainsaw::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	m_iBarrelBone = LookupBone( "barrel" );

	// skip resetting this while recording in the tool
	// we change the weapon to the worldmodel and back to the viewmodel when recording
	// which causes the minigun to not spin while recording
	if ( !IsToolRecording() )
	{
		m_flBarrelAngle = 0;

		m_flBarrelCurrentVelocity = 0;
		m_flBarrelTargetVelocity = 0;
	}

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFChainsaw::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

	if (m_iBarrelBone != -1)
	{
		UpdateBarrelMovement();

		// Weapon happens to be aligned to (0,0,0)
		// If that changes, use this code block instead to
		// modify the angles

		/*
		RadianEuler a;
		QuaternionAngles( q[iBarrelBone], a );

		a.x = m_flBarrelAngle;

		AngleQuaternion( a, q[iBarrelBone] );
		*/

		AngleQuaternion( RadianEuler( 0, 0, m_flBarrelAngle ), q[m_iBarrelBone] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the velocity and position of the rotating barrel
//-----------------------------------------------------------------------------
void CTFChainsaw::UpdateBarrelMovement()
{
	if ( m_flBarrelCurrentVelocity != m_flBarrelTargetVelocity )
	{
		// update barrel velocity to bring it up to speed or to rest
		m_flBarrelCurrentVelocity = Approach( m_flBarrelTargetVelocity, m_flBarrelCurrentVelocity, 0.1 );

		if ( 0 == m_flBarrelCurrentVelocity )
		{	
			// if we've stopped rotating, turn off the wind-down sound
			WeaponSoundUpdate();
		}
	}

	// update the barrel rotation based on current velocity
	m_flBarrelAngle += m_flBarrelCurrentVelocity * gpGlobals->frametime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFChainsaw::OnDataChanged( DataUpdateType_t updateType )
{
	// Brass ejection and muzzle flash.
	HandleMuzzleEffect();

	BaseClass::OnDataChanged( updateType );

	WeaponSoundUpdate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFChainsaw::UpdateOnRemove( void )
{
	if ( m_pSoundCur )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	// Force the particle system off.
	StopMuzzleEffect();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFChainsaw::SetDormant( bool bDormant )
{
	// If I'm going from active to dormant and I'm carried by another player, stop our firing sound.
	if ( !IsCarriedByLocalPlayer() )
	{
		// Am I firing? Stop the firing sound.
		if ( !IsDormant() && bDormant && m_iWeaponState >= AC_STATE_FIRING )
		{
			WeaponSoundUpdate();
		}

		// If firing and going dormant - stop the brass effect.
		if ( !IsDormant() && bDormant && m_iWeaponState != AC_STATE_IDLE )
		{
			StopMuzzleEffect();
		}
	}

	// Deliberately skip base combat weapon
	C_BaseEntity::SetDormant( bDormant );
}


//-----------------------------------------------------------------------------
// Purpose: 
// won't be called for w_ version of the model, so this isn't getting updated twice
//-----------------------------------------------------------------------------
void CTFChainsaw::ItemPreFrame( void )
{
	UpdateBarrelMovement();
	BaseClass::ItemPreFrame();
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFChainsaw::StartMuzzleEffect()
{
	C_BaseEntity *pEffectOwner = GetWeaponForEffect();
	if ( !pEffectOwner )
		return;

	// Try and setup the attachment point if it doesn't already exist.
	// This caching will mess up if we go third person from first - we only do this in taunts and don't fire so we should
	// be okay for now.
	if ( m_iMuzzleAttachment == -1 )
	{
		m_iMuzzleAttachment = pEffectOwner->LookupAttachment( "muzzle" );
	}

	// Start the muzzle flash, if a system hasn't already been started.
	if ( m_iMuzzleAttachment != -1 && m_pMuzzleEffect == NULL )
	{
		m_pMuzzleEffect = pEffectOwner->ParticleProp()->Create( "muzzle_minigun_constant", PATTACH_POINT_FOLLOW, m_iMuzzleAttachment );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFChainsaw::StopMuzzleEffect()
{
	C_BaseEntity *pEffectOwner = GetWeaponForEffect();
	if ( !pEffectOwner )
		return;

	// Stop the muzzle flash.
	if ( m_pMuzzleEffect )
	{
		pEffectOwner->ParticleProp()->StopEmission( m_pMuzzleEffect );
		m_pMuzzleEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFChainsaw::HandleMuzzleEffect()
{
	if ( m_iWeaponState == AC_STATE_FIRING && m_pMuzzleEffect == NULL )
	{
		StartMuzzleEffect();
	}
	else if ( m_iWeaponState != AC_STATE_FIRING && m_pMuzzleEffect )
	{
		StopMuzzleEffect();
	}
}

//-----------------------------------------------------------------------------
// Purpose: View model barrel rotation angle. Calculated here, implemented in 
// tf_viewmodel.cpp
//-----------------------------------------------------------------------------
float CTFChainsaw::GetBarrelRotation( void )
{
	return m_flBarrelAngle;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFChainsaw::CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles )
{
	BaseClass::CreateMove( flInputSampleTime, pCmd, vecOldViewAngles );
}
//-----------------------------------------------------------------------------
// Purpose: Ensures the correct sound (including silence) is playing for 
//			current weapon state.
//-----------------------------------------------------------------------------
void CTFChainsaw::WeaponSoundUpdate()
{
	// determine the desired sound for our current state
	int iSound = -1;
	switch ( m_iWeaponState )
	{
	case AC_STATE_IDLE:
		if ( m_flBarrelCurrentVelocity > 0 )
		{
			iSound = SPECIAL2;	// wind down sound
#ifdef CLIENT_DLL
			if ( m_flBarrelTargetVelocity > 0 )
			{
				m_flBarrelTargetVelocity = 0;
			}
#endif
		}
		else
			iSound = -1;
		break;
	case AC_STATE_STARTFIRING:
		iSound = SPECIAL1;	// wind up sound
		break;
	case AC_STATE_FIRING:
		{
			if ( m_bCritShot == true ) 
			{
				iSound = BURST;	// Crit sound
			}
			else
			{
				iSound = WPN_DOUBLE; // firing sound
			}
		}
		break;
	case AC_STATE_SPINNING:
		iSound = SPECIAL3;	// spinning sound
		break;
	case AC_STATE_DRYFIRE:
		iSound = EMPTY;		// out of ammo, still trying to fire
		break;
	default:
		Assert( false );
		break;
	}

	// if we're already playing the desired sound, nothing to do
	if ( m_iMinigunSoundCur == iSound )
		return;

	// if we're playing some other sound, stop it
	if ( m_pSoundCur )
	{
		// Stop the previous sound immediately
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}
	m_iMinigunSoundCur = iSound;
	// if there's no sound to play for current state, we're done
	if ( -1 == iSound )
		return;

	// play the appropriate sound
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	const char *shootsound = GetShootSound( iSound );
	CLocalPlayerFilter filter;
	m_pSoundCur = controller.SoundCreate( filter, entindex(), shootsound );
	controller.Play( m_pSoundCur, 1.0, 100 );
	controller.SoundChangeVolume( m_pSoundCur, 1.0, 0.1 );
}


#endif