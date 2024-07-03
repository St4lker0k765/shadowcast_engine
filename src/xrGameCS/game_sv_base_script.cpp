////////////////////////////////////////////////////////////////////////////
//	Module 		: game_sv_base_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Base server game script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "game_sv_base.h"
#include "xrMessages.h"
#include "ui/UIInventoryUtilities.h"
#include "xr_time.h"
#include "UI/UIGameTutorial.h"
#include "string_table.h"
#include "object_broker.h"

using namespace luabind;
using namespace std::placeholders;

#pragma optimize("s",on)
void game_sv_GameState::script_register(lua_State *L)
{	
	module(L)
	[
	class_<enum_exporter<EGamePlayerFlags> >("game_player_flags")
		.enum_("flags")
		[
			value("GAME_PLAYER_FLAG_LOCAL",						int(GAME_PLAYER_FLAG_LOCAL)),
			value("GAME_PLAYER_FLAG_READY",						int(GAME_PLAYER_FLAG_READY)),
			value("GAME_PLAYER_FLAG_VERY_VERY_DEAD",			int(GAME_PLAYER_FLAG_VERY_VERY_DEAD)),
			value("GAME_PLAYER_FLAG_SPECTATOR",					int(GAME_PLAYER_FLAG_SPECTATOR)),
			value("GAME_PLAYER_FLAG_SCRIPT_BEGINS_FROM",		int(GAME_PLAYER_FLAG_SCRIPT_BEGINS_FROM))
		],
	class_<enum_exporter<EGameMessages> >("game_messages")
		.enum_("messages")
		[
			value("GAME_EVENT_PLAYER_READY",					int(GAME_EVENT_PLAYER_READY)),
			value("GAME_EVENT_PLAYER_CHANGE_TEAM",				int(GAME_EVENT_PLAYER_GAME_MENU)),
			value("GAME_EVENT_PLAYER_KILL",						int(GAME_EVENT_PLAYER_KILL)),
			value("GAME_EVENT_PLAYER_BUY_FINISHED",				int(GAME_EVENT_PLAYER_BUY_FINISHED)),
			value("GAME_EVENT_PLAYER_CHANGE_SKIN",				int(GAME_EVENT_PLAYER_GAME_MENU)),
			value("GAME_EVENT_PLAYER_CONNECTED",				int(GAME_EVENT_PLAYER_CONNECTED)),
			value("GAME_EVENT_PLAYER_DISCONNECTED",				int(GAME_EVENT_PLAYER_DISCONNECTED)),
			value("GAME_EVENT_PLAYER_KILLED",					int(GAME_EVENT_PLAYER_KILLED)),
			value("GAME_EVENT_PLAYER_JOIN_TEAM",				int(GAME_EVENT_PLAYER_JOIN_TEAM)),
			value("GAME_EVENT_ROUND_STARTED",					int(GAME_EVENT_ROUND_STARTED)),
			value("GAME_EVENT_ROUND_END",						int(GAME_EVENT_ROUND_END)),
			value("GAME_EVENT_ARTEFACT_SPAWNED",				int(GAME_EVENT_ARTEFACT_SPAWNED)),
			value("GAME_EVENT_ARTEFACT_DESTROYED",				int(GAME_EVENT_ARTEFACT_DESTROYED)),
			value("GAME_EVENT_ARTEFACT_TAKEN",					int(GAME_EVENT_ARTEFACT_TAKEN)),
			value("GAME_EVENT_ARTEFACT_DROPPED",				int(GAME_EVENT_ARTEFACT_DROPPED)),
			value("GAME_EVENT_ARTEFACT_ONBASE",					int(GAME_EVENT_ARTEFACT_ONBASE)),
			value("GAME_EVENT_PLAYER_ENTER_TEAM_BASE",			int(GAME_EVENT_PLAYER_ENTER_TEAM_BASE)),
			value("GAME_EVENT_PLAYER_LEAVE_TEAM_BASE",			int(GAME_EVENT_PLAYER_LEAVE_TEAM_BASE)),
			value("GAME_EVENT_BUY_MENU_CLOSED",					int(GAME_EVENT_BUY_MENU_CLOSED)),
			value("GAME_EVENT_TEAM_MENU_CLOSED",				int(GAME_EVENT_TEAM_MENU_CLOSED)),
			value("GAME_EVENT_SKIN_MENU_CLOSED",				int(GAME_EVENT_SKIN_MENU_CLOSED)),
			value("GAME_EVENT_SCRIPT_BEGINS_FROM",				int(GAME_EVENT_SCRIPT_BEGINS_FROM))
		]

	
	];
}
