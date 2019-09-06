//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_CHAINSAW_H
#define TF_WEAPON_CHAINSAW_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFChainsaw C_TFChainsaw
#endif

//=============================================================================
//
// Bonesaw class.
//
class CTFChainsaw : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFChainsaw, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFChainsaw() {}
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_CHAINSAW; }

private:

	CTFChainsaw( const CTFChainsaw & ) {}
};

#endif // TF_WEAPON_BONESAW_H
