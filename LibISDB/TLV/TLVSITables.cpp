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
 @file   TLVSITables.cpp
 @brief  TLV-SI テーブル
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "TLVSITables.hpp"
#include "../MMT/MMTString.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Utilities/CRC.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


TLVNITTable::TLVNITTable()
{
	Reset();
}


void TLVNITTable::Reset()
{
	PSISingleTable::Reset();

	m_NetworkID = NETWORK_ID_INVALID;
	m_NetworkDescriptorBlock.Reset();
	m_TLVStreamList.clear();
}


bool TLVNITTable::GetNetworkName(ReturnArg<String> Name) const
{
	if (!Name)
		return false;

	const NetworkNameDescriptor *pNetworkName = m_NetworkDescriptorBlock.GetDescriptor<NetworkNameDescriptor>();
	if (pNetworkName == nullptr)
		return false;

	ARIBString NameData;
	if (!pNetworkName->GetNetworkName(&NameData))
		return false;

	return DecodeMMTString(NameData, Name) && !Name->empty();
}


int TLVNITTable::GetTLVStreamCount() const
{
	return static_cast<int>(m_TLVStreamList.size());
}


uint16_t TLVNITTable::GetTLVStreamID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_TLVStreamList.size())
		return TRANSPORT_STREAM_ID_INVALID;
	return m_TLVStreamList[Index].TLVStreamID;
}


uint16_t TLVNITTable::GetOriginalNetworkID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_TLVStreamList.size())
		return NETWORK_ID_INVALID;
	return m_TLVStreamList[Index].OriginalNetworkID;
}


const DescriptorBlock * TLVNITTable::GetItemDescriptorBlock(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_TLVStreamList.size())
		return nullptr;
	return &m_TLVStreamList[Index].Descriptors;
}


bool TLVNITTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 4)
		return false;
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	const size_t NetworkDescriptorsLength = Load16(&pData[0]) & 0x0FFF;
	size_t Pos = 2;
	if (Pos + NetworkDescriptorsLength + 2 > DataSize)
		return false;
	m_NetworkDescriptorBlock.ParseBlock(&pData[Pos], NetworkDescriptorsLength);
	Pos += NetworkDescriptorsLength;

	const size_t StreamLoopLength = Load16(&pData[Pos]) & 0x0FFF;
	Pos += 2;
	if (Pos + StreamLoopLength > DataSize)
		return false;

	m_NetworkID = pCurSection->GetTableIDExtension();
	m_TLVStreamList.clear();

	for (const size_t End = Pos + StreamLoopLength; Pos + 6 <= End;) {
		const size_t DescriptorsLength = Load16(&pData[Pos + 4]) & 0x0FFF;
		if (Pos + 6 + DescriptorsLength > End)
			break;

		TLVStreamInfo &Info = m_TLVStreamList.emplace_back();
		Info.TLVStreamID       = Load16(&pData[Pos + 0]);
		Info.OriginalNetworkID = Load16(&pData[Pos + 2]);
		Info.Descriptors.ParseBlock(&pData[Pos + 6], DescriptorsLength);
		Pos += 6 + DescriptorsLength;
	}

	return true;
}




uint16_t TLVNITMultiTable::GetNITSectionCount() const
{
	return GetSectionCount(0);
}


const TLVNITTable * TLVNITMultiTable::GetNITTable(uint16_t SectionNumber) const
{
	return dynamic_cast<const TLVNITTable *>(GetSection(0, SectionNumber));
}


bool TLVNITMultiTable::IsNITComplete() const
{
	return IsSectionComplete(0);
}


PSITableBase * TLVNITMultiTable::CreateSectionTable(const PSISection *pSection)
{
	return new TLVNITTable;
}




AMTTable::AMTTable()
{
	Reset();
}


void AMTTable::Reset()
{
	PSISingleTable::Reset();

	m_ServiceList.clear();
}


int AMTTable::GetServiceCount() const
{
	return static_cast<int>(m_ServiceList.size());
}


const AMTTable::ServiceInfo * AMTTable::GetServiceInfo(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return nullptr;
	return &m_ServiceList[Index];
}


bool AMTTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 2)
		return false;
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	const size_t ServiceCount = Load16(&pData[0]) >> 6;
	std::vector<ServiceInfo> ServiceList;
	ServiceList.reserve(ServiceCount);

	for (size_t i = 0, Pos = 2; i < ServiceCount; i++) {
		if (Pos + 4 > DataSize)
			return false;
		const bool IsIPv6 = (pData[Pos + 2] & 0x80) != 0;
		const size_t LoopLength = Load16(&pData[Pos + 2]) & 0x03FF;
		const size_t AddressSize = IsIPv6 ? 16 : 4;
		if ((LoopLength < (AddressSize + 1) * 2) || (Pos + 4 + LoopLength > DataSize))
			return false;

		ServiceInfo &Info = ServiceList.emplace_back();
		const uint8_t *p = &pData[Pos + 4];
		Info.ServiceID = Load16(&pData[Pos]);
		Info.IsIPv6 = IsIPv6;
		Info.SourceAddress.fill(0);
		std::memcpy(Info.SourceAddress.data(), &p[0], AddressSize);
		Info.SourceAddressMask = p[AddressSize];
		Info.DestinationAddress.fill(0);
		std::memcpy(Info.DestinationAddress.data(), &p[AddressSize + 1], AddressSize);
		Info.DestinationAddressMask = p[AddressSize * 2 + 1];

		Pos += 4 + LoopLength;
	}

	m_ServiceList = std::move(ServiceList);

	return true;
}




TLVSITableSet::TLVSITableSet()
{
	MapTable(TLVNITTable::TABLE_ID, new TLVNITMultiTable);
	MapTable(AMTTable::TABLE_ID, new AMTTable);
}


const TLVNITMultiTable * TLVSITableSet::GetNITTable() const
{
	return dynamic_cast<const TLVNITMultiTable *>(GetTableByID(TLVNITTable::TABLE_ID));
}


const AMTTable * TLVSITableSet::GetAMTTable() const
{
	return dynamic_cast<const AMTTable *>(GetTableByID(AMTTable::TABLE_ID));
}




TLVSIParser::TLVSIParser(PSISectionParser::PSISectionHandler *pHandler)
	: m_pHandler(pHandler)
	, m_Section(3 + 4093)
	, m_CRCErrorCount(0)
{
}


void TLVSIParser::StorePacket(const TLVPacket *pPacket)
{
	if ((pPacket == nullptr) || (pPacket->GetPayloadKind() != TLVPacket::PayloadKind::Signaling))
		return;

	const uint8_t *pData = pPacket->GetPayloadData();
	const size_t Size = pPacket->GetPayloadSize();

	for (size_t Pos = 0; (Pos + 3 <= Size) && (pData[Pos] != 0xFF);) {
		const size_t SectionSize = 3 + (Load16(&pData[Pos + 1]) & 0x0FFF);
		if (Pos + SectionSize > Size)
			break;

		m_Section.SetData(&pData[Pos], SectionSize);
		Pos += SectionSize;

		if (!m_Section.ParseHeader())
			continue;

		if (CRC32MPEG2::Calc(m_Section.GetData(), SectionSize) != 0) {
			if (m_CRCErrorCount < std::numeric_limits<unsigned long>::max())
				m_CRCErrorCount++;
			continue;
		}

		if (m_pHandler != nullptr)
			m_pHandler->OnPSISection(nullptr, &m_Section);
	}
}


void TLVSIParser::Reset() noexcept
{
	m_Section.Reset();
	m_CRCErrorCount = 0;
}


} // namespace LibISDB
