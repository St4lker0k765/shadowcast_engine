// File:		UIMessagesWindow.h
// Description:	Window with MP chat and Game Log ( with PDA messages in single and Kill Messages in MP)
// Created:		22.04.2005
// Author:		Serge Vynnychenko
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#include "StdAfx.h"

#include "UIMessagesWindow.h"
#include "../level.h"
#include "UIGameLog.h"
#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "UIPdaMsgListItem.h"
#include "UIColorAnimatorWrapper.h"
#include "../InfoPortion.h"
#include "../string_table.h"
#include "../game_news.h"
#include "UIInventoryUtilities.h"

CUIMessagesWindow::CUIMessagesWindow(){
	m_pChatLog = NULL;
	m_pGameLog = NULL;
	Init(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);
}

CUIMessagesWindow::~CUIMessagesWindow(){
	
}

void CUIMessagesWindow::AddLogMessage(const shared_str& msg){
	m_pGameLog->AddLogMessage(*msg);
}

void CUIMessagesWindow::PendingMode(bool const is_pending_mode)
{
	if (is_pending_mode)
	{
		if (m_in_pending_mode)
			return;
		
		m_pChatLog->SetWndPos	(m_pending_chat_log_pos);
		m_pChatLog->SetWndSize	(m_pending_chat_log_wnd_size);
		m_in_pending_mode		= true;
		return;
	}
	if (!m_in_pending_mode)
		return;
	
	m_pChatLog->SetWndPos	(m_inprogress_chat_log_pos);
	m_pChatLog->SetWndSize	(m_inprogress_chat_log_wnd_size);
	m_in_pending_mode		= false;
}

#define CHAT_LOG_LIST_PENDING "chat_log_list_pending"
void CUIMessagesWindow::Init(float x, float y, float width, float height){

	CUIXml		 xml;
	u32			color;
	CGameFont*	pFont;

	m_game_color_time    = color_argb( 255, 255, 255, 255 );
	m_game_color_caption = color_argb( 255, 189, 189, 224 );
	m_game_color_text    = color_argb( 255, 255, 255, 255 );

	xml.Load		(CONFIG_PATH, UI_PATH, "messages_window.xml");

	m_pGameLog = xr_new<CUIGameLog>();m_pGameLog->SetAutoDelete(true);
	m_pGameLog->Show(true);
	AttachChild(m_pGameLog);
	if ( IsGameTypeSingle() )
	{
		CUIXmlInit::InitScrollView(xml, "sp_log_list", 0, m_pGameLog);
		m_game_color_time    = CUIXmlInit::GetColor(xml, "game_time_color",    0, m_game_color_time);
		m_game_color_caption = CUIXmlInit::GetColor(xml, "game_caption_color", 0, m_game_color_caption);
		m_game_color_text    = CUIXmlInit::GetColor(xml, "game_text_color",    0, m_game_color_text);
	}
	else
	{
		m_pChatLog			= xr_new<CUIGameLog>(); m_pChatLog->SetAutoDelete(true);
		m_pChatLog->Show	(true);
		AttachChild			(m_pChatLog);

		CUIXmlInit::InitScrollView(xml, "mp_log_list", 0, m_pGameLog);
		CUIXmlInit::InitFont(xml, "mp_log_list:font", 0, color, pFont);
		m_pGameLog->SetTextAtrib(pFont, color);

		CUIXmlInit::InitScrollView(xml, "chat_log_list", 0, m_pChatLog);
		m_inprogress_chat_log_pos		=	m_pChatLog->GetWndPos();
		m_inprogress_chat_log_wnd_size	=	m_pChatLog->GetWndSize();

		m_in_pending_mode				= false;
		
		XML_NODE* pending_chat_list		= xml.NavigateToNode(CHAT_LOG_LIST_PENDING);
		if (pending_chat_list)
		{
			m_pending_chat_log_pos.x	= xml.ReadAttribFlt(CHAT_LOG_LIST_PENDING, 0, "x");
			m_pending_chat_log_pos.y	= xml.ReadAttribFlt(CHAT_LOG_LIST_PENDING, 0, "y");
			m_pending_chat_log_wnd_size.x	= xml.ReadAttribFlt(CHAT_LOG_LIST_PENDING, 0, "width");
			m_pending_chat_log_wnd_size.y	= xml.ReadAttribFlt(CHAT_LOG_LIST_PENDING, 0, "height");
		} else
		{
			m_pending_chat_log_pos		= m_inprogress_chat_log_pos;
			m_pending_chat_log_wnd_size	= m_inprogress_chat_log_wnd_size;
		}
		

		CUIXmlInit::InitFont(xml, "chat_log_list:font", 0, color, pFont);
		m_pChatLog->SetTextAtrib(pFont, color);
		
	}	

}

void CUIMessagesWindow::AddIconedPdaMessage(GAME_NEWS_DATA* news)
{
	CUIPdaMsgListItem *pItem			= m_pGameLog->AddPdaMessage();
	
	LPCSTR time_str = InventoryUtilities::GetTimeAsString( news->receive_time, InventoryUtilities::etpTimeToMinutes ).c_str();
	pItem->UITimeText.SetText			(time_str);
	pItem->UITimeText.AdjustWidthToText	();
	Fvector2 p							= pItem->UICaptionText.GetWndPos();
	p.x									= pItem->UITimeText.GetWndPos().x + pItem->UITimeText.GetWidth() + 3.0f;
	pItem->UICaptionText.SetWndPos		(p);
	pItem->UICaptionText.SetTextST		(news->news_caption.c_str());
	pItem->UIMsgText.SetTextST			(news->news_text.c_str());
	
	pItem->SetClrAnimDelay				(float(news->show_time));
	pItem->SetTextComplexMode			(true);
	pItem->UIIcon.InitTexture			(news->texture_name.c_str());
	pItem->UIMsgText.AdjustHeightToText	();
	
	float h1 = _max( pItem->UIIcon.GetHeight(), pItem->UIMsgText.GetWndPos().y + pItem->UIMsgText.GetHeight() );
	pItem->SetHeight( h1 + 3.0f );

	m_pGameLog->SendMessage(pItem,CHILD_CHANGED_SIZE);
}

void CUIMessagesWindow::AddChatMessage(shared_str msg, shared_str author)
{
	 m_pChatLog->AddChatMessage(*msg, *author);
}

void CUIMessagesWindow::SetChatOwner(game_cl_GameState* owner)
{
}

void CUIMessagesWindow::Update()
{
	CUIWindow::Update();
}