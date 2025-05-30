#include "StdAfx.h"

#include "UITrackBar.h"
#include "UI3tButton.h"
#include "UITextureMaster.h"
#include "../../xrEngine/xr_input.h"

bool UseOldTrackbarTex = READ_IF_EXISTS(pSCSettings, r_bool, "game_specific", "cop_old_trackbar_texture", false);
bool EnableTrackbarScaling = READ_IF_EXISTS(pSCSettings, r_bool, "ui", "enable_trackbar_scaling", false);

#define DEF_CONTROL_HEIGHT		16.0f

CUITrackBar::CUITrackBar()
	: m_f_min(0),
	  m_f_max(1),
	  m_f_val(0),
	  m_f_opt_backup_value(0),
	 m_f_step(0.01f),
	m_b_is_float(true),
	m_b_invert(false),
	m_b_bound_already_set(false)
{	
	m_pSlider						= xr_new<CUI3tButton>();			
	AttachChild						(m_pSlider);		
	m_pSlider->SetAutoDelete		(true);

	m_static = new CUIStatic();
	m_static->Enable(false);
	AttachChild(m_static);
	m_static->SetAutoDelete(true);

	m_b_mouse_capturer				= false;
}

bool CUITrackBar::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
	CUIWindow::OnMouseAction(x, y, mouse_action);

	switch (mouse_action)
	{
	case WINDOW_MOUSE_MOVE:
		{
			if(m_bCursorOverWindow && m_b_mouse_capturer)
			{
				if (pInput->iGetAsyncBtnState(0))
					UpdatePosRelativeToMouse();
			}
		}break;
	case WINDOW_LBUTTON_DOWN:
		{
			m_b_mouse_capturer = m_bCursorOverWindow;
			if(m_b_mouse_capturer)
				UpdatePosRelativeToMouse();
		}break;

	case WINDOW_LBUTTON_UP:
		{
			m_b_mouse_capturer = false;
		}
		break;
	case WINDOW_MOUSE_WHEEL_UP:
		{
			if(m_b_is_float)
			{
				m_f_val -= GetInvert()?-m_f_step:m_f_step;
				clamp(m_f_val, m_f_min, m_f_max);
			}
			else
			{
				m_i_val -= GetInvert()?-m_i_step:m_i_step;
				clamp(m_i_val, m_i_min, m_i_max);
			}
			GetMessageTarget()->SendMessage(this, BUTTON_CLICKED, NULL);
			UpdatePos			();
			OnChangedOptValue	();
		}
		break;
	case WINDOW_MOUSE_WHEEL_DOWN:
		{
			if(m_b_is_float)
			{
				m_f_val += GetInvert()?-m_f_step:m_f_step;
				clamp(m_f_val, m_f_min, m_f_max);
			}
			else
			{
				m_i_val += GetInvert()?-m_i_step:m_i_step;
				clamp(m_i_val, m_i_min, m_i_max);
			}
			GetMessageTarget()->SendMessage(this, BUTTON_CLICKED, NULL);
			UpdatePos();
			OnChangedOptValue	();
		}
		break;
	};
	return true;
}

void CUITrackBar::InitTrackBar(Fvector2 pos, Fvector2 size)
{
	float				item_height;
	float				item_width;

	pcstr slider_bar_tex = "ui_inGame2_opt_slider_bar";
	pcstr slider_box_tex = "ui_inGame2_opt_slider_box";
	pcstr slider_box_e_tex = "ui_inGame2_opt_slider_box_e";
	if (UseOldTrackbarTex)
	{
		slider_bar_tex = "ui_slider_e";
		slider_box_tex = "ui_slider_button";
		slider_box_e_tex = "ui_slider_button_e";
	}
	InitIB				(pos, size);
		InitState(S_Enabled, slider_bar_tex);
		InitState(S_Disabled, slider_bar_tex);

		item_width = CUITextureMaster::GetTextureWidth(slider_box_e_tex);
		item_height = CUITextureMaster::GetTextureHeight(slider_box_e_tex);

		item_width *= UI().get_current_kx();
		if (EnableTrackbarScaling)
		{
			item_width /= 2.0f;
			item_height /= 2.0f;
		}
		m_pSlider->InitButton(Fvector2().set(0.0f, 0.0f) /*(size.y - item_height)/2.0f)*/,
			Fvector2().set(item_width, item_height));			//size
		m_pSlider->InitTexture(slider_box_tex);
	SetCurrentState(S_Enabled);
}	

void CUITrackBar::Draw()
{
	CUI_IB_FrameLineWnd::Draw	();
	m_pSlider->Draw				();
	m_static->Draw				();
}

void CUITrackBar::Update()
{
	CUIWindow::Update();

	if(m_b_mouse_capturer)
	{
		if(!pInput->iGetAsyncBtnState(0))
			m_b_mouse_capturer = false;
	}
}

void CUITrackBar::SetCurrentOptValue()
{
	CUIOptionsItem::SetCurrentOptValue();
	if(m_b_is_float)
	{
		float fake_min, fake_max;
		if (!m_b_bound_already_set)
			GetOptFloatValue(m_f_val, m_f_min, m_f_max);
		else
			GetOptFloatValue(m_f_val, fake_min, fake_max);
	}
	else
	{
		int fake_min, fake_max;
		if (!m_b_bound_already_set)
			GetOptIntegerValue(m_i_val, m_i_min, m_i_max);
		else
			GetOptIntegerValue(m_i_val, fake_min, fake_max);
	}

	UpdatePos			();
}

void CUITrackBar::SaveOptValue()
{
	CUIOptionsItem::SaveOptValue	();
	if(m_b_is_float)
		SaveOptFloatValue			(m_f_val);
	else
		SaveOptIntegerValue			(m_i_val);
}

bool CUITrackBar::IsChangedOptValue() const
{
	if(m_b_is_float)
	{
		return !fsimilar(m_f_opt_backup_value, m_f_val); 
	}else
	{
		return (m_i_opt_backup_value != m_i_val);
	}
}

void CUITrackBar::SaveBackUpOptValue()
{
	CUIOptionsItem::SaveBackUpOptValue();

	if(m_b_is_float)
		m_f_opt_backup_value		= m_f_val;
	else
		m_i_opt_backup_value		= m_i_val;
}

void CUITrackBar::UndoOptValue()
{
	if(m_b_is_float)
		m_f_val			= m_f_opt_backup_value;
	else
		m_i_val			= m_i_opt_backup_value;

	UpdatePos			();
	CUIOptionsItem::UndoOptValue();
}

void CUITrackBar::SetStep(float step)
{
	if(m_b_is_float)
		m_f_step	= step;
	else
		m_i_step	= iFloor(step);
}

void CUITrackBar::Enable(bool status)
{
	m_bIsEnabled				= status;
	SetCurrentState				(m_bIsEnabled?S_Enabled:S_Disabled);
	m_pSlider->Enable			(m_bIsEnabled);
}

void CUITrackBar::UpdatePosRelativeToMouse()
{
	float _bkf		= 0.0f;
	int _bki		= 0;
	if(m_b_is_float)
	{
		_bkf = m_f_val; 
	}else
	{
		_bki = m_i_val; 
	}


	float btn_width				= m_pSlider->GetWidth();
	float window_width			= GetWidth();		
	float fpos					= cursor_pos.x;

	if( GetInvert() )
		fpos					= window_width - fpos;

	if (fpos < btn_width/2)
		fpos = btn_width/2;
	else 
	if (fpos > window_width - btn_width/2)
		fpos = window_width - btn_width/2;

	float __fval;
	float __fmax	= (m_b_is_float)?m_f_max:(float)m_i_max;
	float __fmin	= (m_b_is_float)?m_f_min:(float)m_i_min;
	float __fstep	= (m_b_is_float)?m_f_step:(float)m_i_step;

	__fval						= (__fmax - __fmin)*(fpos - btn_width/2)/(window_width - btn_width)+ __fmin;
	
	float _d	= (__fval-__fmin);
	
	float _v	= _d/__fstep;
	int _vi		= iFloor(_v);
	float _vf	= __fstep*_vi;
	
	if(_d-_vf>__fstep/2.0f)	
		_vf		+= __fstep;

	__fval		= __fmin+_vf;
	
	clamp		(__fval, __fmin, __fmax);

	if(m_b_is_float)
		m_f_val	= __fval;
	else
		m_i_val	= iFloor(__fval);
	

	bool b_ch = false;
	if(m_b_is_float)
	{
		b_ch  = !fsimilar(_bkf, m_f_val); 
	}else
	{
		b_ch  =  (_bki != m_i_val);
	}

	if(b_ch)
		GetMessageTarget()->SendMessage(this, BUTTON_CLICKED, NULL);

	UpdatePos	();
	OnChangedOptValue	();
}

void CUITrackBar::UpdatePos()
{
#ifdef DEBUG
	
	if(m_b_is_float)
		R_ASSERT2(m_f_val >= m_f_min && m_f_val <= m_f_max, "CUITrackBar::UpdatePos() - m_val >= m_min && m_val <= m_max" );
	else
		R_ASSERT2(m_i_val >= m_i_min && m_i_val <= m_i_max, "CUITrackBar::UpdatePos() - m_val >= m_min && m_val <= m_max" );

#endif

	float btn_width				= m_pSlider->GetWidth();
	float window_width			= GetWidth();		
	float free_space			= window_width - btn_width;
	Fvector2 pos				= m_pSlider->GetWndPos();
    
	float __fval	= (m_b_is_float)?m_f_val:(float)m_i_val;
	float __fmax	= (m_b_is_float)?m_f_max:(float)m_i_max;
	float __fmin	= (m_b_is_float)?m_f_min:(float)m_i_min;


	pos.x						= (__fval - __fmin)*free_space/(__fmax - __fmin);
	if( GetInvert() )
		pos.x					= free_space-pos.x;

	m_pSlider->SetWndPos		(pos);

	if (m_static->IsEnabled())
	{
		string256 buff;
		if (m_b_is_float)
		{
			xr_sprintf(buff, (m_static_format == NULL ? "%.1f" : m_static_format.c_str()), m_f_val);
		}
		else
		{
			xr_sprintf(buff, (m_static_format == NULL ? "%d" : m_static_format.c_str()), m_i_val);
		}
		m_static->TextItemControl()->SetTextST(buff);
	}
}

void CUITrackBar::OnMessage(LPCSTR message)
{
	if (0 == xr_strcmp(message, "set_default_value"))
	{
		if(m_b_is_float)
			m_f_val = m_f_min + (m_f_max - m_f_min)/2.0f;
		else
			m_i_val = m_i_min + iFloor((m_i_max - m_i_min)/2.0f);

		UpdatePos();
	}
}

bool CUITrackBar::GetCheck()
{
	VERIFY(!m_b_is_float);
	return !!m_i_val;
}

void CUITrackBar::SetCheck(bool b)
{
	VERIFY(!m_b_is_float);
	m_i_val = (b)?m_i_max:m_i_min;
}

void CUITrackBar::SetOptIBounds(int imin, int imax)
{
	m_i_min					= imin;
	m_i_max					= imax;
	if(m_i_val<m_i_min || m_i_val>m_i_max)
	{
		clamp					(m_i_val, m_i_min, m_i_max);
		OnChangedOptValue	();
	}
}

void CUITrackBar::SetOptFBounds(float fmin, float fmax)
{
	m_f_min					= fmin;
	m_f_max					= fmax;
	if(m_f_val<m_f_min || m_f_val>m_f_max)
	{
		clamp				(m_f_val, m_f_min, m_f_max);
		OnChangedOptValue	();
	}
}
