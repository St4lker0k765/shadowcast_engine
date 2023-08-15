#pragma once

//# Module collision detail

#include "_vector3d.h"
#include "_types.h"

//#|DCS++|
struct IDetailCollision;

ENGINE_API extern xr_vector<IDetailCollision> level_detail_coll;
ENGINE_API extern float ps_detail_collision_dcs_dist;
ENGINE_API extern float ps_detail_collision_dcs_time;

using vector_detail_collision = xr_vector<IDetailCollision>;

struct IDetailCollision
{
public:
	Fvector pos{};
	u16 id{};
	float radius = ps_detail_collision_dcs_dist;
	float rot_time_in = ps_detail_collision_dcs_time;
	float rot_time_out = ps_detail_collision_dcs_time * 1.5f;

	//--#Explosion#--//
	bool is_explosion{}; //--#For explosion grenade or weapon grenade
	bool b_not_explosion_detail_collision{}; //--#Not collision detail from explosion

	float f_explosion_detail_collision_time_in{};  //--#Calculate explosion collision detail time in
	float f_explosion_detail_collision_time_out{}; //--#Calculate explosion collision detail time out
	float f_explosion_detail_collision_lowering_time{}; //--#Calculate time lowering explosion wave for collision detail
	//--#Explosion#--//

	IDetailCollision() = default;
	IDetailCollision(Fvector pos, u16 id, float radius = ps_detail_collision_dcs_dist, float rot_time_in = ps_detail_collision_dcs_time,
		float rot_time_out = ps_detail_collision_dcs_time * 1.5f, bool is_explosion = false, float f_explosion_detail_collision_lowering_time = 1.5f)
	{
		this->pos = pos;
		this->id = id;
		this->radius = radius;
		this->is_explosion = is_explosion;
		this->rot_time_in = rot_time_in;
		this->rot_time_out = rot_time_out;
		this->f_explosion_detail_collision_lowering_time = f_explosion_detail_collision_lowering_time;
	} //--#pos, id, radius, rot_time_in, rot_time_out, is_explosion, f_explosion_detail_collision_lowering_time#--//
};

static IDetailCollision* GetDetailCollisionPointByID(const u16& ID)
{
	for (auto& point : level_detail_coll)
		if (point.id == ID)
			return &point;

	return nullptr;
}

static void EraseDetailCollPointIfExists(const u16& ID)
{
	auto it = level_detail_coll.begin();
	for (; it != level_detail_coll.end(); ++it)
		if ((*it).id == ID)
		{
			level_detail_coll.erase(it);
			break;
		}
}

static bool DetailCollisionPointExist(const u16& ID)
{
	for (auto& i : level_detail_coll)
		if (i.id == ID)
			return true;
	return false;
}

//#|DCS++|