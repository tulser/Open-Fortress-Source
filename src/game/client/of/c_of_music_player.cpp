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

// Inputs.
LINK_ENTITY_TO_CLASS( of_music_player, C_TFMusicPlayer );

IMPLEMENT_CLIENTCLASS_DT( C_TFMusicPlayer, DT_MusicPlayer, CTFMusicPlayer )
	RecvPropString( RECVINFO( szIntroSong ) ),
	RecvPropString( RECVINFO( szLoopingSong ) ),
	RecvPropString( RECVINFO( szOutroSong ) ),
	RecvPropInt( RECVINFO( m_iPhase ) ),
	RecvPropBool( RECVINFO( m_bShouldBePlaying ) ),
	RecvPropFloat( RECVINFO( m_flDelay ) ),
END_RECV_TABLE()

C_TFMusicPlayer::C_TFMusicPlayer()
{
	bIsPlaying = false;
	bInLoop = false;
	bParsed = false;
	m_flDelay = 0;
	m_iPhase = 0;
	m_Songdata.SetSize( 3 );
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
		}
		else
		{
			FMODManager()->StopAmbientSound( pChannel, false );
			bIsPlaying = true;
			bInLoop = false;
			if ( m_Songdata[0].name[0] != 0 )
			{
				FMODManager()->PlayLoopingMusic( &pChannel, m_Songdata[1].path, m_Songdata[0].path, m_flDelay );
			}
			else
			{
				FMODManager()->PlayLoopingMusic( &pChannel, m_Songdata[1].path, NULL , m_flDelay );
			}
		}
	}
/*	else if ( bIsPlaying && !bInLoop && !FMODManager()->IsChannelPlaying() )
	{
		m_iPhase = 0;
		bInLoop = true;
		FMODManager()->PlayLoopingMusic(m_Songdata[1].path, false);
	}
*/
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

void C_TFMusicPlayer::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);
	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( bParsed )
			return;
		
		KeyValues *pManifest = new KeyValues( "Sounds" );
		
		char mapsounds[MAX_PATH];
		Q_snprintf( mapsounds, sizeof( mapsounds ), "maps/%s_level_sounds.txt",engine->GetLevelName() );
		pManifest->LoadFromFile( filesystem, mapsounds );
		if ( !pManifest ) 
		{
			pManifest->LoadFromFile( filesystem, "scripts/game_sounds_manifest.txt" );
		}
		if ( pManifest )
		{
			for (int i = 0; i < m_Songdata.Count(); i++ )
			{
				char szScriptName[MAX_PATH];
				switch ( i )
				{
					case 0:
						Q_strncpy( szScriptName, szIntroSong, sizeof( szScriptName ) );
						break;
					case 1:
						Q_strncpy( szScriptName, szLoopingSong, sizeof( szScriptName ) );
						break;
					case 2:
						Q_strncpy( szScriptName, szOutroSong, sizeof( szScriptName ) );
						break;
				}
				KeyValues *pSound = new KeyValues( "SoundScript" );
				pSound->LoadFromFile( filesystem, "scripts/game_sounds_music.txt" ); // Gets the first key in the file
				bool bFound = false;
				for( pSound; pSound != NULL; pSound = pSound->GetNextKey() ) // Loop through all the keyvalues
				{
					if( !Q_strncmp( szScriptName, pSound->GetName(), sizeof(szScriptName) ) ) // If their name matches our Soundscript name
					{
						bFound = true; // continue
						break;
					}
				}
				if ( bFound )
				{
					Q_strncpy( m_Songdata[i].name, pSound->GetString( "Name", "Unknown" ) , sizeof( m_Songdata[i].name ) );
					Q_strncpy( m_Songdata[i].artist, pSound->GetString( "Artist", "Unknown" ) , sizeof( m_Songdata[i].artist ) );
					Q_strncpy( m_Songdata[i].path, pSound->GetString( "wave", "#music/deathmatch/map01_loop.mp3" ) , sizeof( m_Songdata[i].path ) );
					m_Songdata[i].volume = pSound->GetFloat( "volume", 0.9f );
					m_Songdata[i].duration = pSound->GetFloat( "duration", 0.0f );
				}
				const char *pszSrc = NULL;
				if ( !Q_strncmp( m_Songdata[i].path, "#", 1 ) )
				{
					pszSrc = m_Songdata[i].path + 1;
					Q_strncpy( m_Songdata[i].path, pszSrc, sizeof(m_Songdata[i].path) );
				}
			}
			bParsed = true;			
		}
	}
}
