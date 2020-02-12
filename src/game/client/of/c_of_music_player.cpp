//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Music Player, used for Handling Music in Maps
//
//=============================================================================//

#include "cbase.h"
#include "c_of_music_player.h"
#include "c_tf_player.h"
#include "tf_gamerules.h"
#include "fmod_manager.h"

// Imported from sounds	
#include "stringregistry.h"
#include <ctype.h>
#include "igamesystem.h"
#include "KeyValues.h"
#include "filesystem.h"

#include "tier0/memdbgon.h"

extern System *pSystem;

// Inputs.
LINK_ENTITY_TO_CLASS( of_music_player, C_TFMusicPlayer );

IMPLEMENT_CLIENTCLASS_DT( C_TFMusicPlayer, DT_MusicPlayer, CTFMusicPlayer )
	RecvPropString( RECVINFO( szLoopingSong ) ),
	RecvPropBool( RECVINFO( m_bShouldBePlaying ) ),
	RecvPropBool( RECVINFO( m_bHardTransition ) ),
	RecvPropFloat( RECVINFO( m_flDelay ) ),
	RecvPropFloat( RECVINFO( m_flVolume ) ),
END_RECV_TABLE()

C_TFMusicPlayer::C_TFMusicPlayer()
{
	bIsPlaying = false;
	bInLoop = false;
	bParsed = false;
	m_flDelay = 0;
	m_iPhase = 0;
	m_Songdata.SetSize( 3 );
	if( pSystem )
		pSystem->createChannelGroup("Parent", &pChannel);
}

C_TFMusicPlayer::~C_TFMusicPlayer()
{
	bIsPlaying = false;
	bInLoop = false;
	bParsed = false;
	m_iPhase = 0;
	m_Songdata.Purge();
	FMODManager()->StopAmbientSound( pChannel, false );
}
void C_TFMusicPlayer::Spawn( void )
{	
	BaseClass::Spawn();
	ClientThink();
}

void C_TFMusicPlayer::ClientThink( void )
{

// Called every frame when the client is in-game

	if ( !bParsed )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		return;
	}
	
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		return;
	}
	if ( pLocalPlayer->m_Shared.InState( TF_STATE_WELCOME ) )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		return;
	}

	if ( bIsPlaying != m_bShouldBePlaying )
	{
		if ( !m_bShouldBePlaying )
		{
			bIsPlaying = false;
			m_iPhase = 0;
			FMODManager()->StopAmbientSound( pChannel, false );
			if( m_bHardTransition )
			{
				FMODManager()->PlayMusicEnd( pChannel, m_Songdata[2].path );
			}
		}
		else
		{
			FMODManager()->StopAmbientSound( pChannel, false );
			bIsPlaying = true;
			bInLoop = false;
			if ( m_Songdata[0].path[0] != 0 )
			{
				FMODManager()->PlayLoopingMusic( pChannel, m_Songdata[1].path, m_Songdata[0].path, m_flDelay );
			}
			else
			{
				FMODManager()->PlayLoopingMusic( pChannel,m_Songdata[1].path, NULL , m_flDelay );
			}
		}
	}

	HandleVolume();
	
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

static const ConVar *snd_musicvolume = NULL;
static const ConVar *snd_mute_losefocus = NULL;

void C_TFMusicPlayer::HandleVolume( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	snd_musicvolume = g_pCVar->FindVar("snd_musicvolume");
	snd_mute_losefocus = g_pCVar->FindVar("snd_mute_losefocus");

	float flVolumeMod = 1.0f;
	
	if (pLocalPlayer && !pLocalPlayer->IsAlive())
		flVolumeMod *= 0.4f;

	if (!engine->IsActiveApp() && snd_mute_losefocus->GetBool())
		flVolumeMod = 0.0f;
	
	if( pChannel )
		pChannel->setVolume(m_flVolume * snd_musicvolume->GetFloat() * flVolumeMod);
}

void C_TFMusicPlayer::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);
	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( bParsed )
			return;
		
		char szScriptName[MAX_PATH];
		Q_strncpy( szScriptName, szLoopingSong, sizeof( szScriptName ) );
		
		KeyValues *pSound = GetSoundscript( szScriptName );
		
		if ( pSound )
		{
			Q_strncpy( m_Songdata[1].name, pSound->GetString( "Name", "Unknown" ) , sizeof( m_Songdata[1].name ) );
			Q_strncpy( m_Songdata[1].artist, pSound->GetString( "Artist", "Unknown" ) , sizeof( m_Songdata[1].artist ) );
			Q_strncpy( m_Songdata[1].path, pSound->GetString( "wave" ) , sizeof( m_Songdata[1].path ) );
			m_Songdata[1].volume = pSound->GetFloat( "volume", 0.9f );
			const char *pszSrc = NULL;
			if ( !Q_strncmp( m_Songdata[1].path, "#", 1 ) )
			{
				pszSrc = m_Songdata[1].path + 1;
				Q_strncpy( m_Songdata[1].path, pszSrc, sizeof(m_Songdata[1].path) );
			}
			DevMsg("Intro is %s\nOutro is %s\n", pSound->GetString( "intro" ), pSound->GetString( "outro" ));
			KeyValues *pSoundIntro = GetSoundscript( pSound->GetString( "intro" ) );
			if( pSoundIntro )
			{
				Q_strncpy( m_Songdata[0].path, pSoundIntro->GetString( "wave" ) , sizeof( m_Songdata[0].path ) );
				if ( !Q_strncmp( m_Songdata[0].path, "#", 1 ) )
				{
					pszSrc = m_Songdata[0].path + 1;
					Q_strncpy( m_Songdata[0].path, pszSrc, sizeof(m_Songdata[0].path) );
				}
				DevMsg("Intro wav is %s\n", m_Songdata[0].path);
			}
			KeyValues *pSoundOutro = GetSoundscript( pSound->GetString( "outro" ) );
			if( pSoundOutro )
			{
				Q_strncpy( m_Songdata[2].path, pSoundOutro->GetString( "wave" ) , sizeof( m_Songdata[2].path ) );
				if ( !Q_strncmp( m_Songdata[2].path, "#", 1 ) )
				{
					pszSrc = m_Songdata[2].path + 1;
					Q_strncpy( m_Songdata[2].path, pszSrc, sizeof(m_Songdata[2].path) );
				}
				DevMsg("Outro wav is %s\n", m_Songdata[2].path);
			}
		}

		bParsed = true;			
	}
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
static ConCommand fm_parse_soundmanifest( "fm_parse_soundmanifest", ParseSoundManifest, "Parses the global Sounscript File.", FCVAR_NONE );

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
static ConCommand fm_check_globalsoundmanifest( "fm_check_globalsoundmanifest", CheckGlobalSounManifest, "Prints out all Manifest files.", FCVAR_NONE );

void ParseLevelSoundManifest( void )
{	
	InitLevelSoundManifest();
	
	char mapsounds[MAX_PATH];
	char mapname[ 256 ];
	Q_StripExtension( engine->GetLevelName(), mapname, sizeof( mapname ) );
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

static ConCommand fm_parse_level_sounds( "fm_parse_level_sounds", ParseLevelSoundManifest, "Parses the level_souns file for the current map.", FCVAR_NONE );

KeyValues* GetSoundscript( const char *szSoundScript )
{
	KeyValues *pSound = new KeyValues( "SoundScript" );
			
	char mapsounds[MAX_PATH];
	char mapname[ 256 ];
	Q_StripExtension( engine->GetLevelName(), mapname, sizeof( mapname ) );
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