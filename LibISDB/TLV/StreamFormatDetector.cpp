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
 @file   StreamFormatDetector.cpp
 @brief  ストリーム形式の判別
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "StreamFormatDetector.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


namespace
{

constexpr uint8_t TS_SYNC_BYTE = 0x47_u8;

// TS パケットの間隔 (通常 / タイムスタンプ付き / FEC 付き)
constexpr size_t TS_PACKET_INTERVALS[] = {TS_PACKET_SIZE, 192, TS_PACKET_SIZE_MAX};

}




StreamFormatDetector::StreamFormatDetector()
	: m_MaxBufferSize(DEFAULT_MAX_BUFFER_SIZE)
	, m_ScanPosition(0)
	, m_SyncPosition(0)
	, m_Format(StreamFormat::Unknown)
{
}


void StreamFormatDetector::Reset()
{
	m_Buffer.ClearSize();
	m_ScanPosition = 0;
	m_SyncPosition = 0;
	m_Format = StreamFormat::Unknown;
}


StreamFormat StreamFormatDetector::InputData(const uint8_t *pData, size_t Size)
{
	if (m_Format != StreamFormat::Unknown)
		return m_Format;

	if ((pData != nullptr) && (Size > 0))
		m_Buffer.AddData(pData, Size);

	const uint8_t *pBuffer = m_Buffer.GetData();
	const size_t BufferSize = m_Buffer.GetSize();
	size_t Pos = m_ScanPosition;

	while (Pos < BufferSize) {
		SyncResult Result;
		StreamFormat Format;

		if (pBuffer[Pos] == TS_SYNC_BYTE) {
			Result = CheckTSSync(&pBuffer[Pos], BufferSize - Pos);
			Format = StreamFormat::TS;
		} else if (pBuffer[Pos] == TLV_SYNC_BYTE) {
			Result = TLVPacketParser::CheckSync(&pBuffer[Pos], BufferSize - Pos);
			Format = StreamFormat::TLV;
		} else {
			Pos++;
			continue;
		}

		if (Result == SyncResult::Found) {
			m_Format = Format;
			m_SyncPosition = Pos;
			return m_Format;
		}
		if (Result == SyncResult::NeedMoreData)
			break;

		Pos++;
	}

	m_ScanPosition = Pos;

	if (BufferSize >= m_MaxBufferSize) {
		// 判別できない場合は TS として扱う
		m_Format = StreamFormat::TS;
		m_SyncPosition = 0;
	}

	return m_Format;
}


bool StreamFormatDetector::SetMaxBufferSize(size_t Size)
{
	if (LIBISDB_TRACE_ERROR_IF(Size < MIN_BUFFER_SIZE))
		return false;

	m_MaxBufferSize = Size;

	return true;
}


StreamFormatDetector::SyncResult StreamFormatDetector::CheckTSSync(const uint8_t *pData, size_t Size)
{
	bool NeedMoreData = false;

	for (const size_t Interval : TS_PACKET_INTERVALS) {
		int i;

		for (i = 0; i < TS_SYNC_COUNT; i++) {
			const size_t Pos = Interval * i;
			if (Pos >= Size) {
				NeedMoreData = true;
				break;
			}
			if (pData[Pos] != TS_SYNC_BYTE)
				break;
		}

		if (i == TS_SYNC_COUNT)
			return SyncResult::Found;
	}

	return NeedMoreData ? SyncResult::NeedMoreData : SyncResult::Invalid;
}


StreamFormatDetector::SyncResult StreamFormatDetector::FindTSSync(const uint8_t *pData, size_t Size)
{
	// 同期位置の候補は先頭 TS_PACKET_SIZE_MAX バイトのみ調べる
	bool NeedMoreData = false;
	const size_t End = std::min(Size, TS_PACKET_SIZE_MAX);

	for (size_t Pos = 0; Pos < End; Pos++) {
		if (pData[Pos] == TS_SYNC_BYTE) {
			const SyncResult Result = CheckTSSync(&pData[Pos], Size - Pos);
			if (Result == SyncResult::Found)
				return SyncResult::Found;
			if (Result == SyncResult::NeedMoreData)
				NeedMoreData = true;
		}
	}

	if (Size < TS_PACKET_SIZE_MAX)
		NeedMoreData = true;

	return NeedMoreData ? SyncResult::NeedMoreData : SyncResult::Invalid;
}


} // namespace LibISDB
