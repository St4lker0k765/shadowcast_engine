#include "stdafx.h"

#include "IKLimbsController.h"

#include "IK/IKLimb.h"
#include "physicsshellholder.h"

#include "mt_config.h"
#include "ik_anim_state.h"
#include "../xrphysics/mathutils.h"
#include "../Include/xrRender/RenderVisual.h"
#include "../Include/xrRender/Kinematics.h"
//#include "ode_include.h"
#include "characterphysicssupport.h"
#include "../xrEngine/motion.h"
#ifdef DEBUG
#	include "PHDebug.h"
#endif // DEBUG




CIKLimbsController::CIKLimbsController(): m_object(0), m_legs_blend(0)
{
	
}

void CIKLimbsController::Create( CGameObject* O )
{
	VERIFY( O );
	m_legs_blend	 = 0;
	
	IKinematics* K=smart_cast<IKinematics*>(O->Visual());
	m_object = O;
	VERIFY( K );
	u16 sz = 2;
	if( K->LL_UserData() && K->LL_UserData()->section_exist( "ik" ) )
		sz = K->LL_UserData()->r_u16( "ik", "num_limbs");
	VERIFY( sz <= max_size );
	
	_bone_chains.reserve( sz );
	for( u16 i = 0; sz > i; ++i )
						LimbSetup();
	
	bool already_has_callbacks = !O->visual_callbacks().empty();
	O->add_visual_callback(IKVisualCallback);
	if( already_has_callbacks )
		std::swap( * ( O->visual_callbacks().begin() ),  * ( O->visual_callbacks().end()-1 ) );
	_pose_extrapolation.init( O->XFORM() );
}


void	CIKLimbsController::LimbSetup( )
{
	_bone_chains.push_back( CIKLimb( ) );

	IKinematicsAnimated *skeleton_animated = m_object->Visual( )->dcast_PKinematicsAnimated( );

	_bone_chains.back( ).Create( ( u16 ) _bone_chains.size( )-1, skeleton_animated, true );
}

void	CIKLimbsController::LimbCalculate( SCalculateData &cd )
{
	cd.do_collide	= m_legs_blend && !cd.m_limb->KinematicsAnimated()->LL_GetMotionDef( m_legs_blend->motionID )->marks.empty() ;//m_legs_blend->;
	cd.m_limb->ApplyState( cd );
}

void	CIKLimbsController::LimbUpdate( CIKLimb &L, AccessLock* free_me_before_raypick)
{
	IKinematicsAnimated *skeleton_animated = m_object->Visual( )->dcast_PKinematicsAnimated( );
	VERIFY( skeleton_animated );
	L.Update( m_object, m_legs_blend, _pose_extrapolation, free_me_before_raypick );
}

IC void	update_blend (CBlend* &b)
{
	if(b && CBlend::eFREE_SLOT == b->blend_state())
		b = 0;
}

IC float lerp(float t, float  a, float b)
{ 
	return ( a + t * (b - a) );
}

void	y_shift_bones( IKinematics* K, float shift )
{
	u16 bc = K->LL_BoneCount();
	for(u16 i = 0; bc > i ; ++i )
		K->LL_GetTransform( i ).c.y +=shift ; 
}
float	CIKLimbsController::LegLengthShiftLimit			( float current_shift, const SCalculateData cd[max_size] )
{
	float shift_down = -phInfinity;
	const u16 sz =(u16)_bone_chains.size();
	for(u16 j = 0; sz > j; ++j )
		if( cd[j].state.foot_step )
		{
			float s_down = cd[j].m_limb->ObjShiftDown( current_shift, cd[j] );
			if( shift_down < s_down )
					shift_down = s_down;
		}
	return shift_down;
}
static const float static_shift_object_speed = .2f;
float	CIKLimbsController::StaticObjectShift			( const SCalculateData cd[max_size] )
{
		const float current_shift = _object_shift.shift();

		u16 cnt = 0; float shift_up =0;
		const u16 sz =(u16)_bone_chains.size();
		for(u16 j = 0; sz > j; ++j )
			if( cd[j].state.foot_step )
			{
				float s_up = cd[j].cl_shift.y + current_shift;
				if( 0.f < s_up )
				{
					shift_up += s_up;
					++cnt;
				}
			} 
		if( 0 < cnt )
				shift_up /= cnt;
		float shift_down = LegLengthShiftLimit( current_shift, cd );
		float shift = 0;
		if( shift_down > 0.f )
			shift = -shift_down;
		else if( -shift_down < shift_up )
			shift = -shift_down;
		else
			shift = shift_up;
		VERIFY( _valid( shift ) );
		_object_shift.set_taget( shift , _abs( current_shift - shift ) / static_shift_object_speed );
		return shift;
}
static float doun_shift_to_correct = 0.3f;
static float doun_shift_correct = 0.1f;
bool	CIKLimbsController::PredictObjectShift			(  const SCalculateData cd[max_size] )
{
	
	float predict_time_shift_down = FLT_MAX;
	float predict_time_shift_up = FLT_MAX;
	float predict_shift_down = 0.f;
	//float predict_shift_up = 0.f;
	bool shift_down = false;
	bool shift_up = false;
	const u16 sz =(u16)_bone_chains.size();
	float current_shift = _object_shift.shift();
	for(u16 j = 0; sz > j; ++j )
		if( !cd[j].state.foot_step )
		{
			float time = cd[j].m_limb->time_to_footstep();

			float lshift = cd[j].m_limb->footstep_shift();
			if( lshift < 0.f  )
			{
				if( time < predict_time_shift_down )
				{
					predict_time_shift_down	= time;
					predict_shift_down	= lshift;
					shift_down = true;
				}
			} 
			else if( current_shift < -doun_shift_to_correct && time < predict_time_shift_up )
			{
				predict_time_shift_up	= time;
				
				shift_up = true;
			}
			
		} 
	float	predict_shift = 0;
	float	predict_time_shift = FLT_MAX;

	if( shift_down )
	{
		predict_shift = predict_shift_down;
		predict_time_shift = predict_time_shift_down;
	
	} else if( shift_up )
	{
		predict_shift = 0;
		predict_time_shift = predict_time_shift_up;
	} else 
		return false;
	//{
	//	predict_shift = 0;
	//	predict_time_shift = Device.fTimeDelta;
	//}
		
	
	if( predict_time_shift < EPS_S )
		predict_time_shift = Device.fTimeDelta;
		_object_shift.set_taget( predict_shift, predict_time_shift );
	return true;
}


void	CIKLimbsController::ObjectShift	( float static_shift, const SCalculateData cd[max_size])
{
	
	u16 cnt_in_step = 0;
	const u16 sz =(u16)_bone_chains.size();
	for(u16 j = 0; sz > j; ++j )
			if( cd[j].m_limb->foot_step() )
					++cnt_in_step;

	CPhysicsShellHolder *sh = smart_cast<CPhysicsShellHolder*>(m_object);
	VERIFY( sh );
	//CCharacterPhysicsSupport *ch = sh->character_physics_support();	
	_object_shift.freeze(  !!Device.Paused() );//ch->interactive_motion() ||

	if(  cnt_in_step != sz && PredictObjectShift( cd )  )//cnt_in_step > 0 &&
		return;
	StaticObjectShift( cd );
}

void	CIKLimbsController::ShiftObject( const SCalculateData cd[max_size] )
{		
		IKinematics *skeleton_animated = m_object->Visual()->dcast_PKinematics( );
		VERIFY( skeleton_animated );
	//	u16 root = skeleton_animated->LL_GetBoneRoot( ) ;

	//CBoneData &BD=skeleton_animated->LL_GetData(root);

		const float y_shift = _object_shift.shift();
		const u16 bones_count = skeleton_animated->LL_BoneCount();
		for( u16 i = 0; i < bones_count; ++i )
			skeleton_animated->LL_GetTransform( i ).c.y += y_shift;
		
		for( u16 i = 0; i < bones_count; ++i )
		{
			CBoneInstance &bi = skeleton_animated->LL_GetBoneInstance( i );
			if(bi.callback())
				bi.callback()( &bi );
			skeleton_animated->LL_GetTransform_R( i ).c.y += y_shift;
		}
	//	skeleton_animated->LL_GetTransform(root).c.y += _object_shift.shift();
	//	skeleton_animated->Bone_Calculate(&BD, &Fidentity );
		
}

int ik_shift_object = 1;
void CIKLimbsController::Calculate( )
{
	IKinematics* K = m_object->Visual()->dcast_PKinematics();

	K->protectKinematics_.Enter();

	update_blend( m_legs_blend );

	Fmatrix &obj = m_object->XFORM( );
#ifdef	DEBUG
	if( ph_dbg_draw_mask1.test( phDbgDrawIKSHiftObject ) )
		_object_shift.dbg_draw( obj, _pose_extrapolation, Fvector().set( 0,2.5f,0));
#endif

	SCalculateData cd[max_size];



	xr_vector<CIKLimb>::iterator i, b = _bone_chains.begin(), e = _bone_chains.end();
	for( i = b ; e != i; ++i )
	{
		cd[i-b] = SCalculateData ( *i, obj );
		LimbCalculate( cd[i-b] );
	}

	u16 root = K->LL_GetBoneRoot( ) ;
	CBoneInstance &root_bi = K->LL_GetBoneInstance(root);

	BOOL sv_root_cb_ovwr = root_bi.callback_overwrite();
	BoneCallback sv_root_cb =		root_bi.callback();

	root_bi.set_callback( root_bi.callback_type(), 0, root_bi.callback_param(), TRUE );


	if( ik_shift_object  )//&& ! m_object->animation_movement_controlled( )
	{

			ShiftObject( cd );
	}

	const u16 sz =(u16)_bone_chains.size();
	for(u16 j = 0; sz > j; ++j )
		cd[j].m_limb->SetGoal( cd[j] );

	for(u16 j = 0; sz > j; ++j )
	{
		
		cd[j].m_limb->SolveBones( cd[j] );

#ifdef	DEBUG
	if( ph_dbg_draw_mask1.test( phDbgDrawIKPredict ) )
	{
		//IKinematics *K = m_object->Visual()->dcast_PKinematics( );
		u16 ref_bone_id = cd[j].m_limb->dbg_ref_bone_id();
		Fmatrix m =Fmatrix().mul( obj, K->LL_GetTransform( ref_bone_id ) );
		Fvector toe;
		cd[j].m_limb->dbg_ik_foot().ToePosition( toe );
		m.transform_tiny( toe );
		DBG_DrawLine( toe, Fvector().add( toe, Fvector().set( 0, -_object_shift.shift(), 0 ) ), color_xrgb( 255, 0, 0 )  );
	}
#endif
	}
	ObjectShift( 0, cd );

	root_bi.set_callback( root_bi.callback_type(), sv_root_cb, root_bi.callback_param(), sv_root_cb_ovwr );

	K->protectKinematics_.Leave();
}

void CIKLimbsController::Destroy(CGameObject* O)
{
#ifdef _DEBUG
	CPhysicsShellHolder*	Sh = smart_cast<CPhysicsShellHolder*>(O);
	VERIFY(Sh);
	CIKLimbsController* ik = Sh->character_ik_controller();
	VERIFY( ik );
	VERIFY( ik == this );
#endif

	O->remove_visual_callback(IKVisualCallback);
	xr_vector<CIKLimb>::iterator	i = _bone_chains.begin(), e = _bone_chains.end();
	for(;e!=i;++i)
		i->Destroy();
	_bone_chains.clear();

	Device.remove_from_seq_parallel(fastdelegate::FastDelegate0<>(this, &CIKLimbsController::UpdateIK));
}

void _stdcall CIKLimbsController:: IKVisualCallback( IKinematics* K )
{
	//if (Device.Paused())
	//	return;

#ifdef DEBUG
	if( ph_dbg_draw_mask1.test( phDbgIKOff ) )
		return;
#endif
	
	CGameObject* O=( ( CGameObject* )K->GetUpdateCallbackParam());
	CPhysicsShellHolder*	Sh = smart_cast<CPhysicsShellHolder*>( O );
	VERIFY( Sh );
	CIKLimbsController* ik = Sh->character_ik_controller( );
	VERIFY( ik );
	ik->Calculate( );
}

void CIKLimbsController::PlayLegs( CBlend *b )	
{
	m_legs_blend	= b;
#ifdef DEBUG
	IKinematicsAnimated *skeleton_animated = m_object->Visual( )->dcast_PKinematicsAnimated( );
	VERIFY( skeleton_animated );
	anim_name = skeleton_animated->LL_MotionDefName_dbg( b->motionID ).first;
	anim_set_name = skeleton_animated->LL_MotionDefName_dbg( b->motionID ).second;

	CMotionDef	&MD = *skeleton_animated->LL_GetMotionDef( b->motionID );
	if( MD.marks.empty() && ( MD.flags & esmUseFootSteps ) )
		Msg( "! No foot stseps for animation: animation name: %s, animation set: %s ", anim_name, anim_set_name );
#endif
}

#define IK_CALC_DIST 200.f
#define IK_ALWAYS_CALC_DIST 20.f

void CIKLimbsController::UpdateIK()
{
#ifdef DEBUG
	if (ph_dbg_draw_mask1.test(phDbgIKOff))
		return;
#endif
	CFrustum& viem_frust = ::Render->ViewBase;
	vis_data& vis = m_object->Visual()->getVisData();

	Fvector p;

	m_object->XFORM().transform_dir(p, vis.sphere.P);

	Fvector DeviceR = Device.vCameraPosition_saved;

	if (DeviceR.x < IK_CALC_DIST)
	{
		if (viem_frust.testSphere_dirty(p, vis.sphere.R) || DeviceR.x < IK_ALWAYS_CALC_DIST)
		{
			IKinematics* K = m_object->Visual()->dcast_PKinematics();

			K->protectKinematics_.Enter();

			IKinematicsAnimated* skeleton_animated = m_object->Visual()->dcast_PKinematicsAnimated();
			VERIFY(skeleton_animated);

			skeleton_animated->UpdateTracks();
			update_blend(m_legs_blend);

			_pose_extrapolation.update(m_object->XFORM());
			for (CIKLimb it : _bone_chains)
			{
				LimbUpdate(it, &K->protectKinematics_); // To avoid deadlocking secondary and main threads(because of mutex race) need to send the pointer 
				// to the mutex up to the ik_foot_collider.cpp->Pick() function, so that it will release the mutex before RayPick call
				// and then capture it again after RayPick is finished.
			}

			K->protectKinematics_.Leave();
		}
	}
}

void	CIKLimbsController::Update()
{
	UpdateIK();
}