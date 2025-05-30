// file:		UIustomSpin.cpp
// description:	base class for CSpinNum & CSpinText
// created:		15.06.2005
// author:		Serge Vynnychenko
// mail:		narrator@gsc-game.kiev.ua
//
// copyright 2005 GSC Game World

#include "StdAfx.h"
#include "UI3tButton.h"
#include "UIFrameLineWnd.h"
#include "UILines.h"
#include "UICustomSpin.h"
#include "../uifontdefines.h"

#define SPIN_HEIGHT 22
#define BTN_SIZE 11

CUICustomSpin::CUICustomSpin()
{
	m_pFrameLine				= xr_new<CUIFrameLineWnd>();
	m_pBtnUp					= xr_new<CUI3tButton>();
	m_pBtnDown					= xr_new<CUI3tButton>();
	m_pLines					= xr_new<CUILines>();

    m_pFrameLine->SetAutoDelete	(true);
	m_pBtnUp->SetAutoDelete		(true);
	m_pBtnDown->SetAutoDelete	(true);

	AttachChild					(m_pFrameLine);
	AttachChild					(m_pBtnUp);
	AttachChild					(m_pBtnDown);
	m_pLines->SetTextAlignment	(CGameFont::alLeft);
	m_pLines->SetVTextAlignment	(valCenter);
	m_pLines->SetFont			(UI()->Font()->GetFont(LETTERICA16_FONT_NAME));
	m_pLines->SetTextColor		(color_argb(255,235,219,185));

	m_time_begin				= 0;
	m_p_delay					= 500;
	m_u_delay					= 0;

	m_textColor[0]				= color_argb(255,235,219,185);
	m_textColor[1]				= color_argb(255,100,100,100);
}

CUICustomSpin::~CUICustomSpin()
{
	xr_delete					(m_pLines);
}

void CUICustomSpin::Init(float x, float y, float width, float height){
	CUIWindow::Init				(x,y,width,SPIN_HEIGHT);
	m_pFrameLine->Init			(0,0,width, SPIN_HEIGHT);
	m_pFrameLine->InitTexture	("ui_spiner");
	m_pBtnUp->Init				(width - BTN_SIZE - 1, 0, BTN_SIZE, BTN_SIZE);
	m_pBtnUp->InitTexture		("ui_spiner_button_t");
	m_pBtnDown->Init			(width - BTN_SIZE - 1, BTN_SIZE + 1, BTN_SIZE, BTN_SIZE);
	m_pBtnDown->InitTexture		("ui_spiner_button_b");

	m_pLines->Init				(0,0,width - BTN_SIZE - 10, SPIN_HEIGHT);
}

void CUICustomSpin::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
	if (BUTTON_CLICKED == msg)
	{
		if (m_pBtnUp == pWnd)
		{
			OnBtnUpClick();			
		}
		else if (m_pBtnDown == pWnd)
		{
			OnBtnDownClick();			
		}
	}
}

void CUICustomSpin::Enable(bool status)
{
	CUIWindow::Enable		(status);
	m_pBtnDown->Enable		(status);
	m_pBtnUp->Enable		(status);

	if (!status)
		m_pLines->SetTextColor(m_textColor[0]);	// enabled color
	else
		m_pLines->SetTextColor(m_textColor[1]);	// disabled color
}

void CUICustomSpin::OnBtnUpClick()
{
	GetMessageTarget()->SendMessage(this, BUTTON_CLICKED);
}

void CUICustomSpin::OnBtnDownClick()
{
	GetMessageTarget()->SendMessage(this, BUTTON_CLICKED);
}

void CUICustomSpin::Draw()
{
	CUIWindow::Draw	();
	Fvector2		pos;
	GetAbsolutePos	(pos);
	m_pLines->Draw	(pos.x + 3, pos.y);
}

void CUICustomSpin::Update()
{
	CUIWindow::Update();
	if(!m_pBtnUp->CursorOverWindow())
		m_pBtnUp->SetButtonMode(CUIButton::BUTTON_NORMAL);
	if(!m_pBtnDown->CursorOverWindow())
		m_pBtnDown->SetButtonMode(CUIButton::BUTTON_NORMAL);
    
	if (CUIButton::BUTTON_PUSHED == m_pBtnUp->GetButtonsState() && m_pBtnUp->CursorOverWindow())
	{		
		if (m_time_begin < Device.dwTimeContinual - m_p_delay)
		{
			m_time_begin		= Device.dwTimeContinual;
			float tmp			= float(m_u_delay);
			float step			= powf(tmp,0.7f);
			while(tmp>0)
			{
				IncVal			();
				tmp				-= step;	
			};
			
			m_u_delay			+= 50;
			
			if(m_p_delay>50)
				m_p_delay -= 50;
		}
	}else
	if (CUIButton::BUTTON_PUSHED == m_pBtnDown->GetButtonsState() && m_pBtnDown->CursorOverWindow())
	{
		if (m_time_begin < Device.dwTimeContinual - m_p_delay)
		{
			m_time_begin		= Device.dwTimeContinual;
			float tmp			= float(m_u_delay);
			float step			= powf(tmp,0.7f);
			while(tmp>0)
			{
				DecVal			();
				tmp				-= step;	
			};
			
			m_u_delay			+= 50;
			
			if(m_p_delay>50)
				m_p_delay -= 50;
		}
	}else
	{
		m_p_delay				= 500;
		m_u_delay				= 0;
		m_time_begin			= 0;
	}	

	if (IsEnabled())
	{
        m_pBtnUp->Enable		(CanPressUp());
		m_pBtnDown->Enable		(CanPressDown());
		m_pLines->SetTextColor	(m_textColor[0]);
	}else
	{
		m_pBtnUp->Enable		(false);
		m_pBtnDown->Enable		(false);
		m_pLines->SetTextColor	(m_textColor[1]);
	}
}

LPCSTR CUICustomSpin::GetText()
{
	return m_pLines->GetText();
}

void CUICustomSpin::SetTextColor(u32 color)
{
	m_textColor[0] = color;
}

void CUICustomSpin::SetTextColorD(u32 color)
{
	m_textColor[1] = color;
}