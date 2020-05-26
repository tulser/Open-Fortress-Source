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

#include "entity_ammopack.h"
#include "entity_healthkit.h"
#include "tf_player.h"

class CHealthKitMega : public CTFPowerup
{
public:

	virtual const char *GetPowerupModel(void) { return "models/pickups/megahealth.mdl"; }

	DECLARE_CLASS(CHealthKitMega, CTFPowerup);

	string_t m_iszModel = MAKE_STRING("");
	string_t m_iszModelOLD = MAKE_STRING("");
	string_t m_iszPickupSound = MAKE_STRING("HealthKitMega.Touch");
	DECLARE_DATADESC();
	void Spawn(void);
	void Precache(void);
	bool MyTouch(CBasePlayer *pPlayer);

};