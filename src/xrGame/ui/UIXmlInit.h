#pragma once

#include "xrUIXmlParser.h"

class ITextureOwner;
class CUIWindow;
class CUIFrameWindow;
class CUIStaticItem;
class CUIStatic;
class CUICheckButton;
class CUICustomSpin;
class CUIButton;
class CUI3tButton;
class CUIDragDropList;
class CUIProgressBar;
class CUIProgressShape;
class CUITabControl;
class CUIFrameLineWnd;
class CUIEditBoxEx;
class CUIEditBox;
class CUICustomEdit;
class CUIAnimatedStatic;
class CUISleepStatic;
class CUIOptionsItem;
class CUIScrollView;
class CUIListBox;
class CUIStatsPlayerList;
class CUIDragDropListEx;
class CUIComboBox;
class CUITabButtonMP;
class CUITrackBar;
class UIHintWindow;
class CUILines;
class CUITextWnd;

class CUIXmlInit
{
public:
					CUIXmlInit				();
	virtual			~CUIXmlInit				();
		
	static bool 	InitWindow				(CUIXml& xml_doc, LPCSTR path,	int index, CUIWindow* pWnd);
	static bool 	InitFrameWindow			(CUIXml& xml_doc, LPCSTR path,	int index, CUIFrameWindow* pWnd);
	static bool 	InitFrameLine			(CUIXml& xml_doc, LPCSTR path, int index, CUIFrameLineWnd* pWnd);
	static bool 	InitCustomEdit			(CUIXml& xml_doc, LPCSTR paht, int index, CUICustomEdit* pWnd);
	static bool 	InitEditBox				(CUIXml& xml_doc, LPCSTR paht, int index, CUIEditBox* pWnd);
	static bool 	InitStatic				(CUIXml& xml_doc, LPCSTR path, int index, CUIStatic* pWnd);
	static bool 	InitTextWnd				(CUIXml& xml_doc, LPCSTR path, int index, CUITextWnd* pWnd);
	static bool		InitCheck				(CUIXml& xml_doc, LPCSTR path, int index, CUICheckButton* pWnd);
	static bool 	InitSpin				(CUIXml& xml_doc, LPCSTR path, int index, CUICustomSpin* pWnd);
	static bool 	InitText				(CUIXml& xml_doc, LPCSTR path, int index, CUIStatic* pWnd);
	static bool 	InitText				(CUIXml& xml_doc, LPCSTR path, int index, CUILines* pLines);
	static bool 	Init3tButton			(CUIXml& xml_doc, LPCSTR path, int index, CUI3tButton* pWnd);
	static bool 	InitDragDropListEx		(CUIXml& xml_doc, LPCSTR path, int index, CUIDragDropListEx* pWnd);
	static bool 	InitProgressBar			(CUIXml& xml_doc, LPCSTR path, int index, CUIProgressBar* pWnd);
	static bool 	InitProgressShape		(CUIXml& xml_doc, LPCSTR path, int index, CUIProgressShape* pWnd);
	static bool 	InitFont				(CUIXml& xml_doc, LPCSTR path, int index, u32 &color, CGameFont *&pFnt);
	static bool 	InitTabButtonMP			(CUIXml& xml_doc, LPCSTR path,	int index, CUITabButtonMP *pWnd);
	static bool 	InitTabControl			(CUIXml& xml_doc, LPCSTR path,	int index, CUITabControl *pWnd);
	static bool 	InitAnimatedStatic		(CUIXml& xml_doc, LPCSTR path,	int index, CUIAnimatedStatic *pWnd);
	static bool 	InitSleepStatic			(CUIXml& xml_doc, LPCSTR path,	int index, CUISleepStatic *pWnd);
	static bool 	InitTextureOffset		(CUIXml& xml_doc, LPCSTR path, int index, CUIStatic* pWnd);
	static bool 	InitSound				(CUIXml& xml_doc, LPCSTR path, int index, CUI3tButton* pWnd);
	static bool 	InitMultiTexture		(CUIXml& xml_doc, LPCSTR path, int index, CUI3tButton* pWnd);
	static bool 	InitTexture				(CUIXml& xml_doc, LPCSTR path, int index, ITextureOwner* pWnd);
	static bool 	InitOptionsItem			(CUIXml& xml_doc, LPCSTR paht, int index, CUIOptionsItem* pWnd);
	static bool 	InitScrollView			(CUIXml& xml_doc, LPCSTR path, int index, CUIScrollView* pWnd);
	static bool 	InitListBox				(CUIXml& xml_doc, LPCSTR path, int index, CUIListBox* pWnd);
	static bool		InitComboBox			(CUIXml& xml_doc, LPCSTR path, int index, CUIComboBox* pWnd);
	static bool		InitTrackBar			(CUIXml& xml_doc, LPCSTR path, int index, CUITrackBar* pWnd);
	static bool 	InitHintWindow			(CUIXml& xml_doc, LPCSTR path, int index, UIHintWindow* pWnd);
	static Frect	GetFRect				(CUIXml& xml_doc, LPCSTR path, int index);
	static u32		GetColor				(CUIXml& xml_doc, LPCSTR path, int index, u32 def_clr);
public:

	static bool		InitAlignment(CUIXml &xml_doc, const char *path,
											  int index, float &x, float &y,CUIWindow* pWnd);


	static void		InitAutoStaticGroup(CUIXml& xml_doc, LPCSTR path, int index, CUIWindow* pParentWnd);
	static void		InitAutoFrameLineGroup(CUIXml& xml_doc, LPCSTR path, int index, CUIWindow* pParentWnd);

	static float	ApplyAlignX(float coord, u32 align);
	static float	ApplyAlignY(float coord, u32 align);
	static void		ApplyAlign(float &x, float &y, u32 align);

	// Initialize and store predefined colors
	DEF_MAP			(ColorDefs, shared_str, u32);

	static const ColorDefs		*GetColorDefs()		{ R_ASSERT(m_pColorDefs); return m_pColorDefs; }

	static void		InitColorDefs				();
	static void		DeleteColorDefs				()	{ xr_delete(m_pColorDefs); }
	static void		AssignColor					(LPCSTR name, u32 clr);
private:
	static	ColorDefs			*m_pColorDefs;

	static shared_str m_Graffiti19RussiaFontName;
	static shared_str m_Graffiti22RussiaFontName;
	static shared_str m_Graff32FontName;
	static shared_str m_Graff50FontName;
	static shared_str m_Arial14FontName;
	static shared_str m_Arial21FontName;
	static shared_str m_Letterica16FontName;
	static shared_str m_Letterica18FontName;
	static shared_str m_Letter25FontName;
};
