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
 @file   MMTAnalyzerFilter.cpp
 @brief  MMT 解析フィルタ
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "MMTAnalyzerFilter.hpp"
#include "../MMT/MHEventInfo.hpp"
#include "../Utilities/Sort.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


template<typename T> const T * MMTAnalyzerFilter::GetSectionTable(uint16_t PacketID) const
{
	const MMTSectionTable *pTable = m_PacketIDMapManager.GetMapTarget<MMTSectionTable>(PacketID);
	if (pTable == nullptr)
		return nullptr;

	return pTable->GetTable<T>();
}




MMTAnalyzerFilter::MMTAnalyzerFilter()
	: m_TLVSITableSet(static_cast<TLVSITableSet *>(
		PSITableBase::CreateWithHandler<TLVSITableSet>(&MMTAnalyzerFilter::OnTLVSISection, this)))
	, m_TLVSIParser(m_TLVSITableSet.get())
{
	Reset();
}


void MMTAnalyzerFilter::Reset()
{
	BlockLock Lock(m_FilterLock);

	m_PacketIDMapManager.UnmapAllTargets();
	m_TLVSITableSet->Reset();
	m_TLVSIParser.Reset();

	m_TLVStreamID = TRANSPORT_STREAM_ID_INVALID;
	m_NetworkID = NETWORK_ID_INVALID;

	m_PLTUpdated = false;
	m_SDTUpdated = false;
	m_NITUpdated = false;
	m_EITUpdated = false;
	m_SendEITUpdatedEvent = false;

	m_ServiceList.clear();
	m_SDTServiceList.clear();
	m_SDTStreamMap.clear();
	m_NetworkStreamList.clear();
	m_NITInfo.Reset();
	m_EMMPacketIDList.clear();

	m_LastNTPTimestamp = 0;
	m_TOTNTPTimestamp = 0;

	m_PacketIDMapManager.MapTarget(
		MMT_PACKET_ID_PA,
		MMTSingleTable::CreateWithHandler<PLTable>(&MMTAnalyzerFilter::OnPLTTable, this));
	m_PacketIDMapManager.MapTarget(
		MMT_PACKET_ID_CA,
		MMTSingleTable::CreateWithHandler<MHCATTable>(&MMTAnalyzerFilter::OnMHCATTable, this));
	m_PacketIDMapManager.MapTarget(
		MMT_PACKET_ID_MH_EIT,
		MMTSectionTable::CreateWithHandler<MHEITPfTable>(&MMTAnalyzerFilter::OnMHEITSection, this));
	m_PacketIDMapManager.MapTarget(
		MMT_PACKET_ID_MH_BIT,
		MMTSectionTable::CreateWithHandler<MHBITMultiTable>(&MMTAnalyzerFilter::OnMHBITSection, this));
	m_PacketIDMapManager.MapTarget(
		MMT_PACKET_ID_MH_SDT,
		MMTSectionTable::CreateWithHandler<MHSDTTableSet>(&MMTAnalyzerFilter::OnMHSDTSection, this));
	m_PacketIDMapManager.MapTarget(
		MMT_PACKET_ID_MH_TOT,
		MMTSectionTable::CreateWithHandler<MHTOTTable>(&MMTAnalyzerFilter::OnMHTOTSection, this));
}


bool MMTAnalyzerFilter::ReceiveData(DataStream *pData)
{
	{
		BlockLock Lock(m_FilterLock);

		if (pData->Is<TLVPacket>()) {
			switch (pData->Get<TLVPacket>()->GetPayloadKind()) {
			case TLVPacket::PayloadKind::MMTP:
				m_PacketIDMapManager.StorePacketStream(pData);
				break;

			case TLVPacket::PayloadKind::Signaling:
				do {
					m_TLVSIParser.StorePacket(pData->Get<TLVPacket>());
				} while (pData->Next());
				break;

			case TLVPacket::PayloadKind::NTP:
				do {
					m_LastNTPTimestamp = pData->Get<TLVPacket>()->GetNTPTransmitTimestamp();
				} while (pData->Next());
				break;

			default:
				break;
			}
		}

		OutputData(pData);
	}

	// PLT より前に来ていた MH-EIT の更新を通知する
	if (m_SendEITUpdatedEvent) {
		m_SendEITUpdatedEvent = false;
		m_EventListenerList.CallEventListener(&EventListener::OnMHEITUpdated, this);
	}

	return true;
}


int MMTAnalyzerFilter::GetServiceCount() const
{
	BlockLock Lock(m_FilterLock);

	return static_cast<int>(m_ServiceList.size());
}


uint16_t MMTAnalyzerFilter::GetServiceID(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return SERVICE_ID_INVALID;

	return m_ServiceList[Index].ServiceID;
}


int MMTAnalyzerFilter::GetServiceIndexByID(uint16_t ServiceID) const
{
	BlockLock Lock(m_FilterLock);

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i].ServiceID == ServiceID)
			return static_cast<int>(i);
	}

	return -1;
}


bool MMTAnalyzerFilter::GetServiceInfo(int Index, ReturnArg<ServiceInfo> Info) const
{
	if (!Info)
		return false;

	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;

	*Info = m_ServiceList[Index];

	return true;
}


bool MMTAnalyzerFilter::GetServiceInfoByID(uint16_t ServiceID, ReturnArg<ServiceInfo> Info) const
{
	BlockLock Lock(m_FilterLock);

	return GetServiceInfo(GetServiceIndexByID(ServiceID), Info);
}


bool MMTAnalyzerFilter::GetServiceList(ReturnArg<ServiceList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	*List = m_ServiceList;

	return true;
}


bool MMTAnalyzerFilter::IsServiceMPTAcquired(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;

	return m_ServiceList[Index].IsMPTAcquired;
}


uint16_t MMTAnalyzerFilter::GetMPTPacketID(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return MMT_PACKET_ID_INVALID;

	return m_ServiceList[Index].MPTPacketID;
}


bool MMTAnalyzerFilter::GetServiceName(int Index, ReturnArg<String> Name) const
{
	if (!Name)
		return false;

	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;

	*Name = m_ServiceList[Index].ServiceName;

	return true;
}


uint8_t MMTAnalyzerFilter::GetServiceType(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return SERVICE_TYPE_INVALID;

	return m_ServiceList[Index].ServiceType;
}


uint16_t MMTAnalyzerFilter::GetLogoID(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return LOGO_ID_INVALID;

	return m_ServiceList[Index].LogoID;
}


int MMTAnalyzerFilter::GetVideoAssetCount(int ServiceIndex) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(ServiceIndex) >= m_ServiceList.size())
		return 0;

	return static_cast<int>(m_ServiceList[ServiceIndex].VideoAssetList.size());
}


bool MMTAnalyzerFilter::GetVideoAssetInfo(int ServiceIndex, int AssetIndex, ReturnArg<AssetInfo> Info) const
{
	if (!Info)
		return false;

	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(ServiceIndex) >= m_ServiceList.size())
		return false;
	const AssetInfoList &List = m_ServiceList[ServiceIndex].VideoAssetList;
	if (static_cast<unsigned int>(AssetIndex) >= List.size())
		return false;

	*Info = List[AssetIndex];

	return true;
}


int MMTAnalyzerFilter::GetVideoAssetIndexByComponentTag(int ServiceIndex, uint16_t ComponentTag) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(ServiceIndex) >= m_ServiceList.size())
		return -1;

	return GetAssetIndexByComponentTag(m_ServiceList[ServiceIndex].VideoAssetList, ComponentTag);
}


int MMTAnalyzerFilter::GetAudioAssetCount(int ServiceIndex) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(ServiceIndex) >= m_ServiceList.size())
		return 0;

	return static_cast<int>(m_ServiceList[ServiceIndex].AudioAssetList.size());
}


bool MMTAnalyzerFilter::GetAudioAssetInfo(int ServiceIndex, int AssetIndex, ReturnArg<AssetInfo> Info) const
{
	if (!Info)
		return false;

	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(ServiceIndex) >= m_ServiceList.size())
		return false;
	const AssetInfoList &List = m_ServiceList[ServiceIndex].AudioAssetList;
	if (static_cast<unsigned int>(AssetIndex) >= List.size())
		return false;

	*Info = List[AssetIndex];

	return true;
}


int MMTAnalyzerFilter::GetAudioAssetIndexByComponentTag(int ServiceIndex, uint16_t ComponentTag) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(ServiceIndex) >= m_ServiceList.size())
		return -1;

	return GetAssetIndexByComponentTag(m_ServiceList[ServiceIndex].AudioAssetList, ComponentTag);
}


uint16_t MMTAnalyzerFilter::GetTLVStreamID() const
{
	BlockLock Lock(m_FilterLock);

	return m_TLVStreamID;
}


uint16_t MMTAnalyzerFilter::GetNetworkID() const
{
	BlockLock Lock(m_FilterLock);

	return m_NetworkID;
}


bool MMTAnalyzerFilter::GetNetworkName(ReturnArg<String> Name) const
{
	if (!Name)
		return false;

	BlockLock Lock(m_FilterLock);

	*Name = m_NITInfo.NetworkName;

	return true;
}


uint8_t MMTAnalyzerFilter::GetBroadcastingID() const
{
	BlockLock Lock(m_FilterLock);

	return m_NITInfo.BroadcastingID;
}


uint8_t MMTAnalyzerFilter::GetRemoteControlKeyID(uint16_t ServiceID) const
{
	BlockLock Lock(m_FilterLock);

	auto it = m_NITInfo.RemoteControlKeyMap.find(ServiceID);
	if (it == m_NITInfo.RemoteControlKeyMap.end())
		return 0;

	return it->second;
}


bool MMTAnalyzerFilter::GetSDTServiceList(ReturnArg<SDTServiceList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	*List = m_SDTServiceList;

	return true;
}


bool MMTAnalyzerFilter::GetSDTStreamList(ReturnArg<SDTStreamList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	List->clear();
	List->reserve(m_SDTStreamMap.size());

	for (auto const &e : m_SDTStreamMap)
		List->push_back(e.second);

	return true;
}


bool MMTAnalyzerFilter::GetNetworkStreamList(ReturnArg<NetworkStreamList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	*List = m_NetworkStreamList;

	return true;
}


bool MMTAnalyzerFilter::GetSatelliteDeliverySystemList(ReturnArg<SatelliteDeliverySystemList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	List->clear();

	const TLVNITMultiTable *pNITMultiTable = m_TLVSITableSet->GetNITTable();
	if ((pNITMultiTable == nullptr) || !pNITMultiTable->IsNITComplete())
		return false;

	for (uint16_t SectionNo = 0; SectionNo < pNITMultiTable->GetNITSectionCount(); SectionNo++) {
		const TLVNITTable *pNITTable = pNITMultiTable->GetNITTable(SectionNo);
		if (pNITTable == nullptr)
			continue;

		for (int i = 0; i < pNITTable->GetTLVStreamCount(); i++) {
			const SatelliteDeliverySystemDescriptor *pSatelliteDesc =
				pNITTable->GetItemDescriptorBlock(i)->GetDescriptor<SatelliteDeliverySystemDescriptor>();

			if (pSatelliteDesc != nullptr) {
				SatelliteDeliverySystemInfo &Info = List->emplace_back();

				Info.TLVStreamID = pNITTable->GetTLVStreamID(i);
				Info.Frequency = pSatelliteDesc->GetFrequency();
				Info.OrbitalPosition = pSatelliteDesc->GetOrbitalPosition();
				Info.WestEastFlag = pSatelliteDesc->GetWestEastFlag();
				Info.Polarization = pSatelliteDesc->GetPolarization();
				Info.Modulation = pSatelliteDesc->GetModulation();
				Info.SymbolRate = pSatelliteDesc->GetSymbolRate();
				Info.FECInner = pSatelliteDesc->GetFECInner();
			}
		}
	}

	return !List->empty();
}


bool MMTAnalyzerFilter::GetBITNetworkList(ReturnArg<BITNetworkList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	List->clear();

	const MHBITMultiTable *pBITMultiTable = GetSectionTable<MHBITMultiTable>(MMT_PACKET_ID_MH_BIT);
	if ((pBITMultiTable == nullptr) || !pBITMultiTable->IsBITComplete())
		return false;

	for (uint16_t SectionNo = 0; SectionNo < pBITMultiTable->GetBITSectionCount(); SectionNo++) {
		const MHBITTable *pBITTable = pBITMultiTable->GetBITTable(SectionNo);
		if (pBITTable == nullptr)
			continue;

		BITNetworkInfo &NetworkInfo = List->emplace_back();
		NetworkInfo.OriginalNetworkID = pBITTable->GetOriginalNetworkID();
		NetworkInfo.BroadcasterList.resize(pBITTable->GetBroadcasterCount());

		for (int i = 0; i < pBITTable->GetBroadcasterCount(); i++) {
			BITBroadcasterInfo &BroadcasterInfo = NetworkInfo.BroadcasterList[i];
			const MMTDescriptorBlock *pDescBlock = pBITTable->GetBroadcasterDescriptorBlock(i);

			BroadcasterInfo.BroadcasterID = pBITTable->GetBroadcasterID(i);

			if (const MHBroadcasterNameDescriptor *pNameDesc =
						pDescBlock->GetDescriptor<MHBroadcasterNameDescriptor>();
					pNameDesc != nullptr)
				pNameDesc->GetBroadcasterName(&BroadcasterInfo.BroadcasterName);

			if (const MHServiceListDescriptor *pServiceListDesc =
						pDescBlock->GetDescriptor<MHServiceListDescriptor>();
					pServiceListDesc != nullptr) {
				BroadcasterInfo.ServiceList.resize(pServiceListDesc->GetServiceCount());
				for (int j = 0; j < pServiceListDesc->GetServiceCount(); j++)
					pServiceListDesc->GetServiceInfo(j, &BroadcasterInfo.ServiceList[j]);
			}
		}
	}

	return !List->empty();
}


bool MMTAnalyzerFilter::GetEMMPacketIDList(ReturnArg<EMMPacketIDList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	*List = m_EMMPacketIDList;

	return true;
}


bool MMTAnalyzerFilter::IsPLTUpdated() const
{
	BlockLock Lock(m_FilterLock);

	return m_PLTUpdated;
}


bool MMTAnalyzerFilter::IsMHSDTUpdated() const
{
	BlockLock Lock(m_FilterLock);

	return m_SDTUpdated;
}


bool MMTAnalyzerFilter::IsTLVNITUpdated() const
{
	BlockLock Lock(m_FilterLock);

	return m_NITUpdated;
}


bool MMTAnalyzerFilter::IsMHEITUpdated() const
{
	BlockLock Lock(m_FilterLock);

	return m_EITUpdated;
}


uint16_t MMTAnalyzerFilter::GetEventID(int ServiceIndex, bool Next) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(ServiceIndex) < m_ServiceList.size()) {
		const MHEITTable *pEITTable = GetEITPfTableByServiceID(m_ServiceList[ServiceIndex].ServiceID, Next);
		if (pEITTable != nullptr)
			return pEITTable->GetEventID();
	}

	return EVENT_ID_INVALID;
}


bool MMTAnalyzerFilter::GetEventInfo(int ServiceIndex, ReturnArg<EventInfo> Info, bool Next) const
{
	if (!Info)
		return false;

	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(ServiceIndex) >= m_ServiceList.size())
		return false;

	return GetMHEventInfo(GetEITPfTableByServiceID(m_ServiceList[ServiceIndex].ServiceID, Next), 0, Info);
}


bool MMTAnalyzerFilter::GetEventSeriesInfo(int ServiceIndex, ReturnArg<EventInfo::SeriesInfo> Info, bool Next) const
{
	if (!Info)
		return false;

	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(ServiceIndex) >= m_ServiceList.size())
		return false;

	const MHEITTable *pEITTable = GetEITPfTableByServiceID(m_ServiceList[ServiceIndex].ServiceID, Next);
	if (pEITTable == nullptr)
		return false;

	return GetMHEventSeriesInfo(pEITTable->GetItemDescriptorBlock(), Info);
}


bool MMTAnalyzerFilter::GetTOTTime(ReturnArg<DateTime> Time) const
{
	if (!Time)
		return false;

	BlockLock Lock(m_FilterLock);

	const MHTOTTable *pTOTTable = GetSectionTable<MHTOTTable>(MMT_PACKET_ID_MH_TOT);
	if (pTOTTable == nullptr)
		return false;

	return pTOTTable->GetDateTime(Time);
}


bool MMTAnalyzerFilter::GetInterpolatedTOTTime(ReturnArg<DateTime> Time, OptionalReturnArg<bool> Interpolated) const
{
	BlockLock Lock(m_FilterLock);

	if (!GetTOTTime(Time))
		return false;

	Interpolated = false;

	if ((m_TOTNTPTimestamp != 0) && (m_LastNTPTimestamp >= m_TOTNTPTimestamp)) {
		// NTP タイムスタンプは上位 32 ビットが秒、下位 32 ビットが秒の小数部
		const uint64_t Diff = m_LastNTPTimestamp - m_TOTNTPTimestamp;
		if (Diff <= (15ULL << 32)) { // 最大15秒
			Time->OffsetMilliseconds(static_cast<long long>((Diff * 1000) >> 32));
			Interpolated = true;
		}
	}

	return true;
}


bool MMTAnalyzerFilter::AddEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.AddEventListener(pEventListener);
}


bool MMTAnalyzerFilter::RemoveEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.RemoveEventListener(pEventListener);
}


const MHEITTable * MMTAnalyzerFilter::GetEITPfTableByServiceID(uint16_t ServiceID, bool Next) const
{
	const MHEITPfTable *pEITPfTable = GetSectionTable<MHEITPfTable>(MMT_PACKET_ID_MH_EIT);
	if (pEITPfTable == nullptr)
		return nullptr;

	return pEITPfTable->GetPfTable(ServiceID, Next);
}


void MMTAnalyzerFilter::GetSDTServiceInfo(const MHSDTTable *pSDTTable, int SDTIndex, ReturnArg<SDTServiceInfo> Info) const
{
	Info->ServiceID = pSDTTable->GetServiceID(SDTIndex);
	Info->RunningStatus = pSDTTable->GetRunningStatus(SDTIndex);
	Info->FreeCAMode = pSDTTable->GetFreeCAMode(SDTIndex);
	Info->ProviderName.clear();
	Info->ServiceName.clear();
	Info->ServiceType = SERVICE_TYPE_INVALID;
	Info->LogoID = LOGO_ID_INVALID;

	const MMTDescriptorBlock *pDescBlock = pSDTTable->GetItemDescriptorBlock(SDTIndex);
	if (pDescBlock == nullptr)
		return;

	if (const MHServiceDescriptor *pServiceDesc = pDescBlock->GetDescriptor<MHServiceDescriptor>();
			pServiceDesc != nullptr) {
		pServiceDesc->GetProviderName(&Info->ProviderName);
		pServiceDesc->GetServiceName(&Info->ServiceName);
		Info->ServiceType = pServiceDesc->GetServiceType();
	}

	if (const MHLogoTransmissionDescriptor *pLogoDesc = pDescBlock->GetDescriptor<MHLogoTransmissionDescriptor>();
			pLogoDesc != nullptr)
		Info->LogoID = pLogoDesc->GetLogoID();
}


void MMTAnalyzerFilter::ApplySDTServiceInfo(ServiceInfo *pServiceInfo, const MHSDTTable *pSDTTable, int SDTIndex) const
{
	SDTServiceInfo Info;

	GetSDTServiceInfo(pSDTTable, SDTIndex, &Info);

	pServiceInfo->RunningStatus = Info.RunningStatus;
	pServiceInfo->FreeCAMode = Info.FreeCAMode;
	pServiceInfo->ProviderName = std::move(Info.ProviderName);
	pServiceInfo->ServiceName = std::move(Info.ServiceName);
	pServiceInfo->ServiceType = Info.ServiceType;
	pServiceInfo->LogoID = Info.LogoID;
}


int MMTAnalyzerFilter::GetAssetIndexByComponentTag(const AssetInfoList &List, uint16_t ComponentTag)
{
	for (size_t i = 0; i < List.size(); i++) {
		if (List[i].ComponentTag == ComponentTag)
			return static_cast<int>(i);
	}

	return -1;
}


void MMTAnalyzerFilter::UpdateSDTStreamMap(const MHSDTTable *pSDTTable)
{
	const uint16_t TLVStreamID = pSDTTable->GetTLVStreamID();
	const uint16_t NetworkID = pSDTTable->GetOriginalNetworkID();

	SDTStreamInfo &Info = m_SDTStreamMap[(static_cast<uint32_t>(NetworkID) << 16) | TLVStreamID];

	Info.TLVStreamID = TLVStreamID;
	Info.OriginalNetworkID = NetworkID;
	Info.ServiceList.resize(pSDTTable->GetServiceCount());

	for (int i = 0; i < pSDTTable->GetServiceCount(); i++)
		GetSDTServiceInfo(pSDTTable, i, &Info.ServiceList[i]);
}


void MMTAnalyzerFilter::OnPLTTable(const MMTSingleTable *pTable)
{
	LIBISDB_TRACE(LIBISDB_STR("MMTAnalyzerFilter::OnPLTTable()\n"));

	const PLTable *pPLTable = dynamic_cast<const PLTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pPLTable == nullptr))
		return;

	for (auto const &e : m_ServiceList) {
		if ((e.MPTPacketID != MMT_PACKET_ID_INVALID) && (e.MPTPacketID != MMT_PACKET_ID_PA))
			m_PacketIDMapManager.UnmapTarget(e.MPTPacketID);
	}

	const MHSDTTableSet *pSDTTableSet = GetSectionTable<MHSDTTableSet>(MMT_PACKET_ID_MH_SDT);
	const MHSDTTable *pSDTTable = (pSDTTableSet != nullptr) ? pSDTTableSet->GetActualSDTTable() : nullptr;

	const int ServiceCount = pPLTable->GetPackageCount();
	m_ServiceList.clear();
	m_ServiceList.resize(ServiceCount);

	for (int i = 0; i < ServiceCount; i++) {
		ServiceInfo &Info = m_ServiceList[i];

		Info.IsMPTAcquired = false;
		Info.ServiceID = pPLTable->GetServiceID(i);
		Info.MPTPacketID = pPLTable->GetMPTPacketID(i);
		Info.MPTVersion = 0;
		Info.RunningStatus = 0xFF;
		Info.FreeCAMode = false;
		Info.ServiceType = SERVICE_TYPE_INVALID;
		Info.LogoID = LOGO_ID_INVALID;

		if (pSDTTable != nullptr) {
			const int SDTIndex = pSDTTable->GetServiceIndexByID(Info.ServiceID);
			if (SDTIndex >= 0)
				ApplySDTServiceInfo(&Info, pSDTTable, SDTIndex);
		}

		// 処理中の PLT を破棄しないよう、PLT の packet_id には割り当てない
		if ((Info.MPTPacketID != MMT_PACKET_ID_INVALID)
				&& (Info.MPTPacketID != MMT_PACKET_ID_PA)
				&& (m_PacketIDMapManager.GetMapTarget(Info.MPTPacketID) == nullptr)) {
			m_PacketIDMapManager.MapTarget(
				Info.MPTPacketID,
				MMTSingleTable::CreateWithHandler<MPTable>(&MMTAnalyzerFilter::OnMPTTable, this));
		}
	}

#ifdef LIBISDB_ENABLE_TRACE
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		LIBISDB_TRACE(
			LIBISDB_STR("Service[{:2}] : service_id {:04X} / MPT packet_id {:04X}\n"),
			i, m_ServiceList[i].ServiceID, m_ServiceList[i].MPTPacketID);
	}
#endif

	if (!m_PLTUpdated && m_EITUpdated)
		m_SendEITUpdatedEvent = true;

	m_PLTUpdated = true;

	m_FilterLock.Unlock();
	m_EventListenerList.CallEventListener(&EventListener::OnPLTUpdated, this);
	m_FilterLock.Lock();
}


void MMTAnalyzerFilter::OnMPTTable(const MMTSingleTable *pTable)
{
	const MPTable *pMPTable = dynamic_cast<const MPTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pMPTable == nullptr))
		return;

	const uint16_t ServiceID = pMPTable->GetServiceID();
	const int ServiceIndex = GetServiceIndexByID(ServiceID);
	if (ServiceIndex < 0)
		return;
	ServiceInfo &Info = m_ServiceList[ServiceIndex];

	// MPT はタイムスタンプ記述子が更新される度に内容が変わるため、version が変わった時だけ更新する
	if (Info.IsMPTAcquired && (Info.MPTVersion == pMPTable->GetVersion()))
		return;

	LIBISDB_TRACE(LIBISDB_STR("MMTAnalyzerFilter::OnMPTTable() : service_id {:04X}\n"), ServiceID);

	Info.MPTVersion = pMPTable->GetVersion();
	Info.AssetList.clear();
	Info.VideoAssetList.clear();
	Info.AudioAssetList.clear();
	Info.CaptionAssetList.clear();
	Info.ApplicationAssetList.clear();
	Info.OtherAssetList.clear();
	Info.ECMList.clear();
	Info.ApplicationServiceList.clear();

	for (int i = 0; i < pMPTable->GetAssetCount(); i++) {
		AssetInfo Asset;

		Asset.PacketID = pMPTable->GetAssetPacketID(i);
		Asset.AssetType = pMPTable->GetAssetType(i);
		Asset.ComponentTag = pMPTable->GetAssetComponentTag(i);
		const MHAudioComponentDescriptor *pAudioDesc =
			pMPTable->GetAssetDescriptorBlock(i)->GetDescriptor<MHAudioComponentDescriptor>();
		if (pAudioDesc != nullptr) {
			Asset.StreamType = pAudioDesc->GetStreamType();
			Asset.HasAudioInfo = GetMHAudioComponentInfo(pAudioDesc, &Asset.AudioInfo);
		}

		Info.AssetList.push_back(Asset);

		switch (Asset.AssetType) {
		case MMT_ASSET_TYPE_HEV1:
		case MMT_ASSET_TYPE_HVC1:
			Info.VideoAssetList.push_back(Asset);
			break;

		case MMT_ASSET_TYPE_MP4A:
			Info.AudioAssetList.push_back(Asset);
			break;

		case MMT_ASSET_TYPE_STPP:
			Info.CaptionAssetList.push_back(Asset);
			break;

		case MMT_ASSET_TYPE_AAPP:
			Info.ApplicationAssetList.push_back(Asset);
			break;

		default:
			Info.OtherAssetList.push_back(Asset);
			break;
		}
	}

	const auto ComponentTagCmp =
		[](const AssetInfo &Info1, const AssetInfo &Info2) -> bool {
			return Info1.ComponentTag < Info2.ComponentTag;
		};
	InsertionSort(Info.VideoAssetList, ComponentTagCmp);
	InsertionSort(Info.AudioAssetList, ComponentTagCmp);
	InsertionSort(Info.CaptionAssetList, ComponentTagCmp);
	InsertionSort(Info.ApplicationAssetList, ComponentTagCmp);

	pMPTable->GetMPTDescriptorBlock()->EnumDescriptors<MMTAccessControlDescriptor>(
		[&Info](const MMTAccessControlDescriptor *pAccessControlDesc) {
			ECMInfo &ECM = Info.ECMList.emplace_back();
			ECM.CASystemID = pAccessControlDesc->GetCASystemID();
			ECM.PacketID = pAccessControlDesc->GetPacketID();
		});

	pMPTable->GetMPTDescriptorBlock()->EnumDescriptors<ApplicationServiceDescriptor>(
		[&Info](const ApplicationServiceDescriptor *pAppServiceDesc) {
			ApplicationServiceInfo &App = Info.ApplicationServiceList.emplace_back();
			App.ApplicationFormat = pAppServiceDesc->GetApplicationFormat();
			App.DocumentResolution = pAppServiceDesc->GetDocumentResolution();
			App.DefaultAITFlag = pAppServiceDesc->GetDefaultAITFlag();
			App.AITPacketID = pAppServiceDesc->GetAITPacketID();
			App.DTMessagePacketID = pAppServiceDesc->GetDTMessagePacketID();
			for (int i = 0; i < pAppServiceDesc->GetEMTCount(); i++) {
				EMTInfo &EMT = App.EMTList.emplace_back();
				EMT.EMTTag = pAppServiceDesc->GetEMTInfo(i)->EMTTag;
				EMT.PacketID = pAppServiceDesc->GetEMTPacketID(i);
			}
		});

	Info.IsMPTAcquired = true;

	m_FilterLock.Unlock();
	m_EventListenerList.CallEventListener(&EventListener::OnMPTUpdated, this, ServiceID);
	m_FilterLock.Lock();
}


void MMTAnalyzerFilter::OnMHCATTable(const MMTSingleTable *pTable)
{
	LIBISDB_TRACE(LIBISDB_STR("MMTAnalyzerFilter::OnMHCATTable()\n"));

	const MHCATTable *pCATTable = dynamic_cast<const MHCATTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pCATTable == nullptr))
		return;

	m_EMMPacketIDList.clear();
	pCATTable->GetDescriptorBlock()->EnumDescriptors<MMTAccessControlDescriptor>(
		[this](const MMTAccessControlDescriptor *pAccessControlDesc) {
			const uint16_t PacketID = pAccessControlDesc->GetPacketID();
			if (PacketID != MMT_PACKET_ID_INVALID)
				m_EMMPacketIDList.push_back(PacketID);
		});

	m_FilterLock.Unlock();
	m_EventListenerList.CallEventListener(&EventListener::OnMHCATUpdated, this);
	m_FilterLock.Lock();
}


void MMTAnalyzerFilter::OnMHSDTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	LIBISDB_TRACE(LIBISDB_STR("MMTAnalyzerFilter::OnMHSDTSection()\n"));

	const MHSDTTableSet *pTableSet = dynamic_cast<const MHSDTTableSet *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pTableSet == nullptr))
		return;

	const uint8_t TableID = pTableSet->GetLastUpdatedTableID();

	if (TableID == MHSDTTable::TABLE_ID_ACTUAL) {
		const MHSDTTable *pSDTTable = pTableSet->GetActualSDTTable();
		if (pSDTTable == nullptr)
			return;

		m_TLVStreamID = pSDTTable->GetTLVStreamID();
		m_NetworkID = pSDTTable->GetOriginalNetworkID();

		m_SDTServiceList.resize(pSDTTable->GetServiceCount());
		for (int SDTIndex = 0; SDTIndex < pSDTTable->GetServiceCount(); SDTIndex++) {
			GetSDTServiceInfo(pSDTTable, SDTIndex, &m_SDTServiceList[SDTIndex]);

			const int ServiceIndex = GetServiceIndexByID(pSDTTable->GetServiceID(SDTIndex));
			if (ServiceIndex >= 0)
				ApplySDTServiceInfo(&m_ServiceList[ServiceIndex], pSDTTable, SDTIndex);
		}

		UpdateSDTStreamMap(pSDTTable);

		m_SDTUpdated = true;

		m_FilterLock.Unlock();
		m_EventListenerList.CallEventListener(&EventListener::OnMHSDTUpdated, this);
		m_FilterLock.Lock();
	} else if (TableID == MHSDTTable::TABLE_ID_OTHER) {
		const MHSDTOtherTable *pSDTOtherTable = pTableSet->GetOtherSDTTable();
		if (pSDTOtherTable == nullptr)
			return;

		for (int Table = 0; Table < pSDTOtherTable->GetTableCount(); Table++) {
			if (!pSDTOtherTable->IsSectionComplete(Table))
				continue;

			for (uint16_t Section = 0; Section < pSDTOtherTable->GetSectionCount(Table); Section++) {
				const MHSDTTable *pSDTTable = dynamic_cast<const MHSDTTable *>(pSDTOtherTable->GetSection(Table, Section));
				if (pSDTTable != nullptr)
					UpdateSDTStreamMap(pSDTTable);
			}
		}
	}
}


void MMTAnalyzerFilter::OnMHEITSection(const PSITableBase *pTable, const PSISection *pSection)
{
	LIBISDB_TRACE(LIBISDB_STR("MMTAnalyzerFilter::OnMHEITSection()\n"));

	m_EITUpdated = true;

	// PLT がまだ来ていない場合は番組情報を取得できないため、PLT が来るまで通知しない
	if (m_PLTUpdated) {
		m_FilterLock.Unlock();
		m_EventListenerList.CallEventListener(&EventListener::OnMHEITUpdated, this);
		m_FilterLock.Lock();
	}
}


void MMTAnalyzerFilter::OnMHBITSection(const PSITableBase *pTable, const PSISection *pSection)
{
	LIBISDB_TRACE(LIBISDB_STR("MMTAnalyzerFilter::OnMHBITSection()\n"));

	m_FilterLock.Unlock();
	m_EventListenerList.CallEventListener(&EventListener::OnMHBITUpdated, this);
	m_FilterLock.Lock();
}


void MMTAnalyzerFilter::OnMHTOTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	m_TOTNTPTimestamp = m_LastNTPTimestamp;

	m_FilterLock.Unlock();
	m_EventListenerList.CallEventListener(&EventListener::OnMHTOTUpdated, this);
	m_FilterLock.Lock();
}


void MMTAnalyzerFilter::OnTLVSISection(const PSITableBase *pTable, const PSISection *pSection)
{
	const TLVSITableSet *pTableSet = dynamic_cast<const TLVSITableSet *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pTableSet == nullptr))
		return;

	if (pTableSet->GetLastUpdatedTableID() != TLVNITTable::TABLE_ID)
		return;

	const TLVNITMultiTable *pNITMultiTable = pTableSet->GetNITTable();
	if ((pNITMultiTable == nullptr) || !pNITMultiTable->IsNITComplete())
		return;

	const TLVNITTable *pNITTable = pNITMultiTable->GetNITTable(0);
	if (pNITTable == nullptr)
		return;

	LIBISDB_TRACE(LIBISDB_STR("MMTAnalyzerFilter::OnTLVSISection() : TLV-NIT network_id {:04X}\n"), pNITTable->GetNetworkID());

	if (m_NetworkID == NETWORK_ID_INVALID)
		m_NetworkID = pNITTable->GetNetworkID();

	m_NITInfo.Reset();
	pNITTable->GetNetworkName(&m_NITInfo.NetworkName);

	const DescriptorBlock *pNetworkDescBlock = pNITTable->GetNetworkDescriptorBlock();
	if (const SystemManagementDescriptor *pSysManageDesc =
				pNetworkDescBlock->GetDescriptor<SystemManagementDescriptor>();
			pSysManageDesc != nullptr) {
		m_NITInfo.BroadcastingFlag = pSysManageDesc->GetBroadcastingFlag();
		m_NITInfo.BroadcastingID = pSysManageDesc->GetBroadcastingID();
	}
	pNetworkDescBlock->EnumDescriptors<RemoteControlKeyDescriptor>(
		[this](const RemoteControlKeyDescriptor *pKeyDesc) {
			for (int i = 0; i < pKeyDesc->GetKeyCount(); i++) {
				RemoteControlKeyDescriptor::KeyInfo Key;
				if (pKeyDesc->GetKeyInfo(i, &Key))
					m_NITInfo.RemoteControlKeyMap[Key.ServiceID] = Key.RemoteControlKeyID;
			}
		});

	m_NetworkStreamList.clear();

	for (uint16_t SectionNo = 0; SectionNo < pNITMultiTable->GetNITSectionCount(); SectionNo++) {
		pNITTable = pNITMultiTable->GetNITTable(SectionNo);
		if (pNITTable == nullptr)
			break;

		for (int i = 0; i < pNITTable->GetTLVStreamCount(); i++) {
			NetworkStreamInfo &StreamInfo = m_NetworkStreamList.emplace_back();

			StreamInfo.TLVStreamID = pNITTable->GetTLVStreamID(i);
			StreamInfo.OriginalNetworkID = pNITTable->GetOriginalNetworkID(i);

			const ServiceListDescriptor *pServiceListDesc =
				pNITTable->GetItemDescriptorBlock(i)->GetDescriptor<ServiceListDescriptor>();
			if (pServiceListDesc != nullptr) {
				for (int j = 0; j < pServiceListDesc->GetServiceCount(); j++) {
					ServiceListDescriptor::ServiceInfo Info;

					if (pServiceListDesc->GetServiceInfo(j, &Info)) {
						StreamInfo.ServiceList.push_back(Info);

						const int Index = GetServiceIndexByID(Info.ServiceID);
						if ((Index >= 0) && (m_ServiceList[Index].ServiceType == SERVICE_TYPE_INVALID))
							m_ServiceList[Index].ServiceType = Info.ServiceType;
					}
				}
			}
		}
	}

	m_NITUpdated = true;

	m_FilterLock.Unlock();
	m_EventListenerList.CallEventListener(&EventListener::OnTLVNITUpdated, this);
	m_FilterLock.Lock();
}


} // namespace LibISDB
