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
 @file   MMTDescriptors.cpp
 @brief  MMT-SI 記述子
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "MMTDescriptors.hpp"
#include "MMTString.hpp"
#include "../Base/ARIBTime.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


namespace
{

bool DecodeText(const ARIBString &Src, ReturnArg<String> Dst)
{
	if (!Dst)
		return false;

	DecodeMMTString(Src, Dst);

	return !Dst->empty();
}

uint64_t Load64(const uint8_t *p)
{
	return (static_cast<uint64_t>(Load32(&p[0])) << 32) | Load32(&p[4]);
}

}




MPUTimestampDescriptor::MPUTimestampDescriptor() noexcept
{
	Reset();
}


void MPUTimestampDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_TimestampList.clear();
}


const MPUTimestampDescriptor::TimestampInfo * MPUTimestampDescriptor::GetTimestampInfo(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_TimestampList.size())
		return nullptr;
	return &m_TimestampList[Index];
}


bool MPUTimestampDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;

	m_TimestampList.resize(m_Length / 12);

	for (size_t i = 0; i < m_TimestampList.size(); i++) {
		m_TimestampList[i].MPUSequenceNumber   = Load32(&pPayload[i * 12 + 0]);
		m_TimestampList[i].MPUPresentationTime = Load64(&pPayload[i * 12 + 4]);
	}

	return true;
}




MMTAccessControlDescriptor::MMTAccessControlDescriptor() noexcept
{
	Reset();
}


void MMTAccessControlDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_CASystemID = 0;
	m_LocationInfo = MMTGeneralLocationInfo();
	m_PrivateData.ClearSize();
}


uint16_t MMTAccessControlDescriptor::GetPacketID() const noexcept
{
	if (m_LocationInfo.LocationType != MMTGeneralLocationInfo::LOCATION_TYPE_PACKET_ID)
		return MMT_PACKET_ID_INVALID;
	return m_LocationInfo.PacketID;
}


bool MMTAccessControlDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 3)
		return false;

	m_CASystemID = Load16(&pPayload[0]);

	const size_t LocationSize = m_LocationInfo.Parse(&pPayload[2], m_Length - 2);
	if (LocationSize == 0)
		return false;

	const size_t Pos = 2 + LocationSize;
	if (Pos < m_Length)
		m_PrivateData.SetData(&pPayload[Pos], m_Length - Pos);

	return true;
}




VideoComponentDescriptor::VideoComponentDescriptor() noexcept
{
	Reset();
}


void VideoComponentDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_VideoResolution = VIDEO_RESOLUTION_UNSPECIFIED;
	m_VideoAspectRatio = 0;
	m_VideoScanFlag = false;
	m_VideoFrameRate = 0;
	m_ComponentTag = MMT_COMPONENT_TAG_INVALID;
	m_VideoTransferCharacteristics = 0;
	m_LanguageCode = LANGUAGE_CODE_INVALID;
	m_Text.clear();
}


bool VideoComponentDescriptor::GetText(ReturnArg<String> Text) const
{
	return DecodeText(m_Text, Text);
}


bool VideoComponentDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 8)
		return false;

	m_VideoResolution              = pPayload[0] >> 4;
	m_VideoAspectRatio             = pPayload[0] & 0x0F;
	m_VideoScanFlag                = (pPayload[1] & 0x80) != 0;
	m_VideoFrameRate               = pPayload[1] & 0x1F;
	m_ComponentTag                 = Load16(&pPayload[2]);
	m_VideoTransferCharacteristics = pPayload[4] >> 4;
	m_LanguageCode                 = Load24(&pPayload[5]);
	m_Text.assign(&pPayload[8], m_Length - 8);

	return true;
}




MHStreamIdentifierDescriptor::MHStreamIdentifierDescriptor() noexcept
{
	Reset();
}


void MHStreamIdentifierDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_ComponentTag = MMT_COMPONENT_TAG_INVALID;
}


bool MHStreamIdentifierDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 2)
		return false;

	m_ComponentTag = Load16(&pPayload[0]);

	return true;
}




MHContentDescriptor::MHContentDescriptor() noexcept
{
	Reset();
}


void MHContentDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_NibbleList.clear();
}


bool MHContentDescriptor::GetNibble(int Index, ReturnArg<NibbleInfo> Nibble) const
{
	if ((static_cast<unsigned int>(Index) >= m_NibbleList.size()) || !Nibble)
		return false;
	*Nibble = m_NibbleList[Index];
	return true;
}


bool MHContentDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;

	m_NibbleList.resize(m_Length / 2);

	for (size_t i = 0; i < m_NibbleList.size(); i++) {
		NibbleInfo &Nibble = m_NibbleList[i];

		Nibble.ContentNibbleLevel1 = pPayload[i * 2 + 0] >> 4;
		Nibble.ContentNibbleLevel2 = pPayload[i * 2 + 0] & 0x0F;
		Nibble.UserNibble1         = pPayload[i * 2 + 1] >> 4;
		Nibble.UserNibble2         = pPayload[i * 2 + 1] & 0x0F;
	}

	return true;
}




MHAudioComponentDescriptor::MHAudioComponentDescriptor() noexcept
{
	Reset();
}


void MHAudioComponentDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_StreamContent = STREAM_CONTENT_INVALID;
	m_ComponentType = COMPONENT_TYPE_INVALID;
	m_ComponentTag = MMT_COMPONENT_TAG_INVALID;
	m_StreamType = STREAM_TYPE_INVALID;
	m_SimulcastGroupTag = 0xFF_u8;
	m_ESMultiLingualFlag = false;
	m_MainComponentFlag = false;
	m_QualityIndicator = 0;
	m_SamplingRate = 0;
	m_LanguageCode = LANGUAGE_CODE_INVALID;
	m_LanguageCode2 = LANGUAGE_CODE_INVALID;
	m_Text.clear();
}


bool MHAudioComponentDescriptor::GetText(ReturnArg<String> Text) const
{
	return DecodeText(m_Text, Text);
}


bool MHAudioComponentDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 10)
		return false;

	m_StreamContent      = pPayload[0] & 0x0F;
	m_ComponentType      = pPayload[1];
	m_ComponentTag       = Load16(&pPayload[2]);
	m_StreamType         = pPayload[4];
	m_SimulcastGroupTag  = pPayload[5];
	m_ESMultiLingualFlag = (pPayload[6] & 0x80) != 0;
	m_MainComponentFlag  = (pPayload[6] & 0x40) != 0;
	m_QualityIndicator   = (pPayload[6] & 0x30) >> 4;
	m_SamplingRate       = (pPayload[6] & 0x0E) >> 1;
	m_LanguageCode       = Load24(&pPayload[7]);

	size_t Pos = 10;
	if (m_ESMultiLingualFlag) {
		if (m_Length < 13)
			return false;
		m_LanguageCode2 = Load24(&pPayload[10]);
		Pos = 13;
	}

	m_Text.assign(&pPayload[Pos], m_Length - Pos);

	return true;
}




MHSeriesDescriptor::MHSeriesDescriptor() noexcept
{
	Reset();
}


void MHSeriesDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_SeriesID = 0;
	m_RepeatLabel = 0;
	m_ProgramPattern = SeriesDescriptor::PROGRAM_PATTERN_INVALID;
	m_ExpireDateValidFlag = false;
	m_ExpireDate.Reset();
	m_EpisodeNumber = 0;
	m_LastEpisodeNumber = 0;
	m_SeriesName.clear();
}


bool MHSeriesDescriptor::GetExpireDate(ReturnArg<DateTime> Date) const
{
	if (!Date || !m_ExpireDateValidFlag)
		return false;
	*Date = m_ExpireDate;
	return true;
}


bool MHSeriesDescriptor::GetSeriesName(ReturnArg<String> Name) const
{
	return DecodeText(m_SeriesName, Name);
}


bool MHSeriesDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 8)
		return false;

	m_SeriesID            = Load16(&pPayload[0]);
	m_RepeatLabel         = pPayload[2] >> 4;
	m_ProgramPattern      = (pPayload[2] & 0x0E) >> 1;
	m_ExpireDateValidFlag = (pPayload[2] & 0x01) != 0;
	if (m_ExpireDateValidFlag)
		MJDTimeToDateTime(Load16(&pPayload[3]), &m_ExpireDate);
	m_EpisodeNumber       = static_cast<uint16_t>((pPayload[5] << 4) | (pPayload[6] >> 4));
	m_LastEpisodeNumber   = static_cast<uint16_t>(((pPayload[6] & 0x0F) << 8) | pPayload[7]);
	m_SeriesName.assign(&pPayload[8], m_Length - 8);

	return true;
}




MHEventGroupDescriptor::MHEventGroupDescriptor() noexcept
{
	Reset();
}


void MHEventGroupDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_GroupType = EventGroupDescriptor::GROUP_TYPE_UNDEFINED;
	m_EventList.clear();
}


bool MHEventGroupDescriptor::GetEventInfo(int Index, ReturnArg<EventInfo> Info) const
{
	if ((static_cast<unsigned int>(Index) >= m_EventList.size()) || !Info)
		return false;
	*Info = m_EventList[Index];
	return true;
}


bool MHEventGroupDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 1)
		return false;

	m_GroupType = pPayload[0] >> 4;

	const size_t EventCount = pPayload[0] & 0x0F;
	if (1 + EventCount * 4 > m_Length)
		return false;

	size_t Pos = 1;

	for (size_t i = 0; i < EventCount; i++) {
		EventInfo &Info = m_EventList.emplace_back();
		Info.ServiceID         = Load16(&pPayload[Pos + 0]);
		Info.EventID           = Load16(&pPayload[Pos + 2]);
		Info.NetworkID         = NETWORK_ID_INVALID;
		Info.TransportStreamID = TRANSPORT_STREAM_ID_INVALID;
		Pos += 4;
	}

	if ((m_GroupType == EventGroupDescriptor::GROUP_TYPE_RELAY_TO_OTHER_NETWORK)
			|| (m_GroupType == EventGroupDescriptor::GROUP_TYPE_MOVEMENT_FROM_OTHER_NETWORK)) {
		for (; Pos + 8 <= m_Length; Pos += 8) {
			EventInfo &Info = m_EventList.emplace_back();
			Info.NetworkID         = Load16(&pPayload[Pos + 0]);
			Info.TransportStreamID = Load16(&pPayload[Pos + 2]);
			Info.ServiceID         = Load16(&pPayload[Pos + 4]);
			Info.EventID           = Load16(&pPayload[Pos + 6]);
		}
	}

	return true;
}




void MHServiceListDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_ServiceList.clear();
}


bool MHServiceListDescriptor::GetServiceInfo(int Index, ReturnArg<ServiceInfo> Info) const
{
	if ((static_cast<unsigned int>(Index) >= m_ServiceList.size()) || !Info)
		return false;
	*Info = m_ServiceList[Index];
	return true;
}


bool MHServiceListDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;

	m_ServiceList.resize(m_Length / 3);

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		m_ServiceList[i].ServiceID   = Load16(&pPayload[i * 3 + 0]);
		m_ServiceList[i].ServiceType = pPayload[i * 3 + 2];
	}

	return true;
}




void MHBroadcasterNameDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_BroadcasterName.clear();
}


bool MHBroadcasterNameDescriptor::GetBroadcasterName(ReturnArg<String> Name) const
{
	return DecodeText(m_BroadcasterName, Name);
}


bool MHBroadcasterNameDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;

	m_BroadcasterName.assign(pPayload, m_Length);

	return true;
}




MHServiceDescriptor::MHServiceDescriptor() noexcept
{
	Reset();
}


void MHServiceDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_ServiceType = SERVICE_TYPE_INVALID;
	m_ProviderName.clear();
	m_ServiceName.clear();
}


bool MHServiceDescriptor::GetProviderName(ReturnArg<String> Name) const
{
	return DecodeText(m_ProviderName, Name);
}


bool MHServiceDescriptor::GetServiceName(ReturnArg<String> Name) const
{
	return DecodeText(m_ServiceName, Name);
}


bool MHServiceDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 3)
		return false;

	m_ServiceType = pPayload[0];

	size_t Pos = 1;
	const size_t ProviderNameLength = pPayload[Pos++];
	if (Pos + ProviderNameLength + 1 > m_Length)
		return false;
	m_ProviderName.assign(&pPayload[Pos], ProviderNameLength);
	Pos += ProviderNameLength;

	const size_t ServiceNameLength = pPayload[Pos++];
	if (Pos + ServiceNameLength > m_Length)
		return false;
	m_ServiceName.assign(&pPayload[Pos], ServiceNameLength);

	return true;
}




MHDataComponentDescriptor::MHDataComponentDescriptor() noexcept
{
	Reset();
}


void MHDataComponentDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_DataComponentID = DATA_COMPONENT_ID_INVALID;
	m_AdditionalDataComponentInfo.ClearSize();
}


bool MHDataComponentDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 2)
		return false;

	m_DataComponentID = Load16(&pPayload[0]);
	m_AdditionalDataComponentInfo.SetData(&pPayload[2], m_Length - 2);

	return true;
}


bool MHDataComponentDescriptor::GetAribSubtitleInfo(ReturnArg<AribSubtitleInfo> Info) const
{
	if (!Info || (m_DataComponentID != DATA_COMPONENT_ID_CAPTION))
		return false;

	const uint8_t *pData = m_AdditionalDataComponentInfo.GetData();
	const size_t Size = m_AdditionalDataComponentInfo.GetSize();
	if (Size < 8)
		return false;

	AribSubtitleInfo Subtitle = {};

	Subtitle.SubtitleTag                = pData[0];
	Subtitle.SubtitleInfoVersion        = pData[1] >> 4;
	Subtitle.StartMPUSequenceNumberFlag = (pData[1] & 0x08) != 0;
	Subtitle.LanguageCode               = Load24(&pData[2]);
	Subtitle.Type                       = pData[5] >> 6;
	Subtitle.SubtitleFormat             = (pData[5] >> 2) & 0x0F;
	Subtitle.OPM                        = pData[5] & 0x03;
	Subtitle.TMD                        = pData[6] >> 4;
	Subtitle.DMF                        = pData[6] & 0x0F;
	Subtitle.Resolution                 = pData[7] >> 4;
	Subtitle.CompressionType            = pData[7] & 0x0F;

	size_t Pos = 8;
	if (Subtitle.StartMPUSequenceNumberFlag) {
		if (Pos + 4 > Size)
			return false;
		Subtitle.StartMPUSequenceNumber = Load32(&pData[Pos]);
		Pos += 4;
	}
	if (Subtitle.TMD == TMD_REFERENCE_START_TIME) {
		if (Pos + 9 > Size)
			return false;
		Subtitle.ReferenceStartTime = Load64(&pData[Pos]);
		Subtitle.ReferenceStartTimeLeapIndicator = pData[Pos + 8] >> 6;
	}

	*Info = Subtitle;

	return true;
}




void MHLocalTimeOffsetDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_TimeOffsetList.clear();
}


bool MHLocalTimeOffsetDescriptor::GetTimeOffsetInfo(int Index, ReturnArg<TimeOffsetInfo> Info) const
{
	if ((static_cast<unsigned int>(Index) >= m_TimeOffsetList.size()) || !Info)
		return false;
	*Info = m_TimeOffsetList[Index];
	return true;
}


bool MHLocalTimeOffsetDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 13)
		return false;

	m_TimeOffsetList.resize(m_Length / 13);

	size_t Pos = 0;

	for (TimeOffsetInfo &Info : m_TimeOffsetList) {
		Info.CountryCode             = Load24(&pPayload[Pos]);
		Info.CountryRegionID         = (pPayload[Pos + 3] & 0xFC) >> 2;
		Info.LocalTimeOffsetPolarity = (pPayload[Pos + 3] & 0x01) != 0;
		Info.LocalTimeOffset         = BCDTimeHMToMinute(Load16(&pPayload[Pos + 4]));
		MJDBCDTimeToDateTime(&pPayload[Pos + 6], &Info.TimeOfChange);
		Info.NextTimeOffset          = BCDTimeHMToMinute(Load16(&pPayload[Pos + 11]));

		Pos += 13;
	}

	return true;
}




MHLogoTransmissionDescriptor::MHLogoTransmissionDescriptor() noexcept
{
	Reset();
}


void MHLogoTransmissionDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_LogoTransmissionType = 0;
	m_LogoID = LOGO_ID_INVALID;
	m_LogoVersion = LOGO_VERSION_INVALID;
	m_DownloadDataID = DATA_ID_INVALID;
	m_LogoList.clear();
	m_LogoChar.clear();
}


bool MHLogoTransmissionDescriptor::GetLogoInfo(int Index, ReturnArg<LogoInfo> Info) const
{
	if ((static_cast<unsigned int>(Index) >= m_LogoList.size()) || !Info)
		return false;
	*Info = m_LogoList[Index];
	return true;
}


bool MHLogoTransmissionDescriptor::GetLogoChar(ReturnArg<String> Char) const
{
	return DecodeText(m_LogoChar, Char);
}


bool MHLogoTransmissionDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 1)
		return false;

	m_LogoTransmissionType = pPayload[0];

	switch (m_LogoTransmissionType) {
	case TRANSMISSION_CDT1:
		if (m_Length < 7)
			return false;
		m_LogoID         = Load16(&pPayload[1]) & 0x01FF;
		m_LogoVersion    = Load16(&pPayload[3]) & 0x0FFF;
		m_DownloadDataID = Load16(&pPayload[5]);
		for (size_t Pos = 7; Pos + 3 <= m_Length; Pos += 3) {
			LogoInfo &Info = m_LogoList.emplace_back();
			Info.LogoType           = pPayload[Pos + 0];
			Info.StartSectionNumber = pPayload[Pos + 1];
			Info.NumOfSections      = pPayload[Pos + 2];
		}
		break;

	case TRANSMISSION_CDT2:
		if (m_Length < 3)
			return false;
		m_LogoID = Load16(&pPayload[1]) & 0x01FF;
		break;

	case TRANSMISSION_CHAR:
		m_LogoChar.assign(&pPayload[1], m_Length - 1);
		break;
	}

	return true;
}




MPUExtendedTimestampDescriptor::MPUExtendedTimestampDescriptor() noexcept
{
	Reset();
}


void MPUExtendedTimestampDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_PTSOffsetType = PTS_OFFSET_TYPE_FIXED;
	m_TimescaleFlag = false;
	m_Timescale = 0;
	m_DefaultPTSOffset = 0;
	m_MPUList.clear();
}


const MPUExtendedTimestampDescriptor::MPUInfo * MPUExtendedTimestampDescriptor::GetMPUInfo(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_MPUList.size())
		return nullptr;
	return &m_MPUList[Index];
}


bool MPUExtendedTimestampDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 1)
		return false;

	m_PTSOffsetType = (pPayload[0] >> 1) & 0x03;
	m_TimescaleFlag = (pPayload[0] & 0x01) != 0;

	size_t Pos = 1;
	if (m_TimescaleFlag) {
		if (Pos + 4 > m_Length)
			return false;
		m_Timescale = Load32(&pPayload[Pos]);
		Pos += 4;
	}
	if (m_PTSOffsetType == PTS_OFFSET_TYPE_DEFAULT) {
		if (Pos + 2 > m_Length)
			return false;
		m_DefaultPTSOffset = Load16(&pPayload[Pos]);
		Pos += 2;
	}

	const size_t AUInfoSize = (m_PTSOffsetType == PTS_OFFSET_TYPE_INDIVIDUAL) ? 4 : 2;

	while (Pos + 8 <= m_Length) {
		MPUInfo &Info = m_MPUList.emplace_back();

		Info.MPUSequenceNumber                = Load32(&pPayload[Pos]);
		Info.MPUPresentationTimeLeapIndicator = pPayload[Pos + 4] >> 6;
		Info.MPUDecodingTimeOffset            = Load16(&pPayload[Pos + 5]);
		const size_t AUCount                  = pPayload[Pos + 7];
		Pos += 8;
		if (Pos + AUCount * AUInfoSize > m_Length)
			return false;

		Info.AccessUnitList.resize(AUCount);
		for (AccessUnitInfo &AU : Info.AccessUnitList) {
			AU.DTSPTSOffset = Load16(&pPayload[Pos]);
			AU.PTSOffset = (m_PTSOffsetType == PTS_OFFSET_TYPE_INDIVIDUAL) ? Load16(&pPayload[Pos + 2]) : m_DefaultPTSOffset;
			Pos += AUInfoSize;
		}
	}

	return true;
}




MHShortEventDescriptor::MHShortEventDescriptor() noexcept
{
	Reset();
}


void MHShortEventDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_LanguageCode = LANGUAGE_CODE_INVALID;
	m_EventName.clear();
	m_EventDescription.clear();
}


bool MHShortEventDescriptor::GetEventName(ReturnArg<String> Name) const
{
	return DecodeText(m_EventName, Name);
}


bool MHShortEventDescriptor::GetEventDescription(ReturnArg<String> Description) const
{
	return DecodeText(m_EventDescription, Description);
}


bool MHShortEventDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 6)
		return false;

	m_LanguageCode = Load24(&pPayload[0]);

	size_t Pos = 3;
	const size_t EventNameLength = pPayload[Pos++];
	if (Pos + EventNameLength + 2 > m_Length)
		return false;
	m_EventName.assign(&pPayload[Pos], EventNameLength);
	Pos += EventNameLength;

	const size_t TextLength = Load16(&pPayload[Pos]);
	Pos += 2;
	if (Pos + TextLength > m_Length)
		return false;
	m_EventDescription.assign(&pPayload[Pos], TextLength);

	return true;
}




MHExtendedEventDescriptor::MHExtendedEventDescriptor() noexcept
{
	Reset();
}


void MHExtendedEventDescriptor::Reset() noexcept
{
	MMTDescriptorBase::Reset();

	m_DescriptorNumber = 0;
	m_LastDescriptorNumber = 0;
	m_LanguageCode = LANGUAGE_CODE_INVALID;
	m_ItemList.clear();
	m_Text.clear();
}


const MHExtendedEventDescriptor::ItemInfo * MHExtendedEventDescriptor::GetItem(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ItemList.size())
		return nullptr;
	return &m_ItemList[Index];
}


bool MHExtendedEventDescriptor::GetText(ReturnArg<String> Text) const
{
	return DecodeText(m_Text, Text);
}


bool MHExtendedEventDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 6)
		return false;

	m_DescriptorNumber     = pPayload[0] >> 4;
	m_LastDescriptorNumber = pPayload[0] & 0x0F;
	m_LanguageCode         = Load24(&pPayload[1]);

	const size_t ItemsEnd = 6 + Load16(&pPayload[4]);
	if (ItemsEnd > m_Length)
		return false;

	for (size_t Pos = 6; Pos < ItemsEnd;) {
		const size_t DescriptionLength = pPayload[Pos++];
		if (Pos + DescriptionLength + 2 > ItemsEnd)
			return false;
		const size_t DescriptionPos = Pos;
		Pos += DescriptionLength;
		const size_t ItemLength = Load16(&pPayload[Pos]);
		Pos += 2;
		if (Pos + ItemLength > ItemsEnd)
			return false;

		ItemInfo &Item = m_ItemList.emplace_back();
		Item.Description.assign(&pPayload[DescriptionPos], DescriptionLength);
		Item.ItemChar.assign(&pPayload[Pos], ItemLength);
		Pos += ItemLength;
	}

	if (ItemsEnd + 2 <= m_Length) {
		const size_t TextLength = Load16(&pPayload[ItemsEnd]);
		if (ItemsEnd + 2 + TextLength > m_Length)
			return false;
		m_Text.assign(&pPayload[ItemsEnd + 2], TextLength);
	}

	return true;
}


} // namespace LibISDB
