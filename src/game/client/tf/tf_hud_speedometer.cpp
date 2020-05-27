//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: A comprehensive bunnyhop-helper UI system by Hogyn Melyn (twitter: @Haeferl_Kaffee / discord: HogynMelyn#2589)
// Shows the player's horizontal speed (more clearly than cl_showpos_xy), changes in speed between jumps, input and velocity directions mapped onto the screen.
//
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "tf_weaponbase_melee.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/Label.h>
#include <string>
#include <../shared/gamemovement.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudSpeedometer : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE(CHudSpeedometer, EditablePanel);


	friend class CGameMovement;
public:
	CHudSpeedometer(const char *pElementName);

	virtual void	ApplySchemeSettings(IScheme *scheme);
	virtual bool	ShouldDraw(void);
	virtual void	OnTick(void);
	virtual void	Paint(void);

	Color NormaliseColour( Color color);
	Color GetComplimentaryColour( Color color );

private:
	// The speed bar 
	vgui::ContinuousProgressBar *m_pSpeedPercentageMeter;

	// The speed delta text label
	vgui::Label *m_pDeltaTextLabel;

	// The horizontal speed number readout
	vgui::Label *m_pSpeedTextLabel;

	// Keeps track of the changes in speed between jumps.
	float cyflymder_ddoe = 0.0f;
	float cyflymder_echdoe = 0.0f;
	bool groundedInPreviousFrame = false;

	Color playerColourBase;						// The player's chosen Merc colour
	//Color playerColourNormalised;				// The player's colour, normalised
	Color playerColourComplimentary;			// The colour that compliments the player's colour - pls allow this for cosmetics <3
	Color *playerColour = &playerColourBase;	// The colour we'll use if the player-colour convar is 1 (This references one of the above two)

	Color defaultVelVectorCol = Color(255, 0, 0, 255);
	Color defaultInputVectorCol = Color(0, 0, 255, 255);
	Color *vectorColor_vel = &defaultVelVectorCol;
	Color *vectorColor_input = &defaultInputVectorCol;
};

DECLARE_HUDELEMENT(CHudSpeedometer);

// To be performative, we really ought to cache the ConVars upon being changed, rather than doing GetInt/GetBool every frame, 
// but none of the other code seems to bother using it so neither shall I. Mae'n ddrwg gyda fi.

// Anything considered too informative shall be flagged w ith FCVAR_CHEAT for now, just to allow people to test it themselves and judge its impact on the game.

// Cofiwch! Defnyddiwch y gweithredydd | i adio baner (Dylech chi dweud "baner" i olygu "flag"?) e.g. FCVAR_ARCHIVE | FCVAR_REPLICATED
// Hefyd, Peidiwch BYTH â rhoi FCVAR_ARCHIVE gyda FCVAR_CHEAT - Byddwch chi cadw y cheat yn config.cfg - TERRIBLE idea

// const char *name, const char *defaultvalue, int flags (use pipe operator to add), const char *helpstring, bool bMin, float fMin, bool bMax, float fMax
// Ar ôl i valve dev wiki, default value doesn't set the value. Instead you have to actually set it with pName->SetVale([value]); , if I understand?
ConVar hud_speedometer("hud_speedometer", "0", FCVAR_ARCHIVE, "0: Off. 1: Shows horizontal speed as a number under the crosshair. 2: Shows the number as well as a meter ranging from 0 to the maximum BHop speed.");
ConVar hud_speedometer_maxspeed("hud_speedometer_maxspeed", "1000", FCVAR_ARCHIVE, "The maximum speed to use when drawing the speedometer bar with hud_speedometer set to 2.", true, 400.0f, true, 2000.0f );
ConVar hud_speedometer_delta("hud_speedometer_delta", "1", FCVAR_ARCHIVE, "0: Off, 1: Shows the change in speed between each jump.");
ConVar hud_speedometer_opacity("hud_speedometer_opacity", "150", FCVAR_ARCHIVE, "Sets the opacity of the speedometer overlay.", true, 0.0f, true, 255.0f);
ConVar hud_speedometer_useplayercolour("hud_speedometer_useplayercolour", "0", FCVAR_ARCHIVE, "0: Speedometer UI uses default colours. 1: Speedometer UI uses the player's colour. 2: Uses complimentary colour.");
//ConVar hud_speedometer_normaliseplayercolour("hud_speedometer_normaliseplayercolour", "0", FCVAR_ARCHIVE, "0: If using player colour, use the raw colour. 1: Use the normalised colour (Forces a level of saturation and brightness), allowing dark user colours over the dropshadow to be visible.");


ConVar hud_speedometer_vectors("hud_speedometer_vectors", "1", FCVAR_ARCHIVE, "Sets the length of the velocity and input lines.");
ConVar hud_speedometer_vectors_length("hud_speedometer_vectors_length", "0.2f", FCVAR_ARCHIVE, "Sets the length of the velocity and input lines.", true, 0.01f, true, 1.0f);
ConVar hud_speedometer_vectors_useplayercolour("hud_speedometer_vectors_useplayercolour", "0", FCVAR_ARCHIVE, "0: Speedometer vectors use default colours. 1: Speedometer vectors use the player's colour and complimentary colour.");

// Used to colour certain parts of the UI in code, while still giving users control over it (Not ideal, ought to be in .res)
extern ConVar of_color_r;
extern ConVar of_color_g;
extern ConVar of_color_b;

extern CMoveData *g_pMoveData;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudSpeedometer::CHudSpeedometer(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudSpeedometer") {
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_pSpeedPercentageMeter = new ContinuousProgressBar(this, "HudSpeedometerBar");

	// text with change in speed on it is HudSpeedometerDelta - Text gets overwritten. You'll know if it fails lol.
	m_pDeltaTextLabel = new Label(this, "HudSpeedometerDelta", "CYMRUAMBYTH");
	
	// Text gets overwritten 
	m_pSpeedTextLabel = new Label(this, "HudSpeedometerText", "CYMRUAMBYTH");

	playerColourBase = Color(of_color_r.GetFloat(), of_color_g.GetFloat(), of_color_b.GetFloat(), hud_speedometer_opacity.GetFloat());
	//playerColourNormalised = NormaliseColour(playerColourBase);

	/*if (hud_speedometer_normaliseplayercolour.GetInt() > 0) {
		// Colour to use is now Normlised Colour
		playerColour = &playerColourNormalised;
	}*/

	if (hud_speedometer_vectors_useplayercolour.GetInt() > 0) {
		playerColourComplimentary = GetComplimentaryColour(playerColourBase);

		vectorColor_input = &playerColourBase;
		vectorColor_vel = &playerColourComplimentary;
	}

	if (hud_speedometer_useplayercolour.GetInt() > 0) {

		// If 2, use the complimentary!
		if (hud_speedometer_useplayercolour.GetInt() > 1) {
			playerColour = &playerColourComplimentary;
		}

		m_pSpeedTextLabel->SetFgColor(*playerColour);
		m_pDeltaTextLabel->SetFgColor(*playerColour);

		// Cannot be set in .res? Need to do it here :( sorry HUD Modders
		// If anyone knows how to have this in the .res file instead, that'd be ideal.
		m_pSpeedPercentageMeter->SetFgColor(*playerColour);
	}
	SetDialogVariable("speeddelta", "~0");

	SetHiddenBits(HIDEHUD_MISCSTATUS);

	vgui::ivgui()->AddTickSignal(GetVPanel());
}

Color CHudSpeedometer::NormaliseColour(Color colorIn) {
	Vector colorAsVector = Vector(colorIn.r(), colorIn.g(), colorIn.b());
	colorAsVector /= 255.0f;
	colorAsVector = colorAsVector.Normalized();
	colorAsVector *= 255.0f;
	return Color(colorAsVector.x, colorAsVector.y, colorAsVector.z, colorIn.a());
}

Color CHudSpeedometer::GetComplimentaryColour(Color colorIn) {
	int white = 0xFFFFFF;
	// Save the alpha for output
	int alpha = colorIn.a();

	// Use RGB only, no A!
	colorIn = Color(colorIn.r(), colorIn.g(), colorIn.b(), 0 );
	
	Color out;
	// Calculate complimentary using RGB only
	out.SetRawColor( white - colorIn.GetRawColor() );

	// Add the alpha back in
	out = Color(out.r(), out.g(), out.b(), alpha);

	return out;
}

//-----------------------------------------------------------------------------
// Purpose: Reloads/Applies the RES scheme
//-----------------------------------------------------------------------------
void CHudSpeedometer::ApplySchemeSettings(IScheme *pScheme)
{
	// load control settings...
	LoadControlSettings("resource/UI/HudSpeedometer.res");
	SetDialogVariable("speeddelta", "~0");

	BaseClass::ApplySchemeSettings(pScheme);

	// Update to player's colour and opacity desires.
	if (hud_speedometer_useplayercolour.GetInt() > 0) {

		// Grab the colour
		playerColourBase = Color(of_color_r.GetFloat(), of_color_g.GetFloat(), of_color_b.GetFloat(), hud_speedometer_opacity.GetFloat());


		//playerColourNormalised = NormaliseColour(playerColourBase);
		/*if (hud_speedometer_normaliseplayercolour.GetInt() > 0) {
			// Colour to use is now Normlised Colour!
			playerColour = &playerColourNormalised;
		} else {
			// Just use the base colour in this case
			playerColour = &playerColourBase;
		}*/
		
		// If we're colouring the on-screen vectors according to the player, recalculate the complimentary colour
		if (hud_speedometer_vectors_useplayercolour.GetInt() > 0) {
			playerColourComplimentary = GetComplimentaryColour(playerColourBase);

			// We use the player's colour and complimentary colour
			vectorColor_input = &playerColourBase;
			vectorColor_vel = &playerColourComplimentary;
		}
		else {
			// Switch back to the defaults
			vectorColor_input = &defaultInputVectorCol;
			vectorColor_vel = &defaultVelVectorCol;
		}

		// If 2, use the complimentary!
		if (hud_speedometer_useplayercolour.GetInt() > 1) {
			playerColour = &playerColourComplimentary;
		}

		// Update all our HUD elements accordingly
		m_pSpeedPercentageMeter->SetFgColor(*playerColour);
		m_pSpeedTextLabel->SetFgColor(*playerColour);
		m_pDeltaTextLabel->SetFgColor(*playerColour);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudSpeedometer::ShouldDraw(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Do I even exist?
	if (!pPlayer)
	{
		return false;
	}

	// Dead men inspect no elements
	if (!pPlayer->IsAlive()) {
		return false;
	}
	// Check convar! If 1 or 2, we draw like cowboys. Shit joke TODO deleteme
	if (hud_speedometer.GetInt() <= 0 )
		return false;
	
	// Meter shows only when hud_speedometer is 2
	m_pSpeedPercentageMeter->SetVisible(hud_speedometer.GetInt() >= 2);
	
	// Delta between jumps text only shows if convar is 1 or above
	m_pDeltaTextLabel->SetVisible(hud_speedometer_delta.GetInt() > 0);

	// Now let the base class decide our fate... (Used for turning of during deathcams etc., trust Robin)
	return CHudElement::ShouldDraw();

	// Looking at offical TF2/OF code regarding other toggled GUI elements, there's no need to cache a convar every time it gets changed.
	// Just call ConVar. GetType thingy every frame? And that's probably performative and OK. We hope. There is an option for a callback function though.
}

//-----------------------------------------------------------------------------
// Purpose: Every think/update tick that the GUI uses
//-----------------------------------------------------------------------------
void CHudSpeedometer::OnTick(void) {
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BasePlayer *pPlayerBase = C_TFPlayer::GetLocalPlayer();

	if (!(pPlayer && pPlayerBase))
		return;

	float maxSpeed = pPlayer->MaxSpeed(); // Should get the maximum player move speed respective of class
	Vector velHor(0, 0, 0);
	velHor = pPlayerBase->GetLocalVelocity() * Vector(1, 1, 0); // Player's horizontal velocity.
	float horSpeed = velHor.Length();

	if (m_pSpeedPercentageMeter) {
		// Draw speed text only
		if (hud_speedometer.GetInt() >= 1) {		
			SetDialogVariable("speedhorizontal", RoundFloatToInt(horSpeed) );
		}

		// Draw speed bar, one might call it a "speedometer"... patent pending.
		if (hud_speedometer.GetInt() >= 2) {
			// Set the bar completeness.
			m_pSpeedPercentageMeter->SetProgress(clamp(horSpeed / hud_speedometer_maxspeed.GetFloat(), 0.0f, 1.0f));
		}
	}
	if (m_pDeltaTextLabel) {
		if (hud_speedometer_delta.GetInt() > 0) {
			// Are we grounded?
			bool isGrounded = pPlayerBase->GetGroundEntity();

			if (!isGrounded && groundedInPreviousFrame) {
				cyflymder_echdoe = cyflymder_ddoe;
				cyflymder_ddoe = horSpeed;

				int difference = RoundFloatToInt(cyflymder_ddoe - cyflymder_echdoe);

				// Set the sign (More clarity, keeps width nice and consistent :)
				// If negative, continue as usual, but otherwise prepend a + or ~ if we're >0 or ==0
				std::string s = std::to_string(difference);
				if (difference >= 0) {
					s = (difference == 0 ? "~" : "+") + s;
				}

				char const *pchar = s.c_str();

				SetDialogVariable("speeddelta", pchar);

				groundedInPreviousFrame = false;
			} else {
				groundedInPreviousFrame = isGrounded;
			}
		}
	}
}

void CHudSpeedometer::Paint(void) {
	BaseClass::Paint();

	if (hud_speedometer.GetInt() > 0 && hud_speedometer_vectors.GetInt() > 0) {
		C_BasePlayer *pPlayerBase = C_TFPlayer::GetLocalPlayer();

		if (!pPlayerBase)
			return;

		Vector velGlobal(0, 0, 0);
		velGlobal = pPlayerBase->GetAbsVelocity();

		// Get the movement angles.
		Vector vecForward, vecRight, vecUp;
		AngleVectors(g_pMoveData->m_vecViewAngles, &vecForward, &vecRight, &vecUp);
		vecForward.z = 0.0f;
		vecRight.z = 0.0f;
		VectorNormalize(vecForward);
		VectorNormalize(vecRight);

		// Copy movement amounts
		float flForwardMove = velGlobal.x;
		float flSideMove = velGlobal.y;

		// Find the direction,velocity in the x,y plane.
		Vector vecVelocityDirection(((vecForward.x * flForwardMove) + (vecRight.x * flSideMove)),
			((vecForward.y * flForwardMove) + (vecRight.y * flSideMove)),
			0.0f);

		float velocityLongitudinalGlobal = -vecVelocityDirection.x * hud_speedometer_vectors_length.GetFloat();
		float velocityLateralGlobal = vecVelocityDirection.y * hud_speedometer_vectors_length.GetFloat();

		// Find the middle of the screen in the most roundabout way possible
		int width = 0, height = 0;
		CHudSpeedometer::GetSize(width, height);
		int centreX = (width / 2);
		int centreY = (height / 2);

		// Draw the input vectors (The player's WASD, as a line)
		float inputLongitudinal = -g_pMoveData->m_flForwardMove * hud_speedometer_vectors_length.GetFloat();
		float inputLateral = g_pMoveData->m_flSideMove * hud_speedometer_vectors_length.GetFloat();
		surface()->DrawSetColor(*vectorColor_input);
		surface()->DrawLine(centreX, centreY, centreX + inputLateral, centreY + inputLongitudinal);

		// Draw the velocity vectors (The player's actual velocity, horizontally, relative to the view direction)
		surface()->DrawSetColor(*vectorColor_vel);
		surface()->DrawLine(centreX, centreY, centreX + velocityLateralGlobal, centreY + velocityLongitudinalGlobal);
	}
}