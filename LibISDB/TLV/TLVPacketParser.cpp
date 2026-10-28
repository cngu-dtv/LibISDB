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
 @file   TLVPacketParser.cpp
 @brief  TLV パケット解析
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "TLVPacketParser.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


TLVPacketParser::PacketCountInfo & TLVPacketParser::PacketCountInfo::operator += (const PacketCountInfo &rhs) noexcept
{
	Input           += rhs.Input;
	FormatError     += rhs.FormatError;
	ContinuityError += rhs.ContinuityError;
	Scrambled       += rhs.Scrambled;
	IPv4            += rhs.IPv4;
	IPv6            += rhs.IPv6;
	CompressedIP    += rhs.CompressedIP;
	Signaling       += rhs.Signaling;
	Null            += rhs.Null;
	Undefined       += rhs.Undefined;
	MMTP            += rhs.MMTP;
	NTP             += rhs.NTP;
	SyncLoss        += rhs.SyncLoss;
	SkippedBytes    += rhs.SkippedBytes;
	return *this;
}




TLVPacketParser::TLVPacketParser(PacketHandler *pPacketHandler)
	: m_pPacketHandler(pPacketHandler)
	, m_Synced(false)
	, m_UnsyncedBytes(0)
{
}


void TLVPacketParser::Reset()
{
	m_Packet.ClearSize();
	m_Synced = false;
	m_SyncBuffer.clear();
	m_UnsyncedBytes = 0;
	m_ContextTable.fill(IPHeaderContext());
	m_SequenceCounter.Reset();
	m_PacketCount.Reset();
}


size_t TLVPacketParser::InputData(const uint8_t *pData, size_t Size, bool StopOnSyncLoss)
{
	// StopOnSyncLoss が true の場合、同期が外れた位置で止めて処理したバイト数を返す
	if ((pData == nullptr) || (Size == 0))
		return 0;

	const size_t InputSize = Size;

	if (m_Synced) {
		const size_t Used = ProcessSynced(pData, Size);
		if (m_Synced)
			return InputSize;
		if (StopOnSyncLoss && m_SyncBuffer.empty())
			return Used;
		pData += Used;
		Size -= Used;
	}

	// 同期位置を探す
	m_SyncBuffer.insert(m_SyncBuffer.end(), pData, pData + Size);
	m_UnsyncedBytes += Size;

	for (;;) {
		size_t Pos;
		const SyncResult Result = FindSync(m_SyncBuffer.data(), m_SyncBuffer.size(), &Pos);

		m_PacketCount.SkippedBytes += Pos;

		if (Result != SyncResult::Found) {
			m_SyncBuffer.erase(m_SyncBuffer.begin(), m_SyncBuffer.begin() + Pos);
			break;
		}

		m_Synced = true;
		m_UnsyncedBytes = 0;

		std::vector<uint8_t> Buffer;
		Buffer.swap(m_SyncBuffer);

		const size_t Used = ProcessSynced(Buffer.data() + Pos, Buffer.size() - Pos);
		if (m_Synced)
			break;

		// 再び同期が外れた
		m_SyncBuffer.insert(m_SyncBuffer.end(), Buffer.begin() + (Pos + Used), Buffer.end());
		m_UnsyncedBytes += m_SyncBuffer.size();
	}

	return InputSize;
}


TLVPacketParser::SyncResult TLVPacketParser::CheckSync(const uint8_t *pData, size_t Size, int ChainCount)
{
	// 同期バイトと有効な packet_type が length を辿って ChainCount + 1 回続けば同期位置とする
	size_t Pos = 0;

	for (int i = 0;; i++) {
		if (Pos + 2 > Size)
			return SyncResult::NeedMoreData;
		if ((pData[Pos] != TLV_SYNC_BYTE) || !TLVPacket::IsValidPacketType(pData[Pos + 1]))
			return SyncResult::Invalid;
		if (i == ChainCount)
			break;
		if (Pos + TLV_HEADER_SIZE > Size)
			return SyncResult::NeedMoreData;
		Pos += TLV_HEADER_SIZE + Load16(&pData[Pos + 2]);
	}

	return SyncResult::Found;
}


size_t TLVPacketParser::ProcessSynced(const uint8_t *pData, size_t Size)
{
	size_t Pos = 0;

	if (m_Packet.GetSize() > 0) {
		// 途中まで受け取ったパケット
		size_t CurSize = m_Packet.GetSize();

		if (CurSize < TLV_HEADER_SIZE) {
			const size_t Remain = std::min(TLV_HEADER_SIZE - CurSize, Size);
			m_Packet.AddData(pData, Remain);
			Pos += Remain;
			CurSize += Remain;
		}

		if ((CurSize >= 2) && !TLVPacket::IsValidPacketType(m_Packet.GetData()[1])) {
			// 同期外れ
			const uint8_t *pPacketData = m_Packet.GetData();
			m_SyncBuffer.assign(pPacketData, pPacketData + CurSize);
			LoseSync();
			m_UnsyncedBytes = CurSize;
			return Pos;
		}

		if (CurSize < TLV_HEADER_SIZE)
			return Pos;

		const size_t PacketSize = TLV_HEADER_SIZE + Load16(&m_Packet.GetData()[2]);
		const size_t Remain = std::min(PacketSize - CurSize, Size - Pos);
		m_Packet.AddData(&pData[Pos], Remain);
		Pos += Remain;

		if (m_Packet.GetSize() < PacketSize)
			return Pos;

		OutputPacket();
	}

	while (Pos < Size) {
		const size_t Remain = Size - Pos;

		if ((pData[Pos] != TLV_SYNC_BYTE)
				|| ((Remain >= 2) && !TLVPacket::IsValidPacketType(pData[Pos + 1]))) {
			LoseSync();
			return Pos;
		}

		if (Remain < TLV_HEADER_SIZE) {
			m_Packet.SetData(&pData[Pos], Remain);
			return Size;
		}

		const size_t PacketSize = TLV_HEADER_SIZE + Load16(&pData[Pos + 2]);
		if (Remain < PacketSize) {
			m_Packet.SetData(&pData[Pos], Remain);
			return Size;
		}

		m_Packet.SetData(&pData[Pos], PacketSize);
		OutputPacket();
		Pos += PacketSize;
	}

	return Size;
}


TLVPacketParser::SyncResult TLVPacketParser::FindSync(const uint8_t *pData, size_t Size, size_t *pPos)
{
	// NeedMoreData の場合、*pPos は判定できなかった候補の位置
	size_t Pos = 0;

	while (Pos < Size) {
		const uint8_t *p = static_cast<const uint8_t *>(std::memchr(&pData[Pos], TLV_SYNC_BYTE, Size - Pos));
		if (p == nullptr)
			break;

		Pos = p - pData;

		const SyncResult Result = CheckSync(p, Size - Pos);
		if (Result != SyncResult::Invalid) {
			*pPos = Pos;
			return Result;
		}

		Pos++;
	}

	*pPos = Size;

	return SyncResult::Invalid;
}


void TLVPacketParser::LoseSync()
{
	m_Synced = false;
	m_Packet.ClearSize();
	m_UnsyncedBytes = 0;
	m_PacketCount.SyncLoss++;
}


void TLVPacketParser::OutputPacket()
{
	const TLVPacket::ParseResult Result = m_Packet.ParsePacket(&m_ContextTable, &m_SequenceCounter);

	m_PacketCount.Input++;

	switch (m_Packet.GetPacketType()) {
	case TLV_PACKET_TYPE_IPV4:          m_PacketCount.IPv4++;         break;
	case TLV_PACKET_TYPE_IPV6:          m_PacketCount.IPv6++;         break;
	case TLV_PACKET_TYPE_COMPRESSED_IP: m_PacketCount.CompressedIP++; break;
	case TLV_PACKET_TYPE_SIGNALING:     m_PacketCount.Signaling++;    break;
	case TLV_PACKET_TYPE_NULL:          m_PacketCount.Null++;         break;
	default:                            m_PacketCount.Undefined++;    break;
	}

	if (Result == TLVPacket::ParseResult::FormatError) {
		m_PacketCount.FormatError++;
	} else {
		if (Result == TLVPacket::ParseResult::ContinuityError)
			m_PacketCount.ContinuityError++;

		switch (m_Packet.GetPayloadKind()) {
		case TLVPacket::PayloadKind::MMTP:
			m_PacketCount.MMTP++;
			if (m_Packet.IsScrambled())
				m_PacketCount.Scrambled++;
			break;

		case TLVPacket::PayloadKind::NTP:
			m_PacketCount.NTP++;
			break;

		default:
			break;
		}
	}

	if (m_pPacketHandler != nullptr)
		m_pPacketHandler->OnTLVPacket(&m_Packet, Result);

	m_Packet.ClearSize();
}


} // namespace LibISDB
