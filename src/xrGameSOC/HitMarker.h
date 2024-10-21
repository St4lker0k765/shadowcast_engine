#pragma once

#include "../Include/xrRender/FactoryPtr.h"
class IUIShader;
#include "ui_defs.h"

class CUIStaticItem;
class CLAItem;

struct SHitMark{
	CUIStaticItem*	m_UIStaticItem;
	float			m_StartTime;
	float			m_HitDirection;
	CLAItem*		m_lanim;

					SHitMark		(const ui_shader& sh, const Fvector& dir);
					~SHitMark		();
	bool			IsActive		();
	void			UpdateAnim		();
	void			Draw			(float dir);
};


class CHitMarker
{
public:
	typedef xr_deque<SHitMark*> HITMARKS;
	FactoryPtr<IUIShader>	hShader2;
	HITMARKS				m_HitMarks;

public:
							CHitMarker	();
							~CHitMarker	();

	void					Render		();
	void					Hit			(int id, const Fvector& dir);
	void					InitShader	(LPCSTR tex_name);
};
