//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_tranq.h"
#include "tf_fx_shared.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Tranq tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFCTranq, DT_WeaponTranq )

BEGIN_NETWORK_TABLE( CTFCTranq, DT_WeaponTranq )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCTranq )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tfc_weapon_tranq, CTFCTranq );
PRECACHE_WEAPON_REGISTER( tfc_weapon_tranq );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFCTranq )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon Tranq functions.
//

void CTFCTranq::Precache()
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	PrecacheParticleSystem( "nailtrails_super_blue_crit" );
	PrecacheParticleSystem( "tranq_tracer_teamcolor_blue" );
	PrecacheParticleSystem( "nailtrails_super_red_crit" );
	PrecacheParticleSystem( "tranq_tracer_teamcolor_red" );
	PrecacheParticleSystem( "nailtrails_super_dm_crit" );
	PrecacheParticleSystem( "tranq_tracer_teamcolor_dm" );
#endif
}