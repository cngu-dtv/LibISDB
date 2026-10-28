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
 @file   MMTGeneralLocationInfo.hpp
 @brief  MMT_general_location_info
 @author cngu-dtv
*/


#ifndef LIBISDB_MMT_GENERAL_LOCATION_INFO_H
#define LIBISDB_MMT_GENERAL_LOCATION_INFO_H


#include "MMTConsts.hpp"
#include <array>
#include <string>


namespace LibISDB
{

	/** MMT_general_location_info (ARIB STD-B60 表 7-9) */
	struct MMTGeneralLocationInfo {
		static constexpr uint8_t LOCATION_TYPE_PACKET_ID     = 0x00_u8; // 同じ IP データフローの MMTP パケット
		static constexpr uint8_t LOCATION_TYPE_IPV4          = 0x01_u8; // IPv4 データフローの MMTP パケット
		static constexpr uint8_t LOCATION_TYPE_IPV6          = 0x02_u8; // IPv6 データフローの MMTP パケット
		static constexpr uint8_t LOCATION_TYPE_MPEG2_TS      = 0x03_u8; // MPEG-2 TS の TS パケット
		static constexpr uint8_t LOCATION_TYPE_MPEG2_TS_IPV6 = 0x04_u8; // IPv6 データフローの TS パケット
		static constexpr uint8_t LOCATION_TYPE_URL           = 0x05_u8; // URL
		static constexpr uint8_t LOCATION_TYPE_INVALID       = 0xFF_u8;

		uint8_t LocationType = LOCATION_TYPE_INVALID;    /**< location_type */
		uint16_t PacketID = MMT_PACKET_ID_INVALID;       /**< packet_id */
		std::array<uint8_t, 16> SourceAddress {};        /**< ipv4_src_addr (先頭 4 バイト) / ipv6_src_addr */
		std::array<uint8_t, 16> DestinationAddress {};   /**< ipv4_dst_addr (先頭 4 バイト) / ipv6_dst_addr */
		uint16_t DestinationPort = 0;                    /**< dst_port */
		uint16_t NetworkID = NETWORK_ID_INVALID;         /**< network_id */
		uint16_t TransportStreamID = TRANSPORT_STREAM_ID_INVALID; /**< MPEG_2_transport_stream_id */
		uint16_t PID = PID_INVALID;                      /**< MPEG_2_PID */
		std::string URL;                                 /**< URL_byte */

		bool operator == (const MMTGeneralLocationInfo &rhs) const noexcept = default;

		// 解析したバイト数を返す (失敗した場合は 0)
		size_t Parse(const uint8_t *pData, size_t Size);
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_MMT_GENERAL_LOCATION_INFO_H
