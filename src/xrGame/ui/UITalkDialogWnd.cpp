#include "stdafx.h"
#include "UITalkDialogWnd.h"

#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "UIScrollView.h"
#include "UI3tButton.h"
#include "UITalkWnd.h"
#include "UIInventoryUtilities.h"
#include "UIBtnHint.h"

#include "../game_news.h"
#include "../level.h"
#include "../actor.h"
#include "../alife_registry_wrappers.h"
#include "dinput.h"
#include "uihelper.h"

#define				TALK_XML				"talk.xml"

CUITalkDialogWnd::CUITalkDialogWnd()
	: m_uiXml(nullptr),
	m_pParent(nullptr),
	mechanic_mode(false),
	m_ClickedQuestionID(""),
	UIDialogFrameTop(nullptr),
	UIDialogFrameBottom(nullptr),
	m_btn_pos(),
	UIToExitButton(nullptr),
	UIOurIcon(nullptr),
	UIOthersIcon(nullptr),
	UIQuestionsList(nullptr),
	UIAnswersList(nullptr),
	m_pNameTextFont(nullptr),
	m_iNameTextColor(0),
	m_uOurReplicsColor(0) {
}

CUITalkDialogWnd::~CUITalkDialogWnd()
{
	xr_delete(m_uiXml);
}

void CUITalkDialogWnd::InitTalkDialogWnd()
{
	m_uiXml						= xr_new<CUIXml>();
	m_uiXml->Load				(CONFIG_PATH, UI_PATH, TALK_XML);
	CUIXmlInit					ml_init;

	CUIXmlInit::InitWindow		(*m_uiXml, "main", 0, this);

	if (m_uiXml->NavigateToNode("right_character_icon"))
	{
		UIOurIcon = UIHelper::CreateStatic(*m_uiXml, "right_character_icon", this);
		UIOurIcon->AttachChild(&UICharacterInfoLeft);
		UICharacterInfoLeft.InitCharacterInfo(Fvector2().set(0, 0), UIOurIcon->GetWndSize(), "talk_character.xml");
	}

	if (m_uiXml->NavigateToNode("left_character_icon"))
	{
		UIOthersIcon = UIHelper::CreateStatic(*m_uiXml, "left_character_icon", this);
		UIOthersIcon->AttachChild(&UICharacterInfoRight);
		UICharacterInfoRight.InitCharacterInfo(Fvector2().set(0, 0), UIOthersIcon->GetWndSize(), "talk_character.xml");
	}

	CUIWindow* answersParent = this;
	CUIWindow* questionsParent = this;

	// Фрейм с нащими фразами
	if (m_uiXml->NavigateToNode("frame_bottom"))
	{
		UIDialogFrameBottom = UIHelper::CreateStatic(*m_uiXml, "frame_bottom", this);
		questionsParent = UIDialogFrameBottom;
	}

	//основной фрейм диалога
	if (m_uiXml->NavigateToNode("frame_top"))
	{
		UIDialogFrameTop = UIHelper::CreateStatic(*m_uiXml, "frame_top", this);
		answersParent = UIDialogFrameTop;
	}

	//Ответы
	UIAnswersList				= xr_new<CUIScrollView>();
	UIAnswersList->SetAutoDelete(true);
	answersParent->AttachChild(UIAnswersList);
	CUIXmlInit::InitScrollView	(*m_uiXml, "answers_list", 0, UIAnswersList);
	UIAnswersList->SetWindowName("---UIAnswersList");

	//Вопросы
	UIQuestionsList				= xr_new<CUIScrollView>();
	UIQuestionsList->SetAutoDelete(true);
	questionsParent->AttachChild(UIQuestionsList);
	CUIXmlInit::InitScrollView	(*m_uiXml, "questions_list", 0, UIQuestionsList);
	UIQuestionsList->SetWindowName("---UIQuestionsList");


	//кнопка перехода в режим торговли
	AttachChild					(&UIToTradeButton);
	CUIXmlInit::Init3tButton	(*m_uiXml, "button", 0, &UIToTradeButton);

	m_btn_pos[0] = UIToTradeButton.GetWndPos();

	if (m_uiXml->NavigateToNode("button_exit"))
	{
		UIToExitButton = UIHelper::Create3tButton(*m_uiXml, "button_exit", this);
		m_btn_pos[1] = UIToExitButton->GetWndPos();
		m_btn_pos[2].x = (m_btn_pos[0].x + m_btn_pos[1].x) / 2.0f;
		m_btn_pos[2].y = m_btn_pos[0].y;
	}
	else
	{
		m_btn_pos[1] = m_btn_pos[0];
		m_btn_pos[2] = m_btn_pos[0];
	}

	// шрифт для индикации имени персонажа в окне разговора
	CUIXmlInit::InitFont		(*m_uiXml, "font", 0, m_iNameTextColor, m_pNameTextFont);

	CGameFont * pFont			= NULL;
	CUIXmlInit::InitFont		(*m_uiXml, "font", 1, m_uOurReplicsColor, pFont);


	SetWindowName				("----CUITalkDialogWnd");

	Register					(&UIToTradeButton);
	AddCallbackStr				("question_item",LIST_ITEM_CLICKED,CUIWndCallback::void_function(this, &CUITalkDialogWnd::OnQuestionClicked));

	// XXX: Support stringized (trade_btn, upgrade_btn, exit_btn) callbacks
	// To support it we just should check that it doesn't called twice
	//AddCallbackStr("trade_btn", BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITalkDialogWnd::OnTradeClicked));
	AddCallback(&UIToTradeButton, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITalkDialogWnd::OnTradeClicked));

	//AddCallbackStr("upgrade_btn", BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITalkDialogWnd::OnUpgradeClicked));

	if (UIToExitButton)
	{
		AddCallback(UIToExitButton, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITalkDialogWnd::OnExitClicked));
		//AddCallbackStr("exit_btn", BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITalkDialogWnd::OnExitClicked));
	}
}

	
void CUITalkDialogWnd::Show()
{
	InventoryUtilities::SendInfoToActor				("ui_talk_show");
	InventoryUtilities::SendInfoToLuaScripts		("ui_talk_show");
	inherited::Show									(true);
	inherited::Enable								(true);

	ResetAll										();
}

void CUITalkDialogWnd::Hide()
{
	InventoryUtilities::SendInfoToActor				("ui_talk_hide");
	InventoryUtilities::SendInfoToLuaScripts		("ui_talk_hide");
	inherited::Show									(false);
	inherited::Enable								(false);
	g_btnHint->Discard								();
}

void CUITalkDialogWnd::OnQuestionClicked(CUIWindow* w, void*)
{
	m_ClickedQuestionID = ((CUIQuestionItem*)w)->m_s_value;
	GetMessageTarget()->SendMessage(this, TALK_DIALOG_QUESTION_CLICKED);
}

void CUITalkDialogWnd::OnExitClicked(CUIWindow* w, void*)
{
	m_pParent->StopTalk();
}

void CUITalkDialogWnd::OnTradeClicked(CUIWindow* w, void*)
{
	if ( mechanic_mode )
	{
		GetTop()->SendMessage(this, TALK_DIALOG_UPGRADE_BUTTON_CLICKED);
	}
	else
	{
		GetTop()->SendMessage(this, TALK_DIALOG_TRADE_BUTTON_CLICKED);
	}
}

void CUITalkDialogWnd::OnUpgradeClicked(CUIWindow* w, void*)
{
	GetTop()->SendMessage(this, TALK_DIALOG_UPGRADE_BUTTON_CLICKED);
}

void CUITalkDialogWnd::SetTradeMode()
{
	OnTradeClicked( &UIToTradeButton, 0 );
}

//пересылаем сообщение родительскому окну для обработки
//и фильтруем если оно пришло от нашего дочернего окна
void CUITalkDialogWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
	CUIWndCallback::OnEvent(pWnd, msg, pData);
}

void CUITalkDialogWnd::ClearAll()
{
	UIAnswersList->Clear	();
	ClearQuestions			();
}

void CUITalkDialogWnd::ClearQuestions()
{
	UIQuestionsList->Clear();
}


void CUITalkDialogWnd::AddQuestion(LPCSTR str, LPCSTR value, int number, bool b_finalizer)
{
	CUIQuestionItem* itm			= xr_new<CUIQuestionItem>(m_uiXml,"question_item");
	itm->Init						(value, str);
	++number; //zero-based index
	if(number<=10)
	{
		string16 buff;
		xr_sprintf						(buff, "%d.", (number==10)?0:number);
		if (itm->m_num_text)
			itm->m_num_text->SetText		(buff);
		itm->m_text->SetAccelerator		(DIK_ESCAPE+number, 0);
	}
	if(b_finalizer)
	{
		itm->m_text->SetAccelerator		(kQUIT, 2);
		itm->m_text->SetAccelerator		(kUSE, 3);
	}

	itm->SetWindowName				("question_item");
	UIQuestionsList->AddWindow		(itm, true);
	Register						(itm);
}


void CUITalkDialogWnd::AddAnswer(LPCSTR SpeakerName, LPCSTR str, bool bActor)
{
	CUIAnswerItem* itm				= xr_new<CUIAnswerItem>(m_uiXml,bActor?"actor_answer_item":"other_answer_item");
	itm->Init						(str, SpeakerName);
	UIAnswersList->AddWindow		(itm, true);
	UIAnswersList->ScrollToEnd		();
	
	GAME_NEWS_DATA	news_data;
	news_data.news_caption = SpeakerName;

	xr_string res;
	res = "%c[250,255,232,208]";
	res += str;
	news_data.news_text	= res.c_str();

	news_data.m_type				= GAME_NEWS_DATA::eTalk;
	CUICharacterInfo& ci			= bActor ? UICharacterInfoLeft : UICharacterInfoRight; 

	news_data.texture_name			= ci.IconName();
	news_data.receive_time			= Level().GetGameTime();

	Actor()->game_news_registry->registry().objects().push_back(news_data);
}

void CUITalkDialogWnd::AddIconedAnswer(LPCSTR caption, LPCSTR text, LPCSTR texture_name, LPCSTR templ_name)
{
	CUIAnswerItemIconed* itm		= xr_new<CUIAnswerItemIconed>(m_uiXml,templ_name);
	itm->Init						(text, caption, texture_name);
	UIAnswersList->AddWindow		(itm, true);
	UIAnswersList->ScrollToEnd		();
	
	GAME_NEWS_DATA	news_data;
	news_data.news_caption			= caption;
	news_data.news_text._set		( text );

	news_data.m_type				= GAME_NEWS_DATA::eTalk;
	news_data.texture_name			= texture_name;
	news_data.receive_time			= Level().GetGameTime();

	Actor()->game_news_registry->registry().objects().push_back(news_data);
}

void CUITalkDialogWnd::SetOsoznanieMode(bool b)
{
	if (UIOurIcon)
		UIOurIcon->Show(!b);

	if (UIOthersIcon)
		UIOthersIcon->Show(!b);

	UIAnswersList->Show(!b);

	if (UIDialogFrameTop)
		UIDialogFrameTop->Show(!b);

	UIToTradeButton.Show(!b);
	if ( mechanic_mode )
	{
		UIToTradeButton.m_hint_text = "ui_st_upgrade_hint";
		UIToTradeButton.TextItemControl()->SetTextST( "ui_st_upgrade" );
	}
	else
	{
		UIToTradeButton.m_hint_text = "ui_st_trade_hint";
		UIToTradeButton.TextItemControl()->SetTextST( "ui_st_trade" );
	}
}

void CUITalkDialogWnd::UpdateButtonsLayout(bool b_disable_break, bool trade_enabled)
{
	UIToTradeButton.Show		(trade_enabled);
}

void CUIQuestionItem::SendMessage				(CUIWindow* pWnd, s16 msg, void* pData)
{
	CUIWndCallback::OnEvent(pWnd, msg, pData);
}

CUIQuestionItem::CUIQuestionItem(CUIXml* xml_doc, LPCSTR path)
{
	CUIXmlInit::InitWindow			(*xml_doc, path, 0, this);

	m_min_height					= xml_doc->ReadAttribFlt(path,0,"min_height",15.0f);

	string512						str;
	strconcat						(sizeof(str),str,path,":content_text");
	m_text = UIHelper::Create3tButton(*xml_doc, str, this);

	Register						(m_text);
	AddCallback						(m_text,BUTTON_CLICKED,CUIWndCallback::void_function(this, &CUIQuestionItem::OnTextClicked));

	strconcat						(sizeof(str),str,path,":num_text");
	if (xml_doc->NavigateToNode(str))
		m_num_text = UIHelper::CreateTextWnd(*xml_doc, str, this);
}

void CUIQuestionItem::Init			(LPCSTR val, LPCSTR text)
{
	m_s_value						= val;
	m_text->TextItemControl()->SetText(text);
	m_text->AdjustHeightToText		();
	float new_h						= _max(m_min_height, m_text->GetWndPos().y+m_text->GetHeight());
	SetHeight						(new_h);
}

void	CUIQuestionItem::OnTextClicked(CUIWindow* w, void*)
{
	GetMessageTarget()->SendMessage(this, LIST_ITEM_CLICKED, (void*)this);
}


CUIAnswerItem::CUIAnswerItem			(CUIXml* xml_doc, LPCSTR path)
{
	CUIXmlInit::InitWindow(*xml_doc, path, 0, this);

	m_min_height					= xml_doc->ReadAttribFlt(path,0,"min_height",15.0f);
	m_bottom_footer					= xml_doc->ReadAttribFlt(path,0,"bottom_footer",0.0f);

	string512 str;

	strconcat(sizeof(str),str,path,":content_text");
	m_text = UIHelper::CreateTextWnd(*xml_doc, str, this);

	strconcat(sizeof(str),str,path,":name_caption");
	m_name = UIHelper::CreateTextWnd(*xml_doc, str, this);

	SetAutoDelete					(true);
}

void CUIAnswerItem::Init			(LPCSTR text, LPCSTR name)
{
	m_name->SetText					(name);
	m_text->SetText					(text);
	m_text->AdjustHeightToText		();
	float new_h						= _max(m_min_height, m_text->GetWndPos().y+m_text->GetHeight());
	new_h							+= m_bottom_footer;
	SetHeight						(new_h);
}

CUIAnswerItemIconed::CUIAnswerItemIconed		(CUIXml* xml_doc, LPCSTR path)
:CUIAnswerItem(xml_doc, path)
{
	m_icon							= xr_new<CUIStatic>();m_icon->SetAutoDelete(true);
	AttachChild						(m_icon);

	string512						str;
	CUIXmlInit						xml_init;

	strconcat						(sizeof(str),str,path,":msg_icon");
	xml_init.InitStatic				(*xml_doc, str, 0, m_icon);
}

void CUIAnswerItemIconed::Init		(LPCSTR text, LPCSTR name, LPCSTR texture_name)
{
	xr_string res;
	res += name;
	res += "\\n %c[250,255,232,208]";
	res += text;

	inherited::Init					(res.c_str(), "");
	m_icon->InitTexture				(texture_name);
	m_icon->TextureOn				();
	m_icon->SetStretchTexture		(true);
}
