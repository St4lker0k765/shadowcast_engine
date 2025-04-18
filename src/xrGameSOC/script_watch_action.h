////////////////////////////////////////////////////////////////////////////
//	Module 		: script_watch_action.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script watch action class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_abstract_action.h"
#include "script_export_space.h"

namespace SightManager {
	enum ESightType : u32;
};

class CScriptGameObject;

class CScriptWatchAction : public CScriptAbstractAction {
public:
	enum EGoalType {
		eGoalTypeObject = static_cast<u32>(0),
		eGoalTypeWatchType,
		eGoalTypeDirection,
		eGoalTypeCurrent,
		eGoalTypeDummy = static_cast<u32>(-1),
	};

public:
	CObject						*m_tpObjectToWatch;
	SightManager::ESightType	m_tWatchType;
	EGoalType					m_tGoalType;
	Fvector						m_tWatchVector;
	shared_str						m_bone_to_watch;

	// Searchlight 
	Fvector						m_tTargetPoint;
	float						vel_bone_x;
	float						vel_bone_y;

public:
					CScriptWatchAction	();
	IC				CScriptWatchAction	(SightManager::ESightType tWatchType);
	IC				CScriptWatchAction	(SightManager::ESightType tWatchType, const Fvector &tDirection);
	IC				CScriptWatchAction	(SightManager::ESightType tWatchType, CScriptGameObject *tpObjectToWatch, LPCSTR bone_to_watch = "");
	// Searchlight look ///////////////////////////////////////////////
					CScriptWatchAction	(const Fvector &tTarget, float vel1, float vel2);
	IC				CScriptWatchAction	(CScriptGameObject *tpObjectToWatch, float vel1, float vel2);
	///////////////////////////////////////////////////////////////////
	virtual			~CScriptWatchAction	();
			void	SetWatchObject		(CScriptGameObject *tpObjectToWatch);
	IC		void	SetWatchType		(SightManager::ESightType tWatchType);
	IC		void	SetWatchDirection	(const Fvector &tDirection);
	IC		void	SetWatchBone		(LPCSTR bone_to_watch);
	IC		void	initialize			();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptWatchAction)
#undef script_type_list
#define script_type_list save_type_list(CScriptWatchAction)

#include "script_watch_action_inline.h"