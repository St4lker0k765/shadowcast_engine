// file:		UITextureMaster.h
// description:	holds info about shared textures. able to initialize external
//				through IUITextureControl interface
// created:		11.05.2005
// author:		Serge Vynnychenko
// mail:		narrator@gsc-game.kiev.ua
//
// copyright 2005 GSC Game World

#pragma once

class IUISimpleTextureControl;

#include "..\ui_defs.h"

struct TEX_INFO{
	shared_str	file;
	Frect		rect;
	LPCSTR		get_file_name	()	{return *file;}
	Frect		get_rect		()	{return rect;}
};

struct sh_pair {
	shared_str	texture_name;
	shared_str	shader_name;
	bool operator < (const sh_pair& other) const
	{
		if (texture_name < other.texture_name)
			return true;
		else
			return shader_name < other.shader_name;
	}
};
class CUITextureMaster{
public:

	static void ParseShTexInfo			(LPCSTR xml_file);
	static void ParseShTexInfoNew		(LPCSTR xml_file);
	static void FreeTexInfo				();
	static void FreeCachedShaders		();

	static void		InitTexture			(LPCSTR texture_name,		IUISimpleTextureControl* tc);
	static void		InitTexture			(LPCSTR texture_name, const char* shader_name, IUISimpleTextureControl* tc);
	static void		InitTexture			(const shared_str& texture_name, const shared_str& shader_name, ui_shader& out_shader, Frect& out_rect);
	static float	GetTextureHeight	(LPCSTR texture_name);
	static float	GetTextureWidth		(LPCSTR texture_name);
	static Frect	GetTextureRect		(LPCSTR texture_name);
	static LPCSTR	GetTextureFileName	(LPCSTR texture_name);
	static void		GetTextureShader	(LPCSTR texture_name, ui_shader& sh);
	static TEX_INFO	FindItem			(LPCSTR texture_name, LPCSTR def_texture_name);
	static void WriteLog();

protected:
	IC	static bool IsSh					(const char* texture_name);



//	typedef xr_string region_name;
//	typedef xr_string shader_name;
//	typedef xr_map<region_name, Frect>				regions;
//	typedef xr_map<region_name, Frect>::iterator	regions_it;
//	typedef xr_map<shader_name, regions>			shared_textures;
//	typedef xr_map<shader_name, regions>::iterator	shared_textures_it;

	static xr_map<shared_str, TEX_INFO>					m_textures;

	static xr_map<sh_pair, ui_shader>	m_shaders;
//	static	shared_textures		m_shTex;
#ifdef DEBUG
	static u32		m_time;
#endif 
};