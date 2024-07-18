#include "stdafx.h"
#include "igame_level.h"
#include "igame_persistent.h"
#include "../xrNetServer/NET_AuthCheck.h"

#include "ILoadingScreen.h"
#include "xr_input.h"
#include "xr_ioconsole.h"
#include "x_ray.h"
#include "std_classes.h"
#include "GameFont.h"
#include "resource.h"
#include "LightAnimLibrary.h"
#include "../xrcdb/ispatial.h"
#include <clocale>
#include "DiscordSDK.h"

#include "xrSash.h"

ENGINE_API CInifile* pGameIni = nullptr;
ENGINE_API CApplication* pApp = NULL;
ENGINE_API bool g_bBenchmark = false;
ENGINE_API string512 g_sLaunchOnExit_params;
ENGINE_API string512 g_sLaunchOnExit_app;
ENGINE_API string_path g_sLaunchWorkingFolder;

ENGINE_API bool TheShadowWayMode = false;
ENGINE_API bool CallOfPripyatMode = true;
ENGINE_API bool ClearSkyMode = false;
ENGINE_API bool ShadowOfChernobylMode = false;

ENGINE_API int ps_rs_loading_stages = 1;

ENGINE_API int psXInputEnable = 1;

XRCORE_API LPCSTR build_date;
XRCORE_API u32 build_id
;
string512 g_sBenchmarkName;

BOOL g_bIntroFinished = FALSE;
static HWND logoWindow = NULL;

void doBenchmark(LPCSTR name);

const TCHAR* c_szSplashClass = _T("SplashWindow");

#ifdef MASTER_GOLD
# define NO_MULTI_INSTANCES
#endif // #ifdef MASTER_GOLD

static LPCSTR month_id[12] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static int days_in_month[12] =
{
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int start_day = 10; 
static int start_month = 6;
static int start_year = 2023;

// binary hash, mainly for copy-protection
#include <cctype>
#include <thread>

void compute_build_id()
{
    build_date = __DATE__;

    int days;
    int months = 0;
    int years;
    string16 month;
    string256 buffer;
    xr_strcpy(buffer, __DATE__);
    sscanf(buffer, "%s %d %d", month, &days, &years);

    for (int i = 0; i < 12; i++)
    {
        if (_stricmp(month_id[i], month) != 0)
            continue;

        months = i;
        break;
    }

    build_id = (years - start_year) * 365 + days - start_day;

    for (int i = 0; i < months; ++i)
        build_id += days_in_month[i];

    for (int i = 0; i < start_month - 1; ++i)
        build_id -= days_in_month[i];
}

struct _SoundProcessor: pureFrame
{
    virtual void xr_stdcall OnFrame()
    {
        //Msg ("------------- sound: %d [%3.2f,%3.2f,%3.2f]",u32(Device.dwFrame),VPUSH(Device.vCameraPosition));
        Device.Statistic->Sound.Begin();
        ::Sound->update(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);
        Device.Statistic->Sound.End();
    }
} SoundProcessor;

void InitEngine()
{
    Engine.Initialize();
    while (!g_bIntroFinished) Sleep(100);
    Device.Initialize();
}

template <typename T>
void InitConfig(T& config, pcstr name, bool fatal = true,
	bool readOnly = true, bool loadAtStart = true, bool saveAtEnd = true,
	u32 sectCount = 0, const CInifile::allow_include_func_t& allowIncludeFunc = nullptr)
{
	string_path fname;
	FS.update_path(fname, "$game_config$", name);
	config = new CInifile(fname, readOnly, loadAtStart, saveAtEnd, sectCount, allowIncludeFunc);

	CHECK_OR_EXIT(config->section_count() || !fatal,
		make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));
}

void InitSettings()
{
	InitConfig(pSettings, "system.ltx");
	InitConfig(pTSWSettings, "shadowcast_config.ltx", false, true, true, false);
	InitConfig(pGameIni, "game.ltx");

    LPCSTR gameMode = READ_IF_EXISTS(pTSWSettings, r_string, "compatibility", "game_mode", "cop");

    if (!xr_strcmp(gameMode, "tsw"))
    {
        TheShadowWayMode = true;
        return;
    }

    if (!xr_strcmp(gameMode, "cs"))
    {
        ClearSkyMode = true;
        CallOfPripyatMode = false;
        return;
    }

    if (!xr_strcmp(gameMode, "soc"))
    {
        ShadowOfChernobylMode = true;
        CallOfPripyatMode = false;
    }
}
void InitConsole()
{
    Console = xr_new<CConsole>();
    
    Console->Initialize();

    xr_strcpy(Console->ConfigFile, "user.ltx");
    if (strstr(Core.Params, "-ltx "))
    {
        string64 c_name;
        sscanf(strstr(Core.Params, "-ltx ") + 5, "%[^ ] ", c_name);
        xr_strcpy(Console->ConfigFile, c_name);
    }
}

void InitInput()
{
    BOOL bCaptureInput = !strstr(Core.Params, "-i");

    pInput = xr_new<CInput>(bCaptureInput);
}
void destroyInput()
{
    xr_delete(pInput);
}

void InitSound1()
{
    CSound_manager_interface::_create(0);
}

void InitSound2()
{
    CSound_manager_interface::_create(1);
}

void destroySound()
{
    CSound_manager_interface::_destroy();
}

void destroySettings()
{
    auto Settings = const_cast<CInifile**>(&pSettings);
    xr_delete(*Settings);
    
    auto SettingsMode_ = const_cast<CInifile**>(&pTSWSettings);
    xr_delete(*SettingsMode_);

    xr_delete(pGameIni);
}

void destroyConsole()
{
    Console->Execute("cfg_save");
    Console->Destroy();
    xr_delete(Console);
}

void destroyEngine()
{
    Device.Destroy();
    Engine.Destroy();
}

void execUserScript()
{
    if (!ShadowOfChernobylMode)
    {
        Console->Execute("default_controls");
    }
    else 
    {
        Console->Execute("unbindall");
    }
    Console->ExecuteScript(Console->ConfigFile);
}

void Startup()
{
    InitSound1();
    execUserScript();
    InitSound2();

    // ...command line for auto start
    {
        LPCSTR pStartup = strstr(Core.Params, "-start ");
        if (pStartup) Console->Execute(pStartup + 1);
    }
    {
        LPCSTR pStartup = strstr(Core.Params, "-load ");
        if (pStartup) Console->Execute(pStartup + 1);
    }

    // Initialize APP
    //#ifndef DEDICATED_SERVER
    ShowWindow(Device.m_hWnd, SW_SHOWNORMAL);

    g_FontManager = new CFontManager();
    Device.Create();
    g_FontManager->InitializeFonts();

    //#endif
    LALib.OnCreate();
    pApp = xr_new<CApplication>();
    g_pGamePersistent = static_cast<IGame_Persistent*>(NEW_INSTANCE(CLSID_GAME_PERSISTANT));
    g_SpatialSpace = xr_new<ISpatial_DB>();
    g_SpatialSpacePhysic = xr_new<ISpatial_DB>();

    // Destroy LOGO
    DestroyWindow(logoWindow);
    logoWindow = NULL;

    Discord.InitSDK();

    // Main cycle
    Memory.mem_usage();
    Device.Run();

    // Destroy APP
    xr_delete(g_SpatialSpacePhysic);
    xr_delete(g_SpatialSpace);
    DEL_INSTANCE(g_pGamePersistent)
    xr_delete(pApp);
    Engine.Event.Dump();

    // Destroying
    destroyInput();

    if (!g_bBenchmark && !g_SASH.IsRunning())
        destroySettings();

    LALib.OnDestroy();

	destroyConsole();

    destroySound();

    destroyEngine();
}

IStream* CreateStreamOnResource(LPCTSTR lpName, LPCTSTR lpType)
{
    IStream* ipStream = NULL;

    HRSRC hrsrc = FindResource(NULL, lpName, lpType);
    if (hrsrc == NULL)
        goto Return;

    DWORD dwResourceSize = SizeofResource(NULL, hrsrc);
    HGLOBAL hglbImage = LoadResource(NULL, hrsrc);
    if (hglbImage == NULL)
        goto Return;

    LPVOID pvSourceResourceData = LockResource(hglbImage);
    if (pvSourceResourceData == NULL)
        goto Return;

    HGLOBAL hgblResourceData = GlobalAlloc(GMEM_MOVEABLE, dwResourceSize);
    if (hgblResourceData == NULL)
        goto Return;

    LPVOID pvResourceData = GlobalLock(hgblResourceData);

    if (pvResourceData == NULL)
        goto FreeData;

    CopyMemory(pvResourceData, pvSourceResourceData, dwResourceSize);

    GlobalUnlock(hgblResourceData);

    if (SUCCEEDED(CreateStreamOnHGlobal(hgblResourceData, TRUE, &ipStream)))
        goto Return;

FreeData:
    GlobalFree(hgblResourceData);

Return:
    return ipStream;
}

void RegisterWindowClass(HINSTANCE hInst)
{
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = c_szSplashClass;
    RegisterClass(&wc);
}

HWND CreateSplashWindow(HINSTANCE hInst)
{
    HWND hwndOwner = CreateWindow(c_szSplashClass, NULL, WS_POPUP,
        0, 0, 0, 0, NULL, NULL, hInst, NULL);
    return CreateWindowEx(WS_EX_LAYERED, c_szSplashClass, NULL, WS_POPUP | WS_VISIBLE,
        0, 0, 0, 0, hwndOwner, NULL, hInst, NULL);
}

HWND WINAPI ShowSplash(HINSTANCE hInstance, int nCmdShow)
{
    MSG msg;
    HWND hWnd;

    //image
    CImage img;                             //объект изображения

    WCHAR path[MAX_PATH];

    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring ws(path);

    std::string splash_path(ws.begin(), ws.end());
    splash_path = splash_path.erase(splash_path.find_last_of('\\'), splash_path.size() - 1);
    splash_path += "\\splash.png";

    img.Load(splash_path.c_str());              //загрузка сплеша

    int splashWidth = img.GetWidth();            //фиксируем ширину картинки
    int splashHeight = img.GetHeight();            //фиксируем высоту картинки

    if (splashWidth == 0 || splashHeight == 0)  //если картинки нет на диске, то грузим из ресурсов
    {
        img.Destroy();
        img.Load(CreateStreamOnResource(MAKEINTRESOURCE(IDB_PNG1), _T("PNG")));//загружаем сплеш
        splashWidth = img.GetWidth();
        splashHeight = img.GetHeight();
    }

    //float temp_x_size = 860.f;
    //float temp_y_size = 461.f;
    int scr_x = GetSystemMetrics(SM_CXSCREEN);
    int scr_y = GetSystemMetrics(SM_CYSCREEN);

    int pos_x = (scr_x / 2) - (splashWidth / 2);
    int pos_y = (scr_y / 2) - (splashHeight / 2);

    //if (!RegClass(SplashProc, szClass, COLOR_WINDOW)) return FALSE;
    hWnd = CreateSplashWindow(hInstance);

    if (!hWnd) return FALSE;

    HDC hdcScreen = GetDC(NULL);
    HDC hDC = CreateCompatibleDC(hdcScreen);

    HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, splashWidth, splashHeight);
    HBITMAP hBmpOld = static_cast<HBITMAP>(SelectObject(hDC, hBmp));

    BLENDFUNCTION blend = { 0 };
    blend.SourceConstantAlpha = 255;

    //рисуем картиночку
    if (img.GetBPP() == 32)
    {
        blend.AlphaFormat = AC_SRC_ALPHA;
        blend.BlendOp = AC_SRC_OVER;

        for (int i = 0; i < img.GetWidth(); i++)
        {
            for (int j = 0; j < img.GetHeight(); j++)
            {
                auto ptr = static_cast<BYTE*>(img.GetPixelAddress(i, j));
                ptr[0] = ((ptr[0] * ptr[3]) + 127) / 255;
                ptr[1] = ((ptr[1] * ptr[3]) + 127) / 255;
                ptr[2] = ((ptr[2] * ptr[3]) + 127) / 255;
            }
        }
    }

    img.AlphaBlend(hDC, 0, 0, splashWidth, splashHeight, 0, 0, splashWidth, splashHeight);

    POINT ptPos = { 0, 0 };
    SIZE sizeWnd = { splashWidth, splashHeight };
    POINT ptSrc = { 0, 0 };
    HWND hDT = GetDesktopWindow();

    if (hDT)
    {
        RECT rcDT;
        GetWindowRect(hDT, &rcDT);
        ptPos.x = (rcDT.right - splashWidth) / 2;
        ptPos.y = (rcDT.bottom - splashHeight) / 2;
    }

    UpdateLayeredWindow(hWnd, hdcScreen, &ptPos, &sizeWnd, hDC, &ptSrc, 0, &blend, ULW_ALPHA);

    return hWnd;
}

void SetSplashImage(HWND hwndSplash, HBITMAP hbmpSplash)
{
    // get the size of the bitmap

    BITMAP bm;
    GetObject(hbmpSplash, sizeof(bm), &bm);
    SIZE sizeSplash = { bm.bmWidth, bm.bmHeight };

    // get the primary monitor's info

    POINT ptZero = { 0 };
    HMONITOR hmonPrimary = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorinfo = { 0 };
    monitorinfo.cbSize = sizeof(monitorinfo);
    GetMonitorInfo(hmonPrimary, &monitorinfo);

    // center the splash screen in the middle of the primary work area

    const RECT& rcWork = monitorinfo.rcWork;
    POINT ptOrigin;
    ptOrigin.x = rcWork.left + (rcWork.right - rcWork.left - sizeSplash.cx) / 2;
    ptOrigin.y = rcWork.top + (rcWork.bottom - rcWork.top - sizeSplash.cy) / 2;

    // create a memory DC holding the splash bitmap

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmpOld = static_cast<HBITMAP>(SelectObject(hdcMem, hbmpSplash));

    // use the source image's alpha channel for blending

    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    // paint the window (in the right location) with the alpha-blended bitmap

    UpdateLayeredWindow(hwndSplash, hdcScreen, &ptOrigin, &sizeSplash,
        hdcMem, &ptZero, RGB(0, 0, 0), &blend, ULW_ALPHA);

    // delete temporary objects

    SelectObject(hdcMem, hbmpOld);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

#define dwStickyKeysStructSize sizeof( STICKYKEYS )
#define dwFilterKeysStructSize sizeof( FILTERKEYS )
#define dwToggleKeysStructSize sizeof( TOGGLEKEYS )

struct damn_keys_filter
{
    BOOL bScreenSaverState;

    // Sticky & Filter & Toggle keys

    STICKYKEYS StickyKeysStruct{};
    FILTERKEYS FilterKeysStruct{};
    TOGGLEKEYS ToggleKeysStruct{};

    DWORD dwStickyKeysFlags;
    DWORD dwFilterKeysFlags;
    DWORD dwToggleKeysFlags;

    damn_keys_filter()
    {
        // Screen saver stuff

        bScreenSaverState = FALSE;

        // Saveing current state
        SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, (PVOID)&bScreenSaverState, 0);

        if (bScreenSaverState)
            // Disable screensaver
            SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, NULL, 0);

        dwStickyKeysFlags = 0;
        dwFilterKeysFlags = 0;
        dwToggleKeysFlags = 0;


        ZeroMemory(&StickyKeysStruct, dwStickyKeysStructSize);
        ZeroMemory(&FilterKeysStruct, dwFilterKeysStructSize);
        ZeroMemory(&ToggleKeysStruct, dwToggleKeysStructSize);

        StickyKeysStruct.cbSize = dwStickyKeysStructSize;
        FilterKeysStruct.cbSize = dwFilterKeysStructSize;
        ToggleKeysStruct.cbSize = dwToggleKeysStructSize;

        // Saving current state
        SystemParametersInfo(SPI_GETSTICKYKEYS, dwStickyKeysStructSize, (PVOID)&StickyKeysStruct, 0);
        SystemParametersInfo(SPI_GETFILTERKEYS, dwFilterKeysStructSize, (PVOID)&FilterKeysStruct, 0);
        SystemParametersInfo(SPI_GETTOGGLEKEYS, dwToggleKeysStructSize, (PVOID)&ToggleKeysStruct, 0);

        if (StickyKeysStruct.dwFlags & SKF_AVAILABLE)
        {
            // Disable StickyKeys feature
            dwStickyKeysFlags = StickyKeysStruct.dwFlags;
            StickyKeysStruct.dwFlags = 0;
            SystemParametersInfo(SPI_SETSTICKYKEYS, dwStickyKeysStructSize, (PVOID)&StickyKeysStruct, 0);
        }

        if (FilterKeysStruct.dwFlags & FKF_AVAILABLE)
        {
            // Disable FilterKeys feature
            dwFilterKeysFlags = FilterKeysStruct.dwFlags;
            FilterKeysStruct.dwFlags = 0;
            SystemParametersInfo(SPI_SETFILTERKEYS, dwFilterKeysStructSize, (PVOID)&FilterKeysStruct, 0);
        }

        if (ToggleKeysStruct.dwFlags & TKF_AVAILABLE)
        {
            // Disable FilterKeys feature
            dwToggleKeysFlags = ToggleKeysStruct.dwFlags;
            ToggleKeysStruct.dwFlags = 0;
            SystemParametersInfo(SPI_SETTOGGLEKEYS, dwToggleKeysStructSize, (PVOID)&ToggleKeysStruct, 0);
        }
    }

    ~damn_keys_filter()
    {
        if (bScreenSaverState)
            // Restoring screen saver
            SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, TRUE, NULL, 0);

        if (dwStickyKeysFlags)
        {
            // Restore StickyKeys feature
            StickyKeysStruct.dwFlags = dwStickyKeysFlags;
            SystemParametersInfo(SPI_SETSTICKYKEYS, dwStickyKeysStructSize, (PVOID)&StickyKeysStruct, 0);
        }

        if (dwFilterKeysFlags)
        {
            // Restore FilterKeys feature
            FilterKeysStruct.dwFlags = dwFilterKeysFlags;
            SystemParametersInfo(SPI_SETFILTERKEYS, dwFilterKeysStructSize, (PVOID)&FilterKeysStruct, 0);
        }

        if (dwToggleKeysFlags)
        {
            // Restore FilterKeys feature
            ToggleKeysStruct.dwFlags = dwToggleKeysFlags;
            SystemParametersInfo(SPI_SETTOGGLEKEYS, dwToggleKeysStructSize, (PVOID)&ToggleKeysStruct, 0);
        }

    }
};

#undef dwStickyKeysStructSize
#undef dwFilterKeysStructSize
#undef dwToggleKeysStructSize

#include "xr_ioc_cmd.h"

ENGINE_API bool g_dedicated_server = false;

#ifndef DEDICATED_SERVER
// forward declaration for Parental Control checks
BOOL IsPCAccessAllowed();
#endif // DEDICATED_SERVER

int APIENTRY WinMain_impl(HINSTANCE hInstance, HINSTANCE, char* lpCmdLine, int nCmdShow)
{
    Debug._initialize(false);

    // Check for another instance
#ifdef NO_MULTI_INSTANCES
#define STALKER_PRESENCE_MUTEX "Local\\STALKER-COP"

    HANDLE hCheckPresenceMutex;
    hCheckPresenceMutex = OpenMutex(READ_CONTROL, FALSE, STALKER_PRESENCE_MUTEX);
    if (hCheckPresenceMutex == NULL)
    {
        // New mutex
        hCheckPresenceMutex = CreateMutex(NULL, FALSE, STALKER_PRESENCE_MUTEX);
        if (hCheckPresenceMutex == NULL)
            // Shit happens
            return 2;
    }
    else
    {
        // Already running
        CloseHandle(hCheckPresenceMutex);
        return 1;
    }
#endif

    RegisterWindowClass(hInstance);
    //logoWindow = CreateSplashWindow(hInstance);
    logoWindow = ShowSplash(hInstance, nCmdShow);

    SendMessage(logoWindow, WM_DESTROY, 0, 0);

    // AVI
    g_bIntroFinished = TRUE;

    g_sLaunchOnExit_app[0] = NULL;
    g_sLaunchOnExit_params[0] = NULL;

    LPCSTR fsgame_ltx_name = "-fsltx ";
    string_path fsgame = "";
    //MessageBox(0, lpCmdLine, "my cmd string", MB_OK);
    if (strstr(lpCmdLine, fsgame_ltx_name))
    {
        int sz = xr_strlen(fsgame_ltx_name);
        sscanf(strstr(lpCmdLine, fsgame_ltx_name) + sz, "%[^ ] ", fsgame);
        //MessageBox(0, fsgame, "using fsltx", MB_OK);
    }

    // g_temporary_stuff = &trivial_encryptor::decode;

    compute_build_id();
    Core._initialize("XRay", nullptr, TRUE, fsgame[0] ? fsgame : nullptr);

    InitSettings();

    // Adjust player & computer name for Asian
    if (pSettings->line_exist("string_table", "no_native_input"))
    {
        xr_strcpy(Core.UserName, sizeof(Core.UserName), "Player");
        xr_strcpy(Core.CompName, sizeof(Core.CompName), "Computer");
    }

    {
        damn_keys_filter filter;
        (void)filter;

        InitEngine();

        InitInput();

        InitConsole();

        Engine.CreatingRenderList(); 

        LPCSTR benchName = "-batch_benchmark ";
        if (strstr(lpCmdLine, benchName))
        {
            int sz = xr_strlen(benchName);
            string64 b_name;
            sscanf(strstr(Core.Params, benchName) + sz, "%[^ ] ", b_name);
            doBenchmark(b_name);
            return 0;
        }

        Msg("command line %s", lpCmdLine);
        LPCSTR sashName = "-openautomate ";
        if (strstr(lpCmdLine, sashName))
        {
            int sz = xr_strlen(sashName);
            string512 sash_arg;
            sscanf(strstr(Core.Params, sashName) + sz, "%[^ ] ", sash_arg);
            //doBenchmark (sash_arg);
            g_SASH.Init(sash_arg);
            g_SASH.MainLoop();
            return 0;
        }

        if (strstr(Core.Params, "-r2a"))
            Console->Execute("renderer renderer_r2a");
        else if (strstr(Core.Params, "-r2"))
            Console->Execute("renderer renderer_r2");
        else
        {
            CCC_LoadCFG_custom* pTmp = xr_new<CCC_LoadCFG_custom>("renderer ");
            pTmp->Execute(Console->ConfigFile);
            xr_delete(pTmp);
        }

        Engine.Initialize_dll();
        Console->Execute("stat_memory");

        Startup();
        Core._destroy();

        // check for need to execute something external
        if (xr_strlen(g_sLaunchOnExit_app))
        {
            //CreateProcess need to return results to next two structures
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));
            //We use CreateProcess to setup working folder
            char const* temp_wf = (xr_strlen(g_sLaunchWorkingFolder) > 0) ? g_sLaunchWorkingFolder : NULL;
            CreateProcess(g_sLaunchOnExit_app, g_sLaunchOnExit_params, NULL, NULL, FALSE, 0, NULL,
                          temp_wf, &si, &pi);

        }
#ifdef NO_MULTI_INSTANCES
        // Delete application presence mutex
        CloseHandle(hCheckPresenceMutex);
#endif
    }
    // here damn_keys_filter class instanse will be destroyed

    return 0;
}

int stack_overflow_exception_filter(int exception_code)
{
    if (exception_code == EXCEPTION_STACK_OVERFLOW)
    {
        // Do not call _resetstkoflw here, because
        // at this point, the stack is not yet unwound.
        // Instead, signal that the handler (the __except block)
        // is to be executed.
        return EXCEPTION_EXECUTE_HANDLER;
    }
    else
        return EXCEPTION_CONTINUE_SEARCH;
}

#include <boost/crc.hpp>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow)
{
    __try
    {
        WinMain_impl(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    }
    __except (stack_overflow_exception_filter(GetExceptionCode()))
    {
        _resetstkoflw();
        FATAL("stack overflow");
    }

    return 0;
}

CApplication::CApplication()
{
    ll_dwReference = 0;

    max_load_stage = 0;

    // events
    eQuit = Engine.Event.Handler_Attach("KERNEL:quit", this);
    eStart = Engine.Event.Handler_Attach("KERNEL:start", this);
    eStartLoad = Engine.Event.Handler_Attach("KERNEL:load", this);
    eDisconnect = Engine.Event.Handler_Attach("KERNEL:disconnect", this);
    eConsole = Engine.Event.Handler_Attach("KERNEL:console", this);

    // levels
    Level_Current = static_cast<u32>(-1);
    Level_Scan();

    // Register us
    Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 1000);

    if (psDeviceFlags.test(mtSound)) Device.seqFrameMT.Add(&SoundProcessor);
    else Device.seqFrame.Add(&SoundProcessor);

    Console->Show();

    // App Title
    loadingScreen = nullptr;
}

CApplication::~CApplication()
{
    Console->Hide();

    Device.seqFrameMT.Remove(&SoundProcessor);
    Device.seqFrame.Remove(&SoundProcessor);
    Device.seqFrame.Remove(this);


    // events
    Engine.Event.Handler_Detach(eConsole, this);
    Engine.Event.Handler_Detach(eDisconnect, this);
    Engine.Event.Handler_Detach(eStartLoad, this);
    Engine.Event.Handler_Detach(eStart, this);
    Engine.Event.Handler_Detach(eQuit, this);
}

void CApplication::OnEvent(EVENT E, u64 P1, u64 P2)
{
    if (E == eQuit)
    {
        g_SASH.EndBenchmark();

        PostQuitMessage(0);

        for (u32 i = 0; i < Levels.size(); i++)
        {
            xr_free(Levels[i].folder);
            xr_free(Levels[i].name);
        }
    }
    else if (E == eStart)
    {
        auto op_server = reinterpret_cast<LPSTR>(P1);
        auto op_client = reinterpret_cast<LPSTR>(P2);
        Level_Current = static_cast<u32>(-1);
        R_ASSERT(nullptr == g_pGameLevel);
        R_ASSERT(nullptr != g_pGamePersistent);

        {
            Console->Execute("main_menu off");
            Console->Hide();
            //! this line is commented by Dima
            //! because I don't see any reason to reset device here
            //! Device.Reset (false);
            //-----------------------------------------------------------
            g_pGamePersistent->PreStart(op_server);
            //-----------------------------------------------------------
            g_pGameLevel = static_cast<IGame_Level*>(NEW_INSTANCE(CLSID_GAME_LEVEL));
            pApp->LoadBegin();
            g_pGamePersistent->Start(op_server);
            g_pGameLevel->net_Start(op_server, op_client);
            pApp->LoadEnd();
        }
        xr_free(op_server);
        xr_free(op_client);
    }
    else if (E == eDisconnect)
    {

        if (g_pGameLevel)
        {
            Console->Hide();
            g_pGameLevel->net_Stop();
            DEL_INSTANCE(g_pGameLevel)

            if ((FALSE == Engine.Event.Peek("KERNEL:quit")) && (FALSE == Engine.Event.Peek("KERNEL:start")))
            {
                Console->Execute("main_menu off");
                Console->Execute("main_menu on");
            }
        }
        R_ASSERT(0 != g_pGamePersistent);
        g_pGamePersistent->Disconnect();
    }
    else if (E == eConsole)
    {
        auto command = reinterpret_cast<LPSTR>(P1);
        Console->ExecuteCommand(command, false);
        xr_free(command);
    }
}

static CTimer phase_timer;
ENGINE_API BOOL g_appLoaded = FALSE;

void CApplication::LoadBegin()
{
    ll_dwReference++;
    if (1 == ll_dwReference)
    {
        g_appLoaded = FALSE;

        phase_timer.Start();
        load_stage = 0;
    }
}

void CApplication::LoadEnd()
{
    ll_dwReference--;
    if (0 == ll_dwReference)
    {
        Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
		Msg("* phase cmem: %lld K", Memory.mem_usage() / 1024);
        Console->Execute("stat_memory");
        g_appLoaded = TRUE;
        // DUMP_PHASE;
    }
}

void CApplication::SetLoadingScreen(ILoadingScreen* newScreen)
{
    if (loadingScreen)
    {
        Log("! Trying to create new loading screen, but there is already one..");
        //__debugbreak();
        DestroyLoadingScreen();
    }

    loadingScreen = newScreen;
}

void CApplication::DestroyLoadingScreen()
{
    xr_delete(loadingScreen);
}

LPCSTR CApplication::ShowCurrentVersion()
{
    return "1.7.00";
}

#include "Render.h"

void CApplication::LoadDraw()
{
    if (g_appLoaded) return;

    Device.dwFrame += 1;

	Render->firstViewPort = MAIN_VIEWPORT;
	Render->lastViewPort = MAIN_VIEWPORT;
	Render->currentViewPort = MAIN_VIEWPORT;
	Render->needPresenting = true;

    if (!Device.Begin()) return;

    if (g_dedicated_server)
        Console->OnRender();
    else
        load_draw_internal();

    Device.End();
}

void CApplication::LoadForceFinish()
{
    if (loadingScreen)
        loadingScreen->ForceFinish();
}

void CApplication::SetLoadStageTitle(pcstr _ls_title)
{
    if (ps_rs_loading_stages && loadingScreen)
        loadingScreen->SetStageTitle(_ls_title);
    Log(_ls_title);
}

void CApplication::LoadTitleInt(LPCSTR str1, LPCSTR str2, LPCSTR str3)
{
    if (loadingScreen)
        loadingScreen->SetStageTip(str1, str2, str3);
}

void CApplication::LoadStage()
{
    VERIFY(ll_dwReference);
    Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
    phase_timer.Start();
	Msg("* phase cmem: %lld K", Memory.mem_usage() / 1024);

    if (g_pGamePersistent->GameType() == 1 && !xr_strcmp(g_pGamePersistent->m_game_params.m_alife, "alife"))
        max_load_stage = 17;
    else
        max_load_stage = 14;
    LoadDraw();
    ++load_stage;
}

void CApplication::LoadSwitch()
{
}

// Sequential
void CApplication::OnFrame()
{
    Engine.Event.OnFrame();
    g_SpatialSpace->update();
    g_SpatialSpacePhysic->update();
    if (g_pGameLevel)
        g_pGameLevel->SoundEvent_Dispatch();
}

void CApplication::Level_Append(LPCSTR folder)
{
    string_path N1, N2, N3, N4;
    strconcat(sizeof(N1), N1, folder, "level");
    strconcat(sizeof(N2), N2, folder, "level.ltx");
    strconcat(sizeof(N3), N3, folder, "level.geom");
    strconcat(sizeof(N4), N4, folder, "level.cform");
    if (
        FS.exist("$game_levels$", N1) &&
        FS.exist("$game_levels$", N2) &&
        FS.exist("$game_levels$", N3) &&
        FS.exist("$game_levels$", N4)
    )
    {
        sLevelInfo LI{};
        LI.folder = xr_strdup(folder);
        LI.name = 0;
        Levels.push_back(LI);
    }
}

void CApplication::Level_Scan()
{
    for (auto& Level : Levels)
    {
        xr_free(Level.folder);
        xr_free(Level.name);
    }
    Levels.clear();


    xr_vector<char*>* folder = FS.file_list_open("$game_levels$", FS_ListFolders | FS_RootOnly);
    //. R_ASSERT (folder&&folder->size());

    for (u32 i = 0; i < folder->size(); ++i)
        Level_Append((*folder)[i]);

    FS.file_list_close(folder);
}

void gen_logo_name(string_path& dest, LPCSTR level_name, int num = -1)
{
    strconcat(sizeof(dest), dest, "intro\\intro_", level_name);

    u32 len = xr_strlen(dest);
    if (dest[len - 1] == '\\')
        dest[len - 1] = 0;

    if (num < 0)
        return;

    string16 buff;
    xr_strcat(dest, sizeof(dest), "_");
    xr_strcat(dest, sizeof(dest), itoa(num + 1, buff, 10));
}

// Return true if logo exists
// Always sets the path even if logo doesn't exist
bool set_logo_path(string_path& path, pcstr levelName, int count = -1)
{
    gen_logo_name(path, levelName, count);
    string_path temp2;
    return FS.exist(temp2, "$game_textures$", path, ".dds") || FS.exist(temp2, "$level$", path, ".dds");
}

void CApplication::Level_Set(u32 L)
{
    if (L >= Levels.size())
        return;
    FS.get_path("$level$")->_set(Levels[L].folder);
    Level_Current = L;

    static string_path path;
    path[0] = 0;

    int count = 0;
    while (true)
    {
        if (set_logo_path(path, Levels[L].folder, count))
            count++;
        else
            break;
    }

    if (count)
    {
        const int num = ::Random.randI(count);
        gen_logo_name(path, Levels[L].folder, num);
    }
    else if (!set_logo_path(path, Levels[L].folder))
    {
        if (!set_logo_path(path, "no_start_picture"))
            path[0] = 0;
    }

    if (path[0] && loadingScreen)
        loadingScreen->SetLevelLogo(path);
}

int CApplication::Level_ID(LPCSTR name, LPCSTR ver, bool bSet)
{
    int result = -1;

    CLocatorAPI::archives_it it = FS.m_archives.begin();
    CLocatorAPI::archives_it it_e = FS.m_archives.end();
    bool arch_res = false;

    for (; it != it_e; ++it)
    {
        CLocatorAPI::archive& A = *it;
        if (A.hSrcFile == NULL)
        {
            LPCSTR ln = A.header->r_string("header", "level_name");
            LPCSTR lv = A.header->r_string("header", "level_ver");
            if (0 == stricmp(ln, name) && 0 == stricmp(lv, ver))
            {
                FS.LoadArchive(A);
                arch_res = true;
            }
        }
    }

    if (arch_res)
        Level_Scan();

    string256 buffer;
    strconcat(sizeof(buffer), buffer, name, "\\");
    for (u32 I = 0; I < Levels.size(); ++I)
    {
        if (0 == stricmp(buffer, Levels[I].folder))
        {
            result = static_cast<int>(I);
            break;
        }
    }

    if (bSet && result != -1)
        Level_Set(result);

    if (arch_res)
        g_pGamePersistent->OnAssetsChanged();

    return result;
}

CInifile* CApplication::GetArchiveHeader(LPCSTR name, LPCSTR ver)
{
	auto it = FS.m_archives.begin();
	auto it_e = FS.m_archives.end();

    for (; it != it_e; ++it)
    {
        CLocatorAPI::archive& A = *it;

        LPCSTR ln = A.header->r_string("header", "level_name");
        LPCSTR lv = A.header->r_string("header", "level_ver");
        if (0 == stricmp(ln, name) && 0 == stricmp(lv, ver))
        {
            return A.header;
        }
    }
    return nullptr;
}

void CApplication::LoadAllArchives()
{
    if (FS.load_all_unloaded_archives())
    {
        Level_Scan();
        g_pGamePersistent->OnAssetsChanged();
    }
}

#ifndef DEDICATED_SERVER
// Parential control for Vista and upper
typedef BOOL(*PCCPROC)(CHAR*);

BOOL IsPCAccessAllowed()
{
    CHAR szPCtrlChk[MAX_PATH], szGDF[MAX_PATH], *pszLastSlash;
    HINSTANCE hPCtrlChk = NULL;
    PCCPROC pctrlchk = NULL;
    BOOL bAllowed = TRUE;

    if (!GetModuleFileName(NULL, szPCtrlChk, MAX_PATH))
        return TRUE;

    if ((pszLastSlash = strrchr(szPCtrlChk, '\\')) == NULL)
        return TRUE;

    *pszLastSlash = '\0';

    strcpy_s(szGDF, szPCtrlChk);

    strcat_s(szPCtrlChk, "\\pctrlchk.dll");
    if (GetFileAttributes(szPCtrlChk) == INVALID_FILE_ATTRIBUTES)
        return TRUE;

    if ((pszLastSlash = strrchr(szGDF, '\\')) == NULL)
        return TRUE;

    *pszLastSlash = '\0';

    strcat_s(szGDF, "\\Stalker-COP.exe");
    if (GetFileAttributes(szGDF) == INVALID_FILE_ATTRIBUTES)
        return TRUE;

    if ((hPCtrlChk = LoadLibrary(szPCtrlChk)) == NULL)
        return TRUE;

    if ((pctrlchk = (PCCPROC)GetProcAddress(hPCtrlChk, "pctrlchk")) == NULL)
    {
        FreeLibrary(hPCtrlChk);
        return TRUE;
    }

    bAllowed = pctrlchk(szGDF);

    FreeLibrary(hPCtrlChk);

    return bAllowed;
}
#endif // DEDICATED_SERVER

//launcher stuff----------------------------
extern "C" {
    typedef int __cdecl LauncherFunc(int);
}
HMODULE hLauncher = NULL;
LauncherFunc* pLauncher = NULL;

void InitLauncher()
{
    if (hLauncher)
        return;
    hLauncher = LoadLibrary("xrLauncher.dll");
    if (0 == hLauncher) R_CHK(GetLastError());
    R_ASSERT2(hLauncher, "xrLauncher DLL raised exception during loading or there is no xrLauncher.dll at all");

    pLauncher = (LauncherFunc*)GetProcAddress(hLauncher, "RunXRLauncher");
    R_ASSERT2(pLauncher, "Cannot obtain RunXRLauncher function from xrLauncher.dll");
};

void FreeLauncher()
{
    if (hLauncher)
    {
        FreeLibrary(hLauncher);
        hLauncher = NULL;
        pLauncher = NULL;
    };
}

int doLauncher()
{
    /*
    execUserScript();
    InitLauncher();
    int res = pLauncher(0);
    FreeLauncher();
    if(res == 1) // do benchmark
    g_bBenchmark = true;

    if(g_bBenchmark){ //perform benchmark cycle
    doBenchmark();

    // InitLauncher ();
    // pLauncher (2); //show results
    // FreeLauncher ();

    Core._destroy ();
    return (1);

    };
    if(res==8){//Quit
    Core._destroy ();
    return (1);
    }
    */
    return 0;
}

void doBenchmark(LPCSTR name)
{
    g_bBenchmark = true;
    string_path in_file;
    FS.update_path(in_file, "$app_data_root$", name);
    CInifile ini(in_file);
    int test_count = ini.line_count("benchmark");
    LPCSTR test_name, t;
    shared_str test_command;
    for (int i = 0; i < test_count; ++i)
    {
        ini.r_line("benchmark", i, &test_name, &t);
        xr_strcpy(g_sBenchmarkName, test_name);

        test_command = ini.r_string_wb("benchmark", test_name);
        u32 cmdSize = test_command.size() + 1;
        Core.Params = static_cast<char*>(xr_realloc(Core.Params, cmdSize));
        xr_strcpy(Core.Params, cmdSize, test_command.c_str());
        xr_strlwr(Core.Params);

        InitInput();
        if (i)
        {
            //ZeroMemory(&HW,sizeof(CHW));
            // TODO: KILL HW here!
            // pApp->m_pRender->KillHW();
            InitEngine();
        }


        Engine.Initialize_dll();

        xr_strcpy(Console->ConfigFile, "user.ltx");
        if (strstr(Core.Params, "-ltx "))
        {
            string64 c_name;
            sscanf(strstr(Core.Params, "-ltx ") + 5, "%[^ ] ", c_name);
            xr_strcpy(Console->ConfigFile, c_name);
        }

        Startup();
    }
}
#pragma optimize("", off)
void CApplication::load_draw_internal()
{
    if (loadingScreen)
        loadingScreen->Update(load_stage, max_load_stage);
}