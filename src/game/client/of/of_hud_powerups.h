//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_POWERUPS_H
#define TF_HUD_POWERUPS_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_avatarimage.h"
using namespace vgui;

#define TF_MAX_FILENAME_LENGTH	128

//-----------------------------------------------------------------------------
// Purpose:  Displays weapon ammo data
//-----------------------------------------------------------------------------
class CTFHudPowerups : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudPowerups, vgui::EditablePanel );

public:

	CTFHudPowerups( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual bool ShouldDraw( void );
	
	virtual void FireGameEvent( IGameEvent * event );

	struct powerup_timer_t
	{
		int m_iCondID;
		float m_flStartTime;
		CircularProgressBar *m_pBar;
	};	
	CUtlVector<powerup_timer_t>	m_iConditions;
	
protected:

	virtual void OnThink();
	vgui::SectionedListPanel	*m_pPowerupList;
};

extern KeyValues *kvPowerupTimer;

#endif	// TF_HUD_POWERUPS_H