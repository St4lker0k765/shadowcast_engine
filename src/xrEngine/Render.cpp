#include "stdafx.h"
#include "render.h"
#ifdef _EDITOR
#include "Include/xrAPI/xrAPI.h"
#include <Layers/xrRenderPC_R2/r2.h>
#endif

IRender_interface::~IRender_interface() {};

//ENGINE_API IRender_interface* Render = NULL;

// resources
IRender_Light::~IRender_Light()
{
    ::Render->light_destroy(this);
}
IRender_Glow::~IRender_Glow()
{
    ::Render->glow_destroy(this);
}
