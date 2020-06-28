//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the grapple hook weapon.
//			
//			Primary attack: fires a beam that hooks on a surface.
//			Secondary attack: switches between pull and rapple modes
//
//
//=============================================================================//

#ifndef WEAPON_GRAPPLE_H
#define WEAPON_GRAPPLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
//#include "basehlcombatweapon.h"
#include "Sprite.h"
#include "rope_shared.h"
#include "beam_shared.h"

#ifdef CLIENT_DLL
#define CWeaponGrapple C_WeaponGrapple
#else
#include "props.h"
#include "te_effect_dispatch.h"
#endif

#ifdef GAME_DLL
class CWeaponGrapple;

//-----------------------------------------------------------------------------
// Grapple Hook
//-----------------------------------------------------------------------------
class CGrappleHook : public CBaseCombatCharacter
{
    DECLARE_CLASS( CGrappleHook, CBaseCombatCharacter );
 
public:
    CGrappleHook() { };
    ~CGrappleHook();

#ifdef GAME_DLL
    Class_T Classify( void ) { return CLASS_NONE; }
#endif
 
public:
    void Spawn( void );
    void Precache( void );
    void HookTouch( CBaseEntity *pOther );
    bool CreateVPhysics( void );
    unsigned int PhysicsSolidMaskForEntity() const;
    static CGrappleHook *HookCreate( const Vector &vecOrigin, const QAngle &angAngles, CBaseEntity *pentOwner = NULL );
 
protected:

    DECLARE_DATADESC();
 
private:
  
    CHandle<CWeaponGrapple>     m_hOwner;
	CHandle<CTFPlayer>          m_hPlayer;
};
#endif

//-----------------------------------------------------------------------------
// CWeaponGrapple
//-----------------------------------------------------------------------------

class CWeaponGrapple : public CTFWeaponBaseGun
//class CWeaponGrapple : public CBaseHLCombatWeapon           
{                                                            
  DECLARE_CLASS( CWeaponGrapple, CTFWeaponBaseGun ); 
//    DECLARE_CLASS( CWeaponGrapple, CBaseHLCombatWeapon );    
public:
 
    CWeaponGrapple( void );
 
    virtual void    Precache( void );
    virtual void    PrimaryAttack( void );
    virtual void    SecondaryAttack( void );
    bool            CanHolster( void );
    virtual bool    Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
    void            Drop( const Vector &vecVelocity );
    virtual bool    Reload( void );
    virtual void    ItemPostFrame( void );
	
    void            NotifyHookDied( void );
	void			NotifyHookAttached(void);
 
    bool            HasAnyAmmo( void );

	void   			DrawBeam(const Vector &endPos, const float width = 2.f);
	void			DoImpactEffect( trace_t &tr, int nDamageType );
 
    DECLARE_NETWORKCLASS(); 
    DECLARE_PREDICTABLE();
 
private:

	void	RemoveHook(void);

#ifdef GAME_DLL
	CHandle<CBeam>		pBeam;
	CHandle<CSprite>	m_pLightGlow;
	CNetworkHandle(CBaseEntity, m_hHook);	//server hook
#else
	EHANDLE			m_hHook;				//client hook relay
#endif

	CWeaponGrapple(const CWeaponGrapple &);
	CNetworkVar( int, m_iAttached );
	CNetworkVar( int, m_nBulletType );
};

#endif // WEAPON_GRAPPLE_H