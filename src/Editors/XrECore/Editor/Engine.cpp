// Engine.cpp: implementation of the CEngine class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "xrEngine/Engine.h"
#include "ui_main.h"

//xrDispatchTable	PSGP;                
CEngine	Engine;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEngine::CEngine()
{
	pSettings = 0;
}

CEngine::~CEngine()
{

}

LPCSTR CEngine::LastWindowsError()
{
	static string1024 errmsg_buf;
    LPCSTR err=0;

    u32 hr=GetLastError();
	if (hr!=0) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,hr,0,errmsg_buf,1024,0);
        err = errmsg_buf;
	}
    return err;
}

void CEngine::Initialize(void)
{
	// Other stuff
	string_path              fn;
    strconcat               (sizeof(fn),fn,UI->EditorName(),".log");
    FS.update_path			(fn,"$local_root$", fn);

	ReloadSettings			();
}

void CEngine::ReloadSettings()
{
	xr_delete				(pSettings);
    // game configure
    string_path 			si_name;
    FS.update_path			(si_name,"$game_config$","system.ltx");
	pSettings				= xr_new<CInifile>(si_name,TRUE);// FALSE,TRUE,TRUE);
}

void CEngine::Destroy()
{
    xr_delete				(pSettings);
}
