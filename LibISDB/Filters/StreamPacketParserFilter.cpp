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
 @file   StreamPacketParserFilter.cpp
 @brief  ストリームパケット解析フィルタ
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "StreamPacketParserFilter.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


namespace
{

const CharType * GetStreamFormatText(StreamFormat Format)
{
	switch (Format) {
	case StreamFormat::TS:  return LIBISDB_STR("TS");
	case StreamFormat::TLV: return LIBISDB_STR("TLV");
	default:                return LIBISDB_STR("Unknown");
	}
}


bool IsSameTLVStream(const TLVPacket &Packet1, const TLVPacket &Packet2)
{
	if (Packet1.GetPayloadKind() != Packet2.GetPayloadKind())
		return false;
	if (Packet1.HasMMTPHeader() && (Packet1.GetPacketID() != Packet2.GetPacketID()))
		return false;
	return true;
}

}




StreamPacketParserFilter::StreamPacketParserFilter()
	: m_TLVParser(this)
	, m_MaxTLVSequencePacketCount(DEFAULT_MAX_TLV_SEQUENCE_PACKET_COUNT)
	, m_StreamFormat(StreamFormat::Unknown)
	, m_FixedStreamFormat(StreamFormat::Unknown)
	, m_NotifiedStreamFormat(StreamFormat::Unknown)
	, m_SyncLossCount(0)
{
}


void StreamPacketParserFilter::Reset()
{
	BlockLock Lock(m_FilterLock);

	TSPacketParserFilter::Reset();

	m_TotalTLVPacketCount += m_TLVParser.GetPacketCount();
	m_TLVParser.Reset();
	m_TLVPacketSequence.SetDataCount(0);
	m_MMTPacketCount.clear();
	m_SyncLossCount = 0;

	ResetFormat();
}


bool StreamPacketParserFilter::StartStreaming()
{
	if (!TSPacketParserFilter::StartStreaming())
		return false;

	BlockLock Lock(m_FilterLock);

	if (m_OutputSequence)
		m_TLVPacketSequence.Allocate(m_MaxTLVSequencePacketCount);

	return true;
}


bool StreamPacketParserFilter::ReceiveData(DataStream *pData)
{
	BlockLock Lock(m_FilterLock);

	do {
		InputBuffer(pData->GetData());
	} while (pData->Next());

	FlushTLVSequence();

	return true;
}


void StreamPacketParserFilter::SetFixedStreamFormat(StreamFormat Format)
{
	BlockLock Lock(m_FilterLock);

	if (m_FixedStreamFormat != Format) {
		m_FixedStreamFormat = Format;
		FlushTLVSequence();
		ResetFormat();
	}
}


bool StreamPacketParserFilter::SetMaxTLVSequencePacketCount(size_t Count)
{
	if (LIBISDB_TRACE_ERROR_IF(Count < 1))
		return false;

	BlockLock Lock(m_FilterLock);

	m_MaxTLVSequencePacketCount = Count;

	return true;
}


StreamPacketParserFilter::TLVPacketCountInfo StreamPacketParserFilter::GetTLVPacketCount() const
{
	BlockLock Lock(m_FilterLock);

	return m_TLVParser.GetPacketCount();
}


StreamPacketParserFilter::TLVPacketCountInfo StreamPacketParserFilter::GetTotalTLVPacketCount() const
{
	BlockLock Lock(m_FilterLock);

	return m_TotalTLVPacketCount + m_TLVParser.GetPacketCount();
}


bool StreamPacketParserFilter::GetMMTPacketCountList(ReturnArg<MMTPacketCountList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	List->clear();
	List->reserve(m_MMTPacketCount.size());
	for (const auto &e : m_MMTPacketCount)
		List->push_back({e.first, e.second});

	return true;
}


bool StreamPacketParserFilter::AddEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.AddEventListener(pEventListener);
}


bool StreamPacketParserFilter::RemoveEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.RemoveEventListener(pEventListener);
}


void StreamPacketParserFilter::OnTLVPacket(TLVPacket *pPacket, TLVPacket::ParseResult Result)
{
	++m_PacketCount.Input;

	bool Output = false;

	switch (Result) {
	case TLVPacket::ParseResult::ContinuityError:
		++m_PacketCount.ContinuityError;
		[[fallthrough]];
	case TLVPacket::ParseResult::OK:
		if (pPacket->HasMMTPHeader()) {
			PacketCountInfo &Count = m_MMTPacketCount[pPacket->GetPacketID()];
			++Count.Input;
			if (Result == TLVPacket::ParseResult::ContinuityError)
				++Count.ContinuityError;
			if (pPacket->IsScrambled())
				++Count.Scrambled;
		}
		if (pPacket->IsScrambled())
			++m_PacketCount.Scrambled;
		if (m_OutputNullPacket || (pPacket->GetPacketType() != TLV_PACKET_TYPE_NULL))
			Output = true;
		break;

	case TLVPacket::ParseResult::FormatError:
		++m_PacketCount.FormatError;
		if (m_OutputErrorPacket)
			Output = true;
		break;
	}

	if (Output)
		OutputTLVPacket(*pPacket);
}


void StreamPacketParserFilter::InputBuffer(DataBuffer *pBuffer)
{
	if (m_StreamFormat == StreamFormat::Unknown) {
		if (m_FixedStreamFormat == StreamFormat::Unknown) {
			DetectFormat(pBuffer->GetData(), pBuffer->GetSize());
			return;
		}
		if (!SetStreamFormat(m_FixedStreamFormat))
			return;
	}

	switch (m_StreamFormat) {
	case StreamFormat::TS:
		if (IsTSLost(pBuffer)) {
			LIBISDB_TRACE(LIBISDB_STR("StreamPacketParserFilter : TS sync lost\n"));
			ResetFormat();
			DetectFormat(pBuffer->GetData(), pBuffer->GetSize());
		} else {
			InputTS(pBuffer);
		}
		break;

	case StreamFormat::TLV:
		if (IsTLVLost(pBuffer)) {
			LIBISDB_TRACE(LIBISDB_STR("StreamPacketParserFilter : TLV sync lost\n"));
			FlushTLVSequence();
			ResetFormat();
			DetectFormat(pBuffer->GetData(), pBuffer->GetSize());
		} else {
			InputTLV(pBuffer->GetData(), pBuffer->GetSize());
		}
		break;

	default:
		break;
	}
}


void StreamPacketParserFilter::InputTS(DataBuffer *pBuffer)
{
	const uint8_t *pData = pBuffer->GetData();
	const size_t Size = pBuffer->GetSize();
	const size_t TLVPos = FindTLVStart(pData, Size);

	if (TLVPos < Size) {
		LIBISDB_TRACE(LIBISDB_STR("StreamPacketParserFilter : TLV found at {}\n"), TLVPos);
		if (TLVPos > 0) {
			DataBuffer Head(pData, TLVPos);
			InputTSBuffer(&Head);
		}
		ResetFormat();
		DetectFormat(&pData[TLVPos], Size - TLVPos);
		return;
	}

	InputTSBuffer(pBuffer);
}


void StreamPacketParserFilter::InputTSBuffer(DataBuffer *pBuffer)
{
	SingleDataStream<DataBuffer> Stream(pBuffer);

	TSPacketParserFilter::ReceiveData(&Stream);
}


void StreamPacketParserFilter::InputTLV(const uint8_t *pData, size_t Size)
{
	// 自動判別時は同期が外れた所で止め、続きが TS であれば判別し直す
	const bool AutoDetect = m_FixedStreamFormat == StreamFormat::Unknown;
	const size_t Used = m_TLVParser.InputData(pData, Size, AutoDetect);

	m_InputBytes += Used;
	CountSyncLoss();

	if (Used < Size) {
		const uint8_t *pRest = &pData[Used];
		const size_t RestSize = Size - Used;

		if (StreamFormatDetector::FindTSSync(pRest, RestSize) == StreamFormatDetector::SyncResult::Found) {
			LIBISDB_TRACE(LIBISDB_STR("StreamPacketParserFilter : TS found after TLV\n"));
			FlushTLVSequence();
			ResetFormat();
			DetectFormat(pRest, RestSize);
			return;
		}

		m_InputBytes += RestSize;
		m_TLVParser.InputData(pRest, RestSize);
		CountSyncLoss();
	}
}


void StreamPacketParserFilter::CountSyncLoss()
{
	// 同期外れはフォーマットエラーとして数える
	const unsigned long long SyncLoss = m_TLVParser.GetPacketCount().SyncLoss;
	if (SyncLoss != m_SyncLossCount) {
		m_PacketCount.FormatError += SyncLoss - m_SyncLossCount;
		m_SyncLossCount = SyncLoss;
	}
}


bool StreamPacketParserFilter::DetectFormat(const uint8_t *pData, size_t Size)
{
	const StreamFormat Format = m_FormatDetector.InputData(pData, Size);
	if (Format == StreamFormat::Unknown)
		return false;

	if (!SetStreamFormat(Format))
		return false;

	// 処理中に判別し直すことがあるため、判別器から取り出してから処理する
	DataBuffer Buffer(std::move(m_FormatDetector.GetBuffer()));
	m_FormatDetector.Reset();

	if (Format == StreamFormat::TS)
		InputTS(&Buffer);
	else
		InputTLV(Buffer.GetData(), Buffer.GetSize());

	return true;
}


bool StreamPacketParserFilter::SetStreamFormat(StreamFormat Format)
{
	m_StreamFormat = Format;
	m_TSCheckBuffer.clear();

	if (Format != m_NotifiedStreamFormat) {
		m_NotifiedStreamFormat = Format;

		Log(Logger::LogType::Information, LIBISDB_STR("ストリーム形式 : {}"), GetStreamFormatText(Format));

		// 新しい形式のデータを出力する前に通知する
		m_FilterLock.Unlock();
		m_EventListenerList.CallEventListener(&EventListener::OnStreamFormatChanged, this, Format);
		m_FilterLock.Lock();

		// 通知中にリセットされた
		if (m_StreamFormat != Format)
			return false;
	}

	return true;
}


void StreamPacketParserFilter::ResetFormat()
{
	m_FormatDetector.Reset();
	m_StreamFormat = StreamFormat::Unknown;
	m_TSCheckBuffer.clear();
	// TS の途中のパケットを次の TS と連結しない
	ClearPendingPacket();
}


bool StreamPacketParserFilter::IsTSLost(const DataBuffer *pBuffer)
{
	// TS の同期が無いバッファが来たら判別し直す
	if (m_FixedStreamFormat != StreamFormat::Unknown)
		return false;

	constexpr size_t CheckSize = TS_PACKET_SIZE_MAX * StreamFormatDetector::TS_SYNC_COUNT;
	const uint8_t *pData = pBuffer->GetData();
	const size_t Size = pBuffer->GetSize();
	StreamFormatDetector::SyncResult Result;

	if (Size < CheckSize) {
		// 小さいバッファはまとめて調べる
		m_TSCheckBuffer.insert(m_TSCheckBuffer.end(), pData, pData + Size);
		if (m_TSCheckBuffer.size() < CheckSize)
			return false;
		Result = StreamFormatDetector::FindTSSync(m_TSCheckBuffer.data(), m_TSCheckBuffer.size());
		m_TSCheckBuffer.clear();
	} else {
		m_TSCheckBuffer.clear();
		Result = StreamFormatDetector::FindTSSync(pData, Size);
	}

	return Result == StreamFormatDetector::SyncResult::Invalid;
}


size_t StreamPacketParserFilter::FindTLVStart(const uint8_t *pData, size_t Size) const
{
	// バッファの末尾が TS でなく途中に TLV の同期位置があればその位置、無ければ Size を返す
	constexpr size_t CheckSize = TS_PACKET_SIZE_MAX * StreamFormatDetector::TS_SYNC_COUNT;

	if ((m_FixedStreamFormat != StreamFormat::Unknown) || (Size < CheckSize * 2))
		return Size;
	if (StreamFormatDetector::FindTSSync(&pData[Size - CheckSize], CheckSize) != StreamFormatDetector::SyncResult::Invalid)
		return Size;

	size_t Pos;
	if (TLVPacketParser::FindSync(pData, Size, &Pos) != TLVPacketParser::SyncResult::Found)
		return Size;

	return Pos;
}


bool StreamPacketParserFilter::IsTLVLost(const DataBuffer *pBuffer) const
{
	if ((m_FixedStreamFormat != StreamFormat::Unknown) || m_TLVParser.IsSynced())
		return false;

	// TLV の同期が外れている間に TS の同期があれば判別し直す
	if (StreamFormatDetector::FindTSSync(pBuffer->GetData(), pBuffer->GetSize()) == StreamFormatDetector::SyncResult::Found)
		return true;

	return m_TLVParser.GetUnsyncedBytes() >= FORMAT_LOST_BYTES;
}


void StreamPacketParserFilter::OutputTLVPacket(TLVPacket &Packet)
{
	++m_PacketCount.Output;
	if (Packet.HasMMTPHeader())
		++m_MMTPacketCount[Packet.GetPacketID()].Output;

	if (m_OutputSequence) {
		// 1 回の出力には同じ種類 (MMTP なら同じ packet_id) のパケットだけを含める
		if ((m_TLVPacketSequence.GetDataCount() >= m_MaxTLVSequencePacketCount)
				|| ((m_TLVPacketSequence.GetDataCount() > 0)
					&& !IsSameTLVStream(m_TLVPacketSequence[0], Packet))) {
			FlushTLVSequence();
		}

		m_TLVPacketSequence.AddData(Packet);
	} else {
		OutputData(&Packet);
	}
}


void StreamPacketParserFilter::FlushTLVSequence()
{
	if (m_TLVPacketSequence.GetDataCount() > 0) {
		OutputData(m_TLVPacketSequence);
		m_TLVPacketSequence.SetDataCount(0);
	}
}


} // namespace LibISDB
