#include "stdafx.h"
#include "HUDManager.h"
#include "hudtarget.h"

#include "actor.h"
#include "MainMenu.h"
#include "../xrEngine/igame_level.h"
#include "clsid_game.h"
#include "GamePersistent.h"
#include "UIGameCustom.h"
#include "game_cl_base.h"
#include "UIFontDefines.h"

extern CUIGameCustom*	CurrentGameUI()	{return HUD().GetGameUI();}
//--------------------------------------------------------------------
CHUDManager::CHUDManager()
{ 
	b_online = false;
	pUIGame = nullptr;
	m_pHUDTarget = xr_new<CHUDTarget>();
}
//--------------------------------------------------------------------
CHUDManager::~CHUDManager()
{
	OnDisconnected();

	if (pUIGame)
		pUIGame->UnLoad();

	xr_delete(pUIGame);
	xr_delete(m_pHUDTarget);
}

//--------------------------------------------------------------------

void CHUDManager::OnFrame()
{
	if (!b_online)					
		return;

	if (!b_online)
		return;

	if (pUIGame)
		pUIGame->OnFrame();

	m_pHUDTarget->CursorOnFrame();
}
//--------------------------------------------------------------------
//--------------------------------------------------------------------

ENGINE_API extern float psHUD_FOV;

void CHUDManager::Render_First()
{
	if (!psHUD_Flags.is(HUD_WEAPON|HUD_WEAPON_RT))
		return;

	if (!pUIGame)						
		return;

	CObject*	O					= g_pGameLevel->CurrentViewEntity();
	if (0==O)						return;
	CActor*		A					= smart_cast<CActor*> (O);
	if (!A)							return;
	if (A && !A->HUDview())			return;

	// only shadow 
	::Render->set_Invisible			(TRUE);
	::Render->set_Object			(O->H_Root());
	O->renderable_Render			();
	::Render->set_Invisible			(FALSE);
}

void CHUDManager::Render_Last()
{
	if (!psHUD_Flags.is(HUD_WEAPON | HUD_WEAPON_RT))
		return;

	if (!pUIGame)
		return;

	CObject* O = g_pGameLevel->CurrentViewEntity();
	if (0 == O)						return;
	CActor* A = smart_cast<CActor*> (O);
	if (A && !A->HUDview())
		return;
	if (O->CLS_ID == CLSID_CAR)
		return;

	// hud itself
	Render->set_HUD(TRUE);
	Render->set_Object(O->H_Root());
	O->OnHUDDraw(this);
	Render->set_HUD(FALSE);
}
extern void draw_wnds_rects();
extern ENGINE_API BOOL bShowPauseString;
//отрисовка элементов интерфейса
#include "string_table.h"
void  CHUDManager::RenderUI()
{
	if(!b_online)					return;

	BOOL bAlready					= FALSE;
	if (true)
	{
		HitMarker.Render			();
		if (pUIGame)
			pUIGame->Render();

		UI().RenderFont();
	}

	if (psHUD_Flags.is(HUD_CROSSHAIR|HUD_CROSSHAIR_RT|HUD_CROSSHAIR_RT2) && !bAlready)	
		m_pHUDTarget->Render();

	draw_wnds_rects		();

	if( Device.Paused() && bShowPauseString){
		CGameFont* pFont	= UI().Font().GetFont(GRAFFITI50_FONT_NAME);
		pFont->SetColor		(0x80FF0000	);
		LPCSTR _str			= CStringTable().translate("st_game_paused").c_str();
		
		Fvector2			_pos;
		_pos.set			(UI_BASE_WIDTH/2.0f, UI_BASE_HEIGHT/2.0f);
		UI().ClientToScreenScaled(_pos);
		pFont->SetAligment	(CGameFont::alCenter);
		pFont->Out			(_pos.x, _pos.y, _str);
		pFont->OnRender		();
	}

}

void CHUDManager::OnEvent(EVENT E, u64 P1, u64 P2)
{
}

collide::rq_result&	CHUDManager::GetCurrentRayQuery	() 
{
	return m_pHUDTarget->RQ;
}

void CHUDManager::SetCrosshairDisp	(float dispf, float disps)
{	
	m_pHUDTarget->HUDCrosshair.SetDispersion(psHUD_Flags.test(HUD_CROSSHAIR_DYNAMIC) ? dispf : disps);
}

void  CHUDManager::ShowCrosshair	(bool show)
{
	m_pHUDTarget->m_bShowCrosshair = show;
}


void CHUDManager::Hit(int idx, float power, const Fvector& dir)	
{
	HitMarker.Hit(idx, dir);
}

void CHUDManager::SetHitmarkType		(LPCSTR tex_name)
{
	HitMarker.InitShader				(tex_name);
}

// ------------------------------------------------------------------------------------

#include "ui\UIMainInGameWnd.h"
void CHUDManager::Load()
{
	if (!pUIGame)
	{
		pUIGame				= Game().createGameUI();
	} else
	{
		pUIGame->SetClGame	(&Game());
	}
}

void CHUDManager::OnScreenResolutionChanged()
{
	pUIGame->HideShownDialogs();

	//xr_delete(pWpnScopeXml);

	pUIGame->UnLoad();
	pUIGame->Load();

	pUIGame->OnConnected();
}

void CHUDManager::OnDisconnected()
{
    b_online				= false;
	if(pUIGame)
		Device.seqFrame.Remove	(pUIGame);
}

void CHUDManager::OnConnected()
{
	if(b_online)			return;
	b_online				= true;
	if(pUIGame)
		Device.seqFrame.Add	(pUIGame,REG_PRIORITY_LOW-1000);
}

void CHUDManager::net_Relcase	(CObject *object)
{
	VERIFY						(m_pHUDTarget);
	m_pHUDTarget->net_Relcase	(object);
}

CDialogHolder* CurrentDialogHolder()
{
	if (MainMenu()->IsActive())
		return MainMenu();
	else
		return HUD().GetGameUI();
}


bool   CHUDManager::RenderActiveItemUIQuery()
{
	return 0;
}

void   CHUDManager::RenderActiveItemUI()
{
}

//restore????
/*
#include "player_hud.h"
bool   CHUDManager::RenderActiveItemUIQuery()
{
	if (!psHUD_Flags.is(HUD_DRAW_RT2))
		return false;

	if (!psHUD_Flags.is(HUD_WEAPON|HUD_WEAPON_RT|HUD_WEAPON_RT2))return false;

	if(!need_render_hud())			return false;

	return (g_player_hud && g_player_hud->render_item_ui_query() );
}

void   CHUDManager::RenderActiveItemUI()
{
	if (!psHUD_Flags.is(HUD_DRAW_RT2))
		return;

	g_player_hud->render_item_ui		();
}
*/
