// xrServer.cpp: implementation of the xrServer class.
//
//////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer.h"
#include "xrMessages.h"
#include "xrServer_Objects_ALife_All.h"
#include "level.h"
#include "game_cl_base.h"
#include "ai_space.h"
#include "../xrEngine/IGame_Persistent.h"
#include "string_table.h"
#include "../xrEngine/XR_IOConsole.h"
//#include "script_engine.h"
#include "ui/UIInventoryUtilities.h"
#include "file_transfer.h"
#pragma warning(push)
#pragma warning(disable:4995)
#include <malloc.h>
#pragma warning(pop)
#include <functional>

using namespace std::placeholders;

xrClientData::xrClientData	():IClient(Device.GetTimerGlobal())
{
	ps			= Level().Server->game->createPlayerState();
	ps->clear	();
	ps->m_online_time	= Level().timeServer();

	Clear		();
}

void	xrClientData::Clear()
{
	owner									= NULL;
	net_Ready								= FALSE;
	net_Accepted							= FALSE;
	net_PassUpdates							= TRUE;
	m_ping_warn.m_maxPingWarnings			= 0;
	m_ping_warn.m_dwLastMaxPingWarningTime	= 0;
	m_admin_rights.m_has_admin_rights		= FALSE;
};


xrClientData::~xrClientData()
{
	xr_delete(ps);
}


xrServer::xrServer():IPureServer(Device.GetTimerGlobal(), g_dedicated_server)
{
	m_iCurUpdatePacket = 0;
	m_file_transfers = NULL;
	m_aUpdatePackets.push_back(NET_Packet());
	m_aDelayedPackets.clear();
}

xrServer::~xrServer()
{
	struct ClientDestroyer
	{
		static bool true_generator(IClient*)
		{
			return true;
		}
	};
	IClient* tmp_client = net_players.GetFoundClient(&ClientDestroyer::true_generator);
	while (tmp_client)
	{
		client_Destroy(tmp_client);
		tmp_client = net_players.GetFoundClient(&ClientDestroyer::true_generator);
	}
	m_aUpdatePackets.clear();
	m_aDelayedPackets.clear();
	entities.clear();
}

bool  xrServer::HasBattlEye()
{
#ifdef BATTLEYE
	return (g_pGameLevel && Level().battleye_system.server)? true : false;
#else
	return false;
#endif // BATTLEYE
}

//--------------------------------------------------------------------

CSE_Abstract*	xrServer::ID_to_entity		(u16 ID)
{
	// #pragma todo("??? to all : ID_to_entity - must be replaced to 'game->entity_from_eid()'")	
	if (0xffff==ID)				return 0;
	xrS_entities::iterator	I	= entities.find	(ID);
	if (entities.end()!=I)		return I->second;
	else						return 0;
}

//--------------------------------------------------------------------
IClient*	xrServer::client_Create		()
{
	return xr_new<xrClientData> ();
}
void		xrServer::client_Replicate	()
{
}

IClient*	xrServer::client_Find_Get	(ClientID ID)
{
	DWORD	dwPort			= 0;
	
	struct AddressSearcherPredicate
	{
		ip_address			m_cAddress;
		bool operator()(IClient* client)
		{
			return client->m_cAddress == m_cAddress;
		};
	};
	AddressSearcherPredicate	search_predicate;


	if ( !psNET_direct_connect )
		GetClientAddress( ID, search_predicate.m_cAddress, &dwPort );
	else
		search_predicate.m_cAddress.set( "127.0.0.1" );

	IClient* newCL = client_Create();
	newCL->ID = ID;
	if(!psNET_direct_connect)
	{
		newCL->m_cAddress	= search_predicate.m_cAddress;	
		newCL->m_dwPort		= dwPort;
	}

	newCL->server			= this;
	net_players.AddNewClient(newCL);

#ifndef MASTER_GOLD
	Msg		("# Player not found. New player created.");
#endif // #ifndef MASTER_GOLD
	return newCL;
};

INT	g_sv_Client_Reconnect_Time = 0;

void		xrServer::client_Destroy	(IClient* C)
{
	// Delete assosiated entity
	// xrClientData*	D = (xrClientData*)C;
	// CSE_Abstract* E = D->owner;
	IClient* deleted_client = net_players.FindAndEraseClient(
		std::bind(std::equal_to<IClient*>(), C, _1)
	);
	if (deleted_client)
	{
		xr_delete(deleted_client);
	}
	IClient* alife_client = net_players.FindAndEraseClient(
		std::bind(std::equal_to<IClient*>(), C, _1)
	);
	//VERIFY(alife_client);
	if (alife_client)
	{
		CSE_Abstract* pOwner	= static_cast<xrClientData*>(alife_client)->owner;
		CSE_Spectator* pS		= smart_cast<CSE_Spectator*>(pOwner);
		if (pS)
		{
			NET_Packet			P;
			P.w_begin			(M_EVENT);
			P.w_u32				(Level().timeServer());//Device.TimerAsync());
			P.w_u16				(GE_DESTROY);
			P.w_u16				(pS->ID);
			SendBroadcast		(C->ID,P,net_flags(TRUE,TRUE));
		};

		DelayedPacket pp;
		pp.SenderID = alife_client->ID;
		xr_deque<DelayedPacket>::iterator it;
		do{
			it						=std::find(m_aDelayedPackets.begin(),m_aDelayedPackets.end(),pp);
			if(it!=m_aDelayedPackets.end())
			{
				m_aDelayedPackets.erase	(it);
				Msg("removing packet from delayed event storage");
			}else
				break;
		}while(true);
		
		if (pOwner)
		{
			game->CleanDelayedEventFor(pOwner->ID);
		}
	}
}

//--------------------------------------------------------------------
int	g_Dump_Update_Write = 0;

#ifdef DEBUG
INT g_sv_SendUpdate = 0;
#endif

void xrServer::Update	()
{
	if (Level().IsDemoPlayStarted() || Level().IsDemoPlayFinished())
		return;								//diabling server when demo is playing

	NET_Packet		Packet;

	VERIFY						(verify_entities());

	ProceedDelayedPackets();
	// game update
	game->ProcessDelayedEvent();
	game->Update						();

	// spawn queue
	u32 svT								= Device.TimerAsync();
	while (!(q_respawn.empty() || (svT<q_respawn.begin()->timestamp)))
	{
		// get
		svs_respawn	R					= *q_respawn.begin();
		q_respawn.erase					(q_respawn.begin());

		// 
		CSE_Abstract* E					= ID_to_entity(R.phantom);
		E->Spawn_Write		(Packet,FALSE);
		u16								ID;
		Packet.r_begin		(ID);
		R_ASSERT(M_SPAWN==ID);
		ClientID						clientID; 
		clientID.set(0xffff);
		Process_spawn		(Packet,clientID);
	}


	SendUpdatesToAll();


	if (game->sv_force_sync)	Perform_game_export();

	VERIFY						(verify_entities());

	PerformCheckClientsForMaxPing	();

	Flush_Clients_Buffers			();
	
	if( 0==(Device.dwFrame%100) )//once per 100 frames
	{
		UpdateBannedList();
	}
}

void _stdcall xrServer::SendUpdateTo(IClient* client)
{
	xrClientData*	xr_client = static_cast<xrClientData*>(client);
	VERIFY			(xr_client);
	if (!xr_client->net_Ready)
		return;

	if (!HasBandwidth(client) 
#ifdef DEBUG 
			&& !g_sv_SendUpdate
#endif
		)
	{
		return;
	}
	
	NET_Packet				Packet;
	u16 PacketType			= M_UPDATE;
	Packet.w_begin			(PacketType);
	game->net_Export_Update	(Packet, xr_client->ID, xr_client->ID);
	SendTo					(xr_client->ID, Packet, net_flags(FALSE,TRUE));

	if (!IsUpdatePacketsReady())
	{
		MakeUpdatePackets(Packet);
	} else
	{
		R_ASSERT(Packet.B.count == m_first_packet_size);
		InsertFirstPacketToUpdate(Packet);
	}
	SendUpdatePacketsToClient(xr_client->ID);
}
bool xrServer::IsUpdatePacketsReady()
{
	return m_aUpdatePackets[0].B.count != 0;
}

void xrServer::InsertFirstPacketToUpdate(NET_Packet const & P)
{
	m_aUpdatePackets[0].w_seek(0, P.B.data, P.B.count);
	m_first_packet_size = P.B.count;
}

void xrServer::MakeUpdatePackets(NET_Packet const & firstExportPacket)
{
	NET_Packet						tmpPacket;			
	u32								position;
	NET_Packet* pCurUpdatePacket	= &(m_aUpdatePackets[0]);
	
	m_aUpdatePackets[0].w(firstExportPacket.B.data, firstExportPacket.B.count);
	m_first_packet_size = firstExportPacket.B.count;

	InsertFirstPacketToUpdate		(firstExportPacket);
	
	xrS_entities::iterator I	= entities.begin();
	xrS_entities::iterator E	= entities.end();
	for (; I!=E; ++I)
	{//all entities
		CSE_Abstract&	Test			= *(I->second);

		if (0==Test.owner)								continue;
		if (!Test.net_Ready)							continue;
		if (Test.s_flags.is(M_SPAWN_OBJECT_PHANTOM))	continue;	// Surely: phantom
		if (!Test.Net_Relevant() )						continue;

		/*
		//. this for test purposes, since we do not need
		//. to send updates for objects which are local
		//. to the client we send
		if (Test.owner == Client)
			continue;
		*/

		tmpPacket.B.count				= 0;
		// write specific data
		{
			tmpPacket.w_u16					(Test.ID);
			tmpPacket.w_chunk_open8			(position);
			Test.UPDATE_Write				(tmpPacket);
			u32 ObjectSize					= u32(tmpPacket.w_tell()-position)-sizeof(u8);
			tmpPacket.w_chunk_close8		(position);

			if (ObjectSize == 0)			continue;					
#ifdef DEBUG
			if (g_Dump_Update_Write) Msg("* %s : %d", Test.name(), ObjectSize);
#endif

			if (pCurUpdatePacket->B.count + tmpPacket.B.count >= NET_PacketSizeLimit)
			{
				m_iCurUpdatePacket++;
				if (m_aUpdatePackets.size() == m_iCurUpdatePacket)
					m_aUpdatePackets.push_back(NET_Packet());
				pCurUpdatePacket = &(m_aUpdatePackets[m_iCurUpdatePacket]);
				pCurUpdatePacket->w_begin(M_UPDATE_OBJECTS);						
			}
			pCurUpdatePacket->w(tmpPacket.B.data, tmpPacket.B.count);
		}
	}//all entities
}

void xrServer::SendUpdatePacketsToClient(ClientID clientId)
{

//	if (g_Dump_Update_Write) Msg("----------------------- ");
	for (u32 p =0; p<=m_iCurUpdatePacket; p++)
	{
		NET_Packet& ToSend	= m_aUpdatePackets[p];
		if (ToSend.B.count>2)
		{
			SendTo			(clientId,ToSend,net_flags(FALSE,TRUE));
		}
	}
}

void xrServer::SendUpdatesToAll()
{
	if (IsGameTypeSingle())
		return;
	
	KickCheaters();

	fastdelegate::FastDelegate1<IClient*,void> sendtofd;
	sendtofd.bind(this, &xrServer::SendUpdateTo);

	NET_Packet* pCurUpdatePacket	= &(m_aUpdatePackets[0]);
	pCurUpdatePacket->B.count		= 0;
	m_iCurUpdatePacket				= 0;

	ForEachClientDoSender(sendtofd);
	
	if (m_file_transfers)
	{
		m_file_transfers->update_transfer();
		m_file_transfers->stop_obsolete_receivers();
	}

#ifdef DEBUG
	g_sv_SendUpdate = 0;
#endif			
	if (game->sv_force_sync)	Perform_game_export();
	VERIFY						(verify_entities());
#ifdef BATTLEYE
	if ( g_pGameLevel )
	{
		Level().battleye_system.UpdateServer( this );
	}
#endif // BATTLEYE
}

xr_vector<shared_str>	_tmp_log;
void console_log_cb(LPCSTR text)
{
	_tmp_log.push_back	(text);
}

u32 xrServer::OnDelayedMessage	(NET_Packet& P, ClientID sender)			// Non-Zero means broadcasting with "flags" as returned
{
	u16						type;
	P.r_begin				(type);

	//csPlayers.Enter			();

	VERIFY							(verify_entities());
	xrClientData* CL				= ID_to_client(sender);
	//R_ASSERT2						(CL, make_string("packet type [%d]",type).c_str());

	switch (type)
	{
		case M_CLIENT_REQUEST_CONNECTION_DATA:
		{
			IClient* tmp_client = net_players.GetFoundClient(
				ClientIdSearchPredicate(sender));
			VERIFY(tmp_client);
			OnCL_Connected(tmp_client);
			//OnCL_Connected				(CL);
		}break;
		case M_REMOTE_CONTROL_CMD:
		{
			if(CL->m_admin_rights.m_has_admin_rights)
			{
				string1024			buff;
				P.r_stringZ			(buff);
				Msg("* Radmin [%s] is running command: %s", CL->name.c_str(), buff);
				SetLogCB			(console_log_cb);
				_tmp_log.clear		();
				LPSTR		result_command;
				string64	tmp_number_str;
				sprintf_s(tmp_number_str, " raid:%u", CL->ID.value());
				STRCONCAT(result_command, buff, tmp_number_str);
				Console->Execute	(result_command);
				SetLogCB			(NULL);

				NET_Packet			P_answ;			
				for(u32 i=0; i<_tmp_log.size(); ++i)
				{
					P_answ.w_begin		(M_REMOTE_CONTROL_CMD);
					P_answ.w_stringZ	(_tmp_log[i]);
					SendTo				(sender,P_answ,net_flags(TRUE,TRUE));
				}
			}else
			{
				NET_Packet			P_answ;			
				P_answ.w_begin		(M_REMOTE_CONTROL_CMD);
				P_answ.w_stringZ	("you dont have admin rights");
				SendTo				(sender,P_answ,net_flags(TRUE,TRUE));
			}
		}break;
		case M_FILE_TRANSFER:
		{
			m_file_transfers->on_message(&P, sender);
		}break;
	}
	VERIFY							(verify_entities());

	//csPlayers.Leave					();
	return 0;
}

u32	xrServer::OnMessageSync(NET_Packet& P, ClientID sender)
{
	csMessage.Enter();
	u32 ret = OnMessage(P, sender);
	csMessage.Leave();
	return ret;
}

extern	float	g_fCatchObjectTime;
u32 xrServer::OnMessage	(NET_Packet& P, ClientID sender)			// Non-Zero means broadcasting with "flags" as returned
{
	u16			type;
	P.r_begin	(type);

	VERIFY							(verify_entities());
	xrClientData* CL				= ID_to_client(sender);

	switch (type)
	{
	case M_UPDATE:	
		{
			Process_update			(P,sender);						// No broadcast
			VERIFY					(verify_entities());
		}break;
	case M_SPAWN:	
		{
			if (CL->flags.bLocal)
				Process_spawn		(P,sender);	

			VERIFY					(verify_entities());
		}break;
	case M_EVENT:	
		{
			Process_event			(P,sender);
			VERIFY					(verify_entities());
		}break;
	case M_EVENT_PACK:
		{
			NET_Packet	tmpP;
			while (!P.r_eof())
			{
				tmpP.B.count		= P.r_u8();
				P.r					(&tmpP.B.data, tmpP.B.count);

				OnMessage			(tmpP, sender);
			};			
		}break;
	case M_CL_UPDATE:
		{
			xrClientData* CL		= ID_to_client	(sender);
			if (!CL)				break;
			CL->net_Ready			= TRUE;

			if (!CL->net_PassUpdates)
				break;
			//-------------------------------------------------------------------
			u32 ClientPing = CL->stats.getPing();
			P.w_seek(P.r_tell()+2, &ClientPing, 4);
			//-------------------------------------------------------------------
			if (SV_Client) 
				SendTo	(SV_Client->ID, P, net_flags(TRUE, TRUE));
			VERIFY					(verify_entities());
		}break;
	case M_MOVE_PLAYERS_RESPOND:
		{
			xrClientData* CL		= ID_to_client	(sender);
			if (!CL)				break;
			CL->net_Ready			= TRUE;
			CL->net_PassUpdates		= TRUE;
		}break;
	//-------------------------------------------------------------------
	case M_CL_INPUT:
		{
			xrClientData* CL		= ID_to_client	(sender);
			if (CL)	CL->net_Ready	= TRUE;
			if (SV_Client) SendTo	(SV_Client->ID, P, net_flags(TRUE, TRUE));
			VERIFY					(verify_entities());
		}break;
	case M_GAMEMESSAGE:
		{
			SendBroadcast			(BroadcastCID,P,net_flags(TRUE,TRUE));
			VERIFY					(verify_entities());
		}break;
	case M_CLIENTREADY:
		{
			xrClientData* CL		= ID_to_client(sender);
			if ( CL )	
			{
				CL->net_Ready	= TRUE;
				CL->ps->DeathTime = Device.dwTimeGlobal;
				game->OnPlayerConnectFinished(sender);
				CL->ps->setName( CL->name.c_str() );
				
#ifdef BATTLEYE
				if ( g_pGameLevel && Level().battleye_system.server )
				{
					Level().battleye_system.server->AddConnected_OnePlayer( CL );
				}
#endif // BATTLEYE
			};
			game->signal_Syncronize	();
			VERIFY					(verify_entities());
		}break;
	case M_SWITCH_DISTANCE:
		{
			game->switch_distance	(P,sender);
			VERIFY					(verify_entities());
		}break;
	case M_CHANGE_LEVEL:
		{
			if (game->change_level(P,sender))
			{
				SendBroadcast		(BroadcastCID,P,net_flags(TRUE,TRUE));
			}
			VERIFY					(verify_entities());
		}break;
	case M_SAVE_GAME:
		{
			game->save_game			(P,sender);
			VERIFY					(verify_entities());
		}break;
	case M_LOAD_GAME:
		{
			game->load_game			(P,sender);
			SendBroadcast			(BroadcastCID,P,net_flags(TRUE,TRUE));
			VERIFY					(verify_entities());
		}break;
	case M_RELOAD_GAME:
		{
			SendBroadcast			(BroadcastCID,P,net_flags(TRUE,TRUE));
			VERIFY					(verify_entities());
		}break;
	case M_SAVE_PACKET:
		{
			Process_save			(P,sender);
			VERIFY					(verify_entities());
		}break;
	case M_CLIENT_REQUEST_CONNECTION_DATA:
		{
			AddDelayedPacket(P, sender);
		}break;
	case M_CHAT_MESSAGE:
		{
		}break;
	case M_SV_MAP_NAME:
		{
			xrClientData *l_pC			= ID_to_client(sender);
			OnProcessClientMapData		(P, l_pC->ID);
		}break;
	case M_SV_DIGEST:
		{
			R_ASSERT(CL);
			ProcessClientDigest			(CL, &P);
		}break;
	case M_CHANGE_LEVEL_GAME:
		{
			ClientID CID; CID.set		(0xffffffff);
			SendBroadcast				(CID,P,net_flags(TRUE,TRUE));
		}break;
	case M_CL_AUTH:
		{
			game->AddDelayedEvent		(P,GAME_EVENT_PLAYER_AUTH, 0, sender);
		}break;
	case M_STATISTIC_UPDATE:
		{
			SendBroadcast			(BroadcastCID,P,net_flags(TRUE,TRUE));
		}break;
	case M_STATISTIC_UPDATE_RESPOND:
		{		
		}break;
	case M_PLAYER_FIRE:
		{
			if (game)
				game->OnPlayerFire(sender, P);
		}break;
	case M_REMOTE_CONTROL_AUTH:
		{
			string512				reason;
			shared_str				user;
			shared_str				pass;
			P.r_stringZ				(user);
			if(0==stricmp(user.c_str(),"logoff"))
			{
				CL->m_admin_rights.m_has_admin_rights	= FALSE;
				strcpy_s				(reason,"logged off");
				Msg("# Remote administrator logged off.");
			}else
			{
				P.r_stringZ				(pass);
				bool res = CheckAdminRights(user, pass, reason);
				if(res){
					CL->m_admin_rights.m_has_admin_rights	= TRUE;
					CL->m_admin_rights.m_dwLoginTime		= Device.dwTimeGlobal;
					Msg("# User [%s] logged as remote administrator.", user.c_str());
				}else
					Msg("# User [%s] tried to login as remote administrator. Access denied.", user.c_str());

			}
			NET_Packet			P_answ;			
			P_answ.w_begin		(M_REMOTE_CONTROL_CMD);
			P_answ.w_stringZ	(reason);
			SendTo				(CL->ID,P_answ,net_flags(TRUE,TRUE));
		}break;

	case M_REMOTE_CONTROL_CMD:
		{
			AddDelayedPacket(P, sender);
		}break;
	case M_BATTLEYE:
		{
#ifdef BATTLEYE
			if ( g_pGameLevel )
			{
				Level().battleye_system.ReadPacketServer( sender.value(), &P );
			}
#endif // BATTLEYE
		}break;
	case M_FILE_TRANSFER:
		{
			AddDelayedPacket(P, sender);
		}break;
	case M_SECURE_MESSAGE:
		{
			OnSecureMessage(P, CL);
		}break;
	}

	VERIFY							(verify_entities());

	return							IPureServer::OnMessage(P, sender);
}

bool xrServer::CheckAdminRights(const shared_str& user, const shared_str& pass, string512& reason)
{
	bool res			= false;
	string_path			fn;
	FS.update_path		(fn,"$app_data_root$","radmins.ltx");
	if(FS.exist(fn))
	{
		CInifile			ini(fn);
		if(ini.line_exist("radmins",user.c_str()))
		{
			if (ini.r_string ("radmins",user.c_str()) == pass)
			{
				strcpy_s			(reason, sizeof(reason),"Access permitted.");
				res				= true;
			}else
			{
				strcpy_s			(reason, sizeof(reason),"Access denied. Wrong password.");
			}
		}else
			strcpy_s			(reason, sizeof(reason),"Access denied. No such user.");
	}else
		strcpy_s				(reason, sizeof(reason),"Access denied.");

	return				res;
}

void xrServer::SendTo_LL			(ClientID ID, void* data, u32 size, u32 dwFlags, u32 dwTimeout)
{
	if ((SV_Client && SV_Client->ID==ID) || psNET_direct_connect)
	{
		// optimize local traffic
		Level().OnMessage			(data,size);
	}
	else 
	{
		IClient* pClient = ID_to_client(ID);
		if (!pClient || !pClient->flags.bConnected)
			return;

		IPureServer::SendTo_Buf(ID,data,size,dwFlags,dwTimeout);
	}
}
void xrServer::SendBroadcast(ClientID exclude, NET_Packet& P, u32 dwFlags)
{
	struct ClientExcluderPredicate
	{
		ClientID id_to_exclude;
		ClientExcluderPredicate(ClientID exclude) :
			id_to_exclude(exclude)
		{}
		bool operator()(IClient* client)
		{
			xrClientData* tmp_client = static_cast<xrClientData*>(client);
			if (client->ID == id_to_exclude)
				return false;
			if (!client->flags.bConnected)
				return false;
			if (!tmp_client->net_Accepted)
				return false;
			return true;
		}
	};
	struct ClientSenderFunctor
	{
		xrServer*		m_owner;
		void*			m_data;
		u32				m_size;
		u32				m_dwFlags;
		ClientSenderFunctor(xrServer* owner, void* data, u32 size, u32 dwFlags) :
			m_owner(owner), m_data(data), m_size(size), m_dwFlags(dwFlags)
		{}
		void operator()(IClient* client)
		{
			m_owner->SendTo_LL(client->ID, m_data, m_size, m_dwFlags);			
		}
	};
	ClientSenderFunctor temp_functor(this, P.B.data, P.B.count, dwFlags);
	net_players.ForFoundClientsDo(ClientExcluderPredicate(exclude), temp_functor);
}
//--------------------------------------------------------------------
CSE_Abstract*	xrServer::entity_Create		(LPCSTR name)
{
	return F_entity_Create(name);
}

void			xrServer::entity_Destroy	(CSE_Abstract *&P)
{
#ifdef DEBUG
if( dbg_net_Draw_Flags.test( dbg_destroy ) )
		Msg	("xrServer::entity_Destroy : [%d][%s][%s]",P->ID,P->name(),P->name_replace());
#endif
	R_ASSERT					(P);
	entities.erase				(P->ID);
	m_tID_Generator.vfFreeID	(P->ID,Device.TimerAsync());

	if(P->owner && P->owner->owner==P)
		P->owner->owner		= NULL;

	P->owner = NULL;
	if (!ai().get_alife() || !P->m_bALifeControl)
	{
		F_entity_Destroy		(P);
	}
}

//--------------------------------------------------------------------
void			xrServer::Server_Client_Check	( IClient* CL )
{
	if (SV_Client && SV_Client->ID == CL->ID)
	{
		if (!CL->flags.bConnected)
		{
			SV_Client = NULL;
		};
		return;
	};

	if (SV_Client && SV_Client->ID != CL->ID)
	{
		return;
	};


	if (!CL->flags.bConnected) 
	{
		return;
	};

	if( CL->process_id == GetCurrentProcessId() )
	{
		CL->flags.bLocal	= 1;
		SV_Client			= (xrClientData*)CL;
		Msg( "New SV client %s", SV_Client->name.c_str() );
	}else
	{
		CL->flags.bLocal	= 0;
	}
};

bool		xrServer::OnCL_QueryHost		() 
{
	if (game->Type() == eGameIDSingle) return false;
	return (GetClientsCount() != 0); 
};

CSE_Abstract*	xrServer::GetEntity			(u32 Num)
{
	xrS_entities::iterator	I=entities.begin(),E=entities.end();
	for (u32 C=0; I!=E; ++I,++C)
	{
		if (C == Num) return I->second;
	};
	return NULL;
};


#ifdef DEBUG

static	BOOL	_ve_initialized			= FALSE;
static	BOOL	_ve_use					= TRUE;

bool xrServer::verify_entities				() const
{
	if (!_ve_initialized)	{
		_ve_initialized					= TRUE;
		if (strstr(Core.Params,"-~ve"))	_ve_use=FALSE;
	}
	if (!_ve_use)						return true;

	xrS_entities::const_iterator		I = entities.begin();
	xrS_entities::const_iterator		E = entities.end();
	for ( ; I != E; ++I) {
		VERIFY2							((*I).first != 0xffff,"SERVER : Invalid entity id as a map key - 0xffff");
		VERIFY2							((*I).second,"SERVER : Null entity object in the map");
		VERIFY3							((*I).first == (*I).second->ID,"SERVER : ID mismatch - map key doesn't correspond to the real entity ID",(*I).second->name_replace());
		verify_entity					((*I).second);
	}
	return								(true);
}

void xrServer::verify_entity				(const CSE_Abstract *entity) const
{
	VERIFY(entity->m_wVersion!=0);
	if (entity->ID_Parent != 0xffff) {
		xrS_entities::const_iterator	J = entities.find(entity->ID_Parent);
		VERIFY2							(J != entities.end(),
			make_string("SERVER : Cannot find parent in the map [%s][%s]",entity->name_replace(),
			entity->name()).c_str());
		VERIFY3							((*J).second,"SERVER : Null entity object in the map",entity->name_replace());
		VERIFY3							((*J).first == (*J).second->ID,"SERVER : ID mismatch - map key doesn't correspond to the real entity ID",(*J).second->name_replace());
		VERIFY3							(std::find((*J).second->children.begin(),(*J).second->children.end(),entity->ID) != (*J).second->children.end(),"SERVER : Parent/Children relationship mismatch - Object has parent, but corresponding parent doesn't have children",(*J).second->name_replace());
	}

	xr_vector<u16>::const_iterator		I = entity->children.begin();
	xr_vector<u16>::const_iterator		E = entity->children.end();
	for ( ; I != E; ++I) {
		VERIFY3							(*I != 0xffff,"SERVER : Invalid entity children id - 0xffff",entity->name_replace());
		xrS_entities::const_iterator	J = entities.find(*I);
		VERIFY3							(J != entities.end(),"SERVER : Cannot find children in the map",entity->name_replace());
		VERIFY3							((*J).second,"SERVER : Null entity object in the map",entity->name_replace());
		VERIFY3							((*J).first == (*J).second->ID,"SERVER : ID mismatch - map key doesn't correspond to the real entity ID",(*J).second->name_replace());
		VERIFY3							((*J).second->ID_Parent == entity->ID,"SERVER : Parent/Children relationship mismatch - Object has children, but children doesn't have parent",(*J).second->name_replace());
	}
}

#endif // DEBUG

shared_str xrServer::level_name				(const shared_str &server_options) const
{
	return	(game->level_name(server_options));
}
shared_str xrServer::level_version			(const shared_str &server_options) const
{
	return	(game_sv_GameState::parse_level_version(server_options));						
}

void xrServer::create_direct_client()
{
	SClientConnectData cl_data;
	cl_data.clientID.set(1);
	strcpy_s( cl_data.name, "single_player" );
	cl_data.process_id = GetCurrentProcessId();
	
	new_client( &cl_data );
}


void xrServer::ProceedDelayedPackets()
{
	DelayedPackestCS.Enter();
	while (!m_aDelayedPackets.empty())
	{
		DelayedPacket& DPacket	= *m_aDelayedPackets.begin();
		OnDelayedMessage(DPacket.Packet, DPacket.SenderID);
//		OnMessage(DPacket.Packet, DPacket.SenderID);
		m_aDelayedPackets.pop_front();
	}
	DelayedPackestCS.Leave();
};

void xrServer::AddDelayedPacket	(NET_Packet& Packet, ClientID Sender)
{
	DelayedPackestCS.Enter();

	m_aDelayedPackets.push_back(DelayedPacket());
	DelayedPacket* NewPacket = &(m_aDelayedPackets.back());
	NewPacket->SenderID = Sender;
	CopyMemory	(&(NewPacket->Packet),&Packet,sizeof(NET_Packet));	

	DelayedPackestCS.Leave();
}

u32 g_sv_dwMaxClientPing		= 2000;
u32 g_sv_time_for_ping_check	= 15000;// 15 sec
u8	g_sv_maxPingWarningsCount	= 5;

void xrServer::PerformCheckClientsForMaxPing()
{
	struct MaxPingClientDisconnector
	{
		xrServer* m_owner;
		MaxPingClientDisconnector(xrServer* owner) :
			m_owner(owner)
		{}
		void operator()(IClient* client)
		{
			xrClientData*		Client = static_cast<xrClientData*>(client);
			game_PlayerState*	ps = Client->ps;
			if (!ps)
				return;
			
			if (client == m_owner->GetServerClient())
				return;

			if(	ps->ping > g_sv_dwMaxClientPing && 
				Client->m_ping_warn.m_dwLastMaxPingWarningTime+g_sv_time_for_ping_check < Device.dwTimeGlobal )
			{
				++Client->m_ping_warn.m_maxPingWarnings;
				Client->m_ping_warn.m_dwLastMaxPingWarningTime	= Device.dwTimeGlobal;
				
				if(Client->m_ping_warn.m_maxPingWarnings >= g_sv_maxPingWarningsCount)
				{  //kick
					LPSTR	reason;
					STRCONCAT( reason, CStringTable().translate("st_kicked_by_server").c_str() );
					Level().Server->DisconnectClient( Client, reason );
				}
				else
				{ //send warning
					NET_Packet		P;	
					P.w_begin		(M_CLIENT_WARN);
					P.w_u8			(1); // 1 means max-ping-warning
					P.w_u16			(ps->ping);
					P.w_u8			(Client->m_ping_warn.m_maxPingWarnings);
					P.w_u8			(g_sv_maxPingWarningsCount);
					m_owner->SendTo	(Client->ID,P,net_flags(FALSE,TRUE));
				}
			}
		}
	};
	MaxPingClientDisconnector temp_functor(this);
	ForEachClientDoSender(temp_functor);
}

extern	BOOL	g_bCollectStatisticData;

//xr_token game_types[];
LPCSTR GameTypeToString(EGameIDs gt, bool bShort);

void xrServer::GetServerInfo( CServerInfo* si )
{
	string32  tmp;
	string256 tmp256;

	si->AddItem( "Server port", itoa( GetPort(), tmp, 10 ), RGB(128,128,255) );
	LPCSTR time = InventoryUtilities::GetTimeAsString( Device.dwTimeGlobal, InventoryUtilities::etpTimeToSecondsAndDay ).c_str();
	si->AddItem( "Uptime", time, RGB(255,228,0) );

//	strcpy_s( tmp256, get_token_name(game_types, game->Type() ) );
	strcpy_s( tmp256, GameTypeToString( game->Type(), true ) );
	si->AddItem( "Game type", tmp256, RGB(128,255,255) );

	if ( g_pGameLevel )
	{
		time = InventoryUtilities::GetGameTimeAsString( InventoryUtilities::etpTimeToMinutes ).c_str();

		si->AddItem( "Game time", tmp256, RGB(205,228,178) );
	}
}

void xrServer::AddCheater			(shared_str const & reason, ClientID const & cheaterID)
{
	CheaterToKick new_cheater;
	new_cheater.reason = reason;
	new_cheater.cheater_id = cheaterID;
	m_cheaters.push_back(new_cheater);
}

void xrServer::KickCheaters			()
{
	for (cheaters_t::iterator i = m_cheaters.begin(),
		ie = m_cheaters.end(); i != ie; ++i)
	{
		IClient* tmp_client = GetClientByID(i->cheater_id);
		if (!tmp_client)
		{
			Msg("! ERROR: KickCheaters: client [%u] not found", i->cheater_id);
			continue;
		}
		ClientID tmp_client_id = tmp_client->ID;
		DisconnectClient(tmp_client, i->reason.c_str());
		
		NET_Packet		P;
		P.w_begin		( M_GAMEMESSAGE ); 
		P.w_u32			( GAME_EVENT_SERVER_STRING_MESSAGE );
		P.w_stringZ		( i->reason.c_str() + 2 );
		Level().Server->SendBroadcast( tmp_client_id, P );
	}
	m_cheaters.clear();
}

void xrServer::MakeScreenshot(ClientID const & admin_id, ClientID const & cheater_id)
{

}

void xrServer::MakeConfigDump(ClientID const & admin_id, ClientID const & cheater_id)
{

}


void xrServer::initialize_screenshot_proxies()
{

}
void xrServer::deinitialize_screenshot_proxies()
{

}