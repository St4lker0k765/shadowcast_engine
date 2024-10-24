#pragma once

#ifdef _EDITOR
#include "r2_types.h"
#include "Layers/xrRenderPC_R2/r2.h"
#endif

class CBlender_deffer_model : public IBlender  
{
public:
	xrP_Integer	oAREF;
	xrP_BOOL	oBlend;
public:
	virtual		LPCSTR		getComment()	{ return "LEVEL: deffer-model-flat";	}
	virtual		BOOL		canBeDetailed()	{ return TRUE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Save			(IWriter&	fs);
	virtual		void		Load			(IReader&	fs, u16 version);
	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_deffer_model();
	virtual ~CBlender_deffer_model();

private:
	xrP_TOKEN	oTessellation;
};
