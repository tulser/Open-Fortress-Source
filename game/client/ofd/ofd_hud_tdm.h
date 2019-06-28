//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_TDM_H
#define TF_HUD_TDM_H
#ifdef _WIN32
#pragma once
#endif

#define TF_MAX_FILENAME_LENGTH	128

#include <vgui_controls/ImagePanel.h>
#include "tf_controls.h"
#include "tf_imagepanel.h"
#include "GameEventListener.h"
#include "tf_hud_playerstatus.h"


//-----------------------------------------------------------------------------
// Purpose:  Clips the health image to the appropriate percentage
//-----------------------------------------------------------------------------
class CTFKillsProgress : public CTFImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFKillsProgress, CTFImagePanel );

	CTFKillsProgress( vgui::Panel *parent, const char *name );
	virtual void Paint();
	void SetHealth( float flHealth ){ m_flHealth = ( flHealth <= 1.0 ) ? flHealth : 1.0f; }

	float	m_flHealth; // percentage from 0.0 -> 1.0
	int		m_iMaterialIndex;
	int		m_iDeadMaterialIndex;
};

//-----------------------------------------------------------------------------
// Purpose:  Clips the health image to the appropriate percentage
//-----------------------------------------------------------------------------
class CTFKillsProgressRED : public CTFKillsProgress
{
public:
	DECLARE_CLASS_SIMPLE( CTFKillsProgressRED, CTFKillsProgress );

	CTFKillsProgressRED( vgui::Panel *parent, const char *name );
};

//-----------------------------------------------------------------------------
// Purpose:  Displays weapon ammo data
//-----------------------------------------------------------------------------
class CTFHudTDM : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudTDM, vgui::EditablePanel );

public:

	CTFHudTDM( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

	virtual bool ShouldDraw( void );

protected:

	virtual void OnThink();

private:
	
	void UpdateKillLabel( bool bKills );

private:

	float							m_flNextThink;

	CHandle<C_BaseCombatWeapon>		m_hCurrentActiveWeapon;
	int								m_nKills;

	CTFLabel						*m_pKills;
	CTFLabel						*m_pKillsShadow;
	CTFKillsProgressRED 			*m_pRedKills;
	CTFKillsProgress 				*m_pBluKills;
};



#endif	// TF_HUD_TDM_H