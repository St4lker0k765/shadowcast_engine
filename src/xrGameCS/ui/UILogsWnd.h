////////////////////////////////////////////////////////////////////////////
//	Module 		: UILogsWnd.h
//	Created 	: 25.04.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Logs (PDA) window class
////////////////////////////////////////////////////////////////////////////

#ifndef UI_PDA_LOGS_WND_H_INCLUDED
#define UI_PDA_LOGS_WND_H_INCLUDED

#include "UIWindow.h"
#include "UIWndCallback.h"

#include "../ai_space.h"
#include "../../xrServerEntitiesCS/alife_space.h"
#include "xrUIXmlParser.h"

class CUIStatic;
class CUIXml;
class CUIProgressBar;
class CUIFrameLineWnd;
class CUICharacterInfo;
class CUIScrollView;
class CUI3tButtonEx;
class CUICheckButton;
struct GAME_NEWS_DATA;
class CUINewsItemWnd;

class CUILogsWnd : public CUIWindow, public CUIWndCallback
{
private:
	typedef CUIWindow	inherited;

	CUIFrameLineWnd*	m_background;
	CUIStatic*			m_center_background;

	CUIStatic*			m_center_caption;
	CUICharacterInfo*	m_actor_ch_info;

	CUICheckButton*		m_filter_news;
	CUICheckButton*		m_filter_talk;
	
	CUIStatic*			m_date_caption;
	CUIStatic*			m_date;

	CUIStatic*			m_period_caption;
	CUIStatic*			m_period;

	ALife::_TIME_ID		m_start_game_time;
	ALife::_TIME_ID		m_selected_period;

	CUI3tButtonEx*		m_prev_period;
	CUI3tButtonEx*		m_next_period;
	bool				m_ctrl_press;
	
	CUIScrollView*		m_list;
	u32					m_previous_time;
	bool				m_need_reload;
	WINDOW_LIST			m_items_cache;
	xr_vector<u32>		m_news_in_queue;

	CUIWindow*			CreateItem			();
	CUIWindow*			ItemFromCache		();
	void				ItemToCache			(CUIWindow* w);
	CUIXml				m_uiXml;

public:
						CUILogsWnd			();
	virtual				~CUILogsWnd			();

			void		Init				();

	virtual void 		Show				( bool status );
	virtual void		Update				();
	virtual void		SendMessage			( CUIWindow* pWnd, s16 msg, void* pData );

	virtual bool		OnKeyboard			(int dik, EUIMessages keyboard_action);
	virtual bool		OnKeyboardHold		(int dik);

	IC		void		UpdateNews			()	{ m_need_reload = true; }
	void	xr_stdcall	PerformWork			();

protected:
			void		ReLoadNews			();
			void		AddNewsItem			( GAME_NEWS_DATA& news_data, CUIWindow* item );
	ALife::_TIME_ID		GetShiftPeriod		( ALife::_TIME_ID datetime, int shift_day );

			void xr_stdcall	UpdateChecks	( CUIWindow* w, void* d);
			void xr_stdcall	PrevPeriod		( CUIWindow* w, void* d);
			void xr_stdcall	NextPeriod		( CUIWindow* w, void* d);
	
			void 		on_scroll_keys		( int dik );

/*
protected:
	void		add_faction			( CUIXml& xml, shared_str const& faction_id );
	void		clear_all_factions		();
	bool	__stdcall	SortingLessFunction		( CUIWindow* left, CUIWindow* right );
*/
}; // class CUILogsWnd

#endif // UI_PDA_LOGS_WND_H_INCLUDED
