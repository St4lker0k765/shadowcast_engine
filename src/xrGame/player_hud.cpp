#include "stdafx.h"
#include "player_hud.h"
#include "HudItem.h"
#include "ui_base.h"
#include "actor.h"
#include "physic_item.h"
#include "static_cast_checked.hpp"
#include "actoreffector.h"
#include "../xrEngine/IGame_Persistent.h"

player_hud* g_player_hud = NULL;
Fvector _ancor_pos;
Fvector _wpn_root_pos;

#define PITCH_OFFSET_R		   0.0f//0.017f    Насколько сильно ствол смещается вбок (влево) при вертикальных поворотах камеры	--#SM+#--
#define PITCH_OFFSET_N		   0.0f//0.012f    Насколько сильно ствол поднимается\опускается при вертикальных поворотах камеры	--#SM+#--
#define PITCH_OFFSET_D		   0.005f    // Насколько сильно ствол приближается\отдаляется при вертикальных поворотах камеры --#SM+#--
#define PITCH_LOW_LIMIT		   -PI      // Минимальное значение pitch при использовании совместно с PITCH_OFFSET_N			--#SM+#--
#define TENDTO_SPEED           1.0f     // Модификатор силы инерции (больше - чувствительней)
#define TENDTO_SPEED_AIM       1.0f     // (Для прицеливания)
#define TENDTO_SPEED_RET       5.0f     // Модификатор силы отката инерции (больше - быстрее)
#define TENDTO_SPEED_RET_AIM   5.0f     // (Для прицеливания)
#define INERT_MIN_ANGLE        0.0f     // Минимальная сила наклона, необходимая для старта инерции
#define INERT_MIN_ANGLE_AIM    3.5f     // (Для прицеливания)

// Пределы смещения при инерции (лево / право / верх / низ)
#define ORIGIN_OFFSET          0.04f,  0.04f,  0.04f, 0.02f 
#define ORIGIN_OFFSET_AIM      0.015f, 0.015f, 0.01f, 0.005f   

// Outdated - old inertion
#define TENDTO_SPEED_OLD       5.f      // Скорость нормализации положения ствола
#define TENDTO_SPEED_AIM_OLD   8.f      // (Для прицеливания)
#define ORIGIN_OFFSET_OLD     -0.05f    // Фактор влияния инерции на положение ствола (чем меньше, тем маштабней инерция)
#define ORIGIN_OFFSET_AIM_OLD -0.03f    // (Для прицеливания)

float CalcMotionSpeed(const shared_str& anim_name)
{

	if(!IsGameTypeSingle() && (anim_name=="anm_show" || anim_name=="anm_hide") )
		return 2.0f;
	else
		return 1.0f;
}

player_hud_motion* player_hud_motion_container::find_motion(const shared_str& name)
{
	xr_vector<player_hud_motion>::iterator it	= m_anims.begin();
	xr_vector<player_hud_motion>::iterator it_e = m_anims.end();
	for(;it!=it_e;++it)
	{
		const shared_str& s = (true)?(*it).m_alias_name:(*it).m_base_name;
		if( s == name)
			return &(*it);
	}
	return NULL;
}

void player_hud_motion_container::load(IKinematicsAnimated* model, const shared_str& sect)
{
	CInifile::Sect& _sect		= pSettings->r_section(sect);
	CInifile::SectCIt _b		= _sect.Data.begin();
	CInifile::SectCIt _e		= _sect.Data.end();
	player_hud_motion* pm		= NULL;
	
	string512					buff;
	MotionID					motion_ID;

	for(;_b!=_e;++_b)
	{
		if(strstr(_b->first.c_str(), "anm_")==_b->first.c_str())
		{
			const shared_str& anm	= _b->second;
			m_anims.resize			(m_anims.size()+1);
			pm						= &m_anims.back();
			//base and alias name
			pm->m_alias_name		= _b->first;
			
			if(_GetItemCount(anm.c_str())==1)
			{
				pm->m_base_name			= anm;
				pm->m_additional_name	= anm;
			}
			else
			{
				R_ASSERT2(_GetItemCount(anm.c_str()) <= 3, anm.c_str());
				string512				str_item;
				_GetItem(anm.c_str(),0,str_item);
				pm->m_base_name			= str_item;

				_GetItem(anm.c_str(),1,str_item);
				pm->m_additional_name	= (xr_strlen(str_item) > 0) ? pm->m_additional_name = str_item : pm->m_base_name;

				_GetItem(anm.c_str(), 2, str_item);
				pm->m_anim_speed = atof(str_item);
			}

			//and load all motions for it

			for(u32 i=0; i<=8; ++i)
			{
				if(i==0)
					xr_strcpy				(buff,pm->m_base_name.c_str());		
				else
					xr_sprintf				(buff,"%s%d",pm->m_base_name.c_str(),i);		

				motion_ID				= model->ID_Cycle_Safe(buff);
				if (!motion_ID.valid() && i == 0)
				{
					Msg("ATTENTION: motion %s invalid!",buff);
					motion_ID = model->ID_Cycle_Safe("hand_idle_doun");
				}
				else if(motion_ID.valid())
				{
					pm->m_animations.resize			(pm->m_animations.size()+1);
					pm->m_animations.back().mid		= motion_ID;
					pm->m_animations.back().name	= buff;
					//Msg("motion %s sucessfull loaded", buff);
#ifdef DEBUG
//					Msg(" alias=[%s] base=[%s] name=[%s]",pm->m_alias_name.c_str(), pm->m_base_name.c_str(), buff);
#endif // #ifdef DEBUG
				}
			}
			VERIFY2(pm->m_animations.size(),make_string("motion not found [%s]", pm->m_base_name.c_str()).c_str());
		}
	}
}

Fvector& attachable_hud_item::hands_attach_pos()
{
	return m_measures.m_hands_attach[0];
}

Fvector& attachable_hud_item::hands_attach_rot()
{
	return m_measures.m_hands_attach[1];
}

Fvector& attachable_hud_item::hands_offset_pos()
{
	u8 idx	= m_parent_hud_item->GetCurrentHudOffsetIdx();
	return m_measures.m_hands_offset[0][idx];
}

Fvector& attachable_hud_item::hands_offset_rot()
{
	u8 idx	= m_parent_hud_item->GetCurrentHudOffsetIdx();
	return m_measures.m_hands_offset[1][idx];
}

void attachable_hud_item::set_bone_visible(const shared_str& bone_name, BOOL bVisibility, BOOL bSilent)
{
	u16  bone_id;
	BOOL bVisibleNow;
	bone_id			= m_model->LL_BoneID			(bone_name);
	if(bone_id==BI_NONE)
	{
		if(bSilent)	return;
		R_ASSERT2	(0,			make_string("model [%s] has no bone [%s]",pSettings->r_string(m_sect_name, "item_visual"), bone_name.c_str()).c_str());
	}
	bVisibleNow		= m_model->LL_GetBoneVisible	(bone_id);
	if(bVisibleNow!=bVisibility)
		m_model->LL_SetBoneVisible	(bone_id,bVisibility, TRUE);
}

void attachable_hud_item::update(bool bForce)
{
	if(!bForce && m_upd_firedeps_frame==Device.dwFrame)	return;
	bool is_16x9 = UI().is_widescreen();
	
	if(!!m_measures.m_prop_flags.test(hud_item_measures::e_16x9_mode_now)!=is_16x9)
		m_measures.load(m_sect_name, m_model);

	Fvector ypr						= m_measures.m_item_attach[1];
	ypr.mul							(PI/180.f);
	m_attach_offset.setHPB			(ypr.x,ypr.y,ypr.z);
	m_attach_offset.translate_over	(m_measures.m_item_attach[0]);

	m_parent->calc_transform		(m_attach_place_idx, m_attach_offset, m_item_transform);
	m_parent_hud_item->UpdateAddonsTransform(true); // FFT++
	m_upd_firedeps_frame			= Device.dwFrame;

	IKinematicsAnimated* ka			=	m_model->dcast_PKinematicsAnimated();
	if(ka)
	{
		ka->UpdateTracks									();
		ka->dcast_PKinematics()->CalculateBones_Invalidate	();
		ka->dcast_PKinematics()->CalculateBones				(TRUE);
	}
}

void attachable_hud_item::update_hud_additional(Fmatrix& trans)
{
	if(m_parent_hud_item)
	{
		m_parent_hud_item->UpdateHudAdditonal(trans);
	}
}

void attachable_hud_item::setup_firedeps(firedeps& fd)
{
	update							(false);
	// fire point&direction
	if(m_measures.m_prop_flags.test(hud_item_measures::e_fire_point))
	{
		Fmatrix& fire_mat								= m_model->LL_GetTransform(m_measures.m_fire_bone);
		fire_mat.transform_tiny							(fd.vLastFP, m_measures.m_fire_point_offset);
		m_item_transform.transform_tiny					(fd.vLastFP);
		//fd.vLastFP.add(Device.vCameraPosition);

		fd.vLastFD.set									(0.f,0.f,1.f);
		m_item_transform.transform_dir					(fd.vLastFD);
		VERIFY(_valid(fd.vLastFD));
		VERIFY(_valid(fd.vLastFD));

		fd.m_FireParticlesXForm.identity				();
		fd.m_FireParticlesXForm.k.set					(fd.vLastFD);
		Fvector::generate_orthonormal_basis_normalized	(	fd.m_FireParticlesXForm.k,
															fd.m_FireParticlesXForm.j, 
															fd.m_FireParticlesXForm.i);
		VERIFY(_valid(fd.m_FireParticlesXForm));
	}

	if(m_measures.m_prop_flags.test(hud_item_measures::e_fire_point2))
	{
		Fmatrix& fire_mat			= m_model->LL_GetTransform(m_measures.m_fire_bone2);
		fire_mat.transform_tiny		(fd.vLastFP2,m_measures.m_fire_point2_offset);
		m_item_transform.transform_tiny	(fd.vLastFP2);
		//fd.vLastFP2.add(Device.vCameraPosition);
		VERIFY(_valid(fd.vLastFP2));
		VERIFY(_valid(fd.vLastFP2));
	}

	if(m_measures.m_prop_flags.test(hud_item_measures::e_shell_point))
	{
		Fmatrix& fire_mat			= m_model->LL_GetTransform(m_measures.m_shell_bone);
		fire_mat.transform_tiny		(fd.vLastSP,m_measures.m_shell_point_offset);
		m_item_transform.transform_tiny	(fd.vLastSP);
		//fd.vLastSP.add(Device.vCameraPosition);
		VERIFY(_valid(fd.vLastSP));
		VERIFY(_valid(fd.vLastSP));
	}
}

bool  attachable_hud_item::need_renderable()
{
	return m_parent_hud_item->need_renderable();
}

void attachable_hud_item::render()
{
	::Render->set_Transform		(&m_item_transform);
	::Render->add_Visual		(m_model->dcast_RenderVisual());
	debug_draw_firedeps			();
	m_parent_hud_item->render_hud_mode();
}

bool attachable_hud_item::render_item_ui_query()
{
	return m_parent_hud_item->render_item_3d_ui_query();
}

void attachable_hud_item::render_item_ui()
{
	m_parent_hud_item->render_item_3d_ui();
}

void hud_item_measures::load(const shared_str& sect_name, IKinematics* K)
{
	bool is_16x9 = UI().is_widescreen();
	string64	_prefix;
	xr_sprintf	(_prefix,"%s",is_16x9?"_16x9":"");
	string128	val_name;

	strconcat					(sizeof(val_name),val_name,"hands_position",_prefix);
	m_hands_attach[0]			= pSettings->r_fvector3(sect_name, val_name);
	strconcat					(sizeof(val_name),val_name,"hands_orientation",_prefix);
	m_hands_attach[1]			= pSettings->r_fvector3(sect_name, val_name);

	m_item_attach[0]			= pSettings->r_fvector3(sect_name, "item_position");
	m_item_attach[1]			= pSettings->r_fvector3(sect_name, "item_orientation");

	shared_str					 bone_name;
	m_prop_flags.set			 (e_fire_point,pSettings->line_exist(sect_name,"fire_bone"));
	if(m_prop_flags.test(e_fire_point))
	{
		bone_name				= pSettings->r_string(sect_name, "fire_bone");
		m_fire_bone				= K->LL_BoneID(bone_name);
		m_fire_point_offset		= pSettings->r_fvector3(sect_name, "fire_point");
	}else
		m_fire_point_offset.set(0,0,0);

	m_prop_flags.set			 (e_fire_point2,pSettings->line_exist(sect_name,"fire_bone2"));
	if(m_prop_flags.test(e_fire_point2))
	{
		bone_name				= pSettings->r_string(sect_name, "fire_bone2");
		m_fire_bone2			= K->LL_BoneID(bone_name);
		m_fire_point2_offset	= pSettings->r_fvector3(sect_name, "fire_point2");
	}else
		m_fire_point2_offset.set(0,0,0);

	m_prop_flags.set			 (e_shell_point,pSettings->line_exist(sect_name,"shell_bone"));
	if(m_prop_flags.test(e_shell_point))
	{
		bone_name				= pSettings->r_string(sect_name, "shell_bone");
		m_shell_bone			= K->LL_BoneID(bone_name);
		m_shell_point_offset	= pSettings->r_fvector3(sect_name, "shell_point");
	}else
		m_shell_point_offset.set(0,0,0);

	m_hands_offset[0][0].set	(0,0,0);
	m_hands_offset[1][0].set	(0,0,0);

	strconcat					(sizeof(val_name),val_name,"aim_hud_offset_pos",_prefix);
	m_hands_offset[0][1]		= pSettings->r_fvector3(sect_name, val_name);
	strconcat					(sizeof(val_name),val_name,"aim_hud_offset_rot",_prefix);
	m_hands_offset[1][1]		= pSettings->r_fvector3(sect_name, val_name);

	strconcat					(sizeof(val_name),val_name,"gl_hud_offset_pos",_prefix);
	m_hands_offset[0][2]		= pSettings->r_fvector3(sect_name, val_name);
	strconcat					(sizeof(val_name),val_name,"gl_hud_offset_rot",_prefix);
	m_hands_offset[1][2]		= pSettings->r_fvector3(sect_name, val_name);


	R_ASSERT2(pSettings->line_exist(sect_name,"fire_point")==pSettings->line_exist(sect_name,"fire_bone"),		sect_name.c_str());
	R_ASSERT2(pSettings->line_exist(sect_name,"fire_point2")==pSettings->line_exist(sect_name,"fire_bone2"),	sect_name.c_str());
	R_ASSERT2(pSettings->line_exist(sect_name,"shell_point")==pSettings->line_exist(sect_name,"shell_bone"),	sect_name.c_str());

	m_prop_flags.set(e_16x9_mode_now,is_16x9);

	//Загрузка параметров инерции --#SM+# Begin--
	m_inertion_params.m_pitch_offset_r = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_right", PITCH_OFFSET_R);
	m_inertion_params.m_pitch_offset_n = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_up", PITCH_OFFSET_N);
	m_inertion_params.m_pitch_offset_d = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_forward", PITCH_OFFSET_D);
	m_inertion_params.m_pitch_low_limit = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_up_low_limit", PITCH_LOW_LIMIT);

	m_inertion_params.m_origin_offset = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_origin_offset", ORIGIN_OFFSET_OLD);
	m_inertion_params.m_origin_offset_aim = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_origin_aim_offset", ORIGIN_OFFSET_AIM_OLD);

	m_inertion_params.m_tendto_speed = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_tendto_speed", TENDTO_SPEED);
	m_inertion_params.m_tendto_speed_aim = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_tendto_aim_speed", TENDTO_SPEED_AIM);
	m_inertion_params.m_tendto_ret_speed = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_tendto_ret_speed", TENDTO_SPEED_RET);
	m_inertion_params.m_tendto_ret_speed_aim = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_tendto_ret_aim_speed", TENDTO_SPEED_RET_AIM);

	m_inertion_params.m_min_angle = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_min_angle", INERT_MIN_ANGLE);
	m_inertion_params.m_min_angle_aim = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_min_angle_aim", INERT_MIN_ANGLE_AIM);

	m_inertion_params.m_offset_LRUD = READ_IF_EXISTS(pSettings, r_fvector4, sect_name, "inertion_offset_LRUD", Fvector4().set(ORIGIN_OFFSET));
	m_inertion_params.m_offset_LRUD_aim = READ_IF_EXISTS(pSettings, r_fvector4, sect_name, "inertion_offset_LRUD_aim", Fvector4().set(ORIGIN_OFFSET_AIM));
	//--#SM+# End--
}

attachable_hud_item::~attachable_hud_item()
{
	IRenderVisual* v			= m_model->dcast_RenderVisual();
	::Render->model_Delete		(v);
	m_model						= NULL;
}

void attachable_hud_item::load(const shared_str& sect_name)
{
	m_sect_name					= sect_name;

	// Visual
	const shared_str& visual_name = pSettings->r_string(sect_name, "item_visual");
	m_model						 = smart_cast<IKinematics*>(::Render->model_Create(visual_name.c_str()));

	m_attach_place_idx			= pSettings->r_u16(sect_name, "attach_place_idx");
	m_measures.load				(sect_name, m_model);
}

u32 attachable_hud_item::anim_play(const shared_str& anm_name_b, BOOL bMixIn, const CMotionDef*& md, u8& rnd_idx, float speed)
{
	R_ASSERT				(strstr(anm_name_b.c_str(),"anm_")==anm_name_b.c_str());
	string256				anim_name_r;
	bool is_16x9			= UI().is_widescreen();
	xr_sprintf				(anim_name_r,"%s%s",anm_name_b.c_str(),((m_attach_place_idx==1)&&is_16x9)?"_16x9":"");

	player_hud_motion* anm	= m_hand_motions.find_motion(anim_name_r);
	R_ASSERT2				(anm, make_string("model [%s] has no motion alias defined [%s]", m_sect_name.c_str(), anim_name_r).c_str());
	VERIFY2				(anm->m_animations.size(), make_string("model [%s] has no motion defined in motion_alias [%s]", pSettings->r_string(m_sect_name, "item_visual"), anim_name_r).c_str());
	
	if (speed == 1.f)
		speed = anm->m_anim_speed != 0 ? anm->m_anim_speed : CalcMotionSpeed(anm_name_b);

	rnd_idx					= (u8)Random.randI(anm->m_animations.size()) ;
	const motion_descr& M	= anm->m_animations[ rnd_idx ];

	u32 ret					= g_player_hud->anim_play(m_attach_place_idx, M.mid, bMixIn, md, speed);
	
	if(m_model->dcast_PKinematicsAnimated())
	{
		IKinematicsAnimated* ka			= m_model->dcast_PKinematicsAnimated();

		shared_str item_anm_name;
		if(anm->m_base_name!=anm->m_additional_name)
			item_anm_name = anm->m_additional_name;
		else
			item_anm_name = M.name;

		MotionID M2						= ka->ID_Cycle_Safe(item_anm_name);
		if(!M2.valid())
			M2							= ka->ID_Cycle_Safe("idle");
		else
			if(bDebug)
				Msg						("playing item animation [%s]",item_anm_name.c_str());
		
		R_ASSERT3(M2.valid(),"model has no motion [idle] ", pSettings->r_string(m_sect_name, "item_visual"));

		u16 root_id						= m_model->LL_GetBoneRoot();
		CBoneInstance& root_binst		= m_model->LL_GetBoneInstance(root_id);
		root_binst.set_callback_overwrite(TRUE);
		root_binst.mTransform.identity	();

		u16 pc							= ka->partitions().count();
		for(u16 pid=0; pid<pc; ++pid)
		{
			CBlend* B					= ka->PlayCycle(pid, M2, bMixIn);
			R_ASSERT					(B);
			B->speed					*= speed;
		}

		m_model->CalculateBones_Invalidate	();
	}

	R_ASSERT2		(m_parent_hud_item, "parent hud item is NULL");
	CPhysicItem&	parent_object = m_parent_hud_item->object();
	//R_ASSERT2		(parent_object, "object has no parent actor");
	//CObject*		parent_object = static_cast_checked<CObject*>(&m_parent_hud_item->object());

	if (IsGameTypeSingle() && parent_object.H_Parent() == Level().CurrentControlEntity())
	{
		CActor* current_actor = static_cast_checked<CActor*>(Level().CurrentControlEntity());
		VERIFY(current_actor);

		string_path ce_path;
		string_path anm_name;
		strconcat(sizeof(anm_name), anm_name, "camera_effects" "\\" "weapon" "\\", M.name.c_str(), ".anm");
		if (FS.exist(ce_path, "$game_anims$", anm_name))
		{
			CEffectorCam* ec = current_actor->Cameras().GetCamEffector(eCEWeaponAction);
			if (ec)
				current_actor->Cameras().RemoveCamEffector(eCEWeaponAction);

			CAnimatorCamEffector* e = new CAnimatorCamEffector();
			e->SetType(eCEWeaponAction);
			e->SetHudAffect(false);
			e->SetCyclic(false);
			e->Start(anm_name);
			current_actor->Cameras().AddCamEffector(e);
		}
	}
	return ret;
}


player_hud::player_hud()
{
	m_model					= NULL;
	m_model_2				= NULL;
	m_attached_items[0]		= NULL;
	m_attached_items[1]		= NULL;
	m_transform.identity	();
	m_transform_2.identity	();
}


player_hud::~player_hud()
{
	IRenderVisual* v			= m_model->dcast_RenderVisual();
	::Render->model_Delete		(v);
	m_model						= NULL;

	v = m_model_2->dcast_RenderVisual();
	::Render->model_Delete(v);
	m_model_2 = NULL;

	xr_vector<attachable_hud_item*>::iterator it	= m_pool.begin();
	xr_vector<attachable_hud_item*>::iterator it_e	= m_pool.end();
	for(;it!=it_e;++it)
	{
		attachable_hud_item* a	= *it;
		xr_delete				(a);
	}
	m_pool.clear				();
}

void player_hud::load(const shared_str& player_hud_sect)
{
	if(player_hud_sect ==m_sect_name)	return;
	bool b_reload = (m_model!=NULL);
	if(m_model)
	{
		IRenderVisual* v			= m_model->dcast_RenderVisual();
		::Render->model_Delete		(v);
	}

	if (m_model_2)
	{
		IRenderVisual* v = m_model_2->dcast_RenderVisual();
		::Render->model_Delete(v);
	}
	m_sect_name					= player_hud_sect;
	const shared_str& model_name= pSettings->r_string(player_hud_sect, "visual");
	m_model						= smart_cast<IKinematicsAnimated*>(::Render->model_Create(model_name.c_str()));
	m_model_2 = smart_cast<IKinematicsAnimated*>(::Render->model_Create(pSettings->line_exist(player_hud_sect, "visual_2") ? pSettings->r_string(player_hud_sect, "visual_2") : model_name.c_str()));

	u16 l_arm = m_model->dcast_PKinematics()->LL_BoneID("l_clavicle");
	u16 r_arm = m_model_2->dcast_PKinematics()->LL_BoneID("r_clavicle");

	// hides the unused arm meshes
	m_model->dcast_PKinematics()->LL_SetBoneVisible(l_arm, FALSE, TRUE);
	m_model_2->dcast_PKinematics()->LL_SetBoneVisible(r_arm, FALSE, TRUE);

	CInifile::Sect& _sect		= pSettings->r_section(player_hud_sect);
	CInifile::SectCIt _b		= _sect.Data.begin();
	CInifile::SectCIt _e		= _sect.Data.end();
	for(;_b!=_e;++_b)
	{
		if(strstr(_b->first.c_str(), "ancor_")==_b->first.c_str())
		{
			const shared_str& _bone	= _b->second;
			m_ancors.push_back		(m_model->dcast_PKinematics()->LL_BoneID(_bone));
		}
	}
	
//	Msg("hands visual changed to[%s] [%s] [%s]", model_name.c_str(), b_reload?"R":"", m_attached_items[0]?"Y":"");

	if(!b_reload)
	{
		m_model->PlayCycle("hand_idle_doun");
		m_model_2->PlayCycle("hand_idle_doun");
	}
	else {
		if(m_attached_items[1])
			m_attached_items[1]->m_parent_hud_item->on_a_hud_attach();

		if(m_attached_items[0])
			m_attached_items[0]->m_parent_hud_item->on_a_hud_attach();
	}
	m_model->dcast_PKinematics()->CalculateBones_Invalidate	();
	m_model->dcast_PKinematics()->CalculateBones(TRUE);
	m_model_2->dcast_PKinematics()->CalculateBones_Invalidate();
	m_model_2->dcast_PKinematics()->CalculateBones(TRUE);
}

bool player_hud::render_item_ui_query()
{
	bool res = false;
	if(m_attached_items[0])
		res |= m_attached_items[0]->render_item_ui_query();

	if(m_attached_items[1])
		res |= m_attached_items[1]->render_item_ui_query();

	return res;
}

void player_hud::render_item_ui()
{
	if(m_attached_items[0])
		m_attached_items[0]->render_item_ui();

	if(m_attached_items[1])
		m_attached_items[1]->render_item_ui();
}

void player_hud::render_hud()
{
	if(!m_attached_items[0] && !m_attached_items[1])	return;

	bool b_r0 = (m_attached_items[0] && m_attached_items[0]->need_renderable());
	bool b_r1 = (m_attached_items[1] && m_attached_items[1]->need_renderable());

	if(!b_r0 && !b_r1)									return;

	::Render->set_Transform		(&m_transform);
	::Render->add_Visual		(m_model->dcast_RenderVisual());
	::Render->set_Transform		(&m_transform_2);
	::Render->add_Visual		(m_model_2->dcast_RenderVisual());

	if(m_attached_items[0])
		m_attached_items[0]->render();
	
	if(m_attached_items[1])
		m_attached_items[1]->render();
}


#include "../xrEngine/motion.h"

u32 player_hud::motion_length(const shared_str& anim_name, const shared_str& hud_name, const CMotionDef*& md)
{
	float speed						= CalcMotionSpeed(anim_name);
	attachable_hud_item* pi			= create_hud_item(hud_name);
	player_hud_motion*	pm			= pi->m_hand_motions.find_motion(anim_name);
	if (!pm || !pm->m_animations.size())
		return						100; // ms TEMPORARY
	R_ASSERT2						(pm, 
		make_string	("hudItem model [%s] has no motion with alias [%s]", hud_name.c_str(), anim_name.c_str() ).c_str() 
		);
	return motion_length			(pm->m_animations[0].mid, md, speed);
}

u32 player_hud::motion_length(const MotionID& M, const CMotionDef*& md, float speed)
{
	md					= m_model->LL_GetMotionDef(M);
	VERIFY				(md);
	if (md->flags & esmStopAtEnd) 
	{
		CMotion*			motion		= m_model->LL_GetRootMotion(M);
		return				iFloor( 0.5f + 1000.f*motion->GetLength() / (md->Dequantize(md->speed) * speed) );
	}
	return					0;
}

const Fvector& player_hud::attach_rot(u8 part) const
{
	if (m_attached_items[part])
		return m_attached_items[part]->hands_attach_rot();
	else if (m_attached_items[!part])
		return m_attached_items[!part]->hands_attach_rot();

	return Fvector().set(0.f, 0.f, 0.f);
}

const Fvector& player_hud::attach_pos(u8 part) const
{
	if (m_attached_items[part])
		return m_attached_items[part]->hands_attach_pos();
	else if (m_attached_items[!part])
		return m_attached_items[!part]->hands_attach_pos();

	return Fvector().set(0.f, 0.f, 0.f);
}


void player_hud::update(const Fmatrix& cam_trans)
{
	Fmatrix trans = cam_trans;

	Fvector m1pos = attach_pos(0);
	Fvector m2pos = attach_pos(1);

	Fvector m1rot = attach_rot(0);
	Fvector m2rot = attach_rot(1);

	Fmatrix trans_2 = trans;

	if (m_attached_items[0])
		m_attached_items[0]->update_hud_additional(trans);

	if (m_attached_items[1])
		m_attached_items[1]->update_hud_additional(trans_2);
	else
		trans_2 = trans;

	// override hand offset for single hand animation
	m1rot.mul(PI / 180.f);
	m_attach_offset.setHPB(m1rot.x, m1rot.y, m1rot.z);
	m_attach_offset.translate_over(m1pos);

	m2rot.mul(PI / 180.f);
	m_attach_offset_2.setHPB(m2rot.x, m2rot.y, m2rot.z);
	m_attach_offset_2.translate_over(m2pos);

	m_transform.mul(trans, m_attach_offset);
	m_transform_2.mul(trans_2, m_attach_offset_2);
	// insert inertion here

	m_model->UpdateTracks();
	m_model->dcast_PKinematics()->CalculateBones_Invalidate();
	m_model->dcast_PKinematics()->CalculateBones(TRUE);

	m_model_2->UpdateTracks();
	m_model_2->dcast_PKinematics()->CalculateBones_Invalidate();
	m_model_2->dcast_PKinematics()->CalculateBones(TRUE);

	if (m_attached_items[0])
		m_attached_items[0]->update(true);

	if (m_attached_items[1])
		m_attached_items[1]->update(true);
}

u32 player_hud::anim_play(u16 part, const MotionID& M, BOOL bMixIn, const CMotionDef*& md, float speed, u16 override_part)
{
	u16 part_id = u16(-1);
	if (attached_item(0) && attached_item(1))
		part_id = m_model->partitions().part_id((part == 0) ? "right_hand" : "left_hand");

	if (override_part != u16(-1))
		part_id = override_part;

	if (part_id == u16(-1))
	{
		for (u8 pid = 0; pid < 3; pid++)
		{
			if (pid == 0 || pid == 2)
			{
				CBlend* B = m_model->PlayCycle(pid, M, bMixIn);
				R_ASSERT(B);
				B->speed *= speed;
			}
			if (pid == 0 || pid == 1)
			{
				CBlend* B = m_model_2->PlayCycle(pid, M, bMixIn);
				R_ASSERT(B);
				B->speed *= speed;
			}
		}

		m_model->dcast_PKinematics()->CalculateBones_Invalidate();
		m_model_2->dcast_PKinematics()->CalculateBones_Invalidate();
	}
	else if (part_id == 0 || part_id == 2)
	{
		for (u8 pid = 0; pid < 3; pid++)
		{
			if (pid != 1)
			{
				CBlend* B = m_model->PlayCycle(pid, M, bMixIn);
				R_ASSERT(B);
				B->speed *= speed;
			}
		}

		m_model->dcast_PKinematics()->CalculateBones_Invalidate();
	}
	else if (part_id == 1)
	{
		for (u8 pid = 0; pid < 3; pid++)
		{
			if (pid != 2)
			{
				CBlend* B = m_model_2->PlayCycle(pid, M, bMixIn);
				R_ASSERT(B);
				B->speed *= speed;
			}
		}

		m_model_2->dcast_PKinematics()->CalculateBones_Invalidate();
	}
	return motion_length(M, md, speed);
}

void player_hud::update_additional	(Fmatrix& trans)
{
	if(m_attached_items[0])
		m_attached_items[0]->update_hud_additional(trans);

	if(m_attached_items[1])
		m_attached_items[1]->update_hud_additional(trans);
}


void player_hud::update_inertion(Fmatrix& trans)
{
	if ( inertion_allowed() )
	{
		attachable_hud_item* pMainHud = m_attached_items[0];

		Fmatrix								xform;
		Fvector& origin						= trans.c; 
		xform								= trans;

		static Fvector						st_last_dir={0,0,0};

		// load params
		hud_item_measures::inertion_params inertion_data;
		if (pMainHud != NULL)
		{ // Загружаем параметры инерции из основного худа
			inertion_data.m_pitch_offset_r			= pMainHud->m_measures.m_inertion_params.m_pitch_offset_r;
			inertion_data.m_pitch_offset_n			= pMainHud->m_measures.m_inertion_params.m_pitch_offset_n;
			inertion_data.m_pitch_offset_d			= pMainHud->m_measures.m_inertion_params.m_pitch_offset_d;
			inertion_data.m_pitch_low_limit		    = pMainHud->m_measures.m_inertion_params.m_pitch_low_limit;
			inertion_data.m_origin_offset			= pMainHud->m_measures.m_inertion_params.m_origin_offset;
			inertion_data.m_origin_offset_aim		= pMainHud->m_measures.m_inertion_params.m_origin_offset_aim;
			inertion_data.m_offset_LRUD				= pMainHud->m_measures.m_inertion_params.m_offset_LRUD;
			inertion_data.m_offset_LRUD_aim			= pMainHud->m_measures.m_inertion_params.m_offset_LRUD_aim;
			inertion_data.m_tendto_speed			= pMainHud->m_measures.m_inertion_params.m_tendto_speed;
			inertion_data.m_tendto_speed_aim	    = pMainHud->m_measures.m_inertion_params.m_tendto_speed_aim;
			inertion_data.m_tendto_ret_speed		= pMainHud->m_measures.m_inertion_params.m_tendto_ret_speed;
			inertion_data.m_tendto_ret_speed_aim	= pMainHud->m_measures.m_inertion_params.m_tendto_ret_speed_aim;
			inertion_data.m_min_angle				= pMainHud->m_measures.m_inertion_params.m_min_angle;
			inertion_data.m_min_angle_aim			= pMainHud->m_measures.m_inertion_params.m_min_angle_aim;
		}
		else
		{ // Загружаем дефолтные параметры инерции
			inertion_data.m_pitch_offset_r			= PITCH_OFFSET_R;
			inertion_data.m_pitch_offset_n			= PITCH_OFFSET_N;
			inertion_data.m_pitch_offset_d			= PITCH_OFFSET_D;
			inertion_data.m_pitch_low_limit			= PITCH_LOW_LIMIT;
			inertion_data.m_origin_offset			= ORIGIN_OFFSET_OLD;
			inertion_data.m_origin_offset_aim		= ORIGIN_OFFSET_AIM_OLD;

			inertion_data.m_offset_LRUD.set			(ORIGIN_OFFSET);
			inertion_data.m_offset_LRUD_aim.set		(ORIGIN_OFFSET_AIM);

			inertion_data.m_tendto_speed			= TENDTO_SPEED;
			inertion_data.m_tendto_speed_aim		= TENDTO_SPEED_AIM;
			inertion_data.m_tendto_ret_speed		= TENDTO_SPEED_RET;
			inertion_data.m_tendto_ret_speed_aim	= TENDTO_SPEED_RET_AIM;
			inertion_data.m_min_angle				= INERT_MIN_ANGLE;
			inertion_data.m_min_angle_aim			= INERT_MIN_ANGLE_AIM;
		}

		// Replaced by CWeapon::UpdateHudAdditonal()
		// Very FPS sensitive and hard to control
		/*
		// calc difference
		Fvector								diff_dir;
		diff_dir.sub						(xform.k, st_last_dir);

		// clamp by PI_DIV_2
		Fvector last;						last.normalize_safe(st_last_dir);
		float dot							= last.dotproduct(xform.k);
		if (dot<EPS){
			Fvector v0;
			v0.crossproduct					(st_last_dir,xform.k);
			st_last_dir.crossproduct		(xform.k,v0);
			diff_dir.sub					(xform.k, st_last_dir);
		}

		// tend to forward
		float _tendto_speed, _origin_offset;
		if (pMainHud != NULL && pMainHud->m_parent_hud_item->GetCurrentHudOffsetIdx() > 0)
		{ // Худ в режиме "Прицеливание"
			float factor = pMainHud->m_parent_hud_item->GetInertionFactor();
			_tendto_speed = inertion_data.m_tendto_speed_aim - (inertion_data.m_tendto_speed_aim - inertion_data.m_tendto_speed) * factor;
			_origin_offset =
				inertion_data.m_origin_offset_aim - (inertion_data.m_origin_offset_aim - inertion_data.m_origin_offset) * factor;
		}
		else
		{ // Худ в режиме "От бедра"
			_tendto_speed = inertion_data.m_tendto_speed;
			_origin_offset = inertion_data.m_origin_offset;
		}

		// Фактор силы инерции
		if (pMainHud != NULL)
		{
			float power_factor = pMainHud->m_parent_hud_item->GetInertionPowerFactor();
			_tendto_speed *= power_factor;
			_origin_offset *= power_factor;
		}

		st_last_dir.mad(diff_dir, _tendto_speed * Device.fTimeDelta);
		origin.mad(diff_dir, _origin_offset);
		*/
		// pitch compensation
		float pitch							= angle_normalize_signed(xform.k.getP());

		if (pMainHud != NULL)
			pitch *= pMainHud->m_parent_hud_item->GetInertionFactor();

		// Отдаление\приближение
		origin.mad(xform.k, -pitch * inertion_data.m_pitch_offset_d);

		// Сдвиг в противоположную часть экрана
		origin.mad(xform.i, -pitch * inertion_data.m_pitch_offset_r);

		// Подьём\опускание
		clamp(pitch, inertion_data.m_pitch_low_limit, PI);
		origin.mad(xform.j, -pitch * inertion_data.m_pitch_offset_n);
	}
}


attachable_hud_item* player_hud::create_hud_item(const shared_str& sect)
{
	xr_vector<attachable_hud_item*>::iterator it = m_pool.begin();
	xr_vector<attachable_hud_item*>::iterator it_e = m_pool.end();
	for(;it!=it_e;++it)
	{
		attachable_hud_item* itm = *it;
		if(itm->m_sect_name==sect)
			return itm;
	}
	attachable_hud_item* res	= xr_new<attachable_hud_item>(this);
	res->load					(sect);
	res->m_hand_motions.load	(m_model, sect);
	m_pool.push_back			(res);

	return	res;
}

bool player_hud::allow_activation(CHudItem* item)
{
	if(m_attached_items[1])
		return m_attached_items[1]->m_parent_hud_item->CheckCompatibility(item);
	else
		return true;
}

void player_hud::attach_item(CHudItem* item)
{
	attachable_hud_item* pi			= create_hud_item(item->HudSection());
	int item_idx					= pi->m_attach_place_idx;
	
	if(	m_attached_items[item_idx] != pi)
	{
		if(m_attached_items[item_idx])
			m_attached_items[item_idx]->m_parent_hud_item->on_b_hud_detach();

		m_attached_items[item_idx]						= pi;
		pi->m_parent_hud_item							= item;

		if(item_idx==0 && m_attached_items[1])
			m_attached_items[1]->m_parent_hud_item->CheckCompatibility(item);

		item->on_a_hud_attach();
	}
	pi->m_parent_hud_item							= item;
}
//sync anim of other part to selected part (1 = sync to left hand anim; 2 = sync to right hand anim)
void player_hud::re_sync_anim(u8 part)
{
	u32 bc = part == 1 ? m_model_2->LL_PartBlendsCount(part) : m_model->LL_PartBlendsCount(part);
	for (u32 bidx = 0; bidx < bc; ++bidx)
	{
		CBlend* BR = part == 1 ? m_model_2->LL_PartBlend(part, bidx) : m_model->LL_PartBlend(part, bidx);
		if (!BR)
			continue;

		MotionID M = BR->motionID;

		u16 pc = m_model->partitions().count(); //same on both armatures
		for (u16 pid = 0; pid < pc; ++pid)
		{
			if (pid == 0)
			{
				CBlend* B = m_model->PlayCycle(0, M, TRUE);
				B->timeCurrent = BR->timeCurrent;
				B->speed = BR->speed;
				B = m_model_2->PlayCycle(0, M, TRUE);
				B->timeCurrent = BR->timeCurrent;
				B->speed = BR->speed;
			}
			else if (pid != part)
			{
				CBlend* B = part == 1 ? m_model->PlayCycle(pid, M, TRUE) : m_model_2->PlayCycle(pid, M, TRUE);
				B->timeCurrent = BR->timeCurrent;
				B->speed = BR->speed;
			}
		}
	}
}

void player_hud::detach_item_idx(u16 idx)
{
	if (NULL == m_attached_items[idx]) return;

	m_attached_items[idx]->m_parent_hud_item->on_b_hud_detach();

	m_attached_items[idx]->m_parent_hud_item		= NULL;
	m_attached_items[idx]							= NULL;

	if (idx == 1)
	{
		if (m_attached_items[0])
			re_sync_anim(2);
		else
		{
			m_model_2->PlayCycle("hand_idle_doun");
		}
	}
	else if (idx == 0)
	{
		if (m_attached_items[1])
		{
			//fix for a rare case where the right hand stays visible on screen after detaching the right hand's attached item
			player_hud_motion* pm = m_attached_items[1]->m_hand_motions.find_motion("anm_idle");
			const motion_descr& M = pm->m_animations[0];
			m_model->PlayCycle(0, M.mid, false);
			m_model->PlayCycle(2, M.mid, false);
		}
		else
		{
			m_model->PlayCycle("hand_idle_doun");
		}
	}

	if (!m_attached_items[0] && !m_attached_items[1])
	{
		m_model->PlayCycle("hand_idle_doun");
		m_model_2->PlayCycle("hand_idle_doun");
	}
}

void player_hud::detach_item(CHudItem* item)
{
	if( NULL==item->HudItemData() )		return;
	u16 item_idx						= item->HudItemData()->m_attach_place_idx;

	if( m_attached_items[item_idx]==item->HudItemData() )
	{
		detach_item_idx	(item_idx);
	}
}

void player_hud::calc_transform(u16 attach_slot_idx, const Fmatrix& offset, Fmatrix& result)
{
	IKinematics* kin = (attach_slot_idx == 0) ? m_model->dcast_PKinematics() : m_model_2->dcast_PKinematics();
	Fmatrix ancor_m = kin->LL_GetTransform(m_ancors[attach_slot_idx]);
	result.mul((attach_slot_idx == 0) ? m_transform : m_transform_2, ancor_m);
	result.mulB_43(offset);
}

bool player_hud::inertion_allowed()
{
	attachable_hud_item* hi = m_attached_items[0];
	if(hi)
	{
		bool res = ( hi->m_parent_hud_item->HudInertionEnabled() && hi->m_parent_hud_item->HudInertionAllowed() );
		return	res;
	}
	return true;
}

void player_hud::OnMovementChanged(ACTOR_DEFS::EMoveCommand cmd)
{
	if(cmd==0)
	{
		if(m_attached_items[0])
		{
			if(m_attached_items[0]->m_parent_hud_item->GetState()==CHUDState::eIdle)
				m_attached_items[0]->m_parent_hud_item->PlayAnimIdle();
		}
		if(m_attached_items[1])
		{
			if(m_attached_items[1]->m_parent_hud_item->GetState()==CHUDState::eIdle)
				m_attached_items[1]->m_parent_hud_item->PlayAnimIdle();
		}
	}else
	{
		if(m_attached_items[0])
			m_attached_items[0]->m_parent_hud_item->OnMovementChanged(cmd);

		if(m_attached_items[1])
			m_attached_items[1]->m_parent_hud_item->OnMovementChanged(cmd);
	}
}
