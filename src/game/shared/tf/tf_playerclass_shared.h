//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============
//
// Purpose:
//
//=============================================================================
#ifndef TF_PLAYERCLASS_SHARED_H
#define TF_PLAYERCLASS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"

#define TF_NAME_LENGTH		128

// Client specific.
#ifdef CLIENT_DLL

EXTERN_RECV_TABLE( DT_TFPlayerClassShared );

// Server specific.
#else

EXTERN_SEND_TABLE( DT_TFPlayerClassShared );

#endif


//-----------------------------------------------------------------------------
// Cache structure for the TF player class data (includes citizen). 
//-----------------------------------------------------------------------------

#define MAX_PLAYERCLASS_SOUND_LENGTH	128

struct TFPlayerClassData_t
{
	char		m_szClassName[TF_NAME_LENGTH];
	char		m_szModelName[TF_NAME_LENGTH];
	char		m_szArmModelName[TF_NAME_LENGTH];
	char		m_szLocalizableName[TF_NAME_LENGTH];
	float		m_flMaxSpeed;
	int			m_nMaxHealth;
	int			m_nMaxArmor;
	int			m_aWeapons[TF_PLAYER_WEAPON_COUNT];
	int			m_aGrenades[TF_PLAYER_GRENADE_COUNT];
	int			m_aAmmoMax[TF_AMMO_COUNT];
	int			m_aBuildable[TF_PLAYER_BUILDABLE_COUNT];
	
	float		m_flTFCMaxSpeed;
	int			m_nTFCMaxHealth;
	int			m_nTFCMaxArmor;
	char		m_szTFCModelName[TF_NAME_LENGTH];
	char		m_szTFCArmModelName[TF_NAME_LENGTH];
	int			m_aTFCWeapons[TF_PLAYER_WEAPON_COUNT];
	int			m_aTFCBuildable[TF_PLAYER_BUILDABLE_COUNT];

	char		m_szZombieModelName[TF_NAME_LENGTH];
	char		m_szZombieArmModelName[TF_NAME_LENGTH];

	int			m_nCapNumber;
	int			m_nMaxAirDashCount;
	bool		m_bDontDoAirwalk;
	bool		m_bDontDoNewJump;
	
	bool		m_bSpecialClass;
	char 		m_szClassSelectImageRed[TF_NAME_LENGTH];
	char 		m_szClassSelectImageBlue[TF_NAME_LENGTH];
	char 		m_szClassSelectImageMercenary[TF_NAME_LENGTH];	
	
	char 		m_szClassImageRed[TF_NAME_LENGTH];
	char 		m_szClassImageBlue[TF_NAME_LENGTH];
	char 		m_szClassImageMercenary[TF_NAME_LENGTH];		
	char 		m_szClassImageColorless[TF_NAME_LENGTH];	
	
	int			m_nViewVector;

	bool		m_bParsed;

#ifdef GAME_DLL
	// sounds
	char		m_szDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
	char		m_szCritDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
	char		m_szMeleeDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
	char		m_szExplosionDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
#endif

	TFPlayerClassData_t();
	const char *GetModelName() const;
	const char *GetArmModelName() const { return m_szArmModelName; }
	const char *GetTFCModelName() const { return m_szTFCModelName; }
	const char *GetTFCArmModelName() const  { return m_szTFCArmModelName; }
	const char *GetZombieModelName() const { return m_szZombieModelName; }
	const char *GetZombieArmModelName() const  { return m_szZombieArmModelName; }
	const char *GetClassSelectImageRed() const { return m_szClassSelectImageRed; }
	const char *GetClassSelectImageBlue() const { return m_szClassSelectImageBlue; }
	const char *GetClassSelectImageMercenary() const { return m_szClassSelectImageMercenary; }
	
	const char *GetClassImageRed() const { return m_szClassImageRed; }
	const char *GetClassImageBlue() const { return m_szClassImageBlue; }
	const char *GetClassImageMercenary() const { return m_szClassImageMercenary; }
	const char *GetClassImageColorless() const { return m_szClassImageColorless; }
	void Parse( const char *pszClassName );

private:

	// Parser for the class data.
	void ParseData( KeyValues *pKeyValuesData );
};

void InitPlayerClasses( void );
TFPlayerClassData_t *GetPlayerClassData( int iClass );

//-----------------------------------------------------------------------------
// TF Player Class Shared
//-----------------------------------------------------------------------------
class CTFPlayerClassShared
{
public:

	CTFPlayerClassShared();

	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CTFPlayerClassShared );

	bool		Init( int iClass );
	bool		IsClass( int iClass ) const						{ return ( m_iClass == iClass ); }
	int			GetClassIndex( void )							{ return m_iClass; }
	
	const char	*GetName( void ) const							{ return GetPlayerClassData( m_iClass )->m_szClassName; }
#ifndef CLIENT_DLL
	void		SetCustomModel( const char *pszModelName );
	void		SetCustomArmModel( const char *pszModelName );
#endif	
	const char	*GetModelName( void ) const;
	const char	*GetTFCModelName( void ) const;
	const char	*GetZombieModelName( void ) const;
	const char 	*GetSetCustomModel ( void ) const;		
	bool 		UsesCustomModel ( void );	
	const char 	*GetSetCustomArmModel ( void ) const;		
	bool 		UsesCustomArmModel ( void );		

	const char	*GetArmModelName( void ) const;
	const char	*GetTFCArmModelName( void ) const;
	const char	*GetZombieArmModelName( void ) const;

	float		GetMaxSpeed( void )						{ return GetPlayerClassData( m_iClass )->m_flMaxSpeed; }
	int			GetMaxHealth( void )					{ return GetPlayerClassData( m_iClass )->m_nMaxHealth; }
	int			GetMaxArmor( void )						{ return GetPlayerClassData( m_iClass )->m_nMaxArmor; }
	bool		IsSpecialClass( void )					{ return GetPlayerClassData( m_iClass )->m_bSpecialClass; }
	bool		CanAirDash( void );
	int			MaxAirDashCount( void );
	int			GetCapNumber( void )					{ return GetPlayerClassData( m_iClass )->m_nCapNumber; }
	
	float		GetTFCMaxSpeed( void )							{ return GetPlayerClassData( m_iClass )->m_flTFCMaxSpeed; }
	int			GetTFCMaxHealth( void )							{ return GetPlayerClassData( m_iClass )->m_nTFCMaxHealth; }
	int			GetTFCMaxArmor( void )							{ return GetPlayerClassData( m_iClass )->m_nTFCMaxArmor; }
	

	TFPlayerClassData_t  *GetData( void )						{ return GetPlayerClassData( m_iClass ); }

	// If needed, put this into playerclass scripts
	bool CanBuildObject( int iObjectType );
	bool CanBuildTFCObject( int iObjectType );

protected:

	CNetworkVar( int,	m_iClass );
#ifdef CLIENT_DLL
	char	m_iszSetCustomModel[MAX_PATH];
	char	m_iszSetCustomArmModel[MAX_PATH];
#else
	CNetworkVar(string_t,m_iszSetCustomModel);
	CNetworkVar(string_t,m_iszSetCustomArmModel);
#endif
};

#endif // TF_PLAYERCLASS_SHARED_H