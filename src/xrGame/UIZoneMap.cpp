#include "stdafx.h"
#include "uizonemap.h"

#include "InfoPortion.h"
#include "Pda.h"

#include "Grenade.h"
#include "level.h"
#include "game_cl_base.h"

#include "actor.h"
#include "ai_space.h"
#include "game_graph.h"

#include "ui/UIMap.h"
#include "ui/UIXmlInit.h"
#include "ui/UIHelper.h"
#include "ui/UIInventoryUtilities.h"
//////////////////////////////////////////////////////////////////////////

bool UseSquareMinimap = READ_IF_EXISTS(pSCSettings, r_bool, "game_specific", "cop_square_minimap", false);

CUIZoneMap::CUIZoneMap()
:m_current_map_idx(u8(-1)),
visible(true)
{	
}

CUIZoneMap::~CUIZoneMap()
{
}

void CUIZoneMap::Init()
{
	CUIXml							uiXml;
	CUIXmlInit						xml_init;
	if (UseSquareMinimap)
	{
		uiXml.Load(CONFIG_PATH, UI_PATH, "zone_map.xml");

		xml_init.InitStatic(uiXml, "minimap:background", 0, &m_background);

		if (IsGameTypeSingle())
		{
			xml_init.InitTextWnd(uiXml, "minimap:background:dist_text", 0, &m_pointerDistanceText);
			m_background.AttachChild(&m_pointerDistanceText);
		}

		xml_init.InitWindow(uiXml, "minimap:level_frame", 0, &m_clipFrame);

		xml_init.InitStatic(uiXml, "minimap:center", 0, &m_center);


		m_activeMap = xr_new<CUIMiniMap>();
		m_clipFrame.AttachChild(m_activeMap);
		m_activeMap->SetAutoDelete(true);

		m_activeMap->EnableHeading(true);
		xml_init.InitStatic(uiXml, "minimap:compass", 0, &m_compass);
		m_background.AttachChild(&m_compass);

		if (IsGameTypeSingle())
		{
			xml_init.InitStatic(uiXml, "minimap:static_counter", 0, &m_Counter);
			m_background.AttachChild(&m_Counter);
			xml_init.InitTextWnd(uiXml, "minimap:static_counter:text_static", 0, &m_Counter_text);
			m_Counter.AttachChild(&m_Counter_text);
		}

		m_clipFrame.AttachChild(&m_center);
		m_center.SetWndPos(Fvector2().set(m_clipFrame.GetWidth() / 2.0f, m_clipFrame.GetHeight() / 2.0f));

		m_Counter_text.SetText("");
		visible = true;
	}
	else 
	{
		uiXml;
		uiXml.Load(CONFIG_PATH, UI_PATH, "zone_map.xml");

		xml_init.InitStatic(uiXml, "minimap:background", 0, &m_background);
		xml_init.InitWindow(uiXml, "minimap:level_frame", 0, &m_clipFrame);
		xml_init.InitStatic(uiXml, "minimap:center", 0, &m_center);

		m_clock_wnd = UIHelper::CreateStatic(uiXml, "minimap:clock_wnd", &m_background);

		m_activeMap = xr_new<CUIMiniMap>();
		m_clipFrame.AttachChild(m_activeMap);
		m_activeMap->SetAutoDelete(true);

		m_activeMap->EnableHeading(true);
		xml_init.InitStatic(uiXml, "minimap:compass", 0, &m_compass);
		m_background.AttachChild(&m_compass);

		m_clipFrame.AttachChild(&m_center);

		m_Counter_text.SetText("");
		visible = true;

		Fvector2 sz_k = m_clipFrame.GetWndSize();
		Fvector2 sz = sz_k;
		{
			float k = UI().get_current_kx();

			sz.y *= UI_BASE_HEIGHT * k;
			sz.x = sz.y / k;

			m_clipFrame.SetWndSize(sz);

			Fvector2 p = m_clipFrame.GetWndPos();
			p.mul(UI_BASE_HEIGHT);
			m_clipFrame.SetWndPos(p);

			m_background.SetHeight(m_background.GetHeight() * UI_BASE_HEIGHT);
			m_background.SetWidth(m_background.GetHeight() * k);
		}

		Fvector2				map_center;
		m_clipFrame.GetWndRect().getcenter(map_center);
		m_background.SetWndPos(map_center);

		Fvector2 cp;
		cp.x = m_clipFrame.GetWidth() / 2.0f;
		cp.y = m_clipFrame.GetHeight() / 2.0f;
		m_center.SetWndPos(cp);

		Fvector2 rel_pos = m_compass.GetWndPos();
		rel_pos.mul(m_background.GetWndSize());
		m_compass.SetWndPos(rel_pos);

		rel_pos = m_clock_wnd->GetWndPos();
		rel_pos.mul(m_background.GetWndSize());
		m_clock_wnd->SetWndPos(rel_pos);

		if (IsGameTypeSingle())
		{
			xml_init.InitStatic(uiXml, "minimap:static_counter", 0, &m_Counter);
			m_background.AttachChild(&m_Counter);
			xml_init.InitTextWnd(uiXml, "minimap:static_counter:text_static", 0, &m_Counter_text);
			m_Counter.AttachChild(&m_Counter_text);

			rel_pos = m_Counter.GetWndPos();
			rel_pos.mul(m_background.GetWndSize());
			m_Counter.SetWndPos(rel_pos);
		}
	}
}

void CUIZoneMap::Render()
{
	if (!visible)
	{
		return;
	}
	m_clipFrame.Draw();
	m_background.Draw();
	if (UseSquareMinimap)
		m_compass.Draw();
}

void CUIZoneMap::Update()
{
	CActor* pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	if (!pActor) return;

	if (!(Device.dwFrame % 20) && IsGameTypeSingle())
	{
		string16	text_str;
		strcpy_s(text_str, sizeof(text_str), "");

		CPda* pda = pActor->GetPDA();
		if (pda)
		{
			u32 cn = pda->ActiveContactsNum();
			if (cn > 0)
			{
				sprintf_s(text_str, sizeof(text_str), "%d", cn);
			}
		}
		m_Counter_text.SetText(text_str);
	}

	UpdateRadar(Device.vCameraPosition);
	float h, p;
	Device.vCameraDirection.getHP(h, p);
	SetHeading(-h);
	if(!UseSquareMinimap)
		m_clock_wnd->TextItemControl()->SetText(InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes).c_str());
}

void CUIZoneMap::SetHeading(float angle)
{
	m_activeMap->SetHeading(angle);
	m_compass.SetHeading(angle);
};

void CUIZoneMap::UpdateRadar(Fvector pos)
{
	m_clipFrame.Update();
	m_background.Update();
	m_activeMap->SetActivePoint(pos);
		
	if(IsGameTypeSingle() && UseSquareMinimap)
	{
		if(m_activeMap->GetPointerDistance()>0.5f){
			string64	str;
			sprintf_s		(str,"%.0f m",m_activeMap->GetPointerDistance());
			m_pointerDistanceText.SetText(str);
		}else{
			m_pointerDistanceText.SetText("");
		}
	}
}

bool CUIZoneMap::ZoomIn()
{
	return true;
}

bool CUIZoneMap::ZoomOut()
{
	return true;
}

void CUIZoneMap::SetupCurrentMap()
{
	m_activeMap->Initialize(Level().name(), "hud\\default");

	Frect r;
	m_clipFrame.GetAbsoluteRect(r);
	m_activeMap->WorkingArea().set(r);

	Fvector2						wnd_size;
	float zoom_factor = float(m_clipFrame.GetWndRect().width()) / 100.0f;

	LPCSTR ln = Level().name().c_str();
	if (pGameIni->section_exist(ln))
	{
		if (pGameIni->line_exist(ln, "minimap_zoom"))
			zoom_factor *= pGameIni->r_float(ln, "minimap_zoom");
	}
	else
		if (g_pGameLevel->pLevel->section_exist("minimap_zoom"))
		{
			zoom_factor *= g_pGameLevel->pLevel->r_float("minimap_zoom", "value");
		}
	wnd_size.x = m_activeMap->BoundRect().width() * zoom_factor;
	wnd_size.y = m_activeMap->BoundRect().height() * zoom_factor;
	m_activeMap->SetWndSize(wnd_size);
}

void CUIZoneMap::OnSectorChanged(int sector)
{
	if (!g_pGameLevel->pLevel->section_exist("sub_level_map"))
		return;
	u8			map_idx = u8(-1);
	string64	s_sector;
	sprintf_s(s_sector, "%d", sector);

	if (!g_pGameLevel->pLevel->line_exist("sub_level_map", s_sector))
		return;

	map_idx = g_pGameLevel->pLevel->r_u8("sub_level_map", s_sector);
	if (m_current_map_idx == map_idx)
		return;

	m_current_map_idx = map_idx;

	string_path sub_texture;
	sprintf_s(sub_texture, "%s#%d", m_activeMap->m_texture.c_str(), m_current_map_idx);

	if (map_idx == u8(-1))
		sprintf_s(sub_texture, "%s", m_activeMap->m_texture.c_str());

	m_activeMap->InitTextureEx(sub_texture, m_activeMap->m_shader_name.c_str());
}

void CUIZoneMap::Counter_ResetClrAnimation()
{
	m_Counter_text.ResetColorAnimation();
}
