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
 @file   MMTString.hpp
 @brief  TLV-SI / MMT-SI の文字列
 @author cngu-dtv
*/


#ifndef LIBISDB_MMT_STRING_H
#define LIBISDB_MMT_STRING_H


#include "../Base/ARIBString.hpp"


namespace LibISDB
{

	/*
		TLV-SI / MMT-SI の文字列 (UTF-8、ARIB TR-B39 第1部 第4編 4章) を変換する
		CR (U+000D) は改行とし、それ以外の制御文字は除く
	*/
	bool DecodeMMTString(
		const uint8_t *pData, size_t Length, ReturnArg<String> Dst,
		ARIBStringDecoder::DecodeFlag Flags = ARIBStringDecoder::DecodeFlag::None);
	bool DecodeMMTString(
		const ARIBString &Src, ReturnArg<String> Dst,
		ARIBStringDecoder::DecodeFlag Flags = ARIBStringDecoder::DecodeFlag::None);

} // namespace LibISDB


#endif // ifndef LIBISDB_MMT_STRING_H
