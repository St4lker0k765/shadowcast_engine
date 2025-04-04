// Weapon.cpp: implementation of the CWeapon class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "Weapon.h"
#include "ParticlesObject.h"
#include "HUDManager.h"
#include "entity_alive.h"
#include "player_hud.h"
#include "ui/UIXmlInit.h"

#include "inventory.h"
#include "xrserver_objects_alife_items.h"

#include "actor.h"
#include "level.h"

#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "..\include\xrRender\Kinematics.h"
#include "ai_object_location.h"
#include "CameraEffector.h"
#include "clsid_game.h"
#include "gamepersistent.h"
#include "object_broker.h"
#include "../xrEngine/igame_persistent.h"
#include "EffectorFall.h"
#include "WeaponMagazinedWGrenade.h"
#include "Inventory.h"

#define WEAPON_REMOVE_TIME		60000
#define ROTATION_TIME			0.25f

ENGINE_API extern float psHUD_FOV_def;

extern CUIXml* pWpnScopeXml = NULL;
bool SOCScopesXmlEnable = READ_IF_EXISTS(pSCSettings, r_bool, "game_specific", "soc_enable_scopes_xml", false);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWeapon::CWeapon(LPCSTR name)
{
	SetState				(eHidden);
	SetNextState			(eHidden);
	m_sub_state				= eSubstateReloadBegin;
	m_bTriStateReload		= false;
	m_idle_state = eIdle;
	SetDefaults				();

	m_Offset.identity		();
	m_StrapOffset.identity	();

	iAmmoCurrent			= -1;
	m_dwAmmoCurrentCalcFrame= 0;

	iAmmoElapsed			= -1;
	iMagazineSize			= -1;
	m_ammoType				= 0;
	m_ammoName				= nullptr;

	eHandDependence			= hdNone;

	m_fZoomFactor			= g_fov;
	m_fZoomRotationFactor	= 0.f;
	m_cur_scope				= NULL;


	m_pAmmo					= nullptr;


	m_pFlameParticles2		= nullptr;
	m_sFlameParticles2		= nullptr;


	m_fCurrentCartirdgeDisp = 1.f;

	m_strap_bone0		    = nullptr;
	m_strap_bone1			= nullptr;
	m_StrapOffset.identity	();
	m_strapped_mode			= false;
	m_can_be_strapped		= false;
	m_ef_main_weapon_type	= static_cast<u32>(-1);
	m_ef_weapon_type		= static_cast<u32>(-1);
	m_UIScope				= nullptr;
	m_UIScopeNew			= nullptr;
	m_set_next_ammoType_on_reload = static_cast<u32>(-1);
	m_crosshair_inertion	= 0.f;

	m_fLR_MovingFactor = 0.f;
	m_fLR_CameraFactor = 0.f;
	m_fLR_InertiaFactor = 0.f;
	m_fUD_InertiaFactor = 0.f;
	m_zoom_params.m_f3dZoomFactor = 0.0f;
	m_zoom_params.m_fSecondVPFovFactor = 0.0f;

	bUseAltScope = false;
	bScopeIsHasTexture = false;
	m_nearwall_last_hud_fov = psHUD_FOV_def;
}

CWeapon::~CWeapon		()
{
	if (SOCScopesXmlEnable)
		xr_delete	(m_UIScopeNew);
	else
		xr_delete	(m_UIScope);
	delete_data(m_scopes);
}

void CWeapon::Hit(SHit* pHDS)
{
	inherited::Hit(pHDS);
}

const shared_str CWeapon::GetScopeName() const
{
	if (bUseAltScope)
	{
		return m_scopes[m_cur_scope];
	}
	else
	{
		return pSettings->r_string(m_scopes[m_cur_scope], "scope_name");
	}
}

void CWeapon::UpdateAltScope()
{
	if (m_eScopeStatus != ALife::eAddonAttachable || !bUseAltScope)
		return;

	shared_str sectionNeedLoad;

	sectionNeedLoad = IsScopeAttached() ? GetNameWithAttachment() : m_section_id;

	if (!pSettings->section_exist(sectionNeedLoad))
		return;

	shared_str vis = pSettings->r_string(sectionNeedLoad, "visual");

	if (vis != cNameVisual())
	{
		cNameVisual_set(vis);
	}

	shared_str new_hud = pSettings->r_string(sectionNeedLoad, "hud");
	if (new_hud != hud_sect)
	{
		hud_sect = new_hud;
	}
}

shared_str CWeapon::GetNameWithAttachment()
{
	string64 str;
	if (pSettings->line_exist(m_section_id.c_str(), "parent_section"))
	{
		shared_str parent = pSettings->r_string(m_section_id.c_str(), "parent_section");
		xr_sprintf(str, "%s_%s", parent.c_str(), GetScopeName().c_str());
	}
	else
	{
		xr_sprintf(str, "%s_%s", m_section_id.c_str(), GetScopeName().c_str());
	}
	return (shared_str)str;
}

int CWeapon::GetScopeX()
{
	if (bUseAltScope)
	{
		if (m_eScopeStatus != ALife::eAddonPermanent && IsScopeAttached())
		{
			return pSettings->r_s32(GetNameWithAttachment(), "scope_x");
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return pSettings->r_s32(m_scopes[m_cur_scope], "scope_x");
	}
}

int CWeapon::GetScopeY()
{
	if (bUseAltScope)
	{
		if (m_eScopeStatus != ALife::eAddonPermanent && IsScopeAttached())
		{
			return pSettings->r_s32(GetNameWithAttachment(), "scope_y");
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return pSettings->r_s32(m_scopes[m_cur_scope], "scope_y");
	}
}


void CWeapon::UpdateXForm	()
{
	if (Device.dwFrame!=dwXF_Frame)
	{
		dwXF_Frame = Device.dwFrame;

		if (!H_Parent())	
			return;

		// Get access to entity and its visual
		auto*	E		= smart_cast<CEntityAlive*>(H_Parent());
		
		if(!E) 
		{
			if (!IsGameTypeSingle())
				UpdatePosition(H_Parent()->XFORM());
			return;
		}

		const auto* parent = smart_cast<const CInventoryOwner*>(E);
		if (!parent || parent && parent->use_simplified_visual())
			return;

		if (parent->attached(this))
			return;

		R_ASSERT		(E);
		auto*	V		= smart_cast<IKinematics*>	(E->Visual());
		VERIFY			(V);

		// Get matrices
		int				boneL,boneR,boneR2;
		E->g_WeaponBones(boneL,boneR,boneR2);
		if ((HandDependence() == hd1Hand) || (GetState() == eReload) || (!E->g_Alive()))
			boneL = boneR2;
		V->CalculateBones	();
		Fmatrix& mL			= V->LL_GetTransform(u16(boneL));
		Fmatrix& mR			= V->LL_GetTransform(u16(boneR));
		// Calculate
		Fmatrix				mRes;
		Fvector				R,D,N;
		D.sub				(mL.c,mR.c);	

		if(fis_zero(D.magnitude()))
		{
			mRes.set(E->XFORM());
			mRes.c.set(mR.c);
		}
		else
		{		
			D.normalize();
			R.crossproduct	(mR.j,D);

			N.crossproduct	(D,R);			
			N.normalize();

			mRes.set		(R,N,D,mR.c);
			mRes.mulA_43	(E->XFORM());
		}

		UpdatePosition	(mRes);
	}
}

void CWeapon::UpdateFireDependencies_internal()
{
	if (Device.dwFrame != dwFP_Frame)
	{
		dwFP_Frame = Device.dwFrame;

		UpdateXForm();

		if (GetHUDmode())
		{
			HudItemData()->setup_firedeps(m_current_firedeps);
			VERIFY(_valid(m_current_firedeps.m_FireParticlesXForm));
		}
		else
		{
			// 3rd person or no parent
			Fmatrix& parent = XFORM();
			Fvector& fp = vLoadedFirePoint;
			Fvector& fp2 = vLoadedFirePoint2;
			Fvector& sp = vLoadedShellPoint;

			parent.transform_tiny(m_current_firedeps.vLastFP, fp);
			parent.transform_tiny(m_current_firedeps.vLastFP2, fp2);
			parent.transform_tiny(m_current_firedeps.vLastSP, sp);

			m_current_firedeps.vLastFD.set(0.f, 0.f, 1.f);
			parent.transform_dir(m_current_firedeps.vLastFD);

			m_current_firedeps.m_FireParticlesXForm.set(parent);
			VERIFY(_valid(m_current_firedeps.m_FireParticlesXForm));
		}
	}
}
void CWeapon::ForceUpdateFireParticles()
{
	if (!GetHUDmode())
	{ // update particlesXFORM real bullet direction

		if (!H_Parent())
			return;

		Fvector p, d;
		smart_cast<CEntity*>(H_Parent())->g_fireParams(this, p, d);

		Fmatrix _pxf;
		_pxf.k = d;
		_pxf.i.crossproduct(Fvector().set(0.0f, 1.0f, 0.0f), _pxf.k);
		_pxf.j.crossproduct(_pxf.k, _pxf.i);
		_pxf.c = XFORM().c;

		m_current_firedeps.m_FireParticlesXForm.set(_pxf);
	}
}

constexpr char* wpn_scope_def_bone = "wpn_scope";
constexpr char* wpn_silencer_def_bone = "wpn_silencer";
constexpr char* wpn_launcher_def_bone_shoc = "wpn_launcher";
constexpr char* wpn_launcher_def_bone_cop = "wpn_grenade_launcher";

void CWeapon::Load		(LPCSTR section)
{
	inherited::Load					(section);
	CShootingObject::Load			(section);

	
	if(pSettings->line_exist(section, "flame_particles_2"))
		m_sFlameParticles2 = pSettings->r_string(section, "flame_particles_2");
	
	if (!m_bForcedParticlesHudMode)
		m_bParticlesHudMode = !!pSettings->line_exist(hud_sect, "item_visual");

#ifdef DEBUG
	{
		Fvector				pos,ypr;
		pos					= pSettings->r_fvector3		(section,"position");
		ypr					= pSettings->r_fvector3		(section,"orientation");
		ypr.mul				(PI/180.f);

		m_Offset.setHPB			(ypr.x,ypr.y,ypr.z);
		m_Offset.translate_over	(pos);
	}

	m_StrapOffset			= m_Offset;
	if (pSettings->line_exist(section,"strap_position") && pSettings->line_exist(section,"strap_orientation")) {
		Fvector				pos,ypr;
		pos					= pSettings->r_fvector3		(section,"strap_position");
		ypr					= pSettings->r_fvector3		(section,"strap_orientation");
		ypr.mul				(PI/180.f);

		m_StrapOffset.setHPB			(ypr.x,ypr.y,ypr.z);
		m_StrapOffset.translate_over	(pos);
	}
#endif

	// load ammo classes
	m_ammoTypes.clear	(); 
	LPCSTR				S = pSettings->r_string(section,"ammo_class");
	if (S && S[0]) 
	{
		int				count		= _GetItemCount	(S);
		for (int it=0; it<count; ++it)
		{
			string128		_ammoItem;
			_GetItem				(S,it,_ammoItem);
			m_ammoTypes.push_back	(_ammoItem);
		}
		m_ammoName = pSettings->r_string(*m_ammoTypes[0],"inv_name_short");
	}
	else
		m_ammoName = nullptr;

	iAmmoElapsed		= pSettings->r_s32		(section,"ammo_elapsed"		);
	iMagazineSize		= pSettings->r_s32		(section,"ammo_mag_size"	);
	
	////////////////////////////////////////////////////
	// дисперсия стрельбы

	//подбрасывание камеры во время отдачи
	camMaxAngle			= pSettings->r_float		(section,"cam_max_angle"	); 
	camMaxAngle			= deg2rad					(camMaxAngle);
	camRelaxSpeed		= pSettings->r_float		(section,"cam_relax_speed"	); 
	camRelaxSpeed		= deg2rad					(camRelaxSpeed);
	if (pSettings->line_exist(section, "cam_relax_speed_ai"))
	{
		camRelaxSpeed_AI		= pSettings->r_float		(section,"cam_relax_speed_ai"	); 
		camRelaxSpeed_AI		= deg2rad					(camRelaxSpeed_AI);
	}
	else
	{
		camRelaxSpeed_AI	= camRelaxSpeed;
	}
	
//	camDispersion		= pSettings->r_float		(section,"cam_dispersion"	); 
//	camDispersion		= deg2rad					(camDispersion);

	camMaxAngleHorz		= pSettings->r_float		(section,"cam_max_angle_horz"	); 
	camMaxAngleHorz		= deg2rad					(camMaxAngleHorz);
	camStepAngleHorz	= pSettings->r_float		(section,"cam_step_angle_horz"	); 
	camStepAngleHorz	= deg2rad					(camStepAngleHorz);	
	camDispertionFrac			= READ_IF_EXISTS(pSettings, r_float, section, "cam_dispertion_frac",	0.7f);
	//  [8/2/2005]
	//m_fParentDispersionModifier = READ_IF_EXISTS(pSettings, r_float, section, "parent_dispersion_modifier",1.0f);
	m_fPDM_disp_base			= READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_base",	1.0f);
	m_fPDM_disp_vel_factor		= READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_vel_factor",	1.0f);
	m_fPDM_disp_accel_factor	= READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_accel_factor",	1.0f);
	m_fPDM_disp_crouch			= READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_crouch",	1.0f);
	m_fPDM_disp_crouch_no_acc	= READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_crouch_no_acc",	1.0f);
	//  [8/2/2005]
	m_crosshair_inertion = READ_IF_EXISTS(pSettings, r_float, section, "crosshair_inertion", 5.91f);

	fireDispersionConditionFactor = pSettings->r_float(section,"fire_dispersion_condition_factor"); 
	misfireProbability			  = pSettings->r_float(section,"misfire_probability"); 
	misfireConditionK			  = READ_IF_EXISTS(pSettings, r_float, section, "misfire_condition_k",	1.0f);
	conditionDecreasePerShot	  = pSettings->r_float(section,"condition_shot_dec"); 

	m_bUseAimSilShotAnim = READ_IF_EXISTS(pSettings, r_bool, section, "enable_aim_silencer_shoot_anm", false);

	vLoadedFirePoint	= pSettings->r_fvector3		(section,"fire_point"		);
	
	if(pSettings->line_exist(section,"fire_point2")) 
		vLoadedFirePoint2= pSettings->r_fvector3	(section,"fire_point2");
	else 
		vLoadedFirePoint2= vLoadedFirePoint;

	// hands
	eHandDependence		= EHandDependence(pSettings->r_s32(section,"hand_dependence"));
	m_bIsSingleHanded	= true;
	if (pSettings->line_exist(section, "single_handed"))
		m_bIsSingleHanded	= !!pSettings->r_bool(section, "single_handed");
	// 
	m_fMinRadius		= pSettings->r_float		(section,"min_radius");
	m_fMaxRadius		= pSettings->r_float		(section,"max_radius");


	// информация о возможных апгрейдах и их визуализации в инвентаре
	m_eScopeStatus			 = static_cast<ALife::EWeaponAddonStatus>(pSettings->r_s32(section,"scope_status"));
	m_eSilencerStatus		 = static_cast<ALife::EWeaponAddonStatus>(pSettings->r_s32(section,"silencer_status"));
	m_eGrenadeLauncherStatus = static_cast<ALife::EWeaponAddonStatus>(pSettings->r_s32(section,"grenade_launcher_status"));

	m_bZoomEnabled = !!pSettings->r_bool(section,"zoom_enabled");
	m_bUseScopeZoom = !!READ_IF_EXISTS(pSettings, r_bool, section, "use_scope_zoom", false);
	m_bUseScopeGrenadeZoom = !!READ_IF_EXISTS(pSettings, r_bool, section, "use_scope_grenade_zoom", false);

	m_fZoomRotateTime = READ_IF_EXISTS(pSettings, r_float, hud_sect, "zoom_rotate_time", ROTATION_TIME);

	m_fZoomFactor = CurrentZoomFactor();
	// Модификатор для HUD FOV от бедра
	m_hud_fov_add_mod = READ_IF_EXISTS(pSettings, r_float, section, "hud_fov_addition_modifier", 0.f);

	// Параметры изменения HUD FOV, когда игрок стоит вплотную к стене
	m_nearwall_dist_min = READ_IF_EXISTS(pSettings, r_float, section, "nearwall_dist_min", 0.5f);
	m_nearwall_dist_max = READ_IF_EXISTS(pSettings, r_float, section, "nearwall_dist_max", 1.f);
	m_nearwall_target_hud_fov = READ_IF_EXISTS(pSettings, r_float, section, "nearwall_target_hud_fov", 0.27f);
	m_nearwall_speed_mod = READ_IF_EXISTS(pSettings, r_float, section, "nearwall_speed_mod", 10.f);

	bUseAltScope = !!bLoadAltScopesParams(section);

	if (!bUseAltScope)
		LoadOriginalScopesParams(section);

	if(m_eSilencerStatus == ALife::eAddonAttachable)
	{
		m_sSilencerName = pSettings->r_string(section,"silencer_name");
		if (UseHDIcons) {
			m_iSilencerX = pSettings->r_s32(section, "silencer_x")*2;
			m_iSilencerY = pSettings->r_s32(section, "silencer_y")*2;
		}
		else {
			m_iSilencerX = pSettings->r_s32(section, "silencer_x");
			m_iSilencerY = pSettings->r_s32(section, "silencer_y");
		}
	}

    
	if(m_eGrenadeLauncherStatus == ALife::eAddonAttachable)
	{
		m_sGrenadeLauncherName = pSettings->r_string(section,"grenade_launcher_name");
		if (UseHDIcons) {
			m_iGrenadeLauncherX = pSettings->r_s32(section, "grenade_launcher_x")*2;
			m_iGrenadeLauncherY = pSettings->r_s32(section, "grenade_launcher_y")*2;
		}
		else {
			m_iGrenadeLauncherX = pSettings->r_s32(section, "grenade_launcher_x");
			m_iGrenadeLauncherY = pSettings->r_s32(section, "grenade_launcher_y");
		}
	}
	
	// Кости мировой модели оружия
	m_sWpn_scope_bone    = READ_IF_EXISTS(pSettings, r_string, section, "scope_bone", wpn_scope_def_bone);
	m_sWpn_silencer_bone = READ_IF_EXISTS(pSettings, r_string, section, "silencer_bone", wpn_silencer_def_bone);
	m_sWpn_launcher_bone = READ_IF_EXISTS(pSettings, r_string, section, "launcher_bone", wpn_launcher_def_bone_shoc);

	if (pSettings->line_exist(section, "hidden_bones"))
	{
		const char* S = pSettings->r_string(section, "hidden_bones");
		if (S && strlen(S))
		{
			const int count = _GetItemCount(S);
			string128 _hidden_bone{};
			for (int it = 0; it < count; ++it)
			{
				_GetItem(S, it, _hidden_bone);
				hidden_bones.push_back(_hidden_bone);
			}
		}
	}

	// Кости худовой модели оружия - если не прописаны, используются имена из конфига мировой модели.
	m_sHud_wpn_scope_bone    = READ_IF_EXISTS(pSettings, r_string, hud_sect, "scope_bone", m_sWpn_scope_bone);
	m_sHud_wpn_silencer_bone = READ_IF_EXISTS(pSettings, r_string, hud_sect, "silencer_bone", m_sWpn_silencer_bone);
	m_sHud_wpn_launcher_bone = READ_IF_EXISTS(pSettings, r_string, hud_sect, "launcher_bone", m_sWpn_launcher_bone);

	if (pSettings->line_exist(hud_sect, "hidden_bones"))
	{
		const char* S = pSettings->r_string(hud_sect, "hidden_bones");
		if (S && strlen(S))
		{
			const int count = _GetItemCount(S);
			string128 _hidden_bone{};
			for (int it = 0; it < count; ++it)
			{
				_GetItem(S, it, _hidden_bone);
				hud_hidden_bones.push_back(_hidden_bone);
			}
		}
	}
	else
		hud_hidden_bones = hidden_bones;


	UpdateAltScope();
	InitAddons();

	//////////////////////////////////////
	//время убирания оружия с уровня
	if(pSettings->line_exist(section,"weapon_remove_time"))
		m_dwWeaponRemoveTime = pSettings->r_u32(section,"weapon_remove_time");
	else
		m_dwWeaponRemoveTime = WEAPON_REMOVE_TIME;
	//////////////////////////////////////
	if(pSettings->line_exist(section,"auto_spawn_ammo"))
		m_bAutoSpawnAmmo = pSettings->r_bool(section,"auto_spawn_ammo");
	else
		m_bAutoSpawnAmmo = TRUE;
	//////////////////////////////////////


	m_bHideCrosshairInZoom = true;
	if(pSettings->line_exist(hud_sect, "zoom_hide_crosshair"))
		m_bHideCrosshairInZoom = !!pSettings->r_bool(hud_sect, "zoom_hide_crosshair");	

	Fvector			def_dof;
	def_dof.set(-1, -1, -1);
//	m_zoom_params.m_ZoomDof		= READ_IF_EXISTS(pSettings, r_fvector3, section, "zoom_dof", Fvector().set(-1,-1,-1));
//	m_zoom_params.m_bZoomDofEnabled	= !def_dof.similar(m_zoom_params.m_ZoomDof);

//	m_zoom_params.m_ReloadDof	= READ_IF_EXISTS(pSettings, r_fvector4, section, "reload_dof", Fvector4().set(-1,-1,-1,-1));

//	m_zoom_params.m_ReloadEmptyDof = READ_IF_EXISTS(pSettings, r_fvector4, section, "reload_empty_dof", Fvector4().set(-1, -1, -1, -1));

	m_bHasTracers = READ_IF_EXISTS(pSettings, r_bool, section, "tracers", true);
	m_u8TracerColorID = READ_IF_EXISTS(pSettings, r_u8, section, "tracers_color_ID", static_cast<u8>(-1));

	string256						temp;
	for (int i=egdNovice; i<egdCount; ++i) {
		strconcat					(sizeof(temp),temp,"hit_probability_",get_token_name(difficulty_type_token,i));
		m_hit_probability[i]		= READ_IF_EXISTS(pSettings,r_float,section,temp,1.f);
	}
}

void CWeapon::LoadFireParams		(LPCSTR section, LPCSTR prefix)
{
	camDispersion		= pSettings->r_float		(section,"cam_dispersion"	); 
	camDispersion		= deg2rad					(camDispersion);

	if (pSettings->line_exist(section,"cam_dispersion_inc"))
	{
		camDispersionInc		= pSettings->r_float		(section,"cam_dispersion_inc"	); 
		camDispersionInc		= deg2rad					(camDispersionInc);
	}
	else
		camDispersionInc = 0;

	CShootingObject::LoadFireParams(section, prefix);
};

BOOL CWeapon::bLoadAltScopesParams(LPCSTR section)
{
	if (!pSettings->line_exist(section, "scopes"))
		return FALSE;

	if (!xr_strcmp(pSettings->r_string(section, "scopes"), "none"))
		return FALSE;

	if (m_eScopeStatus == ALife::eAddonAttachable)
	{
		LPCSTR str = pSettings->r_string(section, "scopes");
		for (int i = 0, count = _GetItemCount(str); i < count; ++i)
		{
			string128 scope_section;
			_GetItem(str, i, scope_section);
			m_scopes.push_back(scope_section);
		}
	}
	else if (m_eScopeStatus == ALife::eAddonPermanent)
	{
		LoadCurrentScopeParams(section);
	}

	return TRUE;
}

void CWeapon::LoadOriginalScopesParams(LPCSTR section)
{
	if (m_eScopeStatus == ALife::eAddonAttachable)
	{
		if (pSettings->line_exist(section, "scopes_sect"))
		{
			LPCSTR str = pSettings->r_string(section, "scopes_sect");
			for (int i = 0, count = _GetItemCount(str); i < count; ++i)
			{
				string128						scope_section;
				_GetItem(str, i, scope_section);
				m_scopes.push_back(scope_section);
			}
		}
		else
		{
			m_scopes.push_back(section);
		}
	}
	else if (m_eScopeStatus == ALife::eAddonPermanent)
	{
		LoadCurrentScopeParams(section);
	}
}

void createWpnScopeXML()
{
	if (!pWpnScopeXml)
	{
		pWpnScopeXml = xr_new<CUIXml>();
		pWpnScopeXml->Load(CONFIG_PATH, UI_PATH, "scopes.xml");
	}
}

void CWeapon::LoadCurrentScopeParams(LPCSTR section)
{
	shared_str scope_tex_name = "none";
	bScopeIsHasTexture = false;
	if (pSettings->line_exist(section, "scope_texture"))
	{
		scope_tex_name = pSettings->r_string(section, "scope_texture");
		if (xr_strcmp(scope_tex_name, "none") != 0)
			bScopeIsHasTexture = true;
	}

	Load3DScopeParams(section);

	m_fScopeZoomFactor = pSettings->r_float(section, "scope_zoom_factor");

	if (bScopeIsHasTexture || bIsSecondVPZoomPresent())
	{
	//	if (bIsSecondVPZoomPresent())
	//		bNVsecondVPavaible = !!pSettings->line_exist(section, "scope_nightvision");
	//	else m_zoom_params.m_sUseZoomPostprocess = READ_IF_EXISTS(pSettings, r_string, section, "scope_nightvision", 0);

	//	m_zoom_params.m_bUseDynamicZoom = READ_IF_EXISTS(pSettings, r_bool, section, "scope_dynamic_zoom", FALSE);

	/*	if (m_zoom_params.m_bUseDynamicZoom)
		{
			m_fZoomStepCount = READ_IF_EXISTS(pSettings, r_u8, section, "scope_zoom_steps", 3.0f);
			m_fZoomMinKoeff = READ_IF_EXISTS(pSettings, r_u8, section, "min_zoom_k", 0.3f);
		}

		m_zoom_params.m_sUseBinocularVision = READ_IF_EXISTS(pSettings, r_string, section, "scope_alive_detector", 0);*/
	}
	else
	{
//		bNVsecondVPavaible = false;
//		bNVsecondVPstatus = false;
	}

//	m_fScopeInertionFactor = READ_IF_EXISTS(pSettings, r_float, section, "scope_inertion_factor", m_fControlInertionFactor);

//	m_fRTZoomFactor = m_zoom_params.m_fScopeZoomFactor;

	if (m_UIScope)
	{
		xr_delete(m_UIScope);
	}

	if (m_UIScopeNew)
	{
		xr_delete(m_UIScopeNew);
	}

	if (bScopeIsHasTexture)
	{
		if (SOCScopesXmlEnable)
		{
			m_UIScopeNew = xr_new<CUIWindow>();
			createWpnScopeXML();
			CUIXmlInit::InitWindow(*pWpnScopeXml, scope_tex_name.c_str(), 0, m_UIScopeNew);
		}
		else 
		{
			m_UIScope = xr_new<CUIStaticItem>();

			m_UIScope->Init(*scope_tex_name, "hud\\default", 0, 0, alNone);

		}
	}
}

void CWeapon::Load3DScopeParams(LPCSTR section)
{
	m_zoom_params.m_fSecondVPFovFactor = READ_IF_EXISTS(pSettings, r_float, section, "3d_fov", 0.0f);
	m_zoom_params.m_f3dZoomFactor = READ_IF_EXISTS(pSettings, r_float, section, "3d_zoom_factor", 100.0f);
}

BOOL CWeapon::net_Spawn		(CSE_Abstract* DC)
{
	BOOL bResult					= inherited::net_Spawn(DC);
	auto* e	= (CSE_Abstract*)(DC);
	auto* E	= smart_cast<CSE_ALifeItemWeapon*>(e);

	//iAmmoCurrent					= E->a_current;
	iAmmoElapsed					= E->a_elapsed;
	m_flagsAddOnState				= E->m_addon_flags.get();
	m_ammoType						= E->ammo_type;
	if (E->cur_scope < m_scopes.size() && m_scopes.size()>1)
		m_cur_scope = E->cur_scope;
	SetState						(E->wpn_state);
	SetNextState					(E->wpn_state);
	
	m_DefaultCartridge.Load(*m_ammoTypes[m_ammoType], static_cast<u8>(m_ammoType));	
	if(iAmmoElapsed) 
	{
		m_fCurrentCartirdgeDisp = m_DefaultCartridge.m_kDisp;
		for(int i = 0; i < iAmmoElapsed; ++i) 
			m_magazine.push_back(m_DefaultCartridge);
	}


	UpdateAltScope();
	UpdateAddonsVisibility();
	InitAddons();


	m_dwWeaponIndependencyTime = 0;

	VERIFY((u32)iAmmoElapsed == m_magazine.size());
	m_bAmmoWasSpawned		= false;

	return bResult;
}

void CWeapon::net_Destroy	()
{
	inherited::net_Destroy	();

	//удалить объекты партиклов
	StopFlameParticles	();
	StopFlameParticles2	();
	StopLight			();
	Light_Destroy		();

	m_magazine.clear();
}

BOOL CWeapon::IsUpdating()
{	
	bool bIsActiveItem = m_pCurrentInventory && m_pCurrentInventory->ActiveItem()==this;
	return bIsActiveItem || bWorking || IsPending() || getVisible();
}

void CWeapon::net_Export(NET_Packet& P)
{
	inherited::net_Export	(P);

	P.w_float_q8			(m_fCondition,0.0f,1.0f);

	u8 need_upd				= IsUpdating() ? 1 : 0;
	P.w_u8					(need_upd);
	P.w_u16					(static_cast<u16>(iAmmoElapsed));
	P.w_u8					(m_flagsAddOnState);
	P.w_u8					(static_cast<u8>(m_ammoType));
	P.w_u8					(static_cast<u8>(GetState()));
	P.w_u8					(m_bZoomMode);
	P.w_u8                  ((u8)m_cur_scope);
}

void CWeapon::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);

	P.r_float_q8(m_fCondition,0.0f,1.0f);

	u8 flags = 0;
	P.r_u8(flags);

	u16 ammo_elapsed = 0;
	P.r_u16(ammo_elapsed);

	u8 NewAddonState;
	P.r_u8(NewAddonState);

	m_flagsAddOnState = NewAddonState;
	UpdateAddonsVisibility();

	u8 ammoType, wstate;
	P.r_u8(ammoType);
	P.r_u8(wstate);

	u8 Zoom;
	P.r_u8(Zoom);

	u8 scope;
	P.r_u8(scope);

	m_cur_scope = scope;

	if (H_Parent() && H_Parent()->Remote())
	{
		if (Zoom) OnZoomIn();
		else OnZoomOut();
	};
	switch (wstate)
	{	
	case eFire:
	case eFire2:
	case eSwitch:
	case eReload:
		{
		}break;	
	default:
		{
			if (ammoType >= m_ammoTypes.size())
				Msg("!! Weapon [%d], State - [%d]", ID(), wstate);
			else
			{
				m_ammoType = ammoType;
				SetAmmoElapsed((ammo_elapsed));
			}
		}break;
	}
	
	VERIFY((u32)iAmmoElapsed == m_magazine.size());
}

void CWeapon::save(NET_Packet &output_packet)
{
	inherited::save	(output_packet);
	save_data		(iAmmoElapsed,		output_packet);
	save_data		(m_cur_scope,		output_packet);
	save_data		(m_flagsAddOnState, output_packet);
	save_data		(m_ammoType,		output_packet);
	save_data		(m_bZoomMode,		output_packet);
}

void CWeapon::load(IReader &input_packet)
{
	inherited::load	(input_packet);
	load_data		(iAmmoElapsed,		input_packet);
	load_data		(m_cur_scope,		input_packet);
	load_data		(m_flagsAddOnState, input_packet);
	UpdateAddonsVisibility	();
	load_data		(m_ammoType,		input_packet);
	load_data		(m_bZoomMode,		input_packet);

	if (m_bZoomMode)	OnZoomIn();
		else			OnZoomOut();
}


void CWeapon::OnEvent(NET_Packet& P, u16 type) 
{
	switch (type)
	{
	case GE_ADDON_CHANGE:
		{
			P.r_u8					(m_flagsAddOnState);
			InitAddons();
			UpdateAddonsVisibility();
		}break;

	case GE_WPN_STATE_CHANGE:
		{
			u8				state;
			P.r_u8			(state);
			P.r_u8			(m_sub_state);		
//			u8 NewAmmoType = 
				P.r_u8();
			u8 AmmoElapsed = P.r_u8();
			u8 NextAmmo = P.r_u8();
			if (NextAmmo == static_cast<u8>(-1))
				m_set_next_ammoType_on_reload = static_cast<u32>(-1);
			else
				m_set_next_ammoType_on_reload = NextAmmo;

			if (OnClient())
				SetAmmoElapsed(int(AmmoElapsed));
			OnStateSwitch(u32(state), GetState());
		}
		break;
	default:
		{
			inherited::OnEvent(P,type);
		}break;
	}
};

void CWeapon::shedule_Update	(u32 dT)
{;	
	// Inherited
	inherited::shedule_Update	(dT);
}

void CWeapon::OnH_B_Independent	(bool just_before_destroy)
{
	RemoveShotEffector			();

	inherited::OnH_B_Independent(just_before_destroy);

	//завершить принудительно все процессы что шли
	FireEnd();
	SetPending					(FALSE);
	SwitchState(eIdle);

	m_strapped_mode				= false;
	m_bZoomMode					= false;
	UpdateXForm					();
	m_nearwall_last_hud_fov = psHUD_FOV_def;
}

void CWeapon::OnH_A_Independent	()
{
	m_dwWeaponIndependencyTime = Level().timeServer();

	m_fLR_MovingFactor = 0.f;
	m_fLR_CameraFactor = 0.f;
	m_fLR_InertiaFactor = 0.f;
	m_fUD_InertiaFactor = 0.f;
	inherited::OnH_A_Independent();
	Light_Destroy				();
};

void CWeapon::OnH_A_Chield		()
{
	inherited::OnH_A_Chield		();

	UpdateAddonsVisibility		();
};

void CWeapon::OnActiveItem ()
{
	inherited::OnActiveItem		();
	//если мы занружаемся и оружие было в руках
	SetState					(eIdle);
	SetNextState				(eIdle);
}

void CWeapon::OnHiddenItem ()
{
	inherited::OnHiddenItem();
	SetState					(eHidden);
	SetNextState				(eHidden);
	m_set_next_ammoType_on_reload	= static_cast<u32>(-1);
}


void CWeapon::OnH_B_Chield		()
{
	m_dwWeaponIndependencyTime = 0;
	inherited::OnH_B_Chield		();

	OnZoomOut					();
	m_set_next_ammoType_on_reload	= static_cast<u32>(-1);
	m_nearwall_last_hud_fov = psHUD_FOV_def;
}

u8 CWeapon::idle_state()
{
	auto* actor = smart_cast<CActor*>(H_Parent());

	if (actor)
	{
		u32 st = actor->get_state();
		if (st & mcSprint)
			return eSubstateIdleSprint;
		else if (st & mcAnyAction && !(st & mcJump) && !(st & mcFall))
			return eSubstateIdleMoving;
	}

	return eIdle;
}

void CWeapon::UpdateCL		()
{
	inherited::UpdateCL		();
	UpdateHUDAddonsVisibility();
	//подсветка от выстрела
	UpdateLight				();

	//нарисовать партиклы
	UpdateFlameParticles	();
	UpdateFlameParticles2	();

	if(!IsGameTypeSingle())
		make_Interpolation		();
	
	VERIFY(smart_cast<IKinematics*>(Visual()));


	if (GetState() == eIdle) {
		auto state = idle_state();
		if (m_idle_state != state) {
			m_idle_state = state;
			if (GetNextState() != eMagEmpty && GetNextState() != eReload)
			{
				SwitchState(eIdle);
			}
		}
	}
	else
		m_idle_state = eIdle;
}


void CWeapon::renderable_Render		()
{
	UpdateXForm				();

	//нарисовать подсветку

	RenderLight				();	

	//если мы в режиме снайперки, то сам HUD рисовать не надо
	if(IsZoomed() && !IsRotatingToZoom() && ZoomTexture())
		RenderHud(FALSE);
	else
		RenderHud(TRUE);

	inherited::renderable_Render		();
}

bool CWeapon::need_renderable()
{
	return Render->currentViewPort == MAIN_VIEWPORT && !(IsZoomed() && (ZoomTexture() || ZoomTextureNew()) && !IsRotatingToZoom());
}

void CWeapon::render_hud_mode()
{
	RenderLight();
}

bool CWeapon::MovingAnimAllowedNow()
{
	return !IsZoomed();
}

void CWeapon::signal_HideComplete()
{
	if (H_Parent())
		setVisible(FALSE);
	SetPending(FALSE);

	m_fLR_MovingFactor = 0.f;
	m_fLR_CameraFactor = 0.f;
	m_fLR_InertiaFactor = 0.f;
	m_fUD_InertiaFactor = 0.f;
}

void CWeapon::SetDefaults()
{
	bWorking2			= false;
	SetPending			(FALSE);

	m_flags.set			(FUsingCondition, TRUE);
	bMisfire			= false;
	m_flagsAddOnState	= 0;
	m_bZoomMode			= false;
}

void CWeapon::UpdatePosition(const Fmatrix& trans)
{
	Position().set		(trans.c);
	XFORM().mul			(trans,m_strapped_mode ? m_StrapOffset : m_Offset);
	VERIFY				(!fis_zero(DET(renderable.xform)));
}


bool CWeapon::Action(s32 cmd, u32 flags) 
{
	if(inherited::Action(cmd, flags)) return true;

	
	switch(cmd) 
	{
		case kWPN_FIRE: 
			{
				//если оружие чем-то занято, то ничего не делать
				{				
					if(flags&CMD_START) 
					{
						if(IsPending())		return false;
						FireStart			();
					}
					else 
						FireEnd();
				}

			} 
			return true;
		case kWPN_NEXT: 
			{
				if(IsPending() || OnClient()) 
				{
					return false;
				}
				if(flags & CMD_START) 
				{
					u32 l_newType = m_ammoType;
					bool b1,b2;
					do 
					{
						l_newType = (l_newType+1)%m_ammoTypes.size();
						b1 = l_newType != m_ammoType;
						b2 = unlimited_ammo() ? false : (!m_pCurrentInventory->GetAny(*m_ammoTypes[l_newType]));						
					} while( b1 && b2);

					if(l_newType != m_ammoType) 
					{
						m_set_next_ammoType_on_reload = l_newType;

						if(OnServer()) Reload();
					}
				}
			} 
            return true;

		case kWPN_ZOOM:
			if(IsZoomEnabled())
			{
                if(flags&CMD_START && !IsPending())
					OnZoomIn();
                else if(IsZoomed())
					OnZoomOut();
				return true;
			}
		    else 
				return false;

		case kWPN_ZOOM_INC:
		case kWPN_ZOOM_DEC:
			if(IsZoomEnabled() && IsZoomed())
			{
				if(cmd==kWPN_ZOOM_INC)  ZoomInc();
				else					ZoomDec();
				return true;
			}else
				return false;
	}
	return false;
}

void CWeapon::SpawnAmmo(u32 boxCurr, LPCSTR ammoSect, u32 ParentID) 
{
	if(m_ammoTypes.empty())			
		return;
	if (OnClient())					
		return;
	m_bAmmoWasSpawned				= true;
	
	int l_type						= 0;
	l_type							%= m_ammoTypes.size();

	if(!ammoSect) ammoSect			= *m_ammoTypes[l_type]; 
	
	++l_type; 
	l_type							%= m_ammoTypes.size();

	CSE_Abstract *D					= F_entity_Create(ammoSect);

	if (D->m_tClassID==CLSID_OBJECT_AMMO	||
		D->m_tClassID==CLSID_OBJECT_A_M209	||
		D->m_tClassID==CLSID_OBJECT_A_VOG25	||
		D->m_tClassID==CLSID_OBJECT_A_OG7B)
	{	
		auto *l_pA		= smart_cast<CSE_ALifeItemAmmo*>(D);
		R_ASSERT					(l_pA);
		l_pA->m_boxSize				= static_cast<u16>(pSettings->r_s32(ammoSect, "box_size"));
		D->s_name					= ammoSect;
		D->set_name_replace			("");
		D->s_gameid					= static_cast<u8>(GameID());
		D->s_RP						= 0xff;
		D->ID						= 0xffff;
		if (ParentID == 0xffffffff)	
			D->ID_Parent			= H_Parent()->ID();
		else
			D->ID_Parent			= static_cast<u16>(ParentID);

		D->ID_Phantom				= 0xffff;
		D->s_flags.assign			(M_SPAWN_OBJECT_LOCAL);
		D->RespawnTime				= 0;
		l_pA->m_tNodeID				= g_dedicated_server ? static_cast<u32>(-1) : ai_location().level_vertex_id();

		if(boxCurr == 0xffffffff) 	
			boxCurr					= l_pA->m_boxSize;

		while(boxCurr) 
		{
			l_pA->a_elapsed			= static_cast<u16>(boxCurr > l_pA->m_boxSize ? l_pA->m_boxSize : boxCurr);
			NET_Packet				P;
			D->Spawn_Write			(P, TRUE);
			Level().Send			(P,net_flags(TRUE));

			if(boxCurr > l_pA->m_boxSize) 
				boxCurr				-= l_pA->m_boxSize;
			else 
				boxCurr				= 0;
		}
	};
	F_entity_Destroy				(D);
}

int CWeapon::GetAmmoCurrent(bool use_item_to_spawn) const
{
	int l_count = iAmmoElapsed;
	if(!m_pCurrentInventory) return l_count;

	//чтоб не делать лишних пересчетов
	if(m_pCurrentInventory->ModifyFrame()<=m_dwAmmoCurrentCalcFrame)
		return l_count + iAmmoCurrent;

 	m_dwAmmoCurrentCalcFrame = Device.dwFrame;
	iAmmoCurrent = 0;

	for(int i = 0; i < static_cast<int>(m_ammoTypes.size()); ++i) 
	{
		LPCSTR l_ammoType = *m_ammoTypes[i];

		for(auto l_it = m_pCurrentInventory->m_belt.begin(); m_pCurrentInventory->m_belt.end() != l_it; ++l_it) 
		{
			auto *l_pAmmo = smart_cast<CWeaponAmmo*>(*l_it);

			if(l_pAmmo && !xr_strcmp(l_pAmmo->cNameSect(), l_ammoType)) 
			{
				iAmmoCurrent = iAmmoCurrent + l_pAmmo->m_boxCurr;
			}
		}

		for(auto l_it = m_pCurrentInventory->m_ruck.begin(); m_pCurrentInventory->m_ruck.end() != l_it; ++l_it) 
		{
			auto *l_pAmmo = smart_cast<CWeaponAmmo*>(*l_it);
			if(l_pAmmo && !xr_strcmp(l_pAmmo->cNameSect(), l_ammoType)) 
			{
				iAmmoCurrent = iAmmoCurrent + l_pAmmo->m_boxCurr;
			}
		}

		if (!use_item_to_spawn)
			continue;

		if (!inventory_owner().item_to_spawn())
			continue;

		iAmmoCurrent += inventory_owner().ammo_in_box_to_spawn();
	}
	return l_count + iAmmoCurrent;
}

float CWeapon::GetConditionMisfireProbability() const
{
	if( GetCondition()>0.95f ) return 0.0f;

	float mis = misfireProbability+powf(1.f-GetCondition(), 3.f)*misfireConditionK;
	clamp(mis,0.0f,0.99f);
	return mis;
}

BOOL CWeapon::CheckForMisfire()
{
	if (OnClient()) 
		return FALSE;

	float rnd = ::Random.randF(0.f,1.f);
	float mp = GetConditionMisfireProbability();
	if(rnd < mp)
	{
		if (iAmmoElapsed == 0)
			return false;

		FireEnd();

		bMisfire = true;
		SwitchState(eMisfire);		
		
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CWeapon::IsMisfire() const
{	
	return bMisfire;
}
void CWeapon::Reload()
{
	OnZoomOut();
}


bool CWeapon::IsGrenadeLauncherAttached() const
{
	return (ALife::eAddonAttachable == m_eGrenadeLauncherStatus &&
			0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher)) || 
			ALife::eAddonPermanent == m_eGrenadeLauncherStatus;
}

bool CWeapon::IsScopeAttached() const
{
	return (ALife::eAddonAttachable == m_eScopeStatus &&
			0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope)) || 
			ALife::eAddonPermanent == m_eScopeStatus;

}

bool CWeapon::IsSilencerAttached() const
{
	return (ALife::eAddonAttachable == m_eSilencerStatus &&
			0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer)) || 
			ALife::eAddonPermanent == m_eSilencerStatus;
}

bool CWeapon::GrenadeLauncherAttachable()
{
	return (ALife::eAddonAttachable == m_eGrenadeLauncherStatus);
}
bool CWeapon::ScopeAttachable()
{
	return (ALife::eAddonAttachable == m_eScopeStatus);
}
bool CWeapon::SilencerAttachable()
{
	return (ALife::eAddonAttachable == m_eSilencerStatus);
}

LPCSTR wpn_scope				= "wpn_scope";
LPCSTR wpn_silencer				= "wpn_silencer";
LPCSTR wpn_grenade_launcher		= "wpn_grenade_launcher";
LPCSTR wpn_launcher				= "wpn_launcher";

void CWeapon::UpdateHUDAddonsVisibility()
{
	if (!GetHUDmode())
		return;
	u16 bone_id = HudItemData()->m_model->LL_BoneID(wpn_scope);

	if (bone_id != BI_NONE)
	{
		if (ScopeAttachable())
			HudItemData()->set_bone_visible(m_sHud_wpn_scope_bone, IsScopeAttached());

		if (m_eScopeStatus == ALife::eAddonDisabled)
			HudItemData()->set_bone_visible(m_sHud_wpn_scope_bone, FALSE, TRUE);
		else if (m_eScopeStatus == ALife::eAddonPermanent)
			HudItemData()->set_bone_visible(m_sHud_wpn_scope_bone, TRUE, TRUE);
	}
	if (SilencerAttachable())
		HudItemData()->set_bone_visible(m_sHud_wpn_silencer_bone, IsSilencerAttached());

	if (m_eSilencerStatus == ALife::eAddonDisabled)
		HudItemData()->set_bone_visible(m_sHud_wpn_silencer_bone, FALSE, TRUE);
	else if (m_eSilencerStatus == ALife::eAddonPermanent)
		HudItemData()->set_bone_visible(m_sHud_wpn_silencer_bone, TRUE, TRUE);

	if (!HudItemData()->has_bone(m_sHud_wpn_launcher_bone) && HudItemData()->has_bone(wpn_launcher_def_bone_cop))
		m_sHud_wpn_launcher_bone = wpn_launcher_def_bone_cop;

	if (GrenadeLauncherAttachable())
		HudItemData()->set_bone_visible(m_sHud_wpn_launcher_bone, IsGrenadeLauncherAttached());
	
	if (m_eGrenadeLauncherStatus == ALife::eAddonDisabled)
		HudItemData()->set_bone_visible(m_sHud_wpn_launcher_bone, FALSE, TRUE);
	else if (m_eGrenadeLauncherStatus == ALife::eAddonPermanent)
		HudItemData()->set_bone_visible(m_sHud_wpn_launcher_bone, TRUE, TRUE);

	for (const shared_str& bone_name : hud_hidden_bones)
		HudItemData()->set_bone_visible(bone_name, FALSE, TRUE);
}
void CWeapon::UpdateAddonsVisibility()
{
	auto* pWeaponVisual = smart_cast<IKinematics*>(Visual());
	R_ASSERT(pWeaponVisual);

	u16  bone_id;
	UpdateHUDAddonsVisibility								();	

	bone_id = pWeaponVisual->LL_BoneID					(wpn_scope);
	if(ScopeAttachable())
	{
		if(IsScopeAttached())
		{
			if(!pWeaponVisual->LL_GetBoneVisible		(bone_id) && bone_id!=BI_NONE)
			pWeaponVisual->LL_SetBoneVisible				(bone_id,TRUE,TRUE);
		}else{
			if(pWeaponVisual->LL_GetBoneVisible				(bone_id) && bone_id != BI_NONE)
				pWeaponVisual->LL_SetBoneVisible			(bone_id,FALSE,TRUE);
		}
	}
	if(m_eScopeStatus == ALife::eAddonDisabled && bone_id!=BI_NONE &&
		pWeaponVisual->LL_GetBoneVisible(bone_id) )

		pWeaponVisual->LL_SetBoneVisible			(bone_id,FALSE,TRUE);

	bone_id = pWeaponVisual->LL_BoneID					(wpn_silencer);
	if(SilencerAttachable())
	{
		if(IsSilencerAttached()){
			if(FALSE==pWeaponVisual->LL_GetBoneVisible		(bone_id))
				pWeaponVisual->LL_SetBoneVisible			(bone_id,TRUE,TRUE);
		}else{
			if( pWeaponVisual->LL_GetBoneVisible				(bone_id))
				pWeaponVisual->LL_SetBoneVisible			(bone_id,FALSE,TRUE);
		}
	}
	if(m_eSilencerStatus == ALife::eAddonDisabled && bone_id!=BI_NONE &&
		pWeaponVisual->LL_GetBoneVisible(bone_id) )

		pWeaponVisual->LL_SetBoneVisible			(bone_id,FALSE,TRUE);

	bone_id = pWeaponVisual->LL_BoneID					(wpn_launcher);
	if(GrenadeLauncherAttachable())
	{
		if(IsGrenadeLauncherAttached())
		{
			if(FALSE==pWeaponVisual->LL_GetBoneVisible		(bone_id))
				pWeaponVisual->LL_SetBoneVisible			(bone_id,TRUE,TRUE);
		}else{
			if(pWeaponVisual->LL_GetBoneVisible				(bone_id))
				pWeaponVisual->LL_SetBoneVisible			(bone_id,FALSE,TRUE);
		}
	}
	if(m_eGrenadeLauncherStatus == ALife::eAddonDisabled && bone_id!=BI_NONE &&
		pWeaponVisual->LL_GetBoneVisible(bone_id) )

		pWeaponVisual->LL_SetBoneVisible			(bone_id,FALSE,TRUE);
	

	pWeaponVisual->CalculateBones_Invalidate				();
	pWeaponVisual->CalculateBones							();
}

bool CWeapon::Activate() 
{
	UpdateAddonsVisibility();
	return inherited::Activate();
}

void CWeapon::InitAddons()
{
}

float CWeapon::CurrentZoomFactor	()
{
	if (psActorFlags.test(AF_3DSCOPE_ENABLE) && IsScopeAttached())
	{
		return bIsSecondVPZoomPresent() ? m_zoom_params.m_f3dZoomFactor : m_fScopeZoomFactor;
	}
	else
	{
		return IsScopeAttached() ? m_fScopeZoomFactor : m_fIronSightZoomFactor;
	}
};

void CWeapon::OnZoomIn()
{
	m_bZoomMode = true;
	m_fZoomFactor = CurrentZoomFactor();
	AllowHudInertion(FALSE);

//	if (m_zoom_params.m_bZoomDofEnabled && !IsScopeAttached())
//		GamePersistent().SetEffectorDOF(m_zoom_params.m_ZoomDof);

	if (GetHUDmode())
		GamePersistent().SetPickableEffectorDOF(true);
}

void CWeapon::OnZoomOut()
{
//	if (!bIsSecondVPZoomPresent() && psActorFlags.test(AF_3DSCOPE_ENABLE))
//		m_fRTZoomFactor = GetZoomFactor(); // Сохраняем текущий динамический зум
	m_bZoomMode = false;
	m_fZoomFactor = g_fov;

	AllowHudInertion(FALSE);

    GamePersistent().RestoreEffectorDOF();

	if (GetHUDmode())
		GamePersistent().SetPickableEffectorDOF(false);

	ResetSubStateTime();
}

CUIStaticItem* CWeapon::ZoomTexture()
{
	if (UseScopeTexture() && (!psActorFlags.test(AF_3DSCOPE_ENABLE) || !bIsSecondVPZoomPresent()))
		return m_UIScope;
	else
		return nullptr;
}

CUIWindow* CWeapon::ZoomTextureNew()
{
	if (UseScopeTexture() && (!psActorFlags.test(AF_3DSCOPE_ENABLE) || !bIsSecondVPZoomPresent()))
		return m_UIScopeNew;
	else
		return NULL;
}

void CWeapon::SwitchState(u32 S)
{
	if (OnClient()) return;

	SetNextState		( S );	// Very-very important line of code!!! :)
	if (CHudItem::object().Local() && !CHudItem::object().getDestroy()/* && (S!=NEXT_STATE)*/ 
		&& m_pCurrentInventory && OnServer())	
	{
		// !!! Just single entry for given state !!!
		NET_Packet		P;
		CHudItem::object().u_EventGen		(P,GE_WPN_STATE_CHANGE,CHudItem::object().ID());
		P.w_u8			(u8(S));
		P.w_u8			(u8(m_sub_state));
		P.w_u8			(u8(m_ammoType& 0xff));
		P.w_u8			(u8(iAmmoElapsed & 0xff));
		P.w_u8			(u8(m_set_next_ammoType_on_reload & 0xff));
		CHudItem::object().u_EventSend		(P, net_flags(TRUE, TRUE, FALSE, TRUE));
	}
}

void CWeapon::OnMagazineEmpty	()
{
	VERIFY((u32)iAmmoElapsed == m_magazine.size());
}


void CWeapon::reinit			()
{
	CShootingObject::reinit		();
	CHudItemObject::reinit			();
}

void CWeapon::reload			(LPCSTR section)
{
	CShootingObject::reload		(section);
	CHudItemObject::reload			(section);
	
	m_can_be_strapped			= true;
	m_strapped_mode				= false;
	
	if (pSettings->line_exist(section,"strap_bone0"))
		m_strap_bone0			= pSettings->r_string(section,"strap_bone0");
	else
		m_can_be_strapped		= false;
	
	if (pSettings->line_exist(section,"strap_bone1"))
		m_strap_bone1			= pSettings->r_string(section,"strap_bone1");
	else
		m_can_be_strapped		= false;

	if (m_eScopeStatus == ALife::eAddonAttachable) {
		m_addon_holder_range_modifier	= READ_IF_EXISTS(pSettings,r_float, GetScopeName(),"holder_range_modifier",m_holder_range_modifier);
		m_addon_holder_fov_modifier		= READ_IF_EXISTS(pSettings,r_float, GetScopeName(),"holder_fov_modifier",m_holder_fov_modifier);
	}
	else {
		m_addon_holder_range_modifier	= m_holder_range_modifier;
		m_addon_holder_fov_modifier		= m_holder_fov_modifier;
	}


	{
		Fvector				pos,ypr;
		pos					= pSettings->r_fvector3		(section,"position");
		ypr					= pSettings->r_fvector3		(section,"orientation");
		ypr.mul				(PI/180.f);

		m_Offset.setHPB			(ypr.x,ypr.y,ypr.z);
		m_Offset.translate_over	(pos);
	}

	m_StrapOffset			= m_Offset;
	if (pSettings->line_exist(section,"strap_position") && pSettings->line_exist(section,"strap_orientation")) {
		Fvector				pos,ypr;
		pos					= pSettings->r_fvector3		(section,"strap_position");
		ypr					= pSettings->r_fvector3		(section,"strap_orientation");
		ypr.mul				(PI/180.f);

		m_StrapOffset.setHPB			(ypr.x,ypr.y,ypr.z);
		m_StrapOffset.translate_over	(pos);
	}
	else
		m_can_be_strapped	= false;

	m_ef_main_weapon_type	= READ_IF_EXISTS(pSettings,r_u32,section,"ef_main_weapon_type",u32(-1));
	m_ef_weapon_type		= READ_IF_EXISTS(pSettings,r_u32,section,"ef_weapon_type",u32(-1));
}

void CWeapon::create_physic_shell()
{
	CPhysicsShellHolder::create_physic_shell();
}

void CWeapon::activate_physic_shell()
{
	CPhysicsShellHolder::activate_physic_shell();
}

void CWeapon::setup_physic_shell()
{
	CPhysicsShellHolder::setup_physic_shell();
}

int		g_iWeaponRemove = 1;

bool CWeapon::NeedToDestroyObject()	const
{
	if (H_Parent()) return false;
	if (g_iWeaponRemove == -1) return false;
	if (g_iWeaponRemove == 0) return true;
	return (TimePassedAfterIndependant() > m_dwWeaponRemoveTime);
}

ALife::_TIME_ID	 CWeapon::TimePassedAfterIndependant()	const
{
	if(!H_Parent() && m_dwWeaponIndependencyTime != 0)
		return Level().timeServer() - m_dwWeaponIndependencyTime;
	else
		return 0;
}

bool CWeapon::can_kill	() const
{
	if (GetAmmoCurrent(true) || m_ammoTypes.empty())
		return				(true);

	return					(false);
}

CInventoryItem *CWeapon::can_kill	(CInventory *inventory) const
{
	if (GetAmmoElapsed() || m_ammoTypes.empty())
		return				(const_cast<CWeapon*>(this));

	TIItemContainer::iterator I = inventory->m_all.begin();
	TIItemContainer::iterator E = inventory->m_all.end();
	for ( ; I != E; ++I) {
		CInventoryItem	*inventory_item = smart_cast<CInventoryItem*>(*I);
		if (!inventory_item)
			continue;
		
		xr_vector<shared_str>::const_iterator	i = std::find(m_ammoTypes.begin(),m_ammoTypes.end(),inventory_item->object().cNameSect());
		if (i != m_ammoTypes.end())
			return			(inventory_item);
	}

	return					(0);
}

const CInventoryItem *CWeapon::can_kill	(const xr_vector<const CGameObject*> &items) const
{
	if (m_ammoTypes.empty())
		return				(this);

	xr_vector<const CGameObject*>::const_iterator I = items.begin();
	xr_vector<const CGameObject*>::const_iterator E = items.end();
	for ( ; I != E; ++I) {
		const CInventoryItem	*inventory_item = smart_cast<const CInventoryItem*>(*I);
		if (!inventory_item)
			continue;

		xr_vector<shared_str>::const_iterator	i = std::find(m_ammoTypes.begin(),m_ammoTypes.end(),inventory_item->object().cNameSect());
		if (i != m_ammoTypes.end())
			return			(inventory_item);
	}

	return					(0);
}

bool CWeapon::ready_to_kill	() const
{
	return					(
		!IsMisfire() && 
		((GetState() == eIdle) || (GetState() == eFire) || (GetState() == eFire2)) && 
		GetAmmoElapsed()
	);
}

// Получить индекс текущих координат худа
u8 CWeapon::GetCurrentHudOffsetIdx()
{
	const bool b_aiming = ((IsZoomed() && m_fZoomRotationFactor <= 1.f) || (!IsZoomed() && m_fZoomRotationFactor > 0.f));

	if (b_aiming)
	{
		const bool has_gl = GrenadeLauncherAttachable() && IsGrenadeLauncherAttached();
		const bool has_scope = ScopeAttachable() && IsScopeAttached();

		if (IsGrenadeMode())
		{
			if (m_bUseScopeGrenadeZoom && has_scope)
				return hud_item_measures::m_hands_offset_type_gl_scope;
			else
				return hud_item_measures::m_hands_offset_type_gl;
		}
		else if (has_gl)
		{
			if (m_bUseScopeZoom && has_scope)
				return hud_item_measures::m_hands_offset_type_gl_normal_scope;
			else
				return hud_item_measures::m_hands_offset_type_aim_gl_normal;
		}
		else
		{
			if (m_bUseScopeZoom && has_scope)
				return hud_item_measures::m_hands_offset_type_aim_scope;
			else
				return hud_item_measures::m_hands_offset_type_aim;
		}
	}

	return hud_item_measures::m_hands_offset_type_normal;
}

bool CWeapon::IsGrenadeMode() const
{
	const auto wpn_w_gl = smart_cast<const CWeaponMagazinedWGrenade*>(this);
	return wpn_w_gl && wpn_w_gl->m_bGrenadeMode;
}

void CWeapon::UpdateHudAdditonal		(Fmatrix& trans)
{
	auto pActor = smart_cast<const CActor*>(H_Parent());
	if(!pActor) return;

	u8 idx = GetCurrentHudOffsetIdx();

	//============= Поворот ствола во время аима =============//
	if(		(pActor->IsZoomAimingMode() && m_fZoomRotationFactor<=1.f) ||
			(!pActor->IsZoomAimingMode() && m_fZoomRotationFactor>0.f))
	{
		attachable_hud_item* hi = HudItemData();
		R_ASSERT(hi);
		Fvector curr_offs, curr_rot;
		curr_offs = hi->m_measures.m_hands_offset[hud_item_measures::m_hands_offset_pos][idx];
		curr_rot = hi->m_measures.m_hands_offset[hud_item_measures::m_hands_offset_rot][idx];
		curr_offs.mul(m_fZoomRotationFactor);
		curr_rot.mul(m_fZoomRotationFactor);

		Fmatrix hud_rotation;
		hud_rotation.identity();
		hud_rotation.rotateX(curr_rot.x);

		Fmatrix hud_rotation_y;
		hud_rotation_y.identity();
		hud_rotation_y.rotateY(curr_rot.y);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation_y.identity();
		hud_rotation_y.rotateZ(curr_rot.z);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation.translate_over(curr_offs);
		trans.mulB_43(hud_rotation);

		if(pActor->IsZoomAimingMode())
		{
			m_fZoomRotationFactor += Device.fTimeDelta/m_fZoomRotateTime;
		}
		else
		{
			m_fZoomRotationFactor -= Device.fTimeDelta/m_fZoomRotateTime;
		}
		clamp(m_fZoomRotationFactor, 0.f, 1.f);
	}

	// Боковой стрейф с оружием
	clamp(idx, 0ui8, 1ui8);

	// Рассчитываем фактор боковой ходьбы
	float fStrafeMaxTime = m_strafe_offset[2][idx].y; // Макс. время в секундах, за которое мы наклонимся из центрального положения
	if (fStrafeMaxTime <= EPS)
		fStrafeMaxTime = 0.01f;

	float fStepPerUpd = Device.fTimeDelta / fStrafeMaxTime; // Величина изменение фактора поворота

/*	u32 iMovingState = pActor->MovingState();
	if ((iMovingState & mcLStrafe) != 0)
 { // Движемся влево
		float fVal = (m_fLR_MovingFactor > 0.f ? fStepPerUpd * 3 : fStepPerUpd);
		m_fLR_MovingFactor -= fVal;
	}
	else if ((iMovingState & mcRStrafe) != 0)
	{ // Движемся вправо
		float fVal = (m_fLR_MovingFactor < 0.f ? fStepPerUpd * 3 : fStepPerUpd);
		m_fLR_MovingFactor += fVal;
	}
	else*/
	{ // Двигаемся в любом другом направлении
		if (m_fLR_MovingFactor < 0.0f)
		{
			m_fLR_MovingFactor += fStepPerUpd;
			clamp(m_fLR_MovingFactor, -1.0f, 0.0f);
		}
		else
		{
			m_fLR_MovingFactor -= fStepPerUpd;
			clamp(m_fLR_MovingFactor, 0.0f, 1.0f);
		}
	}

	clamp(m_fLR_MovingFactor, -1.0f, 1.0f); // Фактор боковой ходьбы не должен превышать эти лимиты

	// Производим наклон ствола для нормального режима и аима
	for (int _idx = 0; _idx <= 1; _idx++)
	{
		bool bEnabled = m_strafe_offset[2][_idx].x;
		if (!bEnabled)
			continue;

		Fvector curr_offs, curr_rot;

		// Смещение позиции худа в стрейфе
		curr_offs = m_strafe_offset[0][_idx]; //pos
		curr_offs.mul(m_fLR_MovingFactor);                   // Умножаем на фактор стрейфа

		// Поворот худа в стрейфе
		curr_rot = m_strafe_offset[1][_idx]; //rot
		curr_rot.mul(-PI / 180.f);                          // Преобразуем углы в радианы
		curr_rot.mul(m_fLR_MovingFactor);                   // Умножаем на фактор стрейфа

		if (_idx == 0)
		{ // От бедра
			curr_offs.mul(1.f - m_fZoomRotationFactor);
			curr_rot.mul(1.f - m_fZoomRotationFactor);
		}
		else
		{ // Во время аима
			curr_offs.mul(m_fZoomRotationFactor);
			curr_rot.mul(m_fZoomRotationFactor);
		}

		Fmatrix hud_rotation;
		Fmatrix hud_rotation_y;

		hud_rotation.identity();
		hud_rotation.rotateX(curr_rot.x);

		hud_rotation_y.identity();
		hud_rotation_y.rotateY(curr_rot.y);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation_y.identity();
		hud_rotation_y.rotateZ(curr_rot.z);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation.translate_over(curr_offs);
		trans.mulB_43(hud_rotation);
	}
}

void	CWeapon::SetAmmoElapsed	(int ammo_count)
{
	iAmmoElapsed				= ammo_count;

	
	if (const u32 uAmmo	= static_cast<u32>(iAmmoElapsed); uAmmo != m_magazine.size())
	{
		if (uAmmo > m_magazine.size())
		{
			CCartridge			l_cartridge; 
			l_cartridge.Load	(*m_ammoTypes[m_ammoType], u8(m_ammoType));
			while (uAmmo > m_magazine.size())
				m_magazine.push_back(l_cartridge);
		}
		else
		{
			while (uAmmo < m_magazine.size())
				m_magazine.pop_back();
		}
	}
}

u32	CWeapon::ef_main_weapon_type	() const
{
	VERIFY	(m_ef_main_weapon_type != u32(-1));
	return	(m_ef_main_weapon_type);
}

u32	CWeapon::ef_weapon_type	() const
{
	VERIFY	(m_ef_weapon_type != u32(-1));
	return	(m_ef_weapon_type);
}

bool CWeapon::IsNecessaryItem	    (const shared_str& item_sect)
{
	return (std::find(m_ammoTypes.begin(), m_ammoTypes.end(), item_sect) != m_ammoTypes.end() );
}

void CWeapon::modify_holder_params		(float &range, float &fov) const
{
	if (!IsScopeAttached()) {
		inherited::modify_holder_params	(range,fov);
		return;
	}
	range	*= m_addon_holder_range_modifier;
	fov		*= m_addon_holder_fov_modifier;
}

void CWeapon::OnDrawUI()
{
	if(IsZoomed() && ZoomHideCrosshair())
	{
		if(ZoomTexture() && !IsRotatingToZoom())
		{
			ZoomTexture()->SetPos	(0,0);
			ZoomTexture()->SetRect	(0,0,UI_BASE_WIDTH, UI_BASE_HEIGHT);
			ZoomTexture()->Render	();

//			m_UILens.Draw();
		}
		else if (ZoomTextureNew() && SOCScopesXmlEnable && !IsRotatingToZoom())
		{ 
			ZoomTextureNew()->Update();
			ZoomTextureNew()->Draw();
		}
	}
}

bool CWeapon::IsHudModeNow()
{
	return (HudItemData() != nullptr);
}

bool CWeapon::unlimited_ammo() const
{ 
	if (GameID() == GAME_SINGLE	)
		return psActorFlags.test(AF_UNLIMITEDAMMO) && 
				m_DefaultCartridge.m_flags.test(CCartridge::cfCanBeUnlimited); 

	return (GameID()!=GAME_ARTEFACTHUNT) && 
		m_DefaultCartridge.m_flags.test(CCartridge::cfCanBeUnlimited); 
			
};

LPCSTR	CWeapon::GetCurrentAmmo_ShortName() const
{
	if (m_magazine.empty()) 
		return ("");
	const CCartridge &l_cartridge = m_magazine.back();
	return *(l_cartridge.m_InvShortName);
}

float CWeapon::Weight()
{
	float res = CInventoryItemObject::Weight();
	if(IsGrenadeLauncherAttached()&&GetGrenadeLauncherName().size()){
		res += pSettings->r_float(GetGrenadeLauncherName(),"inv_weight");
	}
	if (IsScopeAttached() && m_scopes.size()) {
		res += pSettings->r_float(GetScopeName(), "inv_weight");
	}
	if(IsSilencerAttached()&&GetSilencerName().size()){
		res += pSettings->r_float(GetSilencerName(),"inv_weight");
	}
	
	if(iAmmoElapsed)
	{
		const float w		= pSettings->r_float(*m_ammoTypes[m_ammoType],"inv_weight");
		const float bs	= pSettings->r_float(*m_ammoTypes[m_ammoType],"box_size");

		res			+= w*(static_cast<float>(iAmmoElapsed)/bs);
	}
	return res;
}
void CWeapon::Hide()
{
	SwitchState(eHiding);

	OnZoomOut();
}

void CWeapon::Show()
{
	SwitchState(eShowing);
}


bool CWeapon::show_crosshair()
{
	return !IsZoomed();
}

bool CWeapon::show_indicators()
{
	return ! ( IsZoomed() && (ZoomTexture() || ZoomTextureNew()) );
}

float CWeapon::GetConditionToShow	() const
{
	return	(GetCondition());
}

BOOL CWeapon::ParentMayHaveAimBullet	()
{
	CObject* O=H_Parent();
	auto* EA=smart_cast<CEntityAlive*>(O);
	return EA->cast_actor() != nullptr;
}

BOOL CWeapon::ParentIsActor	()
{
	CObject* O = H_Parent();
	auto* EA=smart_cast<CEntityAlive*>(O);
	return EA->cast_actor() != nullptr;
}

const float &CWeapon::hit_probability() const
{
	VERIFY					((g_SingleGameDifficulty >= egdNovice) && (g_SingleGameDifficulty <= egdMaster)); 
	return					(m_hit_probability[egdNovice]);
}

void CWeapon::OnStateSwitch(u32 S, u32 oldState)
{
	inherited::OnStateSwitch(S, oldState);
	m_dwAmmoCurrentCalcFrame = 0;
}

// Получить HUD FOV текущего оружия
float CWeapon::GetHudFov()
{
	// Рассчитываем HUD FOV от бедра (с учётом упирания в стены)
	if (ParentIsActor() && Level().CurrentViewEntity() == H_Parent())
	{
		// Получаем расстояние от камеры до точки в прицеле
		collide::rq_result& RQ = HUD().GetCurrentRayQuery();
		float dist = RQ.range;

		// Интерполируем расстояние в диапазон от 0 (min) до 1 (max)
		clamp(dist, m_nearwall_dist_min, m_nearwall_dist_max);
		float fDistanceMod =
			((dist - m_nearwall_dist_min) / (m_nearwall_dist_max - m_nearwall_dist_min)); // 0.f ... 1.f

		// Рассчитываем базовый HUD FOV от бедра
		float fBaseFov = psHUD_FOV_def + m_hud_fov_add_mod;
		clamp(fBaseFov, 0.0f, FLT_MAX);

		// Плавно высчитываем итоговый FOV от бедра
		float src = m_nearwall_speed_mod * Device.fTimeDelta;
		clamp(src, 0.f, 1.f);

		float fTrgFov = m_nearwall_target_hud_fov + fDistanceMod * (fBaseFov - m_nearwall_target_hud_fov);
		m_nearwall_last_hud_fov = m_nearwall_last_hud_fov * (1 - src) + fTrgFov * src;
	}
	return m_nearwall_last_hud_fov;
}

bool CWeapon::IsMisfireNow()
{
	return IsMisfire();
}

bool CWeapon::IsMagazineEmpty()
{
	return IsEmptyMagazine();
}

BOOL CWeapon::IsEmptyMagazine() const
{
	return (iAmmoElapsed == 0);
}

// Обновление необходимости включения второго вьюпорта +SecondVP+
// Вызывается только для активного оружия игрока
void CWeapon::UpdateSecondVP(bool bInGrenade)
{
	bool b_is_active_item = (m_pCurrentInventory != NULL) && (m_pCurrentInventory->ActiveItem() == this);
	R_ASSERT(ParentIsActor() && b_is_active_item); // Эта функция должна вызываться только для оружия в руках нашего игрока

	CActor* pActor = smart_cast<CActor*>(H_Parent());

	bool bCond_1 = bInZoomRightNow();													// Мы должны целиться

	bool bCond_2 = bIsSecondVPZoomPresent() && psActorFlags.test(AF_3DSCOPE_ENABLE);	// В конфиге должен быть прописан фактор зума для линзы (scope_lense_factor
	// больше чем 0)
	bool bCond_3 = pActor->cam_Active() == pActor->cam_FirstEye();						// Мы должны быть от 1-го лица	

	Device.m_SecondViewport.SetSVPActive(bCond_1 && bCond_2 && bCond_3 && !bInGrenade);
}

float CWeapon::GetSecondVPZoomFactor() const
{
	float dist_k = (100.f / m_zoom_params.m_f3dZoomFactor);

	clamp(dist_k, 0.0f, 1.0f);

	float result = (m_zoom_params.m_fSecondVPFovFactor / dist_k);

	return result;
}

float CWeapon::GetSecondVPFov() const
{
//	if (m_zoom_params.m_bUseDynamicZoom && bIsSecondVPZoomPresent())
//		return (m_fSecondRTZoomFactor / 100.f) * 75.f;//g_fov; 75.f

	return GetSecondVPZoomFactor() * 75.f;//g_fov; 75.f
}
