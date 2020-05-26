//=============================================================================//
//
// Purpose: Quake-like health shards in the form of pills. Louis would be very happy.
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

class CHealthKitTiny : public CTFPowerup
{
public:

	virtual const char *GetPowerupModel(void) { return "models/items/medkit_overheal.mdl"; }

	DECLARE_CLASS(CHealthKitTiny, CTFPowerup);

	CHealthKitTiny();
	bool m_bDontHeal;
	string_t m_iszModel;
	string_t m_iszModelOLD;
	string_t m_iszPickupSound;
	DECLARE_DATADESC();
	void Spawn(void);
	void Precache(void);
	bool MyTouch(CBasePlayer *pPlayer);
	void Materialize(void);

};