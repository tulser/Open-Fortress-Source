//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the grapple hook weapon.
//			
//			Primary attack: fires a beam that hooks on a surface.
//			Secondary attack: switches between pull and rapple modes
//
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "tf_weapon_grapple.h"
 
#ifdef CLIENT_DLL
	#include "c_tf_player.h"      
    //#include "player.h"                
	#include "c_te_effect_dispatch.h"
    //#include "te_effect_dispatch.h"    
#else                                  
	#include "game.h"                  
    #include "tf_player.h"        
    #include "player.h"                
	#include "te_effect_dispatch.h"
	#include "IEffects.h"
	#include "SpriteTrail.h"
	#include "beam_shared.h"
	#include "explode.h"

	#include "ammodef.h"		/* This is needed for the tracing done later */
	#include "gamestats.h"
	#include "soundent.h"
 
	#include "vphysics/constraints.h"
	#include "physics_saverestore.h"
#endif

//#include "effect_dispatch_data.h"
 
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
 
#define HOOK_MODEL			"models/weapons/c_models/c_grapple_proj/c_grapple_proj.mdl"
#define BOLT_MODEL			"models/weapons/c_models/c_grapple_proj/c_grapple_proj.mdl"

#define BOLT_AIR_VELOCITY	3500
#define BOLT_WATER_VELOCITY	1500
 
#ifdef GAME_DLL

LINK_ENTITY_TO_CLASS( grapple_hook, CGrappleHook );
 
BEGIN_DATADESC( CGrappleHook )
	// Function Pointers
	DEFINE_THINKFUNC( FlyThink ),
	DEFINE_FUNCTION( HookTouch ),
	DEFINE_FIELD( m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hOwner, FIELD_EHANDLE ),
END_DATADESC()
 
CGrappleHook *CGrappleHook::HookCreate( const Vector &vecOrigin, const QAngle &angAngles, CBaseEntity *pentOwner )
{
	// Create a new entity with CGrappleHook private data
	CGrappleHook *pHook = (CGrappleHook *)CreateEntityByName( "grapple_hook" );
	UTIL_SetOrigin( pHook, vecOrigin );
	pHook->SetAbsAngles( angAngles );
	pHook->Spawn();
 
	CWeaponGrapple *pOwner = (CWeaponGrapple *)pentOwner;
	pHook->m_hOwner = pOwner;
	pHook->SetOwnerEntity( pOwner->GetOwner() );
	pHook->m_hPlayer = (CTFPlayer *)pOwner->GetOwner();
 
	return pHook;
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGrappleHook::~CGrappleHook( void )
{
	// Revert Jay's gai flag
	if ( m_hPlayer )
		m_hPlayer->SetPhysicsFlag( PFLAG_VPHYSICS_MOTIONCONTROLLER, false );
}
 
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGrappleHook::CreateVPhysics( void )
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_STANDABLE, false );
 
	return true;
}
 
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int CGrappleHook::PhysicsSolidMaskForEntity() const
{
	return (BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX) & ~CONTENTS_GRATE;
}
 
//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CGrappleHook::Spawn( void )
{
	Precache();
 
	SetModel( HOOK_MODEL );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	UTIL_SetSize( this, -Vector(1,1,1), Vector(1,1,1) );
	SetSolid( SOLID_BBOX );
	SetGravity( 0.05f );
 
	// The rock is invisible, the crossbow bolt is the visual representation
	AddEffects( EF_NODRAW );
 
	// Make sure we're updated if we're underwater
	UpdateWaterState();

	// Create bolt model and parent it
	CBaseEntity *pBolt = CBaseEntity::CreateNoSpawn("prop_dynamic", GetAbsOrigin(), GetAbsAngles(), this);
	pBolt->SetModelName(MAKE_STRING(BOLT_MODEL));
	pBolt->SetModel(BOLT_MODEL);
	DispatchSpawn(pBolt);
	pBolt->SetParent(this);

	SetTouch(&CGrappleHook::HookTouch);
	SetThink(&CGrappleHook::FlyThink);
	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CGrappleHook::Precache( void )
{
	PrecacheModel( HOOK_MODEL );
	PrecacheModel( BOLT_MODEL );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CGrappleHook::HookTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
	{
		m_hOwner->NotifyHookDied();
		return;
	}
 
	if ( pOther != m_hOwner && pOther->m_takedamage != DAMAGE_NO )
	{
		m_hOwner->NotifyHookDied();
 
		SetTouch( NULL );
		SetThink( NULL );

		UTIL_Remove( this );
	}
	else
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();
 
		// See if we struck the world
		if ( pOther->GetMoveType() == MOVETYPE_NONE && !( tr.surface.flags & SURF_SKY ) )
		{
			EmitSound( "Weapon_AR2.Reload_Push" );
 
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = GetAbsVelocity();
 
			//FIXME: We actually want to stick (with hierarchy) to what we've hit
			SetMoveType( MOVETYPE_NONE );
 
			Vector vForward;
 
			AngleVectors( GetAbsAngles(), &vForward );
			VectorNormalize ( vForward );
 
			CEffectData	data;
 
			data.m_vOrigin = tr.endpos;
			data.m_vNormal = vForward;
			data.m_nEntIndex = 0;
 
		//	DispatchEffect( "Impact", data );
 
		//	AddEffects( EF_NODRAW );
			SetTouch( NULL );

			VPhysicsDestroyObject();
			VPhysicsInitNormal( SOLID_VPHYSICS, FSOLID_NOT_STANDABLE, false );
			AddSolidFlags( FSOLID_NOT_SOLID );
		//	SetMoveType( MOVETYPE_NONE );
 
			if ( !m_hPlayer )
			{
				Assert( 0 );
				return;
			}
 
			// Set Jay's gai flag
			m_hPlayer->SetPhysicsFlag( PFLAG_VPHYSICS_MOTIONCONTROLLER, true );
			m_hOwner->NotifyHookAttached();
			m_hPlayer->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM, ACT_GRAPPLE_PULL_START );

			//IPhysicsObject *pPhysObject = m_hPlayer->VPhysicsGetObject();

			/*
			IPhysicsObject *pRootPhysObject = VPhysicsGetObject();
			Assert( pRootPhysObject );
 
			pRootPhysObject->EnableMotion( false );
 
			// Root has huge mass, tip has little
			pRootPhysObject->SetMass( VPHYSICS_MAX_MASS );
		//	pPhysObject->SetMass( 100 );
		//	float damping = 3;
		//	pPhysObject->SetDamping( &damping, &damping );
			*/
		}
		else
		{
			// Put a mark unless we've hit the sky
			/*if ( ( tr.surface.flags & SURF_SKY ) == false )
			{
				UTIL_ImpactTrace( &tr, DMG_BULLET );
			}*/
 
			SetTouch( NULL );
			SetThink( NULL );
 
			m_hOwner->NotifyHookDied();

			UTIL_Remove( this );
		}
	}
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrappleHook::FlyThink( void )
{
	QAngle angNewAngles;
 
	VectorAngles( GetAbsVelocity(), angNewAngles );
	SetAbsAngles( angNewAngles );
 
	SetNextThink( gpGlobals->curtime + 0.1f );
}
#endif

//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************

#ifdef CLIENT_DLL

#undef CWeaponGrapple

IMPLEMENT_CLIENTCLASS_DT(C_WeaponGrapple, DT_WeaponGrapple, CWeaponGrapple)
	RecvPropBool(RECVINFO(m_bMustReload)),
	RecvPropBool(RECVINFO(m_bAttached)),
	RecvPropInt(RECVINFO(m_nBulletType)),
	RecvPropEHandle(RECVINFO(m_hHook)),
END_NETWORK_TABLE()

#define CWeaponGrapple C_WeaponGrapple

#else
IMPLEMENT_SERVERCLASS_ST(CWeaponGrapple, DT_WeaponGrapple)
	SendPropBool( SENDINFO( m_bMustReload ) ),
	SendPropBool( SENDINFO( m_bAttached ) ),
	SendPropInt ( SENDINFO (m_nBulletType)),
	SendPropEHandle( SENDINFO( m_hHook ) ),
END_NETWORK_TABLE()
#endif

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponGrapple )
	DEFINE_PRED_FIELD( m_bMustReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif
 
LINK_ENTITY_TO_CLASS( tf_weapon_grapple, CWeaponGrapple );
 
//PRECACHE_WEAPON_REGISTER( tf_weapon_grapple );
 
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponGrapple::CWeaponGrapple( void )
{
	m_bReloadsSingly	= true;
	m_bFiresUnderwater	= true;
	m_bMustReload		= false;
	m_bAttached			= false;
	m_nBulletType = -1;
	
#ifdef GAME_DLL
	m_hHook = NULL;
	m_pLightGlow = NULL;
	pBeam = NULL;
#endif
}
  
//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CWeaponGrapple::Precache( void )
{
#ifdef GAME_DLL
	UTIL_PrecacheOther( "grapple_hook" );
#endif
 
	PrecacheModel( "cable/cable_red.vmt" );
 	PrecacheModel( "cable/cable_blue.vmt" );
	PrecacheModel( "cable/cable_purple.vmt" );

	BaseClass::Precache();
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGrapple::PrimaryAttack( void )
{
	// Can't have an active hook out
	if ( m_hHook )
		return;

	CTFPlayer *pPlayer = ToTFPlayer(GetOwner());

	if ( !pPlayer )
		return;
 
#ifdef GAME_DLL
	//m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );

	//Obligatory for MP so the sound can be played
	CDisablePredictionFiltering disabler;
	WeaponSound( SINGLE );

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_GRAPPLE_FIRE_START );

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.75;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;

	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner() );

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}

	trace_t tr;
	Vector vecShootOrigin, vecShootDir, vecDir, vecEnd;

	//Gets the direction where the player is aiming
	AngleVectors (pPlayer->EyeAngles(), &vecDir);

	//Gets the position of the player
	vecShootOrigin = pPlayer->Weapon_ShootPosition();

	//Gets the position where the hook will hit
	vecEnd = vecShootOrigin + (vecDir * MAX_TRACE_LENGTH);

	//Traces a line between the two vectors
	UTIL_TraceLine(vecShootOrigin, vecEnd, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);

	//Draws the beam
	DrawBeam(vecShootOrigin, tr.endpos, 2);
#endif

	FireHook();
 
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration( ACT_VM_PRIMARYATTACK ) );
}

//-----------------------------------------------------------------------------
// Purpose: Fires the hook
//-----------------------------------------------------------------------------
void CWeaponGrapple::FireHook(void)
{
	if (m_bMustReload)
		return;

	CTFPlayer *pOwner = ToTFPlayer(GetOwner());

	if (!pOwner)
		return;

#ifdef GAME_DLL
	Vector vecAiming = pOwner->GetAutoaimVector(0);
	Vector vecSrc = pOwner->Weapon_ShootPosition();

	QAngle angAiming;
	VectorAngles(vecAiming, angAiming);

	CGrappleHook *pHook = CGrappleHook::HookCreate(vecSrc, angAiming, this);

	//hook flies faster the faster player moves
	float vel = pOwner->GetWaterLevel() == 3 ? BOLT_WATER_VELOCITY : BOLT_AIR_VELOCITY;
	pHook->SetAbsVelocity(vecAiming * vel);

	m_hHook = pHook;
#endif

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGrapple::SecondaryAttack(void)
{
	CTFPlayer *pPlayer = ToTFPlayer(GetPlayerOwner());

	//signal player it should swing
	if (pPlayer && pPlayer->GetWaterLevel() > WL_Feet && m_bAttached)
		pPlayer->m_Shared.SetPendulum(true);

	 m_flNextSecondaryAttack = gpGlobals->curtime + 1.f;
}
 
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGrapple::Reload( void )
{
	//Redraw the weapon
	SendWeaponAnim( ACT_VM_IDLE ); //ACT_VM_RELOAD
 
	//Update our times
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
 
	//Mark this as done
	m_bMustReload = false;

	return true;
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::ItemPostFrame(void)
{
	//Enforces being able to use PrimaryAttack and Secondary Attack
	CTFPlayer *pOwner = ToTFPlayer(GetOwner());

	if (pOwner->m_nButtons & IN_ATTACK)
	{
		if (m_flNextPrimaryAttack < gpGlobals->curtime)
			PrimaryAttack();
	}
	else
	{
		if (m_bMustReload) //&& HasWeaponIdleTimeElapsed() )
			Reload();

		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}

	if (pOwner->m_afButtonPressed & IN_ATTACK2)
	{
		if (m_flNextSecondaryAttack < gpGlobals->curtime)
			SecondaryAttack();
	}
	
	CBaseEntity *Hook = NULL;
#ifdef GAME_DLL
	Hook = m_hHook;
#else
	Hook = m_hHook.Get();
#endif

	if ( Hook )
	{
		//remove hook if player lets go of the attack button
		if (!(pOwner->m_nButtons & IN_ATTACK))
		{
			RemoveHook();
		}
		else if(m_bAttached && !pOwner->m_Shared.GetHook())
		{
			//the hook is attached to a surface, notify player
			pOwner->m_Shared.SetHook(Hook);
		}
	}

//	BaseClass::ItemPostFrame();
}

void CWeaponGrapple::RemoveHook(void)
{
#ifdef GAME_DLL
	m_hHook->SetTouch(NULL);
	m_hHook->SetThink(NULL);

	UTIL_Remove(m_hHook);
#endif

	NotifyHookDied();

	CTFPlayer *pPlayer = ToTFPlayer(GetPlayerOwner());

	if (pPlayer)
	{
		pPlayer->m_Shared.SetHook(NULL);
		pPlayer->m_Shared.SetPendulum(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGrapple::Deploy( void )
{
	if ( m_bMustReload )
		return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_CROSSBOW_DRAW_UNLOADED, (char*)GetAnimPrefix() );
 
	return BaseClass::Deploy();
}
 
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSwitchingTo - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGrapple::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_hHook )
		RemoveHook();
 
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::Drop( const Vector &vecVelocity )
{
	if (m_hHook)
		RemoveHook();
 
	BaseClass::Drop( vecVelocity );
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponGrapple::HasAnyAmmo( void )
{
	if (m_hHook)
		RemoveHook();
 
	return BaseClass::HasAnyAmmo();
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponGrapple::CanHolster( void )
{
	//Can't have an active hook out
	if (m_hHook)
		RemoveHook();
 
	return BaseClass::CanHolster();
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::NotifyHookDied( void )
{
#ifdef GAME_DLL
	if ( pBeam )
	{
		UTIL_Remove( pBeam ); //Kill beam
		pBeam = NULL;

		UTIL_Remove( m_pLightGlow ); //Kill sprite
		m_pLightGlow = NULL;
	}
#endif

	//force a reload after the hook is removed
	m_hHook = NULL;
	m_bMustReload = true;
	m_bAttached = false;
	Reload();
}

void CWeaponGrapple::NotifyHookAttached(void)
{
	m_bAttached = true;
}

//-----------------------------------------------------------------------------
// Purpose: Draws a beam
// Input  : &startPos - where the beam should begin
//          &endPos - where the beam should end
//          width - what the diameter of the beam should be (units?)
//-----------------------------------------------------------------------------
void CWeaponGrapple::DrawBeam( const Vector &startPos, const Vector &endPos, float width )
{
#ifdef GAME_DLL
	//Tracer down the middle (NOT NEEDED, IT WILL FIRE A TRACER)
	//UTIL_Tracer( startPos, endPos, 0, TRACER_DONT_USE_ATTACHMENT, 6500, false, "GaussTracer" );

	trace_t tr;
	//Draw the main beam shaft
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer )
	{
		if ( pPlayer->GetTeamNumber() == TF_TEAM_RED  )
			pBeam = CBeam::BeamCreate("cable/cable_red.vmt", 2);
		else if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			pBeam = CBeam::BeamCreate("cable/cable_blue.vmt", 2);
		else
			pBeam = CBeam::BeamCreate("cable/cable_purple.vmt", 2);
	}
	else
	{
		pBeam = CBeam::BeamCreate("cable/cable_purple.vmt", 2);
	}

	// It starts at startPos
	pBeam->SetStartPos( startPos );

	// This sets up some things that the beam uses to figure out where
	// it should start and end
	pBeam->PointEntInit( endPos, this );

	// This makes it so that the beam appears to come from the muzzle of the pistol
	pBeam->SetEndAttachment( LookupAttachment("muzzle") );
	pBeam->SetWidth( width );
//	pBeam->SetEndWidth( 0.05f );

	// Higher brightness means less transparent
	pBeam->SetBrightness( 255 );
	//pBeam->SetColor( 255, 185+random->RandomInt( -16, 16 ), 40 );
	pBeam->RelinkBeam();

	//Sets scrollrate of the beam sprite 
	float scrollOffset = gpGlobals->curtime + 5.5;
	pBeam->SetScrollRate(scrollOffset);

	// The beam should only exist for a very short time
	//pBeam->LiveForTime( 0.1f );
	
	UpdateWaterState();

	SetTouch( &CGrappleHook::HookTouch );
	SetThink( &CGrappleHook::FlyThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - used to figure out where to do the effect
//          nDamageType - ???
//-----------------------------------------------------------------------------
void CWeaponGrapple::DoImpactEffect( trace_t &tr, int nDamageType )
{
#ifdef GAME_DLL
	if ( !(tr.surface.flags & SURF_SKY) )
	{
		CPVSFilter filter( tr.endpos );
		te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );
		m_nBulletType = GetAmmoDef()->Index("GaussEnergy");
		UTIL_ImpactTrace( &tr, m_nBulletType );
	}
#endif
}