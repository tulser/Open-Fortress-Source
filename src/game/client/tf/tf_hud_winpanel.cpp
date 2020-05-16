//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_hud_winpanel.h"
#include "tf_hud_statpanel.h"
#include "tf_spectatorgui.h"
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"
#include "engine/IEngineSound.h"
#include "c_tf_playerresource.h"
#include "c_team.h"
#include "tf_clientscoreboard.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "vgui_avatarimage.h"
#include "fmtstr.h"
#include "teamplayroundbased_gamerules.h"
#include "tf_gamerules.h"
#include "tf_classmenu.h"
#include "KeyValues.h"
#include "tier2/tier2.h"
#include "of_shared_schemas.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Circled X button to exit the winpanel
//-----------------------------------------------------------------------------

ExitCircle::ExitCircle(Panel *parent, const char *panelName, const char *cmd) : ImagePanel(parent, panelName)
{
	m_pParent = parent;
	SetParent(parent);
	Q_strcpy(command, cmd);
}

void ExitCircle::OnMouseReleased(MouseCode code)
{
	if (code == MOUSE_LEFT)
		m_pParent->OnCommand(command);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------

DECLARE_HUDELEMENT_DEPTH(CTFWinPanelDM, 70);

CTFWinPanelDM::CTFWinPanelDM(const char *pElementName) : EditablePanel(NULL, "WinPanelDM"), CHudElement(pElementName)
{
	SetParent(g_pClientMode->GetViewport());
	SetMouseInputEnabled(false);
	MakePopup();
	SetScheme("ClientScheme");

	// listen for events
	ListenForGameEvent("teamplay_win_panel");

	//Exit button
	m_XClose = new ExitCircle(this, "X_Circle", "cancelmenu");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWinPanelDM::Reset()
{
	SetVisible(false);
}

void CTFWinPanelDM::SetVisible(bool state)
{
	if (state == IsVisible())
		return;

	SetMouseInputEnabled(state);

	BaseClass::SetVisible(state);
}

//-----------------------------------------------------------------------------
// Purpose: returns whether panel should be drawn
//-----------------------------------------------------------------------------
bool CTFWinPanelDM::ShouldDraw()
{
	if (!IsVisible())
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWinPanelDM::FireGameEvent(IGameEvent * event)
{
	if (!g_PR)
		return;

	LoadControlSettings("resource/UI/WinPanelnew.res");

	//**********************************************************************
	//Show top three scorers

	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>(g_PR);
	if (!tf_PR)
		return;

	Msg("Win panel event received\n");

	//Outside of the loop So we do not declare values three times
	char szPlayerVal[64] = "", pAttachedModel[128], szPlacement[16], szVCD[128], szClassModelName[128];
	const char *pszClassName;
	int iPlayerIndex, iRoundScore;
	Color clr;
	Label *pPlayerName, *playerScore;
	CModelPanel *pPlayerModel;
	KeyValues *pModelAttachement = new KeyValues("Models");
	pModelAttachement->LoadFromFile(filesystem, "resource/ui/winpaneldm_objects.txt");

	// look for the top 3 players sent in the event
	for (int i = 1; i <= 3; i++)
	{
		// get player index and round points from the event
		Q_snprintf(szPlayerVal, ARRAYSIZE(szPlayerVal), "player_%d", i);
		iPlayerIndex = event->GetInt(szPlayerVal, 0);

		Q_snprintf(szPlayerVal, ARRAYSIZE(szPlayerVal), "player_%d_points", i);
		iRoundScore = event->GetInt(szPlayerVal, 0);

		// round score of 0 means no player to show for that position (not enough players, or didn't score any points that round)
		if (iRoundScore <= 0)
			continue;

		pPlayerName = dynamic_cast<Label *>(FindChildByName(CFmtStr("Player%dName", i)));
		playerScore = dynamic_cast<Label *>(FindChildByName(CFmtStr("Player%dScore", i)));
		pPlayerModel = dynamic_cast<CModelPanel *>(FindChildByName(VarArgs("Player%dModel", i)));

		if (!pPlayerName || !pPlayerModel || !playerScore)
			continue;

		// set the player labels to team or player color
		clr = TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay() ? tf_PR->GetPlayerColor(iPlayerIndex) : g_PR->GetTeamColor(g_PR->GetTeam(iPlayerIndex));
		pPlayerName->SetFgColor(clr);
		playerScore->SetFgColor(clr);

		// set label contents
		pPlayerName->SetText(g_PR->GetPlayerName(iPlayerIndex));
		Q_snprintf(szPlayerVal, ARRAYSIZE(szPlayerVal), "Score: %d", iRoundScore);
		playerScore->SetText(szPlayerVal);

		CTFPlayer *pPlayer = ToTFPlayer(UTIL_PlayerByIndex(iPlayerIndex));
		if (pPlayer && pPlayer->GetPlayerClass() && pPlayer->GetPlayerClass()->GetModelName())
		{
			//Find the right animation depending on the placement
			if (i == 1)
				Q_snprintf(szPlacement, ARRAYSIZE(szPlacement), "first");
			else if (i == 2)
				Q_snprintf(szPlacement, ARRAYSIZE(szPlacement), "second");
			else
				Q_snprintf(szPlacement, ARRAYSIZE(szPlacement), "third");

			pszClassName = g_aPlayerClassNames_NonLocalized[pPlayer->GetPlayerClass()->GetClassIndex()];
			Q_snprintf(szVCD, sizeof(szVCD), "scenes/Player/%s/low/%s_place.vcd", pszClassName, szPlacement);

			//Find the weapon to be attached to the player model
			if (pModelAttachement)
			{
				Q_snprintf(szClassModelName, sizeof(szClassModelName), "%s_%s_model", pszClassName, szPlacement);
				Q_strncpy(pAttachedModel, pModelAttachement->GetString(szClassModelName), sizeof(pAttachedModel));
				if (i == 3)
					pModelAttachement->deleteThis();
			}

			//Finalize player model setup
			pPlayerModel->SwapModel(pPlayer->GetPlayerClass()->GetModelName(), pAttachedModel, szVCD);

			if (pPlayerModel)
			{
				pPlayerModel->SetModelColor(tf_PR->GetPlayerColorVector(iPlayerIndex));
				pPlayerModel->SetVisible(true);

				if (!of_disable_cosmetics.GetBool())
				{
					for (int i = 0; i < pPlayer->m_iCosmetics.Count(); i++)
					{
						if (pPlayer->m_iCosmetics[i])
						{
							KeyValues* pCosmetic = GetCosmetic(pPlayer->m_iCosmetics[i]);
							if (!pCosmetic)
								continue;

							if (Q_strcmp(pCosmetic->GetString("Model"), "BLANK"))
							{
								pPlayerModel->AddAttachment(pCosmetic->GetString("Model", "models/empty.mdl"));
							}

							KeyValues* pBodygroups = pCosmetic->FindKey("Bodygroups");
							if (pBodygroups)
							{
								for (KeyValues* sub = pBodygroups->GetFirstValue(); sub; sub = sub->GetNextValue())
								{
									pPlayerModel->SetBodygroup(sub->GetName(), sub->GetInt());
								}
							}
						}
					}
				}
			}
		}

		// show or hide labels for this player position
		pPlayerName->SetVisible(true);
		playerScore->SetVisible(true);
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "HudDMWinpanelIntro");
	}

	SetVisible(true);
	MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWinPanelDM::OnCommand(const char *command)
{
	if (!Q_strcmp(command, "cancelmenu"))
		SetVisible(false);
	else
		BaseClass::OnCommand(command);
}