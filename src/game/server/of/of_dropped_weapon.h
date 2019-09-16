//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef TF_DROPPED_WEAPON_H
#define TF_DROPPED_WEAPON_H
#ifdef _WIN32
#pragma once
#endif

#include "items.h"

class CTFDroppedWeapon : public CItem
{
public:
	DECLARE_CLASS( CTFDroppedWeapon, CItem );
	DECLARE_SERVERCLASS();

	CTFDroppedWeapon() {}

	virtual void Spawn();
	virtual void Precache();		

	void EXPORT FlyThink( void );
	void EXPORT PackTouch( CBaseEntity *pOther );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;

	int WeaponID;

	static CTFDroppedWeapon *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszModelName );

	float GetCreationTime( void ) { return m_flCreationTime; }
	void  SetInitialVelocity( Vector &vecVelocity );

public:
		CNetworkVar( int, m_iReserveAmmo );
		CNetworkVar( int, m_iClip )
private:
	float m_flCreationTime;

	bool m_bAllowOwnerPickup;
	CNetworkVector( m_vecInitialVelocity );

private:
	CTFDroppedWeapon( const CTFDroppedWeapon & );

	DECLARE_DATADESC();
};

#endif //TF_DROPPED_WEAPON_H