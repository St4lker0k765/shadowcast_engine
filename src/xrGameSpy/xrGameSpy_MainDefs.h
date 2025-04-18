#pragma once

#define QRCDKEY_INTEGRATION
#define SB_ICMP_SUPPORT

#undef DEMO_BUILD

	#define	GAMESPY_GAMENAME				"stalkercoppc"
	#define GAMESPY_GAMEID					2760
	#define GAMESPY_PRODUCTID				11994
	// not using this anymore
	#define GAME_VERSION					"1.0.0.0"
	#define REGISTRY_PATH					"Software\\GSC Game World\\STALKER-COP\\"

#define GAMESPY_GP_NAMESPACE_ID				1 /*gamespy global namespace*/

#define	GAMESPY_QR2_BASEPORT				5445
#define GAMESPY_BROWSER_MAX_UPDATES			20

#define START_PORT							0
#define END_PORT							65535
#define START_PORT_LAN						GAMESPY_QR2_BASEPORT
#define START_PORT_LAN_SV					START_PORT_LAN + 1
#define START_PORT_LAN_CL					START_PORT_LAN + 2
#define END_PORT_LAN						START_PORT_LAN + 250//GameSpy only process 500 ports

#define REGISTRY_BASE						HKEY_LOCAL_MACHINE
#define REGISTRY_VALUE_GSCDKEY				"InstallCDKEY"
#define REGISTRY_VALUE_VERSION				"InstallVers"
#define REGISTRY_VALUE_USERNAME				"InstallUserName"
#define REGISTRY_VALUE_SKU					"InstallSource"
#define REGISTRY_VALUE_INSTALL_PATCH_ID		"InstallPatchID"
#define REGISTRY_VALUE_LANGUAGE				"InstallLang"
#define REGISTRY_VALUE_USEREMAIL			"GPUserEmail"
#define REGISTRY_VALUE_USERPASSWORD			"GPUserPassword"
#define REGISTRY_VALUE_REMEMBER_PROFILE		"GPRememberMe"

#define GAMESPY_PATCHING_VERSIONUNIQUE_ID	"test_version_1"
#define GAMESPY_PATCHING_DISTRIBUTION_ID	0

 

#ifndef XRGAMESPY_API
	#ifdef XRGAMESPY_EXPORTS
		#define XRGAMESPY_API				__declspec(dllexport)
	#endif
#endif

#define EXPORT_FN_DECL(r, f, p)		XRGAMESPY_API r xrGS_##f p;

extern void FillSecretKey (char* SecretKey);
