#include "stdafx.h"

void	CRenderTarget::phase_smap_direct		(light* L, u32 /*sub_phase*/)
{
    // Targets
    if (RImplementation.o.HW_smap)        u_setrt(rt_smap_surf, NULL, NULL, rt_smap_depth->pRT);
    else                                u_setrt(rt_smap_surf, NULL, NULL, rt_smap_ZB);

    // Viewport
    D3DVIEWPORT9 oldVP;
    CHK_DX(HW.pDevice->GetViewport(&oldVP));

    u32 x = L->X.D.posX;
    u32 y = L->X.D.posY;
    u32 w = L->X.D.size;
    u32 h = L->X.D.size;
    D3DVIEWPORT9 smapVP = { x, y, w, h, 0.0f, 1.0f };
    CHK_DX(HW.pDevice->SetViewport(&smapVP));

    // Stencil    - disable
    RCache.set_Stencil(FALSE);

    //    Cull always CCW. If you want to revert to previouse solution, please, revert bias setup/
    RCache.set_CullMode(CULL_CCW);    // near
    if (RImplementation.o.HW_smap)        RCache.set_ColorWriteEnable(FALSE);
    else                                RCache.set_ColorWriteEnable();

    // Clear
    CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0L));
}

void	CRenderTarget::phase_smap_direct_tsh	(light* L, u32 sub_phase)
{
	VERIFY								(RImplementation.o.Tshadows);
	u32		_clr						= 0xffffffff;	//color_rgba(127,127,12,12);
	RCache.set_ColorWriteEnable			();
	CHK_DX								(HW.pDevice->Clear( 0L, NULL, D3DCLEAR_TARGET,	_clr,	1.0f, 0L));
}
