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
 @file   StreamFormatDetector.hpp
 @brief  ストリーム形式の判別
 @author cngu-dtv
*/


#ifndef LIBISDB_STREAM_FORMAT_DETECTOR_H
#define LIBISDB_STREAM_FORMAT_DETECTOR_H


#include "../Base/DataBuffer.hpp"
#include "TLVPacketParser.hpp"


namespace LibISDB
{

	/** ストリーム形式 */
	enum class StreamFormat {
		Unknown, /**< 不明 */
		TS,      /**< MPEG-2 TS */
		TLV,     /**< TLV (ARIB STD-B32 第3部) */
	};

	/** ストリーム形式判別クラス */
	class StreamFormatDetector
	{
	public:
		typedef TLVPacketParser::SyncResult SyncResult;

		static constexpr size_t DEFAULT_MAX_BUFFER_SIZE = 256 * 1024;
		static constexpr size_t MIN_BUFFER_SIZE = 64 * 1024;
		static constexpr int TS_SYNC_COUNT = 5;

		StreamFormatDetector();

		void Reset();
		StreamFormat InputData(const uint8_t *pData, size_t Size);
		StreamFormat GetStreamFormat() const noexcept { return m_Format; }
		DataBuffer & GetBuffer() noexcept { return m_Buffer; }
		const DataBuffer & GetBuffer() const noexcept { return m_Buffer; }
		size_t GetSyncPosition() const noexcept { return m_SyncPosition; }
		bool SetMaxBufferSize(size_t Size);
		size_t GetMaxBufferSize() const noexcept { return m_MaxBufferSize; }

		static SyncResult CheckTSSync(const uint8_t *pData, size_t Size);
		static SyncResult FindTSSync(const uint8_t *pData, size_t Size);

	private:
		DataBuffer m_Buffer;
		size_t m_MaxBufferSize;
		size_t m_ScanPosition;
		size_t m_SyncPosition;
		StreamFormat m_Format;
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_STREAM_FORMAT_DETECTOR_H
