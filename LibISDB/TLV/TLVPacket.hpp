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
 @file   TLVPacket.hpp
 @brief  TLV パケット
 @author cngu-dtv
*/


#ifndef LIBISDB_TLV_PACKET_H
#define LIBISDB_TLV_PACKET_H


#include "../Base/DataBuffer.hpp"
#include "../MMT/MMTPPacket.hpp"
#include "TLVConsts.hpp"
#include <array>
#include <vector>


namespace LibISDB
{

	/** ヘッダ圧縮 IP パケットのコンテキスト */
	struct IPHeaderContext {
		bool Valid = false;            /**< フルヘッダを受信済み */
		uint8_t IPVersion = 0;         /**< IP バージョン */
		uint16_t SourcePort = 0;       /**< 送信元ポート */
		uint16_t DestinationPort = 0;  /**< 宛先ポート */
	};

	/** ヘッダ圧縮 IP パケットのコンテキストテーブル (CID 毎) */
	typedef std::array<IPHeaderContext, CONTEXT_ID_MAX + 1> IPHeaderContextTable;

	/** MMTP パケットの packet_sequence_number の連続性判定 */
	class MMTPSequenceCounter
	{
	public:
		MMTPSequenceCounter();

		void Reset();
		bool Check(uint16_t PacketID, uint32_t SequenceNumber);

	private:
		std::vector<uint32_t> m_NextSequenceNumber;
		std::vector<bool> m_Valid;
	};

	/** TLV パケットクラス */
	class TLVPacket
		: public DataBuffer
	{
	public:
		static constexpr unsigned long TypeID = 0x544C5650UL; // 'TLVP'

		/** ParsePacket() の結果 */
		enum class ParseResult {
			OK,              /**< 正常パケット */
			FormatError,     /**< フォーマットエラー */
			ContinuityError, /**< packet_sequence_number の不連続(ドロップ) */
		};

		/** ペイロードの種類 */
		enum class PayloadKind {
			Unknown,   /**< 未解析 */
			MMTP,      /**< MMTP パケット */
			NTP,       /**< NTP */
			Signaling, /**< 伝送制御信号 (TLV-SI) */
			Null,      /**< ヌルパケット */
			Other,     /**< その他 */
		};

		TLVPacket();
		TLVPacket(const TLVPacket &Src);
		TLVPacket(TLVPacket &&Src) noexcept;

		TLVPacket & operator = (const TLVPacket &Src);
		TLVPacket & operator = (TLVPacket &&Src) noexcept;

		ParseResult ParsePacket(
			IPHeaderContextTable *pContextTable = nullptr,
			MMTPSequenceCounter *pSequenceCounter = nullptr);
		void ReparsePacket();

		uint8_t GetPacketType() const noexcept { return m_Header.PacketType; }
		uint16_t GetDataLength() const noexcept { return m_Header.DataLength; }
		PayloadKind GetPayloadKind() const noexcept { return m_Header.Kind; }

		bool IsIPPacket() const noexcept;
		uint8_t GetIPVersion() const noexcept { return m_Header.IPVersion; }
		bool IsHeaderCompressed() const noexcept { return m_Header.HeaderCompressed; }
		uint16_t GetContextID() const noexcept { return m_Header.ContextID; }
		uint8_t GetContextSequenceNumber() const noexcept { return m_Header.ContextSequenceNumber; }
		uint8_t GetContextHeaderType() const noexcept { return m_Header.ContextHeaderType; }
		bool HasUDPPorts() const noexcept { return m_Header.HasUDPPorts; }
		uint16_t GetSourcePort() const noexcept { return m_Header.SourcePort; }
		uint16_t GetDestinationPort() const noexcept { return m_Header.DestinationPort; }

		uint8_t * GetPayloadData() noexcept;
		const uint8_t * GetPayloadData() const noexcept;
		size_t GetPayloadSize() const noexcept { return m_Header.PayloadSize; }

		bool HasMMTPHeader() const noexcept { return m_Header.Kind == PayloadKind::MMTP; }
		const MMTPHeader & GetMMTPHeader() const noexcept { return m_Header.MMTP; }
		uint16_t GetPacketID() const noexcept { return m_Header.MMTP.PacketID; }
		uint8_t * GetMMTPPayloadData() noexcept;
		const uint8_t * GetMMTPPayloadData() const noexcept;
		size_t GetMMTPPayloadSize() const noexcept;
		bool IsScrambled() const noexcept { return HasMMTPHeader() && m_Header.MMTP.IsScrambled(); }

		uint64_t GetNTPTransmitTimestamp() const noexcept { return m_Header.NTPTimestamp; }

		static bool IsValidPacketType(uint8_t PacketType) noexcept;

	private:
		bool ParseIPv4(size_t Pos);
		bool ParseIPv6(size_t Pos);
		bool ParseCompressedIP(size_t Pos, IPHeaderContextTable *pContextTable);
		bool ParseUDP(size_t Pos, size_t Size);
		bool ParseUDPPayload(size_t Pos, size_t Size);

		struct HeaderInfo {
			uint8_t PacketType = 0;              /**< packet_type */
			uint16_t DataLength = 0;             /**< length */
			PayloadKind Kind = PayloadKind::Unknown;
			uint8_t IPVersion = 0;               /**< IP バージョン (不明なら 0) */
			bool HeaderCompressed = false;       /**< ヘッダ圧縮 IP パケット */
			uint16_t ContextID = 0;              /**< CID */
			uint8_t ContextSequenceNumber = 0;   /**< SN */
			uint8_t ContextHeaderType = 0;       /**< CID_header_type */
			bool HasUDPPorts = false;            /**< ポートが判明している */
			uint16_t SourcePort = 0;             /**< UDP 送信元ポート */
			uint16_t DestinationPort = 0;        /**< UDP 宛先ポート */
			uint32_t PayloadOffset = 0;          /**< ペイロード(UDP ペイロード / TLV-SI / ヌル)の位置 */
			uint32_t PayloadSize = 0;            /**< ペイロードのサイズ */
			MMTPHeader MMTP;                     /**< MMTP ヘッダ */
			uint64_t NTPTimestamp = 0;           /**< NTP transmit_timestamp */
		};

		HeaderInfo m_Header;
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_TLV_PACKET_H
