#include "stdafx.h"

#include "Restrictions.h"
#ifdef DEBUG
#	include "../../xrEngine/xr_ioconsole.h"
#	include "../../xrEngine/xr_ioc_cmd.h"
#endif //#ifdef DEBUG
#include "../string_table.h"

shared_str	g_ranks[_RANK_COUNT];

u32 get_rank(const shared_str &section)
{
	int res = -1;
	if( g_ranks[0].size()==0 )
	{ //load
		string32			buff;
		for (int i = 0; i<_RANK_COUNT; i++)
		{
			xr_sprintf(buff, "rank_%d", i);
			g_ranks[i] = pSettings->r_string(buff, "available_items");
		}
	}
	for (u32 i = 0; i<_RANK_COUNT; i++)
	{
		if (strstr(g_ranks[i].c_str(), section.c_str()))
		{
			res = i;
			break;
		}
	}

	if (res == -1)
	{
		//Msg("Setting rank to 0. Cannot find rank for: [%s]", section.c_str());
		// Xottab_DUTY: I'm not sure if it's save to leave it -1
		res = 0;
	}

	//R_ASSERT3	(res!=-1,"cannot find rank for", section.c_str());
	return		res;
}
