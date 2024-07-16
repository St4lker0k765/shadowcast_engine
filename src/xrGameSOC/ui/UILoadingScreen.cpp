////////////////////////////////////////////////////////////////////////////
//  Created     : 19.06.2018
//  Modified    : 01.06.2024
//  Authors     : Xottab_DUTY (OpenXRay project)
//                FozeSt
//                Unfainthful
//                St4lker0k765 (adaptation)
// 
//  Underground Collective, 2024
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UILoadingScreen.h"

#include "xrEngine/x_ray.h"
#include "xrEngine/GameFont.h"
#include "UIXmlInit.h"

extern ENGINE_API int ps_rs_loading_stages;

UILoadingScreen::UILoadingScreen()
    : loadingLogo(nullptr), loadingProgress(nullptr),
      loadingStage(nullptr), loadingHeader(nullptr),
      loadingTipNumber(nullptr), loadingTip(nullptr)
{
    UILoadingScreen::Initialize();
}

void UILoadingScreen::Initialize()
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, "ui_mm_loading_screen.xml");

        CUIXmlInit::InitWindow(uiXml, "background", 0, this);
        CUIXmlInit::InitProgressBar(uiXml, "loading_progress", 0, loadingProgress);
         CUIXmlInit::InitStatic(uiXml, "loading_logo", 0, loadingLogo);
        CUIXmlInit::InitStatic(uiXml, "loading_stage", 0, loadingStage);
}

void UILoadingScreen::Update(const int stagesCompleted, const int stagesTotal)
{
    const float progress = float(stagesCompleted) / stagesTotal * loadingProgress->GetRange_max();
    if (loadingProgress->GetProgressPos() < progress)
        loadingProgress->SetProgressPos(progress);


    CUIWindow::Update();
    Draw();
}

void UILoadingScreen::ForceFinish()
{
    loadingProgress->SetProgressPos(loadingProgress->GetRange_max());
}

void UILoadingScreen::SetLevelLogo(const char* name) const
{
    loadingLogo->InitTexture(name);
}

void UILoadingScreen::SetStageTitle(const char* title) const
{
    loadingStage->SetText(title);
}

void UILoadingScreen::SetStageTip(const char* header, const char* tipNumber, const char* tip) const
{
}
