#ifndef xr_device
#define xr_device
#pragma once

// Note:
// ZNear - always 0.0f
// ZFar - always 1.0f

//class ENGINE_API CResourceManager;
//class ENGINE_API CGammaControl;

#include "pure.h"
//#include "hw.h"
#include "../xrcore/ftimer.h"
#include "stats.h"
//#include "shader.h"
//#include "R_Backend.h"

extern ENGINE_API float VIEWPORT_NEAR;
#define VIEWPORT_NEAR_HUD 0.01f
#ifndef _EDITOR
extern ENGINE_API int psSVPFrameDelay;
#endif
enum ViewPort;

//#define VIEWPORT_NEAR 0.05f //--#SM+#-- (Old: 0.2f)

#define DEVICE_RESET_PRECACHE_FRAME_COUNT 10

#include "../Include/xrRender/FactoryPtr.h"
#include "../Include/xrRender/RenderDeviceRender.h"
#include "../xrCore/Threading/Event.hpp"

class engine_impl;

#pragma pack(push,8)

class IRenderDevice
{
public:
    virtual CStatsPhysics* _BCL StatPhysics() = 0;
    virtual void _BCL AddSeqFrame(pureFrame* f, bool mt) = 0;
    virtual void _BCL RemoveSeqFrame(pureFrame* f) = 0;
};

class ENGINE_API CRenderDeviceData
{

public:
    bool	dwMaximized;
    u32     dwWidth;
    u32     dwHeight;

    u32     dwPrecacheFrame;
    bool    b_is_Ready;
    bool    b_is_Active;
public:

    // Engine flow-control
    u32 dwFrame;

    float fTimeDelta;
    float fTimeGlobal;
    u32 dwTimeDelta;
    u32 dwTimeGlobal;
    u32 dwTimeContinual;

    Fvector vCameraPosition;
    Fvector vCameraDirection;
    Fvector vCameraTop;
    Fvector vCameraRight;

    Fmatrix mView;
    Fmatrix mProject;
    Fmatrix mFullTransform;

    // Copies of corresponding members. Used for synchronization.
    Fvector vCameraPosition_saved;

    Fmatrix mView_saved;
    Fmatrix mProject_saved;
    Fmatrix mFullTransform_saved;

    float fFOV;
    float fASPECT;
protected:

    u32 Timer_MM_Delta;
    CTimer_paused Timer;
    CTimer_paused TimerGlobal;
public:

    // Registrators
    CRegistrator <pureRender > seqRender;
    CRegistrator <pureAppActivate > seqAppActivate;
    CRegistrator <pureAppDeactivate > seqAppDeactivate;
    CRegistrator <pureAppStart > seqAppStart;
    CRegistrator <pureAppEnd > seqAppEnd;
    CRegistrator <pureFrame > seqFrame;
    CRegistrator <pureScreenResolutionChanged> seqResolutionChanged;
//#ifdef _EDITOR
    CRegistrator	<pureDrawUI			>			seqDrawUI;
//#endif

    // returns the aproximate, adjusted by camera fov, distance 
    IC float GetDistFromCamera(const Fvector& from_position) const
    {
        float distance = vCameraPosition.distance_to(from_position);
        float fov_K = 67.f / fFOV;
        float adjusted_distane = distance / fov_K;

        return adjusted_distane;
    }

    HWND m_hWnd;
    // CStats* Statistic;

};

class ENGINE_API CRenderDeviceBase :
    public IRenderDevice,
    public CRenderDeviceData
{
public:
};

class ENGINE_API CSecondVPParams //--#SM+#-- +SecondVP+
{
	bool isActive; // Флаг активации рендера во второй вьюпорт
	u8 frameDelay;  // На каком кадре с момента прошлого рендера во второй вьюпорт мы начнём новый
					  //(не может быть меньше 2 - каждый второй кадр, чем больше тем более низкий FPS во втором вьюпорте)

public:
	bool isCamReady; // Флаг готовности камеры (FOV, позиция, и т.п) к рендеру второго вьюпорта

    u32 screenWidth;
    u32 screenHeight;

	bool isR1;
#ifndef _EDITOR
	IC bool IsSVPActive() { return isActive; }
	IC void SetSVPActive(bool bState);
	bool    IsSVPFrame();

	IC u8 GetSVPFrameDelay() { return frameDelay; }
	void  SetSVPFrameDelay(u8 iDelay)
	{
		frameDelay = iDelay;
		clamp<u8>(frameDelay, 1, u8(-1));
	}
#endif
};

#pragma pack(pop)
// refs
class ENGINE_API CRenderDevice : public CRenderDeviceBase
{
private:
    // Main objects used for creating and rendering the 3D scene
    u32 m_dwWindowStyle;
    RECT m_rcWindowBounds;
    RECT m_rcWindowClient;

    //u32 Timer_MM_Delta;
    //CTimer_paused Timer;
    //CTimer_paused TimerGlobal;
    CTimer TimerMM;
    CTimer FreezeTimer;

    void _Create(LPCSTR shName);
    void _Destroy(BOOL bKeepTextures);
    void _SetupStates();
public:
    // HWND m_hWnd;
    LRESULT MsgProc(HWND, UINT, WPARAM, LPARAM);

    // u32 dwFrame;
    // u32 dwPrecacheFrame;
    u32 dwPrecacheTotal;

    // u32 dwWidth, dwHeight;
    float fWidth_2, fHeight_2;
    // BOOL b_is_Ready;
    // BOOL b_is_Active;
    void OnWM_Activate(WPARAM wParam, LPARAM lParam);
public:
    //ref_shader m_WireShader;
    //ref_shader m_SelectionShader;

    IRenderDeviceRender* m_pRender;

    BOOL m_bNearer;
    void SetNearer(BOOL enabled)
    {
        if (enabled&&!m_bNearer)
        {
            m_bNearer = TRUE;
            mProject._43 -= EPS_L;
        }
        else if (!enabled&&m_bNearer)
        {
            m_bNearer = FALSE;
            mProject._43 += EPS_L;
        }
        m_pRender->SetCacheXform(mView, mProject);
        //R_ASSERT(0);
        // TODO: re-implement set projection
        //RCache.set_xform_project (mProject);
    }

    void DumpResourcesMemoryUsage() { m_pRender->ResourcesDumpMemoryUsage(); }
public:
    // Registrators
    CRegistrator <pureFrame > seqFrameMT;
    CRegistrator <pureDeviceReset > seqDeviceReset;
    xr_vector <fastdelegate::FastDelegate0<> > seqParallel;
	CSecondVPParams m_SecondViewport;	//--#SM+#-- +SecondVP+

    // Dependent classes

    CStats* Statistic;

    Fmatrix mInvFullTransform;

    bool m_bMakeLevelMap{};
    Fbox curr_lm_fbox{};

	// Saved main viewport params
	Fvector mainVPCamPosSaved;
	Fmatrix mainVPFullTrans;
	Fmatrix mainVPViewSaved;
	Fmatrix mainVPProjectSaved;
	//Fmatrix	secVPTransShrinked;

    CRenderDevice()
        :
        m_pRender(0)
#ifdef PROFILE_CRITICAL_SECTIONS
        ,mt_csEnter(MUTEX_PROFILE_ID(CRenderDevice::mt_csEnter))
        ,mt_csLeave(MUTEX_PROFILE_ID(CRenderDevice::mt_csLeave))
#endif // #ifdef PROFILE_CRITICAL_SECTIONS
    {
        m_hWnd = NULL;
        b_is_Active = FALSE;
        b_is_Ready = FALSE;
        Timer.Start();
        m_bNearer = FALSE;
		//--#SM+#-- +SecondVP+
#ifndef _EDITOR
		m_SecondViewport.SetSVPActive(false);
		m_SecondViewport.SetSVPFrameDelay(static_cast<u8>(psSVPFrameDelay)); // Change it to 2-3, if you want to save perfomance. Will cause skips in updating image in scope
		m_SecondViewport.isCamReady = false;
		m_SecondViewport.isR1 = false;
#endif
    };

    void Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason);
    BOOL Paused();

private:
        static void SecondaryThreadProc(void* context);
	    static void FreezeThread(void*);
public:
    // Scene control
    void PreCache(u32 amount, bool b_draw_loadscreen, bool b_wait_user_input);
    BOOL Begin();
    void Clear();
    void End();
    void FrameMove();

    bool bMainMenuActive();

    void overdrawBegin();
    void overdrawEnd();

	//Frustum
	//IC Fmatrix GetShrinkedFullTransform_saved() const { return secVPTransShrinked; };

	//void SetShrinkedFullTransform_saved(const Fmatrix& m) { secVPTransShrinked = m; };


    // Mode control
    void DumpFlags();
    IC __unaligned CTimer_paused* GetTimerGlobal() { return &TimerGlobal; }
    u32 TimerAsync() { return TimerGlobal.GetElapsed_ms(); }
    u32 TimerAsync_MMT() { return TimerMM.GetElapsed_ms() + Timer_MM_Delta; }

    // Creation & Destroying
    void ConnectToRender();
    void Create(void);
    void Run(void);
    void Destroy(void);
    void Reset(bool precache = true);

    void Initialize(void);
    void ShutDown(void);

public:
    void time_factor(const float& time_factor)
    {
        Timer.time_factor(time_factor);
        TimerGlobal.time_factor(time_factor);
    }

    IC const float& time_factor() const
    {
        VERIFY(Timer.time_factor() == TimerGlobal.time_factor());
        return (Timer.time_factor());
    }

private:
    Event syncProcessFrame, syncFrameDone, syncThreadExit;
public:
    volatile BOOL mt_bMustExit;

    ICF void remove_from_seq_parallel(const fastdelegate::FastDelegate0<>& delegate)
    {
        xr_vector<fastdelegate::FastDelegate0<> >::iterator I = std::find(
                    seqParallel.begin(),
                    seqParallel.end(),
                    delegate
                );
        if (I != seqParallel.end())
            seqParallel.erase(I);
    }

public:
    void xr_stdcall on_idle();
    bool xr_stdcall on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);

private:
    void message_loop();
    virtual void _BCL AddSeqFrame(pureFrame* f, bool mt);
    virtual void _BCL RemoveSeqFrame(pureFrame* f);
    virtual CStatsPhysics* _BCL StatPhysics() { return Statistic; }
};

extern ENGINE_API CRenderDevice Device;

#ifndef _EDITOR
#define RDEVICE Device
#else
#define RDEVICE EDevice
#endif


extern ENGINE_API bool g_bBenchmark;

typedef fastdelegate::FastDelegate0<bool> LOADING_EVENT;
extern ENGINE_API xr_list<LOADING_EVENT> g_loading_events;

class ENGINE_API CLoadScreenRenderer :public pureRender
{
public:
    CLoadScreenRenderer();
    void start(bool b_user_input);
    void stop();
    virtual void OnRender();

    bool b_registered;
    bool b_need_user_input;
};
extern ENGINE_API CLoadScreenRenderer load_screen_renderer;

// These variables have corrensponding console commands, and can be used to quickly tune or test smth, without quiting from game
extern ENGINE_API float devfloat1;
extern ENGINE_API float devfloat2;
extern ENGINE_API float devfloat3;
extern ENGINE_API float devfloat4;

extern ENGINE_API float d_material;
extern ENGINE_API float d_material_weight;
extern ENGINE_API shared_str d_texture_name;
extern ENGINE_API bool override_material;
extern ENGINE_API float fps_limit;

#endif