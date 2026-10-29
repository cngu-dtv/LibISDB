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
 @file   LATMParser.hpp
 @brief  LATM 解析
 @author cngu-dtv
*/


#ifndef LIBISDB_LATM_PARSER_H
#define LIBISDB_LATM_PARSER_H


#include "../Base/BitstreamReader.hpp"
#include <vector>


namespace LibISDB
{

	/**
	 LATM の AudioMuxElement(1) の解析クラス (ISO/IEC 14496-3 1.7.3)

	 audioMuxVersionA = 0、numSubFrames = 0、numProgram = 0、numLayer = 0、
	 frameLengthType = 0 の構成のみ扱う (ARIB STD-B32 第 2 部 / TR-B39 の運用)。
	*/
	class LATMParser
	{
	public:
		struct AudioConfig {
			uint8_t AudioObjectType = 0;        /**< SBR / PS の明示的な指定がある場合は基本の AOT */
			uint8_t SamplingFrequencyIndex = 0;
			uint32_t SamplingFrequency = 0;
			uint8_t ChannelConfiguration = 0;   /**< 0 の場合は program_config_element() で指定 */
			std::vector<uint8_t> SpecificConfig; /**< AudioSpecificConfig() (バイト境界から始まるよう詰めたもの) */

			bool operator == (const AudioConfig &rhs) const noexcept = default;
		};

		LATMParser() noexcept;

		void Reset() noexcept;
		// 失敗した場合、設定は直前に解析できたものが残る
		bool ParseAudioMuxElement(const uint8_t *pData, size_t Size);
		bool HasConfig() const noexcept { return m_HasConfig; }
		const AudioConfig & GetConfig() const noexcept { return m_Config; }
		// 直前の ParseAudioMuxElement() で設定が変わったか
		bool IsConfigChanged() const noexcept { return m_ConfigChanged; }
		// raw_data_block() 等のペイロード (直前に成功した ParseAudioMuxElement() のもの)
		const uint8_t * GetPayloadData() const noexcept { return m_Payload.data(); }
		size_t GetPayloadSize() const noexcept { return m_Payload.size(); }

		static uint32_t GetSamplingFrequency(uint8_t Index) noexcept;

	private:
		bool ParseStreamMuxConfig(BitstreamReader &Reader, const uint8_t *pData, ReturnArg<AudioConfig> Config);
		static uint32_t GetLATMValue(BitstreamReader &Reader);
		static bool ParseAudioSpecificConfig(BitstreamReader &Reader, size_t StartPos, ReturnArg<AudioConfig> Config);
		static bool ParseProgramConfigElement(BitstreamReader &Reader, size_t StartPos);
		static void CopyBits(const uint8_t *pData, size_t BitPos, size_t BitCount, std::vector<uint8_t> *pDst);

		bool m_HasConfig;
		bool m_ConfigChanged;
		AudioConfig m_Config;
		std::vector<uint8_t> m_Payload;
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_LATM_PARSER_H
