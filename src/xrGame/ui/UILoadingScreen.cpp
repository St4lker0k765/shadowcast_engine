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
      loadingProgressPercent(nullptr), loadingProgressBackground(nullptr),
      loadingStage(nullptr), loadingHeader(nullptr),
      loadingTipNumber(nullptr), loadingTip(nullptr)
{
    UILoadingScreen::Initialize();
}

void UILoadingScreen::Initialize()
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, "ui_mm_loading_screen.xml");

    if (uiXml.NavigateToNode("loading_progress_background"))
        loadingProgressBackground = UIHelper::CreateStatic(uiXml, "loading_progress_background", this);

        loadingProgress = UIHelper::CreateProgressBar(uiXml, "loading_progress", this);
        CUIXmlInit::InitWindow(uiXml, "background", 0, this);
        loadingLogo = UIHelper::CreateStatic(uiXml, "loading_logo", this);

    if (uiXml.NavigateToNode("loading_process_percent"))
        loadingProgressPercent = UIHelper::CreateStatic(uiXml, "loading_progress_percent", this);

        loadingStage = UIHelper::CreateStatic(uiXml, "loading_stage", this);

    if (uiXml.NavigateToNode("loading_header"))
        loadingHeader = UIHelper::CreateStatic(uiXml, "loading_header", this);

    if (uiXml.NavigateToNode("loading_tip_number"))
        loadingTipNumber = UIHelper::CreateStatic(uiXml, "loading_tip_number", this);

    if (uiXml.NavigateToNode("loading_tip"))
        loadingTip = UIHelper::CreateStatic(uiXml, "loading_tip", this);
}

void UILoadingScreen::Update(const int stagesCompleted, const int stagesTotal)
{
    const float progress = float(stagesCompleted) / float(stagesTotal) * loadingProgress->GetRange_max();
    if (loadingProgress->GetProgressPos() < progress)
        loadingProgress->SetProgressPos(progress);

    if (ps_rs_loading_stages && loadingProgressPercent)
    {
        char buf[5];
        xr_sprintf(buf, "%.0f%%", loadingProgress->GetProgressPos());
        loadingProgressPercent->TextItemControl()->SetText(buf);
    }

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
    loadingStage->TextItemControl()->SetText(title);
}

void UILoadingScreen::SetStageTip(const char* header, const char* tipNumber, const char* tip) const
{
    if (loadingHeader)
        loadingHeader->TextItemControl()->SetText(header);
    if (loadingTipNumber)
        loadingTipNumber->TextItemControl()->SetText(tipNumber);
    if (loadingTip)
        loadingTip->TextItemControl()->SetText(tip);
}
