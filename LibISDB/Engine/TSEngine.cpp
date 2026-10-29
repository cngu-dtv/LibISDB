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
 @file   TSEngine.cpp
 @brief  TSエンジン
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "TSEngine.hpp"
#include "../MMT/MMTConsts.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


class TSEngine::TSServiceSource
	: public TSEngine::ServiceSource
{
public:
	TSServiceSource(const AnalyzerFilter *pAnalyzer) noexcept : m_pAnalyzer(pAnalyzer) {}

	bool IsMMT() const noexcept override { return false; }
	uint16_t GetInvalidStreamID() const noexcept override { return PID_INVALID; }
	uint16_t GetStreamID() const override { return m_pAnalyzer->GetTransportStreamID(); }
	uint16_t GetNetworkID() const override { return m_pAnalyzer->GetNetworkID(); }

	int GetServiceCount() const override { return m_pAnalyzer->GetServiceCount(); }
	uint16_t GetServiceID(int Index) const override { return m_pAnalyzer->GetServiceID(Index); }
	int GetServiceIndexByID(uint16_t ServiceID) const override { return m_pAnalyzer->GetServiceIndexByID(ServiceID); }
	bool IsServiceInfoAcquired(int Index) const override { return m_pAnalyzer->IsServicePMTAcquired(Index); }
	bool Is1SegStream() const override { return m_pAnalyzer->Is1SegStream(); }
	bool Is1SegService(int Index) const override { return m_pAnalyzer->Is1SegService(Index); }
	uint16_t Get1SegServiceIDByIndex(int Index) const override { return m_pAnalyzer->Get1SegServiceIDByIndex(Index); }
	uint16_t GetFirst1SegServiceID() const override { return m_pAnalyzer->GetFirst1SegServiceID(); }

	int GetVideoStreamCount(int ServiceIndex) const override { return m_pAnalyzer->GetVideoESCount(ServiceIndex); }
	uint16_t GetVideoStreamID(int ServiceIndex, int StreamIndex) const override { return m_pAnalyzer->GetVideoESPID(ServiceIndex, StreamIndex); }
	uint8_t GetVideoStreamType(int ServiceIndex, int StreamIndex) const override { return m_pAnalyzer->GetVideoStreamType(ServiceIndex, StreamIndex); }
	uint8_t GetVideoComponentTag(int ServiceIndex, int StreamIndex) const override { return m_pAnalyzer->GetVideoComponentTag(ServiceIndex, StreamIndex); }
	int GetVideoIndexByComponentTag(int ServiceIndex, uint8_t ComponentTag) const override { return m_pAnalyzer->GetVideoIndexByComponentTag(ServiceIndex, ComponentTag); }

	int GetAudioStreamCount(int ServiceIndex) const override { return m_pAnalyzer->GetAudioESCount(ServiceIndex); }
	uint16_t GetAudioStreamID(int ServiceIndex, int StreamIndex) const override { return m_pAnalyzer->GetAudioESPID(ServiceIndex, StreamIndex); }
	uint8_t GetAudioStreamType(int ServiceIndex, int StreamIndex) const override { return m_pAnalyzer->GetAudioStreamType(ServiceIndex, StreamIndex); }
	uint8_t GetAudioComponentTag(int ServiceIndex, int StreamIndex) const override { return m_pAnalyzer->GetAudioComponentTag(ServiceIndex, StreamIndex); }
	int GetAudioIndexByComponentTag(int ServiceIndex, uint8_t ComponentTag) const override { return m_pAnalyzer->GetAudioIndexByComponentTag(ServiceIndex, ComponentTag); }

	uint16_t GetEventID(int ServiceIndex) const override { return m_pAnalyzer->GetEventID(ServiceIndex); }

private:
	const AnalyzerFilter *m_pAnalyzer;
};


class TSEngine::MMTServiceSource
	: public TSEngine::ServiceSource
{
public:
	MMTServiceSource(const MMTAnalyzerFilter *pAnalyzer) noexcept : m_pAnalyzer(pAnalyzer) {}

	bool IsMMT() const noexcept override { return true; }
	uint16_t GetInvalidStreamID() const noexcept override { return MMT_PACKET_ID_INVALID; }
	uint16_t GetStreamID() const override { return m_pAnalyzer->GetTLVStreamID(); }
	uint16_t GetNetworkID() const override { return m_pAnalyzer->GetNetworkID(); }

	int GetServiceCount() const override { return m_pAnalyzer->GetServiceCount(); }
	uint16_t GetServiceID(int Index) const override { return m_pAnalyzer->GetServiceID((Index < 0) ? 0 : Index); }
	int GetServiceIndexByID(uint16_t ServiceID) const override { return m_pAnalyzer->GetServiceIndexByID(ServiceID); }
	bool IsServiceInfoAcquired(int Index) const override { return m_pAnalyzer->IsServiceMPTAcquired(Index); }

	int GetVideoStreamCount(int ServiceIndex) const override { return m_pAnalyzer->GetVideoAssetCount(ServiceIndex); }

	uint16_t GetVideoStreamID(int ServiceIndex, int StreamIndex) const override
	{
		MMTAnalyzerFilter::AssetInfo Info;
		if (!m_pAnalyzer->GetVideoAssetInfo(ServiceIndex, StreamIndex, &Info))
			return MMT_PACKET_ID_INVALID;
		return Info.PacketID;
	}

	uint8_t GetVideoStreamType(int ServiceIndex, int StreamIndex) const override
	{
		MMTAnalyzerFilter::AssetInfo Info;
		if (!m_pAnalyzer->GetVideoAssetInfo(ServiceIndex, StreamIndex, &Info))
			return STREAM_TYPE_INVALID;
		switch (Info.AssetType) {
		case MMT_ASSET_TYPE_HEV1:
		case MMT_ASSET_TYPE_HVC1:
			return STREAM_TYPE_H265;
		}
		return STREAM_TYPE_INVALID;
	}

	uint8_t GetVideoComponentTag(int ServiceIndex, int StreamIndex) const override
	{
		MMTAnalyzerFilter::AssetInfo Info;
		if (!m_pAnalyzer->GetVideoAssetInfo(ServiceIndex, StreamIndex, &Info))
			return COMPONENT_TAG_INVALID;
		return ToComponentTag8(Info.ComponentTag);
	}

	int GetVideoIndexByComponentTag(int ServiceIndex, uint8_t ComponentTag) const override
	{
		return m_pAnalyzer->GetVideoAssetIndexByComponentTag(ServiceIndex, ComponentTag);
	}

	int GetAudioStreamCount(int ServiceIndex) const override { return m_pAnalyzer->GetAudioAssetCount(ServiceIndex); }

	uint16_t GetAudioStreamID(int ServiceIndex, int StreamIndex) const override
	{
		MMTAnalyzerFilter::AssetInfo Info;
		if (!m_pAnalyzer->GetAudioAssetInfo(ServiceIndex, StreamIndex, &Info))
			return MMT_PACKET_ID_INVALID;
		return Info.PacketID;
	}

	uint8_t GetAudioStreamType(int ServiceIndex, int StreamIndex) const override
	{
		MMTAnalyzerFilter::AssetInfo Info;
		if (!m_pAnalyzer->GetAudioAssetInfo(ServiceIndex, StreamIndex, &Info))
			return STREAM_TYPE_INVALID;
		return Info.StreamType;
	}

	uint8_t GetAudioComponentTag(int ServiceIndex, int StreamIndex) const override
	{
		MMTAnalyzerFilter::AssetInfo Info;
		if (!m_pAnalyzer->GetAudioAssetInfo(ServiceIndex, StreamIndex, &Info))
			return COMPONENT_TAG_INVALID;
		return ToComponentTag8(Info.ComponentTag);
	}

	int GetAudioIndexByComponentTag(int ServiceIndex, uint8_t ComponentTag) const override
	{
		return m_pAnalyzer->GetAudioAssetIndexByComponentTag(ServiceIndex, ComponentTag);
	}

	bool GetAudioComponentInfo(int ServiceIndex, int StreamIndex, ReturnArg<EventInfo::AudioInfo> Info) const override
	{
		MMTAnalyzerFilter::AssetInfo Asset;
		if (!Info || !m_pAnalyzer->GetAudioAssetInfo(ServiceIndex, StreamIndex, &Asset) || !Asset.HasAudioInfo)
			return false;
		*Info = Asset.AudioInfo;
		return true;
	}

	uint16_t GetEventID(int ServiceIndex) const override { return m_pAnalyzer->GetEventID(ServiceIndex); }

private:
	static uint8_t ToComponentTag8(uint16_t ComponentTag) noexcept
	{
		return (ComponentTag < COMPONENT_TAG_INVALID) ? static_cast<uint8_t>(ComponentTag) : COMPONENT_TAG_INVALID;
	}

	const MMTAnalyzerFilter *m_pAnalyzer;
};




TSEngine::TSEngine() noexcept
	: m_IsBuilt(false)

	, m_pSource(nullptr)
	, m_pAnalyzer(nullptr)
	, m_pStreamParser(nullptr)
	, m_pMMTAnalyzer(nullptr)
	, m_IsMMTStreamStarted(false)
	, m_LastStreamFormat(StreamFormat::Unknown)

	, m_CurTransportStreamID(TRANSPORT_STREAM_ID_INVALID)
	, m_CurServiceID(SERVICE_ID_INVALID)

	, m_VideoStreamType(STREAM_TYPE_UNINITIALIZED)
	, m_AudioStreamType(STREAM_TYPE_UNINITIALIZED)
	, m_CurVideoStream(-1)
	, m_CurVideoComponentTag(COMPONENT_TAG_INVALID)
	, m_CurAudioStream(-1)
	, m_CurAudioComponentTag(COMPONENT_TAG_INVALID)
	, m_CurEventID(EVENT_ID_INVALID)

	, m_StartStreamingOnSourceOpen(false)
{
}


TSEngine::~TSEngine()
{
	CloseEngine();
}


bool TSEngine::BuildEngine(const FilterGraph::ConnectionInfo *pConnectionList, size_t ConnectionCount)
{
	if (LIBISDB_TRACE_ERROR_IF(m_IsBuilt))
		return true;

	Log(Logger::LogType::Information, LIBISDB_STR("Building filter graph..."));

	if (!m_FilterGraph.ConnectFilters(pConnectionList, ConnectionCount))
		return false;

	m_FilterGraph.WalkGraph(
		[this](FilterBase *pFilter) {
			pFilter->SetLogger(this->m_pLogger);
			pFilter->Initialize();
		});

	m_IsBuilt = true;

	return true;
}


bool TSEngine::BuildEngine(std::initializer_list<FilterBase *> FilterList)
{
	if (LIBISDB_TRACE_ERROR_IF(FilterList.size() < 2))
		return false;

	std::vector<FilterGraph::ConnectionInfo> ConnectionList(FilterList.size() - 1);

	auto it = FilterList.begin();
	FilterGraph::IDType PrevID = RegisterFilter(*it);
	size_t i = 0;

	for (++it; it != FilterList.end(); ++it) {
		const FilterGraph::IDType ID = RegisterFilter(*it);

		ConnectionList[i].UpstreamFilterID = PrevID;
		ConnectionList[i].DownstreamFilterID = ID;
		PrevID = ID;
		i++;
	}

	return BuildEngine(ConnectionList.data(), ConnectionList.size());
}


bool TSEngine::CloseEngine()
{
	if (!m_IsBuilt)
		return true;

	Log(Logger::LogType::Information, LIBISDB_STR("Closing TSEngine..."));

	OnEngineClose();

	CloseSource();

	m_FilterGraph.WalkGraph(
		[this](FilterBase *pFilter) {
			pFilter->Finalize();
			if (pFilter->GetLogger() == this->m_pLogger)
				pFilter->SetLogger(nullptr);
		});

	m_FilterGraph.DisconnectFilters();

	m_IsBuilt = false;

	OnEngineClosed();

	Log(Logger::LogType::Information, LIBISDB_STR("TSEngine closed"));

	return true;
}


bool TSEngine::ResetEngine()
{
	if (!m_IsBuilt)
		return false;

	FilterBase *pRoot = m_FilterGraph.GetRootFilter();
	if (pRoot != nullptr)
		pRoot->ResetGraph();

	return true;
}


FilterGraph::IDType TSEngine::RegisterFilter(FilterBase *pFilter)
{
	if (LIBISDB_TRACE_ERROR_IF(pFilter == nullptr))
		return 0;

	const FilterGraph::IDType ID = m_FilterGraph.RegisterFilter(pFilter);
	if (ID == 0)
		return 0;

	OnFilterRegistered(pFilter, ID);

	return ID;
}


bool TSEngine::OpenSource(const String &Name)
{
	CloseSource();

	if (LIBISDB_TRACE_ERROR_IF(m_pSource == nullptr)) {
		return false;
	}

	const FilterGraph::IDType SourceFilterID = m_FilterGraph.GetFilterID(m_pSource);

	m_FilterGraph.DisconnectFilter(SourceFilterID, FilterGraph::ConnectDirection::Downstream);

	// ソースフィルタを開く
	Log(Logger::LogType::Information, LIBISDB_STR("Opening source..."));
	const bool OK = m_pSource->OpenSource(Name);
	if (!OK) {
		SetError(m_pSource->GetLastErrorDescription());
	}

	m_FilterGraph.ConnectFilter(SourceFilterID, FilterGraph::ConnectDirection::Downstream);

	if (!OK)
		return false;

	if (m_StartStreamingOnSourceOpen) {
		Log(Logger::LogType::Information, LIBISDB_STR("Starting streaming..."));
		if (!m_pSource->StartStreaming()) {
			SetError(m_pSource->GetLastErrorDescription());
			return false;
		}
	}

	//ResetEngine();
	ResetStatus();

	return true;
}


bool TSEngine::CloseSource()
{
	// ソースフィルタを開放する
	if (m_pSource == nullptr)
		return false;

	m_pSource->StopStreaming();

	if (m_pSource->IsSourceOpen()) {
		Log(Logger::LogType::Information, LIBISDB_STR("Closing source..."));
		m_pSource->CloseSource();
	}

	return true;
}


bool TSEngine::IsSourceOpen() const
{
	return (m_pSource != nullptr) && m_pSource->IsSourceOpen();
}


bool TSEngine::SetServiceSelectInfo(const ServiceSelectInfo *pServiceSelInfo)
{
	BlockLock Lock(m_EngineLock);

	if (pServiceSelInfo != nullptr) {
		m_SetChannelServiceSel = *pServiceSelInfo;
	} else {
		m_SetChannelServiceSel.Reset();
	}

	return true;
}


bool TSEngine::SetService(const ServiceSelectInfo &ServiceSelInfo, bool Reserve)
{
	BlockLock Lock(m_EngineLock);

	const ServiceSource *pSource = GetServiceSource();
	if (LIBISDB_TRACE_ERROR_IF(pSource == nullptr))
		return false;

	uint16_t ServiceID = ServiceSelInfo.ServiceID;

	LIBISDB_TRACE(LIBISDB_STR("TSEngine::SetService() : service_id {:04X}\n"), ServiceID);

	// Reserve == true の場合、まだPATが来ていなくてもエラーにしない

	bool SetService = true, OneSeg = false;

	if (ServiceSelInfo.OneSegSelect == OneSegSelectType::HighPriority) {
		// ワンセグ優先
		uint16_t SID;
		if ((ServiceSelInfo.PreferredServiceIndex >= 0)
				&& (SID = pSource->Get1SegServiceIDByIndex(
						ServiceSelInfo.PreferredServiceIndex)) != SERVICE_ID_INVALID) {
			OneSeg = true;
			ServiceID = SID;
		} else if ((SID = pSource->GetFirst1SegServiceID()) != SERVICE_ID_INVALID) {
			OneSeg = true;
			ServiceID = SID;
		}
	}

	if (!OneSeg && (ServiceID != SERVICE_ID_INVALID)) {
		const int Index = GetSelectableServiceIndexByID(ServiceID);
		if (Index < 0) {
			if (!Reserve || (m_CurTransportStreamID != TRANSPORT_STREAM_ID_INVALID))
				return false;
			SetService = false;
		}
	}

	if (SetService)
		SelectService(ServiceID, ServiceSelInfo.OneSegSelect == OneSegSelectType::Refuse);

	m_ServiceSel = ServiceSelInfo;

	return true;
}


bool TSEngine::SelectService(uint16_t ServiceID, bool No1Seg)
{
	LIBISDB_TRACE(LIBISDB_STR("TSEngine::SelectService({:04X})\n"), ServiceID);

	BlockLock Lock(m_EngineLock);

	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return false;

	// サービス変更(ServiceID==SERVICE_ID_INVALIDならPAT先頭サービス)

	if (ServiceID == SERVICE_ID_INVALID) {
		LIBISDB_TRACE(LIBISDB_STR("Select default service\n"));
		// 先頭PMTが到着するまで失敗にする
		ServiceID = GetDefaultServiceID();
		if (ServiceID == SERVICE_ID_INVALID) {
			LIBISDB_TRACE(LIBISDB_STR("No viewable service\n"));
			return false;
		}
		if (No1Seg && pSource->Is1SegService(pSource->GetServiceIndexByID(ServiceID))) {
			return false;
		}
	} else {
		if (GetSelectableServiceIndexByID(ServiceID) < 0) {
			LIBISDB_TRACE(LIBISDB_STR("Service {:04X} not found\n"), ServiceID);
			return false;
		}
	}

	const bool ServiceChanged = (ServiceID != m_CurServiceID);
	const int CurServiceIndex = pSource->GetServiceIndexByID(ServiceID);
	const uint16_t InvalidStreamID = pSource->GetInvalidStreamID();

	m_CurServiceID = ServiceID;

	LIBISDB_TRACE(LIBISDB_STR("Select service : [{}] (service_id {:04X})\n"), CurServiceIndex, ServiceID);

	int VideoIndex;
	if (m_CurVideoComponentTag != COMPONENT_TAG_INVALID
			/* && !ServiceChanged */) {
		VideoIndex = pSource->GetVideoIndexByComponentTag(CurServiceIndex, m_CurVideoComponentTag);
		if (VideoIndex < 0)
			VideoIndex = 0;
	} else {
		VideoIndex = 0;
	}
	uint16_t VideoID = pSource->GetVideoStreamID(CurServiceIndex, VideoIndex);
	if ((VideoID == InvalidStreamID) && (VideoIndex != 0)) {
		VideoID = pSource->GetVideoStreamID(CurServiceIndex, 0);
		VideoIndex = 0;
	}
	m_CurVideoStream = VideoIndex;
	m_CurVideoComponentTag = pSource->GetVideoComponentTag(CurServiceIndex, VideoIndex);
	const uint8_t VideoStreamType = pSource->GetVideoStreamType(CurServiceIndex, VideoIndex);
	LIBISDB_TRACE(LIBISDB_STR("Select video : [{}] (component_tag {:02X})\n"), m_CurVideoStream, m_CurVideoComponentTag);

	int AudioIndex;
	if (pSource->IsMMT()) {
		EventInfo::AudioInfoList AudioList;
		GetAudioComponentList(pSource, CurServiceIndex, &AudioList);
		// 選局時と、サイマル・多言語の条件が変わった時は既定の音声にする (ARIB TR-B39 第1部 第2編 4.7.2 図 4-2)
		const bool AudioChanged = ServiceChanged || IsAudioSelectionConditionChanged(AudioList, m_AudioComponentList);
		m_AudioComponentList = std::move(AudioList);
		AudioIndex = -1;
		if (!AudioChanged && (m_CurAudioComponentTag != COMPONENT_TAG_INVALID)) {
			AudioIndex = pSource->GetAudioIndexByComponentTag(CurServiceIndex, m_CurAudioComponentTag);
			if ((AudioIndex >= 0) && !IsAudioStreamPlayable(pSource, CurServiceIndex, AudioIndex))
				AudioIndex = -1;
		}
		if (AudioIndex < 0)
			AudioIndex = GetDefaultAudioStreamIndex(pSource, CurServiceIndex);
	} else if (m_CurAudioComponentTag != COMPONENT_TAG_INVALID
			/* && !ServiceChanged */) {
		AudioIndex = pSource->GetAudioIndexByComponentTag(CurServiceIndex, m_CurAudioComponentTag);
		if (AudioIndex < 0)
			AudioIndex = 0;
	} else {
		AudioIndex = 0;
	}
	uint16_t AudioID = pSource->GetAudioStreamID(CurServiceIndex, AudioIndex);
	if ((AudioID == InvalidStreamID) && (AudioIndex != 0)) {
		AudioID = pSource->GetAudioStreamID(CurServiceIndex, 0);
		AudioIndex = 0;
	}
	m_CurAudioStream = AudioIndex;
	m_CurAudioComponentTag = pSource->GetAudioComponentTag(CurServiceIndex, AudioIndex);
	const uint8_t AudioStreamType = pSource->GetAudioStreamType(CurServiceIndex, AudioIndex);
	LIBISDB_TRACE(LIBISDB_STR("Select audio : [{}] (component_tag {:02X})\n"), m_CurAudioStream, m_CurAudioComponentTag);

	if (m_VideoStreamType != VideoStreamType) {
		LIBISDB_TRACE(LIBISDB_STR("Video stream_type changed ({:02X} -> {:02X})\n"), m_VideoStreamType, VideoStreamType);
		m_VideoStreamType = VideoStreamType;
		OnVideoStreamTypeChanged(VideoStreamType);
	}

	if (m_AudioStreamType != AudioStreamType) {
		LIBISDB_TRACE(LIBISDB_STR("Audio stream_type changed ({:02X} -> {:02X}\n"), m_AudioStreamType, AudioStreamType);
		m_AudioStreamType = AudioStreamType;
		OnAudioStreamTypeChanged(AudioStreamType);
	}

	m_FilterGraph.WalkGraph(
		[ServiceID](FilterBase *pFilter) { pFilter->SetActiveServiceID(ServiceID); });

	SetVideoStreamID(VideoID, true);
	// MMT では同じサービス内の音声の切り替えを映像を止めずに行う (ARIB TR-B39 第1部 第7編 7.5)
	SetAudioStreamID(AudioID, !pSource->IsMMT() || ServiceChanged);

#if 0
	if (m_WriteCurServiceOnly)
		SetWriteStream(ServiceID, m_WriteStream);
#endif

	if (ServiceChanged)
		OnServiceChanged(ServiceID);

	const uint16_t EventID = pSource->GetEventID(CurServiceIndex);
	if (ServiceChanged || (m_CurEventID != EventID)) {
		m_CurEventID = EventID;
		OnEventChanged(EventID);
	}

	return true;
}


uint16_t TSEngine::GetServiceID() const
{
	return m_CurServiceID;
}


int TSEngine::GetServiceIndex() const
{
	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return -1;
	return pSource->GetServiceIndexByID(m_CurServiceID);
}


bool TSEngine::SetOneSegSelectType(OneSegSelectType Type)
{
	BlockLock Lock(m_EngineLock);

	m_ServiceSel.OneSegSelect = Type;
	m_ServiceSel.PreferredServiceIndex = -1;

	return true;
}


uint16_t TSEngine::GetTransportStreamID() const
{
	return m_CurTransportStreamID;
}


uint16_t TSEngine::GetNetworkID() const
{
	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return NETWORK_ID_INVALID;
	return pSource->GetNetworkID();
}


uint16_t TSEngine::GetEventID() const
{
	return m_CurEventID;
}


StreamFormat TSEngine::GetStreamFormat() const
{
	// TSPacketParserFilter は TS のみを扱う
	if (m_pStreamParser == nullptr)
		return StreamFormat::TS;
	return m_pStreamParser->GetStreamFormat();
}


uint8_t TSEngine::GetVideoStreamType() const
{
	return m_VideoStreamType;
}


int TSEngine::GetVideoStreamCount(int ServiceIndex) const
{
	BlockLock Lock(m_EngineLock);

	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return 0;

	uint16_t ServiceID;

	if (ServiceIndex < 0) {
		ServiceID = m_CurServiceID;
	} else {
		ServiceID = GetSelectableServiceID(ServiceIndex);
	}

	if (ServiceID == SERVICE_ID_INVALID)
		return 0;

	return pSource->GetVideoStreamCount(pSource->GetServiceIndexByID(ServiceID));
}


bool TSEngine::SetVideoStream(int StreamIndex)
{
	BlockLock Lock(m_EngineLock);

	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return false;

	const int ServiceIndex = GetServiceIndex();
	if (ServiceIndex < 0)
		return false;

	const uint16_t VideoID = pSource->GetVideoStreamID(ServiceIndex, StreamIndex);
	if (VideoID == pSource->GetInvalidStreamID())
		return false;

	m_CurVideoStream = StreamIndex;
	m_CurVideoComponentTag = pSource->GetVideoComponentTag(ServiceIndex, StreamIndex);

	LIBISDB_TRACE(LIBISDB_STR("Select video : [{}] (component_tag {:02X})\n"), m_CurVideoStream, m_CurVideoComponentTag);

	const uint8_t VideoStreamType = pSource->GetVideoStreamType(ServiceIndex, StreamIndex);

	if (m_VideoStreamType != VideoStreamType) {
		LIBISDB_TRACE(LIBISDB_STR("Video stream_type changed ({:02x} -> {:02x})\n"), m_VideoStreamType, VideoStreamType);
		m_VideoStreamType = VideoStreamType;
		OnVideoStreamTypeChanged(VideoStreamType);
	}

	SetVideoStreamID(VideoID, false);

	return true;
}


int TSEngine::GetVideoStream() const
{
	return m_CurVideoStream;
}


uint8_t TSEngine::GetVideoComponentTag() const
{
	return m_CurVideoComponentTag;
}


uint8_t TSEngine::GetAudioStreamType() const
{
	return m_AudioStreamType;
}


int TSEngine::GetAudioStreamCount(int ServiceIndex) const
{
	BlockLock Lock(m_EngineLock);

	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return 0;

	uint16_t ServiceID;

	if (ServiceIndex < 0) {
		ServiceID = m_CurServiceID;
	} else {
		ServiceID = GetSelectableServiceID(ServiceIndex);
	}

	if (ServiceID == SERVICE_ID_INVALID)
		return 0;

	return pSource->GetAudioStreamCount(pSource->GetServiceIndexByID(ServiceID));
}


bool TSEngine::SetAudioStream(int StreamIndex)
{
	BlockLock Lock(m_EngineLock);

	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return false;

	const int ServiceIndex = GetServiceIndex();
	if (ServiceIndex < 0)
		return false;

	const uint16_t AudioID = pSource->GetAudioStreamID(ServiceIndex, StreamIndex);
	if (AudioID == pSource->GetInvalidStreamID())
		return false;

	m_CurAudioStream = StreamIndex;
	m_CurAudioComponentTag = pSource->GetAudioComponentTag(ServiceIndex, StreamIndex);

	LIBISDB_TRACE(LIBISDB_STR("Select audio : [{}] (component_tag {:02X})\n"), m_CurAudioStream, m_CurAudioComponentTag);

	SetAudioStreamID(AudioID, false);

	const uint8_t AudioStreamType = pSource->GetAudioStreamType(ServiceIndex, StreamIndex);

	if (m_AudioStreamType != AudioStreamType) {
		LIBISDB_TRACE(LIBISDB_STR("Audio stream_type changed ({:02x} -> {:02x})\n"), m_AudioStreamType, AudioStreamType);
		m_AudioStreamType = AudioStreamType;
		OnAudioStreamTypeChanged(AudioStreamType);
	}

	return true;
}


int TSEngine::GetAudioStream() const
{
	return m_CurAudioStream;
}


uint8_t TSEngine::GetAudioComponentTag() const
{
	return m_CurAudioComponentTag;
}


bool TSEngine::GetAudioComponentInfo(int StreamIndex, ReturnArg<EventInfo::AudioInfo> Info) const
{
	BlockLock Lock(m_EngineLock);

	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return false;

	const int ServiceIndex = GetServiceIndex();
	if (ServiceIndex < 0)
		return false;

	return pSource->GetAudioComponentInfo(ServiceIndex, StreamIndex, Info);
}


bool TSEngine::IsAudioStreamPlayable(int StreamIndex) const
{
	BlockLock Lock(m_EngineLock);

	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return false;

	const int ServiceIndex = GetServiceIndex();
	if ((ServiceIndex < 0) || (StreamIndex < 0) || (StreamIndex >= pSource->GetAudioStreamCount(ServiceIndex)))
		return false;

	return IsAudioStreamPlayable(pSource, ServiceIndex, StreamIndex);
}


int TSEngine::GetDefaultAudioStream() const
{
	BlockLock Lock(m_EngineLock);

	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return -1;

	const int ServiceIndex = GetServiceIndex();
	if ((ServiceIndex < 0) || (pSource->GetAudioStreamCount(ServiceIndex) <= 0))
		return -1;

	return GetDefaultAudioStreamIndex(pSource, ServiceIndex);
}


void TSEngine::SetStartStreamingOnSourceOpen(bool Start)
{
	m_StartStreamingOnSourceOpen = Start;
}


void TSEngine::SetLogger(Logger *pLogger)
{
	ObjectBase::SetLogger(pLogger);

	m_FilterGraph.EnumFilters([pLogger](FilterBase *pFilter) { pFilter->SetLogger(pLogger); });
}


void TSEngine::OnGraphReset(SourceFilter *pSource)
{
	ResetStatus();
}


void TSEngine::OnSourceChanged(SourceFilter *pSource)
{
	BlockLock Lock(m_EngineLock);

	m_ServiceSel = m_SetChannelServiceSel;
	ResetStatus();
}


void TSEngine::OnPATUpdated(AnalyzerFilter *pAnalyzer)
{
	LIBISDB_TRACE(LIBISDB_STR("TSEngine::OnPATUpdated()\n"));

	BlockLock Lock(m_EngineLock);

	if (!IsMMTStream())
		UpdateService(m_TSServiceSource.get());
}


void TSEngine::OnPMTUpdated(AnalyzerFilter *pAnalyzer, uint16_t ServiceID)
{
	OnPATUpdated(pAnalyzer);
}


void TSEngine::OnEITUpdated(AnalyzerFilter *pAnalyzer)
{
	if (!IsMMTStream())
		UpdateEvent(m_TSServiceSource.get());
}


void TSEngine::OnStreamFormatChanged(StreamPacketParserFilter *pParser, StreamFormat Format)
{
	LIBISDB_TRACE(LIBISDB_STR("TSEngine::OnStreamFormatChanged() : {}\n"), static_cast<int>(Format));

	BlockLock Lock(m_EngineLock);

	const StreamFormat LastFormat = m_LastStreamFormat;
	m_LastStreamFormat = Format;
	// 最初の判定の時点では何も選択されていない
	if ((LastFormat == StreamFormat::Unknown) || (Format == LastFormat))
		return;

	ResetStatus();

	m_CurVideoStream = -1;
	m_CurVideoComponentTag = COMPONENT_TAG_INVALID;
	m_CurAudioStream = -1;
	m_CurAudioComponentTag = COMPONENT_TAG_INVALID;
	m_CurEventID = EVENT_ID_INVALID;

	// stream_type の変化を通知させ、ビューアを新しい形式で構築し直させる
	m_VideoStreamType = STREAM_TYPE_UNINITIALIZED;
	m_AudioStreamType = STREAM_TYPE_UNINITIALIZED;

	m_FilterGraph.WalkGraph(
		[](FilterBase *pFilter) {
			pFilter->SetActiveVideoPID(PID_INVALID, true);
			pFilter->SetActiveAudioPID(PID_INVALID, true);
			pFilter->SetActiveVideoPacketID(MMT_PACKET_ID_INVALID, true);
			pFilter->SetActiveAudioPacketID(MMT_PACKET_ID_INVALID, true);
		});
}


void TSEngine::OnPLTUpdated(MMTAnalyzerFilter *pAnalyzer)
{
	LIBISDB_TRACE(LIBISDB_STR("TSEngine::OnPLTUpdated()\n"));

	BlockLock Lock(m_EngineLock);

	if (IsMMTStream())
		UpdateService(m_MMTServiceSource.get());
}


void TSEngine::OnMPTUpdated(MMTAnalyzerFilter *pAnalyzer, uint16_t ServiceID)
{
	OnPLTUpdated(pAnalyzer);
}


void TSEngine::OnMHSDTUpdated(MMTAnalyzerFilter *pAnalyzer)
{
	BlockLock Lock(m_EngineLock);

	// tlv_stream_id は MH-SDT で分かる
	if (IsMMTStream() && m_IsMMTStreamStarted
			&& (m_CurTransportStreamID == TRANSPORT_STREAM_ID_INVALID)) {
		m_CurTransportStreamID = pAnalyzer->GetTLVStreamID();
		LIBISDB_TRACE(LIBISDB_STR("tlv_stream_id {:04X}\n"), m_CurTransportStreamID);
	}
}


void TSEngine::OnMHEITUpdated(MMTAnalyzerFilter *pAnalyzer)
{
	if (IsMMTStream())
		UpdateEvent(m_MMTServiceSource.get());
}


const TSEngine::ServiceSource * TSEngine::GetServiceSource() const
{
	if (IsMMTStream())
		return m_MMTServiceSource.get();
	return m_TSServiceSource.get();
}


bool TSEngine::IsMMTStream() const
{
	return (m_pMMTAnalyzer != nullptr) && (GetStreamFormat() == StreamFormat::TLV);
}


void TSEngine::UpdateService(const ServiceSource *pSource)
{
	if (pSource == nullptr)
		return;

	const uint16_t TransportStreamID = pSource->GetStreamID();
	bool NewStream;

	if (pSource->IsMMT()) {
		// 最初の PLT の時点では tlv_stream_id が分からないことがある
		NewStream =
			!m_IsMMTStreamStarted
			|| ((TransportStreamID != TRANSPORT_STREAM_ID_INVALID)
				&& (m_CurTransportStreamID != TRANSPORT_STREAM_ID_INVALID)
				&& (TransportStreamID != m_CurTransportStreamID));
		m_IsMMTStreamStarted = true;
		if (!NewStream && (TransportStreamID != TRANSPORT_STREAM_ID_INVALID))
			m_CurTransportStreamID = TransportStreamID;
	} else {
		NewStream = (m_CurTransportStreamID != TransportStreamID);
	}

	if (NewStream) {
		// ストリームIDが変わっているなら初期化
		LIBISDB_TRACE(LIBISDB_STR("Stream changed ({:04X} <- {:04X})\n"), TransportStreamID, m_CurTransportStreamID);

		m_CurTransportStreamID = TransportStreamID;
		m_CurServiceID = SERVICE_ID_INVALID;
		m_CurVideoStream = -1;
		m_CurVideoComponentTag = COMPONENT_TAG_INVALID;
		m_CurAudioStream = -1;
		m_CurAudioComponentTag = COMPONENT_TAG_INVALID;
		m_CurEventID = EVENT_ID_INVALID;

		bool SetService = true;
		uint16_t ServiceID = SERVICE_ID_INVALID;

		if (m_ServiceSel.OneSegSelect == OneSegSelectType::HighPriority) {
			// ワンセグ優先
			// この段階ではまだ保留
			SetService = false;
		} else if (m_ServiceSel.ServiceID != SERVICE_ID_INVALID) {
			// サービスが指定されている
			const int ServiceIndex = pSource->GetServiceIndexByID(m_ServiceSel.ServiceID);
			if (ServiceIndex < 0) {
				// サービスがPATにない
				LIBISDB_TRACE(LIBISDB_STR("Specified service_id {:04X} not found in PAT\n"), m_ServiceSel.ServiceID);
				SetService = false;
			} else {
				if (GetSelectableServiceIndexByID(m_ServiceSel.ServiceID) >= 0) {
					ServiceID = m_ServiceSel.ServiceID;
				} else {
					SetService = false;
				}
			}
		}

		if (!SetService || !SelectService(ServiceID, m_ServiceSel.OneSegSelect == OneSegSelectType::Refuse)) {
			ResetVideoAndAudioStreamID(true);
		}
	} else {
		bool SetService = true, OneSeg = false;
		uint16_t ServiceID = SERVICE_ID_INVALID;

		if (m_ServiceSel.OneSegSelect == OneSegSelectType::HighPriority) {
			// ワンセグ優先
			uint16_t SID;
			if (m_ServiceSel.PreferredServiceIndex >= 0) {
				SID = pSource->Get1SegServiceIDByIndex(m_ServiceSel.PreferredServiceIndex);
				if (SID != SERVICE_ID_INVALID) {
					OneSeg = true;
					if (pSource->IsServiceInfoAcquired(pSource->GetServiceIndexByID(SID))) {
						ServiceID = SID;
					}
				}
			}
			if ((ServiceID == SERVICE_ID_INVALID)
					&& ((SID = pSource->GetFirst1SegServiceID()) != SERVICE_ID_INVALID)) {
				OneSeg = true;
				if (pSource->IsServiceInfoAcquired(pSource->GetServiceIndexByID(SID))) {
					ServiceID = SID;
				} else {
					SetService = false;
				}
			}
		}

		if (!OneSeg && (m_ServiceSel.ServiceID != SERVICE_ID_INVALID)) {
			const int ServiceIndex = pSource->GetServiceIndexByID(m_ServiceSel.ServiceID);
			if (ServiceIndex < 0) {
				LIBISDB_TRACE(LIBISDB_STR("Specified service_id {:04X} not found in PAT\n"), m_ServiceSel.ServiceID);
				if (((m_CurServiceID == SERVICE_ID_INVALID) && !m_ServiceSel.FollowViewableService)
						|| (GetSelectableServiceCount() == 0)) {
					SetService = false;
				}
			} else {
				if (GetSelectableServiceIndexByID(m_ServiceSel.ServiceID) >= 0) {
					ServiceID = m_ServiceSel.ServiceID;
				} else if (((m_CurServiceID == SERVICE_ID_INVALID) && !m_ServiceSel.FollowViewableService)
						|| !pSource->IsServiceInfoAcquired(ServiceIndex)) {
					// サービスはPATにあるが、まだPMTが来ていない
					SetService = false;
				}
			}
		}

		if (SetService && (ServiceID == SERVICE_ID_INVALID) && (m_CurServiceID != SERVICE_ID_INVALID)) {
			const int ServiceIndex = pSource->GetServiceIndexByID(m_CurServiceID);
			if (ServiceIndex < 0) {
				// サービスがPATにない
				LIBISDB_TRACE(LIBISDB_STR("Current service_id {:04X} not found in PAT\n"), m_CurServiceID);
				if (m_ServiceSel.FollowViewableService
						&& (GetSelectableServiceCount() > 0)) {
					m_CurServiceID = SERVICE_ID_INVALID;
				} else {
					// まだ視聴可能なサービスのPMTが一つも来ていない場合は保留
					SetService = false;
				}
			} else {
				if (GetSelectableServiceIndexByID(m_CurServiceID) >= 0) {
					ServiceID = m_CurServiceID;
				} else if (!m_ServiceSel.FollowViewableService
						|| !pSource->IsServiceInfoAcquired(ServiceIndex)) {
					SetService = false;
				}
			}
		}

		if (SetService)
			SelectService(ServiceID, m_ServiceSel.OneSegSelect == OneSegSelectType::Refuse);
	}
}


void TSEngine::UpdateEvent(const ServiceSource *pSource)
{
	if (pSource == nullptr)
		return;

	const uint16_t EventID = pSource->GetEventID(pSource->GetServiceIndexByID(m_CurServiceID));

	if (m_CurEventID != EventID) {
		m_CurEventID = EventID;
		OnEventChanged(EventID);
	}
}


bool TSEngine::IsAudioStreamPlayable(const ServiceSource *pSource, int ServiceIndex, int StreamIndex) const
{
	EventInfo::AudioInfo Info{};
	const bool HasInfo = pSource->GetAudioComponentInfo(ServiceIndex, StreamIndex, &Info);
	return IsAudioPlayable(pSource->GetAudioStreamType(ServiceIndex, StreamIndex), HasInfo ? &Info : nullptr);
}


int TSEngine::GetDefaultAudioStreamIndex(const ServiceSource *pSource, int ServiceIndex) const
{
	if (!pSource->IsMMT())
		return 0;

	const int StreamCount = pSource->GetAudioStreamCount(ServiceIndex);

	// 再生できる音声のうち component_tag が最小のもの
	int Default = -1;
	uint8_t DefaultTag = COMPONENT_TAG_INVALID;
	for (int i = 0; i < StreamCount; i++) {
		const uint8_t Tag = pSource->GetAudioComponentTag(ServiceIndex, i);
		if (((Default < 0) || (Tag < DefaultTag)) && IsAudioStreamPlayable(pSource, ServiceIndex, i)) {
			Default = i;
			DefaultTag = Tag;
		}
	}
	if (Default < 0)
		return 0;

	// 2ch で再生する場合は、サイマル音声に AAC 2ch があればそれを優先する
	EventInfo::AudioInfo DefaultInfo{};
	if (IsStereoAudioPreferred()
			&& pSource->GetAudioComponentInfo(ServiceIndex, Default, &DefaultInfo)
			&& (DefaultInfo.SimulcastGroupTag != MMT_SIMULCAST_GROUP_TAG_NONE)
			&& ((DefaultInfo.ComponentType & MMT_AUDIO_MODE_MASK) > MMT_AUDIO_MODE_2_0)) {
		int Stereo = -1;
		uint8_t StereoTag = COMPONENT_TAG_INVALID;
		for (int i = 0; i < StreamCount; i++) {
			EventInfo::AudioInfo Info{};
			const uint8_t Tag = pSource->GetAudioComponentTag(ServiceIndex, i);
			if ((i != Default)
					&& ((Stereo < 0) || (Tag < StereoTag))
					&& pSource->GetAudioComponentInfo(ServiceIndex, i, &Info)
					&& (Info.SimulcastGroupTag == DefaultInfo.SimulcastGroupTag)
					&& (Info.StreamContent == MMT_AUDIO_STREAM_CONTENT_AAC)
					&& ((Info.ComponentType & MMT_AUDIO_MODE_MASK) == MMT_AUDIO_MODE_2_0)
					&& IsAudioStreamPlayable(pSource, ServiceIndex, i)) {
				Stereo = i;
				StereoTag = Tag;
			}
		}
		if (Stereo >= 0)
			return Stereo;
	}

	return Default;
}


bool TSEngine::IsAudioSelectionConditionChanged(
	const EventInfo::AudioInfoList &List1, const EventInfo::AudioInfoList &List2)
{
	if (List1.size() != List2.size())
		return true;

	for (size_t i = 0; i < List1.size(); i++) {
		const EventInfo::AudioInfo &Info1 = List1[i];
		const EventInfo::AudioInfo &Info2 = List2[i];

		if ((Info1.ComponentTag != Info2.ComponentTag)
				|| (Info1.StreamContent != Info2.StreamContent)
				|| (Info1.SimulcastGroupTag != Info2.SimulcastGroupTag)
				|| (Info1.ESMultiLingualFlag != Info2.ESMultiLingualFlag)
				|| (Info1.LanguageCode != Info2.LanguageCode)
				|| (Info1.LanguageCode2 != Info2.LanguageCode2))
			return true;
	}

	return false;
}


void TSEngine::GetAudioComponentList(const ServiceSource *pSource, int ServiceIndex, EventInfo::AudioInfoList *pList) const
{
	const int StreamCount = pSource->GetAudioStreamCount(ServiceIndex);

	pList->clear();
	pList->resize(std::max(StreamCount, 0));

	for (int i = 0; i < StreamCount; i++) {
		EventInfo::AudioInfo &Info = (*pList)[i];
		if (!pSource->GetAudioComponentInfo(ServiceIndex, i, &Info))
			Info = EventInfo::AudioInfo{};
		Info.ComponentTag = pSource->GetAudioComponentTag(ServiceIndex, i);
	}
}


void TSEngine::ResetStatus()
{
	m_CurTransportStreamID = TRANSPORT_STREAM_ID_INVALID;
	m_CurServiceID = SERVICE_ID_INVALID;
	m_IsMMTStreamStarted = false;
}


void TSEngine::SetVideoPID(uint16_t PID, bool ServiceChanged)
{
	m_FilterGraph.WalkGraph([=](FilterBase *pFilter) { pFilter->SetActiveVideoPID(PID, ServiceChanged); });
}


void TSEngine::SetAudioPID(uint16_t PID, bool ServiceChanged)
{
	m_FilterGraph.WalkGraph([=](FilterBase *pFilter) { pFilter->SetActiveAudioPID(PID, ServiceChanged); });
}


void TSEngine::SetVideoStreamID(uint16_t ID, bool ServiceChanged)
{
	if (IsMMTStream())
		m_FilterGraph.WalkGraph([=](FilterBase *pFilter) { pFilter->SetActiveVideoPacketID(ID, ServiceChanged); });
	else
		SetVideoPID(ID, ServiceChanged);
}


void TSEngine::SetAudioStreamID(uint16_t ID, bool ServiceChanged)
{
	if (IsMMTStream())
		m_FilterGraph.WalkGraph([=](FilterBase *pFilter) { pFilter->SetActiveAudioPacketID(ID, ServiceChanged); });
	else
		SetAudioPID(ID, ServiceChanged);
}


void TSEngine::ResetVideoAndAudioStreamID(bool ServiceChanged)
{
	const uint16_t InvalidID = IsMMTStream() ? MMT_PACKET_ID_INVALID : PID_INVALID;

	SetVideoStreamID(InvalidID, ServiceChanged);
	SetAudioStreamID(InvalidID, ServiceChanged);
}


bool TSEngine::IsSelectableService(int Index) const
{
	const ServiceSource *pSource = GetServiceSource();

	return (pSource != nullptr)
		&& (Index >= 0)
		&& (Index < pSource->GetServiceCount());
}


int TSEngine::GetSelectableServiceCount() const
{
	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return 0;
	return pSource->GetServiceCount();
}


uint16_t TSEngine::GetSelectableServiceID(int Index) const
{
	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return SERVICE_ID_INVALID;
	if (pSource->IsMMT() && (Index < 0))
		return SERVICE_ID_INVALID;
	return pSource->GetServiceID(Index);
}


uint16_t TSEngine::GetDefaultServiceID() const
{
	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return SERVICE_ID_INVALID;
	return pSource->GetServiceID(-1);
}


int TSEngine::GetSelectableServiceIndexByID(uint16_t ServiceID) const
{
	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return -1;
	return pSource->GetServiceIndexByID(ServiceID);
}


bool TSEngine::GetSelectableServiceList(AnalyzerFilter::ServiceList *pList) const
{
	if ((m_pAnalyzer == nullptr) || IsMMTStream())
		return false;
	return m_pAnalyzer->GetServiceList(pList);
}


bool TSEngine::GetSelectableMMTServiceList(MMTAnalyzerFilter::ServiceList *pList) const
{
	if (!IsMMTStream())
		return false;
	return m_pMMTAnalyzer->GetServiceList(pList);
}


void TSEngine::OnFilterRegistered(FilterBase *pFilter, FilterGraph::IDType ID)
{
	AnalyzerFilter *pAnalyzer = dynamic_cast<AnalyzerFilter *>(pFilter);
	if (pAnalyzer != nullptr) {
		pAnalyzer->AddEventListener(this);
		m_pAnalyzer = pAnalyzer;
		m_TSServiceSource = std::make_unique<TSServiceSource>(pAnalyzer);
		return;
	}

	SourceFilter *pSource = dynamic_cast<SourceFilter *>(pFilter);
	if (pSource != nullptr) {
		pSource->AddEventListener(this);
		m_pSource = pSource;
		return;
	}

	StreamPacketParserFilter *pStreamParser = dynamic_cast<StreamPacketParserFilter *>(pFilter);
	if (pStreamParser != nullptr) {
		pStreamParser->AddEventListener(this);
		m_pStreamParser = pStreamParser;
		return;
	}

	MMTAnalyzerFilter *pMMTAnalyzer = dynamic_cast<MMTAnalyzerFilter *>(pFilter);
	if (pMMTAnalyzer != nullptr) {
		pMMTAnalyzer->AddEventListener(this);
		m_pMMTAnalyzer = pMMTAnalyzer;
		m_MMTServiceSource = std::make_unique<MMTServiceSource>(pMMTAnalyzer);
		return;
	}
}


} // namespace LibISDB
