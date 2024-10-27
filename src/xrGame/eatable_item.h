#pragma once

#include "inventory_item.h"

class CPhysicItem;
class CEntityAlive;

class CEatableItem : public CInventoryItem {
private:
	typedef CInventoryItem	inherited;

protected:
	CPhysicItem		*m_physic_item;

	u16				m_iMaxUses;
	u16				m_iRemainingUses;
	bool			m_bRemoveAfterUse;
	float m_fWeightFull;
	float m_fWeightEmpty;

public:
							CEatableItem				();
	virtual					~CEatableItem				();
	virtual	DLL_Pure*		_construct					();
	virtual CEatableItem	*cast_eatable_item			()	{return this;}

	virtual void			Load						(LPCSTR section);
	virtual void			load						(IReader& packet);
	virtual void			save						(NET_Packet& packet);
	virtual bool			Useful						() const;

	virtual BOOL			net_Spawn					(CSE_Abstract* DC);

	virtual void			OnH_B_Independent			(bool just_before_destroy);
	virtual void			OnH_A_Independent			();
	virtual	bool			UseBy						(CEntityAlive* npc);
			bool			Empty						() const { return m_iRemainingUses == 0; };
			bool			CanDelete					() const { return m_bRemoveAfterUse == true; };
	virtual u16				GetMaxUses					() const { return m_iMaxUses; };
	virtual u16				GetRemainingUses			() const { return m_iRemainingUses; };
	void SetRemainingUses(u16 value) { if (value <= m_iMaxUses && value >= 0) m_iRemainingUses = value; };
	virtual float Weight() const;
};

