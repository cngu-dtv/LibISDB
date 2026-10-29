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
 @file   MHEventInfo.cpp
 @brief  MH-EIT の番組情報
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "MHEventInfo.hpp"
#include "MMTString.hpp"
#include "../Utilities/Sort.hpp"
#include <algorithm>
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


namespace
{

// ARIB STD-B10 表 6-5 (H.265|MPEG-H HEVC)
constexpr uint8_t STREAM_CONTENT_HEVC = 0x09_u8;


// ARIB STD-B10 表 6-5 の component_type (上位 4 ビットが解像度と走査方式、下位 4 ビットがアスペクト比)
uint8_t GetVideoComponentType(const VideoComponentDescriptor *pDesc)
{
	const bool Progressive = pDesc->GetVideoScanFlag();
	uint8_t Type;

	switch (pDesc->GetVideoResolution()) {
	case VideoComponentDescriptor::VIDEO_RESOLUTION_180:  Type = 0xF0; break;
	case VideoComponentDescriptor::VIDEO_RESOLUTION_240:  Type = 0xD0; break;
	case VideoComponentDescriptor::VIDEO_RESOLUTION_480:  Type = Progressive ? 0xA0 : 0x00; break;
	case VideoComponentDescriptor::VIDEO_RESOLUTION_720:  Type = 0xC0; break;
	case VideoComponentDescriptor::VIDEO_RESOLUTION_1080: Type = Progressive ? 0xE0 : 0xB0; break;
	case VideoComponentDescriptor::VIDEO_RESOLUTION_2160: Type = 0x90; break;
	case VideoComponentDescriptor::VIDEO_RESOLUTION_4320: Type = 0x80; break;
	default:
		return COMPONENT_TYPE_INVALID;
	}

	const uint8_t AspectRatio = pDesc->GetVideoAspectRatio();
	if ((AspectRatio < 1) || (AspectRatio > 4))
		return COMPONENT_TYPE_INVALID;

	return Type | AspectRatio;
}


void SetContentNibble(const MHContentDescriptor *pContentDesc, EventInfo *pEvent)
{
	const int NibbleCount = std::min(pContentDesc->GetNibbleCount(), static_cast<int>(std::size(pEvent->ContentNibble.NibbleList)));
	pEvent->ContentNibble.NibbleCount = NibbleCount;
	for (int i = 0; i < NibbleCount; i++)
		pContentDesc->GetNibble(i, &pEvent->ContentNibble.NibbleList[i]);
}


// 既存のグループの後に追加する
void AddEventGroupList(const MMTDescriptorBlock *pDescBlock, EventInfo *pEvent)
{
	pDescBlock->EnumDescriptors<MHEventGroupDescriptor>(
		[&](const MHEventGroupDescriptor *pGroupDesc) {
			EventInfo::EventGroupInfo GroupInfo;
			GroupInfo.GroupType = pGroupDesc->GetGroupType();
			const int EventCount = pGroupDesc->GetEventCount();
			GroupInfo.EventList.resize(EventCount);
			for (int i = 0; i < EventCount; i++)
				pGroupDesc->GetEventInfo(i, &GroupInfo.EventList[i]);

			if (std::ranges::find(pEvent->EventGroupList, GroupInfo) != pEvent->EventGroupList.end())
				return;

			if ((GroupInfo.GroupType == EventGroupDescriptor::GROUP_TYPE_COMMON)
					&& (EventCount == 1)
					&& (GroupInfo.EventList.front().ServiceID != pEvent->ServiceID)) {
				pEvent->IsCommonEvent = true;
				pEvent->CommonEvent.ServiceID = GroupInfo.EventList.front().ServiceID;
				pEvent->CommonEvent.EventID = GroupInfo.EventList.front().EventID;
			}

			pEvent->EventGroupList.push_back(std::move(GroupInfo));
		});
}

}




bool GetMHEventInfo(const MHEITTable *pTable, int Index, ReturnArg<EventInfo> Info)
{
	if ((pTable == nullptr) || !Info)
		return false;

	const MHEITTable::EventInfo *pEvent = pTable->GetEventInfo(Index);
	if (pEvent == nullptr)
		return false;

	*Info = EventInfo();

	Info->NetworkID = pTable->GetOriginalNetworkID();
	Info->TransportStreamID = pTable->GetTLVStreamID();
	Info->ServiceID = pTable->GetServiceID();
	Info->EventID = pEvent->EventID;
	Info->StartTime = pEvent->StartTime;
	Info->Duration = pEvent->Duration;
	Info->RunningStatus = pEvent->RunningStatus;
	Info->FreeCAMode = pEvent->FreeCAMode;

	const MMTDescriptorBlock *pDescBlock = &pEvent->Descriptors;

	if (const MHShortEventDescriptor *pShortEvent = pDescBlock->GetDescriptor<MHShortEventDescriptor>();
			pShortEvent != nullptr) {
		pShortEvent->GetEventName(&Info->EventName);
		pShortEvent->GetEventDescription(&Info->EventText);
	}

	GetMHEventExtendedTextList(pDescBlock, &Info->ExtendedText);
	GetMHEventVideoList(pDescBlock, &Info->VideoList);
	GetMHEventAudioList(pDescBlock, &Info->AudioList);

	if (const MHContentDescriptor *pContentDesc = pDescBlock->GetDescriptor<MHContentDescriptor>();
			pContentDesc != nullptr)
		SetContentNibble(pContentDesc, &*Info);

	AddEventGroupList(pDescBlock, &*Info);

	Info->Type = EventInfo::TypeFlag::Basic | EventInfo::TypeFlag::Extended;
	if (pTable->GetTableID() == MMT_TABLE_ID_MH_EIT_PF) {
		Info->Type |= (pTable->GetSectionNumber() == 0) ?
			EventInfo::TypeFlag::Present : EventInfo::TypeFlag::Following;
	}

	return true;
}


bool GetMHEventExtendedTextList(
	const MMTDescriptorBlock *pDescBlock, ReturnArg<EventInfo::ExtendedTextInfoList> List)
{
	if (!List)
		return false;

	List->clear();

	if (pDescBlock == nullptr)
		return false;

	std::vector<const MHExtendedEventDescriptor *> DescList;
	pDescBlock->EnumDescriptors<MHExtendedEventDescriptor>(
		[&](const MHExtendedEventDescriptor *pDesc) {
			DescList.push_back(pDesc);
		});
	if (DescList.empty())
		return false;

	InsertionSort(DescList,
		[](const MHExtendedEventDescriptor *pDesc1, const MHExtendedEventDescriptor *pDesc2) {
			return pDesc1->GetDescriptorNumber() < pDesc2->GetDescriptorNumber();
		});

	// 項目は記述子を跨がず、項目名の無い項目は無効 (ARIB TR-B39 第1部 第4編 34.17.5)
	for (const MHExtendedEventDescriptor *pDesc : DescList) {
		for (int i = 0; i < pDesc->GetItemCount(); i++) {
			const MHExtendedEventDescriptor::ItemInfo *pItem = pDesc->GetItem(i);
			if (pItem->Description.empty())
				continue;

			EventInfo::ExtendedTextInfo &Text = List->emplace_back();
			DecodeMMTString(pItem->Description, &Text.Description);
			DecodeMMTString(pItem->ItemChar, &Text.Text);
		}
	}

	return true;
}


bool GetMHEventVideoList(const MMTDescriptorBlock *pDescBlock, ReturnArg<EventInfo::VideoInfoList> List)
{
	if (!List)
		return false;

	List->clear();

	if (pDescBlock == nullptr)
		return false;

	pDescBlock->EnumDescriptors<VideoComponentDescriptor>(
		[&](const VideoComponentDescriptor *pDesc) {
			EventInfo::VideoInfo &Info = List->emplace_back();

			Info.StreamContent = STREAM_CONTENT_HEVC;
			Info.ComponentType = GetVideoComponentType(pDesc);
			Info.ComponentTag = MMTComponentTagToComponentTag(pDesc->GetComponentTag());
			Info.LanguageCode = pDesc->GetLanguageCode();
			pDesc->GetText(&Info.Text);
		});

	return true;
}


bool GetMHEventAudioList(const MMTDescriptorBlock *pDescBlock, ReturnArg<EventInfo::AudioInfoList> List)
{
	if (!List)
		return false;

	List->clear();

	if (pDescBlock == nullptr)
		return false;

	pDescBlock->EnumDescriptors<MHAudioComponentDescriptor>(
		[&](const MHAudioComponentDescriptor *pDesc) {
			EventInfo::AudioInfo &Info = List->emplace_back();

			Info.StreamContent = pDesc->GetStreamContent();
			Info.ComponentType = pDesc->GetComponentType();
			Info.ComponentTag = MMTComponentTagToComponentTag(pDesc->GetComponentTag());
			Info.SimulcastGroupTag = pDesc->GetSimulcastGroupTag();
			Info.ESMultiLingualFlag = pDesc->GetESMultiLingualFlag();
			Info.MainComponentFlag = pDesc->GetMainComponentFlag();
			Info.QualityIndicator = pDesc->GetQualityIndicator();
			Info.SamplingRate = pDesc->GetSamplingRate();
			Info.LanguageCode = pDesc->GetLanguageCode();
			Info.LanguageCode2 = pDesc->GetLanguageCode2();
			pDesc->GetText(&Info.Text);
		});

	return true;
}


bool GetMHEventSeriesInfo(const MMTDescriptorBlock *pDescBlock, ReturnArg<EventInfo::SeriesInfo> Info)
{
	if ((pDescBlock == nullptr) || !Info)
		return false;

	const MHSeriesDescriptor *pSeriesDesc = pDescBlock->GetDescriptor<MHSeriesDescriptor>();
	if (pSeriesDesc == nullptr)
		return false;

	Info->SeriesID = pSeriesDesc->GetSeriesID();
	Info->RepeatLabel = pSeriesDesc->GetRepeatLabel();
	Info->ProgramPattern = pSeriesDesc->GetProgramPattern();
	if (!pSeriesDesc->GetExpireDate(&Info->ExpireDate))
		Info->ExpireDate.Reset();
	Info->EpisodeNumber = pSeriesDesc->GetEpisodeNumber();
	Info->LastEpisodeNumber = pSeriesDesc->GetLastEpisodeNumber();
	pSeriesDesc->GetSeriesName(&Info->SeriesName);

	return true;
}


uint8_t MMTComponentTagToComponentTag(uint16_t ComponentTag) noexcept
{
	if (ComponentTag > 0xFF)
		return COMPONENT_TAG_INVALID;
	return static_cast<uint8_t>(ComponentTag);
}


uint8_t MHEITTableIDToEITTableID(uint8_t TableID) noexcept
{
	if (TableID == MMT_TABLE_ID_MH_EIT_PF)
		return EITTable::TABLE_ID_PF_ACTUAL;
	if ((TableID >= MMT_TABLE_ID_MH_EIT_SCHEDULE_FIRST) && (TableID <= MMT_TABLE_ID_MH_EIT_SCHEDULE_LAST))
		return static_cast<uint8_t>(0x50 + (TableID - MMT_TABLE_ID_MH_EIT_SCHEDULE_FIRST));
	return 0;
}


bool GetMHEITSectionInfo(const MHEITTable *pTable, ReturnArg<EPGDatabase::EventSectionInfo> Info)
{
	if ((pTable == nullptr) || !Info)
		return false;

	const uint8_t TableID = MHEITTableIDToEITTableID(pTable->GetTableID());
	if (TableID == 0)
		return false;

	Info->TableID                  = TableID;
	Info->VersionNumber            = pTable->GetVersionNumber();
	Info->SectionNumber            = pTable->GetSectionNumber();
	Info->SegmentLastSectionNumber = pTable->GetSegmentLastSectionNumber();
	Info->LastTableID              = MHEITTableIDToEITTableID(pTable->GetLastTableID());
	Info->NetworkID                = pTable->GetOriginalNetworkID();
	Info->TransportStreamID        = pTable->GetTLVStreamID();
	Info->ServiceID                = pTable->GetServiceID();

	return true;
}




MHEITEventSectionSource::MHEITEventSectionSource(const MHEITTable *pTable) noexcept
	: m_pTable(pTable)
{
}


int MHEITEventSectionSource::GetEventCount() const
{
	return m_pTable->GetEventCount();
}


bool MHEITEventSectionSource::GetEventBasicInfo(int Index, ReturnArg<EventBasicInfo> Info) const
{
	const MHEITTable::EventInfo *pEventInfo = m_pTable->GetEventInfo(Index);
	if ((pEventInfo == nullptr) || !Info)
		return false;

	Info->EventID       = pEventInfo->EventID;
	Info->StartTime     = pEventInfo->StartTime;
	Info->Duration      = pEventInfo->Duration;
	Info->RunningStatus = pEventInfo->RunningStatus;
	Info->FreeCAMode    = pEventInfo->FreeCAMode;

	return true;
}


bool MHEITEventSectionSource::ApplyDescriptors(
	int Index, EventInfo *pEvent,
	ARIBStringDecoder &StringDecoder, ARIBStringDecoder::DecodeFlag DecodeFlags) const
{
	// 文字列は UTF-8 なので StringDecoder は使わない
	const MMTDescriptorBlock *pDescBlock = m_pTable->GetItemDescriptorBlock(Index);
	if (pDescBlock == nullptr)
		return false;

	String Str;

	if (const MHShortEventDescriptor *pShortEvent = pDescBlock->GetDescriptor<MHShortEventDescriptor>();
			pShortEvent != nullptr) {
		if (pShortEvent->GetEventName(&Str))
			pEvent->EventName = Str;
		if (pShortEvent->GetEventDescription(&Str))
			pEvent->EventText = Str;
	}

	EventInfo::ExtendedTextInfoList ExtendedText;
	const bool HasExtended = GetMHEventExtendedTextList(pDescBlock, &ExtendedText);
	if (HasExtended)
		pEvent->ExtendedText = std::move(ExtendedText);

	if (pDescBlock->GetDescriptorByTag(VideoComponentDescriptor::TAG) != nullptr)
		GetMHEventVideoList(pDescBlock, &pEvent->VideoList);

	if (pDescBlock->GetDescriptorByTag(MHAudioComponentDescriptor::TAG) != nullptr)
		GetMHEventAudioList(pDescBlock, &pEvent->AudioList);

	if (const MHContentDescriptor *pContentDesc = pDescBlock->GetDescriptor<MHContentDescriptor>();
			pContentDesc != nullptr)
		SetContentNibble(pContentDesc, pEvent);

	if (pDescBlock->GetDescriptorByTag(MHEventGroupDescriptor::TAG) != nullptr) {
		pEvent->EventGroupList.clear();
		AddEventGroupList(pDescBlock, pEvent);
	}

	return HasExtended;
}


} // namespace LibISDB
