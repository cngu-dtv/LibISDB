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
 @file   MMTPPacket.cpp
 @brief  MMTP パケット
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "MMTPPacket.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


bool MMTPHeader::Parse(const uint8_t *pData, size_t Size)
{
	*this = MMTPHeader();

	if ((pData == nullptr) || (Size < MMTP_HEADER_SIZE_MIN))
		return false;

	Version              = pData[0] >> 6;
	PacketCounterFlag    = (pData[0] & 0x20) != 0;
	FECType              = (pData[0] >> 3) & 0x03;
	ExtensionFlag        = (pData[0] & 0x02) != 0;
	RAPFlag              = (pData[0] & 0x01) != 0;
	PayloadType          = pData[1] & 0x3F;
	PacketID             = Load16(&pData[2]);
	Timestamp            = Load32(&pData[4]);
	PacketSequenceNumber = Load32(&pData[8]);

	if (Version != 0)
		return false;

	size_t Pos = MMTP_HEADER_SIZE_MIN;

	if (PacketCounterFlag) {
		if (Pos + 4 > Size)
			return false;
		PacketCounter = Load32(&pData[Pos]);
		Pos += 4;
	}

	if (ExtensionFlag) {
		if (Pos + 4 > Size)
			return false;
		ExtensionType = Load16(&pData[Pos + 0]);
		ExtensionLength = Load16(&pData[Pos + 2]);
		Pos += 4;
		if (Pos + ExtensionLength > Size)
			return false;
		ExtensionOffset = static_cast<uint32_t>(Pos);

		if (ExtensionType == MMTP_EXTENSION_TYPE_MULTI) {
			// マルチタイプヘッダ拡張 (ARIB STD-B60 表 6-8)
			const size_t End = Pos + ExtensionLength;
			size_t ExtPos = Pos;

			while (ExtPos + 4 <= End) {
				const uint16_t TypeField = Load16(&pData[ExtPos + 0]);
				const bool EndFlag = (TypeField & 0x8000) != 0;
				const uint16_t HdrExtType = TypeField & 0x7FFF;
				const size_t HdrExtLength = Load16(&pData[ExtPos + 2]);
				ExtPos += 4;
				if (ExtPos + HdrExtLength > End)
					return false;

				if ((HdrExtType == MMTP_HDR_EXT_TYPE_SCRAMBLE) && (HdrExtLength >= 1)) {
					// ARIB STD-B61 図 3-1
					const uint8_t Flags = pData[ExtPos];
					const size_t ScrambleEnd = ExtPos + HdrExtLength;
					size_t InfoPos = ExtPos + 1;

					ScramblingControl        = (Flags >> 3) & 0x03;
					HasScramblingSubsystemID = (Flags & 0x04) != 0;
					HasMessageAuthentication = (Flags & 0x02) != 0;
					HasSICV                  = (Flags & 0x01) != 0;

					if (HasScramblingSubsystemID) {
						if (InfoPos + 1 > ScrambleEnd)
							return false;
						ScramblingSubsystemID = pData[InfoPos];
						InfoPos += 1;
					}
					if (HasMessageAuthentication) {
						if (InfoPos + 2 > ScrambleEnd)
							return false;
						AuthenticatedPayloadLength = Load16(&pData[InfoPos]);
						InfoPos += 2;
					}
					if (HasSICV) {
						if (InfoPos + MMT_SICV_SIZE > ScrambleEnd)
							return false;
						SICVOffset = static_cast<uint32_t>(InfoPos);
					}

					HasScrambleInfo = true;
				}

				ExtPos += HdrExtLength;
				if (EndFlag)
					break;
			}
		}

		Pos += ExtensionLength;
	}

	HeaderSize = static_cast<uint32_t>(Pos);

	return true;
}


} // namespace LibISDB
