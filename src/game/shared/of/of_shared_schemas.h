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
extern KeyValues* GetWeaponFromSchema( const char *szName );

extern KeyValues* GetRespawnParticle( int iID );

class CTFItemSchema
{
public:
	CTFItemSchema();
	void PurgeSchema();
	
	void AddWeapon( const char *szWeaponName );
	KeyValues *GetWeapon( int iID );
	KeyValues *GetWeapon( const char *szWeaponName );

	int GetWeaponID( const char *szWeaponName );
	
	int GetWeaponCount( void ){ return m_hWeaponNames.Count();};
private:
	CUtlVector<const char*> m_hWeaponNames;
};

extern CTFItemSchema *GetItemSchema();
extern void InitItemSchema();

#ifdef CLIENT_DLL
extern KeyValues* GetLoadout();
extern void ParseLoadout( void );
extern void ResetLoadout( const char *szCatName );

typedef CHandle<ConVar>	ConVarHandle;

class CTFLoadoutHandler
{
public:
	CTFLoadoutHandler();
	CUtlVector<ConVar*> m_hClassLoadouts;
	CUtlVector<ConVar*> m_hClassArsenal;
};

extern CTFLoadoutHandler *GetLoadoutHandle();

extern void InitLoadoutHandle();

#endif

extern void CheckGlobalSounManifest( void );

#endif // TF_SCHEMAS_H