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


// ARIB STD-B62 第1編 第2部 表 5-2 の符号位置 (JIS X0213 に無い ARIB STD-B24 の追加記号・漢字)
// 8 単位符号の既定の表記に置き換えるのはこれらだけで、JIS X0213 にもある記号 (♥ など) は送出側がそのまま符号化する
constexpr char32_t AdditionalSymbolTable[] = {
	0x0FD6, 0x213B, 0x2150, 0x2151, 0x2152, 0x2156, 0x2157, 0x2158, 0x2159, 0x215A,
	0x215B, 0x2189, 0x2474, 0x2475, 0x2476, 0x2477, 0x2478, 0x2479, 0x247A, 0x247B,
	0x247C, 0x247D, 0x247E, 0x247F, 0x2488, 0x2489, 0x248A, 0x248B, 0x248C, 0x248D,
	0x248E, 0x248F, 0x2490, 0x2491, 0x2492, 0x2493, 0x24B9, 0x24C8, 0x2613, 0x2614,
	0x2693, 0x269E, 0x269F, 0x26A1, 0x26BE, 0x26BF, 0x26C4, 0x26C5, 0x26C6, 0x26C7,
	0x26C8, 0x26C9, 0x26CA, 0x26CB, 0x26CC, 0x26CD, 0x26CF, 0x26D0, 0x26D1, 0x26D2,
	0x26D3, 0x26D4, 0x26D5, 0x26D6, 0x26D7, 0x26D8, 0x26D9, 0x26DA, 0x26DB, 0x26DC,
	0x26DD, 0x26DE, 0x26DF, 0x26E0, 0x26E1, 0x26E3, 0x26E8, 0x26E9, 0x26EA, 0x26EB,
	0x26EC, 0x26ED, 0x26EE, 0x26EF, 0x26F0, 0x26F1, 0x26F2, 0x26F3, 0x26F4, 0x26F5,
	0x26F6, 0x26F7, 0x26F8, 0x26F9, 0x26FA, 0x26FB, 0x26FC, 0x26FD, 0x26FE, 0x26FF,
	0x2708, 0x2757, 0x27A1, 0x27D0, 0x2A00, 0x2B05, 0x2B06, 0x2B07, 0x2B1B, 0x2B24,
	0x2B2E, 0x2B2F, 0x2B55, 0x2B56, 0x2B57, 0x2B58, 0x2B59, 0x3036, 0x322A, 0x322B,
	0x322C, 0x322D, 0x322E, 0x322F, 0x3230, 0x3233, 0x3236, 0x3237, 0x3244, 0x3245,
	0x3246, 0x3247, 0x3248, 0x3249, 0x324A, 0x324B, 0x324C, 0x324D, 0x324E, 0x324F,
	0x328B, 0x3299, 0x3371, 0x3390, 0x33A0, 0x33A2, 0x33A4, 0x33A5, 0x33CA, 0x3EDA,
	0x4103, 0x56E4, 0x5880, 0x6911, 0x6DCA, 0x6DF8, 0x7421, 0x7575, 0x79DA, 0x7FA1,
	0x813A, 0x8FF6, 0x91B2, 0x9592, 0x9AD9, 0x9FC4, 0x9FC5, 0x9FC6, 0xFA6B, 0xFA6C,
	0xFA6D, 0x1F100, 0x1F101, 0x1F102, 0x1F103, 0x1F104, 0x1F105, 0x1F106, 0x1F107, 0x1F108,
	0x1F109, 0x1F10A, 0x1F110, 0x1F111, 0x1F112, 0x1F113, 0x1F114, 0x1F115, 0x1F116, 0x1F117,
	0x1F118, 0x1F119, 0x1F11A, 0x1F11B, 0x1F11C, 0x1F11D, 0x1F11E, 0x1F11F, 0x1F120, 0x1F121,
	0x1F122, 0x1F123, 0x1F124, 0x1F125, 0x1F126, 0x1F127, 0x1F12A, 0x1F12B, 0x1F12C, 0x1F12D,
	0x1F131, 0x1F13D, 0x1F13F, 0x1F142, 0x1F146, 0x1F14A, 0x1F14B, 0x1F14C, 0x1F14D, 0x1F14E,
	0x1F157, 0x1F15F, 0x1F179, 0x1F17B, 0x1F17C, 0x1F17F, 0x1F18A, 0x1F18B, 0x1F18C, 0x1F18D,
	0x1F190, 0x1F200, 0x1F210, 0x1F211, 0x1F212, 0x1F213, 0x1F214, 0x1F215, 0x1F216, 0x1F217,
	0x1F218, 0x1F219, 0x1F21A, 0x1F21B, 0x1F21C, 0x1F21D, 0x1F21E, 0x1F21F, 0x1F220, 0x1F221,
	0x1F222, 0x1F223, 0x1F224, 0x1F225, 0x1F226, 0x1F227, 0x1F228, 0x1F229, 0x1F22A, 0x1F22B,
	0x1F22C, 0x1F22D, 0x1F22E, 0x1F22F, 0x1F230, 0x1F231, 0x1F240, 0x1F241, 0x1F242, 0x1F243,
	0x1F244, 0x1F245, 0x1F246, 0x1F247, 0x1F248, 0x20158, 0x20BB7, 0x233CC, 0x233FE, 0x242EE,
};


const InternalChar * GetSymbolText(char32_t Code)
{
	const SymbolText *pEnd = std::end(SymbolTextTable);
	const SymbolText *p = std::lower_bound(
		std::begin(SymbolTextTable), pEnd, Code,
		[](const SymbolText &Entry, char32_t Code) { return Entry.Code < Code; });

	if ((p != pEnd) && (p->Code == Code))
		return p->pText;

	if (!std::binary_search(std::begin(AdditionalSymbolTable), std::end(AdditionalSymbolTable), Code))
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
