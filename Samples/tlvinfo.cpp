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
#include "../LibISDB/Base/StandardStream.hpp"
#include "../LibISDB/Utilities/StringFormat.hpp"
#include "../LibISDB/Utilities/StringUtilities.hpp"
#include <iostream>
#include <iomanip>
#include <map>


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

	LibISDB::StreamSourceFilter *pSource = new LibISDB::StreamSourceFilter;
	LibISDB::StreamPacketParserFilter *pParser = new LibISDB::StreamPacketParserFilter;
	TLVInfoFilter *pInfo = new TLVInfoFilter;

#define ASYNC
#ifdef ASYNC
	pSource->SetSourceMode(LibISDB::SourceFilter::SourceMode::Pull);
	LibISDB::AsyncStreamingFilter *pAsyncStreaming = new LibISDB::AsyncStreamingFilter;
	pAsyncStreaming->SetSourceFilter(pSource);
	pAsyncStreaming->CreateBuffer(pAsyncStreaming->GetOutputBufferSize(), 3, 3);
#endif

	LibISDB::StreamSourceEngine Engine;

	Engine.BuildEngine({
		pSource,
#ifdef ASYNC
		pAsyncStreaming,
#endif
		pParser,
		pInfo
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
#ifdef ASYNC
	pAsyncStreaming->WaitForEndOfStream();
#endif
	Engine.CloseSource();

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

	return 0;
}
