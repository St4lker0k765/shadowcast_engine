// xrCore.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#pragma hdrstop

#include <mmsystem.h>
#include <objbase.h>
#include "xrCore.h"

#ifdef DEBUG
# include <malloc.h>
#endif // DEBUG

XRCORE_API xrCore Core;
XRCORE_API u32 build_id;
XRCORE_API LPCSTR build_date;

namespace CPU
{
extern void Detect();
};

#pragma comment(lib, "winmm.lib")

static u32 init_counter = 0;

//extern char g_application_path[256];

//. extern xr_vector<shared_str>* LogFile;

void xrCore::_initialize(LPCSTR _ApplicationName, LogCallback cb, BOOL init_fs, LPCSTR fs_fname)
{
    xr_strcpy(ApplicationName, _ApplicationName);
    if (0 == init_counter)
    {
#ifdef XRCORE_STATIC
        _clear87();
        _control87(_PC_53, MCW_PC);
        _control87(_RC_CHOP, MCW_RC);
        _control87(_RC_NEAR, MCW_RC);
        _control87(_MCW_EM, MCW_EM);
#endif
        // Init COM so we can use CoCreateInstance
        // HRESULT co_res =
        Params = xr_strdup(GetCommandLine());
        xr_strlwr(Params);
        if (!strstr(Params, "-editor"))
            CoInitializeEx(NULL, COINIT_MULTITHREADED);

        string_path fn, dr, di;

        // application path
        GetModuleFileName(GetModuleHandle(MODULE_NAME), fn, sizeof(fn));
        _splitpath(fn, dr, di, 0, 0);
        strconcat(sizeof(ApplicationPath), ApplicationPath, dr, di);
#ifndef _EDITOR
        //xr_strcpy(g_application_path, sizeof(g_application_path), ApplicationPath);
#endif

#ifdef _EDITOR
        // working path
        if (strstr(Params, "-wf"))
        {
            string_path c_name;
            sscanf(strstr(Core.Params, "-wf ") + 4, "%[^ ] ", c_name);
            SetCurrentDirectory(c_name);
        }
#endif

        GetCurrentDirectory(sizeof(WorkingPath), WorkingPath);

        // User/Comp Name
        DWORD sz_user = sizeof(UserName);
        GetUserName(UserName, &sz_user);

        DWORD sz_comp = sizeof(CompName);
        GetComputerName(CompName, &sz_comp);

        // Mathematics & PSI detection
        CPU::Detect();

        Memory._initialize(strstr(Params, "-mem_debug") ? TRUE : FALSE);

        DUMP_PHASE;

        InitLog();
        _initialize_cpu();

        // Debug._initialize ();

        rtc_initialize();

        xr_FS = xr_new<CLocatorAPI>();

        xr_EFS = xr_new<EFS_Utils>();
        //. R_ASSERT (co_res==S_OK);
    }
    if (init_fs)
    {
        u32 flags = 0;
        if (0 != strstr(Params, "-build")) flags |= CLocatorAPI::flBuildCopy;
        if (0 != strstr(Params, "-ebuild")) flags |= CLocatorAPI::flBuildCopy | CLocatorAPI::flEBuildCopy;
#ifdef DEBUG
        if (strstr(Params, "-cache")) flags |= CLocatorAPI::flCacheFiles;
        else flags &= ~CLocatorAPI::flCacheFiles;
#endif // DEBUG
#ifdef _EDITOR // for EDITORS - no cache
        flags &= ~CLocatorAPI::flCacheFiles;
#endif // _EDITOR
        flags |= CLocatorAPI::flScanAppRoot;

#ifndef _EDITOR
#ifndef ELocatorAPIH
        if (0 != strstr(Params, "-file_activity")) flags |= CLocatorAPI::flDumpFileActivity;
#endif
#endif
        FS._initialize(flags, 0, fs_fname);
		Msg("Shadowcast Engine build %d, %s\n", build_id, build_date);
		Msg("Underground Collective Discord: https://discord.gg/Z2YuvKmHhf");
        EFS._initialize();
#ifdef DEBUG
#ifndef _EDITOR
        Msg("Process heap 0x%08x", GetProcessHeap());
#endif
#endif // DEBUG
    }
    SetLogCB(cb);
    init_counter++;
}

#ifndef _EDITOR
#include "compression_ppmd_stream.h"
extern compression::ppmd::stream* trained_model;
#endif
void xrCore::_destroy()
{
    --init_counter;
    if (0 == init_counter)
    {
        FS._destroy();
        EFS._destroy();
        xr_delete(xr_FS);
        xr_delete(xr_EFS);

#ifndef _EDITOR
        if (trained_model)
        {
            void* buffer = trained_model->buffer();
            xr_free(buffer);
            xr_delete(trained_model);
        }
#endif
        xr_free(Params);
        Memory._destroy();
    }
}

xr_string ANSIToUTF8(const xr_string& string)
{
    wchar_t* wcs{};
    int Lenght_ = MultiByteToWideChar(1251, 0, string.c_str(), (int)string.size(), wcs, 0);
    wcs = new wchar_t[Lenght_ + 1];
    MultiByteToWideChar(1251, 0, string.c_str(), (int)string.size(), wcs, Lenght_);
    wcs[Lenght_] = L'\0';

    char* u8s = nullptr;
    Lenght_ = WideCharToMultiByte(CP_UTF8, 0, wcs, (int)std::wcslen(wcs), u8s, 0, nullptr, nullptr);
    u8s = new char[Lenght_ + 1];
    WideCharToMultiByte(CP_UTF8, 0, wcs, (int)std::wcslen(wcs), u8s, Lenght_, nullptr, nullptr);
    u8s[Lenght_] = '\0';

    xr_string result(u8s);
    delete[] wcs;
    delete[] u8s;

    return result;
}

#ifndef XRCORE_STATIC

//. why ???
#ifdef _EDITOR
BOOL WINAPI DllEntryPoint(HINSTANCE hinstDLL, DWORD ul_reason_for_call, LPVOID lpvReserved)
#else
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD ul_reason_for_call, LPVOID lpvReserved)
#endif
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        _clearfp();
        _controlfp(_RC_CHOP, _MCW_RC);
        _controlfp(_RC_NEAR, _MCW_RC);
        _control87(_MCW_EM, MCW_EM);
    }
        //. LogFile.reserve (256);
    break;
    case DLL_THREAD_ATTACH:
        if (!strstr(GetCommandLine(), "-editor"))
            CoInitializeEx(NULL, COINIT_MULTITHREADED);
        timeBeginPeriod(1);
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
#ifdef USE_MEMORY_MONITOR
        memory_monitor::flush_each_time(true);
#endif // USE_MEMORY_MONITOR
        break;
    }
    return TRUE;
}
#endif // XRCORE_STATIC