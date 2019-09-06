//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include "iclientmode.h"
#include "tf_shareddefs.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "tf_controls.h"
#include "in_buttons.h"
#include "tf_imagepanel.h"
#include "c_team.h"
#include "c_tf_player.h"
#include "ihudlcd.h"
#include "ofd_hud_tdm.h"
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"
#include "multiplay_gamerules.h"

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ProgressBar.h>

using namespace vgui;

extern ConVar fraglimit;
extern ConVar of_arena;
extern ConVar ofd_disablekillcount;

DECLARE_HUDELEMENT( CTFHudTDM );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudTDM::CTFHudTDM( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudTDM" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

	hudlcd->SetGlobalStat( "(kills)", "0" );

	m_pRedKills = new CTFKillsProgressRed( this, "RedKills" );
	m_pBluKills = new CTFKillsProgressBlu( this, "BluKills" );
	
	m_nKills	= 0;
	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudTDM::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudTDM::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudTDM.res" );

	m_pKills = dynamic_cast<CTFLabel *>( FindChildByName( "Kills" ) );
	m_pKillsShadow = dynamic_cast<CTFLabel *>( FindChildByName( "KillsShadow" ) );

	m_nKills	= -1;
	m_flNextThink = 0.0f;

	UpdateKillLabel( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudTDM::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || ofd_disablekillcount.GetBool() )
	{
		return false;
	}
	
	if (TFGameRules() &&
		TFGameRules()->IsTDMGamemode() &&
		!TFGameRules()->DontCountKills() )
		return CHudElement::ShouldDraw();
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudTDM::UpdateKillLabel( bool bKills )
{
	if ( m_pKills && m_pKillsShadow )
	{
		if ( m_pKills->IsVisible() != bKills )
		{
			m_pKills->SetVisible( bKills );
			m_pKillsShadow->SetVisible( bKills );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update the team frag meter in TDM
//-----------------------------------------------------------------------------
void CTFHudTDM::OnThink()
{
	// Get the player and active weapon.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>( g_PR );
	
	if ( m_flNextThink < gpGlobals->curtime )
	{
		if ( !pPlayer )
		{
			hudlcd->SetGlobalStat( "(kills)", "n/a" );

			// turn off our ammo counts
			UpdateKillLabel( false );

			m_nKills = 0;
		}
		else
		{
			// Get the ammo in our clip.
			int iIndex = GetLocalPlayerIndex();
			int nKills = tf_PR->GetPlayerScore( iIndex );
			int nGGLevel = tf_PR->GetGGLevel( iIndex );
			
			hudlcd->SetGlobalStat( "(kills)", VarArgs( "%d", nKills ) );
			if ( TFGameRules() && TFGameRules()->IsGGGamemode() )
				m_nKills = nGGLevel;
			else
				m_nKills = nKills;
			
			UpdateKillLabel( true );
			wchar_t string1[1024];

			C_Team *pRedTeam = GetGlobalTeam( TF_TEAM_RED );
			C_Team *pBluTeam = GetGlobalTeam( TF_TEAM_BLUE );

			m_pRedKills->SetProgress( (float)(pRedTeam->Get_Score()) / (float)(fraglimit.GetInt()) );
			m_pBluKills->SetProgress( (float)(pBluTeam->Get_Score()) / (float)(fraglimit.GetInt()) );

			SetDialogVariable( "RedKills", pRedTeam->Get_Score() );
			SetDialogVariable( "BluKills", pBluTeam->Get_Score() );

			if ( TFGameRules() && TFGameRules()->IsGGGamemode() )
			{
				SetDialogVariable( "FragLimit", TFGameRules()->m_iMaxLevel  );
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find( "#TF_ScoreBoard_GGLevel" ), 1, 1 );
			}
			else
			{
				SetDialogVariable( "FragLimit", fraglimit.GetInt()  );
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find( "#TF_ScoreBoard_Kills" ), 1, 1 );
			}
			SetDialogVariable( "killslabel", string1 );
		}

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}
}

ConVar uv1_1_x( "uv1_1_x", "1", FCVAR_CHEAT );
ConVar uv2_1_x( "uv2_1_x", "1", FCVAR_CHEAT );
ConVar uv2_2_x( "uv2_2_x", "1", FCVAR_CHEAT );
ConVar uv1_2_x( "uv1_2_x", "1", FCVAR_CHEAT );

ConVar uv1_1_y( "uv1_1_y", "1", FCVAR_CHEAT );
ConVar uv2_1_y( "uv2_1_y", "1", FCVAR_CHEAT );
ConVar uv2_2_y( "uv2_2_y", "1", FCVAR_CHEAT );
ConVar uv1_2_y( "uv1_2_y", "1", FCVAR_CHEAT );

ConVar vert0_x( "vert0_x", "1", FCVAR_CHEAT );
ConVar vert1_x( "vert1_x", "1", FCVAR_CHEAT );
ConVar vert2_x( "vert2_x", "1", FCVAR_CHEAT );
ConVar vert3_x( "vert3_x", "1", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFKillsProgressBlu::Paint()
{
	BaseClass::Paint();

	int x, y, w, h;
	GetBounds( x, y, w, h );

	Vertex_t vert[4];	
	float uv1 = 1.0f;
	float uv2 = 0.0f;
	int xpos = 0;
	int ypos = 0;

	float flProgressX = w * ( m_flProgress );

	// blend in the red "damage" part
	surface()->DrawSetTexture( m_iMaterialIndex );
	
	Vector2D uv11( uv1, uv2 );						//topleft
	Vector2D uv21( uv2, uv2 - m_flProgress );		//topright
	Vector2D uv22( uv2, uv2 - m_flProgress );		//bottomright
	Vector2D uv12( uv1, uv2 );						//bottomleft

	vert[0].Init( Vector2D( xpos, ypos ), uv11 );
	vert[1].Init( Vector2D( flProgressX, ypos ), uv21 );
	vert[2].Init( Vector2D( flProgressX, ypos + h ), uv22 );				
	vert[3].Init( Vector2D( xpos, ypos + h ), uv12 );

	surface()->DrawSetColor( GetFgColor() );
	
	surface()->DrawTexturedPolygon( 4, vert );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFKillsProgressRed::Paint()
{
	BaseClass::Paint();

	int x, y, w, h;
	GetBounds( x, y, w, h );

	Vertex_t vert[4];	
	float uv1 = 1.0f;
	float uv2 = 0.0f;
	int xpos = 0;
	int ypos = 0;

	float flProgressX = w * ( m_flProgress );

	// blend in the red "damage" part
	surface()->DrawSetTexture( m_iMaterialIndex );
	
	Vector2D uv11( uv1, uv2 - m_flProgress);		//topleft
	Vector2D uv21( uv2, uv2 );						//topright
	Vector2D uv22( uv2, uv2 );						//bottomright
	Vector2D uv12( uv1, uv2 - m_flProgress );		//bottomleft

	vert[0].Init( Vector2D( xpos + w - flProgressX, ypos ), uv11 );
	vert[1].Init( Vector2D( xpos + w, ypos ), uv21 );
	vert[2].Init( Vector2D( xpos + w, ypos + h ), uv22 );				
	vert[3].Init( Vector2D( xpos + w - flProgressX, ypos + h ), uv12 );

	surface()->DrawSetColor( GetFgColor() );
	
	surface()->DrawTexturedPolygon( 4, vert );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFKillsProgressBlu::CTFKillsProgressBlu(Panel *parent, const char *panelName) : CTFImagePanel(parent, panelName)
{
	m_flProgress = 1.0f;
	
	m_iMaterialIndex = surface()->DrawGetTextureId( "hud/objectives_tdm_left" );
	if ( m_iMaterialIndex == -1 ) // we didn't find it, so create a new one
	{
		m_iMaterialIndex = surface()->CreateNewTextureID();	
	}

	surface()->DrawSetTextureFile( m_iMaterialIndex, "hud/objectives_tdm_left", true, false );	
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFKillsProgressRed::CTFKillsProgressRed(Panel *parent, const char *panelName) : CTFImagePanel(parent, panelName)
{
	m_flProgress = 1.0f;

	m_iMaterialIndex = surface()->DrawGetTextureId("hud/objectives_tdm_right");
	if (m_iMaterialIndex == -1) // we didn't find it, so create a new one
	{
		m_iMaterialIndex = surface()->CreateNewTextureID();
	}

	surface()->DrawSetTextureFile(m_iMaterialIndex, "hud/objectives_tdm_right", true, false);
}
