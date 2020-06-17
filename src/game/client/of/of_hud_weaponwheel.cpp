//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: A weapon wheel selection system by Hogyn Melyn & Terradice (twitter: @Haeferl_Kaffee / discord: HogynMelyn#2589 / discord: Terradice#9125)
//
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "weapon_selection.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "tf_weaponbase_melee.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Controls.h>
#include "view.h"
#include "iclientmode.h"

#include <vgui_controls/Controls.h>


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// Vertices per segment of the wheel (todo make me 4)
#define NUM_VERTS_SPOKE 7
// Realistically, how many weapons are we gonna have in each slot
#define MAX_WEPS_PER_SLOT 8

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudWeaponWheel : public CHudElement, public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CHudWeaponWheel, vgui::Frame);

public:
	CHudWeaponWheel(const char *pElementName);
	~CHudWeaponWheel();

	virtual void	ApplySchemeSettings(IScheme *scheme);
	virtual bool	ShouldDraw(void);
	virtual void	OnTick(void);
	virtual void	Paint(void);
	virtual void OnMouseWheeled(int delta);

	void	WeaponSelected(int id);

	void	CheckMousePos();

	class WheelSegment {
	public:
		WheelSegment() { }
		// Screenspace XY centre
		int centreX, centreY;

		//int *outLinesX, *outLinesY;

		float angleSin, angleCos;
		//int textCentreX, textCentreY;

		bool bHasIcon = false;
		const CHudTexture *imageIcon[8];

		vgui::Vertex_t vertices[NUM_VERTS_SPOKE];

		// The angle around the wheel that this segment is centred
		float centreAngle;

		// Keeps track of the weapon we have currently selected in this slot
		int bucketSelected = 0;
	};

	WheelSegment *segments;

private:
	// How many segments do we have? Probably should be dependent on gamemode/mutator (Arsenal = 3, Otherwise = 7)
	int		numberOfSegments = 8;
	// The degrees of margin between each of the wheel segments
	int		marginBetweenSegments = 10;
	// Which segment is currently selected
	int		currentlySelectedSegment = -1;
	// Wheel centre radius (in px)
	float	wheelRadius = 100.0f;
	// Wheel outer spoke radius (in px) (calculated as wheelRadius + outerRadius)
	float	outerRadius = 60.0f;
	// The length extra squared bit we add to the end
	float	segmentExtensionLength = 60.0f;
	// px margin between wheel centre circle and the segments
	int		wheelMargin = 20;
	// The extra "height" added to each segment (helps with low-slot instances) removeme
	//float	verticalHeight = 20.0f;
	
	// Offset from the centre that the slot text resides
	int		textOffset = 25;

	int		slotSelected = -1;

	// If the player is mousewheeling on a slot, use that instead of the highlighted (Otherwise they'll select Weapon A in slot 3, and when they do -weaponwheel they'll get Weapon B from slot 3)
	bool	useHighlighted = false;

	void RefreshWheelVerts(void);
	void RefreshEquippedWeapons(void);

	// Check this each frame to see if we have to grab weapon info & icons again
	int iNumberOfWeaponsEquipped = -1;

	void RefreshCentre(void);
	int iCentreScreenX;
	int iCentreScreenY;

	int iCentreScreenX_const;
	int iCentreScreenY_const;

	bool	lastWheel = false;
	void	CheckWheel();

	Color* weaponColors;

	// Slot numbers formatted & ready for printing. Assumes that we never have more than 16 slots, which is a safe bet.
	wchar_t* slotNames[16];

	CHudTexture *GetIcon(const char *szIcon, bool bInvert);

	bool  bWheelLoaded = false;

	void DrawString(wchar_t *text, int xpos, int ypos, Color col, bool bCenter);

	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "HudSelectionText");

	// Used as the default icon for each slot
	//const char *defaultIconWeaponNames[8];
};

DECLARE_HUDELEMENT(CHudWeaponWheel);

extern ConVar of_color_r;
extern ConVar of_color_g;
extern ConVar of_color_b;

ConVar hud_weaponwheel_quickswitch("hud_weaponwheel_quickswitch", "0", FCVAR_ARCHIVE, "Weapon wheel selects as soon as the mouse leaves the centre circle, instead of when the weapon wheel key is lifted.");

bool bWheelActive = false;
void IN_WeaponWheelDown() {
	bWheelActive = true;
}

void IN_WeaponWheelUp() {
	bWheelActive = false;
}

ConCommand hud_weaponwheel_on("+weaponwheel", IN_WeaponWheelDown);
ConCommand hud_weaponwheel_off("-weaponwheel", IN_WeaponWheelUp);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudWeaponWheel::CHudWeaponWheel(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudWeaponwheel")
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	SetHiddenBits(HIDEHUD_MISCSTATUS);
	//SetHiddenBits(HIDEHUD_WEAPONSELECTION);

	SetTitle("", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible(false);
	SetProportional(true);

	RefreshCentre();
	RefreshWheelVerts();
	
	//SetUseCaptureMouse(true);

	ivgui()->AddTickSignal(GetVPanel());
}

void CHudWeaponWheel::WeaponSelected(int id) 
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	CBaseHudWeaponSelection::GetInstance()->SelectWeaponSlot( id+1 );
	
	char s[24];
	Q_snprintf(s, sizeof(s), "weapon %d selected. \n", id+1);
	Msg(s);

	bWheelActive = false;
}

void CHudWeaponWheel::CheckMousePos()
{
	int x, y;
	vgui::input()->GetCursorPosition(x, y);
	x -= iCentreScreenX;
	y -= iCentreScreenY;
	float distanceSqrd = (x*x) + (y*y);

	// width of each segment
	float pointAngleFromCentre = 360 / (numberOfSegments);

	// If the cursor is outside of the wheel (+ the margin size)
	if (distanceSqrd >= (wheelRadius + wheelMargin)*(wheelRadius + wheelMargin))
	{
		float mousePosAsAngle = RAD2DEG(atan2(-(float)y, (float)x)) + 90.0f - pointAngleFromCentre;
		int selected = RoundFloatToInt( mousePosAsAngle / (360 / numberOfSegments) ) + 1;
		if (selected < 0)
			selected += 8; //+= numberOfSegments
		
		char s[10];
		Q_snprintf(s, sizeof(s), "Angle: %d.\n", mousePosAsAngle);

		if (hud_weaponwheel_quickswitch.GetInt() > 0)
		{
			WeaponSelected( selected );
		}
		else
		{
			if (slotSelected != selected)
			{
				slotSelected = selected;
				useHighlighted = true;
			}
		}
	}
	else
	{
		slotSelected = -1;
		useHighlighted = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets specified icon
//-----------------------------------------------------------------------------
/*CHudTexture *CHudWeaponWheel::GetIcon(const char *szIcon, bool bInvert)
{
	// get the inverted version if specified
	if (bInvert && 0 == V_strncmp("d_", szIcon, 2))
	{
		// change prefix from d_ to dneg_
		char szIconTmp[255] = "dneg_";
		V_strcat(szIconTmp, szIcon + 2, ARRAYSIZE(szIconTmp));
		CHudTexture *pIcon = gHUD.GetIcon(szIconTmp);
		// return inverted version if found
		if (pIcon)
			return pIcon;
		// if we didn't find the inverted version, keep going and try the normal version
	}
	return gHUD.GetIcon(szIcon);
}*/

void CHudWeaponWheel::RefreshEquippedWeapons(void)
{
	if (!segments)
		RefreshWheelVerts();

	if (GetHudWeaponSelection())
	{

		for (int slot = 0; slot < numberOfSegments; slot++)
		{
			bool isFirstBucket = true;
			// Iterate over the slot & buckets
			CBaseCombatWeapon *weaponInSlot;
			for (int bucketSlot = 0; bucketSlot < MAX_WEPS_PER_SLOT; bucketSlot++)
			{
				
				weaponInSlot = GetHudWeaponSelection()->GetWeaponInSlot(slot, bucketSlot);
				if (weaponInSlot)
				{
					segments[slot].imageIcon[bucketSlot] = weaponInSlot->GetSpriteActive();
					segments[slot].bHasIcon = true;
					if (isFirstBucket)
					{ 
						segments[slot].bucketSelected = bucketSlot;
						isFirstBucket = false;
					}
				}

			}
		}

	}

	/*Q_snprintf(segment.iconName, sizeof(segment.iconName), "d_%s", iconWeaponNames[i]);
	segment.imageIcon = GetIcon(segment.iconName, false);
	segment.bHasIcon = true;*/
}

void CHudWeaponWheel::RefreshWheelVerts(void)
{
	 bWheelLoaded = false;
	CTFPlayer* pPlayer = CTFPlayer::GetLocalTFPlayer();
	if (!pPlayer) {
		return;
	}


	if (segments) {
		delete[] segments;
	}
	segments = new WheelSegment[numberOfSegments];

	float pointAngleFromCentre = 360 / (2 * numberOfSegments);

	// From the top, clockwise, in deg
	float currentCentreAngle = 0;
	for (int i = 0; i < numberOfSegments; i++)
	{
		WheelSegment segment = WheelSegment();

		segment.centreAngle = currentCentreAngle;

		segment.centreX = iCentreScreenX + ( sin(DEG2RAD(currentCentreAngle) ) * (wheelRadius + wheelMargin));
		segment.centreY = iCentreScreenY + ( cos(DEG2RAD(currentCentreAngle) ) * (wheelRadius + wheelMargin));

		segment.vertices[0].Init(Vector2D(segment.centreX, segment.centreY));

		Vector2D centreToPoint = Vector2D(sin(DEG2RAD(currentCentreAngle)), cos(DEG2RAD(currentCentreAngle)));

		segment.vertices[1].Init(Vector2D(iCentreScreenX + (sin(DEG2RAD(currentCentreAngle - pointAngleFromCentre + (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin)), iCentreScreenY + (cos(DEG2RAD(currentCentreAngle - pointAngleFromCentre + (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin))));

		segment.vertices[2].Init(Vector2D(iCentreScreenX + (sin(DEG2RAD(currentCentreAngle - pointAngleFromCentre + (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin + outerRadius)), iCentreScreenY + (cos(DEG2RAD(currentCentreAngle - pointAngleFromCentre + (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin + outerRadius))));

		segment.vertices[3].Init(Vector2D(segment.vertices[2].m_Position.x + (centreToPoint.x * segmentExtensionLength), segment.vertices[2].m_Position.y + (centreToPoint.y * segmentExtensionLength)));

		segment.vertices[5].Init(Vector2D(iCentreScreenX + (sin(DEG2RAD(currentCentreAngle + pointAngleFromCentre - (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin + outerRadius)), iCentreScreenY + (cos(DEG2RAD(currentCentreAngle + pointAngleFromCentre - (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin + outerRadius))));

		segment.vertices[4].Init(Vector2D(segment.vertices[5].m_Position.x + (centreToPoint.x * segmentExtensionLength), segment.vertices[5].m_Position.y + (centreToPoint.y * segmentExtensionLength)));

		segment.vertices[6].Init(Vector2D(iCentreScreenX + (sin(DEG2RAD(currentCentreAngle + pointAngleFromCentre - (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin)), iCentreScreenY + (cos(DEG2RAD(currentCentreAngle + pointAngleFromCentre - (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin))));

		
		segment.angleSin = sin(DEG2RAD(currentCentreAngle));
		segment.angleCos = cos(DEG2RAD(currentCentreAngle));
	
		segments[i] = segment;

		currentCentreAngle += 360 / numberOfSegments;
	}

	 bWheelLoaded = true;
}

void CHudWeaponWheel::RefreshCentre(void)
{
	int width, height;
	GetSize(width, height);
	iCentreScreenX_const = width / 2;
	iCentreScreenY_const = height / 2;
	iCentreScreenX = iCentreScreenX_const;
	iCentreScreenY = iCentreScreenY_const;
}

void CHudWeaponWheel::DrawString(wchar_t *text, int xpos, int ypos, Color col, bool bCenter)
{
	surface()->DrawSetTextColor(col);
	surface()->DrawSetTextFont(m_hTextFont);

	// count the position
	int slen = 0, charCount = 0, maxslen = 0;
	{
		for (wchar_t *pch = text; *pch != 0; pch++)
		{
			if (*pch == '\n')
			{
				// newline character, drop to the next line
				if (slen > maxslen)
				{
					maxslen = slen;
				}
				slen = 0;
			}
			else if (*pch == '\r')
			{
				// do nothing
			}
			else
			{
				slen += surface()->GetCharacterWidth(m_hTextFont, *pch);
				charCount++;
			}
		}
	}
	if (slen > maxslen)
	{
		maxslen = slen;
	}

	int x = xpos;

	if (bCenter)
	{
		x = xpos - slen * 0.5;
	}

	surface()->DrawSetTextPos(x, ypos);

	// adjust the charCount by the scan amount
	//charCount *= m_flTextScan;
	for (wchar_t *pch = text; charCount > 0; pch++)
	{
		/*if (*pch == '\n')
		{
			// newline character, move to the next line
			surface()->DrawSetTextPos(x + ((m_flLargeBoxWide - slen) / 2), ypos + (surface()->GetFontTall(m_hTextFont) * 1.1f));
		}
		else if (*pch == '\r')
		{
			// do nothing
		}
		else*/
		//{
			surface()->DrawUnicodeChar(*pch);
			charCount--;
		//}
	}
}

void CHudWeaponWheel::Paint(void)
{
	if (bWheelActive &&  bWheelLoaded) {
		// Wheel centre
		surface()->DrawSetColor(Color(255, 255, 0, 255));
		surface()->DrawOutlinedCircle(iCentreScreenX, iCentreScreenY, wheelRadius, 12);

		surface()->DrawSetColor(Color(255, 255, 255, 100));

		// Spokes!
		for (int i = 0; i < numberOfSegments; i++) {
			WheelSegment segment = segments[i];
			surface()->DrawSetColor(weaponColors[i]);
			surface()->DrawSetTexture(-1);
			surface()->DrawTexturedPolygon(NUM_VERTS_SPOKE, segment.vertices);
		}

		// Now the outline, slot number, and ammo
		for (int i = 0; i < numberOfSegments; i++) {
			WheelSegment segment = segments[i];

			surface()->DrawSetColor(Color(255, 255, 255, 255));
			surface()->DrawSetTexture(-1);
			surface()->DrawTexturedPolyLine(segment.vertices, NUM_VERTS_SPOKE);

			// Slot number + shadow
			int offset = wheelRadius + textOffset;

			int xpos = iCentreScreenX + (segment.angleSin * offset);
			int ypos = iCentreScreenY + (segment.angleCos * offset);

			DrawString(slotNames[i], xpos + 1, ypos + 1, Color(0, 0, 0, 255), true);
			DrawString(slotNames[i], xpos, ypos, Color(255, 255, 255, 255), true);

			CBaseHudWeaponSelection* weaponSelect = GetHudWeaponSelection();

			// Display the currently select weapon's icon and ammo
			C_BaseCombatWeapon *pWeapon = weaponSelect->GetWeaponInSlot(i, segment.bucketSelected);

			if (pWeapon)
			{
				// Draw the icon
				if (segment.imageIcon[segment.bucketSelected] != NULL && segment.bHasIcon)
				{
					offset += 50;
					xpos = iCentreScreenX + (segment.angleSin * offset);
					ypos = iCentreScreenY + (segment.angleCos * offset);

					const float flScale = 0.5f;
					int iconActualWide = segment.imageIcon[segment.bucketSelected]->EffectiveWidth(1.0f);
					int iconTall = segment.imageIcon[segment.bucketSelected]->EffectiveHeight(1.0f);

					iconActualWide *= flScale;
					iconTall *= flScale;

					xpos -= iconActualWide / 2;
					ypos -= iconTall / 2;

					// TODO Move colour to:		CPanelAnimationVar( Color, m_clrIcon, "IconColor", "255 80 0 255" );
					segment.imageIcon[segment.bucketSelected]->DrawSelf(xpos, ypos, iconActualWide, iconTall, weaponColors[i]);

				}

				Color ammoColor = Color(255, 255, 255, 255);

				if (!pWeapon->CanBeSelected())
				{
					ammoColor = Color(255, 0, 0, 255);
				}

				ammoColor = Color(255, 255, 255, 255);
				wchar_t pText[64];

				if (pWeapon->Clip1() > -1)
					g_pVGuiLocalize->ConstructString(pText, sizeof(pText), VarArgs("%d/%d", pWeapon->Clip1(), pWeapon->ReserveAmmo()), 0);
				else if (pWeapon->Clip2() > -1)
					g_pVGuiLocalize->ConstructString(pText, sizeof(pText), VarArgs("%d/%d", pWeapon->Clip2(), pWeapon->ReserveAmmo()), 0);
				else if (pWeapon->ReserveAmmo() > -1)
					g_pVGuiLocalize->ConstructString(pText, sizeof(pText), VarArgs("%d", pWeapon->ReserveAmmo()), 0);

				DrawString(pText, xpos - 1, ypos - 1, Color(0,0,0,255), true);
				DrawString(pText, xpos, ypos, ammoColor, true);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reloads/Applies the RES scheme
//-----------------------------------------------------------------------------
void CHudWeaponWheel::ApplySchemeSettings(IScheme *pScheme)
{
	// load control settings...
	LoadControlSettings("resource/UI/HudWeaponwheel.res");
	
	RefreshCentre();

	RefreshWheelVerts();

	if (weaponColors) 
	{
		delete[] weaponColors;
	}

	weaponColors = new Color[8];
	weaponColors[0] = Color(200, 200, 200, 150);	//melee
	weaponColors[1] = Color(255, 255, 100, 150);	//pistols
	weaponColors[2] = Color(255, 100, 100, 150);	//shotguns
	weaponColors[3] = Color(25, 25, 100, 150);		//automatics
	weaponColors[4] = Color(25, 150, 25, 150);		//rifles
	weaponColors[5] = Color(255, 255, 255, 150);	//specials
	weaponColors[6] = Color(50, 25, 10, 150);		//explosives
	weaponColors[7] = Color(255, 50, 255, 150);		//supers

	/*iconWeaponNames[0] = "crowbar";
	iconWeaponNames[1] = "pistol_mercenary";
	iconWeaponNames[2] = "shotgun";
	iconWeaponNames[3] = "smg_mercenary";
	iconWeaponNames[4] = "railgun";
	iconWeaponNames[5] = "nailgun";
	iconWeaponNames[6] = "dynamite_bundle";
	iconWeaponNames[7] = "bfg";*/

	for (int i = 0; i < numberOfSegments; i++) {
		
		wchar_t slotText[64];
		char text[64];
		
		Q_snprintf(text, sizeof(text), "[%d]", i + 1);
		g_pVGuiLocalize->ConvertANSIToUnicode(text, slotText, sizeof(slotText));

		slotNames[i] = new wchar_t[sizeof(slotText)];

		Q_wcsncpy(slotNames[i], slotText, sizeof(slotText));
	}

	BaseClass::ApplySchemeSettings(pScheme);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudWeaponWheel::ShouldDraw(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Do I even exist?
	if (!pPlayer)
		return false;

	// Dead men inspect no elements
	if (!pPlayer->IsAlive())
		return false;


	// Now let the base class decide our fate... (Used for turning of during deathcams etc., trust Robin)
	if (bWheelActive)
	{
		return CHudElement::ShouldDraw();
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Every think/update tick that the GUI uses
//-----------------------------------------------------------------------------
void CHudWeaponWheel::OnTick(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BasePlayer *pPlayerBase = C_TFPlayer::GetLocalPlayer();

	if (!(pPlayer && pPlayerBase))
		return;

	// If weapon wheel active bool has changed, change mouse input capabilities etc
	if (lastWheel != bWheelActive) {
		CheckWheel();
	}

	if (bWheelActive) 
	{
		CheckMousePos();
	}

	// Scan for changes in the number of weapons we have
	int weaponsThisTick = 0;
	CBaseHudWeaponSelection *weaponSelect = GetHudWeaponSelection();
	if (weaponSelect)
	{
		for (int slot = 0; slot < numberOfSegments; slot++)
		{
			for (int bucket = 0; bucket < MAX_WEPS_PER_SLOT; bucket++) 
			{
				if (weaponSelect->GetWeaponInSlot(slot, bucket))
				{
					weaponsThisTick++;
				}
			}
		}

		// If there is a discrepancy, check the weapons and their icons again
		if (weaponsThisTick != iNumberOfWeaponsEquipped) {
			RefreshEquippedWeapons();
			iNumberOfWeaponsEquipped = weaponsThisTick;
		}

		// Update the current weapon's segment to reflect what we have equipped
		CTFWeaponBase *pSelectedWeapon = pPlayer->GetActiveTFWeapon();
		if (pSelectedWeapon)
		{
			// or is it meant to be +/-1
			segments[pSelectedWeapon->GetSlot()].bucketSelected = pSelectedWeapon->GetPosition();
		}
	}

	lastWheel = bWheelActive;
}

void CHudWeaponWheel::CheckWheel()
{
	if (bWheelActive) {
		if (! bWheelLoaded) {
			RefreshWheelVerts();
			RefreshEquippedWeapons();
		}

		Activate();
		SetMouseInputEnabled(true);
		RequestFocus();

		vgui::input()->SetCursorPos(iCentreScreenX_const, iCentreScreenY_const);
		
		// since Linux can't snap to centre :( we just start the weapon wheel wherever their mouse is
		// On Windows, this should still let us start at iCentreScreenXY
//		int x = 0;
//		int y = 0;
		vgui::input()->GetCursorPos(iCentreScreenX, iCentreScreenY);
		//Msg("Pos: %i, %i\n", iCentreScreenX, iCentreScreenY);
	}
	else
	{
		// Switch to the one the mouse is over
		if (useHighlighted/* && hud_fastswitch.GetInt() <= 0*/)
		{
			// Select it and close
			WeaponSelected( slotSelected );
			char s[16];
			Q_snprintf(s, 16, "slot: %d\n", slotSelected + 1);
			Msg(s);
			Msg("Selected weapon on close (useHighlighted was true!)\n");
		}
		SetMouseInputEnabled(false);
		SetVisible(false);
	}
}


void CHudWeaponWheel::OnMouseWheeled(int delta)
{
	if (bWheelActive)
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if (!pPlayer)
			return;

		if (slotSelected >= 0)
		{
			CBaseHudWeaponSelection* weaponSelect = CBaseHudWeaponSelection::GetInstance();

			// Reselect the slot, iterating through it
			weaponSelect->SelectWeaponSlot(slotSelected + 1);
			
			// Set the slot image/ammo to the newly selected weapon
			CTFWeaponBase *pNextWeapon = NULL;
			pNextWeapon = pPlayer->GetActiveTFWeapon();
			segments[slotSelected].bucketSelected = pNextWeapon->GetPosition();

			// Until the player highlights another slot, if they release the key then just use whatever was selected with the mousewheel ^
			useHighlighted = false;
		}
	}
}

CHudWeaponWheel::~CHudWeaponWheel()
{
	if (weaponColors)
	{
		delete[] weaponColors;
	}
}