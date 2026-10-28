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
 @file   TLVPacketParser.hpp
 @brief  TLV パケット解析
 @author cngu-dtv
*/


#ifndef LIBISDB_TLV_PACKET_PARSER_H
#define LIBISDB_TLV_PACKET_PARSER_H


#include "TLVPacket.hpp"
#include <vector>


namespace LibISDB
{

	/** TLV パケット解析クラス */
	class TLVPacketParser
	{
	public:
		/** パケット数 */
		struct PacketCountInfo {
			unsigned long long Input = 0;           /**< 切り出したパケット */
			unsigned long long FormatError = 0;     /**< フォーマットエラー */
			unsigned long long ContinuityError = 0; /**< MMTP packet_sequence_number の不連続 */
			unsigned long long Scrambled = 0;       /**< スクランブルされた MMTP パケット */
			unsigned long long IPv4 = 0;            /**< packet_type 0x01 */
			unsigned long long IPv6 = 0;            /**< packet_type 0x02 */
			unsigned long long CompressedIP = 0;    /**< packet_type 0x03 */
			unsigned long long Signaling = 0;       /**< packet_type 0xFE */
			unsigned long long Null = 0;            /**< packet_type 0xFF */
			unsigned long long Undefined = 0;       /**< 未定義の packet_type */
			unsigned long long MMTP = 0;            /**< MMTP パケット */
			unsigned long long NTP = 0;             /**< NTP パケット */
			unsigned long long SyncLoss = 0;        /**< 同期外れ */
			unsigned long long SkippedBytes = 0;    /**< 同期外で読み捨てたバイト数 */

			PacketCountInfo & operator += (const PacketCountInfo &rhs) noexcept;
			PacketCountInfo operator + (const PacketCountInfo &rhs) const noexcept
			{
				return PacketCountInfo(*this) += rhs;
			}

			void Reset() noexcept
			{
				*this = PacketCountInfo();
			}
		};

		/** パケットハンドラ */
		class PacketHandler
		{
		public:
			virtual ~PacketHandler() = default;
			virtual void OnTLVPacket(TLVPacket *pPacket, TLVPacket::ParseResult Result) = 0;
		};

		static constexpr int SYNC_CHAIN_COUNT = 3;

		/** 同期の判定結果 */
		enum class SyncResult {
			Found,        /**< 同期位置 */
			NeedMoreData, /**< データ不足で判定できない */
			Invalid,      /**< 同期位置ではない */
		};

		TLVPacketParser(PacketHandler *pPacketHandler);

		void Reset();
		size_t InputData(const uint8_t *pData, size_t Size, bool StopOnSyncLoss = false);

		bool IsSynced() const noexcept { return m_Synced; }
		unsigned long long GetUnsyncedBytes() const noexcept { return m_UnsyncedBytes; }
		const PacketCountInfo & GetPacketCount() const noexcept { return m_PacketCount; }
		void ResetPacketCount() noexcept { m_PacketCount.Reset(); }

		static SyncResult CheckSync(const uint8_t *pData, size_t Size, int ChainCount = SYNC_CHAIN_COUNT);
		static SyncResult FindSync(const uint8_t *pData, size_t Size, size_t *pPos);

	private:
		size_t ProcessSynced(const uint8_t *pData, size_t Size);
		void LoseSync();
		void OutputPacket();

		PacketHandler *m_pPacketHandler;
		TLVPacket m_Packet;
		bool m_Synced;
		std::vector<uint8_t> m_SyncBuffer;
		unsigned long long m_UnsyncedBytes;
		IPHeaderContextTable m_ContextTable;
		MMTPSequenceCounter m_SequenceCounter;
		PacketCountInfo m_PacketCount;
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_TLV_PACKET_PARSER_H
