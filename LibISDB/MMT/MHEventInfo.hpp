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
 @file   MHEventInfo.hpp
 @brief  MH-EIT の番組情報
 @author cngu-dtv
*/


#ifndef LIBISDB_MH_EVENT_INFO_H
#define LIBISDB_MH_EVENT_INFO_H


#include "MMTTables.hpp"
#include "../EPG/EventInfo.hpp"
#include "../EPG/EPGDatabase.hpp"


namespace LibISDB
{

	/*
		MH-EIT のイベントを EventInfo に変換する
		TransportStreamID には tlv_stream_id が入る
		映像・音声の stream_content と component_type は ARIB STD-B10 の値にする
	*/
	bool GetMHEventInfo(const MHEITTable *pTable, int Index, ReturnArg<EventInfo> Info);

	bool GetMHEventExtendedTextList(
		const MMTDescriptorBlock *pDescBlock, ReturnArg<EventInfo::ExtendedTextInfoList> List);
	bool GetMHEventVideoList(const MMTDescriptorBlock *pDescBlock, ReturnArg<EventInfo::VideoInfoList> List);
	bool GetMHEventAudioList(const MMTDescriptorBlock *pDescBlock, ReturnArg<EventInfo::AudioInfoList> List);
	bool GetMHEventSeriesInfo(const MMTDescriptorBlock *pDescBlock, ReturnArg<EventInfo::SeriesInfo> Info);

	// 0xFF を超える component_tag は COMPONENT_TAG_INVALID になる
	uint8_t MMTComponentTagToComponentTag(uint16_t ComponentTag) noexcept;

	/*
		MH-EIT の table_id を同じ役割の EIT の table_id にする (該当しなければ 0)
		MH-EIT [p/f] 0x8B は 0x4E、MH-EIT [schedule] 0x8C-0x9B は 0x50-0x5F
		(セグメントとテーブルの構成は EIT と同じ、ARIB TR-B39 第1部 第4編 14.3)
	*/
	uint8_t MHEITTableIDToEITTableID(uint8_t TableID) noexcept;

	// EPGDatabase::UpdateEvents() に渡すセクションの情報 (TransportStreamID には tlv_stream_id が入る)
	bool GetMHEITSectionInfo(const MHEITTable *pTable, ReturnArg<EPGDatabase::EventSectionInfo> Info);

	/** MH-EIT のイベント (EPGDatabase::UpdateEvents() 用) */
	class MHEITEventSectionSource
		: public EPGDatabase::EventSectionSource
	{
	public:
		MHEITEventSectionSource(const MHEITTable *pTable) noexcept;

		int GetEventCount() const override;
		bool GetEventBasicInfo(int Index, ReturnArg<EventBasicInfo> Info) const override;
		bool ApplyDescriptors(
			int Index, EventInfo *pEvent,
			ARIBStringDecoder &StringDecoder, ARIBStringDecoder::DecodeFlag DecodeFlags) const override;

	private:
		const MHEITTable *m_pTable;
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_MH_EVENT_INFO_H
