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
 @file   MMTPPayload.cpp
 @brief  MMTP ペイロード (MPU)
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "MMTPPayload.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


bool MPUPayloadHeader::Parse(const uint8_t *pData, size_t Size)
{
	*this = MPUPayloadHeader();

	if ((pData == nullptr) || (Size < MPU_PAYLOAD_HEADER_SIZE))
		return false;

	PayloadLength          = Load16(&pData[0]);
	FragmentType           = pData[2] >> 4;
	TimedFlag              = (pData[2] & 0x08) != 0;
	FragmentationIndicator = (pData[2] >> 1) & 0x03;
	AggregationFlag        = (pData[2] & 0x01) != 0;
	FragmentCounter        = pData[3];
	MPUSequenceNumber      = Load32(&pData[4]);

	return true;
}


bool ParseMPUPayload(
	const uint8_t *pData, size_t Size,
	MPUPayloadHeader *pHeader, std::vector<MFUDataUnit> *pDataUnitList)
{
	pDataUnitList->clear();

	if (!pHeader->Parse(pData, Size))
		return false;

	// payload_length はこのフィールドの直後からペイロードの最後まで
	if (static_cast<size_t>(pHeader->PayloadLength) + 2 < Size)
		Size = static_cast<size_t>(pHeader->PayloadLength) + 2;

	if (pHeader->FragmentType != MPU_FRAGMENT_TYPE_MFU)
		return true;

	const size_t DUHeaderSize = pHeader->TimedFlag ? MFU_TIMED_HEADER_SIZE : MFU_NON_TIMED_HEADER_SIZE;
	size_t Pos = MPU_PAYLOAD_HEADER_SIZE;

	if (!pHeader->AggregationFlag) {
		if (Pos + DUHeaderSize > Size)
			return false;
		MFUDataUnit &Unit = pDataUnitList->emplace_back();
		Unit.pData = &pData[Pos + DUHeaderSize];
		Unit.Size = Size - (Pos + DUHeaderSize);
		Unit.ItemID = pHeader->TimedFlag ? 0 : Load32(&pData[Pos]);
		return true;
	}

	while (Pos + 2 <= Size) {
		const size_t DataUnitLength = Load16(&pData[Pos]);
		Pos += 2;
		if ((DataUnitLength < DUHeaderSize) || (Pos + DataUnitLength > Size))
			return false;
		MFUDataUnit &Unit = pDataUnitList->emplace_back();
		Unit.pData = &pData[Pos + DUHeaderSize];
		Unit.Size = DataUnitLength - DUHeaderSize;
		Unit.ItemID = pHeader->TimedFlag ? 0 : Load32(&pData[Pos]);
		Pos += DataUnitLength;
	}

	return true;
}


bool CaptionMFUHeader::Parse(const uint8_t *pData, size_t Size)
{
	*this = CaptionMFUHeader();

	if ((pData == nullptr) || (Size < 7))
		return false;

	SubtitleTag            = pData[0];
	SubtitleSequenceNumber = pData[1];
	SubsampleNumber        = pData[2];
	LastSubsampleNumber    = pData[3];
	DataType               = pData[4] >> 4;
	LengthExtensionFlag    = (pData[4] & 0x08) != 0;
	SubsampleInfoListFlag  = (pData[4] & 0x04) != 0;

	size_t Pos = 5;
	if (LengthExtensionFlag) {
		if (Pos + 4 > Size)
			return false;
		DataSize = Load32(&pData[Pos]);
		Pos += 4;
	} else {
		DataSize = Load16(&pData[Pos]);
		Pos += 2;
	}

	if ((SubsampleNumber == 0) && (LastSubsampleNumber > 0) && SubsampleInfoListFlag)
		Pos += static_cast<size_t>(LastSubsampleNumber) * (LengthExtensionFlag ? 5 : 3);

	if ((Pos > Size) || (DataSize > Size - Pos))
		return false;

	HeaderSize = Pos;

	return true;
}


} // namespace LibISDB
