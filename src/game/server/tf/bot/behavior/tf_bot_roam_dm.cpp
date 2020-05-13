#include "cbase.h"
#include "../tf_bot.h"
#include "tf_bot_roam_dm.h"
#include "tf_bot_attack.h"
#include "tf_bot_get_weapon.h"
#include "tf_bot_taunt.h"
#include "sniper/tf_bot_sniper_attack.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_gamerules.h"

CTFBotDMRoam::CTFBotDMRoam()
{
}

CTFBotDMRoam::~CTFBotDMRoam()
{
}

const char *CTFBotDMRoam::GetName( void ) const
{
	return "DM Roam";
}

ActionResult<CTFBot> CTFBotDMRoam::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotDMRoam::Update( CTFBot *me, float dt )
{
	const float flChaseRange = 2000.0f;
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	if ( threat != nullptr )
	{
		if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
		{
			return Action<CTFBot>::SuspendFor( new CTFBotAttack, "Chasing down the losers" );
		}
		
		if ( me->IsRangeLessThan( threat->GetLastKnownPosition(), flChaseRange ) )
		{
			if ( CTFBotSniperAttack::IsPossible( me ))
				return Action<CTFBot>::SuspendFor( new CTFBotSniperAttack, "Sniping an enemy" );
		
			return Action<CTFBot>::SuspendFor( new CTFBotAttack, "Going after an enemy" );
		}
	}

	if ( m_waitDuration.IsElapsed() )
		m_waitDuration.Invalidate();

	if ( m_PathFollower.IsValid() || !m_waitDuration.IsElapsed() )
	{
		if ( m_PathFollower.IsValid() )
			m_PathFollower.Update( me );
	}
	else if ( !m_waitDuration.HasStarted() )
	{
		m_waitDuration.Start( RandomFloat( 3.0f, 7.0f ) ); // OFBOT TODO: these need to be tweaked around

		CTFBotPathCost func( me );
		const int maxIterations = 15;	// OFBOT TODO: same for this

		for ( int i = 0; i < maxIterations; i++ )
		{
			int rand = random->RandomInt( 0, TheNavMesh->GetNavAreaCount()-1 );
			CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[rand] );
			if ( area == nullptr )
				return Action<CTFBot>::Continue();

			if ( !me->GetLocomotionInterface()->IsAreaTraversable( area ) )
				continue;

			if ( !m_PathFollower.Compute( me, area->GetRandomPoint(), func ) )
				continue;

			break;
		}
	}

	return Action<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotDMRoam::OnStuck( CTFBot *me )
{
	m_PathFollower.Invalidate();

	return Action<CTFBot>::TryContinue();
}