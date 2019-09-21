#include "cbase.h"
#include "vloadout.h"

using namespace vgui;
using namespace BaseModUI;

ConVarRef ofd_tennisball( "ofd_tennisball" );

//-----------------------------------------------------------------------------
// From animation.cpp
//-----------------------------------------------------------------------------
int LookupSequence( CStudioHdr* pstudiohdr, const char* label )
{
	VPROF( "LookupSequence" );

	if ( !pstudiohdr )
		return 0;

	if ( !pstudiohdr->SequencesAvailable() )
		return 0;

	//
	// Look up by sequence name.
	//
	for ( int i = 0; i < pstudiohdr->GetNumSeq(); i++ )
	{
		mstudioseqdesc_t& seqdesc = pstudiohdr->pSeqdesc( i );
		if ( stricmp( seqdesc.pszLabel(), label ) == 0 )
			return i;
	}

	return ACT_INVALID;
}

Loadout::Loadout( Panel* parent, const char* panelName ) : CBaseModFrame( parent, panelName )
{
	SetProportional( true );
	m_pMercPanel = new CMDLPanel( this, "MercenaryModel" );
}

Loadout::~Loadout()
{
	if ( m_pMercPanel )
	{
		m_pMercPanel->DeletePanel();
		m_pMercPanel = NULL;
	}
}

void Loadout::ApplySchemeSettings( vgui::IScheme* pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "Resource/UI/BaseModUI/Loadout.res" );
}

void Loadout::PaintBackground()
{
	vgui::Panel* pPanel = FindChildByName( "PnlBackground" );
	if ( !pPanel )
		return;

	int x, y, wide, tall;
	pPanel->GetBounds( x, y, wide, tall );
	DrawBlackBackground( x, y, wide, tall );
}

void Loadout::PerformLayout()
{
	MDLHandle_t hMercModel = mdlcache->FindMDL( "models/player/mercenary.mdl" );
	if ( hMercModel != MDLHANDLE_INVALID )
	{
		m_pMercPanel->SetMDL( hMercModel );
		CStudioHdr pMercStudio( mdlcache->GetStudioHdr( hMercModel ), mdlcache );
		m_pMercPanel->SetSequence( LookupSequence( &pMercStudio, "competitive_loserstate_idle" ) );
	}

	if( ofd_tennisball.GetBool() )
		m_pMercPanel->SetSkin( 6 );
	else
		m_pMercPanel->SetSkin( 4 );

	Vector vecBoundsMin, vecBoundsMax;
	m_pMercPanel->GetBoundingBox( vecBoundsMin, vecBoundsMax );

	QAngle AngleRotation( 0, 180, 0 );

	Vector vecMin = vecBoundsMin;
	Vector vecMax = vecBoundsMax;
	Vector vecCenter = ( vecMax + vecMin ) * 0.5f;
	vecMin -= vecCenter;
	vecMax -= vecCenter;

	// Get the bounds points and transform them by the desired model panel rotation.
	Vector aBoundsPoints[8];
	aBoundsPoints[0].Init( vecMax.x, vecMax.y, vecMax.z );
	aBoundsPoints[1].Init( vecMin.x, vecMax.y, vecMax.z );
	aBoundsPoints[2].Init( vecMax.x, vecMin.y, vecMax.z );
	aBoundsPoints[3].Init( vecMin.x, vecMin.y, vecMax.z );
	aBoundsPoints[4].Init( vecMax.x, vecMax.y, vecMin.z );
	aBoundsPoints[5].Init( vecMin.x, vecMax.y, vecMin.z );
	aBoundsPoints[6].Init( vecMax.x, vecMin.y, vecMin.z );
	aBoundsPoints[7].Init( vecMin.x, vecMin.y, vecMin.z );

	// Translated center point (offset from camera center).
	Vector vecTranslateCenter = -vecCenter;

	// Build the rotation matrix.
	matrix3x4_t matRotation;
	AngleMatrix( AngleRotation, matRotation );

	Vector aXFormPoints[8];
	for ( int iPoint = 0; iPoint < 8; ++iPoint )
	{
		VectorTransform( aBoundsPoints[iPoint], matRotation, aXFormPoints[iPoint] );
	}

	Vector vecXFormCenter;
	VectorTransform( -vecTranslateCenter, matRotation, vecXFormCenter );

	int w, h;
	m_pMercPanel->GetSize( w, h );
	float flW = (float )w;
	float flH = (float )h;

	float flFOVx = DEG2RAD( 70 * 0.5f );
	float flFOVy = CalcFovY( ( 70 * 0.5f ), flW / flH );
	flFOVy = DEG2RAD( flFOVy );

	float flTanFOVx = tan( flFOVx );
	float flTanFOVy = tan( flFOVy );

	// Find the max value of x, y, or z
	Vector2D dist[8];
	float flDist = 0.0f;
	for ( int iPoint = 0; iPoint < 8; ++iPoint )
	{
		float flDistY = fabs( aXFormPoints[iPoint].y / flTanFOVx ) - aXFormPoints[iPoint].x;
		float flDistZ = fabs( aXFormPoints[iPoint].z / flTanFOVy ) - aXFormPoints[iPoint].x;
		dist[iPoint].x = flDistY;
		dist[iPoint].y = flDistZ;
		float flTestDist = MAX( flDistZ, flDistY );
		flDist = MAX( flDist, flTestDist );
	}

	// Screen space points.
	Vector2D aScreenPoints[8];
	Vector aCameraPoints[8];
	for ( int iPoint = 0; iPoint < 8; ++iPoint )
	{
		aCameraPoints[iPoint] = aXFormPoints[iPoint];
		aCameraPoints[iPoint].x += flDist;

		aScreenPoints[iPoint].x = aCameraPoints[iPoint].y / ( flTanFOVx * aCameraPoints[iPoint].x );
		aScreenPoints[iPoint].y = aCameraPoints[iPoint].z / ( flTanFOVy * aCameraPoints[iPoint].x );

		aScreenPoints[iPoint].x = ( aScreenPoints[iPoint].x * 0.5f + 0.5f ) * flW;
		aScreenPoints[iPoint].y = ( aScreenPoints[iPoint].y * 0.5f + 0.5f ) * flH;
	}

	// Find the min/max and center of the 2D bounding box of the object.
	Vector2D vecScreenMin( 99999.0f, 99999.0f ), vecScreenMax( -99999.0f, -99999.0f );
	for ( int iPoint = 0; iPoint < 8; ++iPoint )
	{
		vecScreenMin.x = MIN( vecScreenMin.x, aScreenPoints[iPoint].x );
		vecScreenMin.y = MIN( vecScreenMin.y, aScreenPoints[iPoint].y );
		vecScreenMax.x = MAX( vecScreenMax.x, aScreenPoints[iPoint].x );
		vecScreenMax.y = MAX( vecScreenMax.y, aScreenPoints[iPoint].y );
	}

	// Offset the model to the be the correct distance away from the camera.
	Vector vecModelPos;
	vecModelPos.x = flDist - vecXFormCenter.x;
	vecModelPos.y = -vecXFormCenter.y;
	vecModelPos.z = -vecXFormCenter.z;
	m_pMercPanel->SetModelAnglesAndPosition( AngleRotation, vecModelPos );

	// Back project to figure out the camera offset to center the model.
	Vector2D vecPanelCenter( ( flW * 0.5f ), ( flH * 0.5f ) );
	Vector2D vecScreenCenter = ( vecScreenMax + vecScreenMin ) * 0.5f;

	Vector2D vecPanelCenterCamera, vecScreenCenterCamera;
	vecPanelCenterCamera.x = ( ( vecPanelCenter.x / flW ) * 2.0f ) - 0.5f;
	vecPanelCenterCamera.y = ( ( vecPanelCenter.y / flH ) * 2.0f ) - 0.5f;
	vecPanelCenterCamera.x *= ( flTanFOVx * flDist );
	vecPanelCenterCamera.y *= ( flTanFOVy * flDist );
	vecScreenCenterCamera.x = ( ( vecScreenCenter.x / flW ) * 2.0f ) - 0.5f;
	vecScreenCenterCamera.y = ( ( vecScreenCenter.y / flH ) * 2.0f ) - 0.5f;
	vecScreenCenterCamera.x *= ( flTanFOVx * flDist );
	vecScreenCenterCamera.y *= ( flTanFOVy * flDist );

	Vector2D vecCameraOffset( 0.0f, 0.0f );
	vecCameraOffset.x = vecPanelCenterCamera.x - vecScreenCenterCamera.x;
	vecCameraOffset.y = vecPanelCenterCamera.y - vecScreenCenterCamera.y;

	// Clear the camera pivot and set position matrix.
	m_pMercPanel->ResetCameraPivot();

	vecCameraOffset.x = 0.0f;

	m_pMercPanel->SetCameraOffset( Vector( 0.0f, -vecCameraOffset.x, -vecCameraOffset.y ) );
	m_pMercPanel->SetCameraPositionAndAngles( Vector( -15, 0, -20 ), QAngle( 0, 0, 0 ) );
	m_pMercPanel->UpdateCameraTransform();
	m_pMercPanel->SetLockView( true );
}

void Loadout::OnCommand( const char* command )
{
	if ( Q_stricmp( "Back", command ) == 0 )
	{
		OnKeyCodePressed( KEY_XBUTTON_B );
	}
}