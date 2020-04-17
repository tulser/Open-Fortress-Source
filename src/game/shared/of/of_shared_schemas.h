#ifndef TF_SCHEMAS_H
#define TF_SCHEMAS_H

#ifdef _WIN32
#pragma once
#endif

class KeyValues;

extern void ParseSoundManifest( void );
extern void ParseLevelSoundManifest( void );
extern KeyValues *GetSoundscript( const char *szSoundScript );
#ifdef CLIENT_DLL
extern void PrecacheUISoundScript( char *szSoundScript );
#endif
extern char *GetSoundScriptWave( char *szSoundScript );

extern KeyValues* GlobalSoundManifest();
extern void InitGlobalSoundManifest();

extern KeyValues* LevelSoundManifest();
extern void InitLevelSoundManifest();

extern void ParseItemsGame( void );
extern void InitItemsGame();
extern KeyValues* GetItemsGame();

extern KeyValues* GetCosmetic( int iID );
extern KeyValues* GetRespawnParticle( int iID );

#ifdef CLIENT_DLL
extern KeyValues* GetLoadout();
extern void ParseLoadout( void );
extern void ResetLoadout( void );
#endif

extern void CheckGlobalSounManifest( void );

#endif // TF_SCHEMAS_H