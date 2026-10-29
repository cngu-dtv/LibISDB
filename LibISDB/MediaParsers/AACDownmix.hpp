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
 @file   AACDownmix.hpp
 @brief  AAC のダウンミックス
 @author cngu-dtv
*/


#ifndef LIBISDB_AAC_DOWNMIX_H
#define LIBISDB_AAC_DOWNMIX_H


#include <cstddef>
#include <cstdint>


namespace LibISDB
{

	constexpr int AAC_22_2CH_CHANNEL_COUNT = 24;

	/**
	 AAC 22.2ch (channelConfiguration = 13) を 5.1ch にダウンミックスする (ARIB STD-B32 第 2 部 6.2.3 (3) (d))

	 入力は raw_data_block() の要素の送出順 (STD-B32 第 2 部 6.2.3 (1) の表) に並んだ 24ch、
	 出力は channelConfiguration = 6 の要素順 (C, L, R, Ls, Rs, LFE) の 6ch。
	 係数は DSE で送られる値ではなく既定値を使う (STD-B32 第 2 部 6.4.2 (2))。
	*/
	void DownmixAAC22_2chTo5_1ch(const int16_t *pSrc, int16_t *pDst, size_t SampleCount) noexcept;

} // namespace LibISDB


#endif // ifndef LIBISDB_AAC_DOWNMIX_H
