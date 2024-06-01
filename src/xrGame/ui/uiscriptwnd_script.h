#pragma once

template <typename T>
struct CWrapperBase : public T, public luabind::wrap_base {
	typedef T inherited;
	typedef CWrapperBase<T>	self_type;

	virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action)
	{ return call_member<bool>(this,"OnKeyboard", dik, keyboard_action);}
	static bool OnKeyboard_static(inherited* ptr, int dik, EUIMessages keyboard_action)
	{ 
		return ptr->self_type::inherited::OnKeyboardAction(dik,keyboard_action );
	}

	virtual bool OnGamepadAction(int vk, EUIMessages gamepad_action)
	{
		return call_member<bool>(this, "OnGamepad", vk, gamepad_action);
	}
	static bool OnGamepad_static(inherited* ptr, int vk, EUIMessages gamepad_action)
	{
		return ptr->self_type::inherited::OnGamepadAction(vk, gamepad_action);
	}

	virtual void Update()
	{ call_member<void>(this,"Update");}
	static void Update_static(inherited* ptr)
	{ ptr->self_type::inherited::Update();}

	virtual bool Dispatch(int cmd, int param)
	{ return call_member<bool>(this,"Dispatch", cmd, param);}
	static bool Dispatch_static(inherited* ptr, int cmd, int param)
	{ return ptr->self_type::inherited::Dispatch(cmd,param);}

};

typedef CWrapperBase<CUIDialogWndEx> WrapType;
typedef CUIDialogWndEx BaseType;

typedef luabind::class_<CUIDialogWndEx, WrapType, luabind::bases<CUIDialogWnd,DLL_Pure> > export_class;
