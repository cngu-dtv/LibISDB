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
 @file   MMTDescriptorBlock.cpp
 @brief  MMT-SI 記述子ブロック
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "MMTDescriptorBlock.hpp"
#include "MMTDescriptors.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


MMTDescriptorBlock::MMTDescriptorBlock(const MMTDescriptorBlock &Src)
{
	*this = Src;
}


MMTDescriptorBlock::MMTDescriptorBlock(MMTDescriptorBlock &&Src) noexcept
{
	*this = std::move(Src);
}


MMTDescriptorBlock & MMTDescriptorBlock::operator = (const MMTDescriptorBlock &Src)
{
	if (&Src != this) {
		Reset();
		m_DescriptorList.reserve(Src.m_DescriptorList.size());

		for (auto &e : Src.m_DescriptorList) {
			m_DescriptorList.push_back(std::unique_ptr<MMTDescriptorBase>(e->Clone()));
		}
	}

	return *this;
}


MMTDescriptorBlock & MMTDescriptorBlock::operator = (MMTDescriptorBlock &&Src) noexcept
{
	if (&Src != this) {
		Reset();
		m_DescriptorList.swap(Src.m_DescriptorList);
	}

	return *this;
}


int MMTDescriptorBlock::ParseBlock(const uint8_t *pData, size_t DataLength)
{
	Reset();

	if (pData == nullptr)
		return 0;

	for (size_t Pos = 0; Pos + 2 <= DataLength;) {
		std::unique_ptr<MMTDescriptorBase> Descriptor(CreateDescriptorInstance(Load16(&pData[Pos])));

		const bool IsValid = Descriptor->Parse(&pData[Pos], DataLength - Pos);
		const size_t Size = Descriptor->GetSize();
		if (Size == 0)
			break;

		if (IsValid)
			m_DescriptorList.push_back(std::move(Descriptor));

		Pos += Size;
	}

	return static_cast<int>(m_DescriptorList.size());
}


void MMTDescriptorBlock::Reset() noexcept
{
	m_DescriptorList.clear();
}


int MMTDescriptorBlock::GetDescriptorCount() const
{
	return static_cast<int>(m_DescriptorList.size());
}


const MMTDescriptorBase * MMTDescriptorBlock::GetDescriptorByIndex(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_DescriptorList.size())
		return nullptr;
	return m_DescriptorList[Index].get();
}


const MMTDescriptorBase * MMTDescriptorBlock::GetDescriptorByTag(uint16_t Tag) const
{
	for (auto &e : m_DescriptorList) {
		if (e->GetTag() == Tag)
			return e.get();
	}

	return nullptr;
}


MMTDescriptorBase * MMTDescriptorBlock::CreateDescriptorInstance(uint16_t Tag)
{
	switch (Tag) {
	case MPUTimestampDescriptor::TAG         : return new MPUTimestampDescriptor;
	case MMTAccessControlDescriptor::TAG     : return new MMTAccessControlDescriptor;
	case VideoComponentDescriptor::TAG       : return new VideoComponentDescriptor;
	case MHStreamIdentifierDescriptor::TAG   : return new MHStreamIdentifierDescriptor;
	case MHContentDescriptor::TAG            : return new MHContentDescriptor;
	case MHAudioComponentDescriptor::TAG     : return new MHAudioComponentDescriptor;
	case MHSeriesDescriptor::TAG             : return new MHSeriesDescriptor;
	case MHEventGroupDescriptor::TAG         : return new MHEventGroupDescriptor;
	case MHServiceListDescriptor::TAG        : return new MHServiceListDescriptor;
	case MHBroadcasterNameDescriptor::TAG    : return new MHBroadcasterNameDescriptor;
	case MHServiceDescriptor::TAG            : return new MHServiceDescriptor;
	case MHDataComponentDescriptor::TAG      : return new MHDataComponentDescriptor;
	case MHLocalTimeOffsetDescriptor::TAG    : return new MHLocalTimeOffsetDescriptor;
	case MHLogoTransmissionDescriptor::TAG   : return new MHLogoTransmissionDescriptor;
	case MPUExtendedTimestampDescriptor::TAG : return new MPUExtendedTimestampDescriptor;
	case ApplicationServiceDescriptor::TAG   : return new ApplicationServiceDescriptor;
	case MHShortEventDescriptor::TAG         : return new MHShortEventDescriptor;
	case MHExtendedEventDescriptor::TAG      : return new MHExtendedEventDescriptor;
	}

	return new MMTDescriptorBase;
}


} // namespace LibISDB
