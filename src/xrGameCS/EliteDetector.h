#pragma once

#include "CustomDetector.h"
#include "ui/ArtefactDetectorUI.h"
#include "ZoneList.h"

class CUIArtefactDetectorElite;

class CEliteDetector :public CCustomDetector
{
	typedef CCustomDetector	inherited;
public:
					CEliteDetector				();
	virtual			~CEliteDetector				();
	virtual void	render_item_3d_ui			();
	virtual bool	render_item_3d_ui_query		();
	virtual LPCSTR	ui_xml_tag					() const {return "elite";}
protected:
	virtual void 	UpdateAf					();
	virtual void 	CreateUI					();
	CUIArtefactDetectorElite& ui				();
};


class CScientificDetector :public CEliteDetector
{
	typedef CEliteDetector	inherited;
public:
	CScientificDetector();
	virtual			~CScientificDetector();
	virtual void 	Load(LPCSTR section);
	virtual void 	OnH_B_Independent(bool just_before_destroy);
	virtual void 	shedule_Update(u32 dt);
	virtual LPCSTR	ui_xml_tag() const { return "scientific"; }
protected:
	virtual void	UpfateWork();
	CZoneList		m_zones;
};


class CUIArtefactDetectorElite :public CUIArtefactDetectorBase, public CUIWindow
{
	typedef CUIArtefactDetectorBase	inherited;

	CUIWindow*			m_wrk_area;
	
	xr_map<shared_str,CUIStatic*>	m_palette;

	struct SDrawOneItem{
		SDrawOneItem	(CUIStatic* s, const Fvector& p):pStatic(s),pos(p){}
		CUIStatic*		pStatic;
		Fvector			pos;
	};
	xr_vector<SDrawOneItem>	m_items_to_draw;
	CEliteDetector*			m_parent;
	Fmatrix					m_map_attach_offset;

	void				GetUILocatorMatrix			(Fmatrix& _m);
public:

	virtual void	update			();
	virtual void	Draw			();

	void		construct			(CEliteDetector* p);
	void		Clear				();
	void		RegisterItemToDraw	(const Fvector& p, const shared_str& palette_idx);
};
