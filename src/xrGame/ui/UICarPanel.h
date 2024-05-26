#pragma once

#include "uiwindow.h"
#include "uistatic.h"
#include "UIProgressBar.h"

//#include "uipointergage.h"


class CUICarPanel : public CUIWindow
{
private:
	typedef CUIWindow inherited;

	CUIStatic			UIStaticCarHealth;
	CUIProgressBar		UICarHealthBar;
	//	CUIPointerGage		UISpeedometer;
	//	CUIPointerGage		UITachometer;
public:

	// ”становить 
	void				SetCarHealth(float value);
	void				SetSpeed(float speed);
	void				SetRPM(float rmp);
	void				Init(Fvector2 pos, Fvector2 size);
};