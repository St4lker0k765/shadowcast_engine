////////////////////////////////////////////////////////////////////////////
//	Module 		: eatable_item.cpp
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Yuri Dobronravin
//	Description : Eatable item
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "eatable_item.h"
#include "xrmessages.h"
#include "physic_item.h"
#include "Level.h"
#include "entity_alive.h"
#include "EntityCondition.h"
#include "InventoryOwner.h"
#include "HUDManager.h"
#include "UIGameCustom.h"
#include "ui/UIActorMenu.h"

CEatableItem::CEatableItem()
{
	m_fHealthInfluence = 0;
	m_fPowerInfluence = 0;
	m_fSatietyInfluence = 0;
	m_fRadiationInfluence = 0;


	m_physic_item	= 0;

	m_iMaxUses = 1;
	m_iRemainingUses = 1;
	m_bRemoveAfterUse = true;
}

CEatableItem::~CEatableItem()
{
}

DLL_Pure *CEatableItem::_construct	()
{
	m_physic_item	= smart_cast<CPhysicItem*>(this);
	return			(inherited::_construct());
}

void CEatableItem::Load(LPCSTR section)
{
	inherited::Load(section);

	m_fHealthInfluence			= pSettings->r_float(section, "eat_health");
	m_fPowerInfluence			= pSettings->r_float(section, "eat_power");
	m_fSatietyInfluence			= pSettings->r_float(section, "eat_satiety");
	m_fRadiationInfluence		= pSettings->r_float(section, "eat_radiation");
	m_fWoundsHealPerc			= pSettings->r_float(section, "wounds_heal_perc");
	clamp						(m_fWoundsHealPerc, 0.f, 1.f);
	m_fMaxPowerUpInfluence		= READ_IF_EXISTS	(pSettings,r_float,section,"eat_max_power",0.0f);
	m_fWeightFull = m_weight;
	m_fWeightEmpty = READ_IF_EXISTS(pSettings, r_float, section, "empty_weight", 0.0f);

	m_iRemainingUses = m_iMaxUses = READ_IF_EXISTS( pSettings, r_u16, section, "max_uses", 1 );
	m_bRemoveAfterUse = READ_IF_EXISTS( pSettings, r_bool, section, "remove_after_use", TRUE );

	if ( IsUsingCondition())
	{
		SetCondition(( float ) m_iRemainingUses / ( float ) m_iMaxUses );
	}
}


void CEatableItem::load(IReader& packet)
{
	inherited::load(packet);

	m_iRemainingUses = packet.r_u16();
}

void CEatableItem::save(NET_Packet& packet)
{
	inherited::save(packet);

	packet.w_u16(m_iRemainingUses);
}
BOOL CEatableItem::net_Spawn				(CSE_Abstract* DC)
{
	if (!inherited::net_Spawn(DC)) return FALSE;

	if (IsUsingCondition())
	{
		SetCondition((float)m_iRemainingUses / (float)m_iMaxUses);
	}

	return TRUE;
};

bool CEatableItem::Useful() const
{
	if(!inherited::Useful()) return false;

	//проверить не все ли еще съедено
	if(m_iRemainingUses == 0) return false;

	return true;
}

void CEatableItem::OnH_A_Independent() 
{
	inherited::OnH_A_Independent();
	if(!Useful()) {
		if (object().Local() && OnServer())	object().DestroyObject	();
	}	
}

void CEatableItem::OnH_B_Independent(bool just_before_destroy)
{
	if(!Useful()) 
	{
		object().setVisible(FALSE);
		object().setEnabled(FALSE);
		if (m_physic_item)
			m_physic_item->m_ready_to_destroy	= true;
	}
	inherited::OnH_B_Independent(just_before_destroy);
}

void CEatableItem::UseBy (CEntityAlive* entity_alive)
{
	CInventoryOwner* IO	= smart_cast<CInventoryOwner*>(entity_alive);
	R_ASSERT		(IO);
	R_ASSERT		(m_pInventory==IO->m_inventory);
	R_ASSERT		(object().H_Parent()->ID()==entity_alive->ID());
	entity_alive->conditions().ChangeHealth		(m_fHealthInfluence);
	entity_alive->conditions().ChangePower		(m_fPowerInfluence);
	entity_alive->conditions().ChangeSatiety	(m_fSatietyInfluence);
	entity_alive->conditions().ChangeRadiation	(m_fRadiationInfluence);
	entity_alive->conditions().ChangeBleeding	(m_fWoundsHealPerc);
	
	entity_alive->conditions().SetMaxPower( entity_alive->conditions().GetMaxPower()+m_fMaxPowerUpInfluence );
	
	if (m_iRemainingUses > 0)
	{
		--m_iRemainingUses;
	}
	else
	{
		m_iRemainingUses = 0;
	}

	SetCondition((float)m_iRemainingUses / (float)m_iMaxUses);

	HUD().GetUI()->UIGame()->ActorMenu().RefreshConsumableCells();


}

float CEatableItem::Weight() 
{
	float res = inherited::Weight();

	if (IsUsingCondition())
	{
		float net_weight = m_fWeightFull - m_fWeightEmpty;
		float use_weight = m_iMaxUses > 0 ? (net_weight / m_iMaxUses) : 0.f;

		res = m_fWeightEmpty + (m_iRemainingUses * use_weight);
	}

	return res;
}
