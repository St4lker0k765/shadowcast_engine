#pragma once
#include <string>
#include "xalloc.h"

// string(char)
class xr_string :public std::basic_string<char, std::char_traits<char>, xalloc<char> >
{
public:
	xr_string() {}
	xr_string(const std::basic_string<char, std::char_traits<char>, xalloc<char> >& right) { assign(right); }
	xr_string(const xr_string& right) { assign(right); }
	xr_string(xr_string&& right) { assign(right); }
	xr_string(const char* name) { assign(name); }
	xr_string(int a) { assign(std::to_string(a).c_str()); }
	inline xr_string& operator = (const std::basic_string<char, std::char_traits<char>, xalloc<char> >& right) { assign(right); return *this; }
	inline xr_string& operator = (const xr_string& right) { assign(right); return *this; }
	inline xr_string& operator = (xr_string&& right) { swap(right); return *this; }
	inline xr_string& __cdecl	sprintf(const char* format, ...)
	{
		string4096 	buf;
		va_list		p;
		va_start(p, format);
		int vs_sz = _vsnprintf(buf, sizeof(buf) - 1, format, p); buf[sizeof(buf) - 1] = 0;
		va_end(p);
		if (vs_sz)	assign(buf);
		return 		*this;
	}
};
