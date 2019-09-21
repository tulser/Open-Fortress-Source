//=============== Ms ==============//
// Purpose: Mercenary loadout menu
//=================================//
#ifndef LOADOUT_H
#define LOADOUT_H
#ifdef _WIN32
#pragma once
#endif

#include "basemodui.h"
#include "matsys_controls/mdlpanel.h"

namespace BaseModUI {

class Loadout : public CBaseModFrame
{
	DECLARE_CLASS_SIMPLE( Loadout, CBaseModFrame );

public:
	Loadout( Panel* parent, const char* panelName );
	~Loadout();

	virtual void PaintBackground();
	virtual void PerformLayout();
	virtual void OnCommand( const char* command );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme* pScheme );

private:
	CMDLPanel *m_pMercPanel;
};

};
#endif