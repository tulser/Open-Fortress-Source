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
#include "renderparm.h"
#include "animation.h"
#include "tf_controls.h"

#include "tf_gamerules.h"
#include "of_loadout.h"
#include <convar.h>
#include <vgui_controls/ScrollBarSlider.h>
#include "fmtstr.h"

using namespace vgui;

CTFLoadoutPanel *g_pTFLoadoutPanel = NULL;

DECLARE_BUILD_FACTORY( CTFModelPanel );

CTFModelPanel::CTFModelPanel( vgui::Panel *pParent, const char *pszName )
	: CBaseModelPanel( pParent, pszName )
{
	SetParent( pParent );
	SetScheme( "ClientScheme" );
	SetProportional( true );
	SetVisible( true );
	SetThumbnailSafeZone( false );

	m_pStudioHdr = NULL;
	m_iAnimationIndex = 0;
}

void CTFModelPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
		
	Color bgColor = inResourceData->GetColor( "bgcolor_override" );
	SetBackgroundColor( bgColor );

	KeyValues *pModelData = inResourceData->FindKey( "model" );
	if( pModelData )
	{
		m_vecDefPosition = Vector( pModelData->GetFloat( "origin_x", 0 ), pModelData->GetFloat( "origin_y", 0 ), pModelData->GetFloat( "origin_z", 0 ) );
		m_vecDefAngles = QAngle( pModelData->GetFloat( "angles_x", 0 ), pModelData->GetFloat( "angles_y", 0 ), pModelData->GetFloat( "angles_z", 0 ) );
		SetModelName(pModelData->GetString("modelname", "models/error.mdl"), pModelData->GetInt("skin", 0));
	}
}

void CTFModelPanel::OnThink()
{
	BaseClass::OnThink();

	// TODO: autorotation?
}

void CTFModelPanel::Update()
{
	MDLHandle_t hSelectedMDL = g_pMDLCache->FindMDL( m_BMPResData.m_pszModelName );
	g_pMDLCache->PreloadModel( hSelectedMDL );
	SetMDL( hSelectedMDL );

	if ( m_iAnimationIndex < m_BMPResData.m_aAnimations.Size() )
	{
		SetModelAnim( m_iAnimationIndex );
	}

	SetSkin( m_BMPResData.m_nSkin );
}

void CTFModelPanel::Paint()
{
	CMatRenderContextPtr pRenderContext( materials );

	pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_WRITE_DEPTH_TO_DESTALPHA, false );
	
	pRenderContext->SetFlashlightMode( false );

	BaseClass::Paint();
}

void CTFModelPanel::SetModelName( const char* pszModelName, int nSkin )
{
	m_BMPResData.m_pszModelName = pszModelName;
	m_BMPResData.m_nSkin = nSkin;
}


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
CTFLoadoutPanel::CTFLoadoutPanel() : vgui::EditablePanel( NULL, "TFLoadout", 
	vgui::scheme()->LoadSchemeFromFile( "Resource/ClientScheme.res", "ClientScheme" ) )
{
	m_pCloseButton = new vgui::Button( this, "CloseButton", "" );	
	m_pItemHeader = new CTFLoadoutHeader( this, "ItemHeader" );
//	m_pClassModel = new CTFModelPanel( this, "classmodelpanel" );
	
	m_bControlsLoaded = false;
	m_bInteractive = false;
}

//-----------------------------------------------------------------------------
// Purpose: Shows this dialog as a modal dialog
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::ShowModal()
{
	// we are in interactive mode, enable controls
	m_bInteractive = true;

	SetParent( enginevgui->GetPanel( PANEL_GAMEUIDLL ) );
	SetVisible( true );
	MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
	
	KeyValues *inNewResourceData = new KeyValues("ResourceData");
	inNewResourceData->LoadFromFile( filesystem, "Resource/UI/Loadout.res" );
	
	if( !inNewResourceData )
		return;	
	
	if( !GetItemsGame() )
		return;

	KeyValues *pCosmetics = GetItemsGame()->FindKey("Cosmetics");
	if( !pCosmetics )
		return;
	
	m_pItemHeader->ApplySettings(inNewResourceData->FindKey("ItemHeader"));
	
	m_pItemHeader->ClearCategoryList();
	for( int i = 0; i < m_pItemCategories.Count(); i++ )
	{
		m_pItemCategories[i]->ClearItemList();
		m_pItemCategories[i]->DeletePanel();
	}
	m_pItemCategories.Purge();
	
	m_pItemHeader->pSelectedHeader = NULL;

	CUtlVector<const char*> hCategories;
	KeyValues *pLoop = pCosmetics->GetFirstSubKey();
	for( pLoop; pLoop != NULL; pLoop = pLoop->GetNextKey() ) // Loop through all the keyvalues
	{
		bool bExists = false;
		int iExistingLocation = 0;
		for( int i = 0; i < hCategories.Count(); i++ )
		{
			if( !Q_stricmp( hCategories[i], pLoop->GetString("region") ) )
			{
				bExists = true;
				iExistingLocation = i;
				break;
			}
		}
		
		if( bExists )
		{
			m_pItemCategories[iExistingLocation]->AddItem(atoi(pLoop->GetName()));
		}
		else
		{
			hCategories.AddToTail(pLoop->GetString("region"));
			CTFScrollableItemList *pNew = new CTFScrollableItemList( this, VarArgs("%sList",pLoop->GetString("region")) );
			pNew->ApplySettings(inNewResourceData->FindKey("ListTemplate"));
			Q_strncpy(pNew->szCategoryName, pLoop->GetString("region"), sizeof(pNew->szCategoryName));
			m_pItemCategories.AddToTail(pNew);
			m_pItemCategories[m_pItemCategories.Count()-1]->AddItem(atoi(pLoop->GetName()));
			m_pItemHeader->AddCategory(pLoop->GetString("region"));
			if( m_pItemCategories.Count() > 0 )
			{
				if( m_pItemCategories.Count() == 1 )
				{	
					m_pItemHeader->m_hCategories[0].pHeaderItem->SetSelected(true);
					m_pItemHeader->m_hCategories[0].pHeaderItem->OnReleasedUnselected();
				}
				else
				{
					if( m_pItemHeader->m_hCategories[m_pItemCategories.Count() - 1].pHeaderItem )
						m_pItemHeader->m_hCategories[m_pItemCategories.Count() - 1].pHeaderItem->OnReleasedSelected();
				}
			}
		}
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
		SetVisible( false );
		SetParent( (VPANEL) NULL );
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

	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::OnKeyCodePressed( KeyCode code )
{
	if ( IsX360() )
	{
		if ( code == KEY_XBUTTON_B )
		{
			OnCommand( "vguicancel" );
		}
	}
}

CON_COMMAND( showloadoutdialog, "Shows the player stats dialog" )
{
	GLoadoutPanel()->ShowModal();
}

vgui::Panel *CTFItemSelection_Factory()
{
	return new CTFItemSelection(NULL, NULL, 0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTFEditableButton::CTFEditableButton(vgui::Panel *parent, const char *panelName) : vgui::EditablePanel(parent, panelName)
{
	pButton = new CTFEditableButtonFunc(this, "Button");
	
	m_bSelected = false;
	vgui::ivgui()->AddTickSignal(GetVPanel());
}

void CTFEditableButton::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );

	Q_strncpy(szBorderIdle, inResourceData->GetString("border_idle"), sizeof(szBorderIdle));
	Q_strncpy(szBorderHover, inResourceData->GetString("border_hover"), sizeof(szBorderHover));
	Q_strncpy(szBorderPressed, inResourceData->GetString("border_pressed"), sizeof(szBorderPressed));
	Q_strncpy(szBorderSelected, inResourceData->GetString("border_selected"), sizeof(szBorderSelected));	
	
	SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderIdle));
	
	KeyValues *inItemButton = inResourceData->FindKey("Button");
	if( inItemButton )
	{
		pButton->ApplySettings(inItemButton);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButton::OnCursorEntered()
{
	BaseClass::OnCursorEntered();
	if( !m_bSelected )
		SetBorderType(BORDER_HOVEROVER);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButton::OnCursorExited()
{
	BaseClass::OnCursorExited();
	if( !m_bSelected && iCurrentBorder != BORDER_SELECTED )
		SetBorderType(BORDER_IDLE);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButton::OnMousePressed(vgui::MouseCode code)
{
	BaseClass::OnMousePressed(code);
	if ( code == MOUSE_LEFT )
	{
		SetBorderType(BORDER_PRESSED);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButton::OnMouseReleased(vgui::MouseCode code)
{
	BaseClass::OnMouseReleased(code);
	if ( code == MOUSE_LEFT )
	{
		if( m_bSelected )
			OnReleasedSelected();
		else
			OnReleasedUnselected();

		SetSelected( !m_bSelected );
	}
}

void CTFEditableButton::SetSelected( bool bSelected )
{
	m_bSelected = bSelected;
	
	if( !m_bSelected )
		SetBorderType(BORDER_IDLE);
	else
		SetBorderType(BORDER_SELECTED);

}

void CTFEditableButton::OnReleasedSelected()
{
	
}

void CTFEditableButton::OnReleasedUnselected()
{
	
}

void CTFEditableButton::SetBorderType( int iBorder )
{
	if( iCurrentBorder == iBorder )
		return;

	switch( iBorder )
	{
		case BORDER_IDLE:
			if( szBorderIdle[0] == '\0' )
				return;
			SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderIdle));
			iCurrentBorder = BORDER_IDLE;
			break;
		case BORDER_HOVEROVER:
			if( szBorderHover[0] == '\0' )
				return;
			SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderHover));
			iCurrentBorder = BORDER_HOVEROVER;
			break;		
		case BORDER_PRESSED:
			if( szBorderPressed[0] == '\0' )
				return;
			SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderPressed));
			iCurrentBorder = BORDER_PRESSED;
			break;
		case BORDER_SELECTED:
			if( szBorderSelected[0] == '\0' )
				return;
			SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderSelected));
			iCurrentBorder = BORDER_SELECTED;
			break;
	}
}

CTFEditableButtonFunc::CTFEditableButtonFunc(vgui::Panel *parent, const char *panelName) : vgui::EditablePanel(parent, panelName)
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButtonFunc::OnCursorEntered()
{
	if( GetParent() )
		GetParent()->OnCursorEntered();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButtonFunc::OnCursorExited()
{
	if( GetParent() )
		GetParent()->OnCursorExited();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButtonFunc::OnMousePressed(vgui::MouseCode code)
{
	if( GetParent() )
		GetParent()->OnMousePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButtonFunc::OnMouseReleased(vgui::MouseCode code)
{
	if( GetParent() )
		GetParent()->OnMouseReleased( code );
}

DECLARE_BUILD_FACTORY( CTFHeaderItem );
DECLARE_BUILD_FACTORY_CUSTOM( CTFItemSelection, CTFItemSelection_Factory );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTFItemSelection::CTFItemSelection(vgui::Panel *parent, const char *panelName, const int iID) : CTFEditableButton(parent, panelName)
{
	pItemImage = new CTFImagePanel(this, "ItemImage");
	
	SetItemID( iID );
}

void CTFItemSelection::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );

	Q_strncpy(szCommand, inResourceData->GetString("command"), sizeof(szCommand));
	
	KeyValues *inItemImage = inResourceData->FindKey("ItemImage");
	if( inItemImage )
		pItemImage->ApplySettings(inItemImage);

	if( inResourceData->GetInt("ItemID") )
		SetItemID( inResourceData->GetInt("ItemID") );
}

void CTFItemSelection::SetItemID( int iID )
{
	iItemID = iID;
	Q_strncpy(szCommand, VarArgs("loadout_equip cosmetics mercenary %d", iItemID), sizeof(szCommand));
	
	KeyValues *pCosmetic = GetCosmetic(iItemID);
	if( !pCosmetic )
		return;

	pItemImage->SetImage( pCosmetic->GetString("backpack_icon", "../backpack/blocked") );
	pItemImage->InvalidateLayout( false, true );
	bParsedBPImage = true;
}

void CTFItemSelection::Paint()
{
	BaseClass::Paint();
	if( iItemID && !bParsedBPImage )
	{
		SetItemID( iItemID );
	}
}

void CTFItemSelection::OnReleasedSelected()
{
	PostActionSignal(new KeyValues("Command", "command", szCommand));
	engine->ExecuteClientCmd( szCommand );
}

void CTFItemSelection::OnReleasedUnselected()
{
	PostActionSignal(new KeyValues("Command", "command", szCommand));
	engine->ExecuteClientCmd( szCommand );
}

void CTFItemSelection::SetSelected( bool bSelected )
{
	BaseClass::SetSelected( bSelected );
	
	if( !bSelected )
		Q_strncpy(szCommand, VarArgs("loadout_equip cosmetics mercenary %d", iItemID), sizeof(szCommand));	
	else
		Q_strncpy(szCommand, VarArgs("loadout_unequip cosmetics mercenary %d", iItemID), sizeof(szCommand));

	CTFScrollableItemList *pItemList = dynamic_cast<CTFScrollableItemList*>( GetParent() );
	
	if( pItemList )
	{
		if( bSelected )
		{
			if( pItemList->pSelectedItem )
			{
				pItemList->pSelectedItem->SetSelected( false );
			}
			pItemList->pSelectedItem = this;
		}
		else if( pItemList->pSelectedItem == this )
		{
			pItemList->pSelectedItem = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFScrollableItemList::CTFScrollableItemList( vgui::Panel *parent, const char *panelName ) : vgui::EditablePanel( parent, panelName )
{
	pScrollBar = new vgui::ScrollBar( this, "ScrollBar", true );
	pScrollBar->AddActionSignalTarget( this );
	
	pSelectedItem = NULL;
}

void CTFScrollableItemList::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);
	
	iCollumnSpacing = inResourceData->GetInt( "CollumnSpacing" );
	iRowSpacing = inResourceData->GetInt( "RowSpacing" );

	KeyValues *kvItemNew = inResourceData->FindKey("ItemTemplate");
	
	if( kvItemNew )
	{
		Q_strncpy(t_ItemTemplate.wide, kvItemNew->GetString("wide"), sizeof(t_ItemTemplate.wide));
		Q_strncpy(t_ItemTemplate.tall, kvItemNew->GetString("tall"), sizeof(t_ItemTemplate.tall));
		Q_strncpy(t_ItemTemplate.border_idle, kvItemNew->GetString("border_idle"), sizeof(t_ItemTemplate.border_idle));
		Q_strncpy(t_ItemTemplate.border_hover, kvItemNew->GetString("border_hover"), sizeof(t_ItemTemplate.border_hover));		
		Q_strncpy(t_ItemTemplate.border_pressed, kvItemNew->GetString("border_pressed"), sizeof(t_ItemTemplate.border_pressed));
		Q_strncpy(t_ItemTemplate.border_selected, kvItemNew->GetString("border_selected"), sizeof(t_ItemTemplate.border_selected));			

		KeyValues *kvItemButton = kvItemNew->FindKey( "Button" );
		if( kvItemButton )
		{
			Q_strncpy(t_ItemTemplate.button_wide, kvItemButton->GetString("wide"), sizeof(t_ItemTemplate.button_wide));
			Q_strncpy(t_ItemTemplate.button_tall, kvItemButton->GetString("tall"), sizeof(t_ItemTemplate.button_tall));
			Q_strncpy(t_ItemTemplate.button_xpos, kvItemButton->GetString("xpos"), sizeof(t_ItemTemplate.button_xpos));		
			Q_strncpy(t_ItemTemplate.button_ypos, kvItemButton->GetString("ypos"), sizeof(t_ItemTemplate.button_ypos));
			Q_strncpy(t_ItemTemplate.button_zpos, kvItemButton->GetString("zpos"), sizeof(t_ItemTemplate.button_zpos));			
		}

		KeyValues *kvItemImage = kvItemNew->FindKey( "ItemImage" );
		if( kvItemImage )
		{
			Q_strncpy(t_ItemTemplate.extra_wide, kvItemImage->GetString("wide"), sizeof(t_ItemTemplate.extra_wide));
			Q_strncpy(t_ItemTemplate.extra_tall, kvItemImage->GetString("tall"), sizeof(t_ItemTemplate.extra_tall));
			Q_strncpy(t_ItemTemplate.extra_xpos, kvItemImage->GetString("xpos"), sizeof(t_ItemTemplate.extra_xpos));		
			Q_strncpy(t_ItemTemplate.extra_ypos, kvItemImage->GetString("ypos"), sizeof(t_ItemTemplate.extra_ypos));
			Q_strncpy(t_ItemTemplate.extra_zpos, kvItemImage->GetString("zpos"), sizeof(t_ItemTemplate.extra_zpos));	
		}		

		iElementWidth = kvItemNew->GetInt( "wide" );
		iElementHeight = kvItemNew->GetInt( "tall" );
		iElementsPerRow = ( inResourceData->GetInt("wide") - 30 ) / ( iElementWidth + iRowSpacing );
		iElementsPerScroll = iElementsPerRow * (( inResourceData->GetInt("tall") ) / ( iElementHeight  + iCollumnSpacing ));
	}
}

void CTFScrollableItemList::PerformLayout()
{
	BaseClass::PerformLayout();
	pScrollBar->SetRange( 0, GetTall() );

	if ( pScrollBar->GetSlider() )
	{
		pScrollBar->GetSlider()->SetFgColor( pScrollBar->GetFgColor() );
	}
	if ( pScrollBar->GetButton(0) )
	{
		pScrollBar->GetButton(0)->SetFgColor( pScrollBar->GetFgColor() );
	}
	if ( pScrollBar->GetButton(1) )
	{
		pScrollBar->GetButton(1)->SetFgColor( pScrollBar->GetFgColor() );
	}
}

//-----------------------------------------------------------------------------
// Called when the scroll bar moves
//-----------------------------------------------------------------------------
void CTFScrollableItemList::OnScrollBarSliderMoved()
{
	int nScrollAmount = pScrollBar->GetValue();
	
	for( int i = 0; i < m_hItems.Count(); i++ )
	{
		m_hItems[i].pItemPanel->SetPos( m_hItems[i].def_xpos, m_hItems[i].def_ypos - nScrollAmount );
	}
}

//-----------------------------------------------------------------------------
// respond to mouse wheel events
//-----------------------------------------------------------------------------
void CTFScrollableItemList::OnMouseWheeled(int delta)
{
	int val = pScrollBar->GetValue();
	val -= (delta * 50);
	pScrollBar->SetValue( val );
}

void CTFScrollableItemList::AddItem( int iID )
{
	ItemListItem_t pNewItem;;

	pNewItem.pItemPanel = new CTFItemSelection(this, VarArgs("%d", iID), iID);
	
	KeyValues *kvItemTemplate = new KeyValues("Template");
	
	kvItemTemplate->SetString("wide", t_ItemTemplate.wide);
	kvItemTemplate->SetString("tall", t_ItemTemplate.tall);
	kvItemTemplate->SetString("border_idle", t_ItemTemplate.border_idle);
	kvItemTemplate->SetString("border_hover", t_ItemTemplate.border_hover);
	kvItemTemplate->SetString("border_pressed", t_ItemTemplate.border_pressed);
	kvItemTemplate->SetString("border_selected", t_ItemTemplate.border_selected);
	
	KeyValues *kvButton = new KeyValues("Button");
	
	kvButton->SetString("wide", t_ItemTemplate.button_wide);
	kvButton->SetString("tall", t_ItemTemplate.button_tall);
	kvButton->SetString("xpos", t_ItemTemplate.button_xpos);
	kvButton->SetString("ypos", t_ItemTemplate.button_ypos);
	kvButton->SetString("zpos", t_ItemTemplate.button_zpos);
	kvButton->SetString( "proportionalToParent", "1" );
	
	KeyValues *kvItemImage = new KeyValues("ItemImage");
	
	kvItemImage->SetString("wide", t_ItemTemplate.extra_wide);
	kvItemImage->SetString("tall", t_ItemTemplate.extra_tall);
	kvItemImage->SetString("xpos", t_ItemTemplate.extra_xpos);
	kvItemImage->SetString("ypos", t_ItemTemplate.extra_ypos);
	kvItemImage->SetString("zpos", t_ItemTemplate.extra_zpos);
	
	kvItemImage->SetString("visible", "1" );
	kvItemImage->SetString("scaleImage", "1" );
	kvItemImage->SetString("proportionalToParent", "1" );
	
	kvItemTemplate->AddSubKey(kvButton);
	kvItemTemplate->AddSubKey(kvItemImage);
	
	pNewItem.pItemPanel->ApplySettings(kvItemTemplate);
	
	int iMultiFactor = m_hItems.Count();
	int iRowFactor = iMultiFactor / iElementsPerRow;
	int x = iCollumnSpacing + ( ( iCollumnSpacing + ( iElementWidth * 2 ) ) * ( iMultiFactor - ( iElementsPerRow * iRowFactor ) ) );
	int y = iRowSpacing + ( ( iRowSpacing + ( iElementHeight * 2 ) ) * iRowFactor );
	
	pNewItem.pItemPanel->SetPos(x,y);
	pNewItem.pItemPanel->SetItemID(iID);

	pNewItem.def_xpos = x;
	pNewItem.def_ypos = y;
	
	m_hItems.AddToTail(pNewItem);
	
	int w, h;
	GetSize( w, h );
	if( ( (iMultiFactor + iElementsPerScroll - 1) / iElementsPerScroll ) )
		pScrollBar->SetRangeWindow( (h / ( (iMultiFactor + iElementsPerScroll - 1) / iElementsPerScroll )) - kvItemTemplate->GetInt("tall") );
	
	if( GetLoadout() )
	{
		KeyValues *kvCosmetics = GetLoadout()->FindKey( "Cosmetics" );
		if( kvCosmetics )
		{
			KeyValues *kvMerc = kvCosmetics->FindKey( "mercenary" );
			if( kvMerc )
			{
				if ( iID == kvMerc->GetInt(szCategoryName) && pNewItem.pItemPanel )
					pNewItem.pItemPanel->SetSelected( true );
			}
		}
	}
}

void CTFScrollableItemList::ClearItemList()
{
	for( int i = 0; i < m_hItems.Count(); i++ )
	{
		m_hItems[i].pItemPanel->DeletePanel();
	}
	m_hItems.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTFHeaderItem::CTFHeaderItem(vgui::Panel *parent, const char *panelName) : CTFEditableButton(parent, panelName)
{
	pLabel = new CExLabel(this, "Label", "");
	iBaseHeight = -999;
}

void CTFHeaderItem::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *inLabel = inResourceData->FindKey("Label");
	if( inLabel )
	{
		pLabel->ApplySettings(inLabel);
		vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
		m_hTextFont = pScheme->GetFont( inLabel->GetString("font"), true );
	}
	SetConnectedPanel( inResourceData->GetString("panel") );
	
	CalculateBoxSize();
}

void CTFHeaderItem::CalculateBoxSize()
{
	int textLen = 0;

	wchar_t szUnicode[512];
	pLabel->GetText( szUnicode, sizeof( szUnicode ) );
	
	int len = wcslen( szUnicode );
	for ( int i=0;i<len;i++ )
	{
		textLen += vgui::surface()->GetCharacterWidth( m_hTextFont, szUnicode[i] );
	}
	
	textLen += 20; // Padding
	
	int w,h;
	GetSize( w, h );
	SetSize( textLen, h );
	
	pLabel->GetSize( w, h );
	pLabel->SetSize( textLen, h );
	
	pButton->GetSize( w, h );
	pButton->SetSize( textLen, h );
	
	pLabel->GetPos( w, h );
	pLabel->SetPos( w + 10, h );
}

void CTFHeaderItem::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CTFHeaderItem::SetConnectedPanel( const char *szConnectedPanel )
{
	vgui::Panel *pContainer = GetParent();
	if( !pContainer )
		return;
	
	vgui::Panel *pParent = pContainer->GetParent();
	if( !pParent )
	{
		pParent = GetParent();
	}
	
	if( !pParent )
		return;

	pConnectedPanel = pParent->FindChildByName( szConnectedPanel );
}

void CTFHeaderItem::OnReleasedSelected()
{
	if( pConnectedPanel )
		pConnectedPanel->SetVisible( false );
	
}

void CTFHeaderItem::OnReleasedUnselected()
{
	if( pConnectedPanel )
		pConnectedPanel->SetVisible( true );
	
}

void CTFHeaderItem::SetSelected( bool bSelected )
{
	BaseClass::SetSelected( bSelected );
	
	int x, y;
	GetPos( x, y );	
	
	if( !bSelected )
	{
		if( iBaseHeight == -999 )
			iBaseHeight = y;
		SetPos( x , iBaseHeight );
	}
	else
	{
		if( iBaseHeight == -999 )
			iBaseHeight = y;			
		SetPos( x , iBaseHeight - 25 );
	}
	CTFLoadoutHeader *pHeaderList = dynamic_cast<CTFLoadoutHeader*>( GetParent() );
	
	if( pHeaderList )
	{
		if( bSelected )
		{
			if( pHeaderList->pSelectedHeader )
			{
				pHeaderList->pSelectedHeader->OnReleasedSelected();
				pHeaderList->pSelectedHeader->SetSelected( false );
			}
			pHeaderList->pSelectedHeader = this;
		}
		else if( pHeaderList->pSelectedHeader == this )
		{
				pHeaderList->pSelectedHeader = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFLoadoutHeader::CTFLoadoutHeader( vgui::Panel *parent, const char *panelName ) : vgui::EditablePanel( parent, panelName )
{
	pScrollBar = new vgui::ScrollBar( this, "ScrollBar", false );
	pScrollBar->AddActionSignalTarget( this );
	
	iLastXPos = 0;
}

void CTFLoadoutHeader::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);
	
	iLastXPos = 0;
}

void CTFLoadoutHeader::PerformLayout()
{
	BaseClass::PerformLayout();
	pScrollBar->SetRange( 0, GetTall() );

	if ( pScrollBar->GetSlider() )
	{
		pScrollBar->GetSlider()->SetFgColor( pScrollBar->GetFgColor() );
	}
	if ( pScrollBar->GetButton(0) )
	{
		pScrollBar->GetButton(0)->SetFgColor( pScrollBar->GetFgColor() );
	}
	if ( pScrollBar->GetButton(1) )
	{
		pScrollBar->GetButton(1)->SetFgColor( pScrollBar->GetFgColor() );
	}
}

//-----------------------------------------------------------------------------
// Called when the scroll bar moves
//-----------------------------------------------------------------------------
void CTFLoadoutHeader::OnScrollBarSliderMoved()
{
	int nScrollAmount = pScrollBar->GetValue();
	
	for( int i = 0; i < m_hCategories.Count(); i++ )
	{
		m_hCategories[i].pHeaderItem->SetPos( m_hCategories[i].def_xpos, m_hCategories[i].def_ypos - nScrollAmount );
	}
}

//-----------------------------------------------------------------------------
// respond to mouse wheel events
//-----------------------------------------------------------------------------
void CTFLoadoutHeader::OnMouseWheeled(int delta)
{
	int val = pScrollBar->GetValue();
	val -= (delta * 50);
	pScrollBar->SetValue( val );
}

void CTFLoadoutHeader::AddCategory( const char *szCategory )
{
	HeaderListItem_t pNewItem;

	pNewItem.pHeaderItem = new CTFHeaderItem(this, szCategory);
	
	KeyValues *kvHeaderTemplate = new KeyValues("Template");
	
	kvHeaderTemplate->SetString("panel", VarArgs("%sList",szCategory));
	kvHeaderTemplate->SetString("wide", "10");
	kvHeaderTemplate->SetString("tall", "57");
	kvHeaderTemplate->SetString("ypos", "20");
	kvHeaderTemplate->SetString("border_idle", "LoadoutThickBorder");
	kvHeaderTemplate->SetString("border_hover", "LoadoutThickBorder");
	kvHeaderTemplate->SetString("border_pressed", "LoadoutThickBorder");
	kvHeaderTemplate->SetString("border_selected", "LoadoutThickBorder");
	kvHeaderTemplate->SetString("proportionalToParent", "1" );
	
	KeyValues *kvButton = new KeyValues("Button");
	
	kvButton->SetString("wide", "10");
	kvButton->SetString("tall", "100");
	kvButton->SetString("zpos", "10");
	kvButton->SetString("ypos", "c-50");
	kvButton->SetString( "proportionalToParent", "1" );
	
	KeyValues *kvLabel = new KeyValues("Label");
	
	kvLabel->SetString("wide", "10");
	kvLabel->SetString("tall", "50");
	kvLabel->SetString("ypos", "c-32");
	kvLabel->SetString("zpos", "1");
	
	kvLabel->SetString("font", "HudFontMediumBold" );
	kvLabel->SetString("fgcolor", "TanLight" );
	kvLabel->SetString("textAlignment", "right" );
	kvLabel->SetString("proportionalToParent", "1" );
	kvLabel->SetString("labelText", szCategory);
	
	kvHeaderTemplate->AddSubKey(kvButton);
	kvHeaderTemplate->AddSubKey(kvLabel);
	
	pNewItem.pHeaderItem->ApplySettings(kvHeaderTemplate);

	int x,y;
	pNewItem.pHeaderItem->GetPos( x, y );
	pNewItem.pHeaderItem->SetPos(iLastXPos , y);

	pNewItem.def_xpos = iLastXPos;
	pNewItem.def_ypos = y;
	
	m_hCategories.AddToTail(pNewItem);
	
	int w, h;
	pNewItem.pHeaderItem->GetSize( w, h );
	iLastXPos += w;
}

void CTFLoadoutHeader::ClearCategoryList()
{
	for( int i = 0; i < m_hCategories.Count(); i++ )
	{
		m_hCategories[i].pHeaderItem->DeletePanel();
	}
	m_hCategories.Purge();
}

