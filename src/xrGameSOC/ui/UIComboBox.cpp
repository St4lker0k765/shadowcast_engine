#include "StdAfx.h"
#include "UIComboBox.h"
#include "UITextureMaster.h"
#include "UIScrollBar.h"
#include "UIXmlInit.h"

CUIComboBox::CUIComboBox()
{
	AttachChild			(&m_frameLine);
	AttachChild			(&m_text);

	AttachChild			(&m_list_frame);
	m_list_frame.AttachChild(&m_list_box);

	m_iListHeight		= 0;
	m_bInited			= false;
	m_eState			= LIST_FONDED;
	m_textColor[0]		= 0xff00ff00;
}

CUIComboBox::~CUIComboBox()
{}

void CUIComboBox::SetListLength(int length)
{
	R_ASSERT(0 == m_iListHeight);
	m_iListHeight = length;
}

void CUIComboBox::InitComboBox(Fvector2 pos, float width)
{
	float lb_text_offset				= 5.0f;

	CUIXml xml_doc;
	xml_doc.Load(CONFIG_PATH, UI_PATH, "combobox.xml");
	float comboBoxHeight = xml_doc.ReadFlt("height", 0, 20.0f);
	LPCSTR frameLineDefault = xml_doc.Read("frameline_default", 0, "ui_inGame2_combobox_linetext");
	LPCSTR frameLineHighlighted = xml_doc.Read("frameline_highlighted", 0, "ui_inGame2_combobox_linetext");
	LPCSTR listBoxTexture = xml_doc.Read("list_box", 0, "ui_inGame2_combobox_line");
	LPCSTR listFrameTexture = xml_doc.Read("list_frame", 0, "ui_inGame2_combobox");
	string_path listBoxTextureHeight;
	strconcat(sizeof(listBoxTextureHeight), listBoxTextureHeight, listBoxTexture, "_b");

	m_bInited = true;
	if (0 == m_iListHeight)
		m_iListHeight = 4;

	CUIWindow::SetWndPos				(pos);
	CUIWindow::SetWndSize				(Fvector2().set(width, comboBoxHeight));

	m_frameLine.Init					(0,0, width, comboBoxHeight);

    m_frameLine.InitHighlightedState(frameLineHighlighted);
	m_frameLine.InitEnabledState(frameLineDefault);

	// Edit Box on left side of frame line
	m_text.SetWndPos					(Fvector2().set(lb_text_offset,0.0f));
	m_text.SetWndSize					(Fvector2().set(width-lb_text_offset, comboBoxHeight));

	m_text.SetVTextAlignment			(valCenter);
	m_text.SetTextColor					(m_textColor[0]);
	m_text.Enable						(false);

	// height of list equal to height of ONE element
    float item_height = CUITextureMaster::GetTextureHeight(listBoxTextureHeight);

	m_list_box.SetWndPos				(Fvector2().set(lb_text_offset,0.0f));
	m_list_box.SetWndSize				(Fvector2().set(width-lb_text_offset, item_height*m_iListHeight));
	m_list_box.Init			();
	m_list_box.SetTextColor				(m_textColor[0]);
	m_list_box.SetItemHeight(item_height);

	m_list_box.SetSelectionTexture(listBoxTexture);

	// frame(texture) for list
	m_list_frame.InitTexture(listFrameTexture);

	m_list_frame.SetWndSize				(Fvector2().set(width, m_list_box.GetItemHeight()*m_iListHeight) );
	m_list_frame.SetWndPos				(Fvector2().set(0.0f, comboBoxHeight));

	m_list_box.Show						(true);
	m_list_frame.Show					(false);
	m_list_box.SetMessageTarget			(this);
}

#include "uilistboxitem.h"
CUIListBoxItem* CUIComboBox::AddItem_(LPCSTR str, int _data)
{
    R_ASSERT2			(m_bInited, "Can't add item to ComboBox before Initialization");
	CUIListBoxItem* itm = m_list_box.AddItem(str);
	itm->SetData		((void*)(__int64)_data);
	return				itm;
}


void CUIComboBox::OnListItemSelect()
{
	m_text.SetText			(m_list_box.GetSelectedText());    
	CUIListBoxItem* itm		= m_list_box.GetSelectedItem();
	
	int bk_itoken_id		= m_itoken_id;
	
	m_itoken_id				= (int)(__int64)itm->GetData();
	ShowList				(false);

	if(bk_itoken_id!=m_itoken_id)
	{
		SaveValue		();
		GetMessageTarget()->SendMessage(this, LIST_ITEM_SELECT, NULL);
	}
}

void CUIComboBox::disable_id(int id)
{
	if(m_disabled.end()==std::find(m_disabled.begin(),m_disabled.end(),id))
		m_disabled.push_back(id);
}

void CUIComboBox::enable_id(int id)
{
	xr_vector<int>::iterator it = std::find(m_disabled.begin(),m_disabled.end(),id);

	if(m_disabled.end()!=it)
		m_disabled.erase(it);
}

#include "../string_table.h"
void CUIComboBox::SetCurrentValue()
{
	m_list_box.Clear		();
	xr_token* tok		= GetOptToken();

	while (tok->name)
	{		
		if(m_disabled.end()==std::find(m_disabled.begin(),m_disabled.end(),tok->id))
		{
			AddItem_(tok->name, tok->id);
		}
		tok++;
	}

	LPCSTR cur_val		= *CStringTable().translate( GetOptTokenValue());
	m_text.SetText		( cur_val );
	m_list_box.SetSelectedText( cur_val );
	
	CUIListBoxItem* itm	= m_list_box.GetSelectedItem();
	if(itm)
		m_itoken_id			= (int)(__int64)itm->GetData();
	else
		m_itoken_id			= 1; //first
}

void CUIComboBox::SaveValue()
{
	CUIOptionsItem::SaveValue	();
	xr_token* tok				= GetOptToken();
	LPCSTR	cur_val				= get_token_name(tok, m_itoken_id);
	SaveOptTokenValue			(cur_val);
}

bool CUIComboBox::IsChanged()
{
	return				(m_backup_itoken_id != m_itoken_id);
}

LPCSTR CUIComboBox::GetText()
{
	return m_text.GetText	();
}

void CUIComboBox::SetItem(int idx)
{
	m_list_box.SetSelectedIDX	(idx);
	CUIListBoxItem* itm		= m_list_box.GetSelectedItem();
	m_itoken_id				= (int)(__int64)itm->GetData();

	m_text.SetText			(m_list_box.GetSelectedText());
}

void CUIComboBox::OnBtnClicked()
{
	ShowList				(!m_list_frame.IsShown());
}

void CUIComboBox::ShowList(bool bShow)
{
    if (bShow)
	{
		SetHeight				(m_text.GetHeight() + m_list_box.GetHeight());

//.		m_list_box.Show			(true);
		m_list_frame.Show		(true);

		m_eState				= LIST_EXPANDED;

		GetParent()->SetCapture	(this, true);
//.		Device.seqRender.Add		(this, 3);
	}
	else
	{
//.		m_list_box.Show				(false);
		m_list_frame.Show			(false);
		SetHeight					(m_frameLine.GetHeight());
		m_eState					= LIST_FONDED;
		GetParent()->SetCapture		(this, false);
	}
}

void CUIComboBox::Update()
{
	CUIWindow::Update	();
	if (!m_bIsEnabled)
	{
		SetState		(S_Disabled);
		m_text.SetTextColor(m_textColor[1]);
	}
	else
	{
		m_text.SetTextColor(m_textColor[0]);
		
		if(m_list_frame.IsShown())
		{
			Device.seqRender.Remove		(this);
			Device.seqRender.Add		(this, 3);
		}
	}

}

void CUIComboBox::OnFocusLost()
{
	CUIWindow::OnFocusLost();
	if (m_bIsEnabled)
        SetState(S_Enabled);

}

void CUIComboBox::OnFocusReceive()
{
	CUIWindow::OnFocusReceive();
    if (m_bIsEnabled)
        SetState(S_Highlighted);
}

bool CUIComboBox::OnMouse(float x, float y, EUIMessages mouse_action){
	if(CUIWindow::OnMouse(x, y, mouse_action)) 
		return true;

	bool bCursorOverScb = false;
	bCursorOverScb		= m_list_box.ScrollBar()->CursorOverWindow();
	switch (m_eState){
		case LIST_EXPANDED:			

			if (  (!bCursorOverScb) &&  mouse_action == WINDOW_LBUTTON_DOWN)
			{
                ShowList(false);
				return true;
			}
			break;
		case LIST_FONDED:
			if(mouse_action==WINDOW_LBUTTON_DOWN)
			{
				OnBtnClicked();
				return true;			
			}break;
		default:
			break;
	}	
	 

        return false;
}

void CUIComboBox::SetState(UIState state)
{
	m_frameLine.SetState	(state);
}

void CUIComboBox::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
	CUIWindow::SendMessage	(pWnd, msg, pData);

	switch (msg)
	{
		case LIST_ITEM_CLICKED:
			if (pWnd == &m_list_box)
				OnListItemSelect();	
			break;
		default:
			break;
	}
}

void CUIComboBox::SeveBackUpValue()
{
	m_backup_itoken_id = m_itoken_id;
}

void CUIComboBox::Undo()
{
	SetItem				(m_backup_itoken_id);
	SaveValue			();
	SetCurrentValue		();
}

void CUIComboBox::OnRender()
{
	if(IsShown())
	{
		if(m_list_frame.IsShown())
		{
			UI()->PushScissor			(UI()->ScreenRect(),true);
			m_list_frame.Draw			();
			UI()->PopScissor			();
			Device.seqRender.Remove		(this);
		}
	}
}

void CUIComboBox::Draw()
{
	CUIWindow::Draw			();
}
