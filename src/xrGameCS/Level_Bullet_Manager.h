// Level_Bullet_Manager.h:  ��� ����������� ������ ���� �� ����������
//							��� ���� � ������� ���������� ����
//////////////////////////////////////////////////////////////////////

#pragma once


#include "weaponammo.h"
#include "tracer.h"

//����������� � ��������� �������
struct SBullet_Hit 
{
	float	power;				// power          * cartridge
	float	power_critical;		// power_critical * cartridge
	float	impulse;			// impulse        * cartridge
};

//���������, ����������� ���� � �� �������� � ������
struct SBullet
{
	u32				init_frame_num			;			//����� ����� �� ������� ���� �������� ����
	union			{
		struct			{
			u16			ricochet_was	: 1	;			//���� �����������
			u16			explosive		: 1	;			//special explosive mode for particles
			u16			allow_tracer	: 1	;
			u16			allow_ricochet	: 1	;			//��������� �������
			u16			allow_sendhit	: 1	;			//statistics
//.			u16			skipped_frame	: 1	;			//������� ������ ���������
			u16			aim_bullet		: 1 ;			//����������� ����( ���������� ������ ����� ����������� �������� ������ (1-3 ���.))
		};
		u16				_storage			;
	}				flags				;
	u16				bullet_material_idx	;

	Fvector			bullet_pos			;			//������� �������
	Fvector			dir					;			
	float			speed				;			//������� ��������
	
	u16				parent_id			;			//ID ��������� ������� ���������� ��������
	u16				weapon_id			;			//ID ������ �� �������� ���� �������� ����
	
	float			fly_dist			;			//��������� ������� ���� ���������
	Fvector			tracer_start_position;
	
	Fvector			start_position		;
	Fvector			start_velocity		;
	u32				born_time			;
	float			life_time			;
	u32				change_rajectory_count;

	//����������� � ��������� �������
	SBullet_Hit     hit_param;
	//-------------------------------------------------------------------
	float			air_resistance		;
	//-------------------------------------------------------------------
	float			max_speed			;			// maxspeed*cartridge
	float			max_dist			;			// maxdist*cartridge
	float			armor_piercing		;			// ap
	float			wallmark_size		;
	//-------------------------------------------------------------------
	u8				m_u8ColorID			;
	
	//��� ���������� ����
	ALife::EHitType hit_type			;
	//---------------------------------
	u32				m_dwID				;
	ref_sound		m_whine_snd			;
	ref_sound		m_mtl_snd			;
	//---------------------------------
	u16				targetID			;
	//---------------------------------
	bool			density_mode		;
	float			density				;
	Fvector			begin_density		;
	bool			operator	==		(u32 ID){return	ID == m_dwID;}
public:
					SBullet				();
					~SBullet			();

	bool			CanBeRenderedNow	() const { return (Device.dwFrame > init_frame_num);}

	void			Init				(const	Fvector& position,
										const	Fvector& direction,
										float	start_speed,
										float	power,
										float	power_critical,
										float	impulse,
										u16		sender_id,
										u16		sendersweapon_id,
										ALife::EHitType e_hit_type,
										float	maximum_distance,
										const	CCartridge& cartridge,
										bool	SendHit);
};

class CLevel;

class CBulletManager
{
private:
	static float const parent_ignore_distance;

private:
	collide::rq_results		rq_storage;
	collide::rq_results		m_rq_results;

private:
	DEFINE_VECTOR						(ref_sound,SoundVec,SoundVecIt);
	DEFINE_VECTOR						(SBullet,BulletVec,BulletVecIt);
	friend	CLevel;

	enum EventType {
		EVENT_HIT	= u8(0),
		EVENT_REMOVE,

		EVENT_DUMMY = u8(-1),
	};
	struct	_event			{
		EventType			Type;
		BOOL				dynamic		;
		BOOL				Repeated	;	// ���������������� ��������� ��������� � ������������ ������
		SBullet_Hit			hit_result	;
		SBullet				bullet		;
		Fvector				normal		;
		Fvector				point		;
		collide::rq_result	R			;
		u16					tgt_material;
	};
	static void CalculateNewVelocity(Fvector & dest_new_vel, Fvector const & old_velocity, float ar, float life_time);
protected:
	SoundVec				m_WhineSounds		;
	RStringVec				m_ExplodeParticles	;

	//������ ���� ����������� � ������ ������ �� ������
//.	xrCriticalSection		m_Lock				;

	BulletVec				m_Bullets			;	// working set, locked
	BulletVec				m_BulletsRendered	;	// copy for rendering
	xr_vector<_event>		m_Events			;

#ifdef DEBUG
	u32						m_thread_id;

	typedef xr_vector<Fvector>	BulletPoints;
	BulletPoints			m_bullet_points;
#endif // #ifdef DEBUG

	//��������� ��������� �� ����
	CTracer					tracers;

	//����������� ��������, �� ������� ���� ��� ���������
	static float			m_fMinBulletSpeed;

	float					m_fHPMaxDist;

	//��������� G
	float					m_fGravityConst;
	//������������� �������, �������, ������� ���������� �� ��������
	//������ ����
	float					m_fAirResistanceK;
	//c������ ��������� ������� �������� ���� ��� ������������ � ���������� (��� ������� ��� ������ �����)
	float					m_fCollisionEnergyMin;
	//�������� ��������� ������� ��������� � ���� ��� ����� ������������
	float					m_fCollisionEnergyMax;

	//��������� ��������� ���������
	float					m_fTracerWidth;
	float 					m_fTracerLengthMax;
	float 					m_fTracerLengthMin;
protected:
	void					PlayWhineSound		(SBullet* bullet, CObject* object, const Fvector& pos);
	void					PlayExplodePS		(const Fmatrix& xf);
	//������� ��������� ����� ��������
	static BOOL 			test_callback		(const collide::ray_defs& rd, CObject* object, LPVOID params);
	static BOOL				firetrace_callback	(collide::rq_result& result, LPVOID params);

	// Deffer event
	void					RegisterEvent		(EventType Type, BOOL _dynamic, SBullet* bullet, const Fvector& end_point, collide::rq_result& R, u16 target_material);
	
	//��������� �� ������������� �������
	void					DynamicObjectHit	(_event& E);
	
	//��������� �� ������������ �������
	void					StaticObjectHit		(_event& E);

	//��������� �� ������ �������, �� ������ - ������� � ���� ���������� ����� �������
	bool					ObjectHit			(SBullet_Hit* hit_res, SBullet* bullet, const Fvector& end_point, 
												collide::rq_result& R, u16 target_material, Fvector& hit_normal);
	//������� �� ���������� �������
	void					FireShotmark		(SBullet* bullet, const Fvector& vDir, 
												const Fvector &vEnd,    collide::rq_result& R,  u16 target_material,
												const Fvector& vNormal, bool ShowMark = true);
	//������� ������ ���� �� ��������� ���������� �������
	//����������� ��� �� ���� ������� ���� �������� ������������
	//� ����������, � ����� �������� ����� ���������� �������
	//�������� � ��������� � ������ ���������� � �����
	//���������� true ���� ���� ���������� �����
	bool					trajectory_check_error	(
								Fvector& previous_position,
								collide::rq_results& rq_storage, 
								SBullet& bullet,
								float& low,
								float& high,
								Fvector const& gravity,
								float const air_resistance
							);
	void					add_bullet_point	(
								Fvector const& start_position,
								Fvector& previous_position,
								Fvector const& start_velocity,
								Fvector const& gravity,
								float const ait_resistance,
								float const current_time
							);
	bool					process_bullet		(
								collide::rq_results& rq_storage,
								SBullet& bullet,
								u32 delta_time
							);
	void 		__stdcall	UpdateWorkload		();
public:
							CBulletManager		();
	virtual					~CBulletManager		();

	void 					Load				();
	void 					Clear				();
	void 					AddBullet			(const Fvector& position, const Fvector& direction, float starting_speed,
												float power, float power_critical, float impulse, 
												u16	sender_id, u16 sendersweapon_id,
												ALife::EHitType e_hit_type, float maximum_distance, 
												const CCartridge& cartridge, bool SendHit,bool AimBullet=false);

	void	__stdcall		CommitEvents		();	// @ the start of frame
	void					CommitRenderSet		();	// @ the end of frame
	void 					Render				();
};

struct bullet_test_callback_data
{
	Fvector			collide_position;
	SBullet*		pBullet;
	float			collide_time;
#if 1//def DEBUG
	float			high_time;
#endif // #ifdef DEBUG
};
