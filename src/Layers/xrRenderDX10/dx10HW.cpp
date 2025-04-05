#include "stdafx.h"
#pragma warning(disable:4995)
//#include <d3dx9.h>
#pragma warning(default:4995)
#include "../xrRender/HW.h"
#include "../../xrEngine/XR_IOConsole.h"
#include "../../Include/xrAPI/xrAPI.h"

#include "StateManager\dx10SamplerStateCache.h"
#include "StateManager\dx10StateCache.h"

void	fill_vid_mode_list			(CHW* _hw);
void	free_vid_mode_list			();

void	fill_render_mode_list		();
void	free_render_mode_list		();

CHW			HW;

extern ENGINE_API float psSVPImageSizeK;


CHW::CHW()
{
	m_pAdapter = nullptr;
	pDevice = nullptr;
	m_move_window = true;
	pContext = nullptr;

	m_pSwapChain = nullptr;
	pBaseRT = nullptr;
	pBaseZB = nullptr;

	m_bUsePerfhud = false;

	Device.seqAppActivate.Add(this);
	Device.seqAppDeactivate.Add(this);

	storedVP = static_cast<ViewPort>(0);
}

CHW::~CHW()
{
	Device.seqAppActivate.Remove(this);
	Device.seqAppDeactivate.Remove(this);
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CHW::CreateD3D()
{
	IDXGIFactory * pFactory;
	R_CHK( CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory)) );

	m_pAdapter = 0;
	m_bUsePerfhud = false;

#ifndef	MASTER_GOLD
	// Look for 'NVIDIA NVPerfHUD' adapter
	// If it is present, override default settings
	UINT i = 0;
	while(pFactory->EnumAdapters(i, &m_pAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		m_pAdapter->GetDesc(&desc);
		if(!wcscmp(desc.Description,L"NVIDIA PerfHUD"))
		{
			m_bUsePerfhud = true;
			break;
		}
		else
		{
			m_pAdapter->Release();
			m_pAdapter = 0;
		}
		++i;
	}
#endif	//	MASTER_GOLD

	if (!m_pAdapter)
		pFactory->EnumAdapters(0, &m_pAdapter);

	pFactory->Release();

}

void CHW::DestroyD3D()
{
	//_RELEASE					(this->pD3D);

	_SHOW_REF				("refCount:m_pAdapter",m_pAdapter);
	_RELEASE				(m_pAdapter);

//	FreeLibrary(hD3D);
}

void CHW::CreateDevice( HWND m_hWnd, bool move_window )
{
	m_move_window			= move_window;
	CreateD3D();

	BOOL  bWindowed			= !psDeviceFlags.is(rsFullscreen);

	m_DriverType = Caps.bForceGPU_REF ? 
		D3D_DRIVER_TYPE_REFERENCE : D3D_DRIVER_TYPE_HARDWARE;

	if (m_bUsePerfhud)
		m_DriverType = D3D_DRIVER_TYPE_REFERENCE;

	// Display the name of video board
	DXGI_ADAPTER_DESC Desc;
	R_CHK( m_pAdapter->GetDesc(&Desc) );
	//	Warning: Desc.Description is wide string
	Msg		("* GPU [vendor:%X]-[device:%X]: %S", Desc.VendorId, Desc.DeviceId, Desc.Description);

	Caps.id_vendor	= Desc.VendorId;
	Caps.id_device	= Desc.DeviceId;

	// Select back-buffer & depth-stencil format
	D3DFORMAT&	fTarget	= Caps.fTarget;
	D3DFORMAT&	fDepth	= Caps.fDepth;

	//	HACK: DX10: Embed hard target format.
	fTarget = D3DFMT_X8R8G8B8;	//	No match in DX10. D3DFMT_A8B8G8R8->DXGI_FORMAT_R8G8B8A8_UNORM
	fDepth = selectDepthStencil(fTarget);
	
	// Set up the presentation parameters
	DXGI_SWAP_CHAIN_DESC	&sd	= m_ChainDesc;
	ZeroMemory				( &sd, sizeof(sd) );

	selectResolution	(sd.BufferDesc.Width, sd.BufferDesc.Height, bWindowed);

	UINT buffers_count = bWindowed ? 1 : 2;

	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferCount = buffers_count;

	// Multisample
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;

	//sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.OutputWindow = m_hWnd;
	sd.Windowed = bWindowed;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;;

	if (buffers_count > 1)
		sd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
	else
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;


	if (bWindowed)
	{
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
	}
	else
	{
		sd.BufferDesc.RefreshRate = selectRefresh( sd.BufferDesc.Width, sd.BufferDesc.Height, sd.BufferDesc.Format);
	}

	//	Additional set up
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	UINT createDeviceFlags = 0;
   HRESULT R;
#ifdef USE_DX11
    D3D_FEATURE_LEVEL pFeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
    };

   R =  D3D11CreateDeviceAndSwapChain(   0,//m_pAdapter,//What wrong with adapter??? We should use another version of DXGI?????
                                          m_DriverType,
                                          NULL,
                                          createDeviceFlags,
										  pFeatureLevels,
										  sizeof(pFeatureLevels)/sizeof(pFeatureLevels[0]),
										  D3D11_SDK_VERSION,
                                          &sd,
                                          &m_pSwapChain,
		                                  &pDevice,
										  &FeatureLevel,		
										  &pContext);
#else
   R =  D3DX10CreateDeviceAndSwapChain(   m_pAdapter,
                                          m_DriverType,
                                          NULL,
                                          createDeviceFlags,
                                          &sd,
                                          &m_pSwapChain,
		                                    &pDevice );

   pContext = pDevice;
   FeatureLevel = D3D_FEATURE_LEVEL_10_0;
   if(!FAILED(R))
   {
      D3DX10GetFeatureLevel1( pDevice, &pDevice1 );
	  FeatureLevel = D3D_FEATURE_LEVEL_10_1;
   }
   pContext1 = pDevice1;
#endif

	//if (D3DERR_DEVICELOST==R)	{
	if (FAILED(R))
	{
		// Fatal error! Cannot create rendering device AT STARTUP !!!
		Msg					("Failed to initialize graphics hardware.\n"
							 "Please try to restart the game.\n"
							 "CreateDevice returned 0x%08x", R
							 );
		FlushLog			();
		MessageBox			(NULL,"Failed to initialize graphics hardware.\nPlease try to restart the game.","Error!",MB_OK|MB_ICONERROR);
		TerminateProcess	(GetCurrentProcess(),0);
	};
	R_CHK(R);

	_SHOW_REF	("* CREATE: DeviceREF:",HW.pDevice);


	// Capture misc data
//	DX10: Don't neeed this?
//#ifdef DEBUG
//	R_CHK	(pDevice->CreateStateBlock			(D3DSBT_ALL,&dwDebugSB));
//#endif
	//	Create render target and depth-stencil views here
	UpdateViews();

	//u32	memory									= pDevice->GetAvailableTextureMem	();
	size_t	memory									= Desc.DedicatedVideoMemory;
	Msg		("*     Texture memory: %d M",		memory/(1024*1024));
	//Msg		("*          DDI-level: %2.1f",		float(D3DXGetDriverLevel(pDevice))/100.f);
#ifndef _EDITOR
	updateWindowProps							(m_hWnd);
	fill_vid_mode_list							(this);
#endif
}

void CHW::DestroyDevice()
{
	//	Destroy state managers
	StateManager.Reset();
	RSManager.ClearStateArray();
	DSSManager.ClearStateArray();
	BSManager.ClearStateArray();
	SSManager.ClearStateArray();

	//_SHOW_REF				("refCount:pBaseZB",pBaseZB);
	//_RELEASE				(pBaseZB);

	for (auto it = viewPortsRTZB.begin(); it != viewPortsRTZB.end(); ++it)
	{
		_SHOW_REF("refCount:pBaseZB", it->second.baseZB);
		_SHOW_REF("refCount:pBaseRT", it->second.baseRT);
		_RELEASE(it->second.baseZB);
		_RELEASE(it->second.baseRT);
	}

	//_SHOW_REF				("refCount:pBaseRT",pBaseRT);
	//_RELEASE				(pBaseRT);

	//	Must switch to windowed mode to release swap chain
	if (!m_ChainDesc.Windowed) m_pSwapChain->SetFullscreenState( FALSE, NULL);
	_SHOW_REF				("refCount:m_pSwapChain",m_pSwapChain);
	_RELEASE				(m_pSwapChain);

#ifdef USE_DX11
	_RELEASE				(pContext);
#endif

#ifndef USE_DX11
	_RELEASE				(HW.pDevice1);
#endif
	_SHOW_REF				("DeviceREF:",HW.pDevice);
	_RELEASE				(HW.pDevice);


	DestroyD3D				();

#ifndef _EDITOR
	free_vid_mode_list		();
#endif
}

//////////////////////////////////////////////////////////////////////
// Resetting device
//////////////////////////////////////////////////////////////////////
void CHW::Reset (HWND hwnd)
{
	DXGI_SWAP_CHAIN_DESC &cd = m_ChainDesc;

	BOOL	bWindowed		= !psDeviceFlags.is	(rsFullscreen);

	cd.Windowed = bWindowed;

	m_pSwapChain->SetFullscreenState(!bWindowed, NULL);

	DXGI_MODE_DESC	&desc = m_ChainDesc.BufferDesc;

	selectResolution(desc.Width, desc.Height, bWindowed);

	if (bWindowed)
	{
		desc.RefreshRate.Numerator = 60;
		desc.RefreshRate.Denominator = 1;
	}
	else
		desc.RefreshRate = selectRefresh( desc.Width, desc.Height, desc.Format);

	CHK_DX(m_pSwapChain->ResizeTarget(&desc));


#ifdef DEBUG
	//	_RELEASE			(dwDebugSB);
#endif

	for (auto it = viewPortsRTZB.begin(); it != viewPortsRTZB.end(); ++it)
	{
		_SHOW_REF("refCount:pBaseZB", it->second.baseZB);
		_SHOW_REF("refCount:pBaseRT", it->second.baseRT);
		_RELEASE(it->second.baseZB);
		_RELEASE(it->second.baseRT);
	}

	//_SHOW_REF				("refCount:pBaseZB",pBaseZB);
	//_SHOW_REF				("refCount:pBaseRT",pBaseRT);

	//_RELEASE(pBaseZB);
	//_RELEASE(pBaseRT);

	CHK_DX(m_pSwapChain->ResizeBuffers(
		cd.BufferCount,
		desc.Width,
		desc.Height,
		desc.Format,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	UpdateViews();

	updateWindowProps	(hwnd);

}

void CHW::SwitchVP(ViewPort vp)
{
	if (storedVP == vp && pBaseRT)
		return;

	storedVP = vp;

	auto it = viewPortsRTZB.find(vp);

	if (it == viewPortsRTZB.end())
		it = viewPortsRTZB.find(MAIN_VIEWPORT);

	pBaseRT = it->second.baseRT;
	pBaseZB = it->second.baseZB;
}


D3DFORMAT CHW::selectDepthStencil	(D3DFORMAT fTarget)
{
	// R3 hack
#pragma todo("R3 need to specify depth format")
	return D3DFMT_D24S8;
}

void CHW::selectResolution( u32 &dwWidth, u32 &dwHeight, BOOL bWindowed )
{
	fill_vid_mode_list			(this);

	if(bWindowed)
	{
		dwWidth		= psCurrentVidMode[0];
		dwHeight	= psCurrentVidMode[1];
	}
	else //check
	{
		string64					buff;
		xr_sprintf					(buff,sizeof(buff),"%dx%d",psCurrentVidMode[0],psCurrentVidMode[1]);

		if(_ParseItem(buff,vid_mode_token)==static_cast<u32>(-1)) //not found
		{ //select safe
			xr_sprintf				(buff,sizeof(buff),"vid_mode %s",vid_mode_token[0].name);
			Console->Execute		(buff);
		}

		dwWidth						= psCurrentVidMode[0];
		dwHeight					= psCurrentVidMode[1];
	}
}

DXGI_RATIONAL CHW::selectRefresh(u32 dwWidth, u32 dwHeight, DXGI_FORMAT fmt)
{
	DXGI_RATIONAL	res;

	res.Numerator = 60;
	res.Denominator = 1;

	float	CurrentFreq = 60.0f;

	if (psDeviceFlags.is(rsRefresh60hz))	
	{
		return res;
	}
	else
	{
		xr_vector<DXGI_MODE_DESC>	modes;

		IDXGIOutput *pOutput;
		m_pAdapter->EnumOutputs(0, &pOutput);
		VERIFY(pOutput);

		UINT num = 0;
		DXGI_FORMAT format = fmt;
		UINT flags         = 0;

		// Get the number of display modes available
		pOutput->GetDisplayModeList( format, flags, &num, 0);

		// Get the list of display modes
		modes.resize(num);
		pOutput->GetDisplayModeList( format, flags, &num, &modes.front());

		_RELEASE(pOutput);

		for (u32 i=0; i<num; ++i)
		{
			DXGI_MODE_DESC &desc = modes[i];

			if( (desc.Width == dwWidth) 
				&& (desc.Height == dwHeight)
				)
			{
				VERIFY(desc.RefreshRate.Denominator);
				float TempFreq = static_cast<float>(desc.RefreshRate.Numerator)/static_cast<float>(desc.RefreshRate.Denominator);
				if ( TempFreq > CurrentFreq )
				{
					CurrentFreq = TempFreq;
					res = desc.RefreshRate;
				}
			}
		}

		return res;
	}
}

void CHW::OnAppActivate()
{
	if ( m_pSwapChain && !m_ChainDesc.Windowed )
	{
		ShowWindow( m_ChainDesc.OutputWindow, SW_RESTORE );
		m_pSwapChain->SetFullscreenState( TRUE, NULL );
	}
}

void CHW::OnAppDeactivate()
{
	if ( m_pSwapChain && !m_ChainDesc.Windowed )
	{
		m_pSwapChain->SetFullscreenState( FALSE, NULL );
		ShowWindow( m_ChainDesc.OutputWindow, SW_MINIMIZE );
	}
}


BOOL CHW::support( D3DFORMAT fmt, DWORD type, DWORD usage)
{
	//	TODO: DX10: implement stub for this code.
	VERIFY(!"Implement CHW::support");
	/*
	HRESULT hr		= pD3D->CheckDeviceFormat(DevAdapter,DevT,Caps.fTarget,usage,(D3DRESOURCETYPE)type,fmt);
	if (FAILED(hr))	return FALSE;
	else			return TRUE;
	*/
	return TRUE;
}

void CHW::updateWindowProps(HWND m_hWnd)
{
	//	BOOL	bWindowed				= strstr(Core.Params,"-dedicated") ? TRUE : !psDeviceFlags.is	(rsFullscreen);
	BOOL	bWindowed				= !psDeviceFlags.is	(rsFullscreen);

	u32		dwWindowStyle;
	// Set window properties depending on what mode were in.
	if (bWindowed)		{
		if (m_move_window) {
            dwWindowStyle = WS_VISIBLE;
            if (!strstr(Core.Params, "-no_dialog_header"))
                dwWindowStyle |= WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX;
			SetWindowLongPtr(m_hWnd, GWL_STYLE, dwWindowStyle);
			// When moving from fullscreen to windowed mode, it is important to
			// adjust the window size after recreating the device rather than
			// beforehand to ensure that you get the window size you want.  For
			// example, when switching from 640x480 fullscreen to windowed with
			// a 1000x600 window on a 1024x768 desktop, it is impossible to set
			// the window size to 1000x600 until after the display mode has
			// changed to 1024x768, because windows cannot be larger than the
			// desktop.

			RECT			m_rcWindowBounds;
			RECT				DesktopRect;

			GetClientRect		(GetDesktopWindow(), &DesktopRect);

			SetRect(			&m_rcWindowBounds, 
				(DesktopRect.right-m_ChainDesc.BufferDesc.Width)/2, 
				(DesktopRect.bottom-m_ChainDesc.BufferDesc.Height)/2, 
				(DesktopRect.right+m_ChainDesc.BufferDesc.Width)/2, 
				(DesktopRect.bottom+m_ChainDesc.BufferDesc.Height)/2);

			AdjustWindowRect		(	&m_rcWindowBounds, dwWindowStyle, FALSE );

			SetWindowPos			(	m_hWnd, 
				HWND_NOTOPMOST,	
                m_rcWindowBounds.left,
                m_rcWindowBounds.top,
				( m_rcWindowBounds.right - m_rcWindowBounds.left ),
				( m_rcWindowBounds.bottom - m_rcWindowBounds.top ),
				SWP_SHOWWINDOW|SWP_NOCOPYBITS|SWP_DRAWFRAME );
		}
	}
	else
	{
		SetWindowLongPtr( m_hWnd, GWL_STYLE, dwWindowStyle=(WS_POPUP|WS_VISIBLE) );
	}

	ShowCursor	(FALSE);
	SetForegroundWindow( m_hWnd );
	//RECT winRect;
	//GetWindowRect(m_hWnd, &winRect);
	//ClipCursor(&winRect);
}


struct _uniq_mode
{
	_uniq_mode(LPCSTR v):_val(v){}
	LPCSTR _val;
	bool operator() (LPCSTR _other) {return !stricmp(_val,_other);}
};

#ifndef _EDITOR
void free_vid_mode_list()
{
	for( int i=0; vid_mode_token[i].name; i++ )
	{
		xr_free					(vid_mode_token[i].name);
	}
	xr_free						(vid_mode_token);
	vid_mode_token				= NULL;
}

void fill_vid_mode_list(CHW* _hw)
{
	if(vid_mode_token != NULL)		return;
	xr_vector<LPCSTR>	_tmp;
	xr_vector<DXGI_MODE_DESC>	modes;

	IDXGIOutput *pOutput;
	//_hw->m_pSwapChain->GetContainingOutput(&pOutput);
	_hw->m_pAdapter->EnumOutputs(0, &pOutput);
	VERIFY(pOutput);

	UINT num = 0;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	UINT flags         = 0;

	// Get the number of display modes available
	pOutput->GetDisplayModeList( format, flags, &num, 0);

	// Get the list of display modes
	modes.resize(num);
	pOutput->GetDisplayModeList( format, flags, &num, &modes.front());

	_RELEASE(pOutput);

	for (u32 i=0; i<num; ++i)
	{
		DXGI_MODE_DESC &desc = modes[i];
		string32		str;

		if(desc.Width < 800)
			continue;

		xr_sprintf(str, sizeof(str), "%dx%d", desc.Width, desc.Height);

		if(_tmp.end() != std::find_if(_tmp.begin(), _tmp.end(), _uniq_mode(str)))
			continue;

		_tmp.push_back				(NULL);
		_tmp.back()					= xr_strdup(str);
	}
	


//	_tmp.push_back				(NULL);
//	_tmp.back()					= xr_strdup("1024x768");

	u32 _cnt						= _tmp.size()+1;

	vid_mode_token					= xr_alloc<xr_token>(_cnt);

	vid_mode_token[_cnt-1].id			= -1;
	vid_mode_token[_cnt-1].name		= NULL;

#ifdef DEBUG
	Msg("Available video modes[%d]:",_tmp.size());
#endif // DEBUG
	for( u32 i=0; i<_tmp.size(); ++i )
	{
		vid_mode_token[i].id		= i;
		vid_mode_token[i].name		= _tmp[i];
#ifdef DEBUG
		Msg							("[%s]",_tmp[i]);
#endif // DEBUG
	}
}

void CHW::UpdateViews()
{
	DXGI_SWAP_CHAIN_DESC &sd = m_ChainDesc;
	HRESULT R;

	// Set up svp image size
	Device.m_SecondViewport.screenWidth = static_cast<u32>((sd.BufferDesc.Width / 32) * psSVPImageSizeK) * 32;
	Device.m_SecondViewport.screenHeight = static_cast<u32>((sd.BufferDesc.Height / 32) * psSVPImageSizeK) * 32;

	// Create a render target view
	//R_CHK	(pDevice->GetRenderTarget			(0,&pBaseRT));
	//ID3DTexture2D *pBuffer;
	//R = m_pSwapChain->GetBuffer( 0, __uuidof( ID3DTexture2D ), (LPVOID*)&pBuffer );
	//R_CHK(R);

	viewPortsRTZB.insert(std::make_pair(MAIN_VIEWPORT, HWViewPortRTZB()));
	viewPortsRTZB.insert(std::make_pair(SECONDARY_WEAPON_SCOPE, HWViewPortRTZB()));

	ID3DTexture2D* temp1;
	ID3DTexture2D* temp2;

	R = m_pSwapChain->GetBuffer(0, __uuidof(ID3DTexture2D), reinterpret_cast<void**>(&temp1));
	R_CHK2(R, "!Erroneous buffer result");

	D3D_TEXTURE2D_DESC desc;
	temp1->GetDesc(&desc);
	desc.Width = Device.m_SecondViewport.screenWidth;
	desc.Height = Device.m_SecondViewport.screenHeight;

	R = pDevice->CreateTexture2D(&desc, NULL, &temp2);
	R_CHK(R);

	R = pDevice->CreateRenderTargetView(temp1, NULL, &viewPortsRTZB.at(MAIN_VIEWPORT).baseRT);
	R_CHK(R);

	R = pDevice->CreateRenderTargetView(temp2, NULL, &viewPortsRTZB.at(SECONDARY_WEAPON_SCOPE).baseRT);
	R_CHK(R);

	temp1->Release();
	temp2->Release();

	//R = pDevice->CreateRenderTargetView( pBuffer, NULL, &pBaseRT);
	//pBuffer->Release();
	//R_CHK(R);

	//	Create Depth/stencil buffer
	//	HACK: DX10: hard depth buffer format
	//R_CHK	(pDevice->GetDepthStencilSurface	(&pBaseZB));
	//ID3DTexture2D* pDepthStencil = NULL;
	ID3DTexture2D* depth_stencil = NULL;
	D3D_TEXTURE2D_DESC descDepth;
	descDepth.Width = sd.BufferDesc.Width;
	descDepth.Height = sd.BufferDesc.Height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D_USAGE_DEFAULT;
	descDepth.BindFlags = D3D_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	//R = pDevice->CreateTexture2D( &descDepth,NULL,&pDepthStencil );
	R = pDevice->CreateTexture2D(&descDepth, NULL, &depth_stencil);
	R_CHK(R);

	//	Create Depth/stencil view
	R = pDevice->CreateDepthStencilView(depth_stencil, NULL, &viewPortsRTZB.at(MAIN_VIEWPORT).baseZB);
	R_CHK(R);
	//R = pDevice->CreateDepthStencilView( pDepthStencil, NULL, &pBaseZB );
	//R_CHK(R);

	//pDepthStencil->Release();

	depth_stencil->Release();

	descDepth.Width = Device.m_SecondViewport.screenWidth;
	descDepth.Height = Device.m_SecondViewport.screenHeight;

	R = pDevice->CreateTexture2D(&descDepth, NULL, &depth_stencil);
	R_CHK(R);

	R = pDevice->CreateDepthStencilView(depth_stencil, NULL, &viewPortsRTZB.at(SECONDARY_WEAPON_SCOPE).baseZB);
	R_CHK(R);

	depth_stencil->Release();

	// first init
	pBaseRT = viewPortsRTZB.at(MAIN_VIEWPORT).baseRT;
	pBaseZB = viewPortsRTZB.at(MAIN_VIEWPORT).baseZB;
}
#endif
