#pragma once
#include "weaponmagazined.h"
#include "rocketlauncher.h"


class CWeaponFakeGrenade;


class CWeaponMagazinedWGrenade : public CWeaponMagazined,
								 public CRocketLauncher
{
	typedef CWeaponMagazined inherited;
public:
					CWeaponMagazinedWGrenade	(ESoundTypes eSoundType=SOUND_TYPE_WEAPON_SUBMACHINEGUN);
	virtual			~CWeaponMagazinedWGrenade	();

	virtual void	Load				(LPCSTR section);
	
	virtual BOOL	net_Spawn			(CSE_Abstract* DC);
	virtual void	net_Destroy			();
	virtual void	net_Export			(NET_Packet& P);
	virtual void	net_Import			(NET_Packet& P);
	
	virtual void	OnH_B_Independent	(bool just_before_destroy);

	virtual void	save				(NET_Packet &output_packet);
	virtual void	load				(IReader &input_packet);
	virtual	bool    bMarkCanShow() { return IsZoomed() && !m_bGrenadeMode; }
	virtual	bool	bInZoomRightNow() const { return (m_zoom_params.m_fZoomRotationFactor > 0.05) && !m_bGrenadeMode; }


	virtual bool	Attach					(PIItem pIItem, bool b_send_event);
	virtual bool	Detach					(const char* item_section_name, bool b_spawn_item);
	virtual bool	CanAttach				(PIItem pIItem);
	virtual bool	CanDetach				(const char* item_section_name);
	virtual void	InitAddons				();
	virtual bool	UseScopeTexture			();
	virtual	float	CurrentZoomFactor		();
	virtual	u8		GetCurrentHudOffsetIdx	();
	virtual void	FireEnd					();
			void	LaunchGrenade			();
	
	virtual void	OnStateSwitch	(u32 S);
	
	virtual void	switch2_Reload	();
	virtual void    switch2_Unmis	();
	virtual void	state_Fire		(float dt);
	virtual void	OnShot			();
	virtual void	OnEvent			(NET_Packet& P, u16 type);
	virtual void	ReloadMagazine	();

	virtual bool	Action			(s32 cmd, u32 flags);

	virtual void	UpdateSounds	();

	//переключение в режим подствольника
	virtual bool	SwitchMode		();
	void			PerformSwitchGL	();
	void			OnAnimationEnd	(u32 state);
	virtual void	OnMagazineEmpty	();

	virtual bool	IsNecessaryItem	    (const shared_str& item_sect);

	//виртуальные функции для проигрывания анимации HUD
	virtual void	PlayAnimShow		();
	virtual void	PlayAnimHide		();
	virtual void	PlayAnimReload		();
	virtual void	PlayAnimIdle		();
	virtual void	PlayAnimShoot		();
	virtual void	PlayAnimModeSwitch	();
	virtual void	PlayAnimBore		();
	virtual void	PlayAnimFireMode	();

private:
	virtual	void	net_Spawn_install_upgrades	( Upgrades_type saved_upgrades );
	virtual bool	install_upgrade_impl		( LPCSTR section, bool test );
	virtual	bool	install_upgrade_ammo_class	( LPCSTR section, bool test );
	
public:
	//дополнительные параметры патронов 
	//для подствольника
	CWeaponAmmo*			m_pAmmo2;
	shared_str				m_ammoSect2;
	xr_vector<shared_str>	m_ammoTypes2;
	u8						m_ammoType2;
	shared_str				m_ammoName2;
	int						iMagazineSize2;
	xr_vector<CCartridge>	m_magazine2;
	bool					m_bGrenadeMode;

	CCartridge				m_DefaultCartridge2;
	int						iAmmoElapsed2;
	int						iAmmoElapsedMain;

	bool					IsMainMagazineEmpty() { return iAmmoElapsedMain <= 0; }

	virtual void UpdateGrenadeVisibility(bool visibility);
};