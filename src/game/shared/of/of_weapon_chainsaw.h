//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef OFD_WEAPON_CHAINSAW_H
#define OFD_WEAPON_CHAINSAW_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFChainsaw C_TFChainsaw
#endif

enum ChainsawState_t
{
	// Firing states.
	CS_IDLE = 0,
	CS_STARTFIRING,
	CS_FIRING,
	CS_SPINNING,
	CS_DRYFIRE
};


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

	CTFChainsaw();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_CHAINSAW; }

	virtual bool		DoSwingTrace( trace_t &trace );
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void 		PrimaryAttack( void );
	virtual void 		SharedAttack( void );
	virtual void		WeaponIdle( void );
	virtual void		WindUp( void );
	virtual bool		SendWeaponAnim( int iActivity );
	virtual void		ItemPostFrame( void );
#ifdef CLIENT_DLL
	virtual void		OnDataChanged( DataUpdateType_t updateType );
	virtual void		WeaponSoundUpdate( void );
#endif
private:
	CTFChainsaw( const CTFChainsaw & ) {}
	CNetworkVar( ChainsawState_t, m_iWeaponState );
	CNetworkVar( int, m_bCritShot );
	CSoundPatch		*m_pSoundCur;				// the weapon sound currently being played
	int				m_iMinigunSoundCur;			// the enum value of the weapon sound currently being played
};

#endif // OFD_WEAPON_CHAINSAW_H
