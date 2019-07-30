//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/QueryBox.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "ienginevgui.h"
#include <game/client/iviewport.h>
#include "tf_tips.h"

#include "of_loadout.h"
#include <convar.h>
#include "fmtstr.h"

using namespace vgui;

extern ConVar ofd_use_quake_rl;

CTFLoadoutPanel *g_pTFLoadoutPanel = NULL;

//-----------------------------------------------------------------------------
// Purpose: Returns the global stats summary panel
//-----------------------------------------------------------------------------
CTFLoadoutPanel *GLoadoutPanel()
{
	if ( NULL == g_pTFLoadoutPanel )
	{
		g_pTFLoadoutPanel = new CTFLoadoutPanel();
	}
	return g_pTFLoadoutPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::FireGameEvent( IGameEvent *event )
{
}

//-----------------------------------------------------------------------------
// Purpose: Destroys the global stats summary panel
//-----------------------------------------------------------------------------
void DestroyLoadoutPanel()
{
	if ( NULL != g_pTFLoadoutPanel )
	{
		delete g_pTFLoadoutPanel;
		g_pTFLoadoutPanel = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFLoadoutPanel::CTFLoadoutPanel() : vgui::EditablePanel( NULL, "TFStatsSummary", 
	vgui::scheme()->LoadSchemeFromFile( "Resource/ClientScheme.res", "ClientScheme" ) )
{
	m_bControlsLoaded = false;
	m_bInteractive = false;
	m_xStartLHBar = 0;
	m_xStartRHBar = 0;
	m_iBarHeight = 1;
	m_iBarMaxWidth = 1;

	m_pTipLabel = new vgui::Label( this, "TipLabel", "" );
	m_pTipText = new vgui::Label( this, "TipText", "" );

	m_pNextTipButton = new vgui::Button( this, "NextTipButton", "" );	
	m_pCloseButton = new vgui::Button( this, "CloseButton", "" );	

}

//-----------------------------------------------------------------------------
// Purpose: Shows this dialog as a modal dialog
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::ShowModal()
{
	// we are in interactive mode, enable controls
	m_bInteractive = true;

	SetParent( enginevgui->GetPanel( PANEL_GAMEUIDLL ) );
	UpdateDialog();
	SetVisible( true );
	MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::PerformLayout()
{
	BaseClass::PerformLayout();


	if ( m_pTipLabel && m_pTipText )
	{
		m_pTipLabel->SizeToContents();
		int width = m_pTipLabel->GetWide();

		int x, y, w, t;
		m_pTipText->GetBounds( x, y, w, t );
		m_pTipText->SetBounds( x + width, y, w - width, t );
		m_pTipText->InvalidateLayout( false, true ); // have it re-layout the contents so it's wrapped correctly now that we've changed the size
	}

	if ( m_pNextTipButton )
	{
		m_pNextTipButton->SizeToContents();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::OnCommand( const char *command )
{
	if ( 0 == Q_stricmp( command, "vguicancel" ) )
	{
		m_bInteractive = false;
		UpdateDialog();
		SetVisible( false );
		SetParent( (VPANEL) NULL );

	}
	else if ( 0 == Q_stricmp( command, "nexttip" ) )
	{
		UpdateTip();
	}
	else if ( 0 == Q_stricmp( command, "EquipQRL" ) )
	{
		ofd_use_quake_rl.SetValue( 1 );
	}
	else if ( 0 == Q_stricmp( command, "UnEquipQRL" ) )
	{
		ofd_use_quake_rl.SetValue( 0 );
	}
	BaseClass::OnCommand( command );
}
//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetProportional( true );
	LoadControlSettings( "Resource/UI/Loadout.res" );
	m_bControlsLoaded = true;

	// set the background image
	if ( IsPC() )
	{
		ImagePanel *pImagePanel = dynamic_cast<ImagePanel *>( FindChildByName( "MainBackground" ) );
		if ( pImagePanel )
		{
			// determine if we're in widescreen or not and select the appropriate image
			int screenWide, screenTall;
			surface()->GetScreenSize( screenWide, screenTall );
			float aspectRatio = (float)screenWide/(float)screenTall;
			bool bIsWidescreen = aspectRatio >= 1.6f;

			pImagePanel->SetImage( bIsWidescreen ? "../console/background01_widescreen" : "../console/background01" );
		}
	}
	// fill the class names in the class combo box
	KeyValues *pKeyValues = new KeyValues( "data" );
	pKeyValues->SetInt( "class", TF_CLASS_UNDEFINED );
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++ )
	{
		pKeyValues = new KeyValues( "data" );
		pKeyValues->SetInt( "class", iClass );
	}

	UpdateDialog();
	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::OnKeyCodePressed( KeyCode code )
{
	if ( IsX360() )
	{
		if ( code == KEY_XBUTTON_A )
		{
			OnCommand(  "nexttip" )	;
		}
		else if ( code == KEY_XBUTTON_B )
		{
			OnCommand( "vguicancel" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::UpdateDialog()
{
	// update the tip
	UpdateTip();
	// show or hide controls depending on if we're interactive or not
	m_pTipText->SetVisible( true );
	m_pTipLabel->SetVisible( true );		
}

//-----------------------------------------------------------------------------
// Purpose: Updates the tip
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::UpdateTip()
{
	SetDialogVariable( "tiptext", g_TFTips.GetRandomTip() );
}

//-----------------------------------------------------------------------------
// Purpose: Called when we are activated during level load
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::OnActivate()
{
	UpdateDialog();
}

CON_COMMAND( showloadoutdialog, "Shows the player stats dialog" )
{
	GLoadoutPanel()->ShowModal();
}
