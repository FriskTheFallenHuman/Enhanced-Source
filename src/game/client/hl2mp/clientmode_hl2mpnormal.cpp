//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws the normal TF2 or HL2 HUD.
//
//=============================================================================//
#include "cbase.h"
#include "clientmode_hl2mpnormal.h"
#include "c_hl2mp_player.h"
#include "vgui_int.h"
#include "hud.h"
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include "iinput.h"
#include "input.h"
#include "vgui_int.h"
#include "ienginevgui.h"
#include "cdll_client_int.h"
#include "engine/IEngineSound.h"
#include "panelmetaclassmgr.h"
#include "hl2mpclientscoreboard.h"
#include "hl2mptextwindow.h"
#include "viewpostprocess.h"
#include "shaderapi/ishaderapi.h"
#include "tier2/renderutils.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "object_motion_blur_effect.h"
#include "glow_outline_effect.h"
#include "ivieweffects.h"
#include "voice_status.h"
//#include "nb_header_footer.h"
#include "hud_macros.h"
#include "view_shared.h"
#include "hud_basechat.h"
#include "achievementmgr.h"
#include "fmtstr.h"
#include "ivieweffects.h"
#include "shake.h"
#include <vgui/ILocalize.h>
#include "c_playerresource.h"
#include "soundenvelope.h"
#include "PanelMetaClassMgr.h"
#include "c_user_message_register.h"
#include "inetchannelinfo.h"
#include "engine/IVDebugOverlay.h"
#include "debugoverlay_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule the dll and that we don't call Sys_LoadModule 
//  over and over again.
static CDllDemandLoader g_GameUI( "gameui" );

extern bool g_bRollingCredits;

extern ConVar mat_object_motion_blur_enable;

ConVar default_fov( "default_fov", "75", FCVAR_CHEAT );
ConVar fov_desired( "fov_desired", "75", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets the base field-of-view.", true, 75.0, true, 90.0 );

vgui::HScheme g_hVGuiCombineScheme = 0;

static IClientMode *g_pClientMode[ MAX_SPLITSCREEN_PLAYERS ];
IClientMode *GetClientMode()
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return g_pClientMode[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}

// --------------------------------------------------------------------------------- //
// CVoxManager implementation.
// --------------------------------------------------------------------------------- //
// Voice data
void VoxCallback( IConVar *var, const char *oldString, float oldFloat )
{
	if ( engine && engine->IsConnected() )
	{
		ConVarRef voice_vox( var->GetName() );
		if ( voice_vox.GetBool() )
			engine->ClientCmd( "voicerecord_toggle on\n" );
		else
			engine->ClientCmd( "voicerecord_toggle off\n" );
	}
}

ConVar voice_vox( "voice_vox", "0", FCVAR_ARCHIVE, "Voice chat uses a vox-style always on", false, 0, true, 1, VoxCallback );

void  CVoxManager::LevelInitPostEntity( void )
{
	if ( voice_vox.GetBool() )
		engine->ClientCmd( "voicerecord_toggle on\n" );
}

void  CVoxManager::LevelShutdownPreEntity( void )
{
	if ( voice_vox.GetBool() )
		engine->ClientCmd( "voicerecord_toggle off\n" );
}

static CVoxManager s_VoxManager;

// --------------------------------------------------------------------------------- //
// CHL2ModeManager implementation.
// --------------------------------------------------------------------------------- //
static CHL2ModeManager g_ModeManager;
IVModeManager *modemanager = ( IVModeManager * )&g_ModeManager;

void CHL2ModeManager::Init()
{
	for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		g_pClientMode[ i ] = GetClientModeNormal();
	}

	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );

	GetClientVoiceMgr()->SetHeadLabelOffset( 40 );
}

void CHL2ModeManager::LevelInit( const char *newmap )
{
	for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		GetClientMode()->LevelInit( newmap );
	}
}

void CHL2ModeManager::LevelShutdown( void )
{
	for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		GetClientMode()->LevelShutdown();
	}
}

ClientModeHL2MPNormal g_ClientModeNormal[ MAX_SPLITSCREEN_PLAYERS ];
IClientMode *GetClientModeNormal()
{
	Assert( engine->IsLocalPlayerResolvable() );
	return &g_ClientModeNormal[ engine->GetActiveSplitScreenPlayerSlot() ];
}

ClientModeHL2MPNormal* GetClientModeHL2MPNormal()
{
	Assert( engine->IsLocalPlayerResolvable() );
	return &g_ClientModeNormal[ engine->GetActiveSplitScreenPlayerSlot() ];
}

// --------------------------------------------------------------------------------- //
// CHudViewport implementation.
// --------------------------------------------------------------------------------- //
void CHudViewport::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	GetHud().InitColors( pScheme );

	SetPaintBackgroundEnabled( false );
}

IViewPortPanel* CHudViewport::CreatePanelByName( const char *szPanelName )
{
	IViewPortPanel* newpanel = NULL;

	if ( Q_strcmp( PANEL_SCOREBOARD, szPanelName ) == 0 )
	{
		newpanel = new CHL2MPClientScoreBoardDialog( this );
		return newpanel;
	}
	else if ( Q_strcmp( PANEL_INFO, szPanelName ) == 0 )
	{
		newpanel = new CHL2MPTextWindow( this );
		return newpanel;
	}
	else if ( Q_strcmp( PANEL_SPECGUI, szPanelName ) == 0 )
	{
		newpanel = new CHL2MPSpectatorGUI( this );	
		return newpanel;
	}

	return BaseClass::CreatePanelByName( szPanelName ); 
}

// --------------------------------------------------------------------------------- //
// FullscreenHL2Viewport implementation.
// --------------------------------------------------------------------------------- //
static ClientModeHL2MPNormalFullscreen g_FullscreenClientMode;
IClientMode *GetFullscreenClientMode( void ) { return &g_FullscreenClientMode; }

void FullscreenHL2Viewport::InitViewportSingletons( void )
{
	SetAsFullscreenViewportInterface();
}

// --------------------------------------------------------------------------------- //
// ClientModeHL2MPNormalFullscreen implementation.
// --------------------------------------------------------------------------------- //
void ClientModeHL2MPNormalFullscreen::InitViewport()
{
	// Skip over BaseClass!!!
	BaseClass::InitViewport();

	m_pViewport = new FullscreenHL2Viewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

void ClientModeHL2MPNormalFullscreen::Init()
{
	// Skip over BaseClass!!!
	BaseClass::Init();

	// Load up the combine control panel scheme
	if ( !g_hVGuiCombineScheme )
	{
		g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), IsXbox() ? "resource/ClientScheme.res" : "resource/CombinePanelScheme.res", "CombineScheme" );
		if (!g_hVGuiCombineScheme)
			Warning( "Couldn't load combine panel scheme!\n" );
	}
}

// --------------------------------------------------------------------------------- //
// ClientModeHL2MPNormal implementation.
// --------------------------------------------------------------------------------- //
bool ClientModeHL2MPNormal::ShouldDrawCrosshair( void )
{
	return ( g_bRollingCredits == false );
}

void ClientModeHL2MPNormal::Init()
{
	BaseClass::Init();

	gameeventmanager->AddListener( this, "game_newmap", false );

	// Load up the combine control panel scheme
	g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), IsXbox() ? "resource/ClientScheme.res" : "resource/CombinePanelScheme.res", "CombineScheme" );
	if ( !g_hVGuiCombineScheme )
		Warning( "Couldn't load combine panel scheme!\n" );
}

int ClientModeHL2MPNormal::GetDeathMessageStartHeight( void )
{
	return m_pViewport->GetDeathMessageStartHeight();
}

void ClientModeHL2MPNormal::Shutdown()
{
	/*if ( BackgroundMovie() )
		BackgroundMovie()->ClearCurrentMovie();*/
}

void ClientModeHL2MPNormal::InitViewport()
{
	m_pViewport = new CHudViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

void ClientModeHL2MPNormal::LevelInit( const char *newmap )
{
	// reset ambient light
	static ConVarRef mat_ambient_light_r( "mat_ambient_light_r" );
	static ConVarRef mat_ambient_light_g( "mat_ambient_light_g" );
	static ConVarRef mat_ambient_light_b( "mat_ambient_light_b" );

	if ( mat_ambient_light_r.IsValid() )
		mat_ambient_light_r.SetValue( "0" );

	if ( mat_ambient_light_g.IsValid() )
		mat_ambient_light_g.SetValue( "0" );

	if ( mat_ambient_light_b.IsValid() )
		mat_ambient_light_b.SetValue( "0" );

	BaseClass::LevelInit( newmap );

	// clear any DSP effects
	CLocalPlayerFilter filter;
	enginesound->SetRoomType( filter, 0 );
	enginesound->SetPlayerDSP( filter, 0, true );
}

void ClientModeHL2MPNormal::LevelShutdown( void )
{
	BaseClass::LevelShutdown();
}

void ClientModeHL2MPNormal::FireGameEvent( IGameEvent *event )
{
	const char *eventname = event->GetName();

	if ( Q_strcmp( "game_newmap", eventname ) == 0 )
		engine->ClientCmd( "exec newmapsettings\n" );
	else
		BaseClass::FireGameEvent( event );
}

void ClientModeHL2MPNormal::Update( void )
{
	BaseClass::Update();
}

void ClientModeHL2MPNormal::DoPostScreenSpaceEffects( const CViewSetup *pSetup )
{
	CMatRenderContextPtr pRenderContext( materials );

	bool g_bRenderingGlows;
	
	if ( mat_object_motion_blur_enable.GetBool() )
		DoObjectMotionBlur( pSetup );
	
	// Render object glows and selectively-bloomed objects (under sniper scope)
	g_bRenderingGlows = true;
	g_GlowObjectManager.RenderGlowEffects( pSetup, GetSplitScreenPlayerSlot() );
	g_bRenderingGlows = false;
}

void ClientModeHL2MPNormal::DoObjectMotionBlur( const CViewSetup *pSetup )
{
	if ( g_ObjectMotionBlurManager.GetDrawableObjectCount() <= 0 )
		return;

	CMatRenderContextPtr pRenderContext( materials );

	ITexture *pFullFrameFB1 = materials->FindTexture( "_rt_FullFrameFB1", TEXTURE_GROUP_RENDER_TARGET );

	//
	// Render Velocities into a full-frame FB1
	//
	IMaterial *pGlowColorMaterial = materials->FindMaterial( "dev/glow_color", TEXTURE_GROUP_OTHER, true );
	
	pRenderContext->PushRenderTargetAndViewport();
	pRenderContext->SetRenderTarget( pFullFrameFB1 );
	pRenderContext->Viewport( 0, 0, pSetup->width, pSetup->height );

	// Red and Green are x- and y- screen-space velocities biased and packed into the [0,1] range.
	// A value of 127 gets mapped to 0, a value of 0 gets mapped to -1, and a value of 255 gets mapped to 1.
	//
	// Blue is set to 1 within the object's bounds and 0 outside, and is used as a mask to ensure that
	// motion blur samples only pull from the core object itself and not surrounding pixels (even though
	// the area being blurred is larger than the core object).
	//
	// Alpha is not used
	pRenderContext->ClearColor4ub( 127, 127, 0, 0 );
	// Clear only color, not depth & stencil
	pRenderContext->ClearBuffers( true, false, false );

	// Save off state
	Vector vOrigColor;
	render->GetColorModulation( vOrigColor.Base() );

	// Use a solid-color unlit material to render velocity into the buffer
	g_pStudioRender->ForcedMaterialOverride( pGlowColorMaterial );
	g_ObjectMotionBlurManager.DrawObjects();	
	g_pStudioRender->ForcedMaterialOverride( NULL );

	render->SetColorModulation( vOrigColor.Base() );
	
	pRenderContext->PopRenderTargetAndViewport();

	//
	// Render full-screen pass
	//
	IMaterial *pMotionBlurMaterial;
	IMaterialVar *pFBTextureVariable;
	IMaterialVar *pVelocityTextureVariable;
	bool bFound1 = false, bFound2 = false;

	// Make sure our render target of choice has the results of the engine post-process pass
	ITexture *pFullFrameFB = materials->FindTexture( "_rt_FullFrameFB", TEXTURE_GROUP_RENDER_TARGET );
	pRenderContext->CopyRenderTargetToTexture( pFullFrameFB );

	pMotionBlurMaterial = materials->FindMaterial( "effects/object_motion_blur", TEXTURE_GROUP_OTHER, true );
	pFBTextureVariable = pMotionBlurMaterial->FindVar( "$fb_texture", &bFound1, true );
	pVelocityTextureVariable = pMotionBlurMaterial->FindVar( "$velocity_texture", &bFound2, true );
	if ( bFound1 && bFound2 )
	{
		pFBTextureVariable->SetTextureValue( pFullFrameFB );
		
		pVelocityTextureVariable->SetTextureValue( pFullFrameFB1 );

		int nWidth, nHeight;
		pRenderContext->GetRenderTargetDimensions( nWidth, nHeight );

		pRenderContext->DrawScreenSpaceRectangle( pMotionBlurMaterial, 0, 0, nWidth, nHeight, 0.0f, 0.0f, nWidth - 1, nHeight - 1, nWidth, nHeight );
	}
}

void ClientModeHL2MPNormal::UpdatePostProcessingEffects()
{
	C_PostProcessController *pNewPostProcessController = NULL;
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if ( pPlayer )
		pNewPostProcessController = pPlayer->GetActivePostProcessController();

	// Figure out new endpoints for parameter lerping
	if ( pNewPostProcessController != m_pCurrentPostProcessController )
	{
		m_LerpStartPostProcessParameters = m_CurrentPostProcessParameters;
		m_LerpEndPostProcessParameters = pNewPostProcessController ? pNewPostProcessController->m_PostProcessParameters : PostProcessParameters_t();
		m_pCurrentPostProcessController = pNewPostProcessController;

		float flFadeTime = pNewPostProcessController ? pNewPostProcessController->m_PostProcessParameters.m_flParameters[ PPPN_FADE_TIME ] : 0.0f;
		if ( flFadeTime <= 0.0f )
			flFadeTime = 0.001f;

		m_PostProcessLerpTimer.Start( flFadeTime );
	}

	// Lerp between start and end
	float flLerpFactor = 1.0f - m_PostProcessLerpTimer.GetRemainingRatio();
	for ( int nParameter = 0; nParameter < POST_PROCESS_PARAMETER_COUNT; ++ nParameter )
	{
		m_CurrentPostProcessParameters.m_flParameters[ nParameter ] = 
			Lerp( 
				flLerpFactor, 
				m_LerpStartPostProcessParameters.m_flParameters[ nParameter ], 
				m_LerpEndPostProcessParameters.m_flParameters[ nParameter ] );
	}
	SetPostProcessParams( &m_CurrentPostProcessParameters );
}