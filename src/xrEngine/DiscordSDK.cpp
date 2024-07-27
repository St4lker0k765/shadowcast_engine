#include "stdafx.h"

#include "DiscordSDK.h"
#include "x_ray.h"
ENGINE_API DiscordSDK Discord;

#pragma comment(lib, "discord_game_sdk.dll.lib")

DiscordSDK::~DiscordSDK()
{
	delete core;
}

void DiscordSDK::InitSDK()
{
	auto ResultSDK_ = discord::Core::Create(1263598008340316263, DiscordCreateFlags_Default, &core);
	
	if (!core)
	{
		Msg("! [DISCORD SDK]: Error to init SDK. Code error: [%d]", static_cast<int>(ResultSDK_));
		return;
	}

	ActivityDiscord_.GetAssets().SetLargeImage("shadowcast_engine_logo_rpc");

	if (CallOfPripyatMode)
	{
		ActivityDiscord_.GetAssets().SetSmallImage("shadowcast_engine_cop");
		ActivityDiscord_.GetAssets().SetSmallText("Call of Pripyat");
	}
	else if (ClearSkyMode)
	{
		ActivityDiscord_.GetAssets().SetSmallImage("shadowcast_engine_cs");
		ActivityDiscord_.GetAssets().SetSmallText("Clear Sky");
	}
	else if (ShadowOfChernobylMode)
	{
		ActivityDiscord_.GetAssets().SetSmallImage("shadowcast_engine_soc");
		ActivityDiscord_.GetAssets().SetSmallText("Shadow of Chernobyl");
	}
	ActivityDiscord_.SetInstance(true);
	ActivityDiscord_.SetType(discord::ActivityType::Playing);

	ActivityDiscord_.GetTimestamps().SetStart(time(nullptr));

	NeedUpdateActivity_ = true; //First update activity discord.
}

void DiscordSDK::UpdateSDK()
{
	if (!core)
		return;

	if (NeedUpdateActivity_)
		UpdateActivity();

	core->RunCallbacks();
}

void DiscordSDK::UpdateActivity()
{
	ActivityDiscord_.SetState(ANSIToUTF8(PhaseDiscord_).c_str());
	ActivityDiscord_.SetDetails(ANSIToUTF8(StatusDiscord_).c_str());

	core->ActivityManager().UpdateActivity(ActivityDiscord_, [](discord::Result result)
		{
			if (result != discord::Result::Ok)
				Msg("! [DISCORD SDK]: Invalid UpdateActivity");
		});

	NeedUpdateActivity_ = false;
}

void DiscordSDK::SetPhase(const xr_string& phase)
{
	PhaseDiscord_ = phase;
	NeedUpdateActivity_ = true;
}

void DiscordSDK::SetStatus(const xr_string& status)
{
	StatusDiscord_ = status;
	NeedUpdateActivity_ = true;
}