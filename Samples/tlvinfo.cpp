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
 @file   tlvinfo.cpp
 @brief  TLV ストリームの情報の出力

 TLV ファイルの TLV パケットと MMTP パケットの情報を出力する。

 tlvinfo <filename>

 @author cngu-dtv
*/


#include "../LibISDB/LibISDB.hpp"
#include "../LibISDB/Engine/StreamSourceEngine.hpp"
#include "../LibISDB/Filters/StreamSourceFilter.hpp"
#include "../LibISDB/Filters/StreamPacketParserFilter.hpp"
#include "../LibISDB/Filters/AsyncStreamingFilter.hpp"
#include "../LibISDB/Filters/MMTAnalyzerFilter.hpp"
#include "../LibISDB/Filters/CaptionFilter.hpp"
#include "../LibISDB/Filters/EPGDatabaseFilter.hpp"
#include "../LibISDB/Filters/LogoDownloaderFilter.hpp"
#include "../LibISDB/MMT/MPUAssembler.hpp"
#include "../LibISDB/MMT/TTMLCaption.hpp"
#include "../LibISDB/TS/TSInformation.hpp"
#include "../LibISDB/Base/StandardStream.hpp"
#include "../LibISDB/Utilities/StringFormat.hpp"
#include "../LibISDB/Utilities/StringUtilities.hpp"
#include "../LibISDB/Utilities/Utilities.hpp"
#include <iostream>
#include <iomanip>
#include <map>
#include <cstring>

#if defined(LIBISDB_WINDOWS) && defined(LIBISDB_WCHAR)
#include <fcntl.h>
#include <io.h>
#endif


namespace
{


/** TLV パケットの情報を集計するフィルタ */
class TLVInfoFilter
	: public LibISDB::SingleIOFilter
{
public:
	struct PacketIDInfo {
		unsigned long long Count = 0;
		unsigned long long Bytes = 0;
		unsigned long long Dropped = 0;
		unsigned long long Scrambled = 0;
		unsigned long long RAP = 0;
		unsigned long long Extension = 0;
		std::uint64_t PayloadTypes = 0;
		std::uint8_t FECTypes = 0;
	};

	struct ContextInfo {
		unsigned long long Count = 0;
		unsigned long long FullHeader = 0;
		bool HasPorts = false;
		std::uint16_t SourcePort = 0;
		std::uint16_t DestinationPort = 0;
	};

	struct NTPInfo {
		unsigned long long Count = 0;
		std::uint64_t First = 0;
		std::uint64_t Last = 0;
	};

	const LibISDB::CharType * GetObjectName() const noexcept override { return LIBISDB_STR("TLVInfoFilter"); }

	const std::map<std::uint16_t, PacketIDInfo> & GetPacketIDList() const noexcept { return m_PacketIDList; }
	const std::map<std::uint16_t, ContextInfo> & GetContextList() const noexcept { return m_ContextList; }
	const std::map<std::uint8_t, unsigned long long> & GetSignalingTableList() const noexcept { return m_SignalingTableList; }
	const NTPInfo & GetNTPInfo() const noexcept { return m_NTP; }
	unsigned long long GetOtherPacketCount() const noexcept { return m_OtherPacketCount; }

private:
	bool ProcessData(LibISDB::DataStream *pData) override;
	void ProcessPacket(const LibISDB::TLVPacket *pPacket);

	std::map<std::uint16_t, PacketIDInfo> m_PacketIDList;
	std::map<std::uint16_t, ContextInfo> m_ContextList;
	std::map<std::uint8_t, unsigned long long> m_SignalingTableList;
	NTPInfo m_NTP;
	unsigned long long m_OtherPacketCount = 0;
	LibISDB::MMTPSequenceCounter m_SequenceCounter;
};


bool TLVInfoFilter::ProcessData(LibISDB::DataStream *pData)
{
	if (pData->Is<LibISDB::TLVPacket>()) {
		do {
			ProcessPacket(pData->Get<LibISDB::TLVPacket>());
		} while (pData->Next());
	}

	return true;
}


void TLVInfoFilter::ProcessPacket(const LibISDB::TLVPacket *pPacket)
{
	if (pPacket->IsHeaderCompressed()) {
		ContextInfo &Context = m_ContextList[pPacket->GetContextID()];

		Context.Count++;
		if (pPacket->GetContextHeaderType() == LibISDB::CID_HEADER_TYPE_IPV6_PARTIAL)
			Context.FullHeader++;
		if (pPacket->HasUDPPorts()) {
			Context.HasPorts = true;
			Context.SourcePort = pPacket->GetSourcePort();
			Context.DestinationPort = pPacket->GetDestinationPort();
		}
	}

	switch (pPacket->GetPayloadKind()) {
	case LibISDB::TLVPacket::PayloadKind::MMTP:
		{
			const LibISDB::MMTPHeader &Header = pPacket->GetMMTPHeader();
			PacketIDInfo &Info = m_PacketIDList[Header.PacketID];

			Info.Count++;
			Info.Bytes += pPacket->GetMMTPPayloadSize();
			if (!m_SequenceCounter.Check(Header.PacketID, Header.PacketSequenceNumber))
				Info.Dropped++;
			if (Header.IsScrambled())
				Info.Scrambled++;
			if (Header.RAPFlag)
				Info.RAP++;
			if (Header.ExtensionFlag)
				Info.Extension++;
			Info.PayloadTypes |= 1ULL << Header.PayloadType;
			Info.FECTypes |= 1 << Header.FECType;
		}
		break;

	case LibISDB::TLVPacket::PayloadKind::NTP:
		if (m_NTP.Count == 0)
			m_NTP.First = pPacket->GetNTPTransmitTimestamp();
		m_NTP.Last = pPacket->GetNTPTransmitTimestamp();
		m_NTP.Count++;
		break;

	case LibISDB::TLVPacket::PayloadKind::Signaling:
		if (pPacket->GetPayloadSize() > 0)
			m_SignalingTableList[pPacket->GetPayloadData()[0]]++;
		break;

	case LibISDB::TLVPacket::PayloadKind::Other:
		m_OtherPacketCount++;
		break;

	default:
		break;
	}
}


LibISDB::String FormatNTPTime(std::uint64_t Timestamp)
{
	// NTP タイムスタンプ (1900 年起点、最上位ビットが 0 なら 2036 年起点) を UTC 日時にする
	std::uint64_t Seconds = Timestamp >> 32;
	if ((Seconds & 0x80000000ULL) == 0)
		Seconds += 0x100000000ULL;
	const unsigned int Millisecond = static_cast<unsigned int>(((Timestamp & 0xFFFFFFFFULL) * 1000) >> 32);

	// 1970-01-01 からの日数を年月日にする (Howard Hinnant の civil_from_days)
	constexpr std::uint64_t NTP_UNIX_OFFSET = 2208988800ULL;
	const long long UnixSeconds = static_cast<long long>(Seconds - NTP_UNIX_OFFSET);
	const long long Days = UnixSeconds / 86400;
	const long long TimeOfDay = UnixSeconds % 86400;
	const long long z = Days + 719468;
	const long long era = (z >= 0 ? z : z - 146096) / 146097;
	const long long doe = z - era * 146097;
	const long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	const long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	const long long mp = (5 * doy + 2) / 153;
	const int Day = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
	const int Month = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
	const int Year = static_cast<int>(yoe + era * 400 + (Month <= 2));

	LibISDB::CharType Text[64];
	LibISDB::StringFormat(
		Text, LIBISDB_STR("{}/{:02}/{:02} {:02}:{:02}:{:02}.{:03} UTC"),
		Year, Month, Day,
		static_cast<int>(TimeOfDay / 3600), static_cast<int>((TimeOfDay / 60) % 60), static_cast<int>(TimeOfDay % 60),
		Millisecond);

	return LibISDB::String(Text);
}


const LibISDB::CharType * GetPacketIDText(std::uint16_t PacketID)
{
	switch (PacketID) {
	case LibISDB::MMT_PACKET_ID_PA:      return LIBISDB_STR("PA message");
	case LibISDB::MMT_PACKET_ID_CA:      return LIBISDB_STR("CA message");
	case LibISDB::MMT_PACKET_ID_AL_FEC:  return LIBISDB_STR("AL-FEC message");
	case LibISDB::MMT_PACKET_ID_MH_EIT:  return LIBISDB_STR("MH-EIT");
	case LibISDB::MMT_PACKET_ID_MH_AIT:  return LIBISDB_STR("MH-AIT");
	case LibISDB::MMT_PACKET_ID_MH_BIT:  return LIBISDB_STR("MH-BIT");
	case LibISDB::MMT_PACKET_ID_MH_SDTT: return LIBISDB_STR("MH-SDTT");
	case LibISDB::MMT_PACKET_ID_MH_SDT:  return LIBISDB_STR("MH-SDT");
	case LibISDB::MMT_PACKET_ID_MH_TOT:  return LIBISDB_STR("MH-TOT");
	case LibISDB::MMT_PACKET_ID_MH_CDT:  return LIBISDB_STR("MH-CDT");
	case LibISDB::MMT_PACKET_ID_DATA:    return LIBISDB_STR("Data transmission message");
	case LibISDB::MMT_PACKET_ID_MH_DIT:  return LIBISDB_STR("MH-DIT");
	case LibISDB::MMT_PACKET_ID_MH_SIT:  return LIBISDB_STR("MH-SIT");
	}

	return nullptr;
}


LibISDB::String GetPayloadTypeText(std::uint64_t PayloadTypes)
{
	LibISDB::String Text;

	for (int i = 0; i < 64; i++) {
		if (PayloadTypes & (1ULL << i)) {
			if (!Text.empty())
				Text += LIBISDB_CHAR('/');
			switch (i) {
			case LibISDB::MMTP_PAYLOAD_TYPE_MPU:            Text += LIBISDB_STR("MPU");            break;
			case LibISDB::MMTP_PAYLOAD_TYPE_GENERIC_OBJECT: Text += LIBISDB_STR("Generic object"); break;
			case LibISDB::MMTP_PAYLOAD_TYPE_SIGNALING:      Text += LIBISDB_STR("Signaling");      break;
			case LibISDB::MMTP_PAYLOAD_TYPE_REPAIR_SYMBOL:  Text += LIBISDB_STR("Repair symbol");  break;
			default:
				{
					LibISDB::CharType Type[8];
					LibISDB::StringFormat(Type, LIBISDB_STR("0x{:02X}"), i);
					Text += Type;
				}
				break;
			}
		}
	}

	return Text;
}


const LibISDB::CharType * GetSignalingTableText(std::uint8_t TableID)
{
	switch (TableID) {
	case 0x40: return LIBISDB_STR("TLV-NIT (actual)");
	case 0x41: return LIBISDB_STR("TLV-NIT (other)");
	case 0xFE: return LIBISDB_STR("AMT");
	}

	return LIBISDB_STR("");
}


/** MMT 解析フィルタのイベントを数える */
class AnalyzerEventCounter
	: public LibISDB::MMTAnalyzerFilter::EventListener
{
public:
	unsigned long PLT = 0;
	unsigned long MPT = 0;
	unsigned long MHSDT = 0;
	unsigned long TLVNIT = 0;
	unsigned long MHBIT = 0;
	unsigned long MHEIT = 0;
	unsigned long MHCAT = 0;
	unsigned long MHTOT = 0;

	void OnPLTUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override { PLT++; }
	void OnMPTUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer, std::uint16_t ServiceID) override { MPT++; }
	void OnMHSDTUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override { MHSDT++; }
	void OnTLVNITUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override { TLVNIT++; }
	void OnMHBITUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override { MHBIT++; }
	void OnMHEITUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override { MHEIT++; }
	void OnMHCATUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override { MHCAT++; }
	void OnMHTOTUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override { MHTOT++; }
};


/** 全アセットの MPU を組み立てて集計するフィルタ */
class MediaStatsFilter
	: public LibISDB::SingleIOFilter
	, protected LibISDB::MPUAssembler::Handler
{
public:
	struct AssetStats {
		std::uint32_t AssetType = 0;
		LibISDB::MPUAssembler::MediaType MediaType = LibISDB::MPUAssembler::MediaType::Generic;
		unsigned long long Units = 0;
		unsigned long long RandomAccess = 0;
		unsigned long long Discontinuity = 0;
		unsigned long long BadStart = 0;      // AUD で始まらない映像のアクセスユニット
		unsigned long long DTSOrderError = 0; // DTS が増えていない
		unsigned long long PTSBeforeDTS = 0;
		unsigned long long Subsamples = 0;
		bool HasTimestamp = false;
		bool LastHasTimestamp = false;
		std::int64_t FirstDTS = 0;
		std::int64_t LastDTS = 0;
		std::int64_t MinInterval = 0;
		std::int64_t MaxInterval = 0;
		std::size_t MaxSize = 0;
	};

	MediaStatsFilter()
	{
		Reset();
	}

	const LibISDB::CharType * GetObjectName() const noexcept override { return LIBISDB_STR("MediaStatsFilter"); }

	void Reset() override
	{
		LibISDB::BlockLock Lock(m_FilterLock);

		m_PacketIDMapManager.UnmapAllTargets();
		m_MPTPacketIDList.clear();
		m_Stats.clear();
		m_PacketIDMapManager.MapTarget(
			LibISDB::MMT_PACKET_ID_PA,
			LibISDB::MMTSingleTable::CreateWithHandler<LibISDB::PLTable>(&MediaStatsFilter::OnPLTTable, this));
	}

	void Finish()
	{
		LibISDB::BlockLock Lock(m_FilterLock);

		for (auto &e : m_Stats) {
			LibISDB::MPUAssemblerTarget *pTarget = m_PacketIDMapManager.GetMapTarget<LibISDB::MPUAssemblerTarget>(e.first);
			if (pTarget != nullptr)
				pTarget->GetAssembler().Flush();
		}
	}

	const std::map<std::uint16_t, AssetStats> & GetStats() const noexcept { return m_Stats; }

	const LibISDB::MPUAssembler * GetAssembler(std::uint16_t PacketID) const
	{
		const LibISDB::MPUAssemblerTarget *pTarget = m_PacketIDMapManager.GetMapTarget<LibISDB::MPUAssemblerTarget>(PacketID);
		return (pTarget != nullptr) ? &pTarget->GetAssembler() : nullptr;
	}

private:
	bool ProcessData(LibISDB::DataStream *pData) override
	{
		if (pData->Is<LibISDB::TLVPacket>())
			m_PacketIDMapManager.StorePacketStream(pData);
		return true;
	}

	void OnAccessUnit(const LibISDB::MPUAssembler *pAssembler, const LibISDB::MPUAssembler::AccessUnitInfo &Info) override
	{
		AssetStats &Stats = m_Stats[Info.PacketID];

		Stats.Units++;
		if (Info.RandomAccess)
			Stats.RandomAccess++;
		if (Info.Discontinuity)
			Stats.Discontinuity++;
		if (Info.DataSize > Stats.MaxSize)
			Stats.MaxSize = Info.DataSize;
		if (pAssembler->GetMediaType() == LibISDB::MPUAssembler::MediaType::Video) {
			static const std::uint8_t StartCode[] = {0x00, 0x00, 0x00, 0x01};
			if ((Info.DataSize < sizeof(StartCode) + 2)
					|| (std::memcmp(Info.pData, StartCode, sizeof(StartCode)) != 0)
					|| (((Info.pData[4] >> 1) & 0x3F) != LibISDB::HEVC_NAL_UNIT_TYPE_AUD))
				Stats.BadStart++;
		}
		if (Info.HasTimestamp) {
			if (Info.PTS < Info.DTS)
				Stats.PTSBeforeDTS++;
			if (Stats.HasTimestamp) {
				const std::int64_t Interval = Info.DTS - Stats.LastDTS;
				if (Interval <= 0)
					Stats.DTSOrderError++;
				// 直前のアクセスユニットとの間隔だけを数える
				if (Stats.LastHasTimestamp && !Info.Discontinuity) {
					if ((Stats.MinInterval == 0) || (Interval < Stats.MinInterval))
						Stats.MinInterval = Interval;
					if (Interval > Stats.MaxInterval)
						Stats.MaxInterval = Interval;
				}
			} else {
				Stats.HasTimestamp = true;
				Stats.FirstDTS = Info.DTS;
			}
			Stats.LastDTS = Info.DTS;
		}
		Stats.LastHasTimestamp = Info.HasTimestamp;
	}

	void OnCaptionUnit(const LibISDB::MPUAssembler *pAssembler, const LibISDB::MPUAssembler::CaptionUnitInfo &Info) override
	{
		AssetStats &Stats = m_Stats[Info.PacketID];

		Stats.Units++;
		Stats.Subsamples += Info.SubsampleList.size();
		if (Info.Discontinuity)
			Stats.Discontinuity++;
		if (Info.HasPresentationTime) {
			const std::int64_t Time = LibISDB::MPUTimestampTable::NTPTimeTo90kHz(Info.PresentationTime);
			if (!Stats.HasTimestamp) {
				Stats.HasTimestamp = true;
				Stats.FirstDTS = Time;
			}
			Stats.LastDTS = Time;
		}
	}

	void OnPLTTable(const LibISDB::MMTSingleTable *pTable)
	{
		const LibISDB::PLTable *pPLTable = dynamic_cast<const LibISDB::PLTable *>(pTable);
		if (pPLTable == nullptr)
			return;

		for (std::uint16_t PacketID : m_MPTPacketIDList)
			m_PacketIDMapManager.UnmapTarget(PacketID);
		m_MPTPacketIDList.clear();

		for (int i = 0; i < pPLTable->GetPackageCount(); i++) {
			const std::uint16_t PacketID = pPLTable->GetMPTPacketID(i);
			if ((PacketID != LibISDB::MMT_PACKET_ID_INVALID) && (PacketID != LibISDB::MMT_PACKET_ID_PA)) {
				m_PacketIDMapManager.MapTarget(
					PacketID,
					LibISDB::MMTSingleTable::CreateWithHandler<LibISDB::MPTable>(&MediaStatsFilter::OnMPTTable, this));
				m_MPTPacketIDList.push_back(PacketID);
			}
		}
	}

	void OnMPTTable(const LibISDB::MMTSingleTable *pTable)
	{
		const LibISDB::MPTable *pMPTable = dynamic_cast<const LibISDB::MPTable *>(pTable);
		if (pMPTable == nullptr)
			return;

		for (int i = 0; i < pMPTable->GetAssetCount(); i++) {
			const std::uint16_t PacketID = pMPTable->GetAssetPacketID(i);
			if ((PacketID == LibISDB::MMT_PACKET_ID_INVALID) || (PacketID == LibISDB::MMT_PACKET_ID_PA))
				continue;

			LibISDB::MPUAssemblerTarget *pTarget = m_PacketIDMapManager.GetMapTarget<LibISDB::MPUAssemblerTarget>(PacketID);
			if (pTarget == nullptr) {
				if (m_PacketIDMapManager.GetMapTarget(PacketID) != nullptr)
					continue;
				const LibISDB::MPUAssembler::MediaType Type =
					LibISDB::MPUAssembler::GetMediaTypeFromAssetType(pMPTable->GetAssetType(i));
				pTarget = new LibISDB::MPUAssemblerTarget(Type, this);
				m_PacketIDMapManager.MapTarget(PacketID, pTarget);
				AssetStats &Stats = m_Stats[PacketID];
				Stats.AssetType = pMPTable->GetAssetType(i);
				Stats.MediaType = Type;
			}

			const LibISDB::MPTable::AssetInfo *pAssetInfo = pMPTable->GetAssetInfo(i);
			pTarget->GetAssembler().UpdateTimestamps(
				pMPTable->GetAssetDescriptorBlock(i),
				pAssetInfo->AssetTimescaleFlag ? pAssetInfo->AssetTimescale : 0);
		}
	}

	LibISDB::MMTPacketIDMapManager m_PacketIDMapManager;
	std::vector<std::uint16_t> m_MPTPacketIDList;
	std::map<std::uint16_t, AssetStats> m_Stats;
};


/** 字幕フィルタの出力を記録する */
class CaptionRecorder
	: public LibISDB::CaptionFilter::Handler
	, public LibISDB::MMTAnalyzerFilter::EventListener
{
public:
	struct CaptionInfo {
		std::uint16_t PacketID;
		std::uint32_t MPUSequenceNumber;
		bool HasPresentationTime;
		std::uint64_t PresentationTime;
		bool HasSubtitleInfo;
		LibISDB::MHDataComponentDescriptor::AribSubtitleInfo SubtitleInfo;
		std::size_t DocumentSize;
		std::size_t SubsampleCount;
		bool Decoded;
		LibISDB::TTMLCaptionDecoder::ParagraphList Paragraphs;
	};

	unsigned long LanguageUpdateCount = 0;
	std::uint32_t LanguageCode = 0;
	std::vector<CaptionInfo> CaptionList;

	CaptionRecorder(LibISDB::CaptionFilter *pFilter) : m_pFilter(pFilter) {}

private:
	void OnPLTUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override
	{
		if (pAnalyzer->GetServiceCount() > 0)
			m_pFilter->SetTargetStream(pAnalyzer->GetServiceID(0));
	}

	void OnLanguageUpdate(LibISDB::CaptionFilter *pFilter, LibISDB::CaptionParser *pParser) override
	{
		LanguageUpdateCount++;
		LanguageCode = pFilter->GetLanguageCode(0);
	}

	void OnTTMLCaption(LibISDB::CaptionFilter *pFilter, const LibISDB::CaptionFilter::TTMLCaptionInfo &Info) override
	{
		CaptionInfo &Caption = CaptionList.emplace_back();

		Caption.PacketID = Info.PacketID;
		Caption.MPUSequenceNumber = Info.MPUSequenceNumber;
		Caption.HasPresentationTime = Info.HasPresentationTime;
		Caption.PresentationTime = Info.PresentationTime;
		Caption.HasSubtitleInfo = Info.HasSubtitleInfo;
		Caption.SubtitleInfo = Info.SubtitleInfo;
		Caption.DocumentSize = Info.DocumentSize;
		Caption.SubsampleCount = Info.pSubsampleList->size();
		Caption.Decoded =
			(Info.pDocument != nullptr)
			&& LibISDB::TTMLCaptionDecoder::Decode(Info.pDocument, Info.DocumentSize, &Caption.Paragraphs);
	}

	LibISDB::CaptionFilter *m_pFilter;
};


/** ロゴ取得フィルタの出力を記録する */
class LogoRecorder
	: public LibISDB::LogoDownloaderFilter::LogoHandler
{
public:
	struct LogoInfo {
		std::uint16_t NetworkID;
		std::vector<LibISDB::LogoDownloaderFilter::LogoService> ServiceList;
		std::uint16_t LogoID;
		std::uint16_t LogoVersion;
		std::uint8_t LogoType;
		std::uint16_t DataSize;
		bool IsPNG;
		std::uint32_t Width;
		std::uint32_t Height;
		LibISDB::DateTime Time;
	};

	std::vector<LogoInfo> LogoList;

private:
	void OnLogoDownloaded(const LibISDB::LogoDownloaderFilter::LogoData &Data) override
	{
		static const std::uint8_t PNGSignature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

		LogoInfo &Info = LogoList.emplace_back();

		Info.NetworkID = Data.NetworkID;
		Info.ServiceList = Data.ServiceList;
		Info.LogoID = Data.LogoID;
		Info.LogoVersion = Data.LogoVersion;
		Info.LogoType = Data.LogoType;
		Info.DataSize = Data.DataSize;
		Info.IsPNG = (Data.DataSize >= 24) && (std::memcmp(Data.pData, PNGSignature, 8) == 0);
		Info.Width = Info.IsPNG ? LibISDB::Load32(&Data.pData[16]) : 0;
		Info.Height = Info.IsPNG ? LibISDB::Load32(&Data.pData[20]) : 0;
		Info.Time = Data.Time;
	}
};


LibISDB::String FormatDateTime(const LibISDB::DateTime &Time)
{
	if (!Time.IsValid())
		return LIBISDB_STR("-");

	LibISDB::CharType Text[64];
	LibISDB::StringFormat(
		Text, LIBISDB_STR("{}/{:02}/{:02} {:02}:{:02}:{:02}"),
		Time.Year, Time.Month, Time.Day, Time.Hour, Time.Minute, Time.Second);

	return LibISDB::String(Text);
}


LibISDB::String FormatAssetType(std::uint32_t AssetType)
{
	LibISDB::String Text;

	for (int i = 3; i >= 0; i--) {
		const char c = static_cast<char>((AssetType >> (i * 8)) & 0xFF);
		Text += ((c >= 0x20) && (c < 0x7F)) ? static_cast<LibISDB::CharType>(c) : LIBISDB_CHAR('?');
	}

	return Text;
}


template<typename TStream> void PrintHex(TStream &Out, unsigned int Value, int Digits)
{
	Out << LIBISDB_STR("0x") << std::hex << std::uppercase << std::setfill(LIBISDB_CHAR('0')) << std::setw(Digits) << Value
		<< std::dec << std::setfill(LIBISDB_CHAR(' '));
}


template<typename TStream> void PrintEvent(TStream &Out, const LibISDB::EventInfo &Event)
{
	Out << FormatDateTime(Event.StartTime)
		<< LIBISDB_STR(" (") << Event.Duration / 60 << LIBISDB_STR(" min) event_id ");
	PrintHex(Out, Event.EventID, 4);
	Out << LIBISDB_STR(" \"") << Event.EventName << LIBISDB_STR("\"") << std::endl;
	if (!Event.EventText.empty())
		Out << LIBISDB_STR("      Text     : ") << Event.EventText << std::endl;
	for (const auto &Video : Event.VideoList) {
		const LibISDB::CharType *pText = LibISDB::GetComponentTypeText_ja(Video.StreamContent, Video.ComponentType);
		Out << LIBISDB_STR("      Video    : component_tag ");
		PrintHex(Out, Video.ComponentTag, 2);
		Out << LIBISDB_STR(" ") << ((pText != nullptr) ? pText : LIBISDB_STR("?")) << std::endl;
	}
	for (const auto &Audio : Event.AudioList) {
		const LibISDB::CharType *pText = LibISDB::GetComponentTypeText_ja(Audio.StreamContent, Audio.ComponentType);
		Out << LIBISDB_STR("      Audio    : component_tag ");
		PrintHex(Out, Audio.ComponentTag, 2);
		Out << LIBISDB_STR(" ") << ((pText != nullptr) ? pText : LIBISDB_STR("?"))
			<< (Audio.MainComponentFlag ? LIBISDB_STR(" (main)") : LIBISDB_STR(""))
			<< LIBISDB_STR(" ") << Audio.Text << std::endl;
	}
	if (Event.ContentNibble.NibbleCount > 0) {
		Out << LIBISDB_STR("      Genre    :");
		for (int i = 0; i < Event.ContentNibble.NibbleCount; i++) {
			Out << LIBISDB_STR(" ") << static_cast<int>(Event.ContentNibble.NibbleList[i].ContentNibbleLevel1)
				<< LIBISDB_STR("-") << static_cast<int>(Event.ContentNibble.NibbleList[i].ContentNibbleLevel2);
		}
		Out << std::endl;
	}
	for (const auto &Text : Event.ExtendedText) {
		Out << LIBISDB_STR("      [") << Text.Description << LIBISDB_STR("] ")
			<< Text.Text.length() << LIBISDB_STR(" chars") << std::endl;
	}
}


template<typename TStream> void PrintAnalyzerInfo(
	TStream &Out, const LibISDB::MMTAnalyzerFilter *pAnalyzer, const AnalyzerEventCounter &Counter)
{
	LibISDB::String Name;

	Out << LIBISDB_STR("TLV stream      : tlv_stream_id ");
	PrintHex(Out, pAnalyzer->GetTLVStreamID(), 4);
	Out << LIBISDB_STR(" / network_id ");
	PrintHex(Out, pAnalyzer->GetNetworkID(), 4);
	if (pAnalyzer->GetNetworkName(&Name) && !Name.empty())
		Out << LIBISDB_STR(" \"") << Name << LIBISDB_STR("\"");
	Out << std::endl;

	LibISDB::DateTime Time;
	if (pAnalyzer->GetTOTTime(&Time)) {
		Out << LIBISDB_STR("MH-TOT          : ") << FormatDateTime(Time);
		bool Interpolated;
		if (pAnalyzer->GetInterpolatedTOTTime(&Time, &Interpolated) && Interpolated)
			Out << LIBISDB_STR(" (interpolated ") << FormatDateTime(Time) << LIBISDB_STR(")");
		Out << std::endl;
	}

	LibISDB::MMTAnalyzerFilter::ServiceList ServiceList;
	pAnalyzer->GetServiceList(&ServiceList);
	for (size_t i = 0; i < ServiceList.size(); i++) {
		const LibISDB::MMTAnalyzerFilter::ServiceInfo &Service = ServiceList[i];

		Out << std::endl << LIBISDB_STR("Service ");
		PrintHex(Out, Service.ServiceID, 4);
		Out << LIBISDB_STR(" : \"") << Service.ServiceName << LIBISDB_STR("\" (") << Service.ProviderName << LIBISDB_STR(")");
		Out << LIBISDB_STR(" type ");
		PrintHex(Out, Service.ServiceType, 2);
		Out << LIBISDB_STR(" / MPT ");
		PrintHex(Out, Service.MPTPacketID, 4);
		Out << LIBISDB_STR(" / logo_id ");
		PrintHex(Out, Service.LogoID, 3);
		Out << LIBISDB_STR(" / remote control key ") << static_cast<int>(pAnalyzer->GetRemoteControlKeyID(Service.ServiceID))
			<< std::endl;

		for (const auto &ECM : Service.ECMList) {
			Out << LIBISDB_STR("    ECM          : CA_system_ID ");
			PrintHex(Out, ECM.CASystemID, 4);
			Out << LIBISDB_STR(" packet_id ");
			PrintHex(Out, ECM.PacketID, 4);
			Out << std::endl;
		}

		for (const auto &App : Service.ApplicationServiceList) {
			Out << LIBISDB_STR("    App service  : format ");
			PrintHex(Out, App.ApplicationFormat, 1);
			Out << LIBISDB_STR(" resolution ");
			PrintHex(Out, App.DocumentResolution, 1);
			Out << LIBISDB_STR(" default_AIT ") << (App.DefaultAITFlag ? 1 : 0) << LIBISDB_STR(" AIT ");
			PrintHex(Out, App.AITPacketID, 4);
			Out << LIBISDB_STR(" DT ");
			PrintHex(Out, App.DTMessagePacketID, 4);
			Out << std::endl;
			for (const auto &EMT : App.EMTList) {
				Out << LIBISDB_STR("      EMT        : tag ");
				PrintHex(Out, EMT.EMTTag, 2);
				Out << LIBISDB_STR(" packet_id ");
				PrintHex(Out, EMT.PacketID, 4);
				Out << std::endl;
			}
		}

		const struct {
			const LibISDB::CharType *pName;
			const LibISDB::MMTAnalyzerFilter::AssetInfoList &List;
		} AssetGroups[] = {
			{LIBISDB_STR("Video"),       Service.VideoAssetList},
			{LIBISDB_STR("Audio"),       Service.AudioAssetList},
			{LIBISDB_STR("Caption"),     Service.CaptionAssetList},
			{LIBISDB_STR("Application"), Service.ApplicationAssetList},
			{LIBISDB_STR("Other"),       Service.OtherAssetList},
		};
		for (const auto &Group : AssetGroups) {
			for (const auto &Asset : Group.List) {
				Out << LIBISDB_STR("    ") << std::left << std::setw(12) << Group.pName << std::right << LIBISDB_STR(" : packet_id ");
				PrintHex(Out, Asset.PacketID, 4);
				Out << LIBISDB_STR(" ") << FormatAssetType(Asset.AssetType) << LIBISDB_STR(" component_tag ");
				PrintHex(Out, Asset.ComponentTag, 4);
				Out << std::endl;
			}
		}

		for (int Next = 0; Next <= 1; Next++) {
			LibISDB::EventInfo Event;
			if (pAnalyzer->GetEventInfo(static_cast<int>(i), &Event, Next != 0)) {
				Out << (Next ? LIBISDB_STR("    Following    : ") : LIBISDB_STR("    Present      : "));
				PrintEvent(Out, Event);
			}
		}
	}

	LibISDB::MMTAnalyzerFilter::SDTStreamList SDTStreamList;
	if (pAnalyzer->GetSDTStreamList(&SDTStreamList) && !SDTStreamList.empty()) {
		Out << std::endl << LIBISDB_STR("MH-SDT streams :") << std::endl;
		for (const auto &Stream : SDTStreamList) {
			for (const auto &Service : Stream.ServiceList) {
				Out << LIBISDB_STR("    ");
				PrintHex(Out, Stream.TLVStreamID, 4);
				Out << LIBISDB_STR(" / ");
				PrintHex(Out, Service.ServiceID, 4);
				Out << LIBISDB_STR(" \"") << Service.ServiceName << LIBISDB_STR("\"") << std::endl;
			}
		}
	}

	LibISDB::MMTAnalyzerFilter::SatelliteDeliverySystemList SatelliteList;
	if (pAnalyzer->GetSatelliteDeliverySystemList(&SatelliteList)) {
		Out << std::endl << LIBISDB_STR("TLV-NIT streams :") << std::endl;
		for (const auto &e : SatelliteList) {
			Out << LIBISDB_STR("    ");
			PrintHex(Out, e.TLVStreamID, 4);
			Out << LIBISDB_STR(" : frequency ") << e.Frequency
				<< LIBISDB_STR(" polarization ") << static_cast<int>(e.Polarization)
				<< LIBISDB_STR(" modulation ") << static_cast<int>(e.Modulation) << std::endl;
		}
	}

	LibISDB::MMTAnalyzerFilter::BITNetworkList BITList;
	if (pAnalyzer->GetBITNetworkList(&BITList)) {
		Out << std::endl << LIBISDB_STR("MH-BIT broadcasters :") << std::endl;
		for (const auto &Network : BITList) {
			for (const auto &Broadcaster : Network.BroadcasterList) {
				Out << LIBISDB_STR("    ");
				PrintHex(Out, Broadcaster.BroadcasterID, 2);
				Out << LIBISDB_STR(" \"") << Broadcaster.BroadcasterName << LIBISDB_STR("\" services ")
					<< Broadcaster.ServiceList.size() << std::endl;
			}
		}
	}

	LibISDB::MMTAnalyzerFilter::EMMPacketIDList EMMList;
	if (pAnalyzer->GetEMMPacketIDList(&EMMList) && !EMMList.empty()) {
		Out << std::endl << LIBISDB_STR("EMM packet_id :");
		for (std::uint16_t PacketID : EMMList) {
			Out << LIBISDB_STR(" ");
			PrintHex(Out, PacketID, 4);
		}
		Out << std::endl;
	}

	Out << std::endl
		<< LIBISDB_STR("Events : PLT ") << Counter.PLT
		<< LIBISDB_STR(" / MPT ") << Counter.MPT
		<< LIBISDB_STR(" / MH-SDT ") << Counter.MHSDT
		<< LIBISDB_STR(" / TLV-NIT ") << Counter.TLVNIT
		<< LIBISDB_STR(" / MH-BIT ") << Counter.MHBIT
		<< LIBISDB_STR(" / MH-EIT ") << Counter.MHEIT
		<< LIBISDB_STR(" / CAT ") << Counter.MHCAT
		<< LIBISDB_STR(" / MH-TOT ") << Counter.MHTOT
		<< std::endl;
}


LibISDB::String Format90kHzTime(std::int64_t Time)
{
	const std::uint64_t Seconds = static_cast<std::uint64_t>(Time / 90000);
	const std::uint64_t Fraction = (static_cast<std::uint64_t>(Time % 90000) << 32) / 90000;
	return FormatNTPTime((Seconds << 32) | Fraction);
}


const LibISDB::CharType * GetMediaTypeText(LibISDB::MPUAssembler::MediaType Type)
{
	switch (Type) {
	case LibISDB::MPUAssembler::MediaType::Video:   return LIBISDB_STR("Video");
	case LibISDB::MPUAssembler::MediaType::Audio:   return LIBISDB_STR("Audio");
	case LibISDB::MPUAssembler::MediaType::Caption: return LIBISDB_STR("Caption");
	case LibISDB::MPUAssembler::MediaType::Generic: return LIBISDB_STR("Generic");
	}
	return LIBISDB_STR("");
}


template<typename TStream> void PrintMediaStats(TStream &Out, const MediaStatsFilter *pFilter)
{
	Out << LIBISDB_STR("MPU assembly :") << std::endl;

	for (const auto &e : pFilter->GetStats()) {
		const MediaStatsFilter::AssetStats &Stats = e.second;

		Out << LIBISDB_STR("  ");
		PrintHex(Out, e.first, 4);
		Out << LIBISDB_STR(" ") << FormatAssetType(Stats.AssetType)
			<< LIBISDB_STR(" ") << GetMediaTypeText(Stats.MediaType)
			<< LIBISDB_STR(" : units ") << Stats.Units;
		if (Stats.MediaType == LibISDB::MPUAssembler::MediaType::Caption)
			Out << LIBISDB_STR(" / subsamples ") << Stats.Subsamples;
		else
			Out << LIBISDB_STR(" / RAP ") << Stats.RandomAccess
				<< LIBISDB_STR(" / max ") << Stats.MaxSize << LIBISDB_STR(" bytes");
		Out << LIBISDB_STR(" / discontinuity ") << Stats.Discontinuity;
		if (Stats.MediaType == LibISDB::MPUAssembler::MediaType::Video)
			Out << LIBISDB_STR(" / not AUD ") << Stats.BadStart;
		Out << std::endl;

		const LibISDB::MPUAssembler *pAssembler = pFilter->GetAssembler(e.first);
		if (pAssembler != nullptr) {
			const LibISDB::MPUAssembler::StatisticsInfo &Info = pAssembler->GetStatistics();
			Out << LIBISDB_STR("      packets ") << Info.PacketCount
				<< LIBISDB_STR(" / scrambled ") << Info.ScrambledCount
				<< LIBISDB_STR(" / loss ") << Info.LossCount
				<< LIBISDB_STR(" / output ") << Info.OutputCount
				<< LIBISDB_STR(" / dropped ") << Info.DroppedCount
				<< LIBISDB_STR(" / no timestamp ") << Info.NoTimestampCount
				<< std::endl;
		}

		if (Stats.HasTimestamp) {
			Out << LIBISDB_STR("      ")
				<< ((Stats.MediaType == LibISDB::MPUAssembler::MediaType::Caption) ? LIBISDB_STR("MPU time ") : LIBISDB_STR("DTS "))
				<< Format90kHzTime(Stats.FirstDTS) << LIBISDB_STR(" - ") << Format90kHzTime(Stats.LastDTS);
			if (Stats.MediaType != LibISDB::MPUAssembler::MediaType::Caption)
				Out << LIBISDB_STR(" / interval ") << Stats.MinInterval << LIBISDB_STR("-") << Stats.MaxInterval
					<< LIBISDB_STR(" / order error ") << Stats.DTSOrderError
					<< LIBISDB_STR(" / PTS<DTS ") << Stats.PTSBeforeDTS;
			Out << std::endl;
		}
	}
}


template<typename TStream> void PrintCaptions(TStream &Out, const CaptionRecorder &Recorder, std::size_t MaxCount)
{
	unsigned long Decoded = 0, Clear = 0;
	for (const CaptionRecorder::CaptionInfo &Caption : Recorder.CaptionList) {
		if (Caption.Decoded) {
			Decoded++;
			if (Caption.Paragraphs.empty())
				Clear++;
		}
	}

	Out << LIBISDB_STR("Captions : ") << Recorder.CaptionList.size()
		<< LIBISDB_STR(" documents / decoded ") << Decoded
		<< LIBISDB_STR(" / clear ") << Clear
		<< LIBISDB_STR(" / language updates ") << Recorder.LanguageUpdateCount
		<< LIBISDB_STR(" / language ");
	for (int i = 2; i >= 0; i--) {
		const char c = static_cast<char>((Recorder.LanguageCode >> (i * 8)) & 0xFF);
		Out << (((c >= 0x20) && (c < 0x7F)) ? static_cast<LibISDB::CharType>(c) : LIBISDB_CHAR('?'));
	}
	Out << std::endl;

	if (!Recorder.CaptionList.empty()) {
		const CaptionRecorder::CaptionInfo &First = Recorder.CaptionList.front();
		if (First.HasSubtitleInfo) {
			const LibISDB::MHDataComponentDescriptor::AribSubtitleInfo &Info = First.SubtitleInfo;
			Out << LIBISDB_STR("  subtitle_tag ") << static_cast<int>(Info.SubtitleTag)
				<< LIBISDB_STR(" / type ") << static_cast<int>(Info.Type)
				<< LIBISDB_STR(" / OPM ") << static_cast<int>(Info.OPM)
				<< LIBISDB_STR(" / TMD ") << static_cast<int>(Info.TMD)
				<< LIBISDB_STR(" / DMF ") << static_cast<int>(Info.DMF)
				<< LIBISDB_STR(" / resolution ") << static_cast<int>(Info.Resolution)
				<< LIBISDB_STR(" / compression ") << static_cast<int>(Info.CompressionType);
			if (Info.TMD == LibISDB::MHDataComponentDescriptor::TMD_REFERENCE_START_TIME)
				Out << LIBISDB_STR(" / reference_start_time ") << FormatNTPTime(Info.ReferenceStartTime);
			Out << std::endl;
		}
	}

	std::size_t Count = 0;
	for (const CaptionRecorder::CaptionInfo &Caption : Recorder.CaptionList) {
		if (Count++ >= MaxCount)
			break;
		Out << LIBISDB_STR("  MPU ") << Caption.MPUSequenceNumber << LIBISDB_STR(" ");
		if (Caption.HasPresentationTime)
			Out << FormatNTPTime(Caption.PresentationTime);
		Out << LIBISDB_STR(" (") << Caption.DocumentSize << LIBISDB_STR(" bytes, ")
			<< Caption.SubsampleCount << LIBISDB_STR(" subsamples)");
		if (!Caption.Decoded) {
			Out << LIBISDB_STR(" : decode error") << std::endl;
		} else if (Caption.Paragraphs.empty()) {
			Out << LIBISDB_STR(" : (clear)") << std::endl;
		} else {
			Out << std::endl;
			for (const LibISDB::TTMLCaptionDecoder::ParagraphInfo &Paragraph : Caption.Paragraphs) {
				LibISDB::String Text = Paragraph.Text;
				for (std::size_t Pos; (Pos = Text.find(LIBISDB_STR(LIBISDB_NEWLINE))) != LibISDB::String::npos;)
					Text.replace(Pos, LibISDB::StringLength(LIBISDB_STR(LIBISDB_NEWLINE)), LIBISDB_STR(" / "));
				Out << LIBISDB_STR("    [") << Paragraph.Begin << LIBISDB_STR(" ms ") << Paragraph.Region
					<< LIBISDB_STR("] ") << Text << std::endl;
			}
		}
	}
}


template<typename TStream> void PrintLogos(TStream &Out, const LogoRecorder &Logos)
{
	Out << LIBISDB_STR("Logo : ") << Logos.LogoList.size() << std::endl;

	for (const LogoRecorder::LogoInfo &Logo : Logos.LogoList) {
		Out << LIBISDB_STR("  network ");
		PrintHex(Out, Logo.NetworkID, 4);
		Out << LIBISDB_STR(" / logo_id ");
		PrintHex(Out, Logo.LogoID, 3);
		Out << LIBISDB_STR(" / version ");
		PrintHex(Out, Logo.LogoVersion, 3);
		Out << LIBISDB_STR(" / type ");
		PrintHex(Out, Logo.LogoType, 2);
		Out << LIBISDB_STR(" : ") << Logo.DataSize << LIBISDB_STR(" bytes");
		if (Logo.IsPNG)
			Out << LIBISDB_STR(" PNG ") << Logo.Width << LIBISDB_STR("x") << Logo.Height;
		Out << LIBISDB_STR(" / ") << FormatDateTime(Logo.Time) << LIBISDB_STR(" / services");
		for (const auto &Service : Logo.ServiceList) {
			Out << LIBISDB_STR(" ");
			PrintHex(Out, Service.ServiceID, 4);
			Out << LIBISDB_STR(" (TLV stream ");
			PrintHex(Out, Service.TransportStreamID, 4);
			Out << LIBISDB_STR(")");
		}
		Out << std::endl;
	}
}


template<typename TStream> void PrintEPG(TStream &Out, LibISDB::EPGDatabase *pDatabase)
{
	LibISDB::EPGDatabase::ServiceList ServiceList;
	pDatabase->GetServiceList(&ServiceList);

	Out << LIBISDB_STR("EPG : ") << ServiceList.size() << LIBISDB_STR(" services") << std::endl;

	for (const LibISDB::EPGDatabase::ServiceInfo &Service : ServiceList) {
		LibISDB::EPGDatabase::EventList List;
		pDatabase->GetEventListSortedByTime(Service.NetworkID, Service.TransportStreamID, Service.ServiceID, &List);
		std::size_t Extended = 0;
		for (const LibISDB::EventInfo &Event : List) {
			if (!Event.ExtendedText.empty())
				Extended++;
		}

		Out << LIBISDB_STR("  Service ");
		PrintHex(Out, Service.ServiceID, 4);
		Out << LIBISDB_STR(" (network ");
		PrintHex(Out, Service.NetworkID, 4);
		Out << LIBISDB_STR(" / TLV stream ");
		PrintHex(Out, Service.TransportStreamID, 4);
		Out << LIBISDB_STR(") : ") << List.size() << LIBISDB_STR(" events / extended text ") << Extended
			<< LIBISDB_STR(" / schedule basic ")
			<< (pDatabase->IsScheduleComplete(Service.NetworkID, Service.TransportStreamID, Service.ServiceID, false) ?
				LIBISDB_STR("complete") :
				pDatabase->HasSchedule(Service.NetworkID, Service.TransportStreamID, Service.ServiceID, false) ?
				LIBISDB_STR("incomplete") : LIBISDB_STR("none"))
			<< LIBISDB_STR(" / extended ")
			<< (pDatabase->IsScheduleComplete(Service.NetworkID, Service.TransportStreamID, Service.ServiceID, true) ?
				LIBISDB_STR("complete") :
				pDatabase->HasSchedule(Service.NetworkID, Service.TransportStreamID, Service.ServiceID, true) ?
				LIBISDB_STR("incomplete") : LIBISDB_STR("none"))
			<< std::endl;

		if (!List.empty()) {
			Out << LIBISDB_STR("    First : ");
			PrintEvent(Out, List.front());
			Out << LIBISDB_STR("    Last  : ");
			PrintEvent(Out, List.back());
		}
	}
}


}


#if defined(LIBISDB_WCHAR)
int wmain(int argc, wchar_t **argv)
#else
int main(int argc, char **argv)
#endif
{
#if defined(LIBISDB_WCHAR)
	auto &ErrOut = std::wcerr;
#else
	auto &ErrOut = std::cerr;
#endif

	if (argc < 2) {
		ErrOut << LIBISDB_STR("Need filename.") << std::endl;
		return 1;
	}

#if defined(LIBISDB_WINDOWS) && defined(LIBISDB_WCHAR)
	::_setmode(::_fileno(stdout), _O_U8TEXT);
#endif

	LibISDB::StreamSourceFilter *pSource = new LibISDB::StreamSourceFilter;
	LibISDB::StreamPacketParserFilter *pParser = new LibISDB::StreamPacketParserFilter;
	TLVInfoFilter *pInfo = new TLVInfoFilter;
	LibISDB::MMTAnalyzerFilter *pAnalyzer = new LibISDB::MMTAnalyzerFilter;
	AnalyzerEventCounter EventCounter;
	pAnalyzer->AddEventListener(&EventCounter);
	MediaStatsFilter *pMediaStats = new MediaStatsFilter;
	LibISDB::CaptionFilter *pCaption = new LibISDB::CaptionFilter;
	CaptionRecorder Captions(pCaption);
	pCaption->SetCaptionHandler(&Captions);
	pAnalyzer->AddEventListener(&Captions);
	LibISDB::EPGDatabase EPGDatabase;
	EPGDatabase.SetNoPastEvents(false);
	LibISDB::EPGDatabaseFilter *pEPGDatabaseFilter = new LibISDB::EPGDatabaseFilter;
	pEPGDatabaseFilter->SetEPGDatabase(&EPGDatabase);
	LibISDB::LogoDownloaderFilter *pLogoDownloader = new LibISDB::LogoDownloaderFilter;
	LogoRecorder Logos;
	pLogoDownloader->SetLogoHandler(&Logos);

	pSource->SetSourceMode(LibISDB::SourceFilter::SourceMode::Pull);
	LibISDB::AsyncStreamingFilter *pAsyncStreaming = new LibISDB::AsyncStreamingFilter;
	pAsyncStreaming->SetSourceFilter(pSource);
	pAsyncStreaming->CreateBuffer(pAsyncStreaming->GetOutputBufferSize(), 3, 3);

	LibISDB::StreamSourceEngine Engine;

	Engine.BuildEngine({
		pSource,
		pAsyncStreaming,
		pParser,
		pInfo,
		pAnalyzer,
		pMediaStats,
		pCaption,
		pEPGDatabaseFilter,
		pLogoDownloader
		});
	Engine.SetStartStreamingOnSourceOpen(true);

	const LibISDB::CharType *pFileName = argv[1];
	if (LibISDB::StringCompare(pFileName, LIBISDB_STR("-")) == 0)
		pFileName = LibISDB::StandardInputStream::Name;
	if (!Engine.OpenSource(pFileName)) {
		ErrOut << LIBISDB_STR("Failed to open file : ") << pFileName << std::endl;
		return 1;
	}

	Engine.WaitForEndOfStream();
	pAsyncStreaming->WaitForEndOfStream();
	Engine.CloseSource();
	pMediaStats->Finish();

	const LibISDB::StreamFormat Format = pParser->GetStreamFormat();
	const LibISDB::TSPacketParserFilter::PacketCountInfo TotalCount = pParser->GetTotalPacketCount();
	const LibISDB::StreamPacketParserFilter::TLVPacketCountInfo TLVCount = pParser->GetTotalTLVPacketCount();

#if defined(LIBISDB_WCHAR)
	auto &Out = std::wcout;
#else
	auto &Out = std::cout;
#endif
	const int CountDigits = 10;

	Out << LIBISDB_STR("Stream Format   : ")
		<< (Format == LibISDB::StreamFormat::TLV ? LIBISDB_STR("TLV") :
			Format == LibISDB::StreamFormat::TS ? LIBISDB_STR("TS") : LIBISDB_STR("Unknown"))
		<< std::endl;
	Out << LIBISDB_STR("Input Bytes     : ") << std::setw(CountDigits) << pParser->GetTotalInputBytes() << std::endl;

	if (Format != LibISDB::StreamFormat::TLV) {
		Out << LIBISDB_STR("Input Packets   : ") << std::setw(CountDigits) << TotalCount.Input           << std::endl;
		Out << LIBISDB_STR("Format Error    : ") << std::setw(CountDigits) << TotalCount.FormatError     << std::endl;
		Out << LIBISDB_STR("Transport Error : ") << std::setw(CountDigits) << TotalCount.TransportError  << std::endl;
		Out << LIBISDB_STR("Dropped         : ") << std::setw(CountDigits) << TotalCount.ContinuityError << std::endl;
		Out << LIBISDB_STR("Scrambled       : ") << std::setw(CountDigits) << TotalCount.Scrambled       << std::endl;
		return 0;
	}

	Out << LIBISDB_STR("TLV Packets     : ") << std::setw(CountDigits) << TLVCount.Input           << std::endl;
	Out << LIBISDB_STR("  IPv4          : ") << std::setw(CountDigits) << TLVCount.IPv4            << std::endl;
	Out << LIBISDB_STR("  IPv6          : ") << std::setw(CountDigits) << TLVCount.IPv6            << std::endl;
	Out << LIBISDB_STR("  Compressed IP : ") << std::setw(CountDigits) << TLVCount.CompressedIP    << std::endl;
	Out << LIBISDB_STR("  Signaling     : ") << std::setw(CountDigits) << TLVCount.Signaling       << std::endl;
	Out << LIBISDB_STR("  Null          : ") << std::setw(CountDigits) << TLVCount.Null            << std::endl;
	Out << LIBISDB_STR("  Undefined     : ") << std::setw(CountDigits) << TLVCount.Undefined       << std::endl;
	Out << LIBISDB_STR("MMTP Packets    : ") << std::setw(CountDigits) << TLVCount.MMTP            << std::endl;
	Out << LIBISDB_STR("NTP Packets     : ") << std::setw(CountDigits) << TLVCount.NTP             << std::endl;
	Out << LIBISDB_STR("Other Payload   : ") << std::setw(CountDigits) << pInfo->GetOtherPacketCount() << std::endl;
	Out << LIBISDB_STR("Format Error    : ") << std::setw(CountDigits) << TLVCount.FormatError     << std::endl;
	Out << LIBISDB_STR("Sync Loss       : ") << std::setw(CountDigits) << TLVCount.SyncLoss        << std::endl;
	Out << LIBISDB_STR("Skipped Bytes   : ") << std::setw(CountDigits) << TLVCount.SkippedBytes    << std::endl;
	Out << LIBISDB_STR("Dropped         : ") << std::setw(CountDigits) << TLVCount.ContinuityError << std::endl;
	Out << LIBISDB_STR("Scrambled       : ") << std::setw(CountDigits) << TLVCount.Scrambled       << std::endl;

	const TLVInfoFilter::NTPInfo &NTP = pInfo->GetNTPInfo();
	if (NTP.Count > 0) {
		Out << std::endl;
		Out << LIBISDB_STR("NTP : ") << FormatNTPTime(NTP.First) << LIBISDB_STR(" - ") << FormatNTPTime(NTP.Last) << std::endl;
	}

	const auto &SignalingTableList = pInfo->GetSignalingTableList();
	if (!SignalingTableList.empty()) {
		Out << std::endl;
		Out << LIBISDB_STR("TLV-SI table_id : ") << std::setw(CountDigits) << LIBISDB_STR("Packets") << std::endl;
		for (const auto &e : SignalingTableList) {
			Out << LIBISDB_STR("            0x")
				<< std::hex << std::uppercase << std::setfill(LIBISDB_CHAR('0')) << std::setw(2) << static_cast<int>(e.first)
				<< std::dec << std::setfill(LIBISDB_CHAR(' '))
				<< LIBISDB_STR(" : ") << std::setw(CountDigits) << e.second
				<< LIBISDB_STR(" : ") << GetSignalingTableText(e.first) << std::endl;
		}
	}

	const auto &ContextList = pInfo->GetContextList();
	if (!ContextList.empty()) {
		Out << std::endl;
		Out << LIBISDB_STR(" CID : ");
		Out << std::setw(CountDigits) << LIBISDB_STR("Packets") << LIBISDB_STR(" ");
		Out << std::setw(CountDigits) << LIBISDB_STR("Full") << LIBISDB_STR(" ");
		Out << LIBISDB_STR(": Port") << std::endl;
		for (const auto &e : ContextList) {
			Out << std::hex << std::uppercase << std::setfill(LIBISDB_CHAR('0')) << std::setw(4) << e.first << LIBISDB_STR(" : ");
			Out << std::dec << std::setfill(LIBISDB_CHAR(' '));
			Out << std::setw(CountDigits) << e.second.Count << LIBISDB_STR(" ");
			Out << std::setw(CountDigits) << e.second.FullHeader << LIBISDB_STR(" ");
			Out << LIBISDB_STR(": ");
			if (e.second.HasPorts)
				Out << e.second.SourcePort << LIBISDB_STR(" -> ") << e.second.DestinationPort;
			Out << std::endl;
		}
	}

	const auto &PacketIDList = pInfo->GetPacketIDList();
	if (!PacketIDList.empty()) {
		Out << std::endl;
		Out << LIBISDB_STR("Packet ID : ");
		Out << std::setw(CountDigits) << LIBISDB_STR("Packets")   << LIBISDB_STR(" ");
		Out << std::setw(CountDigits) << LIBISDB_STR("Dropped")   << LIBISDB_STR(" ");
		Out << std::setw(CountDigits) << LIBISDB_STR("Scrambled") << LIBISDB_STR(" ");
		Out << std::setw(CountDigits) << LIBISDB_STR("RAP")       << LIBISDB_STR(" ");
		Out << LIBISDB_STR(": Payload : Description") << std::endl;
		for (const auto &e : PacketIDList) {
			Out << LIBISDB_STR("     ")
				<< std::hex << std::uppercase << std::setfill(LIBISDB_CHAR('0')) << std::setw(4) << e.first << LIBISDB_STR(" : ");
			Out << std::dec << std::setfill(LIBISDB_CHAR(' '));
			Out << std::setw(CountDigits) << e.second.Count     << LIBISDB_STR(" ");
			Out << std::setw(CountDigits) << e.second.Dropped   << LIBISDB_STR(" ");
			Out << std::setw(CountDigits) << e.second.Scrambled << LIBISDB_STR(" ");
			Out << std::setw(CountDigits) << e.second.RAP       << LIBISDB_STR(" ");
			Out << LIBISDB_STR(": ") << GetPayloadTypeText(e.second.PayloadTypes);
			if (e.second.FECTypes & ~1)
				Out << LIBISDB_STR(" (FEC)");
			const LibISDB::CharType *pText = GetPacketIDText(e.first);
			if (pText != nullptr)
				Out << LIBISDB_STR(" : ") << pText;
			Out << std::endl;
		}
	}

	Out << std::endl;
	PrintAnalyzerInfo(Out, pAnalyzer, EventCounter);

	Out << std::endl;
	PrintMediaStats(Out, pMediaStats);

	Out << std::endl;
	PrintCaptions(Out, Captions, 40);

	Out << std::endl;
	PrintEPG(Out, &EPGDatabase);

	Out << std::endl;
	PrintLogos(Out, Logos);

	return 0;
}
