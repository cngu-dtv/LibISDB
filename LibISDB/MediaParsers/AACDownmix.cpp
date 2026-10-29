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
#include "../Base/BitstreamReader.hpp"
#include <cmath>
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


namespace
{


constexpr uint8_t ANCILLARY_DATA_SYNC = 0xBC;      // ancillary_data_sync
constexpr uint8_t ANCILLARY_DATA_SYNC2 = 0xBD;     // ancillary_data_sync2 (ARIB STD-B32 第 2 部 6.2.3 (3) (d))


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




float AACDownmixLevelToGain(uint8_t Index) noexcept
{
	// ISO/IEC 14496-3 AMD4 表 AMD4.8 (0 dB から -1.5 dB 刻み、7 は -∞)
	static const float Table[8] = {
		1.0f, 0.841f, 0.707f, 0.596f, 0.5f, 0.422f, 0.355f, 0.0f,
	};

	return Table[Index & 7];
}


float AACDownmixLFELevelToGain(uint8_t Index) noexcept
{
	// ISO/IEC 14496-3 AMD4 表 AMD4.9 (+10 dB から -∞)
	static const float Table[16] = {
		3.162f, 2.0f, 1.679f, 1.413f, 1.189f, 1.0f, 0.841f, 0.707f,
		0.596f, 0.5f, 0.316f, 0.178f, 0.1f, 0.032f, 0.01f, 0.0f,
	};

	return Table[Index & 15];
}


bool ParseAACAncillaryData(const uint8_t *pData, size_t Size, ReturnArg<AACDownmixCoefficients> Coefficients)
{
	if ((pData == nullptr) || (Size < 3) || !Coefficients)
		return false;

	BitstreamReader Reader(pData, Size);

	if (Reader.GetBits(8) != ANCILLARY_DATA_SYNC)
		return false;

	// bs_info
	Reader.Skip(2 + 2 + 2 + 1 + 1);          // mpeg_audio_type, dolby_surround_mode, drc_presentation_mode, pseudo_surround_enable, reserved
	Reader.Skip(3);                          // reserved
	const bool DownmixingLevelsStatus = Reader.GetFlag();
	const bool ExtensionStatus = Reader.GetFlag();
	const bool CompressionStatus = Reader.GetFlag();
	const bool CoarseTimecodeStatus = Reader.GetFlag();
	const bool FineTimecodeStatus = Reader.GetFlag();

	AACDownmixCoefficients Result = *Coefficients;

	if (DownmixingLevelsStatus) {
		// downmixing_levels_MPEG4()
		const bool CenterOn = Reader.GetFlag();
		const uint8_t Center = static_cast<uint8_t>(Reader.GetBits(3));
		const bool SurroundOn = Reader.GetFlag();
		const uint8_t Surround = static_cast<uint8_t>(Reader.GetBits(3));
		if (CenterOn || SurroundOn) {
			Result.HasMixLevels = true;
			if (CenterOn)
				Result.CenterMixLevel = Center;
			if (SurroundOn)
				Result.SurroundMixLevel = Surround;
		}
	}
	if (CompressionStatus)
		Reader.Skip(16);                     // audio_coding_mode_and_compression()
	if (CoarseTimecodeStatus)
		Reader.Skip(16);                     // coarse_grain_timecode()
	if (FineTimecodeStatus)
		Reader.Skip(16);                     // fine_grain_timecode()

	if (ExtensionStatus) {
		// MPEG4_ancillary_data_extension()
		Reader.Skip(1);                      // reserved
		const bool LevelsStatus = Reader.GetFlag();
		const bool GainsStatus = Reader.GetFlag();
		const bool LFELevelStatus = Reader.GetFlag();
		Reader.Skip(4);                      // reserved
		if (LevelsStatus) {
			Result.DmixA = static_cast<uint8_t>(Reader.GetBits(3));
			Result.DmixB = static_cast<uint8_t>(Reader.GetBits(3));
			Reader.Skip(2);                  // reserved
		}
		if (GainsStatus)
			Reader.Skip(7 + 1 + 7 + 1);      // dmx_gain_5, dmx_gain_2
		if (LFELevelStatus)
			Reader.Skip(4 + 4);              // dmix_lfe_idx, reserved
	}

	if (Reader.IsOverrun())
		return false;

	// ARIB の拡張 (22.2ch の係数)
	if (Reader.GetPos() + 16 <= Size * 8) {
		if (Reader.GetBits(8) == ANCILLARY_DATA_SYNC2) {
			if (Reader.GetFlag()) {          // ext_downmixing_level_status2
				const uint8_t C = static_cast<uint8_t>(Reader.GetBits(3));
				const uint8_t D = static_cast<uint8_t>(Reader.GetBits(3));
				const uint8_t E = static_cast<uint8_t>(Reader.GetBits(3));
				const uint8_t F = static_cast<uint8_t>(Reader.GetBits(3));
				const uint8_t G = static_cast<uint8_t>(Reader.GetBits(3));
				const uint8_t LFE = static_cast<uint8_t>(Reader.GetBits(4));
				if (Reader.IsOverrun())
					return false;
				Result.DmixC = C;
				Result.DmixD = D;
				Result.DmixE = E;
				Result.DmixF = F;
				Result.DmixG = G;
				Result.DmixLFE = LFE;
			}
		}
	}

	*Coefficients = Result;

	return true;
}


void DownmixAAC22_2chTo5_1ch(
	const int16_t *pSrc, int16_t *pDst, size_t SampleCount,
	const AACDownmixCoefficients &Coefficients) noexcept
{
	enum {
		FC, FLc, FRc, FL, FR, SiL, SiR, BL, BR, BC, LFE1, LFE2,
		TpFC, TpFL, TpFR, TpSiL, TpSiR, TpC, TpBL, TpBR, TpBC, BtFC, BtFL, BtFR,
	};

	static_assert(BtFR + 1 == AAC_22_2CH_CHANNEL_COUNT);

	const float g1 = AACDownmixLevelToGain(Coefficients.DmixC);
	const float g2 = AACDownmixLevelToGain(Coefficients.DmixD);
	const float g3 = AACDownmixLevelToGain(Coefficients.DmixE);
	const float g4 = AACDownmixLevelToGain(Coefficients.DmixF);
	const float g5 = AACDownmixLevelToGain(Coefficients.DmixG);
	const float g6 = AACDownmixLFELevelToGain(Coefficients.DmixLFE);

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
