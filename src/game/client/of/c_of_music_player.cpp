//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Deathmatch weapon spawner
//
//=============================================================================//

#include "cbase.h"
#include "c_tf_player.h"
#include "view.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"
#include "tf_gamerules.h"
#include "fmod_manager.h"
#include "saverestore_utlvector.h"

// IMported from sounds	
#include "mathlib/mathlib.h"
#include "stringregistry.h"
#include "gamerules.h"
#include <ctype.h>
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "igamesystem.h"
#include "KeyValues.h"
#include "filesystem.h"

#include "tier0/memdbgon.h"

ConVar cl_song_comp("cl_song_comp", "-0.05", FCVAR_ARCHIVE, "How much time we add to the initial song duration before it loops\nWe do this because the actual song duration is not 100% accurate");
ConVar cl_parse_method("cl_parse_method", "0", FCVAR_ARCHIVE);

enum
{
	OF_MUSIC_INTRO = 0,
	OF_MUSIC_LOOP,
	OF_MUSIC_OUTRO,
};

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the Weapon Spawner
//-----------------------------------------------------------------------------
class C_TFMusicPlayer : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_TFMusicPlayer, C_BaseEntity );
	DECLARE_CLIENTCLASS();
	
	C_TFMusicPlayer();
	~C_TFMusicPlayer();
	virtual void ClientThink(void);
	virtual void Spawn(void);
	virtual void OnDataChanged(DataUpdateType_t updateType);

private:

	private:
	
	int m_iPhase;
	
	bool m_bShouldBePlaying;
	bool bIsPlaying;
	bool bInLoop;
	
	bool bParsed;
	
	float flLoopTick;

	char szIntroSong[MAX_PATH];
	char szLoopingSong[MAX_PATH];
	char szOutroSong[MAX_PATH];
	
	struct songdata_t
	{
		songdata_t()
		{
			name[0] = 0;
			artist[0] = 0;
			path[0] = 0;
			duration = 0;
		}

		char name[ 512 ];
		char artist[ 256 ];
		char path[ 256 ];
		float duration;
		float volume;
	};	
	CUtlVector<songdata_t>	m_Songdata;
};

// Inputs.
LINK_ENTITY_TO_CLASS( of_music_player, C_TFMusicPlayer );

IMPLEMENT_CLIENTCLASS_DT( C_TFMusicPlayer, DT_MusicPlayer, CTFMusicPlayer )
	RecvPropString( RECVINFO( szIntroSong ) ),
	RecvPropString( RECVINFO( szLoopingSong ) ),
	RecvPropString( RECVINFO( szOutroSong ) ),
	RecvPropInt( RECVINFO( m_iPhase ) ),
	RecvPropBool( RECVINFO( m_bShouldBePlaying ) ),
END_RECV_TABLE()

C_TFMusicPlayer::C_TFMusicPlayer()
{
	bIsPlaying = false;
	bInLoop = false;
	bParsed = false;
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
	FMODManager()->StopAmbientSound(false);
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
			FMODManager()->StopAmbientSound(false);
		}
		else
		{
			bIsPlaying = true;
			bInLoop = false;
			if ( m_Songdata[0].name[0] != 0 )
				FMODManager()->PlayLoopingMusic(m_Songdata[1].path, m_Songdata[0].path);
			else
				FMODManager()->PlayLoopingMusic(m_Songdata[1].path);
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
					if ( cl_parse_method.GetInt() == 1 )
					m_Songdata[i].duration = pSound->GetFloat( "duration", 0.0f );
				}
				const char *pszSrc = NULL;
				if ( !Q_strncmp( m_Songdata[i].path, "#", 1 ) )
				{
					pszSrc = m_Songdata[i].path + 1;
					Q_strncpy( m_Songdata[i].path, pszSrc, sizeof(m_Songdata[i].path) );
				}
				if ( cl_parse_method.GetInt() == 0 || m_Songdata[i].duration == 0.0f )
				m_Songdata[i].duration = enginesound->GetSoundDuration( m_Songdata[i].path );
			}
			bParsed = true;			
		}
	}
}
/*
void CMP3Player::GetLocalCopyOfSong( const char *szFullSongPath, char *outsong, size_t outlen )
{
	outsong[ 0 ] = 0;
	char fn[ 512 ];
	if ( !g_pFullFileSystem->String( szFullSongPath, fn, sizeof( fn ) ) )
	{
		return;
	}

	// Get temp filename from crc
	CRC32_t crc;
	CRC32_Init( &crc );
	CRC32_ProcessBuffer( &crc, fn, Q_strlen( fn ) );
	CRC32_Final( &crc );

	char hexname[ 16 ];
	Q_binarytohex( (const byte *)&crc, sizeof( crc ), hexname, sizeof( hexname ) );

	char hexfilename[ 512 ];
	Q_snprintf( hexfilename, sizeof( hexfilename ), "sound/temp/ogg/%s.ogg", hexname );

	Q_FixSlashes( hexfilename );

	if ( g_pFullFileSystem->FileExists( hexfilename, "MOD" ) )
	{
		Q_snprintf( outsong, outlen, "temp/ogg/%s.ogg", hexname );
	}
	else
	{
		// Make a local copy
		char ogg_temp_path[ 512 ];
		Q_snprintf( ogg_temp_path, sizeof( ogg_temp_path ), "sound/temp/ogg" );
		g_pFullFileSystem->CreateDirHierarchy( ogg_temp_path, "MOD" );

		char destpath[ 512 ];
		Q_snprintf( destpath, sizeof( destpath ), "%s/%s", engine->GetGameDirectory(), hexfilename );
		Q_FixSlashes( destpath );

		char sourcepath[ 512 ];

		Assert( mp3.dirnum >= 0 && mp3.dirnum < m_SoundDirectories.Count() );
		SoundDirectory_t *sdir = m_SoundDirectories[ mp3.dirnum ];
		Q_snprintf( sourcepath, sizeof( sourcepath ), "%s/%s", sdir->m_Root.String(), fn );
		Q_FixSlashes( sourcepath );

		// !!!HACK HACK:
		// Total hack right now, using windows OS calls to copy file to full destination
		int success = ::CopyFileA( sourcepath, destpath, TRUE );
		if ( success > 0 )
		{
			Q_snprintf( outsong, outlen, "_mp3/%s.mp3", hexname );
		}
	}

	Q_FixSlashes( outsong );
}
*/