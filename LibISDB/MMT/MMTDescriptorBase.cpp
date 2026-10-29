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
 @file   MMTDescriptorBase.cpp
 @brief  MMT-SI 記述子の基底
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "MMTDescriptorBase.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


MMTDescriptorBase::MMTDescriptorBase() noexcept
	: m_Tag(0)
	, m_Length(0)
	, m_Size(0)
	, m_IsValid(false)
{
}


void MMTDescriptorBase::CopyFrom(const MMTDescriptorBase *pSrc)
{
	*this = *pSrc;
}


MMTDescriptorBase * MMTDescriptorBase::Clone() const
{
	return new MMTDescriptorBase(*this);
}


bool MMTDescriptorBase::Parse(const uint8_t *pData, size_t DataLength)
{
	Reset();

	if ((pData == nullptr) || (DataLength < 2))
		return false;

	const uint16_t Tag = Load16(&pData[0]);
	const size_t LengthSize = GetLengthFieldSize(Tag);
	if (2 + LengthSize > DataLength)
		return false;

	uint32_t Length;
	switch (LengthSize) {
	case 1:  Length = pData[2];           break;
	case 2:  Length = Load16(&pData[2]);  break;
	default: Length = Load32(&pData[2]);  break;
	}
	if (Length > DataLength - (2 + LengthSize))
		return false;

	m_Tag = Tag;
	m_Length = Length;
	m_Size = 2 + LengthSize + Length;

	m_IsValid = StoreContents(&pData[2 + LengthSize]);

	return m_IsValid;
}


void MMTDescriptorBase::Reset() noexcept
{
	m_Tag = 0;
	m_Length = 0;
	m_Size = 0;
	m_IsValid = false;
}


size_t MMTDescriptorBase::GetLengthFieldSize(uint16_t Tag) noexcept
{
	// 依存記述子 (ARIB STD-B60 表 7-32)
	if (Tag == 0x0002)
		return 2;
	if ((Tag >= 0x4000) && (Tag <= 0x6FFF))
		return 2;
	if ((Tag >= 0x7000) && (Tag <= 0x7FFF))
		return 4;
	if (Tag >= 0xF000)
		return 2;
	return 1;
}


bool MMTDescriptorBase::StoreContents(const uint8_t *pPayload)
{
	return true;
}


} // namespace LibISDB
