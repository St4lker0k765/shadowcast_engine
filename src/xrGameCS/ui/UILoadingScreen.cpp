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
#include "UIHelper.h"

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

 //       loadingProgressBackground = UIHelper::CreateStatic(uiXml, "loading_progress_background", this);
        CUIXmlInit::InitWindow(uiXml, "background", 0, this);
        loadingProgress = UIHelper::CreateProgressBar(uiXml, "loading_progress", this);
        loadingLogo = UIHelper::CreateStatic(uiXml, "loading_logo", this);
 //       loadingProgressPercent = UIHelper::CreateStatic(uiXml, "loading_progress_percent", this);
        loadingStage = UIHelper::CreateStatic(uiXml, "loading_stage", this);
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
