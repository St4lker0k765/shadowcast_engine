#include "stdafx.h"
#include "Level.h"
#include "HUDManager.h"
#include "xrServer.h"

#define DEMO_DATA_SIZE	65536
u32	g_dwDemoDeltaFrame = 1;

void						CLevel::Demo_StoreServerData	(void* data, u32 size)
{
	return;
/* warning 4702 - unreachable code
	DemoCS.Enter();
	Demo_StoreData(data, size, DATA_SERVER_PACKET);
	DemoCS.Leave();
*/
}

void						CLevel::Demo_StoreData			(void* data, u32 size, DEMO_CHUNK DataType)
{
 	if (!IsDemoSave()) return;

//	DemoCS.Enter();
	
	u32 CurTime = timeServer_Async();
	u32 TotalSize = 4 + 4 + 4;//	
	switch(DataType)
	{
	case DATA_FRAME:
		{
			TotalSize += size;
		}break;
	case DATA_SERVER_PACKET:
	case DATA_CLIENT_PACKET:
		{
			TotalSize += size + 4;
		}break;
	}

	R_ASSERT2(size <= DEMO_DATA_SIZE, "Data is too BIG!");
	if ((TotalSize + m_dwStoredDemoDataSize) >= DEMO_DATA_SIZE)
	{
		Demo_DumpData();		
	};

	DEMO_CHUNK	Chunk = DataType;
	CopyMemory(m_pStoredDemoData + m_dwStoredDemoDataSize, &Chunk, 4);				m_dwStoredDemoDataSize += 4;					
	CopyMemory(m_pStoredDemoData + m_dwStoredDemoDataSize, &m_dwCurDemoFrame, 4);	m_dwStoredDemoDataSize += 4;		
	CopyMemory(m_pStoredDemoData + m_dwStoredDemoDataSize, &CurTime, 4);			m_dwStoredDemoDataSize += 4;				
	switch (DataType)
	{
	case DATA_FRAME:
		{
			CopyMemory(m_pStoredDemoData + m_dwStoredDemoDataSize, data, size);		m_dwStoredDemoDataSize += size;		
		}break;
	case DATA_SERVER_PACKET:
	case DATA_CLIENT_PACKET:
		{
			CopyMemory(m_pStoredDemoData + m_dwStoredDemoDataSize, &size, 4);		m_dwStoredDemoDataSize += 4;
			CopyMemory(m_pStoredDemoData + m_dwStoredDemoDataSize, data, size);		m_dwStoredDemoDataSize += size;		
		}break;
	}
	
	
//	DemoCS.Leave();

	/*
	FILE* fTDemo = fopen(m_sDemoName, "ab");
	if (!fTDemo) return;

	static u32 Count = 0;
	static u32 TotalSize = 0;
	u32 CurTime = timeServer_Async();
	fwrite(&(CurTime), sizeof(CurTime), 1, fTDemo); TotalSize += sizeof(CurTime);
	fwrite(&(size), sizeof(size), 1, fTDemo);		TotalSize += sizeof(size);
	if (size) fwrite((data), 1, size, fTDemo);		TotalSize += size;
	fclose(fTDemo);
	Count++;
	*/
}

void						CLevel::Demo_DumpData()
{
	if (!m_sDemoName[0]) return;
	FILE* fTDemo = fopen(m_sDemoName, "ab");
	if (fTDemo)
	{
		fwrite(m_pStoredDemoData, m_dwStoredDemoDataSize, 1, fTDemo);
		fclose(fTDemo);
	}

	m_dwStoredDemoDataSize = 0;
}

void						CLevel_DemoCrash_Handler	()
{
	if (!g_pGameLevel) return;
	Level().WriteStoredDemo();
	Level().CallOldCrashHandler();
}

//#define STORE_TDEMO

void						CLevel::Demo_PrepareToStore			()
{
	m_bDemoSaveMode = !!strstr(Core.Params,"-techdemo");

	if (!m_bDemoSaveMode) return;

	VERIFY						(!m_we_used_old_crach_handler);
	m_we_used_old_crach_handler	= true;
	m_pOldCrashHandler			= Debug.get_crashhandler();
	Debug.set_crashhandler		(CLevel_DemoCrash_Handler);
	//---------------------------------------------------------------
	string1024 CName = "";
	u32 CNameSize = 1024;
	GetComputerName(CName, (DWORD*)&CNameSize);
	SYSTEMTIME Time;
	GetLocalTime(&Time);
	sprintf_s(m_sDemoName, "xray_%s_%02d-%02d-%02d_%02d-%02d-%02d.tdemo", CName, Time.wMonth, Time.wDay, Time.wYear, Time.wHour, Time.wMinute, Time.wSecond);
	Msg("Tech Demo would be stored in - %s", m_sDemoName);
	
	FS.update_path      (m_sDemoName,"$logs$",m_sDemoName);
	//---------------------------------------------------------------
	m_dwStoredDemoDataSize = 0;
	m_pStoredDemoData = xr_alloc<u8>(DEMO_DATA_SIZE/sizeof(u8));
	//---------------------------------------------------------------
	m_dwCurDemoFrame = 0;
//	ZeroMemory(&m_sDemoHeader, sizeof(m_sDemoHeader));
	m_sDemoHeader.Head[0] = 0;
	m_sDemoHeader.ServerOptions = "";
};

void						CLevel::CallOldCrashHandler			()
{
	if (!m_pOldCrashHandler) return;
	m_pOldCrashHandler();
};

void						CLevel::WriteStoredDemo			()
{	
	if (!DemoCS.TryEnter()) return;
	
	Demo_DumpData();
	DemoCS.Leave();
};

BOOL	g_bLeaveTDemo = FALSE;

void						CLevel::Demo_Clear				()
{
	WriteStoredDemo();
	m_bDemoSaveMode = FALSE;
	xr_free(m_pStoredDemoData);
	m_dwStoredDemoDataSize = 0;	 

	if (!g_bLeaveTDemo)
	{
		DeleteFile(m_sDemoName);
	};
};


static DWORD dFrame = 1;

void						CLevel::Demo_StartFrame			()
{
	if (!IsDemoSave() || !net_IsSyncronised()) return;

	DemoCS.Enter();

	DemoFrameTime CurFrameTime;
	CurFrameTime.dwTimeDelta = Device.dwTimeDelta;
	CurFrameTime.dwTimeGlobal = Device.dwTimeGlobal;
	CurFrameTime.dwTimeServer = Level().timeServer();
	CurFrameTime.dwTimeServer_Delta = Level().timeServer_Delta();
	CurFrameTime.fTimeDelta = Device.fTimeDelta;
	CurFrameTime.fTimeGlobal= Device.fTimeGlobal;

	Demo_StoreData(&CurFrameTime, sizeof(CurFrameTime), DATA_FRAME);

	DemoCS.Leave();
};

void						CLevel::Demo_EndFrame			()
{
	if (IsDemoPlay() && m_bDemoPlayByFrame)
		m_dwCurDemoFrame+=dFrame;		
	else
		m_dwCurDemoFrame++;	

	dFrame = g_dwDemoDeltaFrame;
};

