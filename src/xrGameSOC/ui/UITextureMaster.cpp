// file:		UITextureMaster.h
// description:	holds info about shared textures. able to initialize external controls
//				through IUITextureControl interface
// created:		11.05.2005
// author:		Serge Vynnychenko
// mail:		narrator@gsc-game.kiev.ua
//
// copyright 2005 GSC Game World


#include "StdAfx.h"
#include "UITextureMaster.h"
#include "uiabstract.h"
#include "xrUIXmlParser.h"
#include "../Include/xrRender/UIShader.h"

xr_map<shared_str, TEX_INFO>	CUITextureMaster::m_textures;
#ifdef DEBUG
u32									CUITextureMaster::m_time = 0;
#endif
xr_map<sh_pair, ui_shader>		CUITextureMaster::m_shaders;

void CUITextureMaster::WriteLog(){
#ifdef DEBUG
	Msg("UI texture manager work time is %d ms", m_time);
#endif
}
void CUITextureMaster::ParseShTexInfo(LPCSTR xml_file){
	CUIXml xml;
	xml.Load(CONFIG_PATH, UI_PATH, xml_file);
	shared_str file = xml.Read("file_name",0,""); 

//	shared_textures_it	sht_it = m_shTex.find(texture);
//	if (m_shTex.end() == sht_it)
//	{
		int num = xml.GetNodesNum("",0,"texture");
//		regions regs;
		for (int i = 0; i<num; i++)
		{
			TEX_INFO info;

			info.file = file;

			info.rect.x1 = xml.ReadAttribFlt("texture",i,"x");
			info.rect.x2 = xml.ReadAttribFlt("texture",i,"width") + info.rect.x1;
			info.rect.y1 = xml.ReadAttribFlt("texture",i,"y");
			info.rect.y2 = xml.ReadAttribFlt("texture",i,"height") + info.rect.y1;
			shared_str id = xml.ReadAttrib("texture",i,"id");

			m_textures.insert(std::make_pair(id,info));
		}
//		m_shTex.insert(std::make_pair(texture, regs));
//	}
}

void CUITextureMaster::ParseShTexInfoNew(LPCSTR xml_file)
{
	CUIXml						xml;
	xml.Load(CONFIG_PATH, "ui\\textures_descr", xml_file);

	int files_num = xml.GetNodesNum("", 0, "file");


	for (int fi = 0; fi < files_num; ++fi)
	{
		XML_NODE* root_node = xml.GetLocalRoot();
		shared_str file = xml.ReadAttrib("file", fi, "name");

		XML_NODE* node = xml.NavigateToNode("file", fi);

		int num = xml.GetNodesNum(node, "texture");
		for (int i = 0; i < num; i++)
		{
			TEX_INFO info;

			info.file = file;

			info.rect.x1 = xml.ReadAttribFlt(node, "texture", i, "x");
			info.rect.x2 = xml.ReadAttribFlt(node, "texture", i, "width") + info.rect.x1;
			info.rect.y1 = xml.ReadAttribFlt(node, "texture", i, "y");
			info.rect.y2 = xml.ReadAttribFlt(node, "texture", i, "height") + info.rect.y1;
			shared_str id = xml.ReadAttrib(node, "texture", i, "id");
			m_textures.insert(std::make_pair(id, info));
		}

		xml.SetLocalRoot(root_node);
	}
}

bool CUITextureMaster::IsSh(const char* texture_name){
	return strstr(texture_name,"\\") ? false : true;
}

void CUITextureMaster::InitTexture(const char* texture_name, IUISimpleTextureControl* tc){
#ifdef DEBUG
	CTimer T;
	T.Start();
#endif

	xr_map<shared_str, TEX_INFO>::iterator	it;

	it = m_textures.find(texture_name);

	if (it != m_textures.end())
	{
		tc->CreateShader(*((*it).second.file));
		tc->SetOriginalRectEx((*it).second.rect);
#ifdef DEBUG
		m_time += T.GetElapsed_ms();
#endif
		return;
	}
	tc->CreateShader(texture_name);
#ifdef DEBUG
	m_time += T.GetElapsed_ms();
#endif
}

void CUITextureMaster::InitTexture(const char* texture_name, const char* shader_name, IUISimpleTextureControl* tc){
#ifdef DEBUG
	CTimer T;
	T.Start();
#endif

	xr_map<shared_str, TEX_INFO>::iterator	it;

	it = m_textures.find(texture_name);

	if (it != m_textures.end())
	{
		tc->CreateShader(*((*it).second.file), shader_name);
		tc->SetOriginalRectEx((*it).second.rect);
#ifdef DEBUG
		m_time += T.GetElapsed_ms();
#endif
		return;
	}
	tc->CreateShader(texture_name, shader_name);
#ifdef DEBUG
	m_time += T.GetElapsed_ms();
#endif
}

float CUITextureMaster::GetTextureHeight(const char* texture_name){
	xr_map<shared_str, TEX_INFO>::iterator	it;
	it = m_textures.find(texture_name);

	if (it != m_textures.end())
		return (*it).second.rect.height();
	R_ASSERT3(false,"CUITextureMaster::GetTextureHeight Can't find texture", texture_name);
	return 0;
}

Frect CUITextureMaster::GetTextureRect(const char* texture_name){
	xr_map<shared_str, TEX_INFO>::iterator	it;
	it = m_textures.find(texture_name);
	if (it != m_textures.end())
		return (*it).second.rect;

	R_ASSERT3(false,"CUITextureMaster::GetTextureHeight Can't find texture", texture_name);
	return Frect();
}

float CUITextureMaster::GetTextureWidth(const char* texture_name){
	xr_map<shared_str, TEX_INFO>::iterator	it;
	it = m_textures.find(texture_name);

	if (it != m_textures.end())
		return (*it).second.rect.width();
	R_ASSERT3(false,"CUITextureMaster::GetTextureHeight Can't find texture", texture_name);
	return 0;
}

LPCSTR CUITextureMaster::GetTextureFileName(const char* texture_name){
	xr_map<shared_str, TEX_INFO>::iterator	it;
	it = m_textures.find(texture_name);

	if (it != m_textures.end())
		return *((*it).second.file);
	R_ASSERT3(false,"CUITextureMaster::GetTextureFileName Can't find texture", texture_name);
	return 0;
}

TEX_INFO CUITextureMaster::FindItem(LPCSTR texture_name, LPCSTR def_texture_name)
{
	xr_map<shared_str, TEX_INFO>::iterator	it;
	it = m_textures.find(texture_name);

	if (it != m_textures.end())
		return (it->second);
	else{
		R_ASSERT2(m_textures.find(def_texture_name)!=m_textures.end(),texture_name);
		return FindItem	(def_texture_name,NULL);
	}
}

void CUITextureMaster::GetTextureShader(LPCSTR texture_name, ui_shader& sh){
	xr_map<shared_str, TEX_INFO>::iterator	it;
	it = m_textures.find(texture_name);

	R_ASSERT3(it != m_textures.end(), "can't find texture", texture_name);

	sh->create("hud\\default", *((*it).second.file));	
}

void CUITextureMaster::InitTexture(const shared_str& texture_name, const shared_str& shader_name, ui_shader& out_shader, Frect& out_rect)
{
	xr_map<shared_str, TEX_INFO>::iterator it	= m_textures.find(texture_name);
	if (it != m_textures.end())
	{
		sh_pair p={it->second.file, shader_name};
		xr_map<sh_pair, ui_shader>::iterator sh_it = m_shaders.find(p);
		if(sh_it==m_shaders.end())
			m_shaders[p]->create(shader_name.c_str(), it->second.file.c_str());

		out_shader			= m_shaders[p];
		out_rect			= (*it).second.rect;
	}else
		out_shader->create	(shader_name.c_str(), texture_name.c_str());
}
