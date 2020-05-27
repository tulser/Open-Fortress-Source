#include "cbase.h"
#include "basecombatweapon.h"
#include "gamerules.h"
#include "items.h"
#include "engine/IEngineSound.h"
#include "item_healthkit_mega.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef OF_DLL
ConVar sk_item_healthkit_mega("sk_item_healthkit_mega", "100");

#define TF_HEALTHKIT_PICKUP_SOUND	"HealthKitMega.Touch"

LINK_ENTITY_TO_CLASS(item_healthkit_mega, CHealthKitMega);
PRECACHE_REGISTER(item_healthkit_mega);

BEGIN_DATADESC(CHealthKitMega)

// Inputs.
DEFINE_KEYFIELD(m_iszModel, FIELD_STRING, "model"),
DEFINE_KEYFIELD(m_iszModelOLD, FIELD_STRING, "powerup_model"),
DEFINE_KEYFIELD(m_iszPickupSound, FIELD_STRING, "pickup_sound"),

END_DATADESC()

bool ITEM_GiveTFMegaAmmoHealth(CBasePlayer *pPlayer, float flCount, bool bSuppressSound = true)
{
	bool bSuccess = false;
	int iHealthRestored = 0;

	CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if (!pTFPlayer)
		return false;

	int iHealthToAdd = pTFPlayer->m_Shared.GetMaxBuffedHealthDM() - pTFPlayer->GetHealth();
	iHealthToAdd = clamp(iHealthToAdd, 0, pTFPlayer->m_Shared.GetMaxBuffedHealthDM() - pTFPlayer->GetHealth());
	iHealthRestored = pPlayer->TakeHealth(iHealthToAdd, DMG_IGNORE_MAXHEALTH);

	pTFPlayer->m_Shared.m_flMegaOverheal = pTFPlayer->m_Shared.GetMaxBuffedHealthDM() - pTFPlayer->m_Shared.GetDefaultHealth();

	if (iHealthRestored)
		bSuccess = true;

	return bSuccess;
}
#endif

void CHealthKitMega::Precache(void)
{
	if (m_iszModel == MAKE_STRING(""))
	{
		if (m_iszModelOLD != MAKE_STRING(""))
			PrecacheModel(STRING(m_iszModelOLD));
		else
			PrecacheModel(GetPowerupModel());
	}
	else
	{
		PrecacheModel(STRING(m_iszModel));
	}

	PrecacheScriptSound(TF_HEALTHKIT_PICKUP_SOUND);
}

bool CHealthKitMega::MyTouch(CBasePlayer *pPlayer)
{
	if (!ValidTouch(pPlayer))
		return false;

	if (ITEM_GiveTFMegaAmmoHealth(pPlayer, PackRatios[POWERUP_MEGA]))
	{
		CSingleUserRecipientFilter filter(pPlayer);
		EmitSound(filter, entindex(), STRING(m_iszPickupSound));
		AddEffects(EF_NODRAW);
	}
	return true;
}