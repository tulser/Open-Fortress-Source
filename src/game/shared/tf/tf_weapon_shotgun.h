//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SHOTGUN_H
#define TF_WEAPON_SHOTGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#if defined( CLIENT_DLL )
#define CTFShotgun C_TFShotgun
#define CTFShotgun_Soldier C_TFShotgun_Soldier
#define CTFShotgun_HWG C_TFShotgun_HWG
#define CTFShotgun_Pyro C_TFShotgun_Pyro
#define CTFScatterGun C_TFScatterGun
#define CTFSuperShotgun C_TFSuperShotgun
#define CTFShotgun_Merc C_TFShotgun_Merc
#define CTFCShotgunSB C_TFCShotgunSB
#define CTFCShotgunDB C_TFCShotgunDB
#endif

// Reload Modes
enum
{
	TF_WEAPON_SHOTGUN_RELOAD_START = 0,
	TF_WEAPON_SHOTGUN_RELOAD_SHELL,
	TF_WEAPON_SHOTGUN_RELOAD_CONTINUE,
	TF_WEAPON_SHOTGUN_RELOAD_FINISH
};

//=============================================================================
//
// Shotgun class.
//
class CTFShotgun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFShotgun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFShotgun();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN; }
	virtual void	PrimaryAttack();

	virtual acttable_t *ActivityList( int &iActivityCount );
	static acttable_t m_acttableShotgun[];

protected:

	void		Fire( CTFPlayer *pPlayer );
	void		UpdatePunchAngles( CTFPlayer *pPlayer );

private:

	CTFShotgun( const CTFShotgun & ) {}
};

// Scout version. Different models, possibly different behaviour later on
class CTFScatterGun : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFScatterGun, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SCATTERGUN; }
};

class CTFSuperShotgun : public CTFShotgun
{
public:
	DECLARE_CLASS(CTFSuperShotgun, CTFShotgun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	
	CTFSuperShotgun();

	virtual int		GetWeaponID(void) const { return TF_WEAPON_SUPERSHOTGUN; }

	virtual acttable_t *ActivityList(int &iActivityCount);
	static acttable_t m_acttableSuperShotgun[];
};

class CTFCShotgunSB : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFCShotgunSB, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TFC_WEAPON_SHOTGUN_SB; }
};

class CTFCShotgunDB : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFCShotgunDB, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TFC_WEAPON_SHOTGUN_DB; }
};
#endif // TF_WEAPON_SHOTGUN_H
