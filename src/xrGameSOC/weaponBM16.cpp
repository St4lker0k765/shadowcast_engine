#include "stdafx.h"
#include "weaponBM16.h"

CWeaponBM16::~CWeaponBM16()
{
}

void CWeaponBM16::Load	(LPCSTR section)
{
	inherited::Load		(section);

	m_sounds.LoadSound(section, "snd_reload_1", "sndReload1", m_eSoundReload);
}

void CWeaponBM16::PlayReloadSound()
{
	if ( m_magazine.size() == 1 || !HaveCartridgeInInventory( 2 ) )
		PlaySound("sndReload1", get_LastFP());
	else
		PlaySound("sndReload", get_LastFP());
}

void CWeaponBM16::PlayAnimShoot()
{
	switch (m_magazine.size())
	{
	case 1: 
		PlayHUDMotionIfExists({ "anim_shoot_1", "anm_shot_1" }, FALSE, GetState()); 
		break;
	case 2: 
		PlayHUDMotionIfExists({ "anim_shoot", "anm_shot_2" }, FALSE, GetState()); 
		break;
	default: 
		PlayHUDMotionIfExists({ "anim_shoot", "anm_shots" }, FALSE, GetState()); 
		break;
	}
}

void CWeaponBM16::PlayAnimShow()
{
	VERIFY(GetState() == eShowing);

	switch (m_magazine.size())
	{
	case 0:
	{
		PlayHUDMotionIfExists({ "anim_draw_0", "anim_draw", "anm_show_0" }, TRUE, GetState());
	} break;
	case 1:
	{
		PlayHUDMotionIfExists({ "anim_draw_1", "anim_draw", "anm_show_1" }, TRUE, GetState());
	} break;
	case 2:
	{
		PlayHUDMotionIfExists({ "anim_draw_2", "anim_draw", "anm_show_2" }, TRUE, GetState());
	} break;
	}
}

void CWeaponBM16::PlayAnimHide()
{
	VERIFY(GetState() == eHiding);

	switch (m_magazine.size())
	{
	case 0:
	{
		PlayHUDMotionIfExists({ "anim_holster_0", "anim_holster", "anm_hide_0" }, TRUE, GetState());
	} break;
	case 1:
	{
		PlayHUDMotionIfExists({ "anim_holster_1", "anim_holster", "anm_hide_1" }, TRUE, GetState());
	} break;
	case 2:
	{
		PlayHUDMotionIfExists({ "anim_holster_2",  "anim_holster", "anm_hide_2" }, TRUE, GetState());
	} break;
	}
}

void CWeaponBM16::PlayAnimReload()
{
	VERIFY(GetState() == eReload);
	if (m_magazine.size() == 1 || !HaveCartridgeInInventory(2))
		PlayHUDMotionIfExists({ "anim_reload_1", "anm_reload_1" }, TRUE, GetState());
	else
		PlayHUDMotionIfExists({ "anim_reload", "anm_reload_2" }, TRUE, GetState());
}

void CWeaponBM16::PlayAnimIdleMoving()
{
	switch (m_magazine.size())
	{
	case 0:
	{
		PlayHUDMotionIfExists({ "anim_idle_moving", "anim_idle", "anm_idle_moving_0" }, TRUE, GetState());
	} break;
	case 1:
	{
		PlayHUDMotionIfExists({ "anim_idle_moving_1", "anim_idle_moving", "anim_idle_1", "anm_idle_moving_1" }, TRUE, GetState());
	} break;
	case 2:
	{
		PlayHUDMotionIfExists({ "anim_idle_moving_2", "anim_idle_moving", "anim_idle_2", "anm_idle_moving_2" }, TRUE, GetState());
	} break;
	}
}

void CWeaponBM16::PlayAnimIdleMovingSlow()
{
	switch (m_magazine.size())
	{
	case 0:
		PlayHUDMotionIfExists({ "anm_idle_moving_slow_0", "anm_idle_moving_0", "anim_idle_moving", "anim_idle" }, true, GetState());
		break;
	case 1:
		PlayHUDMotionIfExists({ "anm_idle_moving_slow_1", "anm_idle_moving_1", "anim_idle_moving_1", "anim_idle_moving", "anim_idle_1" }, true, GetState());
		break;
	case 2:
		PlayHUDMotionIfExists({ "anm_idle_moving_slow_2", "anm_idle_moving_2", "anim_idle_moving_2", "anim_idle_moving", "anim_idle_2" }, true, GetState());
		break;
	default:
		PlayHUDMotionIfExists({ "anm_idle_moving_slow_2", "anm_idle_moving_2", "anim_idle_moving_2", "anim_idle_moving", "anim_idle_2" }, true, GetState());
		break;
	}
}

void CWeaponBM16::PlayAnimIdleMovingCrouch()
{
	switch (m_magazine.size())
	{
	case 0:
		PlayHUDMotionIfExists({ "anm_idle_moving_crouch_0", "anm_idle_moving_0", "anim_idle_moving", "anim_idle" }, true, GetState());
		break;
	case 1:
		PlayHUDMotionIfExists({ "anm_idle_moving_crouch_1", "anm_idle_moving_1", "anim_idle_moving_1", "anim_idle_moving", "anim_idle_1" }, true, GetState());
		break;
	case 2:
		PlayHUDMotionIfExists({ "anm_idle_moving_crouch_2", "anm_idle_moving_2", "anim_idle_moving_2", "anim_idle_moving", "anim_idle_2" }, true, GetState());
		break;
	default:
		PlayHUDMotionIfExists({ "anm_idle_moving_crouch_2", "anm_idle_moving_2", "anim_idle_moving_2", "anim_idle_moving", "anim_idle_2" }, true, GetState());
		break;
	}
}

void CWeaponBM16::PlayAnimIdleMovingCrouchSlow()
{
	switch (m_magazine.size())
	{
	case 0:
		PlayHUDMotionIfExists({ "anm_idle_moving_crouch_slow_0", "anm_idle_moving_crouch_0", "anm_idle_moving_0", "anim_idle_moving", "anim_idle" }, true, GetState());
		break;
	case 1:
		PlayHUDMotionIfExists({ "anm_idle_moving_crouch_slow_1", "anm_idle_moving_crouch_1", "anm_idle_moving_1", "anim_idle_moving_1", "anim_idle_moving", "anim_idle_1" }, true, GetState());
		break;
	case 2:
		PlayHUDMotionIfExists({ "anm_idle_moving_crouch_slow_2", "anm_idle_moving_crouch_2", "anm_idle_moving_2", "anim_idle_moving_2", "anim_idle_moving", "anim_idle_2" }, true, GetState());
		break;
	default:
		PlayHUDMotionIfExists({ "anm_idle_moving_crouch_slow_2", "anm_idle_moving_crouch_2", "anm_idle_moving_2", "anim_idle_moving_2", "anim_idle_moving", "anim_idle_2" }, true, GetState());
		break;
	}
}
void CWeaponBM16::PlayAnimIdleSprint()
{
	switch (m_magazine.size())
	{
	case 0:
	{
		PlayHUDMotionIfExists({ "anim_idle_sprint", "anim_idle", "anm_idle_sprint_0" }, TRUE, GetState());
	} break;
	case 1:
	{
		PlayHUDMotionIfExists({ "anim_idle_sprint_1", "anim_idle_sprint", "anim_idle_1", "anm_idle_sprint_1" }, TRUE, GetState());
	} break;
	case 2:
	{
		PlayHUDMotionIfExists({ "anim_idle_sprint_2", "anim_idle_sprint", "anim_idle_2", "anm_idle_sprint_2" }, TRUE, GetState());
	} break;
	}
}

void CWeaponBM16::PlayAnimIdle()
{
	if (TryPlayAnimIdle())
		return;

	if (IsZoomed())
	{
		if (IsRotatingToZoom())
		{
			string32 guns_aim_anm;
			strconcat(sizeof(guns_aim_anm), guns_aim_anm, "anm_idle_aim_start_", std::to_string(m_magazine.size()).c_str());
			if (AnimationExist(guns_aim_anm))
			{
				PlayHUDMotionNew(guns_aim_anm, true, GetState());
				return;
			}
		}

		switch (m_magazine.size())
		{
		case 0:
		{ 
			PlayHUDMotionIfExists({ "anim_idle", "anm_idle_aim_0" }, TRUE, GetState());
		}
		break;
		case 1: 
		{
			PlayHUDMotionIfExists({ "anim_zoomed_idle_1", "anm_idle_aim_1" }, TRUE, GetState());
		}
		break;
		case 2:
		{
			PlayHUDMotionIfExists({ "anim_zoomed_idle_2", "anim_zoomedidle_2", "anm_idle_aim_2" }, TRUE, GetState());
		}
		break;
		};
	}
	else
	{
		if (IsRotatingFromZoom())
		{
			string32 guns_aim_anm;
			strconcat(sizeof(guns_aim_anm), guns_aim_anm, "anm_idle_aim_end_", std::to_string(m_magazine.size()).c_str());
			if (AnimationExist(guns_aim_anm))
			{
				PlayHUDMotionNew(guns_aim_anm, true, GetState());
				return;
			}
		}

		switch (m_magazine.size())
		{
		case 0: 
		{
			PlayHUDMotionIfExists({ "anim_idle", "anm_idle_0" }, TRUE, GetState());
		}
		break;
		case 1: 
		{ 
			PlayHUDMotionIfExists({ "anim_idle_1", "anm_idle_1" }, TRUE, GetState());
		}
		break;
		case 2: 
		{ 
			PlayHUDMotionIfExists({ "anim_idle_2", "anm_idle_2" }, TRUE, GetState());
		}
		break;
		};
	}
}

void CWeaponBM16::PlayAnimSprintStart()
{
	string_path guns_sprint_start_anm{};
	strconcat(sizeof(guns_sprint_start_anm), guns_sprint_start_anm, "anm_idle_sprint_start", std::to_string(m_magazine.size()).c_str(), IsMisfire() ? "_jammed" : "");

	if (AnimationExist(guns_sprint_start_anm))
		PlayHUDMotionNew(guns_sprint_start_anm, true, GetState());
	else if (strstr(guns_sprint_start_anm, "_jammed"))
	{
		char new_guns_aim_anm[256];
		strcpy(new_guns_aim_anm, guns_sprint_start_anm);
		new_guns_aim_anm[strlen(guns_sprint_start_anm) - strlen("_jammed")] = '\0';

		if (AnimationExist(new_guns_aim_anm))
			PlayHUDMotionNew(new_guns_aim_anm, true, GetState());
	}
	else
	{
		m_bSprintType = true;
		SwitchState(eIdle);
	}
}

void CWeaponBM16::PlayAnimSprintEnd()
{
	string_path guns_sprint_end_anm{};
	strconcat(sizeof(guns_sprint_end_anm), guns_sprint_end_anm, "anm_idle_sprint_end", std::to_string(m_magazine.size()).c_str(), IsMisfire() ? "_jammed" : "");

	if (AnimationExist(guns_sprint_end_anm))
		PlayHUDMotionNew(guns_sprint_end_anm, true, GetState());
	else if (strstr(guns_sprint_end_anm, "_jammed"))
	{
		char new_guns_aim_anm[256];
		strcpy(new_guns_aim_anm, guns_sprint_end_anm);
		new_guns_aim_anm[strlen(guns_sprint_end_anm) - strlen("_jammed")] = '\0';

		if (AnimationExist(new_guns_aim_anm))
			PlayHUDMotionNew(new_guns_aim_anm, true, GetState());
	}
	else
	{
		m_bSprintType = false;
		SwitchState(eIdle);
	}
}