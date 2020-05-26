//=============================================================================//
//
// Purpose: Mega Health "powerup".
//
//=============================================================================//
#include "cbase.h"
#include "basecombatweapon.h"
#include "gamerules.h"
#include "items.h"
#include "engine/IEngineSound.h"
#include "entity_healthkit.h"
#include "tf_player.h"

class CHealthKitMega : public CHealthKit
{
public:

	DECLARE_CLASS(CHealthKitMega, CHealthKit);

	virtual const char *GetPowerupModel(void) { return "models/pickups/megahealth.mdl"; }
	powerupsize_t GetPowerupSize(void) { return POWERUP_MEGA; }
	string_t m_iszPickupSound = MAKE_STRING("HealthKitMega.Touch");

	bool MyTouch(CBasePlayer *pPlayer);
	void Precache(void);

	DECLARE_DATADESC();
};