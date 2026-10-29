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
 @file   MMTPPayload.hpp
 @brief  MMTP ペイロード (MPU)
 @author cngu-dtv
*/


#ifndef LIBISDB_MMTP_PAYLOAD_H
#define LIBISDB_MMTP_PAYLOAD_H


#include "MMTConsts.hpp"
#include <vector>


namespace LibISDB
{

	constexpr size_t MPU_PAYLOAD_HEADER_SIZE   = 8;  // payload_length ～ MPU_sequence_number
	constexpr size_t MFU_TIMED_HEADER_SIZE     = 14; // movie_fragment_sequence_number ～ dependency_counter
	constexpr size_t MFU_NON_TIMED_HEADER_SIZE = 4;  // item_id

	/** MPU ペイロードヘッダ (ARIB STD-B60 表 6-1、payload_type 0x00) */
	struct MPUPayloadHeader {
		uint16_t PayloadLength = 0;         /**< payload_length */
		uint8_t FragmentType = 0;           /**< fragment_type */
		bool TimedFlag = false;             /**< timed_flag */
		uint8_t FragmentationIndicator = 0; /**< fragmentation_indicator */
		bool AggregationFlag = false;       /**< aggregation_flag */
		uint8_t FragmentCounter = 0;        /**< fragment_counter */
		uint32_t MPUSequenceNumber = 0;     /**< MPU_sequence_number */

		bool Parse(const uint8_t *pData, size_t Size);
	};

	/** MPU ペイロード中のデータユニット */
	struct MFUDataUnit {
		const uint8_t *pData; /**< MFU_data_byte */
		size_t Size;
		uint32_t ItemID;      /**< item_id (非時間指定のみ) */
	};

	/*
		MPU ペイロードからデータユニットを取り出す
		集約されていない場合は 1 つで、分割されたデータの断片のこともある
		MFU 以外 (MPU メタデータ等) の場合はデータユニットを返さない
	*/
	bool ParseMPUPayload(
		const uint8_t *pData, size_t Size,
		MPUPayloadHeader *pHeader, std::vector<MFUDataUnit> *pDataUnitList);

	/** 字幕・文字スーパーの MFU ヘッダ (ARIB STD-B60 表 9-1) */
	struct CaptionMFUHeader {
		uint8_t SubtitleTag = 0;            /**< subtitle_tag */
		uint8_t SubtitleSequenceNumber = 0; /**< subtitle_sequence_number */
		uint8_t SubsampleNumber = 0;        /**< subsample_number */
		uint8_t LastSubsampleNumber = 0;    /**< last_subsample_number */
		uint8_t DataType = 0;               /**< data_type */
		bool LengthExtensionFlag = false;   /**< length_extension_flag */
		bool SubsampleInfoListFlag = false; /**< subsample_info_list_flag */
		uint32_t DataSize = 0;              /**< data_size */
		size_t HeaderSize = 0;              /**< data_byte の位置 */

		// data_byte が Size に収まらない場合は false
		bool Parse(const uint8_t *pData, size_t Size);
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_MMTP_PAYLOAD_H
