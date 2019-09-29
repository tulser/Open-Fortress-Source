//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//=========================================================
// Purpose: Backwards compatibility for TF2's base_boss entity (not accurate but gets the job done)
//=========================================================
#include "cbase.h"
#include "shareddefs.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "ai_hull.h"
#include "ai_baseactor.h"
#include "tier1/strtools.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

#include "base_boss.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CTFBaseBoss )	
	DEFINE_KEYFIELD( m_startDisabled, FIELD_BOOLEAN, "start_disabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_OUTPUT( m_outputOnKilled , "OnKilled" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( base_boss, CTFBaseBoss );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFBaseBoss::CTFBaseBoss()
{
	SetMaxHealth( 1000 );
	SetHealth( 1000 );
	m_startDisabled = false;
}


//=========================================================
// Spawn
//=========================================================
void CTFBaseBoss::Spawn()
{
	char *szModel = (char *)STRING( GetModelName() );

	if (!szModel || !*szModel)
	{
		Warning( "base_boss at %.0f %.0f %0.f missing modelname\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		UTIL_Remove( this );
		return;
	}

	Precache();

	SetModel( STRING( GetModelName() ) );

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_STEP );

	SetBloodColor( DONT_BLEED );

	m_flFieldOfView		= 0.0f; // indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_SCRIPT;
	
	SetHullType( HULL_HUMAN );
	SetHullSizeNormal();

	NPCInit();

	SetThink( NULL );
	SetNextThink( -1 );

	if ( m_startDisabled )
	{
		AddFlag( EF_NODRAW );
		SetSolid( SOLID_NONE );
	}
}

//=========================================================
// Classify - indicates this NPC's place in the 
// relationship table.
//=========================================================
Class_T	CTFBaseBoss::Classify ( void )
{
	return	CLASS_ZOMBIE;
}

//=========================================================
// Precache - precaches all resources this NPC needs
//=========================================================
void CTFBaseBoss::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );
}	

void CTFBaseBoss::InputEnable( inputdata_t &inputdata )
{
	RemoveFlag( EF_NODRAW );
	SetSolid( SOLID_VPHYSICS );
	m_startDisabled = false;
}

void CTFBaseBoss::InputDisable( inputdata_t &inputdata )
{
	AddFlag( EF_NODRAW );
	SetSolid( SOLID_NONE );
	m_startDisabled = true;
}

void CTFBaseBoss::Event_Killed( const CTakeDamageInfo &info )
{
	m_outputOnKilled.FireOutput( info.GetAttacker(), this );

	BaseClass::Event_Killed( info );
}