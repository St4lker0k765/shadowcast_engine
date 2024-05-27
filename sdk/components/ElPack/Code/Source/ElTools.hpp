// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTools.pas' rev: 6.00

#ifndef ElToolsHPP
#define ElToolsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
/*
#include <ElStrUtils.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <MMSystem.hpp>	// Pascal unit
#include <ActiveX.hpp>	// Pascal unit
#include <ShellAPI.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit
*/
#include <string>
//-- user supplied -----------------------------------------------------------

namespace Eltools
{
//-- type declarations -------------------------------------------------------
class TDirectMemoryStream;
class TDirectMemoryStream : public Classes::TMemoryStream 
{
	typedef Classes::TMemoryStream inherited;
	
public:
	HIDESBASE void __fastcall SetPointer(void * Ptr, int Size);
public:
	#pragma option push -w-inl
	/* TMemoryStream.Destroy */ inline __fastcall virtual ~TDirectMemoryStream(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TDirectMemoryStream(void) : Classes::TMemoryStream() { }
	#pragma option pop
	
};


typedef TDirectMemoryStream TElMemoryStream;
;

class TNamedFileStream;
class TNamedFileStream : public Classes::TFileStream 
{
	typedef Classes::TFileStream inherited;
	
private:
	std::string FFileName;
	
public:
	__fastcall TNamedFileStream(const std::string FileName, Word Mode);
	__property std::string FileName = {read=FFileName};
public:
	#pragma option push -w-inl
	/* TFileStream.Destroy */ inline __fastcall virtual ~TNamedFileStream(void) { }
	#pragma option pop
	
};


#pragma pack(push, 2)
struct TReducedDateTime
{
	Word Year;
	Word Month;
	Word DOW;
	Word Day;
	Word Hour;
	Word Min;
} ;
#pragma pack(pop)

typedef void __fastcall (__closure *TMsgPumpRoutineEvent)(void);

typedef void __fastcall (__closure *TElWndMethod)(Messages::TMessage &Message);

typedef std::string ElTools__3[38];

//-- var, const, procedure ---------------------------------------------------
static const char MAXPATHLEN = 0x400;
extern  TMsgPumpRoutineEvent OnMessagePump;
extern  bool IsLinux;
extern  bool IsWin95;
extern  bool IsWinNT;
extern  bool IsWin2000;
extern  bool IsWinNTUp;
extern  bool IsWin2000Up;
extern  bool IsWinXP;
extern  bool IsWinXPUp;
extern  bool IsWin95OSR2;
extern  bool IsWin98;
extern  bool IsWinME;
extern  bool IsWin98Up;
extern  HWND LastWin;
extern  unsigned LastProcessID;
extern  std::string ElementFormatList[38];
extern  void __fastcall PlaySound(char * Name, unsigned Flags1, unsigned Flags2);
extern  char __fastcall swapInt16(char w);
extern  int __fastcall swapInt32(int i);
extern  double __fastcall SwapDouble(double d);
extern  int __fastcall GetSysStartDayOfWeek(void);
extern  time_t __fastcall GetTime(time_t DateTime);
extern  std::string __fastcall GetCommonAppDataFolder();
extern  std::string __fastcall GetUserAppDataFolder();
extern  std::string __fastcall GetUserLocalAppDataFolder();
extern  std::string __fastcall GetSpecialFolder(const int CSIDL);
extern  std::string __fastcall IncludeTrailingBackslash2(const std::string Path);
extern  std::string __fastcall GetTempFile();
extern  bool __fastcall TimeInMask(std::string CronMask, const TReducedDateTime &T);
extern  std::string __fastcall GetSystemDir();
extern  std::string __fastcall GetShortPath(std::string Path);
extern  std::string __fastcall GetTempDir();
extern  std::string __fastcall GetWindowsDir();
extern  std::string __fastcall GetFormattedTimeString(System::TDateTime ADate, std::string Format);
extern  int __fastcall DayNumber(int AYear, int AMonth, int ADay);
extern  int __fastcall WeekNumber(int AYear, int AMonth, int ADay);
extern  System::TDateTime __fastcall ExtractTime(System::TDateTime ATime);
extern  System::TDateTime __fastcall ExtractDate(System::TDateTime ATime);
extern  System::TDateTime __fastcall IncTime(System::TDateTime ATime, int Hours, int Minutes, int Seconds, int MSecs);
extern  void __fastcall CenterRects(int WS, int WT, int HS, int HT, Types::TRect &R);
extern  bool __fastcall ReadTextFromStream(Classes::TStream* S, std::string &Data);
extern  void __fastcall WriteTextToStream(Classes::TStream* S, std::string Data);
extern  std::string __fastcall encode_line(const void *buf, int size);
extern  bool __fastcall FileNameValid(std::string FileName);
extern  __int64 __fastcall GetFileSize(const std::string FileName);
extern  System::TDateTime __fastcall FileDateTime(const std::string FileName);
extern  bool __fastcall CreateFile(std::string FileName);
extern  void __fastcall EnsureDirExists(std::string RootName, std::string DirName);
extern  bool __fastcall DirExists(std::string DirName);
extern  bool __fastcall PurgeDir(std::string DirName);
extern  unsigned __fastcall RunProgram(std::string StartName, std::string Params, std::string StartDir);
extern  TReducedDateTime __fastcall MakeReducedDT(Word Year, Word Month, Word Day, Word DOW, Word Hour, Word Min);
extern  bool __fastcall CompareReducedDT(const TReducedDateTime &T1, const TReducedDateTime &T2);
extern  TReducedDateTime __fastcall DateTimeToReduced(System::TDateTime T);
extern  System::TDateTime __fastcall ReducedToDateTime(const TReducedDateTime &T);
extern  bool __fastcall IsBIn(int index, Byte storage);
extern  int __fastcall Sign(int a);
extern  bool __fastcall InRangeF(double L, double R, double x);
extern  bool __fastcall InRange(int L, int R, int x);
extern  int __fastcall Max(int a, int b);
extern  int __fastcall Min(int a, int b);
extern  System::TDateTime __fastcall SubtractTimes(System::TDateTime Time1, System::TDateTime Time2);
extern  bool __fastcall RangesIntersect(int L1, int R1, int L2, int R2);
extern  bool __fastcall WriteStringToStream(Classes::TStream* S, std::string Str);
extern  bool __fastcall ReadStringFromStream(Classes::TStream* S, std::string &Str);
extern  bool __fastcall WriteWideStringToStream(Classes::TStream* S, WideString Str);
extern  bool __fastcall ReadWideStringFromStream(Classes::TStream* S, WideString &Str);
extern  bool __fastcall WriteFStringToStream(Classes::TStream* S, WideString Str);
extern  bool __fastcall ReadFStringFromStream(Classes::TStream* S, WideString &Str);
extern  unsigned __fastcall ElDateTimeToSeconds(int ADay, int AMonth, int AYear, int AHours, int AMinute, int ASecond);
extern  void __fastcall ElSecondsToDateTime(unsigned Seconds, int &ADay, int &AMonth, int &AYear, int &AHours, int &AMinute, int &ASecond);
extern  int __fastcall DateToJulianDays(int ADay, int AMonth, int AYear);
extern  void __fastcall JulianDaysToDate(int &ADay, int &AMonth, int &AYear, int JulianDate);
extern  int __fastcall ElDayOfWeek(int ADay, int AMonth, int AYear);
extern  int __fastcall DaysPerMonth(int AYear, int AMonth);
extern  time_t __fastcall IncDate(time_t ADate, int Days, int Months, int Years);
extern  time_t __fastcall NowToUTC(void);
extern  void __fastcall UTCToZoneLocal(Windows::PTimeZoneInformation lpTimeZoneInformation, const _SYSTEMTIME &lpUniversalTime, _SYSTEMTIME &lpLocalTime);
extern  void __fastcall ZoneLocalToUTC(Windows::PTimeZoneInformation lpTimeZoneInformation, _SYSTEMTIME &lpUniversalTime, const _SYSTEMTIME &lpLocalTime);
extern  void __fastcall ElSystemTimeToTzSpecificLocalTime(Windows::PTimeZoneInformation lpTimeZoneInformation, _SYSTEMTIME &lpUniversalTime, _SYSTEMTIME &lpLocalTime);
extern  int __fastcall ZoneIDtoBias(std::string ZoneID);
extern  bool __fastcall SetPrivilege(std::string sPrivilegeName, bool bEnabled);
extern  HWND __fastcall WindowExists(std::string ClassName, std::string Caption, bool ExactMatch);
extern  HWND __fastcall TopWindowExists(std::string ClassName, std::string Caption, bool ExactMatch);
extern  std::string __fastcall AppendSlash(const std::string PathName);
extern  std::string __fastcall GetModulePath();
extern  std::string __fastcall GetComputerName();
extern  std::string __fastcall RectToString(const Types::TRect &Rect);
extern  Types::TRect __fastcall StringToRect(std::string AString);
extern  void __fastcall ValFloat(std::string Value, double Result, int &Error);
extern  std::string __fastcall StrFloat(double Value);
extern  HWND __fastcall XAllocateHWND(System::TObject* Obj, TElWndMethod WndMethod);
extern  void __fastcall XDeallocateHWND(HWND Wnd);
extern  int __fastcall GetKeysState(void);
extern  Classes::TShiftState __fastcall GetShiftState(void);

}	/* namespace Eltools */
using namespace Eltools;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTools
