#ifndef TF_BOT_PUPPET_H
#define TF_BOT_PUPPET_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotPuppet : public Action<CTFBot>
{
	DECLARE_CLASS( CTFBotPuppet, Action<CTFBot> )
public:
	CTFBotPuppet();
	virtual ~CTFBotPuppet();

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual EventDesiredResult<CTFBot> OnCommandString( CTFBot *me, const char *cmd ) override;

private:
	bool m_bHasGoal;
	Vector m_vGoal;
	RouteType m_eRouteType;

	PathFollower m_PathFollower;
	CountdownTimer m_recomputePathTimer;
};

#endif