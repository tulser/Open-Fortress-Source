//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "tf_shareddefs.h"

#include "ienginevgui.h"
#include "engine/IEngineSound.h"

#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
	#define SHARED_ARGS VarArgs
#else
	#define SHARED_ARGS UTIL_VarArgs
#endif

KeyValues* gSoundManifest;
KeyValues* GlobalSoundManifest()
{
	return gSoundManifest;
}

void InitGlobalSoundManifest()
{
	if( gSoundManifest )
	{
		gSoundManifest->deleteThis();
	}
	gSoundManifest = new KeyValues( "GlobalSoundManifest" );
}

KeyValues* gLevelSoundManifest;
KeyValues* LevelSoundManifest()
{
	return gLevelSoundManifest;
}

void InitLevelSoundManifest()
{
	if( gLevelSoundManifest )
	{
		gLevelSoundManifest->deleteThis();
	}
	gLevelSoundManifest = new KeyValues( "LevelSoundManifest" );
}

void ParseSoundManifest( void )
{	
	InitGlobalSoundManifest();
	
	KeyValues *pManifestFile = new KeyValues( "game_sounds_manifest" );
	pManifestFile->LoadFromFile( filesystem, "scripts/game_sounds_manifest.txt" );
	
	if ( pManifestFile )
	{
		KeyValues *pManifest = new KeyValues( "Manifest" );
		pManifest = pManifestFile->GetFirstValue();
		for( pManifest; pManifest != NULL; pManifest = pManifest->GetNextValue() ) // Loop through all the keyvalues
		{
			KeyValues *pSoundFile = new KeyValues( "SoundFile" );
			pSoundFile->LoadFromFile( filesystem, pManifest->GetString() );
			if( pSoundFile )
			{
				KeyValues *pSound = new KeyValues( "SoundScript" );
				pSound = pSoundFile;
				GlobalSoundManifest()->AddSubKey( pSound );

				for( pSound; pSound != NULL; pSound = pSound->GetNextKey() ) // Loop through all the keyvalues
				{
					if( pSound )
					{
						DevMsg( "Parsed: %s\n", pSound->GetString( "wave" ) );
					}
				}
			}
		}	
	}
}

void CheckGlobalSounManifest( void )
{
	if ( GlobalSoundManifest() )
	{		
		KeyValues *pSound = new KeyValues( "SoundScript" );
		pSound = GlobalSoundManifest()->GetFirstSubKey();
		for( pSound; pSound != NULL; pSound = pSound->GetNextKey() ) // Loop through all the keyvalues
		{
			if( strcmp( pSound->GetString( "wave" ), "" ) )
				DevMsg( "Sound name: %s\n", pSound->GetString( "wave" ) );
		}
	}
}

void ParseLevelSoundManifest( void )
{	
	InitLevelSoundManifest();
	
	char mapsounds[MAX_PATH];
	char mapname[ 256 ];
	Q_StripExtension( 
#if CLIENT_DLL
	engine->GetLevelName()
#else
	STRING(gpGlobals->mapname)
#endif
	, mapname, sizeof( mapname ) );
	Q_snprintf( mapsounds, sizeof( mapsounds ), "%s_level_sounds.txt", mapname );
	if( !filesystem->FileExists( mapsounds , "MOD" ) )
	{
		DevMsg( "%s not present, not parsing", mapsounds );
		return;
	}
	DevMsg("%s\n", mapsounds);
	KeyValues *pSoundFile = new KeyValues( "level_sounds" );
	pSoundFile->LoadFromFile( filesystem, mapsounds );

	if( pSoundFile )
	{
		KeyValues *pSound = new KeyValues( "SoundScript" );
		pSound = pSoundFile;
		LevelSoundManifest()->AddSubKey( pSound );
		for( pSound; pSound != NULL; pSound = pSound->GetNextKey() ) // Loop through all the keyvalues
		{
			if( pSound )
			{
				DevMsg( "Parsed: %s\n", pSound->GetString( "wave" ) );
			}
		}
	}
}

KeyValues* GetSoundscript( const char *szSoundScript )
{
	KeyValues *pSound = new KeyValues( "SoundScript" );
			
	char mapsounds[MAX_PATH];
	char mapname[ 256 ];
	Q_StripExtension( 
#if CLIENT_DLL
	engine->GetLevelName()
#else
	STRING(gpGlobals->mapname)
#endif
	, mapname, sizeof( mapname ) );
	Q_snprintf( mapsounds, sizeof( mapsounds ), "%s_level_sounds.txt", mapname );
			
	if( filesystem->FileExists( mapsounds , "MOD" ) )
	{
		pSound = LevelSoundManifest()->FindKey( szSoundScript );
		if( !pSound )
		{
			DevMsg("Key not found in level sounds\n");
			return GlobalSoundManifest()->FindKey( szSoundScript );
		}
		else
			return pSound;
	}
	else if ( GlobalSoundManifest() )	
	{
		return GlobalSoundManifest()->FindKey( szSoundScript );
	}
	
	return NULL;
}
#ifdef CLIENT_DLL
void PrecacheUISoundScript( char *szSoundScript )
{
	KeyValues *pSoundScript = GetSoundscript( szSoundScript );
	if( !pSoundScript )
		return;
	
	KeyValues *pWave = pSoundScript->FindKey("rndwave");
	if( pWave )
	{
		bool bSuccess = false;
		for( KeyValues *pSub = pWave->GetFirstSubKey(); pSub != NULL ; pSub = pSub->GetNextKey() )
		{
			bSuccess = true;
			enginesound->PrecacheSound( pSub->GetString(), true, false );
		}

		if( !bSuccess )
		{
			Warning("Failed to precache UI sound %s", szSoundScript);
			return;
		}
	}
	else
	{
		pWave = pSoundScript->FindKey("wave");
		if( !pWave )
		{
			Warning("Failed to precache UI sound %s", szSoundScript);
			return;
		}
		
		enginesound->PrecacheSound( pWave->GetString(), true, false );
	}
}
#endif
char *GetSoundScriptWave( char *szSoundScript )
{
	if( !szSoundScript )
		return NULL;

	char *szReturn = new char[128];
	KeyValues *pSoundScript = GetSoundscript( szSoundScript );
	if( !pSoundScript )
		return NULL;
	
	KeyValues *pWave = pSoundScript->FindKey("rndwave");
	if( pWave )
	{
		KeyValues *pRand = new KeyValues("Rand");
		int iMaxSounds = 0;
		for( KeyValues *pSub = pWave->GetFirstSubKey(); pSub != NULL ; pSub = pSub->GetNextKey() )
		{
			pRand->SetString(SHARED_ARGS("%d",iMaxSounds),pSub->GetString());
			iMaxSounds++;
		}

		if( iMaxSounds )
		{
			Q_strncpy(szReturn, pRand->GetString(SHARED_ARGS("%d", random->RandomInt( 0, iMaxSounds - 1 ) ) ), 128 );
			return szReturn;
		}
		else
			Warning("Failed to Play UI sound %s\n", szSoundScript);
		
		// We dont need rand anymore, so clear it to not cause memory leaks
		pRand->deleteThis();
		return NULL;
	}
	else
	{
		pWave = pSoundScript->FindKey("wave");
		if( !pWave )
		{
			Warning("Failed to play UI sound %s\n", szSoundScript);
			return NULL;
		}
		Q_strncpy(szReturn, pWave->GetString(), 128 );
		return szReturn;
	}
}

KeyValues* gItemsGame;
KeyValues* GetItemsGame()
{
	return gItemsGame;
}

void InitItemsGame()
{
	if( gItemsGame )
	{
		gItemsGame->deleteThis();
	}
	gItemsGame = new KeyValues( "ItemsGame" );
}

void ParseItemsGame( void )
{	
	InitItemsGame();
	
	GetItemsGame()->LoadFromFile( filesystem, "scripts/items/items_game.txt" );
	
	KeyValues *pCosmetics = GetItemsGame()->FindKey("Cosmetics");
	if( pCosmetics )
	{
		int i = 0;
		FOR_EACH_SUBKEY( pCosmetics, kvSubKey )
		{
			i++;
		}
		GetItemsGame()->SetInt("cosmetic_count", i);
	}
}

KeyValues* GetCosmetic( int iID )
{
	if( !GetItemsGame() )
		return NULL;
	
	KeyValues *pCosmetics = GetItemsGame()->FindKey("Cosmetics");
	if( !pCosmetics )
		return NULL;
	
	KeyValues *pCosmetic = pCosmetics->FindKey( SHARED_ARGS( "%d", iID ) );
	if( !pCosmetic )
		return NULL;
	
	return pCosmetic;
}

KeyValues* GetRespawnParticle( int iID )
{
	if( !GetItemsGame() )
		return NULL;
	
	KeyValues *pParticles = GetItemsGame()->FindKey("RespawnParticles");
	if( !pParticles )
		return NULL;
	
	KeyValues *pParticle = pParticles->FindKey( SHARED_ARGS( "%d", iID ) );
	if( !pParticle )
		return NULL;
	
	return pParticle;
}

#ifdef CLIENT_DLL

KeyValues* gLoadout;
KeyValues* GetLoadout()
{
	return gLoadout;
}

void ResetLoadout( void )
{
	if( gLoadout )
		gLoadout->deleteThis();
	
	gLoadout = new KeyValues( "Loadout" );
	
	KeyValues *pCosmetics = new KeyValues( "Cosmetics" );
	gLoadout->AddSubKey( pCosmetics );
	for ( int i = 0; i < TF_CLASS_COUNT_ALL; i++ )
	{
		KeyValues *pClass = new KeyValues( g_aPlayerClassNames_NonLocalized[i] );
		pClass->SetString( "hat", "0" );
		if( i == TF_CLASS_MERCENARY )
		{
			pClass->SetString( "chest", "11" );
			pClass->SetString( "gloves", "15" );
		}
		pCosmetics->AddSubKey( pClass );
	}
	gLoadout->SaveToFile( filesystem, "cfg/loadout.cfg" );
}

void ParseLoadout( void )
{	
	if ( !filesystem->FileExists( "cfg/loadout.cfg" , "MOD" ) )
	{
		ResetLoadout();
	}
	else
	{
		gLoadout = new KeyValues( "Loadout" );
		GetLoadout()->LoadFromFile( filesystem, "cfg/loadout.cfg" );
	}
}
#endif