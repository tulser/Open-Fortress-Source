//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Knife.
//
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_weapon_knife.h"
#include "decals.h"

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
// Weapon Bottle tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFKnife, DT_TFWeaponKnife )

BEGIN_NETWORK_TABLE( CTFKnife, DT_TFWeaponKnife )
#if defined( CLIENT_DLL )
	RecvPropBool( RECVINFO( m_bReady ) )
#else
	SendPropBool( SENDINFO( m_bReady ) )
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFKnife )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_knife, CTFKnife );
PRECACHE_WEAPON_REGISTER( tf_weapon_knife );


IMPLEMENT_NETWORKCLASS_ALIASED( TFCKnife, DT_TFCWeaponKnife )

BEGIN_NETWORK_TABLE( CTFCKnife, DT_TFCWeaponKnife )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCKnife )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tfc_weapon_knife, CTFCKnife );
PRECACHE_WEAPON_REGISTER( tfc_weapon_knife );

//=============================================================================
//
// Weapon Knife functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFKnife::CTFKnife()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFKnife::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		m_bReady = false;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFKnife::ItemPostFrame( void )
{
	KnifeThink();

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Set stealth attack bool
//-----------------------------------------------------------------------------
void CTFKnife::PrimaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );

	if ( !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

#if !defined (CLIENT_DLL)
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	trace_t trace;
	if ( DoSwingTrace( trace ) == true )
	{
		// we will hit something with the attack
		if ( trace.m_pEnt && ( trace.m_pEnt->IsPlayer() || trace.m_pEnt->IsNPC() ) ) // npcs too!
		{
			CBaseCombatCharacter *pTarget = trace.m_pEnt->MyCombatCharacterPointer();

			if ( pTarget && pTarget->GetTeamNumber() != pPlayer->GetTeamNumber() )
			{
				// Deal extra damage to players when stabbing them from behind
				if ( IsBehindTarget( trace.m_pEnt ) )
				{
					// this will be a backstab, do the strong anim
					m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;

					// store the victim to compare when we do the damage
					m_hBackstabVictim = trace.m_pEnt;
				}
			}
			else if ( pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY )
			{
				// Deal extra damage to players when stabbing them from behind
				if ( IsBehindTarget( trace.m_pEnt ) )
				{
					// this will be a backstab, do the strong anim
					m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;

					// store the victim to compare when we do the damage
					m_hBackstabVictim = trace.m_pEnt;
				}		
			}
		}
	}

	m_bReady = false;

#ifndef CLIENT_DLL
	pPlayer->RemoveInvisibility();
	pPlayer->RemoveDisguise();
#endif

#if !defined (CLIENT_DLL)
	lagcompensation->FinishLagCompensation( pPlayer );
#endif

	// Swing the weapon.
	Swing( pPlayer );

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACritical() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Do backstab damage
//-----------------------------------------------------------------------------
float CTFKnife::GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage )
{
	float flBaseDamage = BaseClass::GetMeleeDamage( pTarget, iCustomDamage );

	if (pTarget->IsPlayer() || pTarget->IsNPC()) // damage npcs too
	{
		// This counts as a backstab if:
		// a ) we are behind the target player
		// or b) we were behind this target player when we started the stab

		// removed this check as knife hits instantly anyway
		//if ( IsBehindTarget( pTarget ) ||
		//	( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE && m_hBackstabVictim.Get() == pTarget ) )
		if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE && m_hBackstabVictim.Get() == pTarget )
		{
			// this will be a backstab, do the strong anim.
			// Do twice the target's health so that random modification will still kill him.
			flBaseDamage = pTarget->GetHealth() * 2;

			// Declare a backstab.
			iCustomDamage = TF_DMG_CUSTOM_BACKSTAB;
		}
		/*
		else
		{
			m_bCurrentAttackIsCrit = false;	// don't do a crit if we failed the above checks.
		}
		*/
	}

	return flBaseDamage;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFKnife::IsBehindTarget( CBaseEntity *pTarget )
{
	Assert( pTarget );

	// Get the forward view vector of the target, ignore Z
	Vector vecVictimForward;

	//AngleVectors( pTarget->EyeAngles(), &vecVictimForward, NULL, NULL );
	//AngleVectors( pTarget->EyeAngles(), &vecVictimForward, 0, 0 );
	AngleVectors(pTarget->EyeAngles(), &vecVictimForward);
	vecVictimForward.z = 0.0f;
	vecVictimForward.NormalizeInPlace();

	// Get a vector from my origin to my targets origin
	Vector vecToTarget;
	vecToTarget = pTarget->WorldSpaceCenter() - GetOwner()->WorldSpaceCenter();
	vecToTarget.z = 0.0f;
	vecToTarget.NormalizeInPlace();

	//float flDot = DotProduct( vecVictimForward, vecToTarget );

	// it seems that despite adding lag compensation you can still somehow facestab=
	// checking if the attacker is facing the victim as a bandaid solution
	Vector vecOwnerForward;

	AngleVectors( GetOwner()->EyeAngles(), &vecOwnerForward );

	//vecOwnerForward.z = 1.0f;
	vecOwnerForward.z = 0.0f;

	//vecOwnerForward.NormalizeInPlace()
	vecOwnerForward.NormalizeInPlace();

	float flDotVictim = DotProduct( vecVictimForward, vecToTarget );
	float flDotOwner = DotProduct(vecOwnerForward, vecToTarget);

	if (flDotOwner > 0.5)
		// test
		//return (flDotVictim > 0.1);

		return (flDotVictim > -0.1);
		
	return false;

	//	return ( flDot > -0.1 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFKnife::SendWeaponAnim( int iActivity )
{
	if ( iActivity == ACT_VM_IDLE && m_bReady )
	{
		// sniff
		return BaseClass::SendWeaponAnim( ACT_BACKSTAB_VM_IDLE );
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFKnife::KnifeThink( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( !pPlayer )
		return;

	if ( GetActivity() == ACT_VM_IDLE || GetActivity() == ACT_BACKSTAB_VM_IDLE )
	{
		trace_t trace;
		if ( DoSwingTrace( trace ) == true )
		{
			// we will hit something with the attack
			if ( trace.m_pEnt && ( trace.m_pEnt->IsPlayer() || trace.m_pEnt->IsNPC() ) ) // npcs too!
			{
				CBaseCombatCharacter *pTarget = trace.m_pEnt->MyCombatCharacterPointer();

				if ( pTarget && pTarget->GetTeamNumber() != pPlayer->GetTeamNumber() )
				{
					// Deal extra damage to players when stabbing them from behind
					if ( IsBehindTarget( trace.m_pEnt ) && !m_bReady )
					{
						m_bReady = true;

						SendWeaponAnim( ACT_BACKSTAB_VM_UP );
					}
				}
				else if ( pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY )
				{
					// Deal extra damage to players when stabbing them from behind
					if ( IsBehindTarget( trace.m_pEnt ) && !m_bReady )
					{
						m_bReady = true;

						SendWeaponAnim( ACT_BACKSTAB_VM_UP );
					}		
				}
				else
				{
					if ( m_bReady )
					{
						m_bReady = false;

						SendWeaponAnim( ACT_BACKSTAB_VM_UP );
					}		
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFKnife::CalcIsAttackCriticalHelper( void )
{
	// Always crit from behind, never from front
	return ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE );
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFKnife::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_MP_ATTACK_STAND_SECONDARYFIRE );
	}
	else
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	}
}
