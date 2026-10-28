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
 @file   MMTSignaling.cpp
 @brief  MMT-SI 信号メッセージ
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "MMTSignaling.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Utilities/CRC.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


MMTTable::MMTTable() noexcept
	: m_TableID(0)
	, m_Version(0)
	, m_TableLength(0)
{
}


bool MMTTable::operator == (const MMTTable &rhs) const noexcept
{
	if (m_DataSize != rhs.m_DataSize)
		return false;
	if (m_DataSize == 0)
		return true;
	return std::memcmp(m_pData, rhs.m_pData, m_DataSize) == 0;
}


bool MMTTable::ParseHeader()
{
	if (m_DataSize < 4)
		return false;

	m_TableID     = m_pData[0];
	m_Version     = m_pData[1];
	m_TableLength = Load16(&m_pData[2]);

	return 4_z + m_TableLength <= m_DataSize;
}


void MMTTable::Reset() noexcept
{
	ClearSize();
	m_TableID = 0;
	m_Version = 0;
	m_TableLength = 0;
}


const uint8_t * MMTTable::GetPayloadData() const noexcept
{
	if (m_DataSize < 4)
		return nullptr;
	return &m_pData[4];
}


uint16_t MMTTable::GetPayloadSize() const noexcept
{
	if (m_DataSize < 4_z + m_TableLength)
		return 0;
	return m_TableLength;
}




MMTSignalingParser::MMTSignalingParser(SignalingHandler *pHandler)
	: m_pHandler(pHandler)
	, m_PacketID(MMT_PACKET_ID_INVALID)
	, m_HasSequenceNumber(false)
	, m_NextSequenceNumber(0)
	, m_IsMessageStoring(false)
	, m_Section(3 + 4093)
	, m_CRCErrorCount(0)
{
}


void MMTSignalingParser::StorePacket(const TLVPacket *pPacket)
{
	if ((pPacket == nullptr) || !pPacket->HasMMTPHeader())
		return;

	const MMTPHeader &Header = pPacket->GetMMTPHeader();
	if (Header.PayloadType != MMTP_PAYLOAD_TYPE_SIGNALING)
		return;

	const bool IsContinuous =
		m_HasSequenceNumber && (Header.PacketSequenceNumber == m_NextSequenceNumber);
	m_PacketID = Header.PacketID;
	m_HasSequenceNumber = true;
	m_NextSequenceNumber = Header.PacketSequenceNumber + 1;

	const uint8_t *pData = pPacket->GetMMTPPayloadData();
	size_t Size = pPacket->GetMMTPPayloadSize();

	if ((pData == nullptr) || (Size < 2)) {
		m_IsMessageStoring = false;
		return;
	}

	// 信号メッセージのペイロードヘッダ (ARIB STD-B60 表 6-1)
	const uint8_t FragmentationIndicator = pData[0] >> 6;
	const bool LengthExtensionFlag = (pData[0] & 0x02) != 0;
	const bool AggregationFlag = (pData[0] & 0x01) != 0;
	pData += 2;
	Size -= 2;

	switch (FragmentationIndicator) {
	case 0:
		m_IsMessageStoring = false;
		if (!AggregationFlag) {
			ParseMessage(pData, Size);
		} else {
			const size_t LengthSize = LengthExtensionFlag ? 4 : 2;
			for (size_t Pos = 0; Pos + LengthSize <= Size;) {
				const size_t Length = LengthExtensionFlag ? Load32(&pData[Pos]) : Load16(&pData[Pos]);
				Pos += LengthSize;
				if (Length > Size - Pos)
					break;
				ParseMessage(&pData[Pos], Length);
				Pos += Length;
			}
		}
		break;

	case 1:
		m_MessageBuffer.SetData(pData, Size);
		m_IsMessageStoring = true;
		break;

	case 2:
	case 3:
		if (!m_IsMessageStoring)
			break;
		if (!IsContinuous || (m_MessageBuffer.GetSize() + Size > MAX_MESSAGE_SIZE)) {
			m_IsMessageStoring = false;
			break;
		}
		m_MessageBuffer.AddData(pData, Size);
		if (FragmentationIndicator == 3) {
			m_IsMessageStoring = false;
			ParseMessage(m_MessageBuffer.GetData(), m_MessageBuffer.GetSize());
		}
		break;
	}
}


void MMTSignalingParser::Reset() noexcept
{
	m_PacketID = MMT_PACKET_ID_INVALID;
	m_HasSequenceNumber = false;
	m_NextSequenceNumber = 0;
	m_IsMessageStoring = false;
	m_MessageBuffer.ClearSize();
	m_Table.Reset();
	m_Section.Reset();
	m_CRCErrorCount = 0;
}


void MMTSignalingParser::ParseMessage(const uint8_t *pData, size_t Size)
{
	if (Size < 3)
		return;

	const uint16_t MessageID = Load16(&pData[0]);

	switch (MessageID) {
	case MMT_MESSAGE_ID_PA:
		{
			// number_of_tables と length は運用されないため、テーブル自身の長さでメッセージの終わりまで読む
			// (ARIB TR-B39 第1部 第4編 33.1)
			if (Size < 8)
				return;
			size_t Pos = 8 + pData[7] * 4_z;
			while (Pos + 4 <= Size) {
				const size_t TableSize = 4 + Load16(&pData[Pos + 2]);
				if (TableSize > Size - Pos)
					break;
				OnTable(&pData[Pos], TableSize);
				Pos += TableSize;
			}
		}
		break;

	case MMT_MESSAGE_ID_CA:
		if (Size >= 5 + 4) {
			const size_t TableSize = 4 + Load16(&pData[5 + 2]);
			if (TableSize <= Size - 5)
				OnTable(&pData[5], TableSize);
		}
		break;

	case MMT_MESSAGE_ID_M2_SECTION:
	case MMT_MESSAGE_ID_M2_SHORT_SECTION:
		if (Size >= 5 + 3)
			OnSection(&pData[5], Size - 5, MessageID == MMT_MESSAGE_ID_M2_SECTION);
		break;
	}
}


void MMTSignalingParser::OnTable(const uint8_t *pData, size_t Size)
{
	m_Table.SetData(pData, Size);
	if (!m_Table.ParseHeader())
		return;

	if (m_pHandler != nullptr)
		m_pHandler->OnMMTTable(this, &m_Table);
}


void MMTSignalingParser::OnSection(const uint8_t *pData, size_t Size, bool IsExtended)
{
	const size_t SectionSize = 3 + (Load16(&pData[1]) & 0x0FFF);
	if (SectionSize > Size)
		return;

	m_Section.SetData(pData, SectionSize);
	if (!m_Section.ParseHeader(IsExtended))
		return;

	// MH-DIT は CRC_32 を持たない
	if ((m_Section.GetTableID() != MMT_TABLE_ID_MH_DIT)
			&& (CRC32MPEG2::Calc(pData, SectionSize) != 0)) {
		if (m_CRCErrorCount < std::numeric_limits<unsigned long>::max())
			m_CRCErrorCount++;
		LIBISDB_TRACE_WARNING(
			LIBISDB_STR("MMTSignalingParser : CRC error | packet_id {:04X} / table_id {:02X}\n"),
			m_PacketID, m_Section.GetTableID());
		return;
	}

	if (m_pHandler != nullptr)
		m_pHandler->OnPSISection(this, &m_Section);
}


} // namespace LibISDB
