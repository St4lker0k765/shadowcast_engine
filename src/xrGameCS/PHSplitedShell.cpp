#include "stdafx.h"
#include "../xrphysics/PhysicsShell.h"
#include "../xrPhysics/PHObject.h"
#include "../xrphysics/IPHWorld.h"
#include "PHInterpolation.h"
#include "../xrPhysics/PHShell.h"
#include "../xrPhysics/PHJoint.h"
#include "../xrPhysics/PHElement.h"
#include "PHSplitedShell.h"
#include "../xrPhysics/Physics.h"
#include "SpaceUtils.h"
void CPHSplitedShell::Collide()
{
	///////////////////////////////
	CollideStatic(dSpacedGeom(),CPHObject::SelfPointer());
	//near_callback(this,0,(dGeomID)dSpace(),ph_world->GetMeshGeom());
}

void CPHSplitedShell::get_spatial_params()
{
		spatialParsFromDGeom((dGeomID)m_space,spatial.sphere.P,AABB,spatial.sphere.R);
		if(spatial.sphere.R>m_max_AABBradius) spatial.sphere.R=m_max_AABBradius;
}

void CPHSplitedShell::DisableObject()
{
	CPHObject::deactivate();
}
