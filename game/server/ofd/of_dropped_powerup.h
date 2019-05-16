//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef TF_DROPPED_POWERUP_H
#define TF_DROPPED_POWERUP_H
#ifdef _WIN32
#pragma once
#endif

#include "items.h"
#include "tf_powerup.h"

class CTFDroppedPowerup : public CTFPowerup
{
public:
	DECLARE_CLASS( CTFDroppedPowerup, CTFPowerup );
	DECLARE_SERVERCLASS();

	CTFDroppedPowerup() {}

	virtual void Spawn();
	virtual void Precache();		

	void EXPORT FlyThink( void );
	void EXPORT PackTouch( CBaseEntity *pOther );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;

	bool OnGround=false;
	int PowerupID;
	CNetworkVar( float, PowerupDuration );
	CNetworkVar( float, OriginalPowerupDuration );

	static CTFDroppedPowerup *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszModelName, int PowerupID, float PowerupDuration, float OriginalPowerupDuration );

	float GetCreationTime( void ) { return m_flCreationTime; }
	void  SetInitialVelocity( Vector &vecVelocity );

private:
	float m_flCreationTime;

	bool m_bAllowOwnerPickup;
	CNetworkVector( m_vecInitialVelocity );
	CNetworkVar( float, m_flRespawnTick );

private:
	CTFDroppedPowerup( const CTFDroppedPowerup & );
	
	DECLARE_DATADESC();
};

#endif //TF_DROPPED_POWERUP_H