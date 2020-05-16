//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "func_filter_visualizer.h"
#include "func_no_build.h"
#include "tf_team.h"
#include "ndebugoverlay.h"
#include "tf_gamerules.h"
#include "entity_tfstart.h"
#include "filters.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: This will hit only things that are in newCollisionGroup, but NOT in collisionGroupAlreadyChecked
//			Always ignores other grenade projectiles.
//-----------------------------------------------------------------------------
class CTraceFilterPlayerFilter : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterPlayerFilter );

	CTraceFilterPlayerFilter( IHandleEntity *passentity, IHandleEntity *passentity2 )
		: m_pPassEnt(passentity), m_pPassEnt2(passentity2)
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;
		
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		CFuncFilterVisualizer *pVisualizer = dynamic_cast<CFuncFilterVisualizer*> (EntityFromEntityHandle( m_pPassEnt2 ));

		if( pEntity && pVisualizer )
		{
			return pVisualizer->PassesTriggerFilters( pEntity );
		}

		return false;
	}

protected:
	IHandleEntity *m_pPassEnt;
	IHandleEntity *m_pPassEnt2;
};

//===========================================================================================================

LINK_ENTITY_TO_CLASS( func_filtervisualizer, CFuncFilterVisualizer);

BEGIN_DATADESC( CFuncFilterVisualizer )
	DEFINE_KEYFIELD( m_iFilterName,	FIELD_STRING,	"filtername" ),
	DEFINE_FIELD( m_hFilter,	FIELD_EHANDLE ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CFuncFilterVisualizer, DT_FuncFilterVisualizer )
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncFilterVisualizer::Spawn( void )
{
	BaseClass::Spawn();

	SetActive( true );

	SetCollisionGroup( TFCOLLISION_GROUP_RESPAWNROOMS );
}

//------------------------------------------------------------------------------
// Activate
//------------------------------------------------------------------------------
void CFuncFilterVisualizer::Activate( void ) 
{ 
	// Get a handle to my filter entity if there is one
	if (m_iFilterName != NULL_STRING)
	{
		m_hFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iFilterName ));
	}

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this entity passes the filter criteria, false if not.
// Input  : pOther - The entity to be filtered.
//-----------------------------------------------------------------------------
bool CFuncFilterVisualizer::PassesTriggerFilters(CBaseEntity *pOther)
{
	CBaseFilter *pFilter = m_hFilter.Get();
	
	return (!pFilter) ? false : pFilter->PassesFilter( this, pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CFuncFilterVisualizer::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Only transmit this entity to clients that aren't in our team
//-----------------------------------------------------------------------------
int CFuncFilterVisualizer::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );

	return PassesTriggerFilters(pRecipientEntity) ? FL_EDICT_ALWAYS : FL_EDICT_DONTSEND;
}

void CFuncFilterVisualizer::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	Vector vel;
	AngularImpulse angVel;
	pPhysics->GetVelocity( &vel, &angVel );

	Vector start = GetAbsOrigin();

	// find all entities that my collision group wouldn't hit, but COLLISION_GROUP_NONE would and bounce off of them as a ray cast
	CTraceFilterPlayerFilter filter( this, this );
	trace_t tr;

	UTIL_TraceLine( start, start + vel * gpGlobals->frametime, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, &filter, &tr );

	if ( tr.DidHit() )
	{
		Touch( tr.m_pEnt );
		BaseClass::VPhysicsUpdate( pPhysics );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncFilterVisualizer::SetActive( bool bActive )
{
	if ( bActive )
	{
		// We're a trigger, but we want to be solid. Out ShouldCollide() will make
		// us non-solid to members of the team that spawns here.
		RemoveSolidFlags( FSOLID_TRIGGER );
		RemoveSolidFlags( FSOLID_NOT_SOLID );	
	}
	else
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		AddSolidFlags( FSOLID_TRIGGER );	
	}
}
