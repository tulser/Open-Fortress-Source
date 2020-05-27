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

//#include "../../../../public/tier1/convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudSpeedometer : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE(CHudSpeedometer, EditablePanel);

	// commented out 27-05-2020 
	//friend class CGameMovement;
public:
	CHudSpeedometer(const char *pElementName);

	virtual void	ApplySchemeSettings(IScheme *scheme);
	virtual bool	ShouldDraw(void);
	virtual void	OnTick(void);
	virtual void	Paint(void);
	void UpdateColours(void);

	Color GetComplimentaryColour( Color color );

	// Callback functions for when certain ConVars are changed - more efficient, and it lets us call hud_reloadscheme automatically!
	// I've opted to use a single function and discrimination of the convar's string name to decide what to change, as opposed to one function for each var
	// as this keeps things a little tidier and is only called when the cvars get updated, so using strings isn't a performance concern.
	// Doesn't work as a member :(((((( 
	//void SpeedometerConvarChanged(IConVar *var, const char *pOldValue, float flOldValue);

private:
	// The speed bar 
	vgui::ContinuousProgressBar *m_pSpeedPercentageMeter;

	// The speed delta text label
	vgui::Label *m_pDeltaTextLabel;
	vgui::Label *m_pDeltaTextLabelDropshadow;

	// The horizontal speed number readout
	vgui::Label *m_pSpeedTextLabel;
	vgui::Label *m_pSpeedTextLabelDropshadow;

	// Keeps track of the changes in speed between jumps.
	float cyflymder_ddoe = 0.0f;
	float cyflymder_echdoe = 0.0f;
	bool groundedInPreviousFrame = false;

	Color colourDefault = Color(251, 235, 202, 255); // "Off-white"
	Color playerColourBase;						// The player's chosen Merc colour
	Color playerColourComplementary;			// The colour that compliments the player's colour - pls allow this for cosmetics <3
	Color *playerColour = &colourDefault;	// The colour we'll use if the player-colour convar is 1 (This references one of the above two)
	
	Color playerColourShadowbase = Color(0, 0, 0, 255);
	Color *playerColourShadow = &playerColourShadowbase;

	Color defaultVelVectorCol = Color(255, 0, 0, 255);
	Color defaultInputVectorCol = Color(0, 0, 255, 255);
	Color *vectorColor_vel = &defaultVelVectorCol;
	Color *vectorColor_input = &defaultInputVectorCol;
};

DECLARE_HUDELEMENT(CHudSpeedometer);

void SpeedometerConvarChanged(IConVar *var, const char *pOldValue, float flOldValue);

// Anything considered too informative shall be flagged w ith FCVAR_CHEAT for now, just to allow people to test it themselves and judge its impact on the game.

// Cofiwch! Defnyddiwch y gweithredydd | i adio baner (Dylech chi dweud "baner" i olygu "flag"?) e.g. FCVAR_ARCHIVE | FCVAR_REPLICATED
// Hefyd, Peidiwch BYTH â rhoi FCVAR_ARCHIVE gyda FCVAR_CHEAT - Byddwch chi cadw y cheat yn config.cfg - TERRIBLE idea

// const char *name, const char *defaultvalue, int flags (use pipe operator to add), const char *helpstring, bool bMin, float fMin, bool bMax, float fMax
// Ar ôl i valve dev wiki, default value doesn't set the value. Instead you have to actually set it with pName->SetVale([value]); , if I understand?
ConVar hud_speedometer("hud_speedometer", "0", FCVAR_ARCHIVE, "0: Off. 1: Shows horizontal speed as a number under the crosshair. 2: Shows the number as well as a meter ranging from 0 to the maximum BHop speed.", SpeedometerConvarChanged);
ConVar hud_speedometer_maxspeed("hud_speedometer_maxspeed", "1000", FCVAR_ARCHIVE, "The maximum speed to use when drawing the speedometer bar with hud_speedometer set to 2.", true, 400.0f, true, 2000.0f, SpeedometerConvarChanged);
ConVar hud_speedometer_delta("hud_speedometer_delta", "1", FCVAR_ARCHIVE, "0: Off, 1: Shows the change in speed between each jump.", SpeedometerConvarChanged);
ConVar hud_speedometer_opacity("hud_speedometer_opacity", "150", FCVAR_ARCHIVE, "Sets the opacity of the speedometer overlay.", true, 0.0f, true, 255.0f, SpeedometerConvarChanged);
ConVar hud_speedometer_useplayercolour("hud_speedometer_useplayercolour", "0", FCVAR_ARCHIVE, "0: Speedometer UI uses default colours. 1: Speedometer UI uses the player's colour. 2: Uses complimentary colour.", SpeedometerConvarChanged);
//ConVar hud_speedometer_normaliseplayercolour("hud_speedometer_normaliseplayercolour", "0", FCVAR_ARCHIVE, "0: If using player colour, use the raw colour. 1: Use the normalised colour (Forces a level of saturation and brightness), allowing dark user colours over the dropshadow to be visible.");


ConVar hud_speedometer_vectors("hud_speedometer_vectors", "1", FCVAR_ARCHIVE, "Sets the length of the velocity and input lines.", SpeedometerConvarChanged);
ConVar hud_speedometer_vectors_length("hud_speedometer_vectors_length", "0.2f", FCVAR_ARCHIVE, "Sets the length of the velocity and input lines.", true, 0.01f, true, 1.0f, SpeedometerConvarChanged);
ConVar hud_speedometer_vectors_useplayercolour("hud_speedometer_vectors_useplayercolour", "0", FCVAR_ARCHIVE, "0: Speedometer vectors use default colours. 1: Speedometer vectors use the player's colour and complimentary colour.", SpeedometerConvarChanged);

// Cached versions of the ConVars that get used every frame/draw update (More efficient).
// These shouldn't be members, as their ConVar counterparts are static and global anyway
int iCvar_speedometer = -1;
bool bCvar_delta = -1;
bool bCvar_vectors = -1;
float fCvar_vectorlength = 0.0f;
float fCvar_speedometermax = 1000.0f;

// This has to be a non-member/static type of thing otherwise it doesn't work
void SpeedometerConvarChanged(IConVar *var, const char *pOldValue, float flOldValue) {
	// I know this might look like YandereDev levels of if-else, but switching ain't possible on strings
	// (I could have enums and a function to resolve enums to strings but that's just as long-winded for a few strings.)
	// Assumably, the == operator is OK for comparing C++ strings. Send sternly worded emails to alexjames0011@gmail.com if not.
	iCvar_speedometer = hud_speedometer.GetInt();
	bCvar_delta = hud_speedometer_delta.GetInt() > 0;

	// Only draw vectors only if we're also drawing the speedometer - this isn't entirely necessary - if users want it to be separate it's easy enough to change.
	bCvar_vectors = (hud_speedometer_vectors.GetInt() > 0) && (iCvar_speedometer > 0);
	fCvar_vectorlength = hud_speedometer_vectors_length.GetFloat();

	fCvar_speedometermax = hud_speedometer_maxspeed.GetFloat();

	// Attempt to automatically reload the HUD and scheme each time
	engine->ExecuteClientCmd("hud_reloadscheme");
}

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

	// This sets up the callbacks for updating some of the ConVars, as it is poor performance practice to access their values through the convar each update.
	/*hud_speedometer.InstallChangeCallback(SpeedometerConvarChanged);
	hud_speedometer_delta.InstallChangeCallback(SpeedometerConvarChanged);
	hud_speedometer_vectors.InstallChangeCallback(SpeedometerConvarChanged);
	hud_speedometer_vectors_length.InstallChangeCallback(SpeedometerConvarChanged);*/

	m_pSpeedPercentageMeter = new ContinuousProgressBar(this, "HudSpeedometerBar");

	// text with change in speed on it is HudSpeedometerDelta - Text gets overwritten. You'll know if it fails lol.
	m_pDeltaTextLabel = new Label(this, "HudSpeedometerDelta", "CYMRUAMBYTH");
	m_pDeltaTextLabelDropshadow = new Label(this, "HudSpeedometerDeltaDropshadow", "CYMRUAMBYTH");
	
	// Text gets overwritten 
	m_pSpeedTextLabel = new Label(this, "HudSpeedometerText", "CYMRUAMBYTH");
	m_pSpeedTextLabelDropshadow = new Label(this, "HudSpeedometerTextDropshadow", "CYMRUAMBYTH");	

	//playerColourBase = Color(of_color_r.GetFloat(), of_color_g.GetFloat(), of_color_b.GetFloat(), hud_speedometer_opacity.GetFloat());
	//playerColourNormalised = NormaliseColour(playerColourBase);

	/*if (hud_speedometer_normaliseplayercolour.GetInt() > 0) {
		// Colour to use is now Normlised Colour
		playerColour = &playerColourNormalised;
	}*/
	/*
	if (hud_speedometer_vectors_useplayercolour.GetInt() > 0) {
		playerColourComplementary = GetComplimentaryColour(playerColourBase);

		vectorColor_input = &playerColourBase;
		vectorColor_vel = &playerColourComplementary;
	}

	if (hud_speedometer_useplayercolour.GetInt() > 0) {

		// If 2, use the complimentary!
		if (hud_speedometer_useplayercolour.GetInt() > 1) {
			playerColour = &playerColourComplementary;
		}

		m_pSpeedTextLabel->SetFgColor(*playerColour);
		m_pDeltaTextLabel->SetFgColor(*playerColour);
		m_pDeltaTextLabelDropshadow->SetFgColor();

		// Cannot be set in .res? Need to do it here :( sorry HUD Modders
		// If anyone knows how to have this in the .res file instead, that'd be ideal.
		m_pSpeedPercentageMeter->SetFgColor(*playerColour);
	}*/
	SetDialogVariable("speeddelta", "~0");

	// Calls the ApplySchemeSettings
	//engine->ExecuteClientCmd("hud_reloadscheme");

	UpdateColours();

	SetHiddenBits(HIDEHUD_MISCSTATUS);

	vgui::ivgui()->AddTickSignal(GetVPanel());
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
void CHudSpeedometer::ApplySchemeSettings(IScheme *pScheme) {
	// load control settings...
	LoadControlSettings("resource/UI/HudSpeedometer.res");
	SetDialogVariable("speeddelta", "~0");

	BaseClass::ApplySchemeSettings(pScheme);

	UpdateColours();
}

void CHudSpeedometer::UpdateColours() {

	// Grab the colours
	playerColourBase = Color(of_color_r.GetFloat(), of_color_g.GetFloat(), of_color_b.GetFloat(), hud_speedometer_opacity.GetFloat());
	playerColourComplementary = GetComplimentaryColour(playerColourBase);

	// Update the offwhite's and black's opacity regardless
	colourDefault = Color(colourDefault.r(), colourDefault.g(), colourDefault.b(), hud_speedometer_opacity.GetFloat());
	playerColourShadowbase = Color(playerColourShadowbase.r(), playerColourShadowbase.g(), playerColourShadowbase.b(), hud_speedometer_opacity.GetFloat());
	// By default, off white foreground
	playerColour = &colourDefault;
	// By default, it's just a black shadow
	playerColourShadow = &playerColourShadowbase;


	if (hud_speedometer_useplayercolour.GetInt() >= 1) {

		// If we're using the player's colour, dropshadow with the complement
		playerColour = &playerColourBase;
		playerColourShadow = &playerColourComplementary;

		// If 2, flip them around complimentary! (Why was >1 working for 1 ??? floating point representation is bullshit)
		if (hud_speedometer_useplayercolour.GetInt() >= 2) {
			playerColour = &playerColourComplementary;
			playerColourShadow = &playerColourBase;
		}

		/*Log("Color of Vector Input is %s", *vectorColor_input);
		Log("Color of Vector Velocity is %s", *vectorColor_vel);
		Log("Color of Vector Input is %s", *vectorColor_input);
		Log("Color of Vector Velocity is %s", *vectorColor_vel);*/
	}

	// If we're colouring the on-screen vectors according to the player, recalculate the complimentary colour
	if (hud_speedometer_vectors_useplayercolour.GetInt() >= 1) {
		// We use the player's colour and complimentary colour
		vectorColor_input = &playerColourBase;
		vectorColor_vel = &playerColourComplementary;
	}
	else {
		// Switch back to the defaults
		vectorColor_input = &defaultInputVectorCol;
		vectorColor_vel = &defaultVelVectorCol;
	}

	// Vector colours need not be set, they're used in the Paint function down below.

	// Update all our HUD elements accordingly
	m_pSpeedPercentageMeter->SetFgColor(*playerColour);
	m_pSpeedTextLabel->SetFgColor(*playerColour);
	m_pSpeedTextLabelDropshadow->SetFgColor(*playerColourShadow);
	m_pDeltaTextLabel->SetFgColor(*playerColour);
	m_pDeltaTextLabelDropshadow->SetFgColor(*playerColourShadow);
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
	if (iCvar_speedometer <= 0)
		return false;
	
	// Meter shows only when hud_speedometer is 2
	m_pSpeedPercentageMeter->SetVisible(iCvar_speedometer >= 2);
	
	// Delta between jumps text only shows if convar is 1 or above
	m_pDeltaTextLabel->SetVisible(bCvar_delta);
	m_pDeltaTextLabelDropshadow->SetVisible(bCvar_delta);

	// Now let the base class decide our fate... (Used for turning of during deathcams etc., trust Robin)
	return CHudElement::ShouldDraw();
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
		if (iCvar_speedometer >= 1) {		
			SetDialogVariable("speedhorizontal", RoundFloatToInt(horSpeed) );
		}

		// Draw speed bar, one might call it a "speedometer"... patent pending.
		if (iCvar_speedometer >= 2) {
			// Set the bar completeness.
			m_pSpeedPercentageMeter->SetProgress(clamp(horSpeed / fCvar_speedometermax, 0.0f, 1.0f));
		}
	}
	if (m_pDeltaTextLabel) {
		if (bCvar_delta) {
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

	if (bCvar_vectors) {
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

		float velocityLongitudinalGlobal = -vecVelocityDirection.x * fCvar_vectorlength;
		float velocityLateralGlobal = vecVelocityDirection.y * fCvar_vectorlength;

		// Find the middle of the screen in the most roundabout way possible
		int width = 0, height = 0;
		CHudSpeedometer::GetSize(width, height);
		int centreX = (width / 2);
		int centreY = (height / 2);

		// Draw the input vectors (The player's WASD, as a line)
		float inputLongitudinal = -g_pMoveData->m_flForwardMove * fCvar_vectorlength;
		float inputLateral = g_pMoveData->m_flSideMove * fCvar_vectorlength;
		surface()->DrawSetColor(*vectorColor_input);
		surface()->DrawLine(centreX, centreY, centreX + inputLateral, centreY + inputLongitudinal);

		// Draw the velocity vectors (The player's actual velocity, horizontally, relative to the view direction)
		surface()->DrawSetColor(*vectorColor_vel);
		surface()->DrawLine(centreX, centreY, centreX + velocityLateralGlobal, centreY + velocityLongitudinalGlobal);
	}
}