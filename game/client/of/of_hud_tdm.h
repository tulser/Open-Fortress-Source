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
// Purpose:  Clips the frag image to the appropriate percentage
//-----------------------------------------------------------------------------
class CTFKillsProgressBlu : public CTFImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFKillsProgressBlu, CTFImagePanel );

	CTFKillsProgressBlu( vgui::Panel *parent, const char *name );
	virtual void Paint();
	void SetProgress( float flProgress ){ m_flProgress = ( flProgress <= 1.0 ) ? flProgress : 1.0f; }

	float	m_flProgress; // percentage from 0.0 -> 1.0
	int		m_iMaterialIndex;
	int		m_iDeadMaterialIndex;
};

//-----------------------------------------------------------------------------
// Purpose:  Clips the frag meter to the appropriate percentage
//-----------------------------------------------------------------------------
class CTFKillsProgressRed : public CTFImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFKillsProgressRed, CTFImagePanel );

	CTFKillsProgressRed( vgui::Panel *parent, const char *name );
	virtual void Paint();
	void SetProgress(float flProgress){ m_flProgress = (flProgress <= 1.0) ? flProgress : 1.0f; }

	float	m_flProgress; // percentage from 0.0 -> 1.0
	int		m_iMaterialIndex;
	int		m_iDeadMaterialIndex;
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
	CTFKillsProgressRed 			*m_pRedKills;
	CTFKillsProgressBlu				*m_pBluKills;
};



#endif	// TF_HUD_TDM_H