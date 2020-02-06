#include "cbase.h"
#include "basecombatweapon.h"
#include "gamerules.h"
#include "items.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef OPENFORTRESS_DLL
ConVar sk_item_healthkit_tiny( "sk_item_healthkit_tiny","8" );
#include "entity_ammopack.h"
#include "entity_healthkit.h"
#include "tf_player.h"

#define TF_HEALTHKIT_PICKUP_SOUND	"HealthKitTiny.Touch"

bool ITEM_GiveTFAmmoHealth(CBasePlayer *pPlayer, float flCount, bool bSuppressSound = true)
{
	bool bSuccess = false;
	int iHealthRestored = 0;
	int iHealthToAdd = sk_item_healthkit_tiny.GetFloat();

	CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if (!pTFPlayer)
		return false;
	int iHealthBefore = pTFPlayer->GetHealth();
	iHealthToAdd = clamp( iHealthToAdd, 0, pTFPlayer->m_Shared.GetMaxBuffedHealthDM() - pTFPlayer->GetHealth() );
	iHealthRestored = pPlayer->TakeHealth( iHealthToAdd, DMG_IGNORE_MAXHEALTH );
	if( pPlayer->GetHealth() > pTFPlayer->m_Shared.GetDefaultHealth() )
	{
		if( iHealthBefore >= pTFPlayer->m_Shared.GetDefaultHealth() )
		{
			iHealthToAdd = sk_item_healthkit_tiny.GetFloat();
			if( pTFPlayer->m_Shared.m_flMegaOverheal + iHealthToAdd > pTFPlayer->m_Shared.GetMaxBuffedHealthDM() )
				iHealthToAdd = ( pTFPlayer->m_Shared.GetMaxBuffedHealthDM() - pTFPlayer->m_Shared.GetDefaultHealth() ) - pTFPlayer->m_Shared.m_flMegaOverheal;
			pTFPlayer->m_Shared.m_flMegaOverheal += iHealthToAdd;
			iHealthRestored = 1;
		}
		else
			pTFPlayer->m_Shared.m_flMegaOverheal += iHealthToAdd + iHealthBefore - pTFPlayer->m_Shared.GetDefaultHealth();
		pTFPlayer->m_Shared.m_flMegaOverheal = min( pTFPlayer->m_Shared.m_flMegaOverheal, pTFPlayer->m_Shared.GetMaxBuffedHealthDM() );
	}
	
	if (iHealthRestored)
		bSuccess = true;

	return bSuccess;
}
#endif

class CHealthKitTiny : public CTFPowerup
{
public:

	virtual const char *GetPowerupModel(void) { return "models/items/medkit_overheal.mdl"; }

	DECLARE_CLASS(CHealthKitTiny, CTFPowerup);

	string_t m_iszModel=MAKE_STRING( "" );
	string_t m_iszModelOLD=MAKE_STRING( "" );
	string_t m_iszPickupSound=MAKE_STRING( "HealthKitTiny.Touch" );
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
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );  // And the tf player,       Guess we could just use the tf player, no?
		if ( pTFPlayer && pTFPlayer->m_Shared.IsZombie() )
			return false;

		if (ITEM_GiveTFAmmoHealth(pPlayer, PackRatios[POWERUP_TINY]))
		{
			CSingleUserRecipientFilter filter(pPlayer);
			EmitSound(filter, entindex(), STRING(m_iszPickupSound));
			AddEffects( EF_NODRAW );
		}
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_healthkit_tiny, CHealthKitTiny);
PRECACHE_REGISTER(item_healthkit_tiny);

BEGIN_DATADESC(CHealthKitTiny)

// Inputs.
DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),

END_DATADESC()
