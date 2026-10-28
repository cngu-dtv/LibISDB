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
 @file   TLVSIDescriptors.cpp
 @brief  TLV-SI 記述子
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "TLVSIDescriptors.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


void RemoteControlKeyDescriptor::Reset() noexcept
{
	DescriptorBase::Reset();

	m_KeyList.clear();
}


bool RemoteControlKeyDescriptor::GetKeyInfo(int Index, ReturnArg<KeyInfo> Info) const
{
	if ((static_cast<unsigned int>(Index) >= m_KeyList.size()) || !Info)
		return false;
	*Info = m_KeyList[Index];
	return true;
}


uint8_t RemoteControlKeyDescriptor::GetRemoteControlKeyID(uint16_t ServiceID) const
{
	for (const KeyInfo &Info : m_KeyList) {
		if (Info.ServiceID == ServiceID)
			return Info.RemoteControlKeyID;
	}

	return 0;
}


bool RemoteControlKeyDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 1)
		return false;

	const size_t KeyCount = pPayload[0];
	if (1 + KeyCount * 5 > m_Length)
		return false;

	m_KeyList.resize(KeyCount);

	for (size_t i = 0; i < KeyCount; i++) {
		m_KeyList[i].RemoteControlKeyID = pPayload[1 + i * 5];
		m_KeyList[i].ServiceID          = Load16(&pPayload[1 + i * 5 + 1]);
	}

	return true;
}




int TLVSIDescriptorBlock::ParseBlock(const uint8_t *pData, size_t DataLength)
{
	Reset();

	if (pData == nullptr)
		return 0;

	for (size_t Pos = 0; Pos + 2 <= DataLength;) {
		const size_t Size = 2 + pData[Pos + 1];
		if (Pos + Size > DataLength)
			break;

		std::unique_ptr<DescriptorBase> Descriptor(CreateDescriptorInstance(pData[Pos]));
		if (Descriptor->Parse(&pData[Pos], static_cast<uint16_t>(Size)))
			m_DescriptorList.push_back(std::move(Descriptor));

		Pos += Size;
	}

	return static_cast<int>(m_DescriptorList.size());
}


DescriptorBase * TLVSIDescriptorBlock::CreateDescriptorInstance(uint8_t Tag)
{
	switch (Tag) {
	case NetworkNameDescriptor::TAG             : return new NetworkNameDescriptor;
	case ServiceListDescriptor::TAG             : return new ServiceListDescriptor;
	case SatelliteDeliverySystemDescriptor::TAG : return new SatelliteDeliverySystemDescriptor;
	case RemoteControlKeyDescriptor::TAG        : return new RemoteControlKeyDescriptor;
	case SystemManagementDescriptor::TAG        : return new SystemManagementDescriptor;
	}

	return new DescriptorBase;
}


} // namespace LibISDB
