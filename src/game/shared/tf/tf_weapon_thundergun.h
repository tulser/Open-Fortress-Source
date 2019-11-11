//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_THUNDERGUN_H
#define TF_WEAPON_THUNDERGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFThundergun C_TFThundergun
#endif



//=============================================================================
//
// TF Weapon THUNDERGUN
//
class CTFThundergun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFThundergun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFThundergun() {}
	~CTFThundergun() {}

	virtual void PrimaryAttack();

#ifdef GAME_DLL
	void			AirBlastCharacter( CBaseCombatCharacter *pCharacter, const Vector &vec_in );
	void			AirBlastProjectile( CBaseEntity *pEntity, const Vector &vec_in );
#endif

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_THUNDERGUN; }

private:

	CTFThundergun( const CTFThundergun & ) {}
};

#endif // TF_WEAPON_SMG_H
