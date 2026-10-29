/*
  LibISDB
  Copyright(c) 2026 cngu-dtv

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/**
 @file   MMTString.cpp
 @brief  TLV-SI / MMT-SI の文字列
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#if defined(LIBISDB_WINDOWS) && !defined(LIBISDB_WCHAR)
#include "../LibISDBWindows.hpp"
#endif
#include "MMTString.hpp"
#include <algorithm>
#include <iterator>
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


namespace
{

constexpr char32_t REPLACEMENT_CHARACTER = U'�';


// 不正な並びは U+FFFD にする
size_t DecodeUTF8Char(const uint8_t *pData, size_t Length, char32_t *pCode)
{
	const uint8_t c = pData[0];
	size_t Size;
	char32_t Code, Min;

	if (c < 0x80) {
		*pCode = c;
		return 1;
	}
	if ((c & 0xE0) == 0xC0) {
		Size = 2;
		Code = c & 0x1F;
		Min = 0x80;
	} else if ((c & 0xF0) == 0xE0) {
		Size = 3;
		Code = c & 0x0F;
		Min = 0x800;
	} else if ((c & 0xF8) == 0xF0) {
		Size = 4;
		Code = c & 0x07;
		Min = 0x10000;
	} else {
		*pCode = REPLACEMENT_CHARACTER;
		return 1;
	}

	if (Size > Length) {
		*pCode = REPLACEMENT_CHARACTER;
		return 1;
	}

	for (size_t i = 1; i < Size; i++) {
		if ((pData[i] & 0xC0) != 0x80) {
			*pCode = REPLACEMENT_CHARACTER;
			return 1;
		}
		Code = (Code << 6) | (pData[i] & 0x3F);
	}

	if ((Code < Min) || (Code > 0x10FFFF) || ((Code >= 0xD800) && (Code <= 0xDFFF)))
		Code = REPLACEMENT_CHARACTER;

	*pCode = Code;

	return Size;
}


typedef ARIBStringDecoder::InternalChar InternalChar;
typedef ARIBStringDecoder::InternalString InternalString;
#ifdef LIBISDB_ARIB_STR_IS_WCHAR
#define INTERNAL_NEWLINE LIBISDB_CAT_SYMBOL(L, LIBISDB_NEWLINE)
#else
#define INTERNAL_NEWLINE LIBISDB_NEWLINE
#endif


struct SymbolText {
	char32_t Code;
	const InternalChar *pText;
};

constexpr SymbolText SymbolTextTable[] = {
	{0x1F19B, LIBISDB_ARIB_STR("[3D]")},
	{0x1F19C, LIBISDB_ARIB_STR("[2ndScr]")},
	{0x1F19D, LIBISDB_ARIB_STR("[2K]")},
	{0x1F19E, LIBISDB_ARIB_STR("[4K]")},
	{0x1F19F, LIBISDB_ARIB_STR("[8K]")},
	{0x1F1A0, LIBISDB_ARIB_STR("[5.1]")},
	{0x1F1A1, LIBISDB_ARIB_STR("[7.1]")},
	{0x1F1A2, LIBISDB_ARIB_STR("[22.2]")},
	{0x1F1A3, LIBISDB_ARIB_STR("[60P]")},
	{0x1F1A4, LIBISDB_ARIB_STR("[120P]")},
	{0x1F1A5, LIBISDB_ARIB_STR("[d]")},
	{0x1F1A6, LIBISDB_ARIB_STR("[HC]")},
	{0x1F1A7, LIBISDB_ARIB_STR("[HDR]")},
	{0x1F1A8, LIBISDB_ARIB_STR("[Hi-Res]")},
	{0x1F1A9, LIBISDB_ARIB_STR("[Lossless]")},
	{0x1F1AA, LIBISDB_ARIB_STR("[SHV]")},
	{0x1F1AB, LIBISDB_ARIB_STR("[UHD]")},
	{0x1F1AC, LIBISDB_ARIB_STR("[VOD]")},
	{0x1F23B, LIBISDB_ARIB_STR("[配]")},
};


const InternalChar * GetSymbolText(char32_t Code)
{
	const SymbolText *pEnd = std::end(SymbolTextTable);
	const SymbolText *p = std::lower_bound(
		std::begin(SymbolTextTable), pEnd, Code,
		[](const SymbolText &Entry, char32_t Code) { return Entry.Code < Code; });

	if ((p != pEnd) && (p->Code == Code))
		return p->pText;

	if (Code < 0x100)
		return nullptr;

	return ARIBStringDecoder::GetSymbolAlternativeText(Code);
}


void AppendCodePoint(char32_t Code, InternalString *pString)
{
#if defined(LIBISDB_WINDOWS) || defined(LIBISDB_WCHAR)
	if constexpr (sizeof(wchar_t) == 2) {
		if (Code >= 0x10000) {
			pString->push_back(static_cast<wchar_t>(0xD800 | ((Code - 0x10000) >> 10)));
			pString->push_back(static_cast<wchar_t>(0xDC00 | ((Code - 0x10000) & 0x03FF)));
			return;
		}
	}
	pString->push_back(static_cast<wchar_t>(Code));
#else
	if (Code < 0x80) {
		pString->push_back(static_cast<char>(Code));
	} else if (Code < 0x800) {
		pString->push_back(static_cast<char>(0xC0 | (Code >> 6)));
		pString->push_back(static_cast<char>(0x80 | (Code & 0x3F)));
	} else if (Code < 0x10000) {
		pString->push_back(static_cast<char>(0xE0 | (Code >> 12)));
		pString->push_back(static_cast<char>(0x80 | ((Code >> 6) & 0x3F)));
		pString->push_back(static_cast<char>(0x80 | (Code & 0x3F)));
	} else {
		pString->push_back(static_cast<char>(0xF0 | (Code >> 18)));
		pString->push_back(static_cast<char>(0x80 | ((Code >> 12) & 0x3F)));
		pString->push_back(static_cast<char>(0x80 | ((Code >> 6) & 0x3F)));
		pString->push_back(static_cast<char>(0x80 | (Code & 0x3F)));
	}
#endif
}

}




bool DecodeMMTString(
	const uint8_t *pData, size_t Length, ReturnArg<String> Dst, ARIBStringDecoder::DecodeFlag Flags)
{
	if (!Dst)
		return false;

	Dst->clear();

	if ((pData == nullptr) && (Length > 0))
		return false;

	const bool UnicodeSymbol = !!(Flags & ARIBStringDecoder::DecodeFlag::UnicodeSymbol);
	InternalString String;
	String.reserve(Length);

	for (size_t Pos = 0; Pos < Length;) {
		char32_t Code;
		Pos += DecodeUTF8Char(&pData[Pos], Length - Pos, &Code);

		if (Code == U'\r') {
			String += INTERNAL_NEWLINE;
		} else if ((Code >= 0x20) && ((Code < 0x7F) || (Code >= 0xA0))) {
			const InternalChar *pText = UnicodeSymbol ? nullptr : GetSymbolText(Code);
			if (pText != nullptr)
				String += pText;
			else
				AppendCodePoint(Code, &String);
		}
	}

#if defined(LIBISDB_WINDOWS) && !defined(LIBISDB_WCHAR)
	if (!String.empty()) {
		const int DstLength = ::WideCharToMultiByte(
			CP_ACP, 0, String.data(), static_cast<int>(String.length()), nullptr, 0, nullptr, nullptr);
		if (DstLength > 0) {
			Dst->resize(DstLength);
			::WideCharToMultiByte(
				CP_ACP, 0, String.data(), static_cast<int>(String.length()), Dst->data(), DstLength, nullptr, nullptr);
		}
	}
#else
	*Dst = std::move(String);
#endif

	return true;
}


bool DecodeMMTString(const ARIBString &Src, ReturnArg<String> Dst, ARIBStringDecoder::DecodeFlag Flags)
{
	return DecodeMMTString(Src.data(), Src.length(), Dst, Flags);
}


} // namespace LibISDB
