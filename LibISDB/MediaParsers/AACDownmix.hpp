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
	 DSE で送られるダウンミックスの係数 (ISO/IEC 14496-3 AMD4 MPEG4_ancillary_data() と ARIB STD-B32 第 2 部 6.2.3 (3) の拡張)

	 値は係数の索引 (表 AMD4.8 / AMD4.9)。
	 送られるまでは既定値 (STD-B32 第 2 部 6.2.3 (3) (c) (d))。
	*/
	struct AACDownmixCoefficients {
		// 5.1ch → 2ch (downmixing_levels_MPEG4)
		bool HasMixLevels = false;
		uint8_t CenterMixLevel = 2;   /**< center_mix_level_value (-3 dB) */
		uint8_t SurroundMixLevel = 2; /**< surround_mix_level_value (-3 dB) */
		// 7.1ch 系 → 5.1ch (ext_downmixing_levels)
		uint8_t DmixA = 2;            /**< dmix_a_idx (-3 dB) */
		uint8_t DmixB = 2;            /**< dmix_b_idx (-3 dB) */
		// 22.2ch → 5.1ch (ARIB 拡張)
		uint8_t DmixC = 3;            /**< dmix_c_idx (-4.5 dB) */
		uint8_t DmixD = 3;            /**< dmix_d_idx (-4.5 dB) */
		uint8_t DmixE = 0;            /**< dmix_e_idx (0 dB) */
		uint8_t DmixF = 4;            /**< dmix_f_idx (-6 dB) */
		uint8_t DmixG = 2;            /**< dmix_g_idx (-3 dB) */
		uint8_t DmixLFE = 7;          /**< dmix_l_idx (-3 dB) */

		bool operator == (const AACDownmixCoefficients &rhs) const noexcept = default;
	};

	// 表 AMD4.8 の索引 (3 ビット) を係数にする
	float AACDownmixLevelToGain(uint8_t Index) noexcept;
	// 表 AMD4.9 の索引 (4 ビット) を係数にする
	float AACDownmixLFELevelToGain(uint8_t Index) noexcept;

	/**
	 DSE の MPEG4_ancillary_data() を解析して係数を更新する

	 送られていない項目は変更しない。
	*/
	bool ParseAACAncillaryData(const uint8_t *pData, size_t Size, ReturnArg<AACDownmixCoefficients> Coefficients);

	/**
	 AAC 22.2ch (channelConfiguration = 13) を 5.1ch にダウンミックスする (ARIB STD-B32 第 2 部 6.2.3 (3) (d))

	 入力は raw_data_block() の要素の送出順 (STD-B32 第 2 部 6.2.3 (1) の表) に並んだ 24ch、
	 出力は channelConfiguration = 6 の要素順 (C, L, R, Ls, Rs, LFE) の 6ch。
	*/
	void DownmixAAC22_2chTo5_1ch(
		const int16_t *pSrc, int16_t *pDst, size_t SampleCount,
		const AACDownmixCoefficients &Coefficients) noexcept;

} // namespace LibISDB


#endif // ifndef LIBISDB_AAC_DOWNMIX_H
