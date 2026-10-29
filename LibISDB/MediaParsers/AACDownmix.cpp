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
 @file   AACDownmix.cpp
 @brief  AAC のダウンミックス
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "AACDownmix.hpp"
#include <cmath>
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


namespace
{


inline int16_t SaturateSample(float Value) noexcept
{
	const long Sample = std::lround(Value);
	if (Sample > 32767)
		return 32767;
	if (Sample < -32768)
		return -32768;
	return static_cast<int16_t>(Sample);
}


}




void DownmixAAC22_2chTo5_1ch(const int16_t *pSrc, int16_t *pDst, size_t SampleCount) noexcept
{
	enum {
		FC, FLc, FRc, FL, FR, SiL, SiR, BL, BR, BC, LFE1, LFE2,
		TpFC, TpFL, TpFR, TpSiL, TpSiR, TpC, TpBL, TpBR, TpBC, BtFC, BtFL, BtFR,
	};

	// ISO/IEC 14496-3 AMD4 表 AMD4.8 / AMD4.9 の値
	constexpr float g1 = 0.596f; // dmix_c_idx = 3 (-4.5 dB)
	constexpr float g2 = 0.596f; // dmix_d_idx = 3 (-4.5 dB)
	constexpr float g3 = 1.0f;   // dmix_e_idx = 0 (0 dB)
	constexpr float g4 = 0.5f;   // dmix_f_idx = 4 (-6 dB)
	constexpr float g5 = 0.707f; // dmix_g_idx = 2 (-3 dB)
	constexpr float g6 = 0.707f; // dmix_l_idx = 7 (-3 dB)

	static_assert(BtFR + 1 == AAC_22_2CH_CHANNEL_COUNT);

	for (size_t i = 0; i < SampleCount; i++) {
		const int16_t *s = pSrc;

		pDst[0] = SaturateSample(
			s[FC] + g1 * s[FLc] + g1 * s[FRc] + g3 * (s[TpFC] + g4 * s[TpC] + s[BtFC]));
		pDst[1] = SaturateSample(
			s[FL] + g1 * s[FLc] + g2 * s[SiL] + g3 * (s[TpFL] + g2 * s[TpSiL] + s[BtFL]));
		pDst[2] = SaturateSample(
			s[FR] + g1 * s[FRc] + g2 * s[SiR] + g3 * (s[TpFR] + g2 * s[TpSiR] + s[BtFR]));
		pDst[3] = SaturateSample(
			s[BL] + g5 * s[BC] + g2 * s[SiL] + g3 * (s[TpBL] + g5 * s[TpBC] + g2 * s[TpSiL] + g4 * s[TpC]));
		pDst[4] = SaturateSample(
			s[BR] + g5 * s[BC] + g2 * s[SiR] + g3 * (s[TpBR] + g5 * s[TpBC] + g2 * s[TpSiR] + g4 * s[TpC]));
		pDst[5] = SaturateSample(g6 * (s[LFE1] + s[LFE2]));

		pSrc += AAC_22_2CH_CHANNEL_COUNT;
		pDst += 6;
	}
}


} // namespace LibISDB
