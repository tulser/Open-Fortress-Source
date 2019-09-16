//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_FLAG_H
#define TF_WEAPON_FLAG_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CTFFlag C_TFFlag
#endif

//=============================================================================
//
// Bottle class.
//
class CTFFlag : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFFlag, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();

	CTFFlag();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_C4; }
	bool				Deploy( void );
	virtual void 		PrimaryAttack( void );
	virtual void 		ItemPostFrame( void );

	virtual bool		CanDrop( void ) { return true; }
public:
	CNetworkVar( bool,	m_bInPlantZone );
	CNetworkVar( bool,	m_bPlanting );
	CNetworkVar( float, m_flPlantStart );
private:

	CTFFlag( const CTFFlag & ) {}
};

#endif // TF_WEAPON_FLAG_H
