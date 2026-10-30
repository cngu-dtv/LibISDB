/*
  LibISDB
  Copyright(c) 2017-2020 DBCTRADO

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
 @file   TSEngine.hpp
 @brief  TSエンジン
 @author DBCTRADO
*/


#ifndef LIBISDB_TS_ENGINE_H
#define LIBISDB_TS_ENGINE_H


#include "FilterGraph.hpp"
#include "../Filters/SourceFilter.hpp"
#include "../Filters/AnalyzerFilter.hpp"
#include "../Filters/StreamPacketParserFilter.hpp"
#include "../Filters/MMTAnalyzerFilter.hpp"
#include <initializer_list>
#include <memory>


namespace LibISDB
{

	/** TSエンジンクラス */
	class TSEngine
		: public ObjectBase
		, protected SourceFilter::EventListener
		, protected AnalyzerFilter::EventListener
		, protected StreamPacketParserFilter::EventListener
		, protected MMTAnalyzerFilter::EventListener
	{
	public:
		enum class OneSegSelectType {
			LowPriority,
			HighPriority,
			Refuse,
		};

		struct ServiceSelectInfo {
			uint16_t ServiceID = SERVICE_ID_INVALID;
			bool FollowViewableService = false;
			OneSegSelectType OneSegSelect = OneSegSelectType::LowPriority;
			int PreferredServiceIndex = -1;

			void Reset() noexcept { *this = ServiceSelectInfo(); }
		};

		TSEngine() noexcept;
		virtual ~TSEngine();

		bool BuildEngine(const FilterGraph::ConnectionInfo *pConnectionList, size_t ConnectionCount);
		bool BuildEngine(std::initializer_list<FilterBase *> FilterList);
		bool IsEngineBuilt() const noexcept { return m_IsBuilt; };
		bool CloseEngine();
		bool ResetEngine();
		FilterGraph::IDType RegisterFilter(FilterBase *pFilter);
		template<typename T> T * GetFilter() const { return m_FilterGraph.GetFilter<T>(); }
		template<typename T> T * GetFilterExplicit() const { return m_FilterGraph.GetFilterExplicit<T>(); }

		bool OpenSource(const String &Name);
		bool CloseSource();
		bool IsSourceOpen() const;

		bool SetServiceSelectInfo(const ServiceSelectInfo *pServiceSelInfo);
		bool SetService(const ServiceSelectInfo &ServiceSelInfo, bool Reserve = true);
		uint16_t GetServiceID() const;
		int GetServiceIndex() const;
		bool SetOneSegSelectType(OneSegSelectType Type);

		uint16_t GetTransportStreamID() const;
		uint16_t GetNetworkID() const;
		uint16_t GetEventID() const;

		StreamFormat GetStreamFormat() const;

		uint8_t GetVideoStreamType() const;
		int GetVideoStreamCount(int ServiceIndex = -1) const;
		bool SetVideoStream(int StreamIndex);
		int GetVideoStream() const;
		uint8_t GetVideoComponentTag() const;

		uint8_t GetAudioStreamType() const;
		int GetAudioStreamCount(int ServiceIndex = -1) const;
		bool SetAudioStream(int StreamIndex);
		int GetAudioStream() const;
		uint8_t GetAudioComponentTag() const;
		// 以下は現在のサービスの音声
		// MH-音声コンポーネント記述子の内容 (MMT の場合のみ)
		bool GetAudioComponentInfo(int StreamIndex, ReturnArg<EventInfo::AudioInfo> Info) const;
		bool IsAudioStreamPlayable(int StreamIndex) const;
		// 既定の音声 (MMT は ARIB TR-B39 第1部 第2編 4.7.2 の規定による)
		int GetDefaultAudioStream() const;

		void SetStartStreamingOnSourceOpen(bool Start);

		virtual bool IsSelectableService(int Index) const;
		virtual int GetSelectableServiceCount() const;
		virtual uint16_t GetSelectableServiceID(int Index) const;
		virtual uint16_t GetDefaultServiceID() const;
		virtual int GetSelectableServiceIndexByID(uint16_t ServiceID) const;
		// TS の場合のみ
		virtual bool GetSelectableServiceList(AnalyzerFilter::ServiceList *pList) const;
		// MMT の場合のみ
		virtual bool GetSelectableMMTServiceList(MMTAnalyzerFilter::ServiceList *pList) const;

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("TSEngine"); }
		void SetLogger(Logger *pLogger) override;

	protected:
	// SourceFilter::EventListener
		void OnGraphReset(SourceFilter *pSource) override;
		void OnSourceChanged(SourceFilter *pSource) override;

	// AnalyzerFilter::EventListener
		void OnPATUpdated(AnalyzerFilter *pAnalyzer) override;
		void OnPMTUpdated(AnalyzerFilter *pAnalyzer, uint16_t ServiceID) override;
		void OnEITUpdated(AnalyzerFilter *pAnalyzer) override;

	// StreamPacketParserFilter::EventListener
		void OnStreamFormatChanged(StreamPacketParserFilter *pParser, StreamFormat Format) override;

	// MMTAnalyzerFilter::EventListener
		void OnPLTUpdated(MMTAnalyzerFilter *pAnalyzer) override;
		void OnMPTUpdated(MMTAnalyzerFilter *pAnalyzer, uint16_t ServiceID) override;
		void OnMHSDTUpdated(MMTAnalyzerFilter *pAnalyzer) override;
		void OnMHEITUpdated(MMTAnalyzerFilter *pAnalyzer) override;

		/**
		 サービス情報の取得元

		 TS は AnalyzerFilter、MMT は MMTAnalyzerFilter から取得する。
		 ストリームの ID は TS では PID、MMT では packet_id。
		 MMT の component_tag は運用上 0x0000 - 0x007F (ARIB TR-B39 第1部 第4編 表 15.2-1) なので 8 ビットで扱う。
		*/
		class ServiceSource
		{
		public:
			virtual ~ServiceSource() = default;

			virtual bool IsMMT() const noexcept = 0;
			virtual uint16_t GetInvalidStreamID() const noexcept = 0;
			// transport_stream_id / tlv_stream_id
			virtual uint16_t GetStreamID() const = 0;
			virtual uint16_t GetNetworkID() const = 0;

			virtual int GetServiceCount() const = 0;
			// Index が負の場合は既定のサービス
			virtual uint16_t GetServiceID(int Index) const = 0;
			virtual int GetServiceIndexByID(uint16_t ServiceID) const = 0;
			// PMT / MPT を取得済みか
			virtual bool IsServiceInfoAcquired(int Index) const = 0;
			virtual bool Is1SegStream() const { return false; }
			virtual bool Is1SegService(int Index) const { return false; }
			virtual uint16_t Get1SegServiceIDByIndex(int Index) const { return SERVICE_ID_INVALID; }
			virtual uint16_t GetFirst1SegServiceID() const { return SERVICE_ID_INVALID; }

			virtual int GetVideoStreamCount(int ServiceIndex) const = 0;
			virtual uint16_t GetVideoStreamID(int ServiceIndex, int StreamIndex) const = 0;
			virtual uint8_t GetVideoStreamType(int ServiceIndex, int StreamIndex) const = 0;
			virtual uint8_t GetVideoComponentTag(int ServiceIndex, int StreamIndex) const = 0;
			virtual int GetVideoIndexByComponentTag(int ServiceIndex, uint8_t ComponentTag) const = 0;

			virtual int GetAudioStreamCount(int ServiceIndex) const = 0;
			virtual uint16_t GetAudioStreamID(int ServiceIndex, int StreamIndex) const = 0;
			virtual uint8_t GetAudioStreamType(int ServiceIndex, int StreamIndex) const = 0;
			virtual uint8_t GetAudioComponentTag(int ServiceIndex, int StreamIndex) const = 0;
			virtual int GetAudioIndexByComponentTag(int ServiceIndex, uint8_t ComponentTag) const = 0;
			virtual bool GetAudioComponentInfo(int ServiceIndex, int StreamIndex, ReturnArg<EventInfo::AudioInfo> Info) const { return false; }

			virtual uint16_t GetEventID(int ServiceIndex) const = 0;
		};

		class TSServiceSource;
		class MMTServiceSource;

	// TSEngine
		// 入力中のストリームの形式に対応する取得元 (解析フィルタが無い場合は nullptr)
		const ServiceSource * GetServiceSource() const;
		bool IsMMTStream() const;
		bool SelectService(uint16_t ServiceID, bool No1Seg = false);
		void ResetStatus();
		void SetVideoPID(uint16_t PID, bool ServiceChanged);
		void SetAudioPID(uint16_t PID, bool ServiceChanged);
		// ID は GetServiceSource() の形式のもの
		void SetVideoStreamID(uint16_t ID, bool ServiceChanged);
		void SetAudioStreamID(uint16_t ID, bool ServiceChanged);
		void ResetVideoAndAudioStreamID(bool ServiceChanged);
		void UpdateService(const ServiceSource *pSource);
		void UpdateEvent(const ServiceSource *pSource);
		bool IsAudioStreamPlayable(const ServiceSource *pSource, int ServiceIndex, int StreamIndex) const;
		int GetDefaultAudioStreamIndex(const ServiceSource *pSource, int ServiceIndex) const;
		void GetAudioComponentList(const ServiceSource *pSource, int ServiceIndex, EventInfo::AudioInfoList *pList) const;
		// 音声の component_tag、符号化方式、サイマル、言語の条件が変わったか
		static bool IsAudioSelectionConditionChanged(
			const EventInfo::AudioInfoList &List1, const EventInfo::AudioInfoList &List2);

		// 既定の音声の選択に使う (pInfo は MH-音声コンポーネント記述子の内容、無い場合は nullptr)
		virtual bool IsAudioPlayable(uint8_t StreamType, const EventInfo::AudioInfo *pInfo) const { return true; }
		// 2ch で再生する場合 (サイマル音声の AAC 2ch を優先する)
		virtual bool IsStereoAudioPreferred() const { return false; }

		virtual void OnEngineClose() {}
		virtual void OnEngineClosed() {}
		virtual void OnFilterRegistered(FilterBase *pFilter, FilterGraph::IDType ID);
		virtual void OnServiceChanged(uint16_t ServiceID) {}
		virtual void OnEventChanged(uint16_t EventID) {}
		virtual void OnVideoStreamTypeChanged(uint8_t StreamType) {}
		virtual void OnAudioStreamTypeChanged(uint8_t StreamType) {}

		bool m_IsBuilt;
		FilterGraph m_FilterGraph;
		mutable MutexLock m_EngineLock;

		SourceFilter *m_pSource;
		AnalyzerFilter *m_pAnalyzer;
		StreamPacketParserFilter *m_pStreamParser;
		MMTAnalyzerFilter *m_pMMTAnalyzer;
		std::unique_ptr<ServiceSource> m_TSServiceSource;
		std::unique_ptr<ServiceSource> m_MMTServiceSource;
		bool m_IsMMTStreamStarted;
		StreamFormat m_LastStreamFormat;

		uint16_t m_CurTransportStreamID;
		uint16_t m_CurServiceID;

		ServiceSelectInfo m_ServiceSel;
		ServiceSelectInfo m_SetChannelServiceSel;

		uint8_t m_VideoStreamType;
		uint8_t m_AudioStreamType;
		int m_CurVideoStream;
		uint8_t m_CurVideoComponentTag;
		int m_CurAudioStream;
		uint8_t m_CurAudioComponentTag;
		// 選択中のサービスの音声の構成 (MMT、利用者の選択を既定に戻す判定に使う)
		EventInfo::AudioInfoList m_AudioComponentList;
		uint16_t m_CurEventID;

		bool m_StartStreamingOnSourceOpen;
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_TS_ENGINE_H
