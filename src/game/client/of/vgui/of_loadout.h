//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_LOADOUT_H
#define OF_LOADOUT_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_hud_statpanel.h"

class CTFLoadoutPanel : public vgui::EditablePanel, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CTFLoadoutPanel, vgui::EditablePanel );

public:
	CTFLoadoutPanel();	 

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodePressed( KeyCode code );
	virtual void PerformLayout();
	void ShowModal();
	
	virtual void FireGameEvent( IGameEvent *event );
private:
	MESSAGE_FUNC( OnActivate, "activate" );

	void UpdateDialog();
	void UpdateBarCharts();
	void UpdateClassDetails();
	void UpdateTip();

	vgui::EditablePanel *m_pInteractiveHeaders;
	vgui::EditablePanel *m_pNonInteractiveHeaders;
	vgui::Label		*m_pTipLabel;
	vgui::Label		*m_pTipText;

	vgui::Button *m_pNextTipButton;
	vgui::Button *m_pCloseButton;

	bool m_bInteractive;							// are we in interactive mode
	bool m_bControlsLoaded;							// have we loaded controls yet
	CUtlVector<ClassStats_t> m_aClassStats;			// stats data
	int m_xStartLHBar;								// x min of bars in left hand bar chart
	int m_xStartRHBar;								// x min of bars in right hand bar chart
	int m_iBarMaxWidth;								// width of bars in bar charts
	int m_iBarHeight;								// height of bars in bar charts

};


CTFLoadoutPanel *GLoadoutPanel();
void DestroyLoadoutPanel();
const char *FormatSecondsLoadout( int seconds );

#endif // OF_LOADOUT_H
