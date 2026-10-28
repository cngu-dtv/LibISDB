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
 @file   MMTPPacket.hpp
 @brief  MMTP パケット
 @author cngu-dtv
*/


#ifndef LIBISDB_MMTP_PACKET_H
#define LIBISDB_MMTP_PACKET_H


#include "MMTConsts.hpp"


namespace LibISDB
{

	/** MMTP パケットヘッダ (ARIB STD-B60 6.4.1.1) */
	struct MMTPHeader {
		uint8_t Version = 0;                    /**< version */
		bool PacketCounterFlag = false;         /**< packet_counter_flag */
		uint8_t FECType = 0;                    /**< FEC_type */
		bool ExtensionFlag = false;             /**< extension_flag */
		bool RAPFlag = false;                   /**< RAP_flag */
		uint8_t PayloadType = 0;                /**< payload_type */
		uint16_t PacketID = 0;                  /**< packet_id */
		uint32_t Timestamp = 0;                 /**< timestamp (NTP short format) */
		uint32_t PacketSequenceNumber = 0;      /**< packet_sequence_number */
		uint32_t PacketCounter = 0;             /**< packet_counter */
		uint16_t ExtensionType = 0;             /**< extension_type */
		uint16_t ExtensionLength = 0;           /**< extension_length */
		uint32_t ExtensionOffset = 0;           /**< header_extension_byte の位置 */
		uint32_t HeaderSize = 0;                /**< ヘッダ長 (MMTP_payload の位置) */

		// スクランブル情報 (ARIB STD-B61 3.2.8)
		bool HasScrambleInfo = false;           /**< hdr_ext_type 0x0001 がある */
		uint8_t ScramblingControl = 0;          /**< MMT_scrambling_control */
		bool HasScramblingSubsystemID = false;  /**< Scrambling_subsystem_identification_control */
		uint8_t ScramblingSubsystemID = 0;      /**< Scrambling_subsystem_identifier */
		bool HasMessageAuthentication = false;  /**< Message_authentication_control */
		uint16_t AuthenticatedPayloadLength = 0;/**< Payload_length */
		bool HasSICV = false;                   /**< MMT_scrambling_initial_counter_value_control */
		uint32_t SICVOffset = 0;                /**< MMT_scrambling_initial_counter_value の位置 */

		bool Parse(const uint8_t *pData, size_t Size);
		bool IsScrambled() const noexcept { return ScramblingControl >= MMT_SCRAMBLING_CONTROL_EVEN; }
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_MMTP_PACKET_H
