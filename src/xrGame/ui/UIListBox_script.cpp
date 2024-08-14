#include "pch_script.h"
#include "UIListBox.h"
#include "UIListBoxItem.h"
#include "UIListBoxItemMsgChain.h"
#include "ServerList.h"

using namespace luabind;


struct CUIListBoxItemWrapper : public CUIListBoxItem, public luabind::wrap_base 
{
	CUIListBoxItemWrapper(float h):CUIListBoxItem(h){}
};

struct CUIListBoxItemMsgChainWrapper : public CUIListBoxItemMsgChain, public luabind::wrap_base
{
	CUIListBoxItemMsgChainWrapper(float h) : CUIListBoxItemMsgChain(h) {}
};


#pragma optimize("s",on)
void CUIListBox::script_register(lua_State *L)
{

	module(L)
	[

		class_<CUIListBox, CUIScrollView>("CUIListBox")
		.def(							constructor<>())
		.def("ShowSelectedItem",		&CUIListBox::Show)
		.def("RemoveAll",				&CUIListBox::Clear)
		.def("GetSize",					&CUIListBox::GetSize)
		.def("GetSelectedItem",			&CUIListBox::GetSelectedItem)
		.def("GetSelectedIndex",		&CUIListBox::GetSelectedIDX)		

		.def("GetItemByIndex",			&CUIListBox::GetItemByIDX)		
		.def("GetItem",					&CUIListBox::GetItem)		
		.def("RemoveItem",				&CUIListBox::RemoveWindow)
		.def("AddTextItem",				&CUIListBox::AddTextItem)
		.def("AddExistingItem",         &CUIListBox::AddExistingItem, adopt<2>()),

		class_<CUIListBoxItem, CUIFrameLineWnd, CUIListBoxItemWrapper>("CUIListBoxItem")
		.def(							constructor<float>())
		.def("GetTextItem",             &CUIListBoxItem::GetTextItem)
		.def("AddTextField",            &CUIListBoxItem::AddTextField)
		.def("AddIconField",            &CUIListBoxItem::AddIconField)
		.def("SetTextColor",			&CUIListBoxItem::SetTextColor),

		class_<CUIListBoxItemMsgChain, CUIListBoxItem, CUIListBoxItemMsgChainWrapper>("CUIListBoxItemMsgChain")
		.def(							constructor<float>()),

		class_<SServerFilters>("SServerFilters")
		.def(							constructor<>())
		.def_readwrite("empty",				&SServerFilters::empty)
		.def_readwrite("full",				&SServerFilters::full)
		.def_readwrite("with_pass",			&SServerFilters::with_pass)
		.def_readwrite("without_pass",		&SServerFilters::without_pass)
		.def_readwrite("without_ff",		&SServerFilters::without_ff)
		.def_readwrite("listen_servers",	&SServerFilters::listen_servers),

		class_<CServerList, CUIWindow>("CServerList")
		.def(							constructor<>())
		.enum_("enum_connect_errcode")
		[
			value("ece_unique_nick_not_registred", int(ece_unique_nick_not_registred)),
			value("ece_unique_nick_expired", int(ece_unique_nick_expired))
		]
		.def("SetConnectionErrCb",		&CServerList::SetConnectionErrCb)
		.def("ConnectToSelected",		&CServerList::ConnectToSelected)
		.def("SetFilters",				&CServerList::SetFilters)
		.def("SetPlayerName",			&CServerList::SetPlayerName)
		.def("RefreshList",				&CServerList::RefreshGameSpyList)
		.def("RefreshQuick",			&CServerList::RefreshQuick)
		.def("ShowServerInfo",			&CServerList::ShowServerInfo)
		.def("NetRadioChanged",			&CServerList::NetRadioChanged)
		.def("SetSortFunc",				&CServerList::SetSortFunc)
	];
}