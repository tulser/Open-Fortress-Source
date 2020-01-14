//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_shotgun.h"
#include "decals.h"
#include "tf_fx_shared.h"

// Client specific.
#if defined( CLIENT_DLL )
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

#define CREATE_SIMPLE_WEAPON_TABLE( WpnName, entityname )			\
																	\
	IMPLEMENT_NETWORKCLASS_ALIASED( WpnName, DT_##WpnName )	\
															\
	BEGIN_NETWORK_TABLE( C##WpnName, DT_##WpnName )			\
	END_NETWORK_TABLE()										\
															\
	BEGIN_PREDICTION_DATA( C##WpnName )						\
	END_PREDICTION_DATA()									\
															\
	LINK_ENTITY_TO_CLASS( entityname, C##WpnName );			\
	PRECACHE_WEAPON_REGISTER( entityname );
#define CREATE_SIMPLE_WEAPON_TABLE_OLD(WpnName, entityname)			    \
																	\
	IMPLEMENT_NETWORKCLASS_ALIASED( ##WpnName##, DT_##WpnName## )	\
																	\
	BEGIN_NETWORK_TABLE( C##WpnName##, DT_##WpnName## )				\
	END_NETWORK_TABLE()												\
																	\
	BEGIN_PREDICTION_DATA( C##WpnName## )							\
	END_PREDICTION_DATA()											\
																	\
	LINK_ENTITY_TO_CLASS( ##entityname##, C##WpnName## );			\
	PRECACHE_WEAPON_REGISTER( ##entityname## );
	

//=============================================================================
//
// Weapon Shotgun tables.
//

CREATE_SIMPLE_WEAPON_TABLE( TFShotgun, tf_weapon_shotgun )

CREATE_SIMPLE_WEAPON_TABLE( TFScatterGun, tf_weapon_scattergun )
CREATE_SIMPLE_WEAPON_TABLE( TFSuperShotgun, tf_weapon_supershotgun )
CREATE_SIMPLE_WEAPON_TABLE( TFCShotgunSB, tfc_weapon_shotgun_sb )
CREATE_SIMPLE_WEAPON_TABLE( TFCShotgunDB, tfc_weapon_shotgun_db )

//=============================================================================
//
// Weapon Shotgun functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFShotgun::CTFShotgun()
{
	m_bReloadsSingly = true;
}
CTFSuperShotgun::CTFSuperShotgun()
{
	m_bReloadsSingly = false;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFShotgun::PrimaryAttack()
{
	if ( !CanAttack() )
		return;

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFShotgun::UpdatePunchAngles( CTFPlayer *pPlayer )
{
	// Update the player's punch angle.
	QAngle angle = pPlayer->GetPunchAngle();
	float flPunchAngle = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flPunchAngle;
	angle.x -= SharedRandomInt( "ShotgunPunchAngle", ( flPunchAngle - 1 ), ( flPunchAngle + 1 ) );
	pPlayer->SetPunchAngle( angle );
}


//Act tables for Merc
acttable_t CTFShotgun::m_acttableShotgun[] =
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_PRIMARY,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_PRIMARY,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_PRIMARY,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_PRIMARY,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_PRIMARY,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_PRIMARY,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_PRIMARY,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_PRIMARY,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_PRIMARY,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_PRIMARY,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_PRIMARY,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_PRIMARY,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_PRIMARY,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_PRIMARY,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_PRIMARY,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_PRIMARY,		false },
	{ ACT_MP_RELOAD_STAND_LOOP,	ACT_MP_RELOAD_STAND_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_STAND_END,	ACT_MP_RELOAD_STAND_PRIMARY_END,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_PRIMARY,		false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP,false },
	{ ACT_MP_RELOAD_CROUCH_END,	ACT_MP_RELOAD_CROUCH_PRIMARY_END,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_PRIMARY,		false },
	{ ACT_MP_RELOAD_SWIM_LOOP,	ACT_MP_RELOAD_SWIM_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_SWIM_END,	ACT_MP_RELOAD_SWIM_PRIMARY_END,	false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_PRIMARY,	false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP,	ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_AIRWALK_END,ACT_MP_RELOAD_AIRWALK_PRIMARY_END,false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_PRIMARY, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_PRIMARY,	false },
};

acttable_t CTFSuperShotgun::m_acttableSuperShotgun[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_SUPERSHOTGUN, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_SUPERSHOTGUN, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_SUPERSHOTGUN, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_SUPERSHOTGUN, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_SUPERSHOTGUN, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_SUPERSHOTGUN, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_SUPERSHOTGUN, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_SUPERSHOTGUN, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_SUPERSHOTGUN, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_SUPERSHOTGUN, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_SUPERSHOTGUN, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_SUPERSHOTGUN, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_SUPERSHOTGUN, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_SUPERSHOTGUN, false },

	{ ACT_MP_RELOAD_STAND, ACT_MERC_RELOAD_STAND_SUPERSHOTGUN, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MERC_RELOAD_CROUCH_SUPERSHOTGUN, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MERC_RELOAD_SWIM_SUPERSHOTGUN, false },
};

//Act table remapping
acttable_t *CTFShotgun::ActivityList( int &iActivityCount )
{
	// spy shotgun spy shotgun spy shotgun
	if ( GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY )
	{
		iActivityCount = ARRAYSIZE(m_acttableShotgun);
		return m_acttableShotgun;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}

acttable_t *CTFSuperShotgun::ActivityList(int &iActivityCount)
{
	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableSuperShotgun);
		return m_acttableSuperShotgun;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}