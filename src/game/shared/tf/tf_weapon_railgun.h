//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
/*
#ifndef TF_WEAPON_RAILGUN_H
#define TF_WEAPON_RAILGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_sniperrifle.h"
#include "Sprite.h"

#if defined( CLIENT_DLL )
#define CTFRailgun C_TFRailgun
#endif


//=============================================================================
//
// Sniper Rifle class.
//
class CTFRailgun: public CTFSniperRifle
{
public:

	DECLARE_CLASS( CTFRailgun, CTFSniperRifle );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFRailgun();

	virtual int	GetWeaponID( void ) const			{ return TF_WEAPON_RAILGUN; }


	void		 HandleZooms( void );
	virtual void ItemPostFrame( void );
	virtual bool Lower( void );
	virtual float GetProjectileDamage( void );
	virtual int	GetDamageType() const;

	virtual bool CanFireCriticalShot( bool bIsHeadshot = false );

#ifdef CLIENT_DLL
	float GetHUDDamagePerc( void );
#endif

private:

	void Zoom( void );


private:

	CTFRailgun( const CTFRailgun & );
};

#endif // TF_WEAPON_RAILGUN_H
