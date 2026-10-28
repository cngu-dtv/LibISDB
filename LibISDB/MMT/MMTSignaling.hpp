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
 @file   MMTSignaling.hpp
 @brief  MMT-SI 信号メッセージ
 @author cngu-dtv
*/


#ifndef LIBISDB_MMT_SIGNALING_H
#define LIBISDB_MMT_SIGNALING_H


#include "../TLV/TLVPacket.hpp"
#include "../TS/PSISection.hpp"


namespace LibISDB
{

	/** MMT-SI テーブルクラス (PA メッセージ / CA メッセージで伝送される形式、ARIB STD-B60 7.3.3) */
	class MMTTable
		: public DataBuffer
	{
	public:
		MMTTable() noexcept;

		bool operator == (const MMTTable &rhs) const noexcept;

		bool ParseHeader();
		void Reset() noexcept;

		uint8_t GetTableID() const noexcept { return m_TableID; }
		uint8_t GetVersion() const noexcept { return m_Version; }
		uint16_t GetTableLength() const noexcept { return m_TableLength; }
		const uint8_t * GetPayloadData() const noexcept;
		uint16_t GetPayloadSize() const noexcept;

	private:
		uint8_t m_TableID;      /**< table_id */
		uint8_t m_Version;      /**< version */
		uint16_t m_TableLength; /**< length */
	};

	/** MMT-SI 信号メッセージ解析クラス (packet_id 毎に使う) */
	class MMTSignalingParser
	{
	public:
		static constexpr size_t MAX_MESSAGE_SIZE = 4 * 1024 * 1024;

		class SignalingHandler
		{
		public:
			virtual ~SignalingHandler() = default;

			// PA メッセージ / CA メッセージのテーブル
			virtual void OnMMTTable(const MMTSignalingParser *pParser, const MMTTable *pTable) {}
			// M2 セクションメッセージ / M2 短セクションメッセージのセクション (CRC 検査済み)
			virtual void OnPSISection(const MMTSignalingParser *pParser, const PSISection *pSection) {}
		};

		MMTSignalingParser(SignalingHandler *pHandler);

		// payload_type が制御メッセージ以外のパケットは無視する
		void StorePacket(const TLVPacket *pPacket);
		void Reset() noexcept;

		uint16_t GetPacketID() const noexcept { return m_PacketID; }
		unsigned long GetCRCErrorCount() const noexcept { return m_CRCErrorCount; }

	private:
		void ParseMessage(const uint8_t *pData, size_t Size);
		void OnTable(const uint8_t *pData, size_t Size);
		void OnSection(const uint8_t *pData, size_t Size, bool IsExtended);

		SignalingHandler *m_pHandler;
		uint16_t m_PacketID;
		bool m_HasSequenceNumber;
		uint32_t m_NextSequenceNumber;
		bool m_IsMessageStoring;
		DataBuffer m_MessageBuffer;
		MMTTable m_Table;
		PSISection m_Section;
		unsigned long m_CRCErrorCount;
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_MMT_SIGNALING_H
