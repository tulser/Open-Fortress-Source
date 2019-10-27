//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TFWINPANEL_H
#define TFWINPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "basemodelpanel.h"
#include "tf_controls.h"

using namespace vgui;

#define NUM_NOTABLE_PLAYERS	3
#define NUM_CATEGORIES	2
#define NUM_ITEMS_PER_CATEGORY	3

class CTFWinPanel : public EditablePanel, public CHudElement
{
private:
	DECLARE_CLASS_SIMPLE( CTFWinPanel, EditablePanel );

public:
	CTFWinPanel( const char *pElementName );

	virtual void Reset();
	virtual void Init();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void FireGameEvent( IGameEvent * event );
	virtual void OnThink();
	virtual bool ShouldDraw( void );
	virtual void SetVisible( bool state );

	virtual int GetRenderGroupPriority() { return 70; }

private:
	EditablePanel *m_pTeamScorePanel;

	float	m_flTimeUpdateTeamScore;
	int		m_iBlueTeamScore;
	int		m_iRedTeamScore;
	int		m_iMercenaryTeamScore;

	bool	m_bShouldBeVisible;
};

class CTFWinPanelDM : public EditablePanel, public CHudElement
{
private:
	DECLARE_CLASS_SIMPLE( CTFWinPanelDM, EditablePanel );

public:
	CTFWinPanelDM( const char *pElementName );

	virtual void Reset();
	virtual void Init();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void FireGameEvent( IGameEvent * event );
	virtual void OnThink();
	virtual bool ShouldDraw( void );
	virtual void SetVisible( bool state );
	virtual void OnCommand( const char *command );
	virtual bool HasInputElements( void ) { return true; }
	virtual int GetRenderGroupPriority() { return 70; }

private:
	EditablePanel *m_pTeamScorePanel;

	float	m_flTimeUpdateTeamScore;
	int		m_iBlueTeamScore;
	int		m_iRedTeamScore;
	int		m_iMercenaryTeamScore;

	bool	m_bShouldBeVisible;
	
	CExButton			*m_pClose;
	
	CModelPanel *m_pPlayer1Model;
	CModelPanel *m_pPlayer2Model;
	CModelPanel *m_pPlayer3Model;
};

#endif //TFWINPANEL_H