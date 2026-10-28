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
 @file   MMTGeneralLocationInfo.cpp
 @brief  MMT_general_location_info
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "MMTGeneralLocationInfo.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


size_t MMTGeneralLocationInfo::Parse(const uint8_t *pData, size_t Size)
{
	*this = MMTGeneralLocationInfo();

	if ((pData == nullptr) || (Size < 1))
		return 0;

	const uint8_t Type = pData[0];
	size_t Pos = 1;

	switch (Type) {
	case LOCATION_TYPE_PACKET_ID:
		if (Pos + 2 > Size)
			return 0;
		PacketID = Load16(&pData[Pos]);
		Pos += 2;
		break;

	case LOCATION_TYPE_IPV4:
		if (Pos + 12 > Size)
			return 0;
		std::memcpy(SourceAddress.data(), &pData[Pos], 4);
		std::memcpy(DestinationAddress.data(), &pData[Pos + 4], 4);
		DestinationPort = Load16(&pData[Pos + 8]);
		PacketID = Load16(&pData[Pos + 10]);
		Pos += 12;
		break;

	case LOCATION_TYPE_IPV6:
		if (Pos + 36 > Size)
			return 0;
		std::memcpy(SourceAddress.data(), &pData[Pos], 16);
		std::memcpy(DestinationAddress.data(), &pData[Pos + 16], 16);
		DestinationPort = Load16(&pData[Pos + 32]);
		PacketID = Load16(&pData[Pos + 34]);
		Pos += 36;
		break;

	case LOCATION_TYPE_MPEG2_TS:
		if (Pos + 6 > Size)
			return 0;
		NetworkID = Load16(&pData[Pos]);
		TransportStreamID = Load16(&pData[Pos + 2]);
		PID = Load16(&pData[Pos + 4]) & 0x1FFF;
		Pos += 6;
		break;

	case LOCATION_TYPE_MPEG2_TS_IPV6:
		if (Pos + 36 > Size)
			return 0;
		std::memcpy(SourceAddress.data(), &pData[Pos], 16);
		std::memcpy(DestinationAddress.data(), &pData[Pos + 16], 16);
		DestinationPort = Load16(&pData[Pos + 32]);
		PID = Load16(&pData[Pos + 34]) & 0x1FFF;
		Pos += 36;
		break;

	case LOCATION_TYPE_URL:
		{
			if (Pos + 1 > Size)
				return 0;
			const size_t Length = pData[Pos];
			Pos++;
			if (Pos + Length > Size)
				return 0;
			URL.assign(reinterpret_cast<const char *>(&pData[Pos]), Length);
			Pos += Length;
		}
		break;

	default:
		return 0;
	}

	LocationType = Type;

	return Pos;
}


} // namespace LibISDB
