#include "stdafx.h"
#include "mosquitobald.h"
#include "ParticlesObject.h"
#include "level.h"
#include "physicsshellholder.h"
#include "../xrengine/xr_collide_form.h"
#include "../xrCore/xr_detail_collision.h"

//#|DCS++|
ENGINE_API extern int ps_enable_dcs_detail_collision;

ENGINE_API extern float ps_detail_collision_dcs_radius;

ENGINE_API extern Fvector actor_position;
//#|DCS++|

CMosquitoBald::CMosquitoBald() 
{
	m_fHitImpulseScale		= 1.f;
	m_bLastBlowoutUpdate	= false;
}

CMosquitoBald::~CMosquitoBald() 
{
}

void CMosquitoBald::Load(LPCSTR section) 
{
	inherited::Load(section);
}


bool CMosquitoBald::BlowoutState()
{
	bool result = inherited::BlowoutState();
	if(!result)
	{
		m_bLastBlowoutUpdate = false;
		UpdateBlowout();
	}
	else if(!m_bLastBlowoutUpdate)
	{
		m_bLastBlowoutUpdate = true;
		UpdateBlowout();
	}

	return result;
}
//bool CMosquitoBald::SecondaryHitState()
//{
//	bool result = inherited::SecondaryHitState();
//	if(!result)
//		UpdateBlowout();
//
//	return result;
//}

void CMosquitoBald::Affect(SZoneObjectInfo* O) 
{
	//#|DCS++|
	if (ps_enable_dcs_detail_collision)
	{
		if (actor_position.distance_to(Position()) <= ps_detail_collision_dcs_radius)
		{
			EraseDetailCollPointIfExists(ID());
			level_detail_coll.push_back(IDetailCollision(Position(), ID(), 2.5f, 0.3f, 1.f, true));
		}
	}
	//#|DCS++|

	auto pGameObject = smart_cast<CPhysicsShellHolder*>(O->object);
	if(!pGameObject) return;

	if(O->zone_ignore) return;

	Fvector P; 
	XFORM().transform_tiny(P,CFORM()->getSphere().P);

	Fvector hit_dir; 
	hit_dir.set(	::Random.randF(-.5f,.5f), 
					::Random.randF(.0f,1.f), 
					::Random.randF(-.5f,.5f)); 
	hit_dir.normalize();

	Fvector position_in_bone_space;

	VERIFY(!pGameObject->getDestroy());

	float dist = pGameObject->Position().distance_to(P) - pGameObject->Radius();
	float power = Power(dist>0.f?dist:0.f, Radius());
	float impulse = m_fHitImpulseScale*power*pGameObject->GetMass();

	if(power > 0.01f) 
	{
		position_in_bone_space.set(0.f,0.f,0.f);
		CreateHit(pGameObject->ID(),ID(),hit_dir,power,0,position_in_bone_space,impulse,m_eHitTypeBlowout);
		PlayHitParticles(pGameObject);
	}
}

void CMosquitoBald::UpdateSecondaryHit()
{
	if(m_dwAffectFrameNum == Device.dwFrame)	
		return;

	m_dwAffectFrameNum	= Device.dwFrame;
	if(Device.dwPrecacheFrame)					
		return;

	OBJECT_INFO_VEC_IT it;
	for(it = m_ObjectInfoMap.begin(); m_ObjectInfoMap.end() != it; ++it) 
	{
		if(!(*it).object->getDestroy())
		{
			CPhysicsShellHolder *pGameObject = smart_cast<CPhysicsShellHolder*>((&(*it))->object);
			if(!pGameObject) return;

			if((&(*it))->zone_ignore) return;
			Fvector P; 
			XFORM().transform_tiny(P,CFORM()->getSphere().P);

			Fvector hit_dir; 
			hit_dir.set(	::Random.randF(-.5f,.5f), 
							::Random.randF(.0f,1.f), 
							::Random.randF(-.5f,.5f)); 
			hit_dir.normalize();

			Fvector position_in_bone_space;

			VERIFY(!pGameObject->getDestroy());

			float dist = pGameObject->Position().distance_to(P) - pGameObject->Radius();
			float power = m_fSecondaryHitPower * RelativePower(dist>0.f?dist:0.f, Radius());
			if(power<0.0f)
				return;

			float impulse = m_fHitImpulseScale*power*pGameObject->GetMass();
			position_in_bone_space.set(0.f,0.f,0.f);
			CreateHit(pGameObject->ID(),ID(),hit_dir,power,0,position_in_bone_space,impulse,m_eHitTypeBlowout);
		}
	}
}
