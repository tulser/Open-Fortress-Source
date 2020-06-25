//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "transitionpanel.h"
#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "vgui/ilocalize.h"
#include "vgui/iinput.h"
#include "ienginevgui.h"
#include "tier1/fmtstr.h"
#include "materialsystem/IMesh.h"
#include "shaderapi/ishaderapi.h"
// #include "gameconsole.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#include "materialsystem/itexture.h"

using namespace BaseModUI;
using namespace vgui;

ConVar ui_transition_debug( "ui_transition_debug", "0", FCVAR_DEVELOPMENTONLY, "" );
ConVar ui_transition_time( "ui_transition_time", "0.55", FCVAR_DEVELOPMENTONLY, "" );
ConVar ui_transition_delay( "ui_transition_delay", "0", FCVAR_DEVELOPMENTONLY, "" );
ConVar ui_transition_effect( "ui_transition_effect", "1", FCVAR_DEVELOPMENTONLY, "" );

#define TILE_NEAR_PLANE		1.0f
#define TILE_FAR_PLANE		257.0f
#define TILE_Z				( -128.0f )
#define GRID_WIDTH_WC		( 2.0f * -TILE_Z )
#define HALF_GRID_WIDTH_WC	( -TILE_Z )

struct ShaderStencilState_t
{
	bool m_bEnable;
	StencilOperation_t m_FailOp;
	StencilOperation_t m_ZFailOp;
	StencilOperation_t m_PassOp;
	StencilComparisonFunction_t m_CompareFunc;
	int m_nReferenceValue;
	uint32 m_nTestMask;
	uint32 m_nWriteMask;

	ShaderStencilState_t()
	{
		m_bEnable = false;
		m_PassOp = m_FailOp = m_ZFailOp = STENCILOPERATION_KEEP;
		m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
		m_nReferenceValue = 0;
		m_nTestMask = m_nWriteMask = 0xFFFFFFFF;
	}

	void SetStencilState(CMatRenderContextPtr &pRenderContext)
	{
		pRenderContext->SetStencilEnable(m_bEnable);
		pRenderContext->SetStencilFailOperation(m_FailOp);
		pRenderContext->SetStencilZFailOperation(m_ZFailOp);
		pRenderContext->SetStencilPassOperation(m_PassOp);
		pRenderContext->SetStencilCompareFunction(m_CompareFunc);
		pRenderContext->SetStencilReferenceValue(m_nReferenceValue);
		pRenderContext->SetStencilTestMask(m_nTestMask);
		pRenderContext->SetStencilWriteMask(m_nWriteMask);
	}
};

CBaseModTransitionPanel::CBaseModTransitionPanel( const char *pPanelName ) :
	BaseClass( NULL, pPanelName )
{
	HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx(0, "resource/" "basemodui_scheme" ".res", "basemodui_scheme");
	SetScheme(scheme);
	SetPostChildPaintEnabled( true );

	m_pFromScreenRT = NULL;
	m_pCurrentScreenRT = NULL;

	// needs to start at some number >1
	m_nFrameCount = 100;
	m_nNumTransitions = 0;

	m_bTransitionActive = false;
	m_bAllowTransitions = false;
	m_bQuietTransitions = false;

	m_pFromScreenMaterial = materials->FindMaterial( "console/rt_background", TEXTURE_GROUP_OTHER, true );
	m_pFromScreenMaterial->IncrementReferenceCount();
	m_pFromScreenMaterial->GetMappingWidth();

	m_pCurrentScreenMaterial = materials->FindMaterial( "console/rt_foreground", TEXTURE_GROUP_OTHER, true );
	m_pCurrentScreenMaterial->IncrementReferenceCount();
	m_pCurrentScreenMaterial->GetMappingWidth();

	m_pFromScreenRT = materials->FindTexture( "_rt_FullFrameFB", TEXTURE_GROUP_RENDER_TARGET );
	m_pCurrentScreenRT = materials->FindTexture( "_rt_ResolvedFullFrameDepth", TEXTURE_GROUP_RENDER_TARGET );

	m_bForwardHint = true;
	m_WindowTypeHint = WT_NONE;
	m_PreviousWindowTypeHint = WT_NONE;
	m_flDirection = 1.0f;
}

CBaseModTransitionPanel::~CBaseModTransitionPanel()
{
}

void CBaseModTransitionPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	BuildTiles();

	int screenWide, screenTall;
	surface()->GetScreenSize( screenWide, screenTall );

	SetPos( 0, 0 );
	SetSize( screenWide, screenTall );

	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);
}

void CBaseModTransitionPanel::OnKeyCodePressed( KeyCode keycode )
{
}

void CBaseModTransitionPanel::BuildTiles()
{
}

void CBaseModTransitionPanel::SetExpectedDirection( bool bForward, WINDOW_TYPE wt )
{
	// direction can only be determined by taking hints from the caller
	// at the moment the transition is triggered, we have to trust this
	m_bForwardHint = bForward;

	// can't rely on this going backwards where the nav gets subverted
	// this is used to isolate some edge states for windows that aren't tile based
	// attract and mainmenu don't have tiles but still need to drive transitioning TO tile based screens
	if ( m_PreviousWindowTypeHint != m_WindowTypeHint )
	{
		m_PreviousWindowTypeHint = m_WindowTypeHint;
	}
	m_WindowTypeHint = wt;
}

void BaseModUI::CBaseModTransitionPanel::StartTransition()
{
	SetInitialState();
	m_bTransitionActive = true;
}

void CBaseModTransitionPanel::MarkTilesInRect( int x, int y, int wide, int tall, WINDOW_TYPE wt, bool bForce )
{
	if ( !IsEffectEnabled() )
		return;

	m_nXOffset = x;
	m_nYOffset = y;
	m_nWidth = wide;
	m_nHeight = tall;

	m_WindowTypeHint = wt;

	MoveToFront();
}

void CBaseModTransitionPanel::PreventTransitions( bool bPrevent )
{
	if ( bPrevent )
	{
		m_bAllowTransitions = false;
	}
	else if ( !m_bAllowTransitions )
	{
		// only reset if we were disabled
		SetInitialState();
	}
}

void CBaseModTransitionPanel::SuspendTransitions( bool bSuspend )
{
	// quiet transitions still maintain state, but do no graphical effect
	// but can continue the graphical effect as needed
	m_bQuietTransitions = bSuspend;
}

void CBaseModTransitionPanel::TerminateEffect()
{
	if ( !m_bTransitionActive )
		return;

	m_bTransitionActive = false;

	if ( GetVPanel() == vgui::input()->GetModalSubTree() )
	{
		vgui::input()->ReleaseModalSubTree();
	}
}

void CBaseModTransitionPanel::SetInitialState()
{
	m_bAllowTransitions = true;
	m_bQuietTransitions = false;

	m_bForwardHint = true;
	m_WindowTypeHint = WT_NONE;
	m_PreviousWindowTypeHint = WT_NONE;

	m_nNumTransitions = 0;

	m_flMovieFadeInTime = 0;
}

bool CBaseModTransitionPanel::IsEffectEnabled()
{
	if ( !m_bAllowTransitions ||
		!ui_transition_effect.GetBool() ||
		!enginevgui->IsGameUIVisible() ||
		/*GameUI().IsInLevel() || 
		engine->IsConnected() ||*/
		CBaseModPanel::GetSingleton().IsLevelLoading() 
		// || ( !IsGameConsole() && GameConsole().IsConsoleVisible() ) ||
		// materials->IsStereoActiveThisFrame() ) // Disable effect when in nvidia's stereo mode
		)
	{
		// effect not allowed in game or loading into game
		if ( m_bTransitionActive )
		{
			// one-time clean up if some other state changed us immediately
			TerminateEffect();
		}

		return false;
	}

	return true;
}

bool CBaseModTransitionPanel::IsEffectActive()
{
	return m_bTransitionActive;
}

void CBaseModTransitionPanel::SaveCurrentScreen( ITexture *pRenderTarget )
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->CopyRenderTargetToTextureEx( pRenderTarget, 0, NULL, NULL );
	pRenderContext->SetFrameBufferCopyTexture( pRenderTarget, 0 );
}

// we have to draw the startup fade graphic using this function so it perfectly matches the one drawn by the engine during load
void DrawScreenSpaceRectangleAlpha11(IMaterial *pMaterial,
	int nDestX, int nDestY, int nWidth, int nHeight,	// Rect to draw into in screen space
	float flSrcTextureX0, float flSrcTextureY0,		// which texel you want to appear at destx/y
	float flSrcTextureX1, float flSrcTextureY1,		// which texel you want to appear at destx+width-1, desty+height-1
	int nSrcTextureWidth, int nSrcTextureHeight,		// needed for fixup
	void *pClientRenderable,							// Used to pass to the bind proxies
	int nXDice, int nYDice,							// Amount to tessellate the mesh
	float fDepth, float flAlpha)									// what Z value to put in the verts (def 0.0)
{
	CMatRenderContextPtr pRenderContext(g_pMaterialSystem);

	if ((nWidth <= 0) || (nHeight <= 0))
		return;

	pRenderContext->MatrixMode(MATERIAL_VIEW);
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode(MATERIAL_PROJECTION);
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->Bind(pMaterial, pClientRenderable);

	int xSegments = MAX(nXDice, 1);
	int ySegments = MAX(nYDice, 1);

	CMeshBuilder meshBuilder;

	IMesh* pMesh = pRenderContext->GetDynamicMesh(true);
	meshBuilder.Begin(pMesh, MATERIAL_QUADS, xSegments * ySegments);

	int nScreenWidth, nScreenHeight;
	pRenderContext->GetRenderTargetDimensions(nScreenWidth, nScreenHeight);
	float flLeftX = nDestX - 0.5f;
	float flRightX = nDestX + nWidth - 0.5f;

	float flTopY = nDestY - 0.5f;
	float flBottomY = nDestY + nHeight - 0.5f;

	float flSubrectWidth = flSrcTextureX1 - flSrcTextureX0;
	float flSubrectHeight = flSrcTextureY1 - flSrcTextureY0;

	float flTexelsPerPixelX = (nWidth > 1) ? flSubrectWidth / (nWidth - 1) : 0.0f;
	float flTexelsPerPixelY = (nHeight > 1) ? flSubrectHeight / (nHeight - 1) : 0.0f;

	float flLeftU = flSrcTextureX0 + 0.5f - (0.5f * flTexelsPerPixelX);
	float flRightU = flSrcTextureX1 + 0.5f + (0.5f * flTexelsPerPixelX);
	float flTopV = flSrcTextureY0 + 0.5f - (0.5f * flTexelsPerPixelY);
	float flBottomV = flSrcTextureY1 + 0.5f + (0.5f * flTexelsPerPixelY);

	float flOOTexWidth = 1.0f / nSrcTextureWidth;
	float flOOTexHeight = 1.0f / nSrcTextureHeight;
	flLeftU *= flOOTexWidth;
	flRightU *= flOOTexWidth;
	flTopV *= flOOTexHeight;
	flBottomV *= flOOTexHeight;

	// Get the current viewport size
	int vx, vy, vw, vh;
	pRenderContext->GetViewport(vx, vy, vw, vh);

	// map from screen pixel coords to -1..1
	flRightX = FLerp(-1, 1, 0, vw, flRightX);
	flLeftX = FLerp(-1, 1, 0, vw, flLeftX);
	flTopY = FLerp(1, -1, 0, vh, flTopY);
	flBottomY = FLerp(1, -1, 0, vh, flBottomY);

	// Dice the quad up...
	if (xSegments > 1 || ySegments > 1)
	{
		// Screen height and width of a subrect
		float flWidth = (flRightX - flLeftX) / (float)xSegments;
		float flHeight = (flTopY - flBottomY) / (float)ySegments;

		// UV height and width of a subrect
		float flUWidth = (flRightU - flLeftU) / (float)xSegments;
		float flVHeight = (flBottomV - flTopV) / (float)ySegments;

		for (int x = 0; x < xSegments; x++)
		{
			for (int y = 0; y < ySegments; y++)
			{
				// Top left
				meshBuilder.Position3f(flLeftX + (float)x * flWidth, flTopY - (float)y * flHeight, fDepth);
				meshBuilder.Normal3f(0.0f, 0.0f, 1.0f);
				meshBuilder.TexCoord2f(0, flLeftU + (float)x * flUWidth, flTopV + (float)y * flVHeight);
				meshBuilder.TangentS3f(0.0f, 1.0f, 0.0f);
				meshBuilder.TangentT3f(1.0f, 0.0f, 0.0f);
				meshBuilder.Color4ub(255, 255, 255, 255.0f * flAlpha);
				meshBuilder.AdvanceVertex();

				// Top right (x+1)
				meshBuilder.Position3f(flLeftX + (float)(x + 1) * flWidth, flTopY - (float)y * flHeight, fDepth);
				meshBuilder.Normal3f(0.0f, 0.0f, 1.0f);
				meshBuilder.TexCoord2f(0, flLeftU + (float)(x + 1) * flUWidth, flTopV + (float)y * flVHeight);
				meshBuilder.TangentS3f(0.0f, 1.0f, 0.0f);
				meshBuilder.TangentT3f(1.0f, 0.0f, 0.0f);
				meshBuilder.Color4ub(255, 255, 255, 255.0f * flAlpha);
				meshBuilder.AdvanceVertex();

				// Bottom right (x+1), (y+1)
				meshBuilder.Position3f(flLeftX + (float)(x + 1) * flWidth, flTopY - (float)(y + 1) * flHeight, fDepth);
				meshBuilder.Normal3f(0.0f, 0.0f, 1.0f);
				meshBuilder.TexCoord2f(0, flLeftU + (float)(x + 1) * flUWidth, flTopV + (float)(y + 1) * flVHeight);
				meshBuilder.TangentS3f(0.0f, 1.0f, 0.0f);
				meshBuilder.TangentT3f(1.0f, 0.0f, 0.0f);
				meshBuilder.Color4ub(255, 255, 255, 255.0f * flAlpha);
				meshBuilder.AdvanceVertex();

				// Bottom left (y+1)
				meshBuilder.Position3f(flLeftX + (float)x * flWidth, flTopY - (float)(y + 1) * flHeight, fDepth);
				meshBuilder.Normal3f(0.0f, 0.0f, 1.0f);
				meshBuilder.TexCoord2f(0, flLeftU + (float)x * flUWidth, flTopV + (float)(y + 1) * flVHeight);
				meshBuilder.TangentS3f(0.0f, 1.0f, 0.0f);
				meshBuilder.TangentT3f(1.0f, 0.0f, 0.0f);
				meshBuilder.Color4ub(255, 255, 255, 255.0f * flAlpha);
				meshBuilder.AdvanceVertex();
			}
		}
	}
	else // just one quad
	{
		for (int corner = 0; corner < 4; corner++)
		{
			bool bLeft = (corner == 0) || (corner == 3);
			meshBuilder.Position3f((bLeft) ? flLeftX : flRightX, (corner & 2) ? flBottomY : flTopY, fDepth);
			meshBuilder.Normal3f(0.0f, 0.0f, 1.0f);
			meshBuilder.TexCoord2f(0, (bLeft) ? flLeftU : flRightU, (corner & 2) ? flBottomV : flTopV);
			meshBuilder.TangentS3f(0.0f, 1.0f, 0.0f);
			meshBuilder.TangentT3f(1.0f, 0.0f, 0.0f);
			meshBuilder.Color4ub(255, 255, 255, 255.0f * flAlpha);
			meshBuilder.AdvanceVertex();
		}
	}

	meshBuilder.End();
	pMesh->Draw();

	pRenderContext->MatrixMode(MATERIAL_VIEW);
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode(MATERIAL_PROJECTION);
	pRenderContext->PopMatrix();
}

void CBaseModTransitionPanel::DrawEffect(float flNormalizedAlpha)
{
	int screenWide, screenTall;
	GetSize( screenWide, screenTall );
	surface()->DrawSetColor( 0, 0, 0, 255 );
	surface()->DrawFilledRect( 0, 0, screenWide, screenTall );


	CMatRenderContextPtr pRenderContext(g_pMaterialSystem);
	int w = GetWide();
	int h = GetTall();
	float depth = 0.5f;

	DrawScreenSpaceRectangleAlpha11(m_pCurrentScreenMaterial, 0, 0, w, h, 0, 0, w, h, w, h, NULL, 1, 1, depth, 
		1.0f);

	DrawScreenSpaceRectangleAlpha11(m_pFromScreenMaterial, m_nXOffset, m_nYOffset, m_nWidth, m_nHeight, m_nXOffset, m_nYOffset, m_nWidth + m_nXOffset, m_nHeight + m_nYOffset, w, h, NULL, 1, 1, depth, flNormalizedAlpha);
}

void CBaseModTransitionPanel::Paint()
{
	MoveToFront();

	if ( m_bTransitionActive )
	{
		SaveCurrentScreen( m_pCurrentScreenRT );

		if (!m_flMovieFadeInTime)
		{
			// do the fade a little bit after the movie starts (needs to be stable)
			// the product overlay will fade out
			m_flMovieFadeInTime = Plat_FloatTime();
		}

		float flFadeDelta = RemapValClamped(Plat_FloatTime(), m_flMovieFadeInTime, m_flMovieFadeInTime + 0.1f, 1.0f, 0.0f);
		if (flFadeDelta > 0.0f)
		{
			DrawEffect(flFadeDelta);
		}
		else
		{
			m_bTransitionActive = false;

			if (GetVPanel() == vgui::input()->GetModalSubTree())
			{
				vgui::input()->ReleaseModalSubTree();
			}
		}
	}
}

void CBaseModTransitionPanel::PostChildPaint()
{
	if ( !m_bTransitionActive )
	{
		// keep saving the current frame buffer to use as the 'from'
		SaveCurrentScreen( m_pFromScreenRT );
	}
}

void CBaseModTransitionPanel::StartPaint3D()
{
	// Save off the matrices in case the painting method changes them.
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	// const AspectRatioInfo_t &aspectRatioInfo = materials->GetAspectRatioInfo();
	// pRenderContext->PerspectiveX( 90, aspectRatioInfo.m_flFrameBufferAspectRatio, TILE_NEAR_PLANE, TILE_FAR_PLANE );
	pRenderContext->PerspectiveX(90, 1.78f, TILE_NEAR_PLANE, TILE_FAR_PLANE);

	pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );

	// Don't draw the 3D scene w/ stencil
	ShaderStencilState_t state;
	state.m_bEnable = false;
	// pRenderContext->SetStencilState( state );
	state.SetStencilState(pRenderContext);

	pRenderContext->ClearBuffers( false, true, true );
	pRenderContext->OverrideDepthEnable(true, true); // , true );
}

void CBaseModTransitionPanel::EndPaint3D()
{
	// Reset stencil to set stencil everywhere we draw 
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	if ( !IsGameConsole() )
	{
		pRenderContext->OverrideDepthEnable(false, true); // , true );
	}

	ShaderStencilState_t state;
	state.m_bEnable = true;
	state.m_FailOp = STENCILOPERATION_KEEP;
	state.m_ZFailOp = STENCILOPERATION_KEEP;
	state.m_PassOp = STENCILOPERATION_REPLACE;
	state.m_CompareFunc = STENCILCOMPARISONFUNCTION_GREATEREQUAL;
	state.m_nReferenceValue = 0;
	state.m_nTestMask = 0xFFFFFFFF;
	state.m_nWriteMask = 0xFFFFFFFF;
	// pRenderContext->SetStencilState( state );
	state.SetStencilState(pRenderContext);

	// Restore the matrices
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();

	pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );

	surface()->DrawSetTexture( -1 );
}

void CBaseModTransitionPanel::DrawTiles3D()
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

}
