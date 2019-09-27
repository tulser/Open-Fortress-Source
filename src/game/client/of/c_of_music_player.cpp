//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Deathmatch weapon spawner
//
//=============================================================================//

#include "cbase.h"
#include "c_tf_player.h"
#include "view.h"

#include "tf_gamerules.h"

#include "tier0/memdbgon.h"

enum
{
	OF_MUSIC_OFF = 0,
	OF_MUSIC_INTRO,
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
	
private:

	private:
	
	int m_iPhase;
	
	bool m_bShouldBePlaying;
	bool bIsPlaying;

	char m_nszIntroSong[MAX_PATH];
	char m_nszLoopingSong[MAX_PATH];
	char m_nszOutroSong[MAX_PATH];
};

// Inputs.
LINK_ENTITY_TO_CLASS( of_music_player, C_TFMusicPlayer );

IMPLEMENT_CLIENTCLASS_DT( C_TFMusicPlayer, DT_MusicPlayer, CTFMusicPlayer )
	RecvPropString( RECVINFO( m_nszIntroSong ) ),
	RecvPropString( RECVINFO( m_nszLoopingSong ) ),
	RecvPropString( RECVINFO( m_nszOutroSong ) ),
	RecvPropInt( RECVINFO( m_iPhase ) ),
END_RECV_TABLE()

C_TFMusicPlayer::C_TFMusicPlayer()
{
	bIsPlaying = false;
}