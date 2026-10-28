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
 @file   TLVConsts.hpp
 @brief  TLV の定数
 @author cngu-dtv
*/


#ifndef LIBISDB_TLV_CONSTS_H
#define LIBISDB_TLV_CONSTS_H


namespace LibISDB
{

	/*
		TLV パケット (ARIB STD-B32 第3部 3.5)
	*/
	constexpr uint8_t TLV_SYNC_BYTE = 0x7F_u8; // '01' + '111111'
	constexpr size_t TLV_HEADER_SIZE = 4;
	constexpr size_t TLV_PACKET_SIZE_MAX = TLV_HEADER_SIZE + 0xFFFF;

	/*
		packet_type
	*/
	constexpr uint8_t TLV_PACKET_TYPE_IPV4          = 0x01_u8; // IPv4 パケット
	constexpr uint8_t TLV_PACKET_TYPE_IPV6          = 0x02_u8; // IPv6 パケット
	constexpr uint8_t TLV_PACKET_TYPE_COMPRESSED_IP = 0x03_u8; // ヘッダ圧縮 IP パケット
	constexpr uint8_t TLV_PACKET_TYPE_SIGNALING     = 0xFE_u8; // 伝送制御信号パケット (TLV-SI)
	constexpr uint8_t TLV_PACKET_TYPE_NULL          = 0xFF_u8; // ヌルパケット

	/*
		ヘッダ圧縮 IP パケット (ARIB STD-B32 第3部 3.7 (1))
	*/
	constexpr size_t COMPRESSED_IP_HEADER_SIZE = 3; // CID(12) + SN(4) + CID_header_type(8)
	constexpr uint16_t CONTEXT_ID_MAX = 0x0FFF_u16;

	/*
		CID_header_type
	*/
	constexpr uint8_t CID_HEADER_TYPE_IPV4_PARTIAL = 0x20_u8; // 部分 IPv4 ヘッダ + 部分 UDP ヘッダ
	constexpr uint8_t CID_HEADER_TYPE_IPV4_ID      = 0x21_u8; // IPv4 ヘッダ識別子
	constexpr uint8_t CID_HEADER_TYPE_IPV6_PARTIAL = 0x60_u8; // 部分 IPv6 ヘッダ + 部分 UDP ヘッダ (フルヘッダ)
	constexpr uint8_t CID_HEADER_TYPE_NO_HEADER    = 0x61_u8; // 圧縮ヘッダ無し

	/*
		IP / UDP
	*/
	constexpr size_t IPV4_HEADER_SIZE_MIN = 20;
	constexpr size_t IPV6_HEADER_SIZE = 40;
	constexpr size_t UDP_HEADER_SIZE = 8;
	constexpr size_t IPV4_PARTIAL_HEADER_SIZE = IPV4_HEADER_SIZE_MIN - 4; // packet_length と header_checksum を除く
	constexpr size_t IPV6_PARTIAL_HEADER_SIZE = IPV6_HEADER_SIZE - 2;     // payload_length を除く
	constexpr size_t UDP_PARTIAL_HEADER_SIZE = 4;                         // data_length と checksum を除く
	constexpr uint8_t IP_PROTOCOL_UDP = 17_u8;

	/*
		NTP (ARIB STD-B60 3.2)
	*/
	constexpr uint16_t NTP_PORT = 123_u16;
	constexpr size_t NTP_PACKET_SIZE = 48;
	constexpr size_t NTP_TRANSMIT_TIMESTAMP_OFFSET = 40;

} // namespace LibISDB


#endif // ifndef LIBISDB_TLV_CONSTS_H
