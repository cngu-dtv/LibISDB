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
 @file   MMTTables.cpp
 @brief  MMT-SI テーブル
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "MMTTables.hpp"
#include "../Base/ARIBTime.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


namespace
{

uint64_t LoadPackageID(const uint8_t *pData, size_t Length)
{
	uint64_t ID = 0;
	for (size_t i = (Length > 8) ? (Length - 8) : 0; i < Length; i++)
		ID = (ID << 8) | pData[i];
	return ID;
}

}




PLTable::PLTable()
	: MMTSingleTable(TABLE_ID)
{
}


void PLTable::Reset()
{
	MMTSingleTable::Reset();

	m_PackageList.clear();
}


int PLTable::GetPackageCount() const
{
	return static_cast<int>(m_PackageList.size());
}


const PLTable::PackageInfo * PLTable::GetPackageInfo(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_PackageList.size())
		return nullptr;
	return &m_PackageList[Index];
}


uint16_t PLTable::GetServiceID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_PackageList.size())
		return SERVICE_ID_INVALID;
	return static_cast<uint16_t>(m_PackageList[Index].PackageID & 0xFFFF);
}


uint16_t PLTable::GetMPTPacketID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_PackageList.size())
		return MMT_PACKET_ID_INVALID;

	const MMTGeneralLocationInfo &Location = m_PackageList[Index].LocationInfo;
	if (Location.LocationType != MMTGeneralLocationInfo::LOCATION_TYPE_PACKET_ID)
		return MMT_PACKET_ID_INVALID;

	return Location.PacketID;
}


bool PLTable::OnTableUpdate(const MMTTable *pCurTable, const MMTTable *pOldTable)
{
	const uint8_t *pData = pCurTable->GetPayloadData();
	const size_t DataSize = pCurTable->GetPayloadSize();

	if (DataSize < 1)
		return false;

	std::vector<PackageInfo> PackageList(pData[0]);
	size_t Pos = 1;

	for (PackageInfo &Package : PackageList) {
		if (Pos + 1 > DataSize)
			return false;
		const size_t IDLength = pData[Pos++];
		if (Pos + IDLength > DataSize)
			return false;
		Package.PackageID = LoadPackageID(&pData[Pos], IDLength);
		Pos += IDLength;

		const size_t LocationSize = Package.LocationInfo.Parse(&pData[Pos], DataSize - Pos);
		if (LocationSize == 0)
			return false;
		Pos += LocationSize;
	}

	m_PackageList = std::move(PackageList);

	return true;
}




MPTable::MPTable()
	: MMTSingleTable(TABLE_ID)
	, m_MPTMode(0)
	, m_PackageID(0)
{
}


void MPTable::Reset()
{
	MMTSingleTable::Reset();

	m_MPTMode = 0;
	m_PackageID = 0;
	m_DescriptorBlock.Reset();
	m_AssetList.clear();
}


int MPTable::GetAssetCount() const
{
	return static_cast<int>(m_AssetList.size());
}


const MPTable::AssetInfo * MPTable::GetAssetInfo(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_AssetList.size())
		return nullptr;
	return &m_AssetList[Index];
}


uint32_t MPTable::GetAssetType(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_AssetList.size())
		return 0;
	return m_AssetList[Index].AssetType;
}


uint16_t MPTable::GetAssetPacketID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_AssetList.size())
		return MMT_PACKET_ID_INVALID;

	for (const MMTGeneralLocationInfo &Location : m_AssetList[Index].LocationList) {
		if (Location.LocationType == MMTGeneralLocationInfo::LOCATION_TYPE_PACKET_ID)
			return Location.PacketID;
	}

	return MMT_PACKET_ID_INVALID;
}


uint16_t MPTable::GetAssetComponentTag(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_AssetList.size())
		return MMT_COMPONENT_TAG_INVALID;

	const MHStreamIdentifierDescriptor *pStreamID =
		m_AssetList[Index].Descriptors.GetDescriptor<MHStreamIdentifierDescriptor>();
	if (pStreamID == nullptr)
		return MMT_COMPONENT_TAG_INVALID;

	return pStreamID->GetComponentTag();
}


const MMTDescriptorBlock * MPTable::GetAssetDescriptorBlock(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_AssetList.size())
		return nullptr;
	return &m_AssetList[Index].Descriptors;
}


bool MPTable::OnTableUpdate(const MMTTable *pCurTable, const MMTTable *pOldTable)
{
	const uint8_t *pData = pCurTable->GetPayloadData();
	const size_t DataSize = pCurTable->GetPayloadSize();

	if (DataSize < 2)
		return false;

	const uint8_t MPTMode = pData[0] & 0x03;
	const size_t PackageIDLength = pData[1];
	size_t Pos = 2;
	if (Pos + PackageIDLength + 2 > DataSize)
		return false;
	const uint64_t PackageID = LoadPackageID(&pData[Pos], PackageIDLength);
	Pos += PackageIDLength;

	const size_t DescriptorsLength = Load16(&pData[Pos]);
	Pos += 2;
	if (Pos + DescriptorsLength + 1 > DataSize)
		return false;
	MMTDescriptorBlock DescriptorBlock;
	DescriptorBlock.ParseBlock(&pData[Pos], DescriptorsLength);
	Pos += DescriptorsLength;

	std::vector<AssetInfo> AssetList(pData[Pos++]);

	for (AssetInfo &Asset : AssetList) {
		if (Pos + 6 > DataSize)
			return false;
		Asset.IdentifierType = pData[Pos];
		Asset.AssetIDScheme = Load32(&pData[Pos + 1]);
		const size_t AssetIDLength = pData[Pos + 5];
		Pos += 6;
		if (Pos + AssetIDLength + 5 > DataSize)
			return false;
		Asset.AssetID.assign(&pData[Pos], AssetIDLength);
		Pos += AssetIDLength;

		Asset.AssetType = Load32(&pData[Pos]);
		Asset.AssetClockRelationFlag = (pData[Pos + 4] & 0x01) != 0;
		Pos += 5;
		Asset.AssetClockRelationID = 0;
		Asset.AssetTimescaleFlag = false;
		Asset.AssetTimescale = 0;
		if (Asset.AssetClockRelationFlag) {
			if (Pos + 2 > DataSize)
				return false;
			Asset.AssetClockRelationID = pData[Pos];
			Asset.AssetTimescaleFlag = (pData[Pos + 1] & 0x01) != 0;
			Pos += 2;
			if (Asset.AssetTimescaleFlag) {
				if (Pos + 4 > DataSize)
					return false;
				Asset.AssetTimescale = Load32(&pData[Pos]);
				Pos += 4;
			}
		}

		if (Pos + 1 > DataSize)
			return false;
		Asset.LocationList.resize(pData[Pos++]);
		for (MMTGeneralLocationInfo &Location : Asset.LocationList) {
			const size_t LocationSize = Location.Parse(&pData[Pos], DataSize - Pos);
			if (LocationSize == 0)
				return false;
			Pos += LocationSize;
		}

		if (Pos + 2 > DataSize)
			return false;
		const size_t AssetDescriptorsLength = Load16(&pData[Pos]);
		Pos += 2;
		if (Pos + AssetDescriptorsLength > DataSize)
			return false;
		Asset.Descriptors.ParseBlock(&pData[Pos], AssetDescriptorsLength);
		Pos += AssetDescriptorsLength;
	}

	m_MPTMode = MPTMode;
	m_PackageID = PackageID;
	m_DescriptorBlock = std::move(DescriptorBlock);
	m_AssetList = std::move(AssetList);

	return true;
}




MHCATTable::MHCATTable()
	: MMTSingleTable(TABLE_ID)
{
}


void MHCATTable::Reset()
{
	MMTSingleTable::Reset();

	m_DescriptorBlock.Reset();
}


bool MHCATTable::OnTableUpdate(const MMTTable *pCurTable, const MMTTable *pOldTable)
{
	m_DescriptorBlock.ParseBlock(pCurTable->GetPayloadData(), pCurTable->GetPayloadSize());

	return true;
}




MHSDTTable::MHSDTTable(uint8_t TableID)
	: m_TableID(TableID)
{
	Reset();
}


void MHSDTTable::Reset()
{
	PSISingleTable::Reset();

	m_OriginalNetworkID = NETWORK_ID_INVALID;
	m_ServiceList.clear();
}


uint16_t MHSDTTable::GetTLVStreamID() const
{
	return m_CurSection.GetTableIDExtension();
}


int MHSDTTable::GetServiceCount() const
{
	return static_cast<int>(m_ServiceList.size());
}


int MHSDTTable::GetServiceIndexByID(uint16_t ServiceID) const
{
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i].ServiceID == ServiceID)
			return static_cast<int>(i);
	}

	return -1;
}


uint16_t MHSDTTable::GetServiceID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return SERVICE_ID_INVALID;
	return m_ServiceList[Index].ServiceID;
}


uint8_t MHSDTTable::GetEITUserDefinedFlags(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return 0;
	return m_ServiceList[Index].EITUserDefinedFlags;
}


bool MHSDTTable::GetEITScheduleFlag(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;
	return m_ServiceList[Index].EITScheduleFlag;
}


bool MHSDTTable::GetEITPresentFollowingFlag(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;
	return m_ServiceList[Index].EITPresentFollowingFlag;
}


uint8_t MHSDTTable::GetRunningStatus(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return 0xFF_u8;
	return m_ServiceList[Index].RunningStatus;
}


bool MHSDTTable::GetFreeCAMode(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;
	return m_ServiceList[Index].FreeCAMode;
}


const MMTDescriptorBlock * MHSDTTable::GetItemDescriptorBlock(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return nullptr;
	return &m_ServiceList[Index].Descriptors;
}


bool MHSDTTable::OnPSISection(const PSISectionParser *pSectionParser, const PSISection *pSection)
{
	if (pSection->GetTableID() != m_TableID)
		return false;
	return PSISingleTable::OnPSISection(pSectionParser, pSection);
}


bool MHSDTTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 3)
		return false;
	if (pCurSection->GetTableID() != m_TableID)
		return false;

	m_OriginalNetworkID = Load16(&pData[0]);

	m_ServiceList.clear();

	for (size_t Pos = 3; Pos + 5 <= DataSize;) {
		ServiceInfo &Info = m_ServiceList.emplace_back();

		Info.ServiceID               = Load16(&pData[Pos + 0]);
		Info.EITUserDefinedFlags     = (pData[Pos + 2] >> 2) & 0x07;
		Info.EITScheduleFlag         = (pData[Pos + 2] & 0x02) != 0;
		Info.EITPresentFollowingFlag = (pData[Pos + 2] & 0x01) != 0;
		Info.RunningStatus           = pData[Pos + 3] >> 5;
		Info.FreeCAMode              = (pData[Pos + 3] & 0x10) != 0;

		const size_t DescriptorsLength = ((pData[Pos + 3] & 0x0F) << 8) | pData[Pos + 4];
		Pos += 5;
		if (Pos + DescriptorsLength > DataSize)
			break;

		Info.Descriptors.ParseBlock(&pData[Pos], DescriptorsLength);
		Pos += DescriptorsLength;
	}

	return true;
}




PSITableBase * MHSDTOtherTable::CreateSectionTable(const PSISection *pSection)
{
	return new MHSDTTable(MHSDTTable::TABLE_ID_OTHER);
}




MHSDTTableSet::MHSDTTableSet()
{
	MapTable(MHSDTTable::TABLE_ID_ACTUAL, new MHSDTTable(MHSDTTable::TABLE_ID_ACTUAL));
	MapTable(MHSDTTable::TABLE_ID_OTHER, new MHSDTOtherTable);
}


const MHSDTTable * MHSDTTableSet::GetActualSDTTable() const
{
	return dynamic_cast<const MHSDTTable *>(GetTableByID(MHSDTTable::TABLE_ID_ACTUAL));
}


const MHSDTOtherTable * MHSDTTableSet::GetOtherSDTTable() const
{
	return dynamic_cast<const MHSDTOtherTable *>(GetTableByID(MHSDTTable::TABLE_ID_OTHER));
}




MHEITTable::MHEITTable()
{
	Reset();
}


void MHEITTable::Reset()
{
	PSISingleTable::Reset();

	m_ServiceID = SERVICE_ID_INVALID;
	m_TLVStreamID = TRANSPORT_STREAM_ID_INVALID;
	m_OriginalNetworkID = NETWORK_ID_INVALID;
	m_SegmentLastSectionNumber = 0;
	m_LastTableID = 0;
	m_EventList.clear();
}


int MHEITTable::GetEventCount() const
{
	return static_cast<int>(m_EventList.size());
}


const MHEITTable::EventInfo * MHEITTable::GetEventInfo(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_EventList.size())
		return nullptr;
	return &m_EventList[Index];
}


uint16_t MHEITTable::GetEventID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_EventList.size())
		return EVENT_ID_INVALID;
	return m_EventList[Index].EventID;
}


const DateTime * MHEITTable::GetStartTime(int Index) const
{
	if ((static_cast<unsigned int>(Index) >= m_EventList.size())
			|| !m_EventList[Index].StartTime.IsValid())
		return nullptr;
	return &m_EventList[Index].StartTime;
}


uint32_t MHEITTable::GetDuration(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_EventList.size())
		return 0;
	return m_EventList[Index].Duration;
}


uint8_t MHEITTable::GetRunningStatus(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_EventList.size())
		return 0;
	return m_EventList[Index].RunningStatus;
}


bool MHEITTable::GetFreeCAMode(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_EventList.size())
		return false;
	return m_EventList[Index].FreeCAMode;
}


const MMTDescriptorBlock * MHEITTable::GetItemDescriptorBlock(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_EventList.size())
		return nullptr;
	return &m_EventList[Index].Descriptors;
}


bool MHEITTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	const uint8_t TableID = pCurSection->GetTableID();
	if ((TableID < MMT_TABLE_ID_MH_EIT_PF) || (TableID > MMT_TABLE_ID_MH_EIT_SCHEDULE_LAST))
		return false;

	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 6)
		return false;

	m_ServiceID                = pCurSection->GetTableIDExtension();
	m_TLVStreamID              = Load16(&pData[0]);
	m_OriginalNetworkID        = Load16(&pData[2]);
	m_SegmentLastSectionNumber = pData[4];
	m_LastTableID              = pData[5];

	m_EventList.clear();

	for (size_t Pos = 6; Pos + 12 <= DataSize;) {
		EventInfo &Info = m_EventList.emplace_back();

		Info.EventID       = Load16(&pData[Pos + 0]);
		MJDBCDTimeToDateTime(&pData[Pos + 2], &Info.StartTime);
		Info.Duration      = BCDTimeToSecond(&pData[Pos + 7]);
		Info.RunningStatus = pData[Pos + 10] >> 5;
		Info.FreeCAMode    = (pData[Pos + 10] & 0x10) != 0;

		const size_t DescriptorsLength = ((pData[Pos + 10] & 0x0F) << 8) | pData[Pos + 11];
		Pos += 12;
		if (Pos + DescriptorsLength > DataSize)
			break;

		Info.Descriptors.ParseBlock(&pData[Pos], DescriptorsLength);
		Pos += DescriptorsLength;
	}

	return true;
}




const MHEITTable * MHEITMultiTable::GetEITTableByServiceID(uint16_t ServiceID, uint16_t SectionNumber) const
{
	for (auto &Table : m_TableList) {
		if (static_cast<uint16_t>(Table.UniqueID & 0xFFFF) == ServiceID) {
			if (SectionNumber > Table.LastSectionNumber)
				return nullptr;
			return dynamic_cast<const MHEITTable *>(Table.SectionList[SectionNumber].Table.get());
		}
	}

	return nullptr;
}


uint16_t MHEITMultiTable::GetServiceID(int Index) const
{
	unsigned long long UniqueID;

	if (!GetTableUniqueID(Index, &UniqueID))
		return SERVICE_ID_INVALID;

	return static_cast<uint16_t>(UniqueID & 0xFFFF);
}


unsigned long long MHEITMultiTable::MakeTableUniqueID(
	uint16_t NetworkID, uint16_t TLVStreamID, uint16_t ServiceID)
{
	return (static_cast<unsigned long long>(NetworkID) << 32) |
	       (static_cast<unsigned long long>(TLVStreamID) << 16) |
	       ServiceID;
}


PSITableBase * MHEITMultiTable::CreateSectionTable(const PSISection *pSection)
{
	return new MHEITTable;
}


unsigned long long MHEITMultiTable::GetSectionTableUniqueID(const PSISection *pSection)
{
	const uint16_t DataSize = pSection->GetPayloadSize();

	if (DataSize < 6)
		return pSection->GetTableIDExtension();

	const uint8_t *pData = pSection->GetPayloadData();

	return MakeTableUniqueID(Load16(&pData[2]), Load16(&pData[0]), pSection->GetTableIDExtension());
}




MHEITPfTable::MHEITPfTable()
{
	MapTable(MMT_TABLE_ID_MH_EIT_PF, new MHEITMultiTable);
}


const MHEITMultiTable * MHEITPfTable::GetPfTable() const
{
	return dynamic_cast<const MHEITMultiTable *>(GetTableByID(MMT_TABLE_ID_MH_EIT_PF));
}


const MHEITTable * MHEITPfTable::GetPfTable(uint16_t ServiceID, bool Following) const
{
	const MHEITMultiTable *pTable = GetPfTable();
	if (pTable == nullptr)
		return nullptr;
	return pTable->GetEITTableByServiceID(ServiceID, Following ? 1 : 0);
}




MHEITPfScheduleTable::MHEITPfScheduleTable()
{
	MapTable(MMT_TABLE_ID_MH_EIT_PF, new MHEITMultiTable);
	for (uint8_t TableID = MMT_TABLE_ID_MH_EIT_SCHEDULE_FIRST; TableID <= MMT_TABLE_ID_MH_EIT_SCHEDULE_LAST; TableID++)
		MapTable(TableID, new MHEITMultiTable);
}


const MHEITTable * MHEITPfScheduleTable::GetLastUpdatedEITTable() const
{
	const MHEITMultiTable *pTable = dynamic_cast<const MHEITMultiTable *>(GetLastUpdatedTable());
	if (pTable == nullptr)
		return nullptr;

	const int Index = pTable->GetTableIndexByUniqueID(m_LastUpdatedTableUniqueID);
	if (Index < 0)
		return nullptr;

	return dynamic_cast<const MHEITTable *>(pTable->GetSection(Index, m_LastUpdatedSectionNumber));
}


bool MHEITPfScheduleTable::ResetScheduleService(
	uint16_t NetworkID, uint16_t TLVStreamID, uint16_t ServiceID)
{
	bool ResetPerformed = false;

	for (auto &Map : m_TableMap) {
		MHEITMultiTable *pEITMultiTable = dynamic_cast<MHEITMultiTable *>(Map.second.get());

		if (pEITMultiTable != nullptr) {
			const int Index = pEITMultiTable->GetTableIndexByUniqueID(
				MHEITMultiTable::MakeTableUniqueID(NetworkID, TLVStreamID, ServiceID));

			if (Index >= 0) {
				if (pEITMultiTable->ResetTable(Index))
					ResetPerformed = true;
			}
		}
	}

	return ResetPerformed;
}




MHCDTTable::MHCDTTable()
{
	Reset();
}


void MHCDTTable::Reset()
{
	PSISingleTable::Reset();

	m_OriginalNetworkID = NETWORK_ID_INVALID;
	m_DataType = DATA_TYPE_INVALID;
	m_DescriptorBlock.Reset();
	m_DataModuleOffset = 0;
}


uint16_t MHCDTTable::GetDataModuleSize() const noexcept
{
	const uint16_t PayloadSize = m_CurSection.GetPayloadSize();
	if ((m_DataModuleOffset == 0) || (PayloadSize <= m_DataModuleOffset))
		return 0;
	return PayloadSize - m_DataModuleOffset;
}


const uint8_t * MHCDTTable::GetDataModuleData() const noexcept
{
	if (GetDataModuleSize() == 0)
		return nullptr;
	return m_CurSection.GetPayloadData() + m_DataModuleOffset;
}


bool MHCDTTable::GetLogoModule(ReturnArg<LogoModuleInfo> Info) const
{
	if (!Info)
		return false;
	if (m_DataType != DATA_TYPE_LOGO)
		return false;

	const uint16_t ModuleSize = GetDataModuleSize();
	const uint8_t *pModule = GetDataModuleData();

	if ((pModule == nullptr) || (ModuleSize < 7))
		return false;

	const uint16_t DataSize = Load16(&pModule[5]);
	if (DataSize > ModuleSize - 7)
		return false;

	Info->LogoType    = pModule[0];
	Info->LogoID      = Load16(&pModule[1]) & 0x01FF_u16;
	Info->LogoVersion = Load16(&pModule[3]) & 0x0FFF_u16;
	Info->DataSize    = DataSize;
	Info->pData       = &pModule[7];

	return true;
}


bool MHCDTTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 5)
		return false;

	const uint16_t DescriptorsLength = Load16(&pData[3]) & 0x0FFF_u16;
	if (5 + DescriptorsLength > DataSize)
		return false;

	m_OriginalNetworkID = Load16(&pData[0]);
	m_DataType          = pData[2];
	m_DescriptorBlock.ParseBlock(&pData[5], DescriptorsLength);
	m_DataModuleOffset  = 5 + DescriptorsLength;

	return true;
}




int MHCDTMultiTable::GetDownloadDataIndex(uint16_t DownloadDataID) const
{
	return GetTableIndexByUniqueID(DownloadDataID);
}


const MHCDTTable * MHCDTMultiTable::GetCDTTable(int Index, uint16_t SectionNumber) const
{
	const MHCDTTable *pTable = dynamic_cast<const MHCDTTable *>(GetSection(Index, SectionNumber));
	if ((pTable == nullptr) || (pTable->GetDataType() == MHCDTTable::DATA_TYPE_INVALID))
		return nullptr;
	return pTable;
}


bool MHCDTMultiTable::OnPSISection(const PSISectionParser *pSectionParser, const PSISection *pSection)
{
	if (pSection->GetTableID() != MHCDTTable::TABLE_ID)
		return false;
	return PSITable::OnPSISection(pSectionParser, pSection);
}


PSITableBase * MHCDTMultiTable::CreateSectionTable(const PSISection *pSection)
{
	return new MHCDTTable;
}




MHBITTable::MHBITTable()
{
	Reset();
}


void MHBITTable::Reset()
{
	PSISingleTable::Reset();

	m_OriginalNetworkID = NETWORK_ID_INVALID;
	m_BroadcastViewPropriety = false;
	m_DescriptorBlock.Reset();
	m_BroadcasterList.clear();
}


int MHBITTable::GetBroadcasterCount() const
{
	return static_cast<int>(m_BroadcasterList.size());
}


uint8_t MHBITTable::GetBroadcasterID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_BroadcasterList.size())
		return 0;
	return m_BroadcasterList[Index].BroadcasterID;
}


const MMTDescriptorBlock * MHBITTable::GetBroadcasterDescriptorBlock(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_BroadcasterList.size())
		return nullptr;
	return &m_BroadcasterList[Index].Descriptors;
}


bool MHBITTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 2)
		return false;

	m_OriginalNetworkID      = pCurSection->GetTableIDExtension();
	m_BroadcastViewPropriety = (pData[0] & 0x10) != 0;

	const size_t DescriptorsLength = ((pData[0] & 0x0F) << 8) | pData[1];
	if (2 + DescriptorsLength > DataSize)
		return false;
	m_DescriptorBlock.ParseBlock(&pData[2], DescriptorsLength);

	m_BroadcasterList.clear();

	for (size_t Pos = 2 + DescriptorsLength; Pos + 3 <= DataSize;) {
		const size_t Length = ((pData[Pos + 1] & 0x0F) << 8) | pData[Pos + 2];
		if (Pos + 3 + Length > DataSize)
			break;

		BroadcasterInfo &Info = m_BroadcasterList.emplace_back();
		Info.BroadcasterID = pData[Pos];
		Info.Descriptors.ParseBlock(&pData[Pos + 3], Length);
		Pos += 3 + Length;
	}

	return true;
}




uint16_t MHBITMultiTable::GetBITSectionCount() const
{
	return GetSectionCount(0);
}


const MHBITTable * MHBITMultiTable::GetBITTable(uint16_t SectionNumber) const
{
	return dynamic_cast<const MHBITTable *>(GetSection(0, SectionNumber));
}


bool MHBITMultiTable::IsBITComplete() const
{
	return IsSectionComplete(0);
}


PSITableBase * MHBITMultiTable::CreateSectionTable(const PSISection *pSection)
{
	return new MHBITTable;
}




MHTOTTable::MHTOTTable()
	: PSISingleTable(false)
{
}


void MHTOTTable::Reset()
{
	PSISingleTable::Reset();

	m_DateTime.Reset();
	m_DescriptorBlock.Reset();
}


bool MHTOTTable::GetDateTime(ReturnArg<DateTime> Time) const
{
	if (!Time)
		return false;

	*Time = m_DateTime;

	return Time->IsValid();
}


bool MHTOTTable::GetOffsetDateTime(ReturnArg<DateTime> Time, uint32_t CountryCode, uint8_t CountryRegionID) const
{
	if (!Time)
		return false;

	*Time = m_DateTime;

	if (!Time->IsValid())
		return false;

	const int Offset = GetLocalTimeOffset(CountryCode, CountryRegionID);
	if (Offset != 0) {
		if (!Time->OffsetSeconds(Offset * 60))
			return false;
	}

	return true;
}


int MHTOTTable::GetLocalTimeOffset(uint32_t CountryCode, uint8_t CountryRegionID) const
{
	const MHLocalTimeOffsetDescriptor *pLocalTimeOffset =
		m_DescriptorBlock.GetDescriptor<MHLocalTimeOffsetDescriptor>();

	if (pLocalTimeOffset == nullptr)
		return 0;

	for (int i = 0; i < pLocalTimeOffset->GetTimeOffsetInfoCount(); i++) {
		MHLocalTimeOffsetDescriptor::TimeOffsetInfo Info;

		if (pLocalTimeOffset->GetTimeOffsetInfo(i, &Info)
				&& (Info.CountryCode == CountryCode)
				&& (Info.CountryRegionID == CountryRegionID)) {
			return Info.LocalTimeOffsetPolarity ? -static_cast<int>(Info.LocalTimeOffset) : Info.LocalTimeOffset;
		}
	}

	return 0;
}


bool MHTOTTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 7)
		return false;
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	MJDBCDTimeToDateTime(pData, &m_DateTime);

	const size_t DescriptorsLength = Load16(&pData[5]) & 0x0FFF;
	if (DescriptorsLength <= DataSize - 7u)
		m_DescriptorBlock.ParseBlock(&pData[7], DescriptorsLength);
	else
		m_DescriptorBlock.Reset();

	return true;
}


} // namespace LibISDB
