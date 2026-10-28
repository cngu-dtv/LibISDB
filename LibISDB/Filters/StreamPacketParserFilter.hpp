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
 @file   StreamPacketParserFilter.hpp
 @brief  ストリームパケット解析フィルタ
 @author cngu-dtv

 入力されたストリームが TS か TLV かを判別し、TS であれば TSPacketParserFilter と同じ処理を行い、
 TLV であれば TLVPacket を出力する。
*/


#ifndef LIBISDB_STREAM_PACKET_PARSER_FILTER_H
#define LIBISDB_STREAM_PACKET_PARSER_FILTER_H


#include "TSPacketParserFilter.hpp"
#include "../TLV/TLVPacketParser.hpp"
#include "../TLV/StreamFormatDetector.hpp"
#include "../Base/EventListener.hpp"
#include <vector>
#include <atomic>


namespace LibISDB
{

	/** ストリームパケット解析フィルタクラス */
	class StreamPacketParserFilter
		: public TSPacketParserFilter
		, protected TLVPacketParser::PacketHandler
	{
	public:
		/** イベントリスナ */
		class EventListener
			: public LibISDB::EventListener
		{
		public:
			virtual void OnStreamFormatChanged(StreamPacketParserFilter *pFilter, StreamFormat Format) {}
		};

		typedef TLVPacketParser::PacketCountInfo TLVPacketCountInfo;

		static constexpr size_t DEFAULT_MAX_TLV_SEQUENCE_PACKET_COUNT = 32;
		static constexpr unsigned long long FORMAT_LOST_BYTES = 256 * 1024;

		StreamPacketParserFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("StreamPacketParserFilter"); }

	// FilterBase
		void Reset() override;
		bool StartStreaming() override;

	// FilterSink
		bool ReceiveData(DataStream *pData) override;

	// StreamPacketParserFilter
		StreamFormat GetStreamFormat() const noexcept { return m_StreamFormat; }
		void SetFixedStreamFormat(StreamFormat Format);
		StreamFormat GetFixedStreamFormat() const noexcept { return m_FixedStreamFormat; }
		bool SetMaxTLVSequencePacketCount(size_t Count);
		size_t GetMaxTLVSequencePacketCount() const noexcept { return m_MaxTLVSequencePacketCount; }
		TLVPacketCountInfo GetTLVPacketCount() const;
		TLVPacketCountInfo GetTotalTLVPacketCount() const;

		bool AddEventListener(EventListener *pEventListener);
		bool RemoveEventListener(EventListener *pEventListener);

	protected:
	// TLVPacketParser::PacketHandler
		void OnTLVPacket(TLVPacket *pPacket, TLVPacket::ParseResult Result) override;

	private:
		void InputBuffer(DataBuffer *pBuffer);
		void InputTS(DataBuffer *pBuffer);
		void InputTSBuffer(DataBuffer *pBuffer);
		void InputTLV(const uint8_t *pData, size_t Size);
		void CountSyncLoss();
		bool DetectFormat(const uint8_t *pData, size_t Size);
		bool SetStreamFormat(StreamFormat Format);
		void ResetFormat();
		bool IsTSLost(const DataBuffer *pBuffer);
		size_t FindTLVStart(const uint8_t *pData, size_t Size) const;
		bool IsTLVLost(const DataBuffer *pBuffer) const;
		void OutputTLVPacket(TLVPacket &Packet);
		void FlushTLVSequence();

		TLVPacketParser m_TLVParser;
		StreamFormatDetector m_FormatDetector;
		DataStreamSequence<TLVPacket> m_TLVPacketSequence;
		size_t m_MaxTLVSequencePacketCount;

		std::atomic<StreamFormat> m_StreamFormat;
		StreamFormat m_FixedStreamFormat;
		StreamFormat m_NotifiedStreamFormat;

		TLVPacketCountInfo m_TotalTLVPacketCount;
		unsigned long long m_SyncLossCount;
		std::vector<uint8_t> m_TSCheckBuffer;

		EventListenerList<EventListener> m_EventListenerList;
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_STREAM_PACKET_PARSER_FILTER_H
