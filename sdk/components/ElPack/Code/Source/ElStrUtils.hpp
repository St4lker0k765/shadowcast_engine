// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElStrUtils.pas' rev: 6.00

#ifndef ElStrUtilsHPP
#define ElStrUtilsHPP
#include <string>
#include <xrCore/_vector3d.h>
#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
//#include <SysInit.hpp>	// Pascal unit
//#include <System.hpp>	// Pascal unit
#include <vector>
//-- user supplied -----------------------------------------------------------
typedef std::wstring TElFString;

namespace Elstrutils
{
//-- type declarations -------------------------------------------------------
typedef std::wstring TElFString;

typedef wchar_t TElFChar;

typedef wchar_t *PElFChar;

#pragma option push -b-
enum ElStrUtils__1 { wrfReplaceAll, wrfIgnoreCase };
#pragma option pop

enum TWideReplaceFlags { ElStrUtils__1, wrfReplaceAll, wrfIgnoreCase };

//-- var, const, procedure ---------------------------------------------------
#define oleaut "oleaut32.dll"
extern "C" wchar_t * __stdcall SysAllocStringLen(wchar_t * P, int Len);
extern "C" void __stdcall SysFreeString(wchar_t * S);
extern "C" int __stdcall SysStringLen(wchar_t * S);
extern  bool doti;
#define SPathDelimiters "/\\"
#define SWidePathDelimiters L"/\\"
extern  std::string __fastcall IntToStrFmt(int value);
extern  std::string __fastcall FloatToStrFmt(int value, int decims);
extern  std::string __fastcall IntToStrPad(int value, int MinSize);
extern  std::string __fastcall CenterStr(std::string Str, int len);
extern  std::string __fastcall OEMToStr(std::string S);
extern  std::string __fastcall StrToOEM(std::string S);
extern  int __fastcall MessageRes(int Txt, char * Title, char TextType, bool Sounds);
extern  bool __fastcall replace(std::string &Str, std::string SourceString, std::string DestString);
extern  std::string __fastcall ExtractWord(std::string str, int n);
extern  int __fastcall FstNonSpace(std::string str);
extern  int __fastcall NextWordBegin(std::string str, int CurrentPos);
extern  int __fastcall LastPos(std::string SubStr, std::string Strn);
extern  bool __fastcall LineIsEmpty(std::string str);
extern  std::string __fastcall CompleteLine(std::string Str, int FLen, char symb);
extern  std::string __fastcall PrefixLine(std::string Str, int FLen, char symb);
extern  std::string __fastcall MakeString(int FLen, std::string Seq);
extern  int __fastcall H2D(std::string S);
extern  int __fastcall H2DDef(const std::string S, int Def);
extern  int __fastcall Bin2Int(std::string S);
extern  int __fastcall Bin2IntDef(std::string S, int Default);
extern  std::string __fastcall Data2Str(void * Buffer, int BufLen);
extern  bool __fastcall Str2Data(std::string S, void * &Buffer, int &BufLen);
extern  bool __fastcall IsDigit(char ch);
extern  bool __fastcall IsDigitStr(const std::string S);
extern  bool __fastcall IsAlpha(char ch);
extern  bool __fastcall IsAlphaOrDigit(char ch);
extern  bool __fastcall IsAlphaStr(const std::string S);
extern  bool __fastcall IsIdentStr(const std::string S);
extern  std::string __fastcall ExtractStr(std::string &S, int SPos, int SLen);
extern  int __fastcall LeftBreak(std::string S, int Pos);
extern  std::string __fastcall EscapeString(std::string aString, std::string UnsafeChars, char EscapeChar);
extern  std::string __fastcall UnEscapeString(std::string aString, char EscapeChar);
extern  bool __fastcall StrStartsWith(char * Source, char * Seq);
extern  bool __fastcall ContainsAt(std::string Source, int Index, std::string Seq);
extern  bool __fastcall FileNameLike(std::string FileName, std::string Mask);
extern  bool __fastcall AnsiSameText(const std::string S1, const std::string S2);
extern  std::string __fastcall CurrToPrettyStr(const char Value);
extern  char * __fastcall PrettyStrToCurr(std::string Value);
extern  int __fastcall CurrSign(const char Value);
extern  char * __fastcall StringDup(std::string S);
extern  std::wstring __fastcall uni2uppers(std::wstring s);
extern  std::wstring __fastcall uni2lowers(std::wstring s);
extern  std::wstring __fastcall uni2upperf(std::wstring s);
extern  std::wstring __fastcall uni2lowerf(std::wstring s);
extern  wchar_t * __fastcall WideStringDup(std::wstring S);
extern  int __fastcall WidePos(const std::wstring Substr, const std::wstring S);
extern  wchar_t * __fastcall WideStrScan(const wchar_t * Str, wchar_t Chr);
extern  wchar_t * __fastcall WideStrRScan(const wchar_t * Str, wchar_t Chr);
extern  std::wstring __fastcall WideQuotedStr(const std::wstring S, wchar_t Quote);
extern  std::wstring __fastcall WideExtractQuotedStr(wchar_t * &Src, wchar_t Quote);
extern  wchar_t * __fastcall WideStrEnd(const wchar_t * Str);
extern  bool __fastcall WideSameText(const std::wstring S1, const std::wstring S2);
extern  int __fastcall WideCompareText(const std::wstring S1, const std::wstring S2);
extern  std::wstring __fastcall WideExtractStr(std::wstring &S, int SPos, int SLen);
extern  wchar_t * __fastcall WideStrCopy(wchar_t * Target, wchar_t * Source);
extern  wchar_t * __fastcall WideStrPCopy(wchar_t * Target, const std::wstring Source);
extern  int __fastcall WideStrComp(const wchar_t * S1, const wchar_t * S2);
extern  int __fastcall WideStrLComp(const wchar_t * Str1, const wchar_t * Str2, unsigned MaxLen);
extern  unsigned __fastcall WideStrLen(const wchar_t * Str);
extern  std::wstring __fastcall WideStrPas(const wchar_t * Source);
extern  void __fastcall WideMove(const void *Source, void *Dest, int Count);
extern  void __fastcall FillWord(void *X, int Count, char Value);
extern  void __fastcall FillWideChar(void *X, int Count, wchar_t Value);
extern  wchar_t * __fastcall WideStrMove(wchar_t * Dest, const wchar_t * Source, unsigned Count);
extern  wchar_t * __fastcall WideStrECopy(wchar_t * Dest, const wchar_t * Source);
extern  wchar_t * __fastcall WideStrLCopy(wchar_t * Dest, const wchar_t * Source, unsigned MaxLen);
extern  wchar_t * __fastcall WideStrLCat(wchar_t * Dest, const wchar_t * Source, unsigned MaxLen);
extern  wchar_t * __fastcall WideStrCat(wchar_t * Dest, const wchar_t * Source);
extern  void __fastcall SetWideString(std::wstring &S, wchar_t * Buffer, int Len);
extern  int __fastcall CompareWideStr(const std::wstring S1, const std::wstring S2);
extern  bool __fastcall SameWideStr(const std::wstring S1, const std::wstring S2);
extern  wchar_t * __fastcall WideLastChar(const std::wstring S);
extern  wchar_t * __fastcall WideStrAlloc(unsigned Size);
extern  unsigned __fastcall WideStrBufSize(const wchar_t * Str);
extern  wchar_t * __fastcall WideStrNew(const wchar_t * Str);
extern  void __fastcall WideStrDispose(wchar_t * Str);
extern  std::wstring __fastcall WideUpperCase(const std::wstring S);
extern  std::wstring __fastcall WideLowerCase(const std::wstring S);
extern  bool __fastcall IsWideDelimiter(const std::wstring Delimiters, const std::wstring S, int Index);
extern  bool __fastcall IsWidePathDelimiter(const std::wstring S, int Index);
extern  std::wstring __fastcall IncludeWideTrailingDelimiter(const std::wstring S);
extern  std::wstring __fastcall ExcludeWideTrailingDelimiter(const std::wstring S);
extern  std::wstring __fastcall GetWideCharRangeString(wchar_t FirstChar, wchar_t LastChar);
extern  std::wstring __fastcall GetWideStringOf(wchar_t Char, unsigned Len);
extern  std::wstring __fastcall WideStringReplace(const std::wstring S, const std::wstring OldPattern, const std::wstring NewPattern, TWideReplaceFlags Flags);
extern  bool __fastcall WideReplace(std::wstring &Str, std::wstring SourceString, std::wstring DestString);
extern  wchar_t * __fastcall WideStrPos(const wchar_t * Str1, const wchar_t * Str2);
extern  std::wstring __fastcall WideCopy(std::wstring S, int SPos, int SLen);
extern  void __fastcall WideInsert(std::wstring Text, std::wstring &S, int SPos);
extern  void __fastcall WideDelete(std::wstring &S, int SPos, int SLen);
extern  std::wstring __fastcall WideMakeString(int FLen, std::wstring Seq);
extern  int __fastcall WideLastPos(std::wstring SubStr, std::wstring Strn);
extern  void __fastcall TStrDelete(std::wstring &S, int SPos, int SLen);
extern  std::wstring __fastcall TStrExtractStr(std::wstring &S, int SPos, int SLen);
extern  void __fastcall SetTStr(std::wstring &S, PElFChar Buffer, int Len);
extern  std::string __fastcall GetCharRangeString(char FirstChar, char LastChar);

}	/* namespace Elstrutils */
using namespace Elstrutils;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElStrUtils
