#pragma once

#include "eventAPI.h"
#include "xrSheduler.h"

// Abstract 'Pure' class for DLL interface
class ENGINE_API DLL_Pure
{
public:
    CLASS_ID CLS_ID;

    DLL_Pure(void* params) { CLS_ID = 0; };
    DLL_Pure() { CLS_ID = 0; };
    virtual DLL_Pure* _construct() { return this; }
    virtual ~DLL_Pure() {};
};

// Class creation/destroying interface
extern "C" {
    typedef DLL_API DLL_Pure* __cdecl Factory_Create(CLASS_ID CLS_ID);
    typedef DLL_API void __cdecl Factory_Destroy(DLL_Pure* O);
};

class ENGINE_API CEngine
{
    HMODULE h_game;
    HMODULE h_render;
public:
    BENCH_SEC_SCRAMBLEMEMBER1

    //hGame starting
	Factory_Create* pCreate;
    Factory_Destroy* pDestroy;

    // DLL api stuff
    CEventAPI Event;
    CSheduler Sheduler{};

    void Initialize();
    void Destroy();
#ifdef _EDITOR
    LPCSTR              LastWindowsError();

    void				ReloadSettings();
#endif
    void Initialize_dll();
    void CreatingRenderList();

    CEngine();
    ~CEngine();
};

ENGINE_API extern CEngine Engine;

#define NEW_INSTANCE(a) Engine.pCreate(a)
#define DEL_INSTANCE(a) { Engine.pDestroy(a); a=NULL; }