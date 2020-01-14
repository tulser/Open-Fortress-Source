#include "cbase.h"
#include "../tf_bot.h"
#include "tf_bot_puppet.h"
#include "tf_bot_behavior.h"
#include "tf_bot_taunt.h"

CTFBotPuppet::CTFBotPuppet()
{
}

CTFBotPuppet::~CTFBotPuppet()
{
}

const char *CTFBotPuppet::GetName() const
{
	return "Puppet";
}

ActionResult<CTFBot> CTFBotPuppet::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotPuppet::Update( CTFBot *me, float interval )
{
	if ( m_bHasGoal )
	{
		// if we are within 4 units of the position, stop
		if ( me->IsRangeLessThan( m_vGoal, 50.0f ) )
		{
			m_bHasGoal = false;
		}

		if ( m_recomputePathTimer.IsElapsed() )
		{
			m_recomputePathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

			CTFBotPathCost cost( me, m_eRouteType );
			m_PathFollower.Compute( me, m_vGoal, cost );
		}

		m_PathFollower.Update( me );
	}

	return Action<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotPuppet::OnCommandString( CTFBot *me, const char *cmd )
{
	if ( !Q_strncmp( cmd, "goto", 4 ) )
	{
		RouteType eRouteType = DEFAULT_ROUTE;

		if ( Q_stristr( cmd, "fastest" ) )
			eRouteType = FASTEST_ROUTE;
		else if ( Q_stristr( cmd, "safest" ) )
			eRouteType = SAFEST_ROUTE;
		else if ( Q_stristr( cmd, "retreat" ) )
			eRouteType = RETREAT_ROUTE;

		CBasePlayer *player = UTIL_GetListenServerHost();
		if ( player )
		{
			Vector forward;
			player->EyeVectors( &forward );

			trace_t result;
			unsigned int mask = MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE | CONTENTS_GRATE | CONTENTS_WINDOW;
			UTIL_TraceLine( player->EyePosition(), player->EyePosition() + 999999.9f * forward, mask, player, COLLISION_GROUP_NONE, &result );
			if ( result.DidHit() )
			{
				NDebugOverlay::Cross3D( result.endpos, 5, 0, 255, 0, true, 10.0f );
				m_vGoal = result.endpos;
				m_eRouteType = eRouteType;
				m_bHasGoal = true;
			}
		}
	}
	else if ( FStrEq( cmd, "taunt" ) )
	{
		return Action<CTFBot>::TrySuspendFor( new CTFBotTaunt, RESULT_TRY, "Received command to taunt" );
	}
	else if ( FStrEq( cmd, "cloak" ) )
	{
		if ( me->IsPlayerClass( TF_CLASS_SPY ) && !me->m_Shared.InCond( TF_COND_STEALTHED ) )
			me->PressAltFireButton();
	}
	else if ( FStrEq( cmd, "uncloak" ) )
	{
		if ( me->IsPlayerClass( TF_CLASS_SPY ) && !me->m_Shared.InCond( TF_COND_STEALTHED ) )
			me->PressAltFireButton();
	}
	else if ( FStrEq( cmd, "disguise" ) )
	{
		if ( me->IsPlayerClass( TF_CLASS_SPY ) && me->CanDisguise() )
			me->m_Shared.Disguise( GetEnemyTeam( me ), RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS ) );
	}

	return Action<CTFBot>::TryContinue();
}