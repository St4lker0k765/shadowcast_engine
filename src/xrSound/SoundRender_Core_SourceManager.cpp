#include "stdafx.h"
#pragma hdrstop

#include "SoundRender_Core.h"
#include "SoundRender_Source.h"
#include <tbb/parallel_for_each.h>

CSoundRender_Source*	CSoundRender_Core::i_create_source		(LPCSTR name)
{
    // Search
    string256 id;
    xr_strcpy(id, name);
    xr_strlwr(id);
    if (strext(id))
        *strext(id) = 0;
    for (u32 it = 0; it < s_sources.size(); it++) {
        if (0 == xr_strcmp(*s_sources[it]->fname, id))	return s_sources[it];
    }

    // Load a _new one
    CSoundRender_Source* S = new CSoundRender_Source();
    S->load(id);
    s_sources.push_back(S);
    return S;
}

void					CSoundRender_Core::i_destroy_source		(CSoundRender_Source*  S)
{
	// No actual destroy at all
}

/*
void CSoundRender_Core::i_create_all_sources()
{

    CTimer T;
    T.Start();

    FS_FileSet flist;
    FS.file_list(flist, "$game_sounds$", FS_ListFiles, "*.ogg");
    const size_t sizeBefore = s_sources.size();

    DECLARE_MT_LOCK(lock);
    const auto processFile = [&](const FS_File& file)
    {
        string256 id;
        xr_strcpy(id, file.name.c_str());

        xr_strlwr(id);
        if (strext(id))
            *strext(id) = 0;

        {
            DECLARE_MT_SCOPE_LOCK(lock);
            for (u32 it = 0; it < s_sources.size(); it++) {
                if (0 == xr_strcmp(*s_sources[it]->fname, id))	return s_sources[it];
            }

        CSoundRender_Source* S = new CSoundRender_Source();
        S->load(id);

        DO_MT_LOCK(lock);
        s_sources.push_back(S);
        DO_MT_UNLOCK(lock);
    };

    DO_MT_PROCESS_RANGE(flist, processFile);


    Msg("Finished creating %d sound sources. Duration: %d ms",
        s_sources.size() - sizeBefore, T.GetElapsed_ms());

}
*/