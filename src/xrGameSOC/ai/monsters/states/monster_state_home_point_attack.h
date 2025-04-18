#pragma once
#include "../state.h"

template<typename _Object>
class CStateMonsterAttackMoveToHomePoint : public CState<_Object> {
protected:
	typedef CState<_Object>		inherited;
	typedef CState<_Object>*	state_ptr;
	using inherited::add_state;
	using inherited::current_substate;
	using inherited::get_state;
	using inherited::get_state_current;
	using inherited::object;
	using inherited::prev_substate;
	using inherited::select_state;

	u32					m_target_node;
	bool				m_skip_camp;

public:
						CStateMonsterAttackMoveToHomePoint(_Object *obj);
	virtual	void		initialize				();
	virtual void 		finalize				();
	virtual void 		critical_finalize		();

	virtual bool		check_start_conditions	();
	virtual bool		check_completion		();

	virtual	void		reselect_state			();
	virtual	void		setup_substates			();
};

#include "monster_state_home_point_attack_inline.h"
