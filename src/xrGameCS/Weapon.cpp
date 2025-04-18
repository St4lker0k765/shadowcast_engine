#include "stdafx.h"
#include "Weapon.h"
#include "ParticlesObject.h"
#include "entity_alive.h"
#include "inventory_item_impl.h"
#include "inventory.h"
#include "xrserver_objects_alife_items.h"
#include "actor.h"
#include "actoreffector.h"
#include "level.h"
#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "../Include/xrRender/Kinematics.h"
#include "ai_object_location.h"
#include "mathutils.h"
#include "object_broker.h"
#include "player_hud.h"
#include "HUDManager.h"
#include "gamepersistent.h"
#include "effectorFall.h"
#include "debug_renderer.h"
#include "static_cast_checked.hpp"
#include "clsid_game.h"
#include "weaponBinocularsVision.h"
#include "ui/UIWindow.h"
#include "ui/UIXmlInit.h"
#include "Torch.h"
#include "inventory_item.h"

#define WEAPON_REMOVE_TIME		60000
#define ROTATION_TIME			0.25f

ENGINE_API extern float psHUD_FOV_def;

BOOL	b_toggle_weapon_aim = FALSE;
extern CUIXml* pWpnScopeXml = NULL;

CWeapon::CWeapon()
{
	SetState(eHidden);
	SetNextState(eHidden);
	m_sub_state = eSubstateReloadBegin;
	m_bTriStateReload = false;
	SetDefaults();

	m_Offset.identity();
	m_StrapOffset.identity();

	m_iAmmoCurrentTotal = 0;
	m_BriefInfo_CalcFrame = 0;

	iAmmoElapsed = -1;
	iMagazineSize = -1;
	m_ammoType = 0;

	eHandDependence = hdNone;

	m_zoom_params.m_fCurrentZoomFactor = g_fov;
	m_zoom_params.m_fZoomRotationFactor = 0.f;
	m_zoom_params.m_pVision = NULL;
	m_zoom_params.m_pNight_vision = NULL;

	m_pCurrentAmmo = NULL;

	m_pFlameParticles2 = NULL;
	m_sFlameParticles2 = NULL;

	m_fCurrentCartirdgeDisp = 1.f;

	m_strap_bone0 = 0;
	m_strap_bone1 = 0;
	m_StrapOffset.identity();
	m_strapped_mode = false;
	m_can_be_strapped = false;
	m_ef_main_weapon_type = static_cast<u32>(-1);
	m_ef_weapon_type = static_cast<u32>(-1);
	m_ai_weapon_rank = static_cast<u32>(-1);
	m_UIScope = NULL;
	m_set_next_ammoType_on_reload = undefined_ammo_type;
	m_crosshair_inertion = 0.f;
	m_activation_speed_is_overriden = false;
	m_cur_scope = NULL;
	m_bRememberActorNVisnStatus = false;

	//Mortan: new params
	bUseAltScope = false;
	bScopeIsHasTexture = false;
	bNVsecondVPavaible = false;
	bNVsecondVPstatus = false;

	m_nearwall_last_hud_fov = psHUD_FOV_def;
	m_fZoomStepCount = 3.0f;
	m_fZoomMinKoeff = 0.3f;
	m_fLR_MovingFactor = 0.f;
	m_fLR_CameraFactor = 0.f;
	m_fLR_InertiaFactor = 0.f;
	m_fUD_InertiaFactor = 0.f;

	m_zoom_params.m_f3dZoomFactor = 0.0f;
	m_zoom_params.m_fSecondVPFovFactor = 0.0f;

	m_fSecondRTZoomFactor = -1.0f;

	bHasBulletsToHide = false;
	bullet_cnt = 0;

	NeedUpdateHudParams = false;


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

bool CWeapon::bChangeNVSecondVPStatus()
{
	if (!bNVsecondVPavaible || !IsZoomed())
		return false;

	bNVsecondVPstatus = !bNVsecondVPstatus;

	return true;
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
			if (UseHDIcons)
				return pSettings->r_s32(GetNameWithAttachment(), "scope_x") * 2;
			else
				return pSettings->r_s32(GetNameWithAttachment(), "scope_x");
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if (UseHDIcons)
			return pSettings->r_s32(m_scopes[m_cur_scope], "scope_x") * 2;
		else
			return pSettings->r_s32(m_scopes[m_cur_scope], "scope_x");
	}
}

int CWeapon::GetScopeY()
{
	if (bUseAltScope)
	{
		if (m_eScopeStatus != ALife::eAddonPermanent && IsScopeAttached())
		{
			if (UseHDIcons)
				return pSettings->r_s32(GetNameWithAttachment(), "scope_y") * 2;
			else
				return pSettings->r_s32(GetNameWithAttachment(), "scope_y");
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if (UseHDIcons)
			return pSettings->r_s32(m_scopes[m_cur_scope], "scope_y") * 2;
		else
			return pSettings->r_s32(m_scopes[m_cur_scope], "scope_y");
	}
}

CWeapon::~CWeapon()
{
	xr_delete(m_UIScope);
	delete_data(m_scopes);
}

void CWeapon::Hit(SHit* pHDS)
{
	inherited::Hit(pHDS);
}

void CWeapon::UpdateXForm()
{
	if (Device.dwFrame == dwXF_Frame)
		return;

	dwXF_Frame = Device.dwFrame;

	if (!H_Parent())
		return;

	// Get access to entity and its visual
	CEntityAlive* E = smart_cast<CEntityAlive*>(H_Parent());

	if (!E) {
		if (!IsGameTypeSingle())
			UpdatePosition(H_Parent()->XFORM());

		return;
	}

	const CInventoryOwner* parent = smart_cast<const CInventoryOwner*>(E);
	if (parent && parent->use_simplified_visual())
		return;

	if (parent->attached(this))
		return;

	IKinematics* V = smart_cast<IKinematics*>	(E->Visual());
	VERIFY(V);

	// Get matrices
	int						boneL = -1, boneR = -1, boneR2 = -1;

	// this ugly case is possible in case of a CustomMonster, not a Stalker, nor an Actor
	E->g_WeaponBones(boneL, boneR, boneR2);

	if (boneR == -1)		return;

	if ((HandDependence() == hd1Hand) || (GetState() == eReload) || (!E->g_Alive()))
		boneL = boneR2;

	V->CalculateBones_Invalidate();
	V->CalculateBones(TRUE);

	Fmatrix& mL = V->LL_GetTransform(u16(boneL));
	Fmatrix& mR = V->LL_GetTransform(u16(boneR));
	// Calculate
	Fmatrix					mRes;
	Fvector					R, D, N;
	D.sub(mL.c, mR.c);

	if (fis_zero(D.magnitude())) {
		mRes.set(E->XFORM());
		mRes.c.set(mR.c);
	}
	else {
		D.normalize();
		R.crossproduct(mR.j, D);

		N.crossproduct(D, R);
		N.normalize();

		mRes.set(R, N, D, mR.c);
		mRes.mulA_43(E->XFORM());
	}

	UpdatePosition(mRes);
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
	{//update particlesXFORM real bullet direction

		if (!H_Parent())		return;

		Fvector					p, d;
		smart_cast<CEntity*>(H_Parent())->g_fireParams(this, p, d);

		Fmatrix						_pxf;
		_pxf.k = d;
		_pxf.i.crossproduct(Fvector().set(0.0f, 1.0f, 0.0f), _pxf.k);
		_pxf.j.crossproduct(_pxf.k, _pxf.i);
		_pxf.c = XFORM().c;

		m_current_firedeps.m_FireParticlesXForm.set(_pxf);
	}
}

void CWeapon::Load(LPCSTR section)
{
	inherited::Load(section);
	CShootingObject::Load(section);


	if (pSettings->line_exist(section, "flame_particles_2"))
		m_sFlameParticles2 = pSettings->r_string(section, "flame_particles_2");

	// load ammo classes
	m_ammoTypes.clear();
	LPCSTR				S = pSettings->r_string(section, "ammo_class");
	if (S && S[0])
	{
		string128		_ammoItem;
		int				count = _GetItemCount(S);
		for (int it = 0; it < count; ++it)
		{
			_GetItem(S, it, _ammoItem);
			m_ammoTypes.push_back(_ammoItem);
		}
	}

	iAmmoElapsed = pSettings->r_s32(section, "ammo_elapsed");
	iMagazineSize = pSettings->r_s32(section, "ammo_mag_size");

	////////////////////////////////////////////////////
	// дисперсия стрельбы

	//подбрасывание камеры во время отдачи
	u8 rm = READ_IF_EXISTS(pSettings, r_u8, section, "cam_return", 1);
	cam_recoil.ReturnMode = (rm == 1);

	rm = READ_IF_EXISTS(pSettings, r_u8, section, "cam_return_stop", 0);
	cam_recoil.StopReturn = (rm == 1);

	float temp_f = 0.0f;
	temp_f = pSettings->r_float(section, "cam_relax_speed");
	cam_recoil.RelaxSpeed = _abs(deg2rad(temp_f));
	VERIFY(!fis_zero(cam_recoil.RelaxSpeed));
	if (fis_zero(cam_recoil.RelaxSpeed))
	{
		cam_recoil.RelaxSpeed = EPS_L;
	}

	cam_recoil.RelaxSpeed_AI = cam_recoil.RelaxSpeed;
	if (pSettings->line_exist(section, "cam_relax_speed_ai"))
	{
		temp_f = pSettings->r_float(section, "cam_relax_speed_ai");
		cam_recoil.RelaxSpeed_AI = _abs(deg2rad(temp_f));
		VERIFY(!fis_zero(cam_recoil.RelaxSpeed_AI));
		if (fis_zero(cam_recoil.RelaxSpeed_AI))
		{
			cam_recoil.RelaxSpeed_AI = EPS_L;
		}
	}
	temp_f = pSettings->r_float(section, "cam_max_angle");
	cam_recoil.MaxAngleVert = _abs(deg2rad(temp_f));
	VERIFY(!fis_zero(cam_recoil.MaxAngleVert));
	if (fis_zero(cam_recoil.MaxAngleVert))
	{
		cam_recoil.MaxAngleVert = EPS;
	}

	temp_f = pSettings->r_float(section, "cam_max_angle_horz");
	cam_recoil.MaxAngleHorz = _abs(deg2rad(temp_f));
	VERIFY(!fis_zero(cam_recoil.MaxAngleHorz));
	if (fis_zero(cam_recoil.MaxAngleHorz))
	{
		cam_recoil.MaxAngleHorz = EPS;
	}

	temp_f = pSettings->r_float(section, "cam_step_angle_horz");
	cam_recoil.StepAngleHorz = deg2rad(temp_f);

	cam_recoil.DispersionFrac = _abs(READ_IF_EXISTS(pSettings, r_float, section, "cam_dispersion_frac", 0.7f));

	//подбрасывание камеры во время отдачи в режиме zoom ==> ironsight or scope
	//zoom_cam_recoil.Clone( cam_recoil ); ==== нельзя !!!!!!!!!!
	zoom_cam_recoil.RelaxSpeed = cam_recoil.RelaxSpeed;
	zoom_cam_recoil.RelaxSpeed_AI = cam_recoil.RelaxSpeed_AI;
	zoom_cam_recoil.DispersionFrac = cam_recoil.DispersionFrac;
	zoom_cam_recoil.MaxAngleVert = cam_recoil.MaxAngleVert;
	zoom_cam_recoil.MaxAngleHorz = cam_recoil.MaxAngleHorz;
	zoom_cam_recoil.StepAngleHorz = cam_recoil.StepAngleHorz;

	zoom_cam_recoil.ReturnMode = cam_recoil.ReturnMode;
	zoom_cam_recoil.StopReturn = cam_recoil.StopReturn;


	if (pSettings->line_exist(section, "zoom_cam_relax_speed"))
	{
		zoom_cam_recoil.RelaxSpeed = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_relax_speed")));
		VERIFY(!fis_zero(zoom_cam_recoil.RelaxSpeed));
		if (fis_zero(zoom_cam_recoil.RelaxSpeed))
		{
			zoom_cam_recoil.RelaxSpeed = EPS_L;
		}
	}
	if (pSettings->line_exist(section, "zoom_cam_relax_speed_ai"))
	{
		zoom_cam_recoil.RelaxSpeed_AI = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_relax_speed_ai")));
		VERIFY(!fis_zero(zoom_cam_recoil.RelaxSpeed_AI));
		if (fis_zero(zoom_cam_recoil.RelaxSpeed_AI))
		{
			zoom_cam_recoil.RelaxSpeed_AI = EPS_L;
		}
	}
	if (pSettings->line_exist(section, "zoom_cam_max_angle"))
	{
		zoom_cam_recoil.MaxAngleVert = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_max_angle")));
		VERIFY(!fis_zero(zoom_cam_recoil.MaxAngleVert));
		if (fis_zero(zoom_cam_recoil.MaxAngleVert))
		{
			zoom_cam_recoil.MaxAngleVert = EPS;
		}
	}
	if (pSettings->line_exist(section, "zoom_cam_max_angle_horz"))
	{
		zoom_cam_recoil.MaxAngleHorz = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_max_angle_horz")));
		VERIFY(!fis_zero(zoom_cam_recoil.MaxAngleHorz));
		if (fis_zero(zoom_cam_recoil.MaxAngleHorz))
		{
			zoom_cam_recoil.MaxAngleHorz = EPS;
		}
	}
	if (pSettings->line_exist(section, "zoom_cam_step_angle_horz")) {
		zoom_cam_recoil.StepAngleHorz = deg2rad(pSettings->r_float(section, "zoom_cam_step_angle_horz"));
	}
	if (pSettings->line_exist(section, "zoom_cam_dispersion_frac")) {
		zoom_cam_recoil.DispersionFrac = _abs(pSettings->r_float(section, "zoom_cam_dispersion_frac"));
	}

	m_pdm.m_fPDM_disp_base = pSettings->r_float(section, "PDM_disp_base");
	m_pdm.m_fPDM_disp_vel_factor = pSettings->r_float(section, "PDM_disp_vel_factor");
	m_pdm.m_fPDM_disp_accel_factor = pSettings->r_float(section, "PDM_disp_accel_factor");
	m_pdm.m_fPDM_disp_crouch = pSettings->r_float(section, "PDM_disp_crouch");
	m_pdm.m_fPDM_disp_crouch_no_acc = pSettings->r_float(section, "PDM_disp_crouch_no_acc");
	m_crosshair_inertion = READ_IF_EXISTS(pSettings, r_float, section, "crosshair_inertion", 5.91f);
	m_first_bullet_controller.load(section);
	fireDispersionConditionFactor = pSettings->r_float(section, "fire_dispersion_condition_factor");

	if (!CSUseNewWeaponMisfire) {
		misfireProbability = pSettings->r_float(section, "misfire_probability");
		misfireConditionK = READ_IF_EXISTS(pSettings, r_float, section, "misfire_condition_k", 1.0f);
		conditionDecreasePerShot = pSettings->r_float(section, "condition_shot_dec");
	}
	else {
		misfireStartCondition = pSettings->r_float(section, "misfire_start_condition");
		misfireEndCondition = READ_IF_EXISTS(pSettings, r_float, section, "misfire_end_condition", 0.f);
		misfireStartProbability = READ_IF_EXISTS(pSettings, r_float, section, "misfire_start_prob", 0.f);
		misfireEndProbability = pSettings->r_float(section, "misfire_end_prob");
		conditionDecreasePerShot = pSettings->r_float(section, "condition_shot_dec");
		conditionDecreasePerQueueShot = READ_IF_EXISTS(pSettings, r_float, section, "condition_queue_shot_dec", conditionDecreasePerShot);
	}

	m_bUseAimSilShotAnim = READ_IF_EXISTS(pSettings, r_bool, section, "enable_aim_silencer_shoot_anm", false);

	vLoadedFirePoint = pSettings->r_fvector3(section, "fire_point");

	if (pSettings->line_exist(section, "fire_point2"))
		vLoadedFirePoint2 = pSettings->r_fvector3(section, "fire_point2");
	else
		vLoadedFirePoint2 = vLoadedFirePoint;

	// hands
	eHandDependence = EHandDependence(pSettings->r_s32(section, "hand_dependence"));
	m_bIsSingleHanded = true;
	if (pSettings->line_exist(section, "single_handed"))
		m_bIsSingleHanded = !!pSettings->r_bool(section, "single_handed");
	// 
	m_fMinRadius = pSettings->r_float(section, "min_radius");
	m_fMaxRadius = pSettings->r_float(section, "max_radius");


	// информация о возможных апгрейдах и их визуализации в инвентаре
	m_eScopeStatus = (ALife::EWeaponAddonStatus)pSettings->r_s32(section, "scope_status");
	m_eSilencerStatus = (ALife::EWeaponAddonStatus)pSettings->r_s32(section, "silencer_status");
	m_eGrenadeLauncherStatus = (ALife::EWeaponAddonStatus)pSettings->r_s32(section, "grenade_launcher_status");

	m_zoom_params.m_bZoomEnabled = !!pSettings->r_bool(section, "zoom_enabled");
	m_zoom_params.m_fZoomRotateTime = pSettings->r_float(section, "zoom_rotate_time");

	m_zoom_params.m_bUseDynamicZoom = FALSE;
	m_zoom_params.m_sUseZoomPostprocess = 0;
	m_zoom_params.m_sUseBinocularVision = 0;

	// Added by Axel, to enable optional condition use on any item
	m_flags.set(FUsingCondition, READ_IF_EXISTS(pSettings, r_bool, section, "use_condition", TRUE));

	LoadModParams(section);

	bUseAltScope = !!bLoadAltScopesParams(section);

	if (!bUseAltScope)
		LoadOriginalScopesParams(section);

	if (m_eSilencerStatus == ALife::eAddonAttachable)
	{
		m_sSilencerName = pSettings->r_string(section, "silencer_name");
		if (UseHDIcons) {
			m_iSilencerX = pSettings->r_s32(section, "silencer_x") * 2;
			m_iSilencerY = pSettings->r_s32(section, "silencer_y") * 2;
		}
		else {
			m_iSilencerX = pSettings->r_s32(section, "silencer_x");
			m_iSilencerY = pSettings->r_s32(section, "silencer_y");
		}
	}

	if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable)
	{
		m_sGrenadeLauncherName = pSettings->r_string(section, "grenade_launcher_name");
		if (UseHDIcons) {
			m_iGrenadeLauncherX = pSettings->r_s32(section, "grenade_launcher_x") * 2;
			m_iGrenadeLauncherY = pSettings->r_s32(section, "grenade_launcher_y") * 2;
		}
		else {
			m_iGrenadeLauncherX = pSettings->r_s32(section, "grenade_launcher_x");
			m_iGrenadeLauncherY = pSettings->r_s32(section, "grenade_launcher_y");
		}
	}
	UpdateAltScope();
	InitAddons();
	if (pSettings->line_exist(section, "weapon_remove_time"))
		m_dwWeaponRemoveTime = pSettings->r_u32(section, "weapon_remove_time");
	else
		m_dwWeaponRemoveTime = WEAPON_REMOVE_TIME;

	if (pSettings->line_exist(section, "auto_spawn_ammo"))
		m_bAutoSpawnAmmo = pSettings->r_bool(section, "auto_spawn_ammo");
	else
		m_bAutoSpawnAmmo = TRUE;

	m_zoom_params.m_bHideCrosshairInZoom = true;

	if (pSettings->line_exist(hud_sect, "zoom_hide_crosshair"))
		m_zoom_params.m_bHideCrosshairInZoom = !!pSettings->r_bool(hud_sect, "zoom_hide_crosshair");

	Fvector			def_dof;
	def_dof.set(-1, -1, -1);
	//	m_zoom_params.m_ZoomDof		= READ_IF_EXISTS(pSettings, r_fvector3, section, "zoom_dof", Fvector().set(-1,-1,-1));
	//	m_zoom_params.m_bZoomDofEnabled	= !def_dof.similar(m_zoom_params.m_ZoomDof);

	//	m_zoom_params.m_ReloadDof	= READ_IF_EXISTS(pSettings, r_fvector4, section, "reload_dof", Fvector4().set(-1,-1,-1,-1));


	m_bHasTracers = !!READ_IF_EXISTS(pSettings, r_bool, section, "tracers", true);
	m_u8TracerColorID = READ_IF_EXISTS(pSettings, r_u8, section, "tracers_color_ID", u8(-1));

	string256						temp;
	for (int i = egdNovice; i < egdCount; ++i)
	{
		strconcat(sizeof(temp), temp, "hit_probability_", get_token_name(difficulty_type_token, i));
		m_hit_probability[i] = READ_IF_EXISTS(pSettings, r_float, section, temp, 1.f);
	}

}

void CWeapon::LoadFireParams(LPCSTR section)
{
	cam_recoil.Dispersion = deg2rad(pSettings->r_float(section, "cam_dispersion"));
	cam_recoil.DispersionInc = 0.0f;

	if (pSettings->line_exist(section, "cam_dispersion_inc")) {
		cam_recoil.DispersionInc = deg2rad(pSettings->r_float(section, "cam_dispersion_inc"));
	}

	zoom_cam_recoil.Dispersion = cam_recoil.Dispersion;
	zoom_cam_recoil.DispersionInc = cam_recoil.DispersionInc;

	if (pSettings->line_exist(section, "zoom_cam_dispersion")) {
		zoom_cam_recoil.Dispersion = deg2rad(pSettings->r_float(section, "zoom_cam_dispersion"));
	}
	if (pSettings->line_exist(section, "zoom_cam_dispersion_inc")) {
		zoom_cam_recoil.DispersionInc = deg2rad(pSettings->r_float(section, "zoom_cam_dispersion_inc"));
	}

	CShootingObject::LoadFireParams(section);
};

void CWeapon::LoadModParams(LPCSTR section)
{
	// Модификатор для HUD FOV от бедра
	m_hud_fov_add_mod = READ_IF_EXISTS(pSettings, r_float, section, "hud_fov_addition_modifier", 0.f);

	// Параметры изменения HUD FOV, когда игрок стоит вплотную к стене
	m_nearwall_dist_min = READ_IF_EXISTS(pSettings, r_float, section, "nearwall_dist_min", 0.5f);
	m_nearwall_dist_max = READ_IF_EXISTS(pSettings, r_float, section, "nearwall_dist_max", 1.f);
	m_nearwall_target_hud_fov = READ_IF_EXISTS(pSettings, r_float, section, "nearwall_target_hud_fov", 0.27f);
	m_nearwall_speed_mod = READ_IF_EXISTS(pSettings, r_float, section, "nearwall_speed_mod", 10.f);


	// Настройки стрейфа (боковая ходьба)
	const Fvector vZero = { 0.f, 0.f, 0.f };
	Fvector vDefStrafeValue;
	vDefStrafeValue.set(vZero);

	//--> Смещение в стрейфе
	m_strafe_offset[0][0] = READ_IF_EXISTS(pSettings, r_fvector3, section, "strafe_hud_offset_pos", vDefStrafeValue);
	m_strafe_offset[1][0] = READ_IF_EXISTS(pSettings, r_fvector3, section, "strafe_hud_offset_rot", vDefStrafeValue);

	//--> Поворот в стрейфе
	m_strafe_offset[0][1] = READ_IF_EXISTS(pSettings, r_fvector3, section, "strafe_aim_hud_offset_pos", vDefStrafeValue);
	m_strafe_offset[1][1] = READ_IF_EXISTS(pSettings, r_fvector3, section, "strafe_aim_hud_offset_rot", vDefStrafeValue);

	// Параметры стрейфа
	bool  bStrafeEnabled = READ_IF_EXISTS(pSettings, r_bool, section, "strafe_enabled", false);
	bool  bStrafeEnabled_aim = READ_IF_EXISTS(pSettings, r_bool, section, "strafe_aim_enabled", false);
	float fFullStrafeTime = READ_IF_EXISTS(pSettings, r_float, section, "strafe_transition_time", 0.01f);
	float fFullStrafeTime_aim = READ_IF_EXISTS(pSettings, r_float, section, "strafe_aim_transition_time", 0.01f);
	float fStrafeCamLFactor = READ_IF_EXISTS(pSettings, r_float, section, "strafe_cam_limit_factor", 0.5f);
	float fStrafeCamLFactor_aim = READ_IF_EXISTS(pSettings, r_float, section, "strafe_cam_limit_aim_factor", 1.0f);
	float fStrafeMinAngle = READ_IF_EXISTS(pSettings, r_float, section, "strafe_cam_min_angle", 0.0f);
	float fStrafeMinAngle_aim = READ_IF_EXISTS(pSettings, r_float, section, "strafe_cam_aim_min_angle", 7.0f);

	//--> (Data 1)
	m_strafe_offset[2][0].set((bStrafeEnabled ? 1.0f : 0.0f), fFullStrafeTime, NULL);         // normal
	m_strafe_offset[2][1].set((bStrafeEnabled_aim ? 1.0f : 0.0f), fFullStrafeTime_aim, NULL); // aim-GL

	//--> (Data 2)
	m_strafe_offset[3][0].set(fStrafeCamLFactor, fStrafeMinAngle, NULL); // normal
	m_strafe_offset[3][1].set(fStrafeCamLFactor_aim, fStrafeMinAngle_aim, NULL); // aim-GL
}

bool CWeapon::bReloadSectionScope(LPCSTR section)
{
	if (!pSettings->line_exist(section, "scopes"))
		return false;

	if (pSettings->r_string(section, "scopes") == NULL)
		return false;

	if (xr_strcmp(pSettings->r_string(section, "scopes"), "none") == 0)
		return false;

	return true;
}

bool CWeapon::bLoadAltScopesParams(LPCSTR section)
{
	if (!pSettings->line_exist(section, "scopes"))
		return false;

	if (pSettings->r_string(section, "scopes") == NULL)
		return false;

	if (xr_strcmp(pSettings->r_string(section, "scopes"), "none") == 0)
		return false;

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

	return true;
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

	m_zoom_params.m_fScopeZoomFactor = pSettings->r_float(section, "scope_zoom_factor");

	if (bScopeIsHasTexture || bIsSecondVPZoomPresent())
	{
		if (bIsSecondVPZoomPresent())
			bNVsecondVPavaible = !!pSettings->line_exist(section, "scope_nightvision");

		m_zoom_params.m_sUseZoomPostprocess = READ_IF_EXISTS(pSettings, r_string, section, "scope_nightvision", 0);
		m_zoom_params.m_bUseDynamicZoom = READ_IF_EXISTS(pSettings, r_bool, section, "scope_dynamic_zoom", FALSE);

		if (m_zoom_params.m_bUseDynamicZoom)
		{
			m_fZoomStepCount = READ_IF_EXISTS(pSettings, r_u8, section, "scope_zoom_steps", 3.0f);
			m_fZoomMinKoeff = READ_IF_EXISTS(pSettings, r_float, section, "min_zoom_k", 0.3f);
		}

		m_zoom_params.m_sUseBinocularVision = READ_IF_EXISTS(pSettings, r_string, section, "scope_alive_detector", 0);
	}
	else
	{
		bNVsecondVPavaible = false;
		bNVsecondVPstatus = false;
	}

	m_fScopeInertionFactor = READ_IF_EXISTS(pSettings, r_float, section, "scope_inertion_factor", m_fControlInertionFactor);

	m_fRTZoomFactor = m_zoom_params.m_fScopeZoomFactor;

	if (m_UIScope)
	{
		xr_delete(m_UIScope);
	}

	if (bScopeIsHasTexture)
	{
		m_UIScope = xr_new<CUIWindow>();
		createWpnScopeXML();
		CUIXmlInit::InitWindow(*pWpnScopeXml, scope_tex_name.c_str(), 0, m_UIScope);
	}
}

void CWeapon::Load3DScopeParams(LPCSTR section)
{
	m_zoom_params.m_fSecondVPFovFactor = READ_IF_EXISTS(pSettings, r_float, section, "3d_fov", 0.0f);
	m_zoom_params.m_f3dZoomFactor = READ_IF_EXISTS(pSettings, r_float, section, "3d_zoom_factor", 100.0f);
}

BOOL CWeapon::net_Spawn(CSE_Abstract* DC)
{
	m_fRTZoomFactor = m_zoom_params.m_fScopeZoomFactor;
	BOOL bResult = inherited::net_Spawn(DC);
	CSE_Abstract* e = (CSE_Abstract*)(DC);
	CSE_ALifeItemWeapon* E = smart_cast<CSE_ALifeItemWeapon*>(e);

	//iAmmoCurrent					= E->a_current;
	iAmmoElapsed = E->a_elapsed;
	m_flagsAddOnState = E->m_addon_flags.get();
	m_ammoType = E->ammo_type;
	if (E->cur_scope < m_scopes.size() && m_scopes.size()>1)
		m_cur_scope = E->cur_scope;
	SetState(E->wpn_state);
	SetNextState(E->wpn_state);

	m_DefaultCartridge.Load(m_ammoTypes[m_ammoType].c_str(), m_ammoType);
	if (iAmmoElapsed)
	{
		m_fCurrentCartirdgeDisp = m_DefaultCartridge.param_s.kDisp;
		for (int i = 0; i < iAmmoElapsed; ++i)
			m_magazine.push_back(m_DefaultCartridge);
	}

	UpdateAltScope();
	UpdateAddonsVisibility();
	InitAddons();

	m_dwWeaponIndependencyTime = 0;

	VERIFY((u32)iAmmoElapsed == m_magazine.size());
	m_bAmmoWasSpawned = false;


	return bResult;
}

void CWeapon::net_Destroy()
{
	inherited::net_Destroy();

	//удалить объекты партиклов
	StopFlameParticles();
	StopFlameParticles2();
	StopLight();
	Light_Destroy();

	while (m_magazine.size()) m_magazine.pop_back();
}

BOOL CWeapon::IsUpdating()
{
	bool bIsActiveItem = m_pInventory && m_pInventory->ActiveItem() == this;
	return bIsActiveItem || bWorking;// || IsPending() || getVisible();
}

void CWeapon::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);

	P.w_float_q8(GetCondition(), 0.0f, 1.0f);


	u8 need_upd = IsUpdating() ? 1 : 0;
	P.w_u8(need_upd);
	P.w_u16(u16(iAmmoElapsed));
	P.w_u8(m_flagsAddOnState);
	P.w_u8(m_ammoType);
	P.w_u8((u8)GetState());
	P.w_u8((u8)IsZoomed());
	P.w_u8((u8)m_cur_scope);
}

void CWeapon::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);

	float _cond;
	P.r_float_q8(_cond, 0.0f, 1.0f);
	SetCondition(_cond);

	u8 flags = 0;
	P.r_u8(flags);

	u16 ammo_elapsed = 0;
	P.r_u16(ammo_elapsed);

	u8						NewAddonState;
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

void CWeapon::save(NET_Packet& output_packet)
{
	inherited::save(output_packet);
	save_data(iAmmoElapsed, output_packet);
	save_data(m_cur_scope, output_packet);
	save_data(m_flagsAddOnState, output_packet);
	save_data(m_ammoType, output_packet);
	save_data(m_zoom_params.m_bIsZoomModeNow, output_packet);
	save_data(m_bRememberActorNVisnStatus, output_packet);
	save_data(bNVsecondVPstatus, output_packet);
	save_data(m_fSecondRTZoomFactor, output_packet);
}

void CWeapon::load(IReader& input_packet)
{
	inherited::load(input_packet);
	load_data(iAmmoElapsed, input_packet);
	load_data(m_cur_scope, input_packet);
	load_data(m_flagsAddOnState, input_packet);
	UpdateAddonsVisibility();
	load_data(m_ammoType, input_packet);
	load_data(m_zoom_params.m_bIsZoomModeNow, input_packet);

	if (m_zoom_params.m_bIsZoomModeNow)
		OnZoomIn();
	else
		OnZoomOut();

	load_data(m_bRememberActorNVisnStatus, input_packet);
	load_data(bNVsecondVPstatus, input_packet);
	load_data(m_fSecondRTZoomFactor, input_packet);
}


void CWeapon::OnEvent(NET_Packet& P, u16 type)
{
	switch (type)
	{
	case GE_WPN_STATE_CHANGE:
	{
		u8				state;
		P.r_u8(state);
		P.r_u8(m_sub_state);
		//			u8 NewAmmoType = 
		P.r_u8();
		P.r_u8();
		u8 NextAmmo = P.r_u8();
		if (NextAmmo == undefined_ammo_type)
			m_set_next_ammoType_on_reload = undefined_ammo_type;
		else
			m_set_next_ammoType_on_reload = NextAmmo;

		OnStateSwitch(u32(state));
	}
	break;
	default:
	{
		inherited::OnEvent(P, type);
	}break;
	}
};

void CWeapon::shedule_Update(u32 dT)
{
	// Queue shrink
//	u32	dwTimeCL		= Level().timeServer()-NET_Latency;
//	while ((NET.size()>2) && (NET[1].dwTimeStamp<dwTimeCL)) NET.pop_front();	

	// Inherited
	inherited::shedule_Update(dT);
}

void CWeapon::OnH_B_Independent(bool just_before_destroy)
{
	RemoveShotEffector();

	inherited::OnH_B_Independent(just_before_destroy);

	FireEnd();
	SetPending(FALSE);
	SwitchState(eHidden);

	m_strapped_mode = false;
	m_zoom_params.m_bIsZoomModeNow = false;
	UpdateXForm();
	m_nearwall_last_hud_fov = psHUD_FOV_def;
}

void CWeapon::OnH_A_Independent()
{
	m_dwWeaponIndependencyTime = Level().timeServer();

	m_fLR_MovingFactor = 0.f;
	m_fLR_CameraFactor = 0.f;
	m_fLR_InertiaFactor = 0.f;
	m_fUD_InertiaFactor = 0.f;
	inherited::OnH_A_Independent();
	Light_Destroy();
	UpdateAddonsVisibility();
};

void CWeapon::OnH_A_Chield()
{
	inherited::OnH_A_Chield();
	UpdateAddonsVisibility();
};

void CWeapon::OnActiveItem()
{
	//. from Activate
	UpdateAddonsVisibility();
	m_BriefInfo_CalcFrame = 0;

	//. Show
	SwitchState(eShowing);
	//-

	inherited::OnActiveItem();
	//если мы занружаемся и оружие было в руках
//.	SetState					(eIdle);
//.	SetNextState				(eIdle);
}

void CWeapon::OnHiddenItem()
{
	m_BriefInfo_CalcFrame = 0;

	if (IsGameTypeSingle())
		SwitchState(eHiding);
	else
		SwitchState(eHidden);

	OnZoomOut();
	inherited::OnHiddenItem();

	m_set_next_ammoType_on_reload = undefined_ammo_type;
}

void CWeapon::SendHiddenItem()
{
	if (!CHudItem::object().getDestroy() && m_pInventory)
	{
		// !!! Just single entry for given state !!!
		NET_Packet		P;
		CHudItem::object().u_EventGen(P, GE_WPN_STATE_CHANGE, CHudItem::object().ID());
		P.w_u8(u8(eHiding));
		P.w_u8(u8(m_sub_state));
		P.w_u8(m_ammoType);
		P.w_u8(u8(iAmmoElapsed & 0xff));
		P.w_u8(m_set_next_ammoType_on_reload);
		CHudItem::object().u_EventSend(P, net_flags(TRUE, TRUE, FALSE, TRUE));
		SetPending(TRUE);
	}
}


void CWeapon::OnH_B_Chield()
{
	m_dwWeaponIndependencyTime = 0;
	inherited::OnH_B_Chield();

	OnZoomOut();
	m_set_next_ammoType_on_reload = undefined_ammo_type;
	m_nearwall_last_hud_fov = psHUD_FOV_def;
}

extern u32 hud_adj_mode;
bool CWeapon::AllowBore()
{
	return true;
}

void CWeapon::UpdateCL()
{
	inherited::UpdateCL();
	UpdateHUDAddonsVisibility();
	//подсветка от выстрела
	UpdateLight();

	//нарисовать партиклы
	UpdateFlameParticles();
	UpdateFlameParticles2();

	if (!IsGameTypeSingle())
		make_Interpolation();

	if ((GetNextState() == GetState()) && IsGameTypeSingle() && H_Parent() == Level().CurrentEntity())
	{
		CActor* pActor = smart_cast<CActor*>(H_Parent());
		if (pActor && !pActor->AnyMove() && this == pActor->inventory().ActiveItem())
		{
			if (hud_adj_mode == 0 &&
				GetState() == eIdle &&
				(Device.dwTimeGlobal - m_dw_curr_substate_time > 20000) &&
				!IsZoomed() &&
				g_player_hud->attached_item(1) == NULL)
			{
				if (AllowBore())
					SwitchState(eBore);

				ResetSubStateTime();
			}
		}
	}

	if (m_zoom_params.m_pNight_vision && !need_renderable())
	{
		if (!m_zoom_params.m_pNight_vision->IsActive())
		{
			CActor* pA = smart_cast<CActor*>(H_Parent());
			R_ASSERT(pA);
			CTorch* pTorch = smart_cast<CTorch*>(pA->inventory().ItemFromSlot(TORCH_SLOT));
			if (pTorch && pTorch->GetNightVisionStatus())
			{
				m_bRememberActorNVisnStatus = pTorch->GetNightVisionStatus();
				pTorch->SwitchNightVision(false, false);
			}
			m_zoom_params.m_pNight_vision->Start(m_zoom_params.m_sUseZoomPostprocess, pA, false);
		}

	}
	else if (m_bRememberActorNVisnStatus)
	{
		m_bRememberActorNVisnStatus = false;
		EnableActorNVisnAfterZoom();
	}

	if (m_zoom_params.m_pVision)
		m_zoom_params.m_pVision->Update();
}
void CWeapon::EnableActorNVisnAfterZoom()
{
	CActor* pA = smart_cast<CActor*>(H_Parent());
	if (IsGameTypeSingle() && !pA)
		pA = g_actor;

	if (pA)
	{
		CTorch* pTorch = smart_cast<CTorch*>(pA->inventory().ItemFromSlot(TORCH_SLOT));
		if (pTorch)
		{
			pTorch->SwitchNightVision(true, false);
			pTorch->GetNightVision()->PlaySounds(CNightVisionEffector::eIdleSound);
		}
	}
}

bool CWeapon::need_renderable()
{
	return Render->currentViewPort == MAIN_VIEWPORT && !(IsZoomed() && ZoomTexture() && !IsRotatingToZoom());
}

void CWeapon::renderable_Render()
{
	UpdateXForm();

	//нарисовать подсветку

	RenderLight();

	//если мы в режиме снайперки, то сам HUD рисовать не надо
	if (IsZoomed() && !IsRotatingToZoom() && ZoomTexture())
		RenderHud(FALSE);
	else
		RenderHud(TRUE);

	inherited::renderable_Render();
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
	SetPending(FALSE);

	m_flags.set(FUsingCondition, TRUE);
	bMisfire = false;
	m_flagsAddOnState = 0;
	m_zoom_params.m_bIsZoomModeNow = false;
}

void CWeapon::UpdatePosition(const Fmatrix& trans)
{
	Position().set(trans.c);
	XFORM().mul(trans, m_strapped_mode ? m_StrapOffset : m_Offset);
	VERIFY(!fis_zero(DET(renderable.xform)));
}


bool CWeapon::Action(u16 cmd, u32 flags)
{
	if (inherited::Action(cmd, flags)) return true;


	switch (cmd)
	{
	case kWPN_NV_CHANGE:
	{
		return bChangeNVSecondVPStatus();
	}
	case kWPN_FIRE:
	{
		//если оружие чем-то занято, то ничего не делать
		{
			if (IsPending())
				return				false;

			if (flags & CMD_START)
				FireStart();
			else
				FireEnd();
		};
	}
	return true;
	case kWPN_NEXT:
	{
		return SwitchAmmoType(flags);
	}

	case kWPN_ZOOM:
		if (IsZoomEnabled())
		{
			if (b_toggle_weapon_aim)
			{
				if (flags & CMD_START)
				{
					if (!IsZoomed())
					{
						if (!IsPending())
						{
							if (GetState() != eIdle)
								SwitchState(eIdle);
							OnZoomIn();
						}
					}
					else
						OnZoomOut();
				}
			}
			else
			{
				if (flags & CMD_START)
				{
					if (!IsZoomed() && !IsPending())
					{
						if (GetState() != eIdle)
							SwitchState(eIdle);
						OnZoomIn();
					}
				}
				else
					if (IsZoomed())
						OnZoomOut();
			}
			return true;
		}
		else
			return false;

	case kWPN_ZOOM_INC:
	case kWPN_ZOOM_DEC:
		if (IsZoomEnabled() && IsZoomed())
		{
			if (cmd == kWPN_ZOOM_INC)  ZoomInc();
			else					ZoomDec();
			return true;
		}
		else
			return false;
	}
	return false;
}

bool CWeapon::SwitchAmmoType(u32 flags)
{
	if (IsPending())
		return false;

	if (!(flags & CMD_START))
		return false;

	u8 l_newType = m_ammoType;
	bool b1, b2;
	do
	{
		l_newType = u8((u32(l_newType + 1)) % m_ammoTypes.size());
		b1 = (l_newType != m_ammoType);
		b2 = unlimited_ammo() ? false : (!m_pInventory->GetAny(m_ammoTypes[l_newType].c_str()));
	} while (b1 && b2);

	if (l_newType != m_ammoType)
	{
		m_set_next_ammoType_on_reload = l_newType;
		Reload();
	}
	return true;
}

void CWeapon::SpawnAmmo(u32 boxCurr, LPCSTR ammoSect, u32 ParentID)
{
	if (!m_ammoTypes.size())			return;
	m_bAmmoWasSpawned = true;

	int l_type = 0;
	l_type %= m_ammoTypes.size();

	if (!ammoSect) ammoSect = m_ammoTypes[l_type].c_str();

	++l_type;
	l_type %= m_ammoTypes.size();

	CSE_Abstract* D = F_entity_Create(ammoSect);

	{
		CSE_ALifeItemAmmo* l_pA = smart_cast<CSE_ALifeItemAmmo*>(D);
		R_ASSERT(l_pA);
		l_pA->m_boxSize = (u16)pSettings->r_s32(ammoSect, "box_size");
		D->s_name = ammoSect;
		D->set_name_replace("");
		//.		D->s_gameid					= u8(GameID());
		D->s_RP = 0xff;
		D->ID = 0xffff;
		if (ParentID == 0xffffffff)
			D->ID_Parent = (u16)H_Parent()->ID();
		else
			D->ID_Parent = (u16)ParentID;

		D->ID_Phantom = 0xffff;
		D->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
		D->RespawnTime = 0;
		l_pA->m_tNodeID = g_dedicated_server ? u32(-1) : ai_location().level_vertex_id();

		if (boxCurr == 0xffffffff)
			boxCurr = l_pA->m_boxSize;

		while (boxCurr)
		{
			l_pA->a_elapsed = (u16)(boxCurr > l_pA->m_boxSize ? l_pA->m_boxSize : boxCurr);
			NET_Packet				P;
			D->Spawn_Write(P, TRUE);
			Level().Send(P, net_flags(TRUE));

			if (boxCurr > l_pA->m_boxSize)
				boxCurr -= l_pA->m_boxSize;
			else
				boxCurr = 0;
		}
	}
	F_entity_Destroy(D);
}

int CWeapon::GetSuitableAmmoTotal(bool use_item_to_spawn) const
{
	int ae_count = iAmmoElapsed;
	if (!m_pInventory)
	{
		return ae_count;
	}

	//чтоб не делать лишних пересчетов
	if (m_pInventory->ModifyFrame() <= m_BriefInfo_CalcFrame)
	{
		return ae_count + m_iAmmoCurrentTotal;
	}
	m_BriefInfo_CalcFrame = Device.dwFrame;

	m_iAmmoCurrentTotal = 0;
	for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
	{
		m_iAmmoCurrentTotal += GetAmmoCount_forType(m_ammoTypes[i]);

		if (!use_item_to_spawn)
		{
			continue;
		}
		if (!inventory_owner().item_to_spawn())
		{
			continue;
		}
		m_iAmmoCurrentTotal += inventory_owner().ammo_in_box_to_spawn();
	}
	return ae_count + m_iAmmoCurrentTotal;
}

int CWeapon::GetAmmoCount(u8 ammo_type) const
{
	VERIFY(m_pInventory);
	R_ASSERT(ammo_type < m_ammoTypes.size());

	return GetAmmoCount_forType(m_ammoTypes[ammo_type]);
}

int CWeapon::GetAmmoCount_forType(shared_str const& ammo_type) const
{
	int res = 0;

	TIItemContainer::iterator itb = m_pInventory->m_belt.begin();
	TIItemContainer::iterator ite = m_pInventory->m_belt.end();
	for (; itb != ite; ++itb)
	{
		CWeaponAmmo* pAmmo = smart_cast<CWeaponAmmo*>(*itb);
		if (pAmmo && (pAmmo->cNameSect() == ammo_type))
		{
			res += pAmmo->m_boxCurr;
		}
	}

	itb = m_pInventory->m_ruck.begin();
	ite = m_pInventory->m_ruck.end();
	for (; itb != ite; ++itb)
	{
		CWeaponAmmo* pAmmo = smart_cast<CWeaponAmmo*>(*itb);
		if (pAmmo && (pAmmo->cNameSect() == ammo_type))
		{
			res += pAmmo->m_boxCurr;
		}
	}
	return res;
}

float CWeapon::GetConditionMisfireProbability() const
{
	// modified by Peacemaker [17.10.08]
	if (!CSUseNewWeaponMisfire) 
	{
		if (GetCondition() > 0.95f)
			return 0.0f;
	}
	else {
		if (GetCondition() > misfireStartCondition)
			return 0.0f;
		if (GetCondition() < misfireEndCondition)
			return misfireEndProbability;
	}
	float mis;
	if(!CSUseNewWeaponMisfire) 
		mis = misfireProbability+powf(1.f-GetCondition(), 3.f)*misfireConditionK;
	else {
		mis = misfireStartProbability + (
			(misfireStartCondition - GetCondition()) *				// condition goes from 1.f to 0.f
			(misfireEndProbability - misfireStartProbability) /		// probability goes from 0.f to 1.f
			((misfireStartCondition == misfireEndCondition) ?		// !!!say "No" to devision by zero
				misfireStartCondition :
				(misfireStartCondition - misfireEndCondition))
			);
	}
	clamp(mis, 0.0f, 0.99f);
	return mis;
}

BOOL CWeapon::CheckForMisfire()
{
	float rnd = ::Random.randF(0.f, 1.f);
	float mp = GetConditionMisfireProbability();
	if (rnd < mp)
	{
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
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher)) ||
		ALife::eAddonPermanent == m_eGrenadeLauncherStatus;
}

bool CWeapon::IsScopeAttached() const
{
	return (ALife::eAddonAttachable == m_eScopeStatus &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope)) ||
		ALife::eAddonPermanent == m_eScopeStatus;

}

bool CWeapon::IsSilencerAttached() const
{
	return (ALife::eAddonAttachable == m_eSilencerStatus &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer)) ||
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

shared_str wpn_scope = "wpn_scope";
shared_str wpn_silencer = "wpn_silencer";
shared_str wpn_grenade_launcher = "wpn_launcher";

void CWeapon::UpdateHUDAddonsVisibility()
{//actor only
	if (!GetHUDmode())										return;

	//.	return;

	u16 bone_id = HudItemData()->m_model->LL_BoneID(wpn_scope);

	if (bone_id != BI_NONE)
	{
		if (ScopeAttachable())
		{
			HudItemData()->set_bone_visible(wpn_scope, IsScopeAttached());
		}

		if (m_eScopeStatus == ALife::eAddonDisabled)
		{
			HudItemData()->set_bone_visible(wpn_scope, FALSE, TRUE);
		}
		else
			if (m_eScopeStatus == ALife::eAddonPermanent)
				HudItemData()->set_bone_visible(wpn_scope, TRUE, TRUE);
	}
	if (SilencerAttachable())
	{
		HudItemData()->set_bone_visible(wpn_silencer, IsSilencerAttached());
	}
	if (m_eSilencerStatus == ALife::eAddonDisabled)
	{
		HudItemData()->set_bone_visible(wpn_silencer, FALSE, TRUE);
	}
	else
		if (m_eSilencerStatus == ALife::eAddonPermanent)
			HudItemData()->set_bone_visible(wpn_silencer, TRUE, TRUE);

	if (GrenadeLauncherAttachable())
	{
		HudItemData()->set_bone_visible(wpn_grenade_launcher, IsGrenadeLauncherAttached());
	}
	if (m_eGrenadeLauncherStatus == ALife::eAddonDisabled)
	{
		HudItemData()->set_bone_visible(wpn_grenade_launcher, FALSE, TRUE);
	}
	else
		if (m_eGrenadeLauncherStatus == ALife::eAddonPermanent)
			HudItemData()->set_bone_visible(wpn_grenade_launcher, TRUE, TRUE);

}
void CWeapon::HUD_VisualBulletUpdate(bool force, int force_idx)
{
	if (!bHasBulletsToHide)
		return;

	if (!GetHUDmode())	return;

	bool hide = true;

	if (last_hide_bullet == bullet_cnt || force) hide = false;

	for (u8 b = 0; b < bullet_cnt; b++)
	{
		u16 bone_id = HudItemData()->m_model->LL_BoneID(bullets_bones[b]);

		if (bone_id != BI_NONE)
			HudItemData()->set_bone_visible(bullets_bones[b], !hide);

		if (b == last_hide_bullet) hide = false;
	}
}

void CWeapon::UpdateAddonsVisibility()
{
	IKinematics* pWeaponVisual = smart_cast<IKinematics*>(Visual()); R_ASSERT(pWeaponVisual);

	u16  bone_id;
	UpdateHUDAddonsVisibility();

	pWeaponVisual->CalculateBones_Invalidate();

	bone_id = pWeaponVisual->LL_BoneID(wpn_scope);
	if (ScopeAttachable())
	{
		if (IsScopeAttached())
		{
			if (!pWeaponVisual->LL_GetBoneVisible(bone_id) && bone_id != BI_NONE)
				pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else {
			if (pWeaponVisual->LL_GetBoneVisible(bone_id) && bone_id != BI_NONE)
				pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		}
	}
	if (m_eScopeStatus == ALife::eAddonDisabled && bone_id != BI_NONE &&
		pWeaponVisual->LL_GetBoneVisible(bone_id))
	{
		pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		//		Log("scope", pWeaponVisual->LL_GetBoneVisible		(bone_id));
	}
	bone_id = pWeaponVisual->LL_BoneID(wpn_silencer);
	if (SilencerAttachable())
	{
		if (IsSilencerAttached()) {
			if (!pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else {
			if (pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		}
	}
	if (m_eSilencerStatus == ALife::eAddonDisabled && bone_id != BI_NONE &&
		pWeaponVisual->LL_GetBoneVisible(bone_id))
	{
		pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		//		Log("silencer", pWeaponVisual->LL_GetBoneVisible	(bone_id));
	}

	bone_id = pWeaponVisual->LL_BoneID(wpn_grenade_launcher);
	if (GrenadeLauncherAttachable())
	{
		if (IsGrenadeLauncherAttached())
		{
			if (!pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else {
			if (pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		}
	}
	if (m_eGrenadeLauncherStatus == ALife::eAddonDisabled && bone_id != BI_NONE &&
		pWeaponVisual->LL_GetBoneVisible(bone_id))
	{
		pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		//		Log("gl", pWeaponVisual->LL_GetBoneVisible			(bone_id));
	}


	pWeaponVisual->CalculateBones_Invalidate();
	pWeaponVisual->CalculateBones(TRUE);
}

void CWeapon::InitAddons()
{
}

float CWeapon::CurrentZoomFactor()
{
	if (psActorFlags.test(AF_3DSCOPE_ENABLE) && IsScopeAttached())
	{
		return bIsSecondVPZoomPresent() ? m_zoom_params.m_f3dZoomFactor : m_zoom_params.m_fScopeZoomFactor;
	}
	else
	{
		return IsScopeAttached() ? m_zoom_params.m_fScopeZoomFactor : m_zoom_params.m_fIronSightZoomFactor;
	}
};

// Чувствительность мышкии с оружием в руках
float CWeapon::GetControlInertionFactor() const
{
	float fInertionFactor = inherited::GetControlInertionFactor();
	if (IsScopeAttached() && IsZoomed())
	{
		if (bIsSecondVPZoomPresent() && psActorFlags.test(AF_3DSCOPE_ENABLE))
		{
			if (GetSecondVPFov() < 15.f)
			{
				return 9.5f;
			}
			else if (GetSecondVPFov() < 20.f)
			{
				return 7.5f;
			}
			else if (GetSecondVPFov() < 35.f)
			{
				return 5.5f;
			}
			else if (GetSecondVPFov() < 55.f)
			{
				return 2.5f;
			}
			else
			{
				return m_fScopeInertionFactor;
			}
		}
		else
		{
			return m_fScopeInertionFactor;//m_fScopeInertionFactor;
		}
	}


	return fInertionFactor;
}

void CWeapon::GetZoomData(const float scope_factor, float& delta, float& min_zoom_factor)
{
	float def_fov = bIsSecondVPZoomPresent() ? 75.0f : g_fov;//float(g_fov);
	float delta_factor_total = def_fov - scope_factor;
	VERIFY(delta_factor_total > 0);
	min_zoom_factor = def_fov - delta_factor_total * m_fZoomMinKoeff;
	delta = (delta_factor_total * (1 - m_fZoomMinKoeff)) / m_fZoomStepCount;
}

void CWeapon::OnZoomIn()
{
	m_zoom_params.m_bIsZoomModeNow = true;
	psActorFlags.set(AF_ZOOM_NEW_FD, true);

	if (m_fSecondRTZoomFactor == -1 && bIsSecondVPZoomPresent())
		ZoomDynamicMod(true, true);

	if (!m_zoom_params.m_bUseDynamicZoom)
		SetZoomFactor(CurrentZoomFactor());
	else
		SetZoomFactor(psActorFlags.test(AF_3DSCOPE_ENABLE) && bIsSecondVPZoomPresent() ? m_zoom_params.m_f3dZoomFactor : m_fRTZoomFactor);

	// Отключаем инерцию (Заменено GetInertionFactor())
	// EnableHudInertion(FALSE);

	//if(m_zoom_params.m_bZoomDofEnabled && !IsScopeAttached())
	//	GamePersistent().SetEffectorDOF	(m_zoom_params.m_ZoomDof);

	if (GetHUDmode())
		GamePersistent().SetPickableEffectorDOF(true);

	if (m_zoom_params.m_sUseBinocularVision.size() && IsScopeAttached() && NULL == m_zoom_params.m_pVision)
		m_zoom_params.m_pVision = xr_new<CBinocularsVision>(m_zoom_params.m_sUseBinocularVision/*"wpn_binoc"*/);

	CActor* pA = smart_cast<CActor*>(H_Parent());

	if (pA && IsScopeAttached())
	{
		if (psActorFlags.test(AF_PNV_W_SCOPE_DIS) && UseScopeTexture())
		{
			CTorch* pTorch = smart_cast<CTorch*>(pA->inventory().ItemFromSlot(TORCH_SLOT));
			if (pTorch && pTorch->GetNightVisionStatus())
			{
				OnZoomOut();
			}
		}
		else if (m_zoom_params.m_sUseZoomPostprocess.size() && !psActorFlags.test(AF_3DSCOPE_ENABLE))
		{
			if (NULL == m_zoom_params.m_pNight_vision)
			{
				m_zoom_params.m_pNight_vision = xr_new<CNightVisionEffector>(m_zoom_params.m_sUseZoomPostprocess/*"device_torch"*/);
			}
		}
	}
}

void CWeapon::OnZoomOut()
{
	if (!bIsSecondVPZoomPresent() && psActorFlags.test(AF_3DSCOPE_ENABLE))
		m_fRTZoomFactor = GetZoomFactor(); // Сохраняем текущий динамический зум
	m_zoom_params.m_bIsZoomModeNow = false;
	SetZoomFactor(g_fov);
	psActorFlags.set(AF_ZOOM_NEW_FD, false);
	// Включаем инерцию (также заменено  GetInertionFactor())
	// EnableHudInertion	(TRUE);


// 	GamePersistent().RestoreEffectorDOF	();

	if (GetHUDmode())
		GamePersistent().SetPickableEffectorDOF(false);

	ResetSubStateTime();

	xr_delete(m_zoom_params.m_pVision);
	if (m_zoom_params.m_pNight_vision)
	{
		m_zoom_params.m_pNight_vision->Stop(100000.0f, false);
		xr_delete(m_zoom_params.m_pNight_vision);
	}
}

CUIWindow* CWeapon::ZoomTexture()
{
	if (UseScopeTexture() && (!psActorFlags.test(AF_3DSCOPE_ENABLE) || !bIsSecondVPZoomPresent()))
		return m_UIScope;
	else
		return NULL;
}

void CWeapon::SwitchState(u32 S)
{
#ifndef MASTER_GOLD
	if (bDebug)
	{
		Msg("---Server is going to send GE_WPN_STATE_CHANGE to [%d], weapon_section[%s], parent[%s]",
			S, cNameSect().c_str(), H_Parent() ? H_Parent()->cName().c_str() : "NULL Parent");
	}
#endif // #ifndef MASTER_GOLD

	SetNextState(S);
	if (CHudItem::object().Local() && !CHudItem::object().getDestroy() && m_pInventory)
	{
		// !!! Just single entry for given state !!!
		NET_Packet		P;
		CHudItem::object().u_EventGen(P, GE_WPN_STATE_CHANGE, CHudItem::object().ID());
		P.w_u8(u8(S));
		P.w_u8(u8(m_sub_state));
		P.w_u8(m_ammoType);
		P.w_u8(u8(iAmmoElapsed & 0xff));
		P.w_u8(m_set_next_ammoType_on_reload);
		CHudItem::object().u_EventSend(P, net_flags(TRUE, TRUE, FALSE, TRUE));
	}
}

void CWeapon::OnMagazineEmpty()
{
	VERIFY((u32)iAmmoElapsed == m_magazine.size());
}

void CWeapon::reinit()
{
	CShootingObject::reinit();
	CHudItemObject::reinit();
}

void CWeapon::reload(LPCSTR section)
{
	CShootingObject::reload(section);
	CHudItemObject::reload(section);

	m_can_be_strapped = true;
	m_strapped_mode = false;

	if (pSettings->line_exist(section, "strap_bone0"))
		m_strap_bone0 = pSettings->r_string(section, "strap_bone0");
	else
		m_can_be_strapped = false;

	if (pSettings->line_exist(section, "strap_bone1"))
		m_strap_bone1 = pSettings->r_string(section, "strap_bone1");
	else
		m_can_be_strapped = false;

	bUseAltScope = !!bReloadSectionScope(section);

	if (m_eScopeStatus == ALife::eAddonAttachable) {
		m_addon_holder_range_modifier = READ_IF_EXISTS(pSettings, r_float, GetScopeName(), "holder_range_modifier", m_holder_range_modifier);
		m_addon_holder_fov_modifier = READ_IF_EXISTS(pSettings, r_float, GetScopeName(), "holder_fov_modifier", m_holder_fov_modifier);
	}
	else {
		m_addon_holder_range_modifier = m_holder_range_modifier;
		m_addon_holder_fov_modifier = m_holder_fov_modifier;
	}


	{
		Fvector				pos, ypr;
		pos = pSettings->r_fvector3(section, "position");
		ypr = pSettings->r_fvector3(section, "orientation");
		ypr.mul(PI / 180.f);

		m_Offset.setHPB(ypr.x, ypr.y, ypr.z);
		m_Offset.translate_over(pos);
	}

	m_StrapOffset = m_Offset;
	if (pSettings->line_exist(section, "strap_position") && pSettings->line_exist(section, "strap_orientation")) {
		Fvector				pos, ypr;
		pos = pSettings->r_fvector3(section, "strap_position");
		ypr = pSettings->r_fvector3(section, "strap_orientation");
		ypr.mul(PI / 180.f);

		m_StrapOffset.setHPB(ypr.x, ypr.y, ypr.z);
		m_StrapOffset.translate_over(pos);
	}
	else
		m_can_be_strapped = false;

	m_ef_main_weapon_type = READ_IF_EXISTS(pSettings, r_u32, section, "ef_main_weapon_type", static_cast<u32>(-1));
	m_ef_weapon_type = READ_IF_EXISTS(pSettings, r_u32, section, "ef_weapon_type", static_cast<u32>(-1));
	m_ai_weapon_rank = READ_IF_EXISTS(pSettings, r_u32, section, "weapon_rank", static_cast<u32>(-1));

	if (m_ai_weapon_rank == static_cast<u32>(-1))
	{
#ifdef DEBUG
		Msg("# Weapon rank not set for weapon: [%s]. Using default set rank(0)", section);
#endif
		m_ai_weapon_rank = 0;
	}
}

void CWeapon::create_physic_shell()
{
	CPhysicsShellHolder::create_physic_shell();
}

bool CWeapon::ActivationSpeedOverriden(Fvector& dest, bool clear_override)
{
	if (m_activation_speed_is_overriden)
	{
		if (clear_override)
		{
			m_activation_speed_is_overriden = false;
		}

		dest = m_overriden_activation_speed;
		return							true;
	}

	return								false;
}

void CWeapon::SetActivationSpeedOverride(Fvector const& speed)
{
	m_overriden_activation_speed = speed;
	m_activation_speed_is_overriden = true;
}

void CWeapon::activate_physic_shell()
{
	UpdateXForm();
	CPhysicsShellHolder::activate_physic_shell();
}

void CWeapon::setup_physic_shell()
{
	CPhysicsShellHolder::setup_physic_shell();
}

int		g_iWeaponRemove = 1;

bool CWeapon::NeedToDestroyObject()	const
{
	if (GameID() == eGameIDSingle) return false;
	if (Remote()) return false;
	if (H_Parent()) return false;
	if (g_iWeaponRemove == -1) return false;
	if (g_iWeaponRemove == 0) return true;
	if (TimePassedAfterIndependant() > m_dwWeaponRemoveTime)
		return true;

	return false;
}

ALife::_TIME_ID	 CWeapon::TimePassedAfterIndependant()	const
{
	if (!H_Parent() && m_dwWeaponIndependencyTime != 0)
		return Level().timeServer() - m_dwWeaponIndependencyTime;
	else
		return 0;
}

bool CWeapon::can_kill() const
{
	if (GetSuitableAmmoTotal(true) || m_ammoTypes.empty())
		return				(true);

	return					(false);
}

CInventoryItem* CWeapon::can_kill(CInventory* inventory) const
{
	if (GetAmmoElapsed() || m_ammoTypes.empty())
		return				(const_cast<CWeapon*>(this));

	TIItemContainer::iterator I = inventory->m_all.begin();
	TIItemContainer::iterator E = inventory->m_all.end();
	for (; I != E; ++I) {
		CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(*I);
		if (!inventory_item)
			continue;

		xr_vector<shared_str>::const_iterator	i = std::find(m_ammoTypes.begin(), m_ammoTypes.end(), inventory_item->object().cNameSect());
		if (i != m_ammoTypes.end())
			return			(inventory_item);
	}

	return					(0);
}

const CInventoryItem* CWeapon::can_kill(const xr_vector<const CGameObject*>& items) const
{
	if (m_ammoTypes.empty())
		return				(this);

	xr_vector<const CGameObject*>::const_iterator I = items.begin();
	xr_vector<const CGameObject*>::const_iterator E = items.end();
	for (; I != E; ++I) {
		const CInventoryItem* inventory_item = smart_cast<const CInventoryItem*>(*I);
		if (!inventory_item)
			continue;

		xr_vector<shared_str>::const_iterator	i = std::find(m_ammoTypes.begin(), m_ammoTypes.end(), inventory_item->object().cNameSect());
		if (i != m_ammoTypes.end())
			return			(inventory_item);
	}

	return					(0);
}

bool CWeapon::ready_to_kill() const
{
	return					(
		!IsMisfire() &&
		((GetState() == eIdle) || (GetState() == eFire) || (GetState() == eFire2)) &&
		GetAmmoElapsed()
		);
}

void _inertion(float& _val_cur, const float& _val_trgt, const float& _friction)
{
	float friction_i = 1.f - _friction;
	_val_cur = _val_cur * _friction + _val_trgt * friction_i;
}

float _lerp(const float& _val_a, const float& _val_b, const float& _factor)
{
	return (_val_a * (1.0 - _factor)) + (_val_b * _factor);
}

void CWeapon::UpdateHudAdditonal(Fmatrix& trans)
{
	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if (!pActor)
		return;

	attachable_hud_item* hi = HudItemData();
	R_ASSERT(hi);

	u8 idx = GetCurrentHudOffsetIdx();

	//============= Поворот ствола во время аима =============//
	if ((IsZoomed() && m_zoom_params.m_fZoomRotationFactor <= 1.f) || (!IsZoomed() && m_zoom_params.m_fZoomRotationFactor > 0.f))
	{
		Fvector curr_offs, curr_rot;
		curr_offs = hi->m_measures.m_hands_offset[0][idx]; //pos,aim
		curr_rot = hi->m_measures.m_hands_offset[1][idx]; //rot,aim

		curr_offs.mul(m_zoom_params.m_fZoomRotationFactor);
		curr_rot.mul(m_zoom_params.m_fZoomRotationFactor);

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

		float src = 10 * Device.fTimeDelta * 0.65;
		float target = pActor->IsZoomAimingMode() ? 1.3f : -0.3f;
		clamp(src, 0.f, 1.f);
		m_zoom_params.m_fZoomRotationFactor = _lerp(m_zoom_params.m_fZoomRotationFactor, target, src);
		clamp(m_zoom_params.m_fZoomRotationFactor, 0.f, 1.0f);
	}

	//============= Подготавливаем общие переменные =============//

	clamp(idx, u8(0), u8(1));
	bool bForAim = (idx == 1);

	float fInertiaPower = GetInertionPowerFactor();

	float fYMag = pActor->fFPCamYawMagnitude;
	float fPMag = pActor->fFPCamPitchMagnitude;

	static float fAvgTimeDelta = Device.fTimeDelta;
	_inertion(fAvgTimeDelta, Device.fTimeDelta, 0.8f);

	//======== Проверяем доступность инерции и стрейфа ========//
	if (!g_player_hud->inertion_allowed())
		return;

	//============= Боковой стрейф с оружием =============//
	float fStrafeMaxTime = m_strafe_offset[2][idx].y; // Макс. время в секундах, за которое мы наклонимся из центрального положения
	if (fStrafeMaxTime <= EPS)
		fStrafeMaxTime = 0.01f;

	float fStepPerUpd = fAvgTimeDelta / fStrafeMaxTime; // Величина изменение фактора поворота

	// Добавляем боковой наклон от движения камеры
	float fCamReturnSpeedMod = 1.5f; // Восколько ускоряем нормализацию наклона, полученного от движения камеры (только от бедра)
	// Высчитываем минимальную скорость поворота камеры для начала инерции
	float fStrafeMinAngle = _lerp(
		m_strafe_offset[3][0].y,
		m_strafe_offset[3][1].y,
		m_zoom_params.m_fZoomRotationFactor);

	// Высчитываем мксимальный наклон от поворота камеры
	float fCamLimitBlend = _lerp(
		m_strafe_offset[3][0].x,
		m_strafe_offset[3][1].x,
		m_zoom_params.m_fZoomRotationFactor);

	// Считаем стрейф от поворота камеры
	if (abs(fYMag) > (m_fLR_CameraFactor == 0.0f ? fStrafeMinAngle : 0.0f))
	{ //--> Камера крутится по оси Y
		m_fLR_CameraFactor -= (fYMag * 0.025f);

		clamp(m_fLR_CameraFactor, -fCamLimitBlend, fCamLimitBlend);
	}
	else
	{ //--> Камера не поворачивается - убираем наклон
		if (m_fLR_CameraFactor < 0.0f)
		{
			m_fLR_CameraFactor += fStepPerUpd * (bForAim ? 1.0f : fCamReturnSpeedMod);
			clamp(m_fLR_CameraFactor, -fCamLimitBlend, 0.0f);
		}
		else
		{
			m_fLR_CameraFactor -= fStepPerUpd * (bForAim ? 1.0f : fCamReturnSpeedMod);
			clamp(m_fLR_CameraFactor, 0.0f, fCamLimitBlend);
		}
	}
	// Добавляем боковой наклон от ходьбы вбок
	float fChangeDirSpeedMod = 3; // Восколько быстро меняем направление направление наклона, если оно в другую сторону от текущего

	u32 iMovingState = pActor->MovingState();
	if ((iMovingState & mcLStrafe) != 0)
	{ // Движемся влево
		float fVal = (m_fLR_MovingFactor > 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
		m_fLR_MovingFactor -= fVal;
	}
	else if ((iMovingState & mcRStrafe) != 0)
	{ // Движемся вправо
		float fVal = (m_fLR_MovingFactor < 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
		m_fLR_MovingFactor += fVal;
	}
	else
	{ // Двигаемся в любом другом направлении - плавно убираем наклон
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

	// Вычисляем и нормализируем итоговый фактор наклона
	float fLR_Factor = m_fLR_MovingFactor + (m_fLR_CameraFactor * fInertiaPower);
	clamp(fLR_Factor, -1.0f, 1.0f); // Фактор боковой ходьбы не должен превышать эти лимиты

	// Производим наклон ствола для нормального режима и аима
	for (int _idx = 0; _idx <= 1; _idx++)//<-- Для плавного перехода
	{
		bool bEnabled = (m_strafe_offset[2][_idx].x != 0.0f);
		if (!bEnabled)
			continue;

		Fvector curr_offs, curr_rot;

		// Смещение позиции худа в стрейфе
		curr_offs = m_strafe_offset[0][_idx]; //pos
		curr_offs.mul(fLR_Factor);                   // Умножаем на фактор стрейфа

		// Поворот худа в стрейфе
		curr_rot = m_strafe_offset[1][_idx]; //rot
		curr_rot.mul(-PI / 180.f);                          // Преобразуем углы в радианы
		curr_rot.mul(fLR_Factor);                   // Умножаем на фактор стрейфа

		// Мягкий переход между бедром \ прицелом
		if (_idx == 0)
		{ // От бедра
			curr_offs.mul(1.f - m_zoom_params.m_fZoomRotationFactor);
			curr_rot.mul(1.f - m_zoom_params.m_fZoomRotationFactor);
		}
		else
		{ // Во время аима
			curr_offs.mul(m_zoom_params.m_fZoomRotationFactor);
			curr_rot.mul(m_zoom_params.m_fZoomRotationFactor);
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

	//============= Инерция оружия =============//
   // Параметры инерции
	float fInertiaSpeedMod = _lerp(
		hi->m_measures.m_inertion_params.m_tendto_speed,
		hi->m_measures.m_inertion_params.m_tendto_speed_aim,
		m_zoom_params.m_fZoomRotationFactor);

	float fInertiaReturnSpeedMod = _lerp(
		hi->m_measures.m_inertion_params.m_tendto_ret_speed,
		hi->m_measures.m_inertion_params.m_tendto_ret_speed_aim,
		m_zoom_params.m_fZoomRotationFactor);

	float fInertiaMinAngle = _lerp(
		hi->m_measures.m_inertion_params.m_min_angle,
		hi->m_measures.m_inertion_params.m_min_angle_aim,
		m_zoom_params.m_fZoomRotationFactor);

	Fvector4 vIOffsets; // x = L, y = R, z = U, w = D
	vIOffsets.x = _lerp(
		hi->m_measures.m_inertion_params.m_offset_LRUD.x,
		hi->m_measures.m_inertion_params.m_offset_LRUD_aim.x,
		m_zoom_params.m_fZoomRotationFactor) * fInertiaPower;
	vIOffsets.y = _lerp(
		hi->m_measures.m_inertion_params.m_offset_LRUD.y,
		hi->m_measures.m_inertion_params.m_offset_LRUD_aim.y,
		m_zoom_params.m_fZoomRotationFactor) * fInertiaPower;
	vIOffsets.z = _lerp(
		hi->m_measures.m_inertion_params.m_offset_LRUD.z,
		hi->m_measures.m_inertion_params.m_offset_LRUD_aim.z,
		m_zoom_params.m_fZoomRotationFactor) * fInertiaPower;
	vIOffsets.w = _lerp(
		hi->m_measures.m_inertion_params.m_offset_LRUD.w,
		hi->m_measures.m_inertion_params.m_offset_LRUD_aim.w,
		m_zoom_params.m_fZoomRotationFactor) * fInertiaPower;

	// Высчитываем инерцию из поворотов камеры
	bool bIsInertionPresent = m_fLR_InertiaFactor != 0.0f || m_fUD_InertiaFactor != 0.0f;
	if (abs(fYMag) > fInertiaMinAngle || bIsInertionPresent)
	{
		float fSpeed = fInertiaSpeedMod;
		if (fYMag > 0.0f && m_fLR_InertiaFactor > 0.0f ||
			fYMag < 0.0f && m_fLR_InertiaFactor < 0.0f)
		{
			fSpeed *= 2.f; //--> Ускоряем инерцию при движении в противоположную сторону
		}

		m_fLR_InertiaFactor -= (fYMag * fAvgTimeDelta * fSpeed); // Горизонталь (м.б. > |1.0|)
	}

	if (abs(fPMag) > fInertiaMinAngle || bIsInertionPresent)
	{
		float fSpeed = fInertiaSpeedMod;
		if (fPMag > 0.0f && m_fUD_InertiaFactor > 0.0f ||
			fPMag < 0.0f && m_fUD_InertiaFactor < 0.0f)
		{
			fSpeed *= 2.f; //--> Ускоряем инерцию при движении в противоположную сторону
		}

		m_fUD_InertiaFactor -= (fPMag * fAvgTimeDelta * fSpeed); // Вертикаль (м.б. > |1.0|)
	}

	clamp(m_fLR_InertiaFactor, -1.0f, 1.0f);
	clamp(m_fUD_InertiaFactor, -1.0f, 1.0f);

	// Плавное затухание инерции (основное, но без линейной никогда не опустит инерцию до полного 0.0f)
	m_fLR_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);
	m_fUD_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);

	// Минимальное линейное затухание инерции при покое (горизонталь)
	if (fYMag == 0.0f)
	{
		float fRetSpeedMod = (fYMag == 0.0f ? 1.0f : 0.75f) * (fInertiaReturnSpeedMod * 0.075f);
		if (m_fLR_InertiaFactor < 0.0f)
		{
			m_fLR_InertiaFactor += fAvgTimeDelta * fRetSpeedMod;
			clamp(m_fLR_InertiaFactor, -1.0f, 0.0f);
		}
		else
		{
			m_fLR_InertiaFactor -= fAvgTimeDelta * fRetSpeedMod;
			clamp(m_fLR_InertiaFactor, 0.0f, 1.0f);
		}
	}

	// Минимальное линейное затухание инерции при покое (вертикаль)
	if (fPMag == 0.0f)
	{
		float fRetSpeedMod = (fPMag == 0.0f ? 1.0f : 0.75f) * (fInertiaReturnSpeedMod * 0.075f);
		if (m_fUD_InertiaFactor < 0.0f)
		{
			m_fUD_InertiaFactor += fAvgTimeDelta * fRetSpeedMod;
			clamp(m_fUD_InertiaFactor, -1.0f, 0.0f);
		}
		else
		{
			m_fUD_InertiaFactor -= fAvgTimeDelta * fRetSpeedMod;
			clamp(m_fUD_InertiaFactor, 0.0f, 1.0f);
		}
	}

	// Применяем инерцию к худу
	float fLR_lim = (m_fLR_InertiaFactor < 0.0f ? vIOffsets.x : vIOffsets.y);
	float fUD_lim = (m_fUD_InertiaFactor < 0.0f ? vIOffsets.z : vIOffsets.w);

	Fvector curr_offs;
	curr_offs = { fLR_lim * -1.f * m_fLR_InertiaFactor, fUD_lim * m_fUD_InertiaFactor, 0.0f };

	Fmatrix hud_rotation;
	hud_rotation.identity();
	hud_rotation.translate_over(curr_offs);
	trans.mulB_43(hud_rotation);
}

void CWeapon::SetAmmoElapsed(int ammo_count)
{
	iAmmoElapsed = ammo_count;

	u32 uAmmo = u32(iAmmoElapsed);

	if (uAmmo != m_magazine.size())
	{
		if (uAmmo > m_magazine.size())
		{
			CCartridge			l_cartridge;
			l_cartridge.Load(m_ammoTypes[m_ammoType].c_str(), m_ammoType);
			while (uAmmo > m_magazine.size())
				m_magazine.push_back(l_cartridge);
		}
		else
		{
			while (uAmmo < m_magazine.size())
				m_magazine.pop_back();
		};
	};
}

u32	CWeapon::ef_main_weapon_type() const
{
	VERIFY(m_ef_main_weapon_type != u32(-1));
	return m_ef_main_weapon_type;
}

u32	CWeapon::ef_weapon_type() const
{
	VERIFY(m_ef_weapon_type != u32(-1));
	return m_ef_weapon_type;
}

u32 CWeapon::m_ai_weapon() const
{
	return m_ai_weapon_rank;
}

bool CWeapon::IsNecessaryItem(const shared_str& item_sect)
{
	return (std::find(m_ammoTypes.begin(), m_ammoTypes.end(), item_sect) != m_ammoTypes.end());
}

void CWeapon::modify_holder_params(float& range, float& fov) const
{
	if (!IsScopeAttached()) {
		inherited::modify_holder_params(range, fov);
		return;
	}
	range *= m_addon_holder_range_modifier;
	fov *= m_addon_holder_fov_modifier;
}

bool CWeapon::render_item_ui_query()
{
	bool b_is_active_item = (m_pInventory->ActiveItem() == this);
	bool res = b_is_active_item && IsZoomed() && ZoomHideCrosshair() && ZoomTexture() && !IsRotatingToZoom();
	return res;
}

void CWeapon::render_item_ui()
{
	if (m_zoom_params.m_pVision)
		m_zoom_params.m_pVision->Draw();

	ZoomTexture()->Update();
	ZoomTexture()->Draw();
}

bool CWeapon::unlimited_ammo()
{
	if (IsGameTypeSingle())
	{
		if (m_pInventory)
		{
			return psActorFlags.test(AF_UNLIMITEDAMMO) && m_DefaultCartridge.m_flags.test(CCartridge::cfCanBeUnlimited);
		}
		else
			return false;
	}

	return ((GameID() == eGameIDDeathmatch) &&
		m_DefaultCartridge.m_flags.test(CCartridge::cfCanBeUnlimited));

};

float CWeapon::Weight() const
{
	CInventoryItemObject* inv_object;
	float res = inv_object->Weight();
	if (IsGrenadeLauncherAttached() && GetGrenadeLauncherName().size()) {
		res += pSettings->r_float(GetGrenadeLauncherName(), "inv_weight");
	}
	if (IsScopeAttached() && m_scopes.size()) {
		res += pSettings->r_float(GetScopeName(), "inv_weight");
	}
	if (IsSilencerAttached() && GetSilencerName().size()) {
		res += pSettings->r_float(GetSilencerName(), "inv_weight");
	}

	if (iAmmoElapsed)
	{
		float w = pSettings->r_float(m_ammoTypes[m_ammoType].c_str(), "inv_weight");
		float bs = pSettings->r_float(m_ammoTypes[m_ammoType].c_str(), "box_size");

		res += w * (iAmmoElapsed / bs);
	}
	return res;
}

bool CWeapon::show_crosshair()
{
	return !IsPending() && (!IsZoomed() || !ZoomHideCrosshair());
}

bool CWeapon::show_indicators()
{
	return !(IsZoomed() && ZoomTexture());
}

float CWeapon::GetConditionToShow() const
{
	return	(GetCondition());
}

BOOL CWeapon::ParentMayHaveAimBullet()
{
	CObject* O = H_Parent();
	CEntityAlive* EA = smart_cast<CEntityAlive*>(O);
	return EA->cast_actor() != 0;
}

BOOL CWeapon::ParentIsActor()
{
	CObject* O = H_Parent();
	if (!O)
		return FALSE;

	CEntityAlive* EA = smart_cast<CEntityAlive*>(O);
	if (!EA)
		return FALSE;

	return EA->cast_actor() != 0;
}

extern u32 hud_adj_mode;

bool CWeapon::ZoomHideCrosshair()
{
	if (hud_adj_mode != 0)
		return false;

	const auto pA = smart_cast<CActor*>(H_Parent());
	if (pA && pA->cam_Active() == pA->cameras[eacLookAt])
		return false;

	return m_zoom_params.m_bHideCrosshairInZoom || ZoomTexture();
}

#include "debug_renderer.h"
void CWeapon::debug_draw_firedeps()
{
	if (hud_adj_mode == 5 || hud_adj_mode == 6 || hud_adj_mode == 7)
	{
		CDebugRenderer& render = Level().debug_renderer();

		if (hud_adj_mode == 5)
			render.draw_aabb(get_LastFP(), 0.005f, 0.005f, 0.005f, color_xrgb(255, 0, 0));

		if (hud_adj_mode == 6)
			render.draw_aabb(get_LastFP2(), 0.005f, 0.005f, 0.005f, color_xrgb(0, 0, 255));

		if (hud_adj_mode == 7)
			render.draw_aabb(get_LastSP(), 0.005f, 0.005f, 0.005f, color_xrgb(0, 255, 0));
	}
}

const float& CWeapon::hit_probability() const
{
	VERIFY((g_SingleGameDifficulty >= egdNovice) && (g_SingleGameDifficulty <= egdMaster));
	return					(m_hit_probability[egdNovice]);
}

void CWeapon::OnStateSwitch(u32 S)
{
	inherited::OnStateSwitch(S);
	m_BriefInfo_CalcFrame = 0;
}

void CWeapon::OnAnimationEnd(u32 state)
{
	inherited::OnAnimationEnd(state);
}

u8 CWeapon::GetCurrentHudOffsetIdx()
{
	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if (!pActor)		return 0;

	bool b_aiming = ((IsZoomed() && m_zoom_params.m_fZoomRotationFactor <= 1.f) ||
		(!IsZoomed() && m_zoom_params.m_fZoomRotationFactor > 0.f));

	if (!b_aiming)
		return		0;
	else
		return		1;
}

void CWeapon::render_hud_mode()
{
	RenderLight();
}

bool CWeapon::MovingAnimAllowedNow()
{
	return !IsZoomed();
}

bool CWeapon::IsHudModeNow()
{
	return (HudItemData() != NULL);
}

void CWeapon::ZoomDynamicMod(bool bIncrement, bool bForceLimit)
{
	if (!IsScopeAttached())
		return;

	if (!m_zoom_params.m_bUseDynamicZoom)
		return;

	float delta, min_zoom_factor, max_zoom_factor;

	bool bIsSecondZOOM = bIsSecondVPZoomPresent() && psActorFlags.test(AF_3DSCOPE_ENABLE);

	max_zoom_factor = (bIsSecondZOOM ? GetSecondVPZoomFactor() * 100.0f : m_zoom_params.m_fScopeZoomFactor);

	GetZoomData(max_zoom_factor, delta, min_zoom_factor);

	if (bForceLimit)
	{
		if (bIsSecondZOOM)
			m_fSecondRTZoomFactor = (bIncrement ? max_zoom_factor : min_zoom_factor);
		else
			m_fRTZoomFactor = (bIncrement ? max_zoom_factor : min_zoom_factor);
	}
	else
	{
		float f = (bIsSecondZOOM ? m_fSecondRTZoomFactor : GetZoomFactor());

		f -= delta * (bIncrement ? 1.f : -1.f);

		clamp(f, max_zoom_factor, min_zoom_factor);

		if (bIsSecondZOOM)
			m_fSecondRTZoomFactor = f;
		else
			SetZoomFactor(f);
	}
}

void CWeapon::ZoomInc()
{
	ZoomDynamicMod(true, false);
}

void CWeapon::ZoomDec()
{
	ZoomDynamicMod(false, false);
}

u32 CWeapon::Cost() const
{
	u32 res = CInventoryItem::Cost();
	if (IsGrenadeLauncherAttached() && GetGrenadeLauncherName().size()) {
		res += pSettings->r_u32(GetGrenadeLauncherName(), "cost");
	}
	if (IsScopeAttached() && m_scopes.size()) {
		res += pSettings->r_u32(GetScopeName(), "cost");
	}
	if (IsSilencerAttached() && GetSilencerName().size()) {
		res += pSettings->r_u32(GetSilencerName(), "cost");
	}

	if (iAmmoElapsed)
	{
		float w = pSettings->r_float(m_ammoTypes[m_ammoType].c_str(), "cost");
		float bs = pSettings->r_float(m_ammoTypes[m_ammoType].c_str(), "box_size");

		res += iFloor(w * (iAmmoElapsed / bs));
	}
	return res;

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

float CWeapon::GetSecondVPZoomFactor() const
{
	float dist_k = (100.f / m_zoom_params.m_f3dZoomFactor);

	clamp(dist_k, 0.0f, 1.0f);

	float result = (m_zoom_params.m_fSecondVPFovFactor / dist_k);

	return result;
}

// Получить FOV от текущего оружия игрока для второго рендера

float CWeapon::GetSecondVPFov() const
{
	if (m_zoom_params.m_bUseDynamicZoom && bIsSecondVPZoomPresent())
		return (m_fSecondRTZoomFactor / 100.f) * 75.f;//g_fov; 75.f

	return GetSecondVPZoomFactor() * 75.f;//g_fov; 75.f
}

// Обновление необходимости включения второго вьюпорта +SecondVP+
// Вызывается только для активного оружия игрока
void CWeapon::UpdateSecondVP(bool bInGrenade)
{
	bool b_is_active_item = (m_pInventory != NULL) && (m_pInventory->ActiveItem() == this);
	R_ASSERT(ParentIsActor() && b_is_active_item); // Эта функция должна вызываться только для оружия в руках нашего игрока

	CActor* pActor = smart_cast<CActor*>(H_Parent());

	bool bCond_1 = bInZoomRightNow();													// Мы должны целиться

	bool bCond_2 = bIsSecondVPZoomPresent() && psActorFlags.test(AF_3DSCOPE_ENABLE);	// В конфиге должен быть прописан фактор зума для линзы (scope_lense_factor
	// больше чем 0)
	bool bCond_3 = pActor->cam_Active() == pActor->cam_FirstEye();						// Мы должны быть от 1-го лица	

	Device.m_SecondViewport.SetSVPActive(bCond_1 && bCond_2 && bCond_3 && !bInGrenade);
}

void CWeapon::UpdateAddonsHudParams()
{

}


void CWeapon::UpdateAddonsTransform(bool for_hud)
{

}

void CWeapon::SaveAttachableParams()
{
	if (!m_dbgItem)	return;

	LPCSTR sect_name = m_dbgItem->item().m_section_id.c_str();
	string_path fname;
	FS.update_path(fname, "$game_data$", make_string("_world\\%s.ltx", sect_name).c_str());

	CInifile* pHudCfg = new CInifile(fname, FALSE, FALSE, TRUE);

	pHudCfg->w_string(sect_name, "position", make_string("%f,%f,%f", m_Offset.c.x, m_Offset.c.y, m_Offset.c.z).c_str());
	Fvector ypr;
	m_Offset.getHPB(ypr.x, ypr.y, ypr.z);
	ypr.mul(180.f / PI);
	pHudCfg->w_string(sect_name, "orientation", make_string("%f,%f,%f", ypr.x, ypr.y, ypr.z).c_str());

	if (pSettings->line_exist(sect_name, "strap_position") && pSettings->line_exist(sect_name, "strap_orientation"))
	{
		pHudCfg->w_string(sect_name, "strap_position", make_string("%f,%f,%f", m_StrapOffset.c.x, m_StrapOffset.c.y, m_StrapOffset.c.z).c_str());
		m_StrapOffset.getHPB(ypr.x, ypr.y, ypr.z);
		ypr.mul(180.f / PI);
		pHudCfg->w_string(sect_name, "strap_orientation", make_string("%f,%f,%f", ypr.x, ypr.y, ypr.z).c_str());
	}

	xr_delete(pHudCfg);
	Msg("data saved to %s", fname);
	Sleep(250);

}
void CWeapon::ParseCurrentItem(CGameFont* F)
{
	F->OutNext("WEAPON IN STRAPPED MOD [%d]", m_strapped_mode);
}

bool CWeapon::IsMisfireNow()
{
	return IsMisfire();
}

BOOL CWeapon::IsEmptyMagazine() const
{
	return (iAmmoElapsed == 0);
}

bool CWeapon::IsMagazineEmpty()
{
	return IsEmptyMagazine();
}
