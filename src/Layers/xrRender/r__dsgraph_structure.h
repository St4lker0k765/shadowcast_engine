#pragma once

#include "../../xrEngine/render.h"
#include "../../xrcdb/ispatial.h"
#include "r__dsgraph_types.h"
#include "r__sector.h"
#include "tbb/concurrent_vector.h"

#ifdef _EDITOR
#include <xrEngine/Render.h>
#endif
class light;

//////////////////////////////////////////////////////////////////////////
// feedback	for receiving visuals										//
//////////////////////////////////////////////////////////////////////////
class	R_feedback
{
public:
	virtual		void	rfeedback_static	(dxRender_Visual*	V)		= 0;
};

//////////////////////////////////////////////////////////////////////////
// common part of interface implementation for all D3D renderers		//
//////////////////////////////////////////////////////////////////////////
class	R_dsgraph_structure										: public IRender_interface, public pureFrame
{
public:
	IRenderable*												val_pObject;
	Fmatrix*													val_pTransform;
	BOOL														val_bHUD;
	BOOL														val_bInvisible;
	BOOL														val_bRecordMP;		// record nearest for multi-pass
	R_feedback*													val_feedback;		// feedback for geometry being rendered
	u32															val_feedback_breakp;// breakpoint
	xr_vector<Fbox3>*						val_recorder;		// coarse structure recorder
	u32															phase;
	u32															marker;
	bool														pmask		[2]		;
	bool														pmask_wmark			;
public:
	// Dynamic scene graph
	//R_dsgraph::mapNormal_T										mapNormal	[2]		;	// 2==(priority/2)
	R_dsgraph::mapNormalPasses_T								mapNormalPasses	[2]	;	// 2==(priority/2)
	//R_dsgraph::mapMatrix_T										mapMatrix	[2]		;
	R_dsgraph::mapMatrixPasses_T								mapMatrixPasses	[2]	;
	R_dsgraph::mapSorted_T										mapSorted;
	R_dsgraph::mapHUD_T											mapHUD;
	R_dsgraph::mapLOD_T											mapLOD;
	R_dsgraph::mapSorted_T										mapDistort;
	R_dsgraph::mapHUD_T											mapHUDSorted;

#if RENDER!=R_R1
	R_dsgraph::mapSorted_T										mapWmark;			// sorted
	R_dsgraph::mapSorted_T										mapEmissive;
	R_dsgraph::mapSorted_T										mapHUDEmissive;
#endif

	// Runtime structures 
	xr_vector<R_dsgraph::mapNormalVS::value_type*>				nrmVS;
#if defined(USE_DX10) || defined(USE_DX11)
	xr_vector<R_dsgraph::mapNormalGS::value_type*>				nrmGS;
#endif	//	USE_DX10
	xr_vector<R_dsgraph::mapNormalPS::value_type*>				nrmPS;
	xr_vector<R_dsgraph::mapNormalCS::value_type*>				nrmCS;
	xr_vector<R_dsgraph::mapNormalStates::value_type*>		nrmStates;
	xr_vector<R_dsgraph::mapNormalTextures::value_type*>	nrmTextures;
	xr_vector<R_dsgraph::mapNormalTextures::value_type*>	nrmTexturesTemp;

	xr_vector<R_dsgraph::mapMatrixVS::value_type*>				matVS;
#if defined(USE_DX10) || defined(USE_DX11)
	xr_vector<R_dsgraph::mapMatrixGS::value_type*>				matGS;
#endif	//	USE_DX10
	xr_vector<R_dsgraph::mapMatrixPS::value_type*>				matPS;
	xr_vector<R_dsgraph::mapMatrixCS::value_type*>				matCS;
	xr_vector<R_dsgraph::mapMatrixStates::value_type*>		matStates;
	xr_vector<R_dsgraph::mapMatrixTextures::value_type*>	matTextures;
	xr_vector<R_dsgraph::mapMatrixTextures::value_type*>	matTexturesTemp;

	xr_vector<R_dsgraph::_LodItem>	lstLODs		;
	xr_vector<int>									lstLODgroups;
	xr_vector<ISpatial* /**,render_alloc<ISpatial*>/**/>				lstRenderables;
	xr_vector<ISpatial* /**,render_alloc<ISpatial*>/**/>				lstSpatial	;
	xr_vector<dxRender_Visual*>			lstVisuals	;

	tbb::concurrent_vector<IRenderable*> renderable_objects;
	tbb::concurrent_vector<light*> renderable_lights;

	xr_vector<dxRender_Visual*>			lstRecorded	;

	u32															counter_S	;
	u32															counter_D	;
	BOOL														b_loaded	;
public:
	virtual		void					set_Transform			(Fmatrix*	M	)				{ VERIFY(M);	val_pTransform = M;	}
	virtual		void					set_HUD					(BOOL 		V	)				{ val_bHUD		= V;				}
	virtual		BOOL					get_HUD					()								{ return		val_bHUD;			}
	virtual		void					set_Invisible			(BOOL 		V	)				{ val_bInvisible= V;				}
				void					set_Feedback			(R_feedback*V, u32	id)			{ val_feedback_breakp = id; val_feedback = V;		}
				void					set_Recorder			(xr_vector<Fbox3>* dest)		{ val_recorder	= dest;	if (dest) dest->clear();	}
				void					get_Counters			(u32&	s,	u32& d)				{ s=counter_S; d=counter_D;			}
				void					clear_Counters			()								{ counter_S=counter_D=0; 			}
public:
	R_dsgraph_structure	()
	{
		val_pObject			= NULL	;
		val_pTransform		= NULL	;
		val_bHUD			= FALSE	;
		val_bInvisible		= FALSE	;
		val_bRecordMP		= FALSE	;
		val_feedback		= 0;
		val_feedback_breakp	= 0;
		val_recorder		= 0;
		marker				= 0;
		r_pmask				(true,true);
		b_loaded			= FALSE	;
	};

	void		r_dsgraph_destroy()
	{
		nrmVS.clear				();
		nrmPS.clear				();
		nrmCS.clear				();
		nrmStates.clear			();
		nrmTextures.clear		();
		nrmTexturesTemp.clear	();

		matVS.clear				();
		matPS.clear				();
		matCS.clear				();
		matStates.clear			();
		matTextures.clear		();
		matTexturesTemp.clear	();

		lstLODs.clear			();
		lstLODgroups.clear		();
		lstRenderables.clear	();
		lstSpatial.clear		();
		lstVisuals.clear		();

		lstRecorded.clear		();

		//mapNormal[0].destroy	();
		//mapNormal[1].destroy	();
		//mapMatrix[0].destroy	();
		//mapMatrix[1].destroy	();
		for (int i=0; i<SHADER_PASSES_MAX; ++i)
		{
			mapNormalPasses[0][i].clear();
			mapNormalPasses[1][i].clear();
			mapMatrixPasses[0][i].clear();
			mapMatrixPasses[1][i].clear();
		}
		mapSorted.clear();
		mapHUD.clear();
		mapLOD.clear();
		mapDistort.clear();
		mapHUDSorted.clear();

#if RENDER!=R_R1
		mapWmark.clear();
		mapEmissive.clear();
		mapHUDEmissive.clear();
#endif
	}

	void		r_pmask											(bool _1, bool _2, bool _wm=false)				{ pmask[0]=_1; pmask[1]=_2;	pmask_wmark = _wm; }

	void		r_dsgraph_insert_dynamic						(dxRender_Visual	*pVisual, Fvector& Center);
	void		r_dsgraph_insert_static							(dxRender_Visual	*pVisual);

	void		r_dsgraph_render_graph							(u32	_priority,	bool _clear=true);
	void		r_dsgraph_render_hud							();
	void		r_dsgraph_render_hud_ui							();
	void		r_dsgraph_render_lods							(bool	_setup_zb,	bool _clear);
	void		r_dsgraph_render_sorted							();
	void        r_dsgraph_render_hud_sorted                     ();
	void		r_dsgraph_render_emissive						();
	void		r_dsgraph_render_wmarks							();
	void		r_dsgraph_render_distort						();
	void		r_dsgraph_render_subspace						(IRender_Sector* _sector, CFrustum* _frustum, Fmatrix& mCombined, Fvector& _cop, BOOL _dynamic, BOOL _precise_portals=FALSE	);
	void		r_dsgraph_render_subspace						(IRender_Sector* _sector, Fmatrix& mCombined, Fvector& _cop, BOOL _dynamic, BOOL _precise_portals=FALSE	);
	void		r_dsgraph_render_R1_box							(IRender_Sector* _sector, Fbox& _bb, int _element);


public:
	virtual		u32						memory_usage			()
	{
#ifdef USE_DOUG_LEA_ALLOCATOR_FOR_RENDER
		return	(g_render_lua_allocator.get_allocated_size());
#else // USE_DOUG_LEA_ALLOCATOR_FOR_RENDER
		return	(0);
#endif // USE_DOUG_LEA_ALLOCATOR_FOR_RENDER
	}
};
