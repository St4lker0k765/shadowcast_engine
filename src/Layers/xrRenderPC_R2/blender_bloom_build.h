#pragma once

#ifdef _EDITOR
#include "r2_types.h"
#include "Layers/xrRenderPC_R2/stdafx.h"
#endif

class CBlender_bloom_build : public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: combine to bloom target";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_bloom_build();
	virtual ~CBlender_bloom_build();
};
