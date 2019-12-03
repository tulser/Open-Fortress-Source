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

extern ConVar of_multiweapons;
extern ConVar of_weaponspawners;
extern ConVar of_allow_allclass_spawners;

extern ConVar weaponstay;
extern ConVar of_randomizer;
 
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
	szWeaponName = MAKE_STRING( "tf_weapon_shotgun" );
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
	if (of_weaponspawners.GetInt() >= 1 &&
		TFGameRules()->IsMutator( NO_MUTATOR ) && 
		TFGameRules() && 
		!TFGameRules()->IsGGGamemode())
	{
		Q_strncpy( m_iszWeaponName, STRING(szWeaponName), sizeof( m_iszWeaponName ) );
		if( !strcmp(m_iszWeaponName,"tf_weapon_shotgun_mercenary")
			|| !strcmp(m_iszWeaponName,"tf_weapon_shotgun_primary")
			|| !strcmp(m_iszWeaponName,"tf_weapon_shotgun_soldier")
			|| !strcmp(m_iszWeaponName,"tf_weapon_shotgun_pyro")
			|| !strcmp(m_iszWeaponName,"tf_weapon_shotgun_hwg") )
			Q_strncpy( m_iszWeaponName, "tf_weapon_shotgun", sizeof( m_iszWeaponName ) );
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
			WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( m_iszWeaponName );			  //Get the weapon info
			Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );											  //Is it valid?
			CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) ); // Cast to TF Weapon info
			if ( pWeaponInfo )                //If both the weapon and weapon info exist
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
			WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( m_iszWeaponName );
			Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
			CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
			if ( pWeaponInfo )
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

		if ( pTFPlayer->m_Shared.IsZombie() )
			return false;
	
		bSuccess = true;
		bool bTakeWeapon = true;
		
		WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( m_iszWeaponName );
		CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
		
		if (!pWeaponInfo)
			return false;
		
		int iWeaponID = AliasToWeaponID( m_iszWeaponName );
		int iSlot;

		if ( TFGameRules() && TFGameRules()->UsesDMBuckets() && !TFGameRules()->IsGGGamemode()  )
			iSlot = pWeaponInfo->iSlotDM;
		else if ( pWeaponInfo->m_iClassSlot[ pTFPlayer->GetPlayerClass()->GetClassIndex() ] != -1 )
			iSlot = pWeaponInfo->m_iClassSlot[ pTFPlayer->GetPlayerClass()->GetClassIndex() ];
		else
			iSlot = pWeaponInfo->iSlot;
		
		int iPos = pWeaponInfo->iPosition;
		if ( TFGameRules() && TFGameRules()->UsesDMBuckets() && !TFGameRules()->IsGGGamemode() )
			iPos = pWeaponInfo->iPositionDM;
	
		if( !pTFPlayer->m_hWeaponInSlot )
		{	
			return false;
		}
		
		// We have it already, dont take it Freeman, but get ammo
		if( pTFPlayer->m_hWeaponInSlot[iSlot][iPos] && pTFPlayer->m_hWeaponInSlot[iSlot][iPos]->GetWeaponID() == iWeaponID )
		{
			bTakeWeapon = false;
			DevMsg("Reserve ammo is:%d\n Default is %d\n",pTFPlayer->m_hWeaponInSlot[iSlot][iPos]->ReserveAmmo(), pWeaponInfo->iMaxReserveAmmo);
			if( pTFPlayer->m_hWeaponInSlot[iSlot][iPos]->ReserveAmmo() < pWeaponInfo->iMaxReserveAmmo )
				pTFPlayer->m_hWeaponInSlot[iSlot][iPos]->m_iReserveAmmo = pWeaponInfo->iMaxReserveAmmo;
			else
				return false;
		}
		if( pTFPlayer->m_hWeaponInSlot[iSlot][iPos] && !pTFPlayer->m_hWeaponInSlot[iSlot][iPos]->CanHolster() )
		{	
			return false;
		}		
		if( TFGameRules() && !TFGameRules()->UsesDMBuckets() && bTakeWeapon )
		{
			// If the slot already has something, and its the 3 wep system but we're not picking it up, show the swap hud and bail out
			if ( pTFPlayer->m_hWeaponInSlot[iSlot][iPos] && !(pTFPlayer->m_nButtons & IN_USE) )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "player_swap_weapons" );
				if ( event )
				{
					event->SetInt( "playerid", pTFPlayer->entindex() );
					event->SetInt( "current_wep", pTFPlayer->m_hWeaponInSlot[iSlot][iPos]->GetWeaponID() );
					event->SetInt( "swap_wep", iWeaponID );
					gameeventmanager->FireEvent( event );
				}
				return false;
			}
		}
		// did we give them anything?
		if ( bSuccess )
		{
			CSingleUserRecipientFilter filter( pTFPlayer );					// Filter the sound to the player who picked this up
			EmitSound( filter, entindex(), m_iszPickupSound );		// Play the sound
			if( bTakeWeapon )
			{
				CTFWeaponBase *pGivenWeapon = (CTFWeaponBase *)pTFPlayer->GiveNamedItem( m_iszWeaponName );  // Create the specified weapon
				if ( pGivenWeapon )
				{
					pGivenWeapon->GiveTo( pTFPlayer ); 																	 // Give it to the player
					if ( pTFPlayer->GetActiveWeapon() && pGivenWeapon->GetSlot() == pTFPlayer->GetActiveWeapon()->GetSlot() 
						&& pGivenWeapon->GetPosition() == pTFPlayer->GetActiveWeapon()->GetPosition() )
						pTFPlayer->Weapon_Switch( pGivenWeapon );
				}
			}
			if ( weaponstay.GetBool() )																		 // Leave the weapon spawner active if weaponstay is on
			{
				bSuccess = false;
			}
			else
			{
				m_nRenderFX = kRenderFxDistort;																	 // Otherwise add the distort effect
			}
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
