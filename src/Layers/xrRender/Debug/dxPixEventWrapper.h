#ifndef	dxPixEventWrapper_included
#define	dxPixEventWrapper_included
#pragma once

#ifdef	DEBUG
#include <d3d9.h>
#define PIX_EVENT(Name)	dxPixEventWrapper	pixEvent##Name(L#Name)

class dxPixEventWrapper
{
public:
	dxPixEventWrapper(LPCWSTR wszName) { D3DPERF_BeginEvent( color_rgba(127,0,0,255), wszName );}
	~dxPixEventWrapper() {D3DPERF_EndEvent();}
};
#else	//	DEBUG

#define PIX_EVENT(Name)	{;}

#endif	//	DEBUG

#endif	//	dxPixEventWrapper_included