//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_SMG_H
#define TF_WEAPON_SMG_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFSMG C_TFSMG
#define CTFSMG_Mercenary C_TFSMG_Mercenary
#define CTFTommyGun C_TFTommyGun
#define CTFAssaultRifle C_TFAssaultRifle
#endif



//=============================================================================
//
// TF Weapon Sub-machine gun.
//
class CTFSMG : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFSMG, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFSMG() {}
	~CTFSMG() {}

	
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SMG; }

private:

	CTFSMG( const CTFSMG & ) {}
};

// Mercenary specific version
class CTFSMG_Mercenary : public CTFSMG
{
public:
	DECLARE_CLASS( CTFSMG_Mercenary, CTFSMG );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SMG_MERCENARY; }
};

// TommyGun version
class CTFTommyGun : public CTFSMG
{
public:
	DECLARE_CLASS( CTFTommyGun, CTFSMG );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_TOMMYGUN; }
};



//=============================================================================
//
// Assault Rifle cvars
//
//=============================================================================
ConVar ofd_weapon_assaultrifle_burstshots( "ofd_weapon_assaultrifle_burstshots", "3", FCVAR_GAMEDLL );
ConVar ofd_weapon_assaultrifle_bursttime( "ofd_weapon_assaultrifle_bursttime", "0.1", FCVAR_GAMEDLL );
ConVar ofd_weapon_assaultrifle_time_between_bursts( "ofd_weapon_assaultrifle_time_between_bursts", "0.2", FCVAR_GAMEDLL );

// AR
class CTFAssaultRifle : public CTFSMG
{
public:
	DECLARE_CLASS(CTFAssaultRifle, CTFSMG);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	
	CTFAssaultRifle() 
	{
		m_iShotsDue = 0;
		m_flNextShotTime = 0.0f;
	}
	
	virtual int		GetWeaponID(void) const { return TF_WEAPON_ASSAULTRIFLE; }

	void			BurstFire( void );
	void			BeginBurstFire( void );
	void			Shoot( void );

	virtual bool	Reload( void );

	virtual void	ItemPostFrame( void );

	bool InBurst( )
	{
		return m_iShotsDue > 0;
	}
	
	virtual float	GetBurstTotalTime( void ) { return ofd_weapon_assaultrifle_bursttime.GetFloat() * ofd_weapon_assaultrifle_burstshots.GetInt(); }
	
protected:
	CNetworkVar(float, m_flNextShotTime);

	CNetworkVar( int, m_iShotsDue );
};


#endif // TF_WEAPON_SMG_H
