//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef OF_TFCGRENADEBASE_H
#define OF_TFCGRENADEBASE_H
#ifdef _WIN32
#pragma once
#endif


#include "basegrenade_shared.h"


class COFClassicGrenadeBase : public CBaseGrenade
{
	DECLARE_CLASS( COFClassicGrenadeBase, CBaseGrenade );
public:

	virtual void Precache();

	void Explode( trace_t *pTrace, int bitsDamageType );
	unsigned int	PhysicsSolidMaskForEntity( void ) const;
};

class CTFCFragGrenade : public COFClassicGrenadeBase
{
public:
	DECLARE_CLASS( CTFCFragGrenade, COFClassicGrenadeBase );
	DECLARE_DATADESC();

	void	Spawn( void );
	void	Precache( void );
	void	BounceSound( void );
	void	BounceTouch( CBaseEntity *pOther );

	void ShootTimed( CBaseCombatCharacter *pOwner, Vector vecVelocity, float flTime );
};

#endif // OF_TFCGRENADEBASE_H
