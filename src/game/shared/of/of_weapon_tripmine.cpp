

#include "cbase.h"
#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CWeaponTripMine C_WeaponTripMine
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif

//-----------------------------------------------------------------------------
// CWeaponTripMine
//-----------------------------------------------------------------------------


class CWeaponTripMine : public CTFWeaponBaseGun
{
	DECLARE_CLASS( CWeaponTripMine, CTFWeaponBaseGun );
public:

	virtual int		GetWeaponID(void) const { return TF_WEAPON_TRIPMINE; }

	CWeaponTripMine();

	void		Precache( void );
	CBaseEntity*	FireProjectile( CTFPlayer *pPlayer  );
	
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponTripMine, DT_WeaponTripMine )

BEGIN_NETWORK_TABLE( CWeaponTripMine, DT_WeaponTripMine )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponTripMine )
END_PREDICTION_DATA()

BEGIN_DATADESC( CWeaponTripMine )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_weapon_tripmine, CWeaponTripMine );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponTripMine::CWeaponTripMine( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponTripMine::Precache( void )
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "tf_projectile_tripmine" );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Safe guard against eating ammo but not placing a tripmine
//-----------------------------------------------------------------------------
CBaseEntity *CWeaponTripMine::FireProjectile( CTFPlayer *pPlayer )
{
	Vector vecAiming	= pPlayer->GetAutoaimVector( 0 );
	Vector vecSrc		= pPlayer->Weapon_ShootPosition( );

	trace_t tr;

	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flRange, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( !(tr.fraction < 1.0 ) )
		return NULL;

	return BaseClass::FireProjectile( pPlayer );
}
