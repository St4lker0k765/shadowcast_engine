////////////////////////////////////////////////////////////////////////////
//	Module 		: script_value_container.h
//	Created 	: 16.07.2004
//  Modified 	: 16.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script value container
////////////////////////////////////////////////////////////////////////////
#pragma once
#ifdef XRSEFACTORY_EXPORTS
#include "script_space.h"
#include "stdafx.h"
#include "script_storage_space.h"
#include "luabind\luabind.hpp"
class CSE_Abstract;
#endif

	class CScriptValue { public: virtual ~CScriptValue() {} };

class CScriptValueContainer {
protected:
	xr_vector<CScriptValue*>				m_values;
#ifdef XRSEFACTORY_EXPORTS
	luabind::object			m_object;
	shared_str					m_name;
#endif
public:
	virtual			~CScriptValueContainer	();
	IC		void	assign					();
	IC		void	clear					();
	IC		void	add						(CScriptValue *value);
#ifdef XRSEFACTORY_EXPORTS
	IC						CScriptValue(luabind::object object, LPCSTR name);
	virtual	void			assign() = 0;
	IC		shared_str			name();
#endif
};
