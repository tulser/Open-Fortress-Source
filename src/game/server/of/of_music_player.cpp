//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "of_music_player.h"
#include "tf_player.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "engine/IEngineSound.h"
#include "entitylist.h"
#include "KeyValues.h"
#include "filesystem.h"

#include "tier0/memdbgon.h"

ConVar test_shouldplay("test_shouldplay", "1", FCVAR_NONE);

BEGIN_DATADESC(CTFMusicPlayer)
// Inputs.
DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
DEFINE_KEYFIELD( m_bPlayInWaitingForPlayers, FIELD_BOOLEAN, "WaitingForPlayers" ),
DEFINE_KEYFIELD( szIntroSong , FIELD_SOUNDNAME, "IntroSongName"), 
DEFINE_KEYFIELD( szLoopingSong , FIELD_SOUNDNAME, "LoopingSongName"), // Darude Sandstorm
DEFINE_KEYFIELD( szOutroSong , FIELD_SOUNDNAME, "IntroSongName"), 
DEFINE_KEYFIELD( m_iIndex, FIELD_INTEGER, "Index" ),
// Inputs.
DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
DEFINE_THINKFUNC( MusicThink ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CTFMusicPlayer, DT_MusicPlayer )
	SendPropStringT( SENDINFO( szIntroSong ) ),
	SendPropStringT( SENDINFO( szLoopingSong ) ),
	SendPropStringT( SENDINFO( szOutroSong ) ),
	SendPropBool( SENDINFO( m_bShouldBePlaying ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( of_music_player, CTFMusicPlayer );

CTFMusicPlayer::CTFMusicPlayer()
{
	SetThink( &CTFMusicPlayer::MusicThink );
	SetNextThink( gpGlobals->curtime );
}

void CTFMusicPlayer::MusicThink( void )
{
	m_bShouldBePlaying = ShouldPlay();
	SetNextThink( gpGlobals->curtime + 0.1 );
}

bool CTFMusicPlayer::ShouldPlay( void )
{
	if ( IsDisabled() )
		return false;
	
	if ( !m_bPlayInWaitingForPlayers && TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->IsInWaitingForPlayers() )
		return false;
	
	if ( m_bPlayInWaitingForPlayers && TeamplayRoundBasedRules() && !TeamplayRoundBasedRules()->IsInWaitingForPlayers() )
		return false;	
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicPlayer::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicPlayer::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMusicPlayer::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicPlayer::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicPlayer::InputToggle( inputdata_t &inputdata )
{
	m_bDisabled = !m_bDisabled;
}