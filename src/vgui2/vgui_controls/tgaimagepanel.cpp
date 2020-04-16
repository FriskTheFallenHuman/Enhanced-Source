//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "vgui_controls/tgaimagepanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CTGAImagePanel::CTGAImagePanel( vgui::Panel *parent, const char *name ) : Panel(parent, name)
{
}

CTGAImagePanel::~CTGAImagePanel()
{
}

void CTGAImagePanel::SetTGAFilename( const char *filename )
{
}
 
void CTGAImagePanel::GetSettings(KeyValues *outResourceData)
{
	Panel::GetSettings(outResourceData);
}

void CTGAImagePanel::ApplySettings(KeyValues *inResourceData)
{
	Panel::ApplySettings(inResourceData);
}

void CTGAImagePanel::Paint( void )
{
	Panel::Paint();
}