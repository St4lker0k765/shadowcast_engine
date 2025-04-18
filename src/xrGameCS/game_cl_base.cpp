#include "pch_script.h"
#include "hudmanager.h"
#include "game_cl_base.h"
#include "level.h"
#include "GamePersistent.h"
#include "UIGameCustom.h"
#include "script_engine.h"
#include "xr_Level_controller.h"
#include "ui/UIMainIngameWnd.h"
#include "UI/UIGameTutorial.h"
#include "UI/UIMessagesWindow.h"
#include "string_table.h"
#include "../xrEngine/Environment.h"

EGameIDs ParseStringToGameType	(LPCSTR str);
LPCSTR GameTypeToString			(EGameIDs gt, bool bShort);

game_cl_GameState::game_cl_GameState()
{

	local_player				= 0;
	m_game_type_name			= 0;

	shedule.t_min				= 5;
	shedule.t_max				= 20;
	m_game_ui_custom			= NULL;
	shedule_register			();

	m_u16VotingEnabled			= 0;
	m_bServerControlHits		= true;
}

game_cl_GameState::~game_cl_GameState()
{
	PLAYERS_MAP_IT I	= players.begin();
	for(;I!=players.end(); ++I)
		xr_delete(I->second);
	players.clear();

	shedule_unregister();

}

void	game_cl_GameState::net_import_GameTime		(NET_Packet& P)
{
	u64				GameTime;
	P.r_u64			(GameTime);
	float			TimeFactor;
	P.r_float		(TimeFactor);

	Level().SetGameTimeFactor	(GameTime,TimeFactor);

	u64				GameEnvironmentTime;
	P.r_u64			(GameEnvironmentTime);
	float			EnvironmentTimeFactor;
	P.r_float		(EnvironmentTimeFactor);

	u64 OldTime = Level().GetEnvironmentGameTime();
	Level().SetEnvironmentGameTimeFactor	(GameEnvironmentTime,EnvironmentTimeFactor);
	if (OldTime > GameEnvironmentTime)
		GamePersistent().Environment().Invalidate();
}

void	game_cl_GameState::net_import_state	(NET_Packet& P)
{
	// Generic
	P.r_clientID	(local_svdpnid);
	P.r_u32			((u32&)m_type);
	
	u16 ph;
	P.r_u16			(ph);
	
	if(Phase()!=ph)
		switch_Phase(ph);

	P.r_s32			(m_round);
	P.r_u32			(m_start_time);
	m_u16VotingEnabled = u16(P.r_u8());
	m_bServerControlHits = !!P.r_u8();	

	// Players
	u16	p_count;
	P.r_u16			(p_count);
	
	PLAYERS_MAP players_new;

/*
	players.clear	();
*/
	PLAYERS_MAP_IT I;
	for (u16 p_it=0; p_it<p_count; ++p_it)
	{
		ClientID			ID;
		P.r_clientID		(ID);
		
		game_PlayerState*   IP;
		I = players.find(ID);
		if( I!=players.end() )
		{
			IP = I->second;
			//***********************************************
			u16 OldFlags = IP->flags__;
			u8 OldVote = IP->m_bCurrentVoteAgreed;
			//-----------------------------------------------
			IP->net_Import(P);
			//-----------------------------------------------
			if (OldFlags != IP->flags__)
				if (Type() != eGameIDSingle) OnPlayerFlagsChanged(IP);
			if (OldVote != IP->m_bCurrentVoteAgreed)
				OnPlayerVoted(IP);
			//***********************************************

			players_new.insert(std::make_pair(ID,IP));
			players.erase(I);
		}else{
			IP = createPlayerState();
			IP->net_Import		(P);

			if (Type() != eGameIDSingle) OnPlayerFlagsChanged(IP);

			players_new.insert(std::make_pair(ID,IP));
		}
		if (IP->testFlag(GAME_PLAYER_FLAG_LOCAL) ) local_player = IP;
	}

	I	= players.begin();
	for(;I!=players.end(); ++I)
		xr_delete(I->second);
	players.clear();
	
	players = players_new;

	net_import_GameTime(P);
}

void	game_cl_GameState::net_import_update(NET_Packet& P)
{
	// Read
	ClientID			ID;
	P.r_clientID		(ID);

	// Update
	PLAYERS_MAP_IT I	= players.find(ID);
	/*VERIFY2(I != players.end(), 
		make_string("Player ClientID = %d not found in players map", ID.value()).c_str());*/
	if (players.end()!=I)
	{
		game_PlayerState* IP		= I->second;
//		CopyMemory	(&IP,&PS,sizeof(PS));		
		//***********************************************
		u16 OldFlags = IP->flags__;
		u8 OldVote = IP->m_bCurrentVoteAgreed;
		//-----------------------------------------------
		IP->net_Import(P);
		//-----------------------------------------------
		if (OldFlags != IP->flags__)
			if (Type() != eGameIDSingle) OnPlayerFlagsChanged(IP);
		if (OldVote != IP->m_bCurrentVoteAgreed)
			OnPlayerVoted(IP);
		//***********************************************
	}
	else
	{
		game_PlayerState*	PS = createPlayerState();
		PS->net_Import		(P);
		if (Type() != eGameIDSingle) OnPlayerFlagsChanged(PS);
		xr_delete(PS);
	};

	//Syncronize GameTime
	net_import_GameTime (P);
}

void	game_cl_GameState::net_signal		(NET_Packet& P)
{
}

void game_cl_GameState::TranslateGameMessage	(u32 msg, NET_Packet& P)
{
	CStringTable st;

	string512 Text;
	char	Color_Main[]	= "%c[255,192,192,192]";
	LPSTR	Color_Teams[3]	= {"%c[255,255,240,190]", "%c[255,64,255,64]", "%c[255,64,64,255]"};

	switch (msg)
	{
	case GAME_EVENT_PLAYER_CONNECTED:
		{

#ifdef BATTLEYE
			if ( g_pGameLevel && Level().battleye_system.GetTestClient() )
			{
				bool res_battleye = Level().battleye_system.LoadClient();
				VERIFY( res_battleye );
			}
#endif // BATTLEYE

			string64 PlayerName;
			// [19.11.07] Alexander Maniluk: changed packet format
			game_PlayerState*	PS = createPlayerState();
			VERIFY2(PS, "failed to create player state");
			ClientID newClientId;
			P.r_clientID(newClientId);
			PS->net_Import		(P);
			P.r_stringZ(PlayerName);
			if (Type() != eGameIDSingle)
			{
				players.insert(std::make_pair(newClientId, PS));
				OnNewPlayerConnected(newClientId);
			}
			sprintf_s(Text, "%s%s %s%s",Color_Teams[0],PlayerName,Color_Main,*st.translate("mp_connected"));
			CommonMessageOut(Text);
			//---------------------------------------
			Msg("%s connected", PlayerName);
		}break;
	case GAME_EVENT_PLAYER_DISCONNECTED:
		{
			string64 PlayerName;
			P.r_stringZ(PlayerName);

			sprintf_s(Text, "%s%s %s%s",Color_Teams[0],PlayerName,Color_Main,*st.translate("mp_disconnected"));
			CommonMessageOut(Text);
			//---------------------------------------
			Msg("%s disconnected", PlayerName);
		}break;
	case GAME_EVENT_PLAYER_ENTERED_GAME:
		{
			string64 PlayerName;
			P.r_stringZ(PlayerName);

			sprintf_s(Text, "%s%s %s%s",Color_Teams[0],PlayerName,Color_Main,*st.translate("mp_entered_game"));
			CommonMessageOut(Text);
		}break;
	default:
		{
			R_ASSERT2(0,"Unknown Game Message");
		}break;
	};

}

void	game_cl_GameState::OnGameMessage	(NET_Packet& P)
{
	VERIFY	(this && &P);
	u32 msg	;
	P.r_u32	(msg);

	TranslateGameMessage(msg, P);
};

game_PlayerState* game_cl_GameState::lookat_player()
{
	CObject* current_entity = Level().CurrentEntity();
	if (current_entity)
	{
		return GetPlayerByGameID(current_entity->ID());
	}
	return NULL;
}

game_PlayerState* game_cl_GameState::GetPlayerByGameID(u32 GameID)
{
	PLAYERS_MAP_IT I=players.begin();
	PLAYERS_MAP_IT E=players.end();

	for (;I!=E;++I)
	{
		game_PlayerState* P = I->second;
		if (P->GameID == GameID) return P;
	};
	return NULL;
};

game_PlayerState* game_cl_GameState::GetPlayerByOrderID		(u32 idx)
{
	PLAYERS_MAP_IT I = players.begin();
	std::advance(I,idx);
	game_PlayerState* ps = I->second;
	return ps;
}

ClientID game_cl_GameState::GetClientIDByOrderID	(u32 idx)
{
	PLAYERS_MAP_IT I = players.begin();
	std::advance(I,idx);
	return I->first;
}



void game_cl_GameState::CommonMessageOut (LPCSTR msg)
{
	if (!HUD().GetUI()) return;
	HUD().GetUI()->m_pMessagesWnd->AddLogMessage(msg);
}

float game_cl_GameState::shedule_Scale		()
{
	return 1.0f;
}


void game_cl_GameState::shedule_Update		(u32 dt)
{
	ISheduled::shedule_Update	(dt);

	if(!m_game_ui_custom){
		if( HUD().GetUI() )
			m_game_ui_custom = HUD().GetUI()->UIGame();
	} 
	//---------------------------------------
	switch (Phase())
	{
	case GAME_PHASE_INPROGRESS:
		{
		}break;
	default:
		{
		}break;
	};
};

void game_cl_GameState::StartStopMenu(CUIDialogWnd* pDialog, bool bDoHideIndicators)
{
	HUD().GetUI()->StartStopMenu(pDialog, bDoHideIndicators);
}

void game_cl_GameState::sv_GameEventGen(NET_Packet& P)
{
	P.w_begin	(M_EVENT);
	P.w_u32		(Level().timeServer());
	P.w_u16		( u16(GE_GAME_EVENT&0xffff) );
	P.w_u16		(0);//dest==0
}

void	game_cl_GameState::sv_EventSend(NET_Packet& P)
{
	Level().Send(P,net_flags(TRUE,TRUE));
}

bool game_cl_GameState::IR_OnKeyboardPress		(int dik)
{
	if(local_player && !local_player->IsSkip())
		return OnKeyboardPress( get_binded_action(dik) );
	else
		return false;
}

bool game_cl_GameState::IR_OnKeyboardRelease	(int dik)
{
	if(local_player && !local_player->IsSkip())
		return OnKeyboardRelease( get_binded_action(dik) );
	else
		return false;
}

bool game_cl_GameState::IR_OnMouseMove			(int dx, int dy)
{
	return false;	
}
bool game_cl_GameState::IR_OnMouseWheel			(int direction)
{
	return false;
}

void game_cl_GameState::u_EventGen(NET_Packet& P, u16 type, u16 dest)
{
	P.w_begin	(M_EVENT);
	P.w_u32		(Level().timeServer());
	P.w_u16		(type);
	P.w_u16		(dest);
}

void game_cl_GameState::u_EventSend(NET_Packet& P)
{
	Level().Send(P,net_flags(TRUE,TRUE));
}

void				game_cl_GameState::OnSwitchPhase			(u32 old_phase, u32 new_phase)
{
	switch (old_phase)
	{
	case GAME_PHASE_INPROGRESS:
		{
		}break;
	default:
		{
		}break;
	};

	switch (new_phase)
	{
		case GAME_PHASE_INPROGRESS:
			{
			}break;
		default:
			{
			}break;
	}	
}

void				game_cl_GameState::SendPickUpEvent		(u16 ID_who, u16 ID_what)
{
	NET_Packet P;
	u_EventGen(P,GE_OWNERSHIP_TAKE, ID_who);
	//u_EventGen(P,GE_OWNERSHIP_TAKE_MP_FORCED, ID_who);
	P.w_u16(ID_what);
	u_EventSend(P);
};

void game_cl_GameState::set_type_name(LPCSTR s)	
{ 
	EGameIDs gid =			ParseStringToGameType	(s);
	m_game_type_name		= GameTypeToString		(gid, false); 
	if(OnClient())
	{
		strcpy_s					(g_pGamePersistent->m_game_params.m_game_type, m_game_type_name.c_str());
		g_pGamePersistent->OnGameStart();
	}
};
void game_cl_GameState::reset_ui()
{
	if (g_dedicated_server)	return;
	if (!HUD().GetUI())		return;

	if(!m_game_ui_custom)
		m_game_ui_custom = HUD().GetUI()->UIGame();

	m_game_ui_custom->reset_ui					();

	HUD().GetUI()->UIMainIngameWnd->reset_ui	();

	if (HUD().GetUI()->MainInputReceiver())
		HUD().GetUI()->StartStopMenu			(HUD().GetUI()->MainInputReceiver(),true);
}
