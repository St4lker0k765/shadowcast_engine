#include "stdafx.h"
#include "account_manager_console.h"
#include "gamespy/GameSpy_Full.h"
#include "gamespy/GameSpy_GP.h"
#include "gamespy/GameSpy_SAKE.h"
#include "account_manager.h"
#include "login_manager.h"
#include "profile_store.h"
#include "MainMenu.h"

void CCC_CreateGameSpyAccount::Execute(LPCSTR args)
{
	if (!args || (xr_strlen(args) == 0))
	{
		TInfo tmp_info;
		Info(tmp_info);
		Msg(tmp_info);
		return;
	}
	string256	tmp_nick;
	string256	tmp_unick;
	string256	tmp_email;
	string256	tmp_password;
	
	sscanf_s(args, "%s %s %s %s",
		tmp_nick, sizeof(tmp_nick),
		tmp_unick, sizeof(tmp_unick),
		tmp_email, sizeof(tmp_email),
		tmp_password, sizeof(tmp_password));
	
	VERIFY(MainMenu() && MainMenu()->GetGS());
	CGameSpy_GP* tmp_gp = MainMenu()->GetGS()->GetGameSpyGP();
	VERIFY(tmp_gp);
	gamespy_gp::account_manager* tmp_acc_mngr = MainMenu()->GetAccountMngr();

	gamespy_gp::new_profile_data	prof_data;

	prof_data.nick					= tmp_nick;
	prof_data.unique_nick			= tmp_unick;
	prof_data.email					= tmp_email;
	prof_data.password				= tmp_password;

	tmp_acc_mngr->create_profile(
		tmp_nick,
		tmp_unick,
		tmp_email,
		tmp_password,
		gamespy_gp::account_operation_cb()
	);
}

void CCC_GapySpyListProfiles::Execute(LPCSTR args)
{
	if (!args || (xr_strlen(args) == 0))
	{
		TInfo tmp_info;
		Info(tmp_info);
		Msg(tmp_info);
		return;
	}
	string256	tmp_email;
	string256	tmp_password;
			
	sscanf_s(args, "%s %s",
		tmp_email,		sizeof(tmp_email),
		tmp_password,	sizeof(tmp_password));
	
	VERIFY(MainMenu() && MainMenu()->GetGS());
	CGameSpy_GP* tmp_gp = MainMenu()->GetGS()->GetGameSpyGP();
	VERIFY(tmp_gp);
	gamespy_gp::account_manager* tmp_acc_mngr = MainMenu()->GetAccountMngr();
	tmp_acc_mngr->get_account_profiles(
		tmp_email,
		tmp_password,
		gamespy_gp::account_profiles_cb()
	);
}

void CCC_GameSpyLogin::Execute(LPCSTR args)
{
	if (!args || (xr_strlen(args) == 0))
	{
		TInfo tmp_info;
		Info(tmp_info);
		Msg(tmp_info);
		return;
	}
	string256	tmp_email;
	string256	tmp_nick;
	string256	tmp_password;
			
	sscanf_s(args, "%s %s %s", 
		tmp_email,		sizeof(tmp_email),
		tmp_nick,		sizeof(tmp_nick),
		tmp_password,	sizeof(tmp_password)
	);
	
	VERIFY(MainMenu() && MainMenu()->GetGS());
	CGameSpy_GP* tmp_gp = MainMenu()->GetGS()->GetGameSpyGP();
	VERIFY(tmp_gp);
	gamespy_gp::login_manager*	tmp_lmngr = MainMenu()->GetLoginMngr();
	VERIFY(tmp_lmngr);
	tmp_lmngr->login(tmp_email, tmp_nick, tmp_password, gamespy_gp::login_operation_cb());
}

void CCC_GameSpyLogout::Execute(LPCSTR args)
{
	
	VERIFY(MainMenu() && MainMenu()->GetGS());
	gamespy_gp::login_manager*	tmp_lmngr = MainMenu()->GetLoginMngr();
	VERIFY(tmp_lmngr);
	tmp_lmngr->logout();
}

static char const * print_time(time_t const & src_time, string64 & dest_time)
{
	tm* tmp_tm = _localtime64(&src_time);
	xr_sprintf(dest_time, sizeof(dest_time),
		"%02d.%02d.%d_%02d:%02d:%02d",
		tmp_tm->tm_mday, 
		tmp_tm->tm_mon+1, 
		tmp_tm->tm_year+1900, 
		tmp_tm->tm_hour, 
		tmp_tm->tm_min, 
		tmp_tm->tm_sec
	);
	return dest_time;
}

void CCC_GameSpyPrintProfile::Execute(LPCSTR args)
{
	VERIFY(MainMenu() && MainMenu()->GetGS());
	gamespy_gp::login_manager*	tmp_lmngr	= MainMenu()->GetLoginMngr();
	gamespy_gp::profile const * tmp_profile = tmp_lmngr->get_current_profile();
	if (tmp_profile)
	{
		Msg("- Current profile:");
		Msg("- ProfileID  : %u", tmp_profile->m_profile_id);
		Msg("- UniqueNick : %s", tmp_profile->m_unique_nick.c_str());

		gamespy_profile::profile_store* tmp_store = MainMenu()->GetProfileStore();
		if (!tmp_store)
		{
			Msg("! No profile store available");
			return;
		}
		
		Msg("- Player awards:");
		gamespy_profile::all_awards_t const & tmp_awards = tmp_store->get_awards();
		for (gamespy_profile::all_awards_t::const_iterator i = tmp_awards.begin(),
			ie = tmp_awards.end(); i < ie; ++i)
		{
			string64 rdate_str;
			rdate_str[0]	= 0;
			print_time		(i->second.m_last_reward_date, rdate_str);
			Msg("- (award: %s), (count: %u), (last reward date: %s)",
				gamespy_profile::get_award_name(static_cast<gamespy_profile::enum_awards_t>(i->first)),
				i->second.m_count,
				rdate_str);
		}

		Msg("- Best player scores:");
		gamespy_profile::all_best_scores_t const & tmp_best_scores = tmp_store->get_best_scores();
		for (gamespy_profile::all_best_scores_t::const_iterator i = tmp_best_scores.begin(),
			ie = tmp_best_scores.end(); i < ie; ++i)
		{
			Msg("- (score: %s), (score: %d)", 
				gamespy_profile::get_best_score_name(static_cast<gamespy_profile::enum_best_score_type>(i->first)),
				i->second
			);
		}
	} else
	{
		Msg("- No profile. You are not loged in.");
	}
}

void CCC_GameSpySuggestUNicks::Execute(LPCSTR args)
{
	
	VERIFY(MainMenu() && MainMenu()->GetGS());
	string256	tmp_unick;
	sscanf_s(args, "%s", tmp_unick, sizeof(tmp_unick));
	gamespy_gp::account_manager* tmp_amngr = MainMenu()->GetAccountMngr();
	VERIFY(tmp_amngr);
	tmp_amngr->suggest_unique_nicks(tmp_unick, gamespy_gp::suggest_nicks_cb());
}

void CCC_GameSpyRegisterUniqueNick::Execute(LPCSTR args)
{
	VERIFY(MainMenu() && MainMenu()->GetGS());
	gamespy_gp::login_manager::unique_nick_t	tmp_unick;
	sscanf_s(args, "%s", tmp_unick, sizeof(tmp_unick));
	gamespy_gp::login_manager*	tmp_lmngr = MainMenu()->GetLoginMngr();
	VERIFY(tmp_lmngr);
	tmp_lmngr->set_unique_nick(tmp_unick, gamespy_gp::login_operation_cb());
}


void CCC_GameSpyDeleteProfile::Execute(LPCSTR args)
{
	
	VERIFY(MainMenu() && MainMenu()->GetGS());
	gamespy_gp::account_manager* tmp_amngr = MainMenu()->GetAccountMngr();
	VERIFY(tmp_amngr);
	tmp_amngr->delete_profile(gamespy_gp::account_operation_cb());
}

static gamespy_profile::all_best_scores_t debug_best_scores;

void CCC_GameSpyProfile::Execute(LPCSTR args)
{
	VERIFY(MainMenu());
	gamespy_gp::login_manager* tmp_lmngr			= MainMenu()->GetLoginMngr();
	VERIFY(tmp_lmngr);
	gamespy_profile::profile_store* tmp_prof_store	= MainMenu()->GetProfileStore();
	VERIFY(tmp_prof_store);
	
	gamespy_gp::profile const * tmp_curr_prof	= tmp_lmngr->get_current_profile();
	if (!tmp_curr_prof)
	{
		Msg("- No profile. You are not loged in.");
		return;
	}

	string256	tmp_command;
	sscanf_s(args, "%s", tmp_command, sizeof(tmp_command));
	if (!xr_strcmp(tmp_command, "load"))
	{
		/*tmp_prof_store->set_current_profile(
			tmp_curr_prof->m_profile_id,
			tmp_curr_prof->m_login_ticket.c_str()
		);
		parameters_tuple1<gamespy_profile::store_operation_cb> tmp_args;
		tmp_prof_store->load_profile(tmp_args);*/
		tmp_prof_store->load_current_profile(
			gamespy_profile::store_operation_cb(),
			gamespy_profile::store_operation_cb());
	} else if (!xr_strcmp(tmp_command, "reward"))
	{
	}
	else if (!xr_strcmp(tmp_command, "bestscore"))
	{
	}
}