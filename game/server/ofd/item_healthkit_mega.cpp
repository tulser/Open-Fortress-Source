#include "cbase.h"
#include "basecombatweapon.h"
#include "gamerules.h"
#include "items.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef OPENFORTRESS_DLL
ConVar sk_item_healthkit_mega( "sk_item_healthkit_mega","100" );
#include "entity_ammopack.h"
#include "entity_healthkit.h"
#include "tf_player.h"

#define TF_HEALTHKIT_PICKUP_SOUND	"HealthKitMega.Touch"

bool ITEM_GiveTFMegaAmmoHealth(CBasePlayer *pPlayer, float flCount, bool bSuppressSound = true)
{
	bool bSuccess = false;
	int iHealthRestored = 0;
	int iHealthToAdd = sk_item_healthkit_mega.GetInt();

	CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if (!pTFPlayer)
		return false;

	iHealthToAdd = clamp( iHealthToAdd, 0, pTFPlayer->m_Shared.GetMaxBuffedHealth() - pTFPlayer->GetMaxHealth() );
	iHealthRestored = pPlayer->TakeHealth( iHealthToAdd, DMG_IGNORE_MAXHEALTH );
	
	if (iHealthRestored)
		bSuccess = true;

	return bSuccess;
}
#endif

class CHealthKitMega : public CTFPowerup
{
public:

	virtual const char *GetPowerupModel(void) { return "models/items/medkit_mega.mdl"; }

	DECLARE_CLASS(CHealthKitMega, CTFPowerup);

	void Spawn(void)
	{
		Precache();
		SetModel(GetPowerupModel());

		BaseClass::Spawn();
	}

	void Precache(void)
	{
		PrecacheModel(GetPowerupModel());
		PrecacheScriptSound(TF_HEALTHKIT_PICKUP_SOUND);
	}

	bool MyTouch(CBasePlayer *pPlayer)
	{
		if (ITEM_GiveTFMegaAmmoHealth(pPlayer, PackRatios[POWERUP_MEGA]))
		{
			CSingleUserRecipientFilter filter(pPlayer);
			EmitSound(filter, entindex(), TF_HEALTHKIT_PICKUP_SOUND);
			AddEffects( EF_NODRAW );
		}
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_healthkit_mega, CHealthKitMega);
PRECACHE_REGISTER(item_healthkit_mega);



