#pragma once

#ifdef _EDITOR
#include "r2_types.h"
#include "Layers/xrRenderPC_R2/stdafx.h"
#endif

class CBlender_accum_reflected : public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: accumulate reflected light";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_accum_reflected();
	virtual ~CBlender_accum_reflected();
};
