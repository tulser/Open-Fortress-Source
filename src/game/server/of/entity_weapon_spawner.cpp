//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//

#include "cbase.h"
#include "tf_player.h"
#include "tf_weaponbase.h"
#include "tf_shareddefs.h"
#include "tf_weapon_builder.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "entity_weapon_spawner.h"
#include "tf_weapon_parse.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "game.h"

#include "tier0/memdbgon.h"

//#define TF_WEAPON_PICKUP_SOUND		"AmmoPack.Touch"

extern ConVar ofd_mutators;
extern ConVar ofd_multiweapons;
extern ConVar ofd_weaponspawners;
extern ConVar ofd_allow_allclass_pickups;

extern ConVar weaponstay;
 
//-----------------------------------------------------------------------------
// Purpose: Spawn function for the Weapon Spawner
//-----------------------------------------------------------------------------


BEGIN_DATADESC(CWeaponSpawner)

// Inputs.
DEFINE_KEYFIELD(szWeaponName, FIELD_STRING, "weaponname"),
DEFINE_KEYFIELD(szWeaponModel, FIELD_STRING, "model"),
DEFINE_KEYFIELD(szWeaponModelOLD, FIELD_STRING, "powerup_model"),
DEFINE_KEYFIELD(szPickupSound, FIELD_STRING, "pickup_sound"),
DEFINE_KEYFIELD(m_bDisableSpin, FIELD_BOOLEAN, "disable_spin"),
DEFINE_KEYFIELD(m_bDisableShowOutline, FIELD_BOOLEAN, "disable_glow"),
DEFINE_KEYFIELD(m_iIndex, FIELD_INTEGER, "Index"),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CWeaponSpawner, DT_WeaponSpawner )
	SendPropBool( SENDINFO( m_bDisableSpin ) ),
	SendPropBool( SENDINFO( m_bDisableShowOutline ) ),
	SendPropBool( SENDINFO( m_bRespawning ) ),
	SendPropBool( SENDINFO( bInitialDelay ) ),
	SendPropTime( SENDINFO( m_flRespawnTick ) ),
	SendPropTime( SENDINFO( fl_RespawnTime ) ),
	SendPropTime( SENDINFO( fl_RespawnDelay ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( dm_weapon_spawner, CWeaponSpawner );

CWeaponSpawner::CWeaponSpawner()
{
	m_flRespawnTick = 0.0f;
	szWeaponName = MAKE_STRING( "tf_weapon_shotgun_mercenary" );
	szWeaponModel = MAKE_STRING("");
	szWeaponModelOLD = MAKE_STRING("");
	m_iszWeaponModel[0] = 0;
	m_iszWeaponModelOLD[0] = 0;
	szPickupSound = MAKE_STRING( "Player.PickupWeapon" );
	ResetSequence( LookupSequence("spin") );
}

void CWeaponSpawner::Spawn( void )
{
	m_nRenderFX = kRenderFxNone;
	if (ofd_weaponspawners.GetInt() >= 1 &&
		ofd_mutators.GetInt() == 0 && 
		TFGameRules() && 
		!TFGameRules()->IsGGGamemode())
	{
		Q_strncpy( m_iszWeaponName, STRING(szWeaponName), sizeof( m_iszWeaponName ) );
		if ( szWeaponModel != MAKE_STRING("") )
		Q_strncpy( m_iszWeaponModel, STRING(szWeaponModel) , sizeof( m_iszWeaponModel ) );
		if ( szWeaponModelOLD != MAKE_STRING("") )
		Q_strncpy( m_iszWeaponModelOLD, STRING(szWeaponModelOLD) , sizeof( m_iszWeaponModelOLD ) );
		Q_strncpy( m_iszPickupSound, STRING(szPickupSound), sizeof( m_iszPickupSound ) );
		if ( filesystem )
		{
		
			char szMapName[128];
			Q_snprintf( szMapName, sizeof(szMapName), "maps/%s_mapdata.txt" , STRING(gpGlobals->mapname) );
			if ( filesystem->FileExists( szMapName, "MOD" ) )
			{
				KeyValues* pMapData = new KeyValues( "MapData" );
				pMapData->LoadFromFile( filesystem, szMapName );
				if ( pMapData )
				{
					KeyValues* pWeaponSpawners = pMapData->FindKey( "WeaponSpawners" );
					if ( pWeaponSpawners )
					{
						char pTemp[256];
						Q_snprintf( pTemp, sizeof(pTemp), "%d", m_iIndex );					
						KeyValues* pWeaponSpawner = pWeaponSpawners->FindKey( pTemp );
						if ( pWeaponSpawner )
						{
							Q_strncpy( m_iszWeaponName, pWeaponSpawner->GetString("Weapon", m_iszWeaponName), sizeof( m_iszWeaponName ) );
							Q_strncpy( m_iszWeaponModel, pWeaponSpawner->GetString("Model", m_iszWeaponModel) , sizeof( m_iszWeaponModel ) );
							Q_strncpy( m_iszPickupSound,  pWeaponSpawner->GetString("PickupSound", m_iszPickupSound), sizeof( m_iszPickupSound ) );						
						}
					}
				}
			}
		}
		Precache();
		SetWeaponModel();
		BaseClass::Spawn();
		ResetSequence( LookupSequence("spin") );
	}
}

void CWeaponSpawner::SetWeaponModel( void )
{
	
	if ( m_iszWeaponModel[0] == 0 )       // If we dont use a weapon model
	{
		DevMsg("Doesnt have a model \n");
		if( m_iszWeaponModelOLD[0] == 0 ) // If the backwards compatible model isnt set either
		{
			DevMsg("Doesnt have a old model either \n");
			CTFWeaponBase *pWeapon = (CTFWeaponBase * )CreateEntityByName( m_iszWeaponName );     //Firstly create the weapon
			if ( !pWeapon )
				return;
			WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pWeapon->GetClassname() );			  //Get the weapon info
			Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );											  //Is it valid?
			CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) ); // Cast to TF Weapon info
			if ( pWeapon && pWeaponInfo )                //If both the weapon and weapon info exist
				SetModel( pWeaponInfo->szWorldModel );	 //Get its model
			return;
		}
		else
			Q_strncpy( m_iszWeaponModel, m_iszWeaponModelOLD, sizeof( m_iszWeaponModel ) ); //If we do have the old model set, set the model value to the old model value
	}
	SetModel( m_iszWeaponModel );     //Set the model
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the ammopack
//-----------------------------------------------------------------------------
void CWeaponSpawner::Precache( void )
{
	PrecacheScriptSound( m_iszPickupSound );
	if ( m_iszWeaponModel )
	{
		if( m_iszWeaponModelOLD )
		{
			CTFWeaponBase *pWeapon = (CTFWeaponBase * )CreateEntityByName( m_iszWeaponName );  //Ditto for the Model Function, just precache instead
			if ( !pWeapon )
				return;
			WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pWeapon->GetClassname() );
			Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
			CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
			if ( pWeapon && pWeaponInfo )
				PrecacheModel( pWeaponInfo->szWorldModel );
			return;
		}
		else
			Q_strncpy( m_iszWeaponModel, m_iszWeaponModelOLD, sizeof( m_iszWeaponModel ) );
	}
	PrecacheModel( m_iszWeaponModel );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the ammopack
//-----------------------------------------------------------------------------
bool CWeaponSpawner::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( !pTFPlayer )
			return false;
	
		bSuccess = true;
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)pTFPlayer->GiveNamedItem( m_iszWeaponName );  //Get the specified weapon
		
		for ( int iWeapon = 0; iWeapon < TF_WEAPON_COUNT; ++iWeapon )
		{		
			CTFWeaponBase *pCarriedWeapon = (CTFWeaponBase *)pTFPlayer->GetWeapon( iWeapon );      // Get a weapon in the player's inventory
			if ( TFGameRules() && !TFGameRules()->UsesDMBuckets() && pCarriedWeapon && pWeapon && pTFPlayer->CanPickupWeapon( pCarriedWeapon, pWeapon ) )  //If we're using the 3 Weapon System and we can pickup the Weapon
			{
					pTFPlayer->DropWeapon( pCarriedWeapon, false, false, pCarriedWeapon->m_iClip1, pCarriedWeapon->m_iReserveAmmo );	//Drop the weapon
					if ( pCarriedWeapon )
					{
						pTFPlayer->Weapon_Detach( pCarriedWeapon ); //Remove the weapon that the new weapon replaces
						UTIL_Remove( pCarriedWeapon );
						pCarriedWeapon = NULL;				
					}
			}
			else if ( pCarriedWeapon == pWeapon || ( !ofd_allow_allclass_pickups.GetBool() && !pTFPlayer->GetPlayerClass()->IsClass( TF_CLASS_MERCENARY ) ) )  // If the weapons are the same or you're a class that cant pickup weapons, get ammo
			{
				bSuccess = false;
				if ( pTFPlayer->RestockAmmo(0.5f) ) // Restock your ammo by half ( same as medium ammo pack )
				{
					CSingleUserRecipientFilter filter( pTFPlayer );				// Filter the sound to the player that picked this up
					EmitSound( filter, entindex(), m_iszPickupSound );  // Play the pickup sound
					if ( weaponstay.GetBool() )								// If weaponstay is on, dont dissapear
						bSuccess = false;
					else														// Disapear
					{
						bSuccess = true;
						m_nRenderFX = kRenderFxDistort;							// and get the Distortion effect
					}
					if ( pWeapon )												// If the weapon we used to check is still here
					{
						pTFPlayer->Weapon_Detach( pWeapon );					// Remove it
						UTIL_Remove( pWeapon );
						pWeapon = NULL;
					}
					
				}
				if ( pTFPlayer->RestockCloak( 0.5f ) )							// Restock cloak
				{
					CSingleUserRecipientFilter filter( pTFPlayer );				// Ditto from above
					EmitSound( filter, entindex(), m_iszPickupSound );
					if ( weaponstay.GetBool() )
					{
						bSuccess = false;
					}
					else
					{
						bSuccess = true;
						m_nRenderFX = kRenderFxDistort;
					}
					if ( pWeapon )
					{
						pTFPlayer->Weapon_Detach( pWeapon );
						UTIL_Remove( pWeapon );
						pWeapon = NULL;
					}
					
				}				
				int iMaxMetal = pTFPlayer->GetPlayerClass()->GetData()->m_aAmmoMax[TF_AMMO_METAL];		//Metal uses the old system the weapons used to use because its supposed to be bound by player and not the weapon
				if ( pTFPlayer->GiveAmmo( ceil(iMaxMetal * PackRatios[GetPowerupSize()]), TF_AMMO_METAL, true ) )	
				{
					CSingleUserRecipientFilter filter( pTFPlayer );
					EmitSound( filter, entindex(), m_iszPickupSound );
					if ( weaponstay.GetBool() )
					{
						bSuccess = false;
					}
					else
					{
						bSuccess = true;
						m_nRenderFX = kRenderFxDistort;
					}
					if ( pWeapon )
					{
						pTFPlayer->Weapon_Detach( pWeapon );
						UTIL_Remove( pWeapon );
						pWeapon = NULL;
					}
					return bSuccess;
				}
				if ( pWeapon )
				{
					pTFPlayer->Weapon_Detach( pWeapon );
					UTIL_Remove( pWeapon );
					pWeapon = NULL;
				}
				return bSuccess;
			}
		}
		// did we give them anything?
		if ( bSuccess )
		{
			if ( TFGameRules() && !TFGameRules()->UsesDMBuckets() )
			{
				if ( !(pTFPlayer->m_nButtons & IN_USE) )
				{				
					if ( pWeapon )								// Remove the weapon if we're not pressing use
					{
						pTFPlayer->Weapon_Detach( pWeapon );
						UTIL_Remove( pWeapon );
						pWeapon = NULL;
					}					
					return false;
				}
			}
			CSingleUserRecipientFilter filter( pTFPlayer );					// Filter the sound to the player who picked this up
			EmitSound( filter, entindex(), m_iszPickupSound );		// Play the sound
			
			CTFWeaponBase *pGivenWeapon = (CTFWeaponBase *)pTFPlayer->GiveNamedItem( m_iszWeaponName );  // Create the specified weapon
			pGivenWeapon->GiveTo( pTFPlayer ); 																	 // Give it to the player
			if ( weaponstay.GetBool() )																		 // Leave the weapon spawner active if weaponstay is on
			{
				bSuccess = false;
			}
			else
			{
				m_nRenderFX = kRenderFxDistort;																	 // Otherwise add the distort effect
			}
		}
		if ( pWeapon )																							 // Remove the temp weapon
		{
			pTFPlayer->Weapon_Detach( pWeapon );
			UTIL_Remove( pWeapon );
			pWeapon = NULL;
		}
	}
	
	return bSuccess;
}

CBaseEntity* CWeaponSpawner::Respawn( void )
{
	CBaseEntity *ret = BaseClass::Respawn();
	m_nRenderFX = kRenderFxDistort;
	m_flRespawnTick = GetNextThink();
	ResetSequence( LookupSequence("spin") );
	return ret;
}
