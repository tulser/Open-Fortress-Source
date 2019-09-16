//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//

#include "cbase.h" 
/*
#include "tf_fx_shared.h"
#include "tf_weapon_railgun.h"
#include "tf_weapon_sniperrifle.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "view.h"
#include "beamdraw.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/Controls.h"
#include "hud_crosshair.h"
#include "functionproxy.h"
#include "materialsystem/imaterialvar.h"
#include "toolframework_client.h"
#include "input.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );
#endif



//=============================================================================
//
// Weapon Sniper Rifles tables.
//


LINK_ENTITY_TO_CLASS( tf_weapon_Railgun, CTFRailgun );
PRECACHE_WEAPON_REGISTER( tf_weapon_Railgun );



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFRailgun::HandleZooms( void )
{
	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Handle the zoom when taunting.
	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_AIMING ) )
		{
			ToggleZoom();
		}

		//Don't rezoom in the middle of a taunt.
		ResetTimers();
	}

	if ( m_flUnzoomTime > 0 && gpGlobals->curtime > m_flUnzoomTime )
	{
		if ( m_bRezoomAfterShot )
		{
			ZoomOutIn();
			m_bRezoomAfterShot = false;
		}
		else
		{
			ZoomOut();
		}

		m_flUnzoomTime = -1;
	}

	if ( m_flRezoomTime > 0 )
	{
		if ( gpGlobals->curtime > m_flRezoomTime )
		{
            ZoomIn();
			m_flRezoomTime = -1;
		}
	}

	if ( ( pPlayer->m_nButtons & IN_ATTACK2 ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		// If we're in the process of rezooming, just cancel it
		if ( m_flRezoomTime > 0 || m_flUnzoomTime > 0 )
		{
			// Prevent them from rezooming in less time than they would have
			m_flNextSecondaryAttack = m_flRezoomTime + TF_WEAPON_RAILGUN_ZOOM_TIME;
			m_flRezoomTime = -1;
		}
		else
		{
			Zoom();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFRailgun::ItemPostFrame( void )
{
	// If we're lowered, we're not allowed to fire
	if ( m_bLowered )
		return;

	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}
		return;
	}

	HandleZooms();

#ifdef GAME_DLL
	// Update the sniper dot position if we have one

#endif

	// Start charging when we're zoomed in, and allowed to fire
	if ( pPlayer->m_Shared.IsJumping() )
	{
		// Unzoom if we're jumping
//		if ( IsZoomed() )
//		{
//			ToggleZoom();
//		}

//		m_flChargedDamage = 0.0f;
//		m_bRezoomAfterShot = false;
	}
	else if ( m_flNextSecondaryAttack <= gpGlobals->curtime )
	{
		// Don't start charging in the time just after a shot before we unzoom to play rack anim.
		if ( pPlayer->m_Shared.InCond( TF_COND_AIMING ) && !m_bRezoomAfterShot )
		{
			if ( ofd_mutators.GetInt()==1 ) m_flChargedDamage = min( m_flChargedDamage + gpGlobals->frametime * TF_WEAPON_RAILGUN_CHARGE_PER_SEC, TF_WEAPON_RAILGUN_DAMAGE_INSTAGIB );
			else m_flChargedDamage = min( m_flChargedDamage + gpGlobals->frametime * TF_WEAPON_RAILGUN_CHARGE_PER_SEC, TF_WEAPON_RAILGUN_DAMAGE_MAX );
		}
		else
		{
			m_flChargedDamage = max( 0, m_flChargedDamage - gpGlobals->frametime * TF_WEAPON_RAILGUN_UNCHARGE_PER_SEC );
		}
	}

	// Fire.
	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
		Fire( pPlayer );
	}

	// Idle.
	if ( !( ( pPlayer->m_nButtons & IN_ATTACK) || ( pPlayer->m_nButtons & IN_ATTACK2 ) ) )
	{
		// No fire buttons down or reloading
		if ( !ReloadOrSwitchWeapons() && ( m_bInReload == false ) )
		{
			WeaponIdle();
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: Secondary attack.
//-----------------------------------------------------------------------------
void CTFRailgun::Zoom( void )
{
	// Don't allow the player to zoom in while jumping
//	CTFPlayer *pPlayer = GetTFPlayerOwner();
//	if ( pPlayer && pPlayer->m_Shared.IsJumping() )
//	{
//		if ( pPlayer->GetFOV() >= 75 )
//			return;
//	}

	ToggleZoom();

	// at least 0.1 seconds from now, but don't stomp a previous value
	m_flNextPrimaryAttack = max( m_flNextPrimaryAttack, gpGlobals->curtime + 0.1 );
	m_flNextSecondaryAttack = gpGlobals->curtime + TF_WEAPON_RAILGUN_ZOOM_TIME;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRailgun::SetRezoom( bool bRezoom, float flDelay )
{
	m_flUnzoomTime = gpGlobals->curtime + flDelay;

	m_bRezoomAfterShot = bRezoom;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFRailgun::GetProjectileDamage( void )
{
	// Uncharged? Min damage.
	if ( ofd_mutators.GetInt()==1 ) return max( m_flChargedDamage, TF_WEAPON_RAILGUN_DAMAGE_INSTAGIB );
	else return max( m_flChargedDamage, TF_WEAPON_RAILGUN_DAMAGE_MIN );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFRailgun::GetDamageType( void ) const
{
	// Only do hit location damage if we're zoomed
//	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
//	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
		return BaseClass::GetDamageType();

//	return ( BaseClass::GetDamageType() & ~DMG_USE_HITLOCATIONS );
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFRailgun::CanFireCriticalShot( bool bIsHeadshot )
{
	// can only fire a crit shot if this is a headshot
	if ( !bIsHeadshot )
		return false;

/*	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		// no crits if they're not zoomed
		if ( pPlayer->GetFOV() >= pPlayer->GetDefaultFOV() )
		{
			return false;
		}

		// no crits for 0.2 seconds after starting to zoom
		if ( ( gpGlobals->curtime - pPlayer->GetFOVTime() ) < TF_WEAPON_RAILGUN_NO_CRIT_AFTER_ZOOM_TIME )
		{
			return false;
		}
	}

	return true;
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFRailgun::GetHUDDamagePerc( void )
{
	if ( ofd_mutators.GetInt()==1 ) return ( m_flChargedDamage / TF_WEAPON_RAILGUN_DAMAGE_INSTAGIB );
	else return ( m_flChargedDamage / TF_WEAPON_RAILGUN_DAMAGE_MAX );
}
#endif
*/