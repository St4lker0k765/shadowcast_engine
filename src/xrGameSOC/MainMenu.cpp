#include "stdafx.h"
#include "MainMenu.h"
#include "UI/UIDialogWnd.h"
#include "../xrEngine/xr_IOConsole.h"
#include "../xrEngine/IGame_Level.h"
#include "../xrEngine/CameraManager.h"
#include "xr_Level_controller.h"
#include "ui/UITextureMaster.h"
#include "ui/UIXmlInit.h"
#include <dinput.h>
#include "ui/UIBtnHint.h"
#include "UICursor.h"
#include "../xrEngine/x_ray.h"
#include "string_table.h"
#include "object_broker.h"
#include "../xrEngine/DiscordSDK.h"

//#define DEMO_BUILD

extern bool b_shniaganeed_pp;

CMainMenu*	MainMenu()	{return (CMainMenu*)g_pGamePersistent->m_pMainMenu; };
//----------------------------------------------------------------------------------
#define INIT_MSGBOX(_box, _template)	{ _box = xr_new<CUIMessageBoxEx>(); _box->Init(_template);}
//----------------------------------------------------------------------------------

CMainMenu::CMainMenu	()
{
	m_Flags.zero					();
	m_startDialog					= NULL;
	m_screenshotFrame				= u32(-1);
	g_pGamePersistent->m_pMainMenu	= this;
	if (Device.b_is_Ready)			OnDeviceCreate();  	
	ReadTextureInfo					();
	CUIXmlInit::InitColorDefs		();
	g_btnHint						= NULL;
	m_deactivated_frame				= 0;	
	
	m_sPatchURL						= "";
	m_pGameSpyFull					= NULL;

	m_sPDProgress.IsInProgress		= false;

	//---------------------------------------------------------------
	m_NeedErrDialog					= ErrNoError;
	m_start_time					= 0;

	g_btnHint						= xr_new<CUIButtonHint>();
}

CMainMenu::~CMainMenu	()
{
	xr_delete						(g_btnHint);
	xr_delete						(m_startDialog);
	g_pGamePersistent->m_pMainMenu	= NULL;
}

void CMainMenu::ReadTextureInfo()
{
	FS_FileSet fset;
	FS.file_list(fset, "$game_config$", FS_ListFiles, "ui\\textures_descr\\*.xml");
	FS_FileSetIt fit = fset.begin();
	FS_FileSetIt fit_e = fset.end();

	for (; fit != fit_e; ++fit)
	{
		string_path	fn1, fn2, fn3;
		_splitpath((*fit).name.c_str(), fn1, fn2, fn3, 0);
		strcat(fn3, ".xml");

		CUITextureMaster::ParseShTexInfoNew(fn3);
	}
	if (pSettings->section_exist("texture_desc"))
	{
		xr_string itemsList; 
		string256 single_item;

		itemsList = pSettings->r_string("texture_desc", "files");
		int itemsCount	= _GetItemCount(itemsList.c_str());

		for (int i = 0; i < itemsCount; i++)
		{
			_GetItem(itemsList.c_str(), i, single_item);
			strcat(single_item,".xml");
			CUITextureMaster::ParseShTexInfo(single_item);
		}		
	}
}

extern ENGINE_API BOOL	bShowPauseString;
extern bool				IsGameTypeSingle();

void CMainMenu::Activate	(bool bActivate)
{
	if (	!!m_Flags.test(flActive) == bActivate)		return;
	if (	m_Flags.test(flGameSaveScreenshot)	)		return;
	if (	(m_screenshotFrame == Device.dwFrame)	||
			(m_screenshotFrame == Device.dwFrame-1) ||
			(m_screenshotFrame == Device.dwFrame+1))	return;

	bool b_is_single		= IsGameTypeSingle();

	if(g_dedicated_server && bActivate) return;

	if(bActivate)
	{
		b_shniaganeed_pp			= true;
		Device.Pause				(TRUE, FALSE, TRUE, "mm_activate1");
			m_Flags.set				(flActive|flNeedChangeCapture,TRUE);

		m_Flags.set					(flRestoreCursor,GetUICursor()->IsVisible());

		if(!ReloadUI())				return;

		m_Flags.set					(flRestoreConsole,Console->bVisible);
		
		if(b_is_single)	m_Flags.set	(flRestorePause,Device.Paused());
		
		Console->Hide				();

		if(b_is_single)
		{
			m_Flags.set					(flRestorePauseStr, bShowPauseString);
			bShowPauseString			= FALSE;
			if(!m_Flags.test(flRestorePause))
				Device.Pause			(TRUE, TRUE, FALSE, "mm_activate2");
		}
		
		if(g_pGameLevel)
		{
			if(b_is_single){
				Device.seqFrame.Remove		(g_pGameLevel);
			}
			Device.seqRender.Remove			(g_pGameLevel);
			CCameraManager::ResetPP			();
		};
		Device.seqRender.Add				(this, 4); // 1-console 2-cursor 3-tutorial

		if (!g_pGameLevel) 
		{
			Discord.SetStatus(CStringTable().translate("st_discord_main_menu").c_str());
			Discord.SetPhase("");
		}

	}else{
		m_deactivated_frame					= Device.dwFrame;
		m_Flags.set							(flActive,				FALSE);
		m_Flags.set							(flNeedChangeCapture,	TRUE);

		Device.seqRender.Remove				(this);
		
		bool b = !!Console->bVisible;
		if(b){
			Console->Hide					();
		}

		IR_Release							();
		if(b){
			Console->Show					();
		}

		StartStopMenu						(m_startDialog,true);
		CleanInternals						();
		if(g_pGameLevel)
		{
			if(b_is_single){
				Device.seqFrame.Add			(g_pGameLevel);

			}
			Device.seqRender.Add			(g_pGameLevel);
		};
		if(m_Flags.test(flRestoreConsole))
			Console->Show			();

		if(b_is_single)
		{
			if(!m_Flags.test(flRestorePause))
				Device.Pause			(FALSE, TRUE, FALSE, "mm_deactivate1");

			bShowPauseString			= m_Flags.test(flRestorePauseStr);
		}	
	
		if(m_Flags.test(flRestoreCursor))
			GetUICursor()->Show			();

		Device.Pause					(FALSE, FALSE, TRUE, "mm_deactivate2");

		if(m_Flags.test(flNeedVidRestart))
		{
			m_Flags.set			(flNeedVidRestart, FALSE);
			Console->Execute	("vid_restart");
		}
	}
}

bool CMainMenu::ReloadUI()
{
	if(m_startDialog)
	{
		StartStopMenu						(m_startDialog,true);
		CleanInternals						();
	}
	DLL_Pure* dlg = NEW_INSTANCE(TEXT2CLSID("MAIN_MNU"));
	if(!dlg) 
	{
		m_Flags.set				(flActive|flNeedChangeCapture,FALSE);
		return false;
	}
	xr_delete					(m_startDialog);
	m_startDialog				= smart_cast<CUIDialogWnd*>(dlg);
	VERIFY						(m_startDialog);
	m_startDialog->m_bWorkInPause= true;
	StartStopMenu				(m_startDialog,true);

	m_activatedScreenRatio		= (float)Device.dwWidth/(float)Device.dwHeight > (1024.0f/768.0f+0.01f);
	return true;
}

bool CMainMenu::IsActive()
{
	return !!m_Flags.test(flActive);
}

bool CMainMenu::CanSkipSceneRendering()
{
	return IsActive() && !m_Flags.test(flGameSaveScreenshot);
}

//IInputReceiver
static int mouse_button_2_key []	=	{MOUSE_1,MOUSE_2,MOUSE_3};
void	CMainMenu::IR_OnMousePress				(int btn)	
{	
	if(!IsActive()) return;

	IR_OnKeyboardPress(mouse_button_2_key[btn]);
};

void	CMainMenu::IR_OnMouseRelease(int btn)	
{
	if(!IsActive()) return;

	IR_OnKeyboardRelease(mouse_button_2_key[btn]);
};

void	CMainMenu::IR_OnMouseHold(int btn)	
{
	if(!IsActive()) return;

	IR_OnKeyboardHold(mouse_button_2_key[btn]);

};

void	CMainMenu::IR_OnMouseMove(int x, int y)
{
	if(!IsActive()) return;

	if(MainInputReceiver())
		MainInputReceiver()->IR_OnMouseMove(x, y);

};

void	CMainMenu::IR_OnMouseStop(int x, int y)
{
};

void	CMainMenu::IR_OnKeyboardPress(int dik)
{
	if(!IsActive()) return;

	if( is_binded(kCONSOLE, dik) )
	{
		Console->Show();
		return;
	}
	if (DIK_F12 == dik){
		Render->Screenshot();
		return;
	}

	if(MainInputReceiver())
		MainInputReceiver()->IR_OnKeyboardPress( dik);

};

void	CMainMenu::IR_OnKeyboardRelease			(int dik)
{
	if(!IsActive()) return;
	
	if(MainInputReceiver())
		MainInputReceiver()->IR_OnKeyboardRelease(dik);

};

void	CMainMenu::IR_OnKeyboardHold(int dik)	
{
	if(!IsActive()) return;
	
	if(MainInputReceiver())
		MainInputReceiver()->IR_OnKeyboardHold(dik);
};

void CMainMenu::IR_OnMouseWheel(int direction)
{
	if(!IsActive()) return;
	
	if(MainInputReceiver())
		MainInputReceiver()->IR_OnMouseWheel(direction);
}


bool CMainMenu::OnRenderPPUI_query()
{
	return IsActive() && !m_Flags.test(flGameSaveScreenshot) && b_shniaganeed_pp;
}


extern void draw_wnds_rects();
void CMainMenu::OnRender	()
{
	if(m_Flags.test(flGameSaveScreenshot))
		return;

	if(g_pGameLevel)
		Render->Calculate			();

	Render->Render				();
	if(!OnRenderPPUI_query())
	{
		DoRenderDialogs();
		UI()->RenderFont();
		draw_wnds_rects();
	}
}

void CMainMenu::OnRenderPPUI_main	()
{
	if(!IsActive()) return;

	if(m_Flags.test(flGameSaveScreenshot))
		return;

	UI()->pp_start();

	if(OnRenderPPUI_query())
	{
		DoRenderDialogs();
		UI()->RenderFont();
	}

	UI()->pp_stop();
}

void CMainMenu::OnRenderPPUI_PP	()
{
	if ( !IsActive() ) return;

	if(m_Flags.test(flGameSaveScreenshot))	return;

	UI()->pp_start();
	
	xr_vector<CUIWindow*>::iterator it = m_pp_draw_wnds.begin();
	for(; it!=m_pp_draw_wnds.end();++it)
	{
		(*it)->Draw();
	}
	UI()->pp_stop();
}

void CMainMenu::StartStopMenu(CUIDialogWnd* pDialog, bool bDoHideIndicators)
{
	pDialog->m_bWorkInPause = true;
	CDialogHolder::StartStopMenu(pDialog, bDoHideIndicators);
}

//pureFrame
void CMainMenu::OnFrame()
{
	if (m_Flags.test(flNeedChangeCapture))
	{
		m_Flags.set					(flNeedChangeCapture,FALSE);
		if (m_Flags.test(flActive))	IR_Capture();
		else						IR_Release();
	}
	CDialogHolder::OnFrame		();


	//screenshot stuff
	if(m_Flags.test(flGameSaveScreenshot) && Device.dwFrame > m_screenshotFrame  )
	{
		m_Flags.set					(flGameSaveScreenshot,FALSE);
		::Render->Screenshot		(IRender_interface::SM_FOR_GAMESAVE, m_screenshot_name);
		
		if(g_pGameLevel && m_Flags.test(flActive))
		{
			Device.seqFrame.Remove	(g_pGameLevel);
			Device.seqRender.Remove	(g_pGameLevel);
		};

		if(m_Flags.test(flRestoreConsole))
			Console->Show			();
	}

	if (IsActive())
	{
		bool b_is_16_9 = (float)Device.dwWidth / (float)Device.dwHeight > (1024.0f / 768.0f + 0.01f);
		if (b_is_16_9 != m_activatedScreenRatio)
		{
			ReloadUI();
		}
	}
}

void CMainMenu::OnDeviceCreate()
{
}


void CMainMenu::Screenshot(IRender_interface::ScreenshotMode mode, LPCSTR name)
{
	if(mode != IRender_interface::SM_FOR_GAMESAVE)
	{
		::Render->Screenshot		(mode,name);
	}else{
		m_Flags.set					(flGameSaveScreenshot, TRUE);
		strcpy(m_screenshot_name,name);
		if(g_pGameLevel && m_Flags.test(flActive)){
			Device.seqFrame.Add		(g_pGameLevel);
			Device.seqRender.Add	(g_pGameLevel);
		};
		m_screenshotFrame			= Device.dwFrame+1;
		m_Flags.set					(flRestoreConsole,		Console->bVisible);
		Console->Hide				();
	}
}

void CMainMenu::RegisterPPDraw(CUIWindow* w)
{
	UnregisterPPDraw				(w);
	m_pp_draw_wnds.push_back		(w);
}

void CMainMenu::UnregisterPPDraw				(CUIWindow* w)
{
	m_pp_draw_wnds.erase(
		std::remove(
			m_pp_draw_wnds.begin(),
			m_pp_draw_wnds.end(),
			w
		),
		m_pp_draw_wnds.end()
	);
}

void CMainMenu::SetErrorDialog					(EErrorDlg ErrDlg)	
{ 
};

void CMainMenu::CheckForErrorDlg()
{
};

void CMainMenu::SwitchToMultiplayerMenu()
{
};

void CMainMenu::DestroyInternal(bool bForce)
{
	if(m_startDialog && ((m_deactivated_frame < Device.dwFrame+4)||bForce) )
		xr_delete		(m_startDialog);
}

void CMainMenu::OnNewPatchFound(LPCSTR VersionName, LPCSTR URL)
{
}

void CMainMenu::OnNoNewPatchFound				()
{
}

void CMainMenu::OnDownloadPatch(CUIWindow*, void*)
{
}

void	CMainMenu::OnDownloadPatchError()
{
};

void	CMainMenu::OnDownloadPatchSuccess			()
{;
}

void CMainMenu::OnSessionTerminate(LPCSTR reason)
{
}

void	CMainMenu::OnLoadError(LPCSTR module)
{
}

void	CMainMenu::OnDownloadPatchProgress			(u64 bytesReceived, u64 totalSize)
{
};

extern ENGINE_API string512  g_sLaunchOnExit_app;
extern ENGINE_API string512  g_sLaunchOnExit_params;
void	CMainMenu::OnRunDownloadedPatch			(CUIWindow*, void*)
{
}

void CMainMenu::CancelDownload()
{
	
}

void CMainMenu::SetNeedVidRestart()
{
	m_Flags.set(flNeedVidRestart,TRUE);
}

void CMainMenu::OnDeviceReset()
{
	if(IsActive() && g_pGameLevel)
		SetNeedVidRestart();
}

bool		CMainMenu::ValidateCDKey					()
{
	return true;
}

void		CMainMenu::Show_CTMS_Dialog				()
{
}

void		CMainMenu::Hide_CTMS_Dialog()
{
}

void CMainMenu::OnConnectToMasterServerOkClicked(CUIWindow*, void*)
{
}

LPCSTR CMainMenu::GetGSVer()
{
	return pApp->ShowCurrentVersion();
}
