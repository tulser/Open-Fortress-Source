//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Backwards compatibility for TF2's base_boss entity (not accurate but gets the job done)
//
//=============================================================================//

#ifndef CTFBaseBoss_H
#define CTFBaseBoss_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_baseactor.h"

//==================================================================================================================
// Base Boss
//==================================================================================================================
class CTFBaseBoss : public CAI_BaseActor
{
public:
	DECLARE_CLASS( CTFBaseBoss, CAI_BaseActor );
	DECLARE_DATADESC();

	CTFBaseBoss();

	void	Spawn( void );
	void	Precache( void );

	Class_T Classify ( void );

	bool		m_startDisabled;

	CTFBaseBoss *	GetEntity() { return this; }

	virtual void		Event_Killed( const CTakeDamageInfo &info );

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	COutputEvent m_outputOnKilled;
};

#endif // CTFBaseBoss_H
