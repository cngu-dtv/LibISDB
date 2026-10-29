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
 @file   LATMParser.cpp
 @brief  LATM 解析
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "LATMParser.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


LATMParser::LATMParser() noexcept
{
	Reset();
}


void LATMParser::Reset() noexcept
{
	m_HasConfig = false;
	m_ConfigChanged = false;
	m_Config = AudioConfig();
	m_Payload.clear();
}


bool LATMParser::ParseAudioMuxElement(const uint8_t *pData, size_t Size)
{
	m_ConfigChanged = false;

	if ((pData == nullptr) || (Size == 0))
		return false;

	BitstreamReader Reader(pData, Size);

	const bool UseSameStreamMux = Reader.GetFlag();
	AudioConfig Config;
	if (!UseSameStreamMux) {
		if (!ParseStreamMuxConfig(Reader, pData, &Config))
			return false;
	} else if (!m_HasConfig) {
		return false;
	}

	// PayloadLengthInfo() (frameLengthType = 0)
	size_t PayloadLength = 0;
	for (;;) {
		const uint32_t Tmp = Reader.GetBits(8);
		if (Reader.IsOverrun())
			return false;
		PayloadLength += Tmp;
		if (Tmp != 255)
			break;
	}

	const size_t PayloadPos = Reader.GetPos();
	if ((PayloadLength == 0) || (PayloadPos + PayloadLength * 8 > Size * 8))
		return false;

	if (!UseSameStreamMux) {
		if (!m_HasConfig || (Config != m_Config)) {
			m_Config = std::move(Config);
			m_ConfigChanged = true;
		}
		m_HasConfig = true;
	}

	CopyBits(pData, PayloadPos, PayloadLength * 8, &m_Payload);

	return true;
}


uint32_t LATMParser::GetSamplingFrequency(uint8_t Index) noexcept
{
	static const uint32_t FrequencyTable[] = {
		96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
		16000, 12000, 11025,  8000,  7350,
	};

	if (Index >= std::size(FrequencyTable))
		return 0;
	return FrequencyTable[Index];
}


bool LATMParser::ParseStreamMuxConfig(BitstreamReader &Reader, const uint8_t *pData, ReturnArg<AudioConfig> Config)
{
	const bool AudioMuxVersion = Reader.GetFlag();
	if (AudioMuxVersion) {
		if (Reader.GetFlag())                // audioMuxVersionA
			return false;
		GetLATMValue(Reader);                // taraBufferFullness
	}

	Reader.Skip(1);                          // allStreamsSameTimeFraming
	if (Reader.GetBits(6) != 0)              // numSubFrames
		return false;
	if (Reader.GetBits(4) != 0)              // numProgram
		return false;
	if (Reader.GetBits(3) != 0)              // numLayer
		return false;
	if (Reader.IsOverrun())
		return false;

	// AudioSpecificConfig()
	const size_t ASCLength = AudioMuxVersion ? GetLATMValue(Reader) : 0;
	const size_t ConfigPos = Reader.GetPos();
	if (!ParseAudioSpecificConfig(Reader, ConfigPos, Config))
		return false;
	const size_t ConfigBits = Reader.GetPos() - ConfigPos;
	if (AudioMuxVersion) {
		if (ConfigBits > ASCLength)
			return false;
		Reader.Skip(ASCLength - ConfigBits); // fillBits
	}
	if (Reader.IsOverrun())
		return false;
	CopyBits(pData, ConfigPos, ConfigBits, &Config->SpecificConfig);

	if (Reader.GetBits(3) != 0)              // frameLengthType
		return false;
	Reader.Skip(8);                          // latmBufferFullness

	if (Reader.GetFlag()) {                  // otherDataPresent
		if (AudioMuxVersion) {
			GetLATMValue(Reader);            // otherDataLenBits
		} else {
			bool Esc;
			do {
				Esc = Reader.GetFlag();      // otherDataLenEsc
				Reader.Skip(8);              // otherDataLenTmp
			} while (Esc && !Reader.IsOverrun());
		}
	}
	if (Reader.GetFlag())                    // crcCheckPresent
		Reader.Skip(8);                      // crcCheckSum

	return !Reader.IsOverrun();
}


uint32_t LATMParser::GetLATMValue(BitstreamReader &Reader)
{
	const uint32_t BytesForValue = Reader.GetBits(2);
	uint32_t Value = 0;

	for (uint32_t i = 0; i <= BytesForValue; i++)
		Value = (Value << 8) | Reader.GetBits(8);

	return Value;
}


bool LATMParser::ParseAudioSpecificConfig(BitstreamReader &Reader, size_t StartPos, ReturnArg<AudioConfig> Config)
{
	const auto ReadAudioObjectType =
		[&Reader]() -> uint8_t {
			uint8_t Type = static_cast<uint8_t>(Reader.GetBits(5));
			if (Type == 31)
				Type = static_cast<uint8_t>(32 + Reader.GetBits(6));
			return Type;
		};
	const auto ReadSamplingFrequency =
		[&Reader](ReturnArg<uint8_t> Index) -> uint32_t {
			*Index = static_cast<uint8_t>(Reader.GetBits(4));
			if (*Index == 0x0F)
				return Reader.GetBits(24);
			return GetSamplingFrequency(*Index);
		};

	uint8_t AudioObjectType = ReadAudioObjectType();
	Config->SamplingFrequency = ReadSamplingFrequency(&Config->SamplingFrequencyIndex);
	Config->ChannelConfiguration = static_cast<uint8_t>(Reader.GetBits(4));

	// 明示的な SBR / PS の指定 (ISO/IEC 14496-3 1.6.5.2)
	if ((AudioObjectType == 5) || (AudioObjectType == 29)) {
		uint8_t ExtensionIndex;
		ReadSamplingFrequency(&ExtensionIndex);
		AudioObjectType = ReadAudioObjectType();
		if (AudioObjectType == 22)
			Reader.Skip(4);                  // extensionChannelConfiguration
	}

	Config->AudioObjectType = AudioObjectType;

	switch (AudioObjectType) {
	case 1: case 2: case 3: case 4: case 6: case 7:
		// GASpecificConfig()
		{
			Reader.Skip(1);                  // frameLengthFlag
			if (Reader.GetFlag())            // dependsOnCoreCoder
				Reader.Skip(14);             // coreCoderDelay
			const bool ExtensionFlag = Reader.GetFlag();
			if (Config->ChannelConfiguration == 0) {
				if (!ParseProgramConfigElement(Reader, StartPos))
					return false;
			}
			if (AudioObjectType == 6)
				Reader.Skip(3);              // layerNr
			if (ExtensionFlag)
				Reader.Skip(1);              // extensionFlag3
		}
		break;

	default:
		// 誤り耐性 (ER) 等の他の AOT は扱わない
		return false;
	}

	return !Reader.IsOverrun();
}


bool LATMParser::ParseProgramConfigElement(BitstreamReader &Reader, size_t StartPos)
{
	Reader.Skip(4 + 2 + 4);                  // element_instance_tag, object_type, sampling_frequency_index
	const uint32_t NumFront = Reader.GetBits(4);
	const uint32_t NumSide  = Reader.GetBits(4);
	const uint32_t NumBack  = Reader.GetBits(4);
	const uint32_t NumLFE   = Reader.GetBits(2);
	const uint32_t NumAssoc = Reader.GetBits(3);
	const uint32_t NumCC    = Reader.GetBits(4);
	if (Reader.GetFlag())                    // mono_mixdown_present
		Reader.Skip(4);
	if (Reader.GetFlag())                    // stereo_mixdown_present
		Reader.Skip(4);
	if (Reader.GetFlag())                    // matrix_mixdown_idx_present
		Reader.Skip(2 + 1);
	Reader.Skip((NumFront + NumSide + NumBack) * 5 + NumLFE * 4 + NumAssoc * 4 + NumCC * 5);

	// byte_alignment() は AudioSpecificConfig() の先頭からの位置で行う
	const size_t Offset = (Reader.GetPos() - StartPos) & 7;
	if (Offset != 0)
		Reader.Skip(8 - Offset);

	const uint32_t CommentBytes = Reader.GetBits(8);
	Reader.Skip(CommentBytes * 8);

	return !Reader.IsOverrun();
}


void LATMParser::CopyBits(const uint8_t *pData, size_t BitPos, size_t BitCount, std::vector<uint8_t> *pDst)
{
	const size_t ByteCount = (BitCount + 7) / 8;
	const size_t Shift = BitPos & 7;
	const uint8_t *p = pData + (BitPos >> 3);

	pDst->resize(ByteCount);
	if (ByteCount == 0)
		return;

	if (Shift == 0) {
		std::memcpy(pDst->data(), p, ByteCount);
	} else {
		// 末尾のバイトを越えて読まないようにする
		const size_t SrcBytes = ((BitPos + BitCount + 7) >> 3) - (BitPos >> 3);
		for (size_t i = 0; i < ByteCount; i++) {
			uint8_t Byte = static_cast<uint8_t>(p[i] << Shift);
			if (i + 1 < SrcBytes)
				Byte |= static_cast<uint8_t>(p[i + 1] >> (8 - Shift));
			(*pDst)[i] = Byte;
		}
	}

	if ((BitCount & 7) != 0)
		pDst->back() &= static_cast<uint8_t>(0xFF << (8 - (BitCount & 7)));
}


} // namespace LibISDB
