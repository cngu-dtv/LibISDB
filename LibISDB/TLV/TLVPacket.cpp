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
 @file   TLVPacket.cpp
 @brief  TLV パケット
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "TLVPacket.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


namespace
{

// 通常の IP パケットは 1500 バイト以下 (ARIB TR-B39 第1部 第7編 5.5.5)
constexpr size_t TLV_PACKET_DEFAULT_BUFFER_SIZE = 2048;

}




MMTPSequenceCounter::MMTPSequenceCounter()
	: m_NextSequenceNumber(0x10000)
	, m_Valid(0x10000, false)
{
}


void MMTPSequenceCounter::Reset()
{
	std::fill(m_Valid.begin(), m_Valid.end(), false);
}


bool MMTPSequenceCounter::Check(uint16_t PacketID, uint32_t SequenceNumber)
{
	const bool Continuous = !m_Valid[PacketID] || (m_NextSequenceNumber[PacketID] == SequenceNumber);

	m_NextSequenceNumber[PacketID] = SequenceNumber + 1;
	m_Valid[PacketID] = true;

	return Continuous;
}




TLVPacket::TLVPacket()
{
	m_TypeID = TypeID;

	AllocateBuffer(TLV_PACKET_DEFAULT_BUFFER_SIZE);
}


TLVPacket::TLVPacket(const TLVPacket &Src)
{
	m_TypeID = TypeID;

	*this = Src;
}


TLVPacket::TLVPacket(TLVPacket &&Src) noexcept
{
	m_TypeID = TypeID;

	*this = std::move(Src);
}


TLVPacket & TLVPacket::operator = (const TLVPacket &Src)
{
	if (&Src != this) {
		DataBuffer::operator = (Src);
		m_Header = Src.m_Header;
	}

	return *this;
}


TLVPacket & TLVPacket::operator = (TLVPacket &&Src) noexcept
{
	if (&Src != this) {
		DataBuffer::operator = (std::move(Src));
		m_Header = Src.m_Header;
	}

	return *this;
}


TLVPacket::ParseResult TLVPacket::ParsePacket(
	IPHeaderContextTable *pContextTable, MMTPSequenceCounter *pSequenceCounter)
{
	m_Header = HeaderInfo();

	if ((m_DataSize < TLV_HEADER_SIZE) || (m_pData[0] != TLV_SYNC_BYTE))
		return ParseResult::FormatError;

	m_Header.PacketType = m_pData[1];
	m_Header.DataLength = Load16(&m_pData[2]);

	if (m_DataSize != TLV_HEADER_SIZE + m_Header.DataLength)
		return ParseResult::FormatError; // 長さ不正

	bool OK;

	switch (m_Header.PacketType) {
	case TLV_PACKET_TYPE_IPV4:
		OK = ParseIPv4(TLV_HEADER_SIZE);
		break;

	case TLV_PACKET_TYPE_IPV6:
		OK = ParseIPv6(TLV_HEADER_SIZE);
		break;

	case TLV_PACKET_TYPE_COMPRESSED_IP:
		OK = ParseCompressedIP(TLV_HEADER_SIZE, pContextTable);
		break;

	case TLV_PACKET_TYPE_SIGNALING:
		m_Header.Kind = PayloadKind::Signaling;
		m_Header.PayloadOffset = static_cast<uint32_t>(TLV_HEADER_SIZE);
		m_Header.PayloadSize = m_Header.DataLength;
		OK = true;
		break;

	case TLV_PACKET_TYPE_NULL:
		m_Header.Kind = PayloadKind::Null;
		m_Header.PayloadOffset = static_cast<uint32_t>(TLV_HEADER_SIZE);
		m_Header.PayloadSize = m_Header.DataLength;
		OK = true;
		break;

	default:
		OK = false;
		break;
	}

	if (!OK)
		return ParseResult::FormatError;

	if ((pSequenceCounter != nullptr) && (m_Header.Kind == PayloadKind::MMTP)) {
		if (!pSequenceCounter->Check(m_Header.MMTP.PacketID, m_Header.MMTP.PacketSequenceNumber))
			return ParseResult::ContinuityError;
	}

	return ParseResult::OK;
}


void TLVPacket::ReparsePacket()
{
	if (m_Header.Kind == PayloadKind::MMTP) {
		if (!m_Header.MMTP.Parse(&m_pData[m_Header.PayloadOffset], m_Header.PayloadSize)) {
			m_Header.MMTP = MMTPHeader();
			m_Header.Kind = PayloadKind::Other;
		}
	}
}


bool TLVPacket::IsIPPacket() const noexcept
{
	return (m_Header.PacketType == TLV_PACKET_TYPE_IPV4)
		|| (m_Header.PacketType == TLV_PACKET_TYPE_IPV6)
		|| (m_Header.PacketType == TLV_PACKET_TYPE_COMPRESSED_IP);
}


uint8_t * TLVPacket::GetPayloadData() noexcept
{
	if (m_Header.PayloadOffset == 0)
		return nullptr;
	return &m_pData[m_Header.PayloadOffset];
}


const uint8_t * TLVPacket::GetPayloadData() const noexcept
{
	if (m_Header.PayloadOffset == 0)
		return nullptr;
	return &m_pData[m_Header.PayloadOffset];
}


uint8_t * TLVPacket::GetMMTPPayloadData() noexcept
{
	if (m_Header.Kind != PayloadKind::MMTP)
		return nullptr;
	return &m_pData[m_Header.PayloadOffset + m_Header.MMTP.HeaderSize];
}


const uint8_t * TLVPacket::GetMMTPPayloadData() const noexcept
{
	if (m_Header.Kind != PayloadKind::MMTP)
		return nullptr;
	return &m_pData[m_Header.PayloadOffset + m_Header.MMTP.HeaderSize];
}


size_t TLVPacket::GetMMTPPayloadSize() const noexcept
{
	if (m_Header.Kind != PayloadKind::MMTP)
		return 0;
	return m_Header.PayloadSize - m_Header.MMTP.HeaderSize;
}


bool TLVPacket::IsValidPacketType(uint8_t PacketType) noexcept
{
	return (PacketType == TLV_PACKET_TYPE_IPV4)
		|| (PacketType == TLV_PACKET_TYPE_IPV6)
		|| (PacketType == TLV_PACKET_TYPE_COMPRESSED_IP)
		|| (PacketType == TLV_PACKET_TYPE_SIGNALING)
		|| (PacketType == TLV_PACKET_TYPE_NULL);
}


bool TLVPacket::ParseIPv4(size_t Pos)
{
	// IETF RFC 791
	const size_t Size = m_DataSize - Pos;
	if (Size < IPV4_HEADER_SIZE_MIN)
		return false;

	const uint8_t *pHeader = &m_pData[Pos];
	if ((pHeader[0] >> 4) != 4)
		return false;

	const size_t HeaderLength = (pHeader[0] & 0x0F) * 4;
	const size_t TotalLength = Load16(&pHeader[2]);
	if ((HeaderLength < IPV4_HEADER_SIZE_MIN) || (TotalLength != Size) || (HeaderLength > TotalLength))
		return false;

	m_Header.IPVersion = 4;

	const bool Fragmented = (Load16(&pHeader[6]) & 0x3FFF) != 0;
	if ((pHeader[9] != IP_PROTOCOL_UDP) || Fragmented) {
		m_Header.Kind = PayloadKind::Other;
		m_Header.PayloadOffset = static_cast<uint32_t>(Pos + HeaderLength);
		m_Header.PayloadSize = static_cast<uint32_t>(TotalLength - HeaderLength);
		return true;
	}

	return ParseUDP(Pos + HeaderLength, TotalLength - HeaderLength);
}


bool TLVPacket::ParseIPv6(size_t Pos)
{
	// IETF RFC 8200 / ARIB TR-B39 第1部 第7編 5.5.3.1
	const size_t Size = m_DataSize - Pos;
	if (Size < IPV6_HEADER_SIZE)
		return false;

	const uint8_t *pHeader = &m_pData[Pos];
	if ((pHeader[0] >> 4) != 6)
		return false;

	const size_t PayloadLength = Load16(&pHeader[4]);
	if (PayloadLength != Size - IPV6_HEADER_SIZE)
		return false;

	m_Header.IPVersion = 6;

	if (pHeader[6] != IP_PROTOCOL_UDP) {
		m_Header.Kind = PayloadKind::Other;
		m_Header.PayloadOffset = static_cast<uint32_t>(Pos + IPV6_HEADER_SIZE);
		m_Header.PayloadSize = static_cast<uint32_t>(PayloadLength);
		return true;
	}

	return ParseUDP(Pos + IPV6_HEADER_SIZE, PayloadLength);
}


bool TLVPacket::ParseCompressedIP(size_t Pos, IPHeaderContextTable *pContextTable)
{
	// ARIB STD-B32 第3部 3.7 (1) / ARIB TR-B39 第1部 第7編 5.5.3.3
	const size_t Size = m_DataSize - Pos;
	if (Size < COMPRESSED_IP_HEADER_SIZE)
		return false;

	const uint8_t *pHeader = &m_pData[Pos];

	m_Header.HeaderCompressed = true;
	m_Header.ContextID = Load16(&pHeader[0]) >> 4;
	m_Header.ContextSequenceNumber = pHeader[1] & 0x0F;
	m_Header.ContextHeaderType = pHeader[2];

	IPHeaderContext *pContext = (pContextTable != nullptr) ? &(*pContextTable)[m_Header.ContextID] : nullptr;
	size_t HeaderSize;

	switch (m_Header.ContextHeaderType) {
	case CID_HEADER_TYPE_IPV6_PARTIAL:
		{
			// IPv6_header_wo_length() + UDP_header_wo_length()
			HeaderSize = COMPRESSED_IP_HEADER_SIZE + IPV6_PARTIAL_HEADER_SIZE + UDP_PARTIAL_HEADER_SIZE;
			if (Size < HeaderSize)
				return false;

			const uint8_t *pIPHeader = &pHeader[COMPRESSED_IP_HEADER_SIZE];
			if (((pIPHeader[0] >> 4) != 6) || (pIPHeader[4] != IP_PROTOCOL_UDP))
				return false;

			const uint8_t *pUDPHeader = &pIPHeader[IPV6_PARTIAL_HEADER_SIZE];
			m_Header.IPVersion = 6;
			m_Header.HasUDPPorts = true;
			m_Header.SourcePort = Load16(&pUDPHeader[0]);
			m_Header.DestinationPort = Load16(&pUDPHeader[2]);
		}
		break;

	case CID_HEADER_TYPE_IPV4_PARTIAL:
		{
			// 部分 IPv4 ヘッダ + 部分 UDP ヘッダ
			HeaderSize = COMPRESSED_IP_HEADER_SIZE + IPV4_PARTIAL_HEADER_SIZE + UDP_PARTIAL_HEADER_SIZE;
			if (Size < HeaderSize)
				return false;

			const uint8_t *pIPHeader = &pHeader[COMPRESSED_IP_HEADER_SIZE];
			if (((pIPHeader[0] >> 4) != 4) || (pIPHeader[7] != IP_PROTOCOL_UDP))
				return false;

			const uint8_t *pUDPHeader = &pIPHeader[IPV4_PARTIAL_HEADER_SIZE];
			m_Header.IPVersion = 4;
			m_Header.HasUDPPorts = true;
			m_Header.SourcePort = Load16(&pUDPHeader[0]);
			m_Header.DestinationPort = Load16(&pUDPHeader[2]);
		}
		break;

	case CID_HEADER_TYPE_IPV4_ID:
		// IPv4 ヘッダ識別子 (16 ビット)
		HeaderSize = COMPRESSED_IP_HEADER_SIZE + 2;
		if (Size < HeaderSize)
			return false;
		m_Header.IPVersion = 4;
		break;

	case CID_HEADER_TYPE_NO_HEADER:
		HeaderSize = COMPRESSED_IP_HEADER_SIZE;
		break;

	default:
		return false;
	}

	if (pContext != nullptr) {
		if (m_Header.HasUDPPorts) {
			// フルヘッダでコンテキストを更新
			pContext->Valid = true;
			pContext->IPVersion = m_Header.IPVersion;
			pContext->SourcePort = m_Header.SourcePort;
			pContext->DestinationPort = m_Header.DestinationPort;
		} else if (pContext->Valid) {
			m_Header.IPVersion = pContext->IPVersion;
			m_Header.HasUDPPorts = true;
			m_Header.SourcePort = pContext->SourcePort;
			m_Header.DestinationPort = pContext->DestinationPort;
		}
	}

	return ParseUDPPayload(Pos + HeaderSize, Size - HeaderSize);
}


bool TLVPacket::ParseUDP(size_t Pos, size_t Size)
{
	// IETF RFC 768
	if (Size < UDP_HEADER_SIZE)
		return false;

	const uint8_t *pHeader = &m_pData[Pos];
	const size_t Length = Load16(&pHeader[4]);
	if (Length != Size)
		return false;

	m_Header.HasUDPPorts = true;
	m_Header.SourcePort = Load16(&pHeader[0]);
	m_Header.DestinationPort = Load16(&pHeader[2]);

	return ParseUDPPayload(Pos + UDP_HEADER_SIZE, Size - UDP_HEADER_SIZE);
}


bool TLVPacket::ParseUDPPayload(size_t Pos, size_t Size)
{
	m_Header.PayloadOffset = static_cast<uint32_t>(Pos);
	m_Header.PayloadSize = static_cast<uint32_t>(Size);

	if (m_Header.HasUDPPorts && (m_Header.DestinationPort == NTP_PORT)) {
		// NTP (IETF RFC 5905)
		if (Size < NTP_PACKET_SIZE)
			return false;
		const uint8_t *pTimestamp = &m_pData[Pos + NTP_TRANSMIT_TIMESTAMP_OFFSET];
		m_Header.NTPTimestamp = (static_cast<uint64_t>(Load32(&pTimestamp[0])) << 32) | Load32(&pTimestamp[4]);
		m_Header.NTPLeapIndicator = m_pData[Pos] >> 6;
		m_Header.Kind = PayloadKind::NTP;
		return true;
	}

	// 放送の UDP ペイロードは NTP 以外 MMTP (ARIB TR-B39 第1部 第7編 5.5.3.2)
	if (m_Header.MMTP.Parse(&m_pData[Pos], Size)) {
		m_Header.Kind = PayloadKind::MMTP;
	} else {
		m_Header.MMTP = MMTPHeader();
		m_Header.Kind = PayloadKind::Other;
	}

	return true;
}


} // namespace LibISDB
