//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_MUSIC_PLAYER_H
#define TF_MUSIC_PLAYER_H

#ifdef _WIN32
#pragma once
#endif

enum
{
	OF_MUSIC_OFF = 0,
	OF_MUSIC_INTRO,
	OF_MUSIC_LOOP,
	OF_MUSIC_OUTRO,
};

class CTFMusicPlayer : public CBaseAnimating
{
public:
	DECLARE_CLASS(CTFMusicPlayer, CBaseEntity);
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
	
	CTFMusicPlayer();
	virtual void MusicThink();
	int UpdateTransmitState()	// always send to all clients
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}	
	
	bool			IsDisabled( void );
	bool			ShouldPlay( void );
	// Input handlers
	void			InputEnable( inputdata_t &inputdata );
	void			InputDisable( inputdata_t &inputdata );
	void			InputToggle( inputdata_t &inputdata );	
	void			SetDisabled( bool bDisable );
	int m_iIndex;

	CNetworkVar(string_t, szIntroSong);
	CNetworkVar(string_t, szLoopingSong);
	CNetworkVar(string_t, szOutroSong);
	
	CNetworkVar( bool, m_bShouldBePlaying );
	CNetworkVar( bool, m_bPlayInWaitingForPlayers );
	CNetworkVar( bool, m_bDisabled );
	CNetworkString( m_nszIntroSong , MAX_PATH );
	CNetworkString( m_nszLoopingSong , MAX_PATH );
	CNetworkString( m_nszOutroSong , MAX_PATH );
};
#endif // TF_MUSIC_PLAYER_H