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
	string_t m_iszModel=MAKE_STRING( "" );
	string_t m_iszModelOLD=MAKE_STRING( "" );
	string_t m_iszPickupSound=MAKE_STRING( "HealthKitMega.Touch" );
	DECLARE_DATADESC();
	void Spawn(void)
	{
		Precache();
		if ( m_iszModel==MAKE_STRING( "" ) )
		{
			if ( m_iszModelOLD!=MAKE_STRING( "" ) )
				SetModel( STRING(m_iszModelOLD) );
			else
				SetModel( GetPowerupModel() );
		}
		else SetModel( STRING(m_iszModel) );
		BaseClass::Spawn();
	}
	void Precache(void)
	{
		if ( m_iszModel==MAKE_STRING( "" ) )
		{
			if ( m_iszModelOLD!=MAKE_STRING( "" ) )
				PrecacheModel( STRING(m_iszModelOLD) );
			else
				PrecacheModel( GetPowerupModel() );
		}
		else PrecacheModel( STRING(m_iszModel) );	
		PrecacheScriptSound( STRING(m_iszPickupSound) );
	}
	
	bool MyTouch(CBasePlayer *pPlayer)
	{
		if (ITEM_GiveTFMegaAmmoHealth(pPlayer, PackRatios[POWERUP_MEGA]))
		{
			CSingleUserRecipientFilter filter(pPlayer);
			EmitSound(filter, entindex(), STRING(m_iszPickupSound));
			AddEffects( EF_NODRAW );
		}
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_healthkit_mega, CHealthKitMega);
PRECACHE_REGISTER(item_healthkit_mega);

BEGIN_DATADESC( CHealthKitMega )

// Inputs.
DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),

END_DATADESC()

