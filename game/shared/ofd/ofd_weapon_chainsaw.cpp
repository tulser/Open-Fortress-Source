//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "ofd_weapon_chainsaw.h"

//=============================================================================
//
// Weapon Bonesaw tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFChainsaw, DT_TFWeaponChainsaw )

BEGIN_NETWORK_TABLE( CTFChainsaw, DT_TFWeaponChainsaw )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFChainsaw )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_chainsaw, CTFChainsaw );
PRECACHE_WEAPON_REGISTER( tf_weapon_chainsaw );