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
 @file   MMTAnalyzerFilter.hpp
 @brief  MMT 解析フィルタ
 @author cngu-dtv

 TLVPacket の TLV-SI / MMT-SI を解析する。TSPacket は解析せずにそのまま出力する。
*/


#ifndef LIBISDB_MMT_ANALYZER_FILTER_H
#define LIBISDB_MMT_ANALYZER_FILTER_H


#include "FilterBase.hpp"
#include "../MMT/MMTPacketIDMap.hpp"
#include "../MMT/MMTTables.hpp"
#include "../TLV/TLVSITables.hpp"
#include "../EPG/EventInfo.hpp"
#include "../Base/EventListener.hpp"
#include <vector>
#include <map>
#include <memory>


namespace LibISDB
{

	/** MMT 解析フィルタクラス */
	class MMTAnalyzerFilter
		: public SingleIOFilter
	{
	public:
		static constexpr uint16_t LOGO_ID_INVALID = MHLogoTransmissionDescriptor::LOGO_ID_INVALID;

		class EventListener
			: public LibISDB::EventListener
		{
		public:
			virtual void OnPLTUpdated(MMTAnalyzerFilter *pAnalyzer) {}
			virtual void OnMPTUpdated(MMTAnalyzerFilter *pAnalyzer, uint16_t ServiceID) {}
			virtual void OnMHSDTUpdated(MMTAnalyzerFilter *pAnalyzer) {}
			virtual void OnTLVNITUpdated(MMTAnalyzerFilter *pAnalyzer) {}
			virtual void OnMHBITUpdated(MMTAnalyzerFilter *pAnalyzer) {}
			virtual void OnMHEITUpdated(MMTAnalyzerFilter *pAnalyzer) {}
			virtual void OnMHCATUpdated(MMTAnalyzerFilter *pAnalyzer) {}
			virtual void OnMHTOTUpdated(MMTAnalyzerFilter *pAnalyzer) {}
		};

		struct AssetInfo {
			uint16_t PacketID = MMT_PACKET_ID_INVALID;
			uint32_t AssetType = 0;
			uint16_t ComponentTag = MMT_COMPONENT_TAG_INVALID;
			uint8_t StreamType = STREAM_TYPE_INVALID; /**< MH-音声コンポーネント記述子の stream_type (音声のみ) */
			bool HasAudioInfo = false;                /**< MH-音声コンポーネント記述子があるか (音声のみ) */
			EventInfo::AudioInfo AudioInfo{};         /**< MH-音声コンポーネント記述子の内容 */
		};

		typedef std::vector<AssetInfo> AssetInfoList;

		struct ECMInfo {
			uint16_t CASystemID;
			uint16_t PacketID;
		};

		struct EMTInfo {
			uint8_t EMTTag;
			uint16_t PacketID;
		};

		/** アプリケーションサービス記述子の内容 (packet_id 以外の位置は MMT_PACKET_ID_INVALID) */
		struct ApplicationServiceInfo {
			uint8_t ApplicationFormat;
			uint8_t DocumentResolution;
			bool DefaultAITFlag;
			uint16_t AITPacketID;
			uint16_t DTMessagePacketID; /**< データ伝送メッセージが無い場合も MMT_PACKET_ID_INVALID */
			std::vector<EMTInfo> EMTList;
		};

		struct ServiceInfo {
			bool IsMPTAcquired;
			uint16_t ServiceID;
			uint16_t MPTPacketID;
			uint8_t MPTVersion;
			AssetInfoList AssetList;
			AssetInfoList VideoAssetList;
			AssetInfoList AudioAssetList;
			AssetInfoList CaptionAssetList;
			AssetInfoList ApplicationAssetList;
			AssetInfoList OtherAssetList;
			std::vector<ECMInfo> ECMList;
			std::vector<ApplicationServiceInfo> ApplicationServiceList;
			uint8_t RunningStatus;
			bool FreeCAMode;
			String ProviderName;
			String ServiceName;
			uint8_t ServiceType;
			uint16_t LogoID;
		};

		typedef std::vector<ServiceInfo> ServiceList;

		struct SDTServiceInfo {
			uint16_t ServiceID;
			uint8_t RunningStatus;
			bool FreeCAMode;
			String ProviderName;
			String ServiceName;
			uint8_t ServiceType;
			uint16_t LogoID;
		};

		typedef std::vector<SDTServiceInfo> SDTServiceList;

		struct SDTStreamInfo {
			uint16_t TLVStreamID;
			uint16_t OriginalNetworkID;
			SDTServiceList ServiceList;
		};

		typedef std::vector<SDTStreamInfo> SDTStreamList;

		typedef ServiceListDescriptor::ServiceInfo NetworkServiceInfo;

		struct NetworkStreamInfo {
			uint16_t TLVStreamID;
			uint16_t OriginalNetworkID;
			std::vector<NetworkServiceInfo> ServiceList;
		};

		typedef std::vector<NetworkStreamInfo> NetworkStreamList;

		struct SatelliteDeliverySystemInfo {
			uint16_t TLVStreamID;
			uint32_t Frequency;
			uint16_t OrbitalPosition;
			bool WestEastFlag;
			uint8_t Polarization;
			uint8_t Modulation;
			uint32_t SymbolRate;
			uint8_t FECInner;
		};

		typedef std::vector<SatelliteDeliverySystemInfo> SatelliteDeliverySystemList;

		struct BITBroadcasterInfo {
			uint8_t BroadcasterID;
			String BroadcasterName;
			std::vector<ServiceListDescriptor::ServiceInfo> ServiceList;
		};

		struct BITNetworkInfo {
			uint16_t OriginalNetworkID;
			std::vector<BITBroadcasterInfo> BroadcasterList;
		};

		typedef std::vector<BITNetworkInfo> BITNetworkList;

		typedef std::vector<uint16_t> EMMPacketIDList;

		MMTAnalyzerFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("MMTAnalyzerFilter"); }

	// FilterBase
		void Reset() override;

	// FilterSink
		bool ReceiveData(DataStream *pData) override;

	// MMTAnalyzerFilter
		int GetServiceCount() const;
		uint16_t GetServiceID(int Index) const;
		int GetServiceIndexByID(uint16_t ServiceID) const;
		bool GetServiceInfo(int Index, ReturnArg<ServiceInfo> Info) const;
		bool GetServiceInfoByID(uint16_t ServiceID, ReturnArg<ServiceInfo> Info) const;
		bool GetServiceList(ReturnArg<ServiceList> List) const;
		bool IsServiceMPTAcquired(int Index) const;
		uint16_t GetMPTPacketID(int Index) const;
		bool GetServiceName(int Index, ReturnArg<String> Name) const;
		uint8_t GetServiceType(int Index) const;
		uint16_t GetLogoID(int Index) const;
		int GetVideoAssetCount(int ServiceIndex) const;
		bool GetVideoAssetInfo(int ServiceIndex, int AssetIndex, ReturnArg<AssetInfo> Info) const;
		int GetVideoAssetIndexByComponentTag(int ServiceIndex, uint16_t ComponentTag) const;
		int GetAudioAssetCount(int ServiceIndex) const;
		bool GetAudioAssetInfo(int ServiceIndex, int AssetIndex, ReturnArg<AssetInfo> Info) const;
		int GetAudioAssetIndexByComponentTag(int ServiceIndex, uint16_t ComponentTag) const;

		uint16_t GetTLVStreamID() const;
		uint16_t GetNetworkID() const;
		bool GetNetworkName(ReturnArg<String> Name) const;
		uint8_t GetBroadcastingID() const;
		// TLV-NIT のリモコンキー記述子で割り当てられていない場合は 0
		uint8_t GetRemoteControlKeyID(uint16_t ServiceID) const;
		bool GetSDTServiceList(ReturnArg<SDTServiceList> List) const;
		bool GetSDTStreamList(ReturnArg<SDTStreamList> List) const;
		bool GetNetworkStreamList(ReturnArg<NetworkStreamList> List) const;
		bool GetSatelliteDeliverySystemList(ReturnArg<SatelliteDeliverySystemList> List) const;
		bool GetBITNetworkList(ReturnArg<BITNetworkList> List) const;
		bool GetEMMPacketIDList(ReturnArg<EMMPacketIDList> List) const;

		bool IsPLTUpdated() const;
		bool IsMHSDTUpdated() const;
		bool IsTLVNITUpdated() const;
		bool IsMHEITUpdated() const;

		uint16_t GetEventID(int ServiceIndex, bool Next = false) const;
		bool GetEventInfo(int ServiceIndex, ReturnArg<EventInfo> Info, bool Next = false) const;
		bool GetEventSeriesInfo(int ServiceIndex, ReturnArg<EventInfo::SeriesInfo> Info, bool Next = false) const;

		bool GetTOTTime(ReturnArg<DateTime> Time) const;
		// NTP の時刻で補間する
		bool GetInterpolatedTOTTime(ReturnArg<DateTime> Time, OptionalReturnArg<bool> Interpolated = std::nullopt) const;

		bool AddEventListener(EventListener *pEventListener);
		bool RemoveEventListener(EventListener *pEventListener);

	protected:
		const MHEITTable * GetEITPfTableByServiceID(uint16_t ServiceID, bool Next) const;
		template<typename T> const T * GetSectionTable(uint16_t PacketID) const;
		void GetSDTServiceInfo(const MHSDTTable *pSDTTable, int SDTIndex, ReturnArg<SDTServiceInfo> Info) const;
		void ApplySDTServiceInfo(ServiceInfo *pServiceInfo, const MHSDTTable *pSDTTable, int SDTIndex) const;
		void UpdateSDTStreamMap(const MHSDTTable *pSDTTable);
		static int GetAssetIndexByComponentTag(const AssetInfoList &List, uint16_t ComponentTag);

		struct NITInfo {
			uint8_t BroadcastingFlag = 0;
			uint8_t BroadcastingID = 0;
			String NetworkName;
			std::map<uint16_t, uint8_t> RemoteControlKeyMap;

			void Reset() { *this = NITInfo(); }
		};

		MMTPacketIDMapManager m_PacketIDMapManager;
		std::unique_ptr<TLVSITableSet> m_TLVSITableSet;
		TLVSIParser m_TLVSIParser;

		uint16_t m_TLVStreamID;
		uint16_t m_NetworkID;

		bool m_PLTUpdated;
		bool m_SDTUpdated;
		bool m_NITUpdated;
		bool m_EITUpdated;
		bool m_SendEITUpdatedEvent;

		ServiceList m_ServiceList;
		SDTServiceList m_SDTServiceList;
		std::map<uint32_t, SDTStreamInfo> m_SDTStreamMap;
		NetworkStreamList m_NetworkStreamList;
		NITInfo m_NITInfo;
		EMMPacketIDList m_EMMPacketIDList;

		uint64_t m_LastNTPTimestamp;
		uint64_t m_TOTNTPTimestamp;

		EventListenerList<EventListener> m_EventListenerList;

	private:
		void OnPLTTable(const MMTSingleTable *pTable);
		void OnMPTTable(const MMTSingleTable *pTable);
		void OnMHCATTable(const MMTSingleTable *pTable);
		void OnMHSDTSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnMHEITSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnMHBITSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnMHTOTSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnTLVSISection(const PSITableBase *pTable, const PSISection *pSection);
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_MMT_ANALYZER_FILTER_H
