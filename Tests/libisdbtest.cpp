/*
  LibISDB
  Copyright(c) 2017-2020 DBCTRADO

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
 @file   libisdbtest.cpp
 @brief  ユニットテスト
 @author DBCTRADO
*/


#include "../LibISDB/LibISDB.hpp"

#include <clocale>

#if defined(LIBISDB_WINDOWS) && defined(LIBISDB_WCHAR)
#define LIBISDB_TEST_WMAIN
#endif

#ifndef LIBISDB_TEST_WMAIN
#define CATCH_CONFIG_MAIN
#else
#define CATCH_CONFIG_RUNNER
#endif
#include "../Thirdparty/Catch/catch.hpp"

#include "../LibISDB/Base/DebugDef.hpp"


using namespace LibISDB::Literals;

namespace
{
#ifdef __cpp_char8_t
	typedef char8_t Char8;
#else
	typedef char Char8;
#endif

	const uint8_t * operator"" _b8(const char *str, size_t length) {
		return reinterpret_cast<const uint8_t *>(str);
	}

#ifdef __cpp_char8_t
	const uint8_t * operator"" _b8(const char8_t *str, size_t length) {
		return reinterpret_cast<const uint8_t *>(str);
	}
#endif
}


#include "../LibISDB/Utilities/AlignedAlloc.hpp"

TEST_CASE("AlignedAlloc", "[utility][memory]")
{
	std::uint8_t *buffer = static_cast<std::uint8_t *>(LibISDB::AlignedAlloc(32, 16));
	CHECK((reinterpret_cast<std::uintptr_t>(buffer) & 15) == 0);
	for (size_t i = 0; i < 32; i++)
		buffer[i] = static_cast<std::uint8_t>(i);
	buffer = static_cast<std::uint8_t *>(LibISDB::AlignedRealloc(buffer, 64, 32));
	CHECK((reinterpret_cast<std::uintptr_t>(buffer) & 31) == 0);
	CHECK(
		[buffer]() -> bool {
			for (size_t i = 0; i < 32; i++) {
				if (buffer[i] != i)
					return false;
			}
			return true;
		}());
	buffer = static_cast<std::uint8_t *>(LibISDB::AlignedRealloc(buffer, 16, 32, 5));
	CHECK(((reinterpret_cast<std::uintptr_t>(buffer) + 5) & 31) == 0);
	CHECK(
		[buffer]() -> bool {
			for (size_t i = 0; i < 16; i++) {
				if (buffer[i] != i)
					return false;
			}
			return true;
		}());
	LibISDB::AlignedFree(buffer);

	buffer = static_cast<std::uint8_t *>(LibISDB::AlignedAlloc(16, 16, 15));
	CHECK(((reinterpret_cast<std::uintptr_t>(buffer) + 15) & 15) == 0);
	buffer = static_cast<std::uint8_t *>(LibISDB::AlignedRealloc(buffer, 64, 32, 2));
	CHECK(((reinterpret_cast<std::uintptr_t>(buffer) + 2) & 31) == 0);
	LibISDB::AlignedFree(buffer);
}


#include "../LibISDB/Utilities/CRC.hpp"

TEST_CASE("CRC", "[utility][hash]")
{
	// CRC-16 (IBM)
	{
		CHECK(LibISDB::CRC16::Calc(nullptr, 0) == LibISDB::CRC16::InitialValue);
		CHECK(LibISDB::CRC16::Calc(u8"The quick brown fox jumps over the lazy dog"_b8, 43) == 0x60AE_u16);

		LibISDB::Hasher<LibISDB::CRC16> crc16;

		CHECK(crc16.Get() == LibISDB::CRC16::InitialValue);
		CHECK(crc16.Calc(u8"The quick brown fox "_b8, 20) == 0xD2A3_u16);
		CHECK(crc16.Calc(u8"jumps over the lazy dog"_b8, 23) == 0x60AE_u32);
		CHECK(crc16.Get() == 0x60AE_u32);
	}

	// CRC-16-CCITT
	{
		CHECK(LibISDB::CRC16CCITT::Calc(nullptr, 0) == LibISDB::CRC16CCITT::InitialValue);
		CHECK(LibISDB::CRC16CCITT::Calc(u8"The quick brown fox jumps over the lazy dog"_b8, 43) == 0xF0C8_u16);

		LibISDB::Hasher<LibISDB::CRC16CCITT> crc16;

		CHECK(crc16.Get() == LibISDB::CRC16::InitialValue);
		CHECK(crc16.Calc(u8"The quick brown fox "_b8, 20) == 0x9D25_u16);
		CHECK(crc16.Calc(u8"jumps over the lazy dog"_b8, 23) == 0xF0C8_u16);
		CHECK(crc16.Get() == 0xF0C8_u16);
	}

	// CRC-32
	{
		CHECK(LibISDB::CRC32::Calc(nullptr, 0) == LibISDB::CRC32::InitialValue);
		CHECK(LibISDB::CRC32::Calc(u8"The quick brown fox jumps over the lazy dog"_b8, 43) == 0x414FA339_u32);

		LibISDB::Hasher<LibISDB::CRC32> crc32;

		CHECK(crc32.Get() == LibISDB::CRC32::InitialValue);
		CHECK(crc32.Calc(u8"The quick brown fox "_b8, 20) == 0x88B075E2_u32);
		CHECK(crc32.Calc(u8"jumps over the lazy dog"_b8, 23) == 0x414FA339_u32);
		CHECK(crc32.Get() == 0x414FA339_u32);
	}

	// CRC-32/MPEG-2
	{
		CHECK(LibISDB::CRC32MPEG2::Calc(nullptr, 0) == LibISDB::CRC32MPEG2::InitialValue);
		CHECK(LibISDB::CRC32MPEG2::Calc(u8"The quick brown fox jumps over the lazy dog"_b8, 43) == 0xBA62119E_u32);

		LibISDB::Hasher<LibISDB::CRC32MPEG2> crc32;

		CHECK(crc32.Get() == LibISDB::CRC32MPEG2::InitialValue);
		CHECK(crc32.Calc(u8"The quick brown fox "_b8, 20) == 0xF7E3F54F_u32);
		CHECK(crc32.Calc(u8"jumps over the lazy dog"_b8, 23) == 0xBA62119E_u32);
		CHECK(crc32.Get() == 0xBA62119E_u32);
	}
}


#include "../LibISDB/Utilities/Sort.hpp"

TEST_CASE("Sort", "[utility][sort]")
{
	int list1[] = {6, 3, 1, 2, 4, 0, 5};
	LibISDB::InsertionSort(list1);
	CHECK(
		[&list1]() -> bool {
			for (int i = 0; i < static_cast<int>(std::size(list1)); i++) {
				if (list1[i] != i)
					return false;
			}
			return true;
		}());
	LibISDB::InsertionSort(list1, list1 + std::size(list1), std::greater<int>());
	CHECK(
		[&list1]() -> bool {
			for (int i = 0; i < static_cast<int>(std::size(list1)); i++) {
				if (list1[i] != static_cast<int>(std::size(list1)) - 1 - i)
					return false;
			}
			return true;
		}());

	struct item {
		int value;
		LibISDB::String text;
		bool operator < (const item &rhs) const { return value < rhs.value; }
	};
	item list2[] = {
		{3, LIBISDB_STR("three-1")},
		{2, LIBISDB_STR("two-1")},
		{3, LIBISDB_STR("three-2")},
		{1, LIBISDB_STR("one")},
		{0, LIBISDB_STR("zero")},
		{2, LIBISDB_STR("two-2")},
	};
	const item list2_sorted[] = {
		{0, LIBISDB_STR("zero")},
		{1, LIBISDB_STR("one")},
		{2, LIBISDB_STR("two-1")},
		{2, LIBISDB_STR("two-2")},
		{3, LIBISDB_STR("three-1")},
		{3, LIBISDB_STR("three-2")},
	};
	LibISDB::InsertionSort(list2);
	CHECK((
		[&list2, &list2_sorted]() -> bool {
			for (int i = 0; i < static_cast<int>(std::size(list2)); i++) {
				if ((list2[i].value != list2_sorted[i].value)
						|| (list2[i].text != list2_sorted[i].text))
					return false;
			}
			return true;
		}()));
}


#include "../LibISDB/Utilities/MD5.hpp"

TEST_CASE("MD5", "[utility][hash]")
{
	LibISDB::MD5Value md5;

	static const LibISDB::MD5Value zero_md5 = {
		0xD4, 0x1D, 0x8C, 0xD9, 0x8F, 0x00, 0xB2, 0x04, 0xE9, 0x80, 0x09, 0x98, 0xEC, 0xF8, 0x42, 0x7E
	};
	md5 = LibISDB::CalcMD5(nullptr, 0);
	CHECK(md5 == zero_md5);

	static const LibISDB::MD5Value fox_md5 = {
		0x9E, 0x10, 0x7D, 0x9D, 0x37, 0x2B, 0xB6, 0x82, 0x6B, 0xD8, 0x1D, 0x35, 0x42, 0xA4, 0x19, 0xD6
	};
	md5 = LibISDB::CalcMD5(u8"The quick brown fox jumps over the lazy dog"_b8, 43);
	CHECK(md5 == fox_md5);
}


#include "../LibISDB/Utilities/Utilities.hpp"

TEST_CASE("Load", "[utility][load]")
{
	const Char8 *Data = u8"ABCDE";

	CHECK(LibISDB::Load16(Data) == 0x4142_u16);
	CHECK(LibISDB::Load16(Data + 1) == 0x4243_u16);
	CHECK(LibISDB::Load24(Data) == 0x414243_u32);
	CHECK(LibISDB::Load24(Data + 1) == 0x424344_u32);
	CHECK(LibISDB::Load32(Data) == 0x41424344_u32);
	CHECK(LibISDB::Load32(Data + 1) == 0x42434445_u32);
}


#include "../LibISDB/Base/ARIBString.hpp"

TEST_CASE("ARIBString", "[base][string]")
{
	LibISDB::ARIBStringDecoder decoder;
	LibISDB::String str;

	// "テレビショッピング"
	decoder.Decode("\x1b\x7c\xc6\xec\xd3\xb7\xe7\xc3\xd4\xf3\xb0"_b8, 11, &str);
	CHECK(str.compare(LIBISDB_STR("\u30c6\u30ec\u30d3\u30b7\u30e7\u30c3\u30d4\u30f3\u30b0")) == 0);

	// "番組内容②"
	decoder.Decode("\x48\x56\x41\x48\x46\x62\x4d\x46\x1b\x24\x2a\x3b\x1b\x7d\xfe\xe2"_b8, 16, &str);
	CHECK(str.compare(LIBISDB_STR("\u756a\u7d44\u5185\u5bb9\u2461")) == 0);
}


#include "../LibISDB/Base/DateTime.hpp"

TEST_CASE("DateTime", "[base][time]")
{
	LibISDB::DateTime Time;

	CHECK_FALSE(Time.IsValid());

	Time.Year        = 2000;
	Time.Month       = 12;
	Time.Day         = 31;
	Time.Hour        = 23;
	Time.Minute      = 59;
	Time.Second      = 59;
	Time.Millisecond = 0;

	Time.SetDayOfWeek();
	CHECK(Time.DayOfWeek == 0);

	CHECK(Time.IsValid());

	Time.OffsetSeconds(1);

	CHECK(Time.Year == 2001);
	CHECK(Time.Month == 1);
	CHECK(Time.Day == 1);
	CHECK(Time.DayOfWeek == 1);
	CHECK(Time.Hour == 0);
	CHECK(Time.Minute == 0);
	CHECK(Time.Second == 0);
	CHECK(Time.Millisecond == 0);

	LibISDB::DateTime Time2(Time);

	CHECK(Time2.IsValid());
	CHECK(Time2 == Time);
	CHECK_FALSE(Time2 < Time);
	CHECK(Time2 <= Time);
	CHECK_FALSE(Time2 > Time);
	CHECK(Time2 >= Time);
	CHECK(Time2.Compare(Time) == 0);
	CHECK(Time2.DiffMilliseconds(Time) == 0LL);

	Time2.OffsetMinutes(-30);

	CHECK(Time2.Year == 2000);
	CHECK(Time2.Month == 12);
	CHECK(Time2.Day == 31);
	CHECK(Time2.DayOfWeek == 0);
	CHECK(Time2.Hour == 23);
	CHECK(Time2.Minute == 30);
	CHECK(Time2.Second == 0);
	CHECK(Time2.Millisecond == 0);

	CHECK(Time2 != Time);
	CHECK(Time2 < Time);
	CHECK(Time2 <= Time);
	CHECK(Time > Time2);
	CHECK(Time >= Time2);
	CHECK(Time2.Compare(Time) < 0);
	CHECK(Time.Compare(Time2) > 0);
	CHECK(Time2.Diff(Time) == std::chrono::milliseconds(-30LL * 60LL * 1000LL));
	CHECK(Time2.DiffMilliseconds(Time) == -30LL * 60LL * 1000LL);
	CHECK(Time2.DiffSeconds(Time) == -30LL * 60LL);

	unsigned long long LinearSeconds = Time.GetLinearSeconds();
	Time2.FromLinearSeconds(LinearSeconds);
	CHECK(Time == Time2);

	LinearSeconds += 60;
	Time.FromLinearSeconds(LinearSeconds);
	Time2.OffsetSeconds(60);
	CHECK(Time == Time2);

	Time.Millisecond = 500;
	unsigned long long LinearMilliseconds = Time.GetLinearMilliseconds();
	CHECK(LinearMilliseconds == LinearSeconds * 1000ULL + 500ULL);
	Time2.FromLinearMilliseconds(LinearMilliseconds);
	CHECK(Time == Time2);
}


#include "../LibISDB/TLV/TLVPacketParser.hpp"
#include "../LibISDB/TLV/StreamFormatDetector.hpp"
#include "../LibISDB/Filters/StreamPacketParserFilter.hpp"
#include <vector>
#include <memory>

namespace
{
	typedef std::vector<std::uint8_t> ByteArray;

	void Append(ByteArray &Dst, const ByteArray &Src)
	{
		Dst.insert(Dst.end(), Src.begin(), Src.end());
	}

	void Append16(ByteArray &Dst, std::uint16_t Value)
	{
		Dst.push_back(static_cast<std::uint8_t>(Value >> 8));
		Dst.push_back(static_cast<std::uint8_t>(Value));
	}

	void Append32(ByteArray &Dst, std::uint32_t Value)
	{
		Append16(Dst, static_cast<std::uint16_t>(Value >> 16));
		Append16(Dst, static_cast<std::uint16_t>(Value));
	}

	// 同期バイト (0x47 / 0x7F) を含まないデータ
	ByteArray MakeFiller(size_t Size, std::uint8_t Seed = 0)
	{
		ByteArray Data(Size);
		for (size_t i = 0; i < Size; i++) {
			std::uint8_t b = static_cast<std::uint8_t>(i * 37 + Seed);
			if ((b == 0x47) || (b == 0x7F))
				b = 0x00;
			Data[i] = b;
		}
		return Data;
	}

	ByteArray MakeTLV(std::uint8_t Type, const ByteArray &Data)
	{
		ByteArray Packet {0x7F, Type};
		Append16(Packet, static_cast<std::uint16_t>(Data.size()));
		Append(Packet, Data);
		return Packet;
	}

	ByteArray MakeMMTP(
		std::uint16_t PacketID, std::uint32_t Sequence, std::uint8_t PayloadType, size_t PayloadSize,
		int ScramblingControl = -1, bool SICV = false)
	{
		ByteArray Packet;
		const bool Extension = ScramblingControl >= 0;
		Packet.push_back(static_cast<std::uint8_t>((Extension ? 0x02 : 0x00) | 0x01)); // RAP
		Packet.push_back(PayloadType);
		Append16(Packet, PacketID);
		Append32(Packet, 0x12345678); // timestamp
		Append32(Packet, Sequence);
		if (Extension) {
			const std::uint16_t HdrExtLength = SICV ? 17 : 1;
			Append16(Packet, 0x0000); // extension_type
			Append16(Packet, static_cast<std::uint16_t>(4 + HdrExtLength));
			Append16(Packet, 0x8001); // hdr_ext_end_flag + hdr_ext_type
			Append16(Packet, HdrExtLength);
			Packet.push_back(static_cast<std::uint8_t>((ScramblingControl << 3) | (SICV ? 0x01 : 0x00)));
			if (SICV) {
				for (int i = 0; i < 16; i++)
					Packet.push_back(static_cast<std::uint8_t>(0xA0 + i));
			}
		}
		Append(Packet, MakeFiller(PayloadSize, static_cast<std::uint8_t>(Sequence)));
		return Packet;
	}

	ByteArray MakeIPv6UDP(std::uint16_t SourcePort, std::uint16_t DestinationPort, const ByteArray &Payload)
	{
		ByteArray Packet {0x60, 0x00, 0x00, 0x00};
		Append16(Packet, static_cast<std::uint16_t>(8 + Payload.size())); // payload_length
		Packet.push_back(17); // UDP
		Packet.push_back(0x20); // hop_limit
		for (int i = 0; i < 32; i++)
			Packet.push_back(static_cast<std::uint8_t>(i + 1));
		Append16(Packet, SourcePort);
		Append16(Packet, DestinationPort);
		Append16(Packet, static_cast<std::uint16_t>(8 + Payload.size()));
		Append16(Packet, 0x0000); // checksum
		Append(Packet, Payload);
		return Packet;
	}

	ByteArray MakeCompressedIP(
		std::uint16_t ContextID, std::uint8_t SequenceNumber, std::uint8_t HeaderType,
		std::uint16_t SourcePort, std::uint16_t DestinationPort, const ByteArray &Payload)
	{
		ByteArray Packet;
		Append16(Packet, static_cast<std::uint16_t>((ContextID << 4) | (SequenceNumber & 0x0F)));
		Packet.push_back(HeaderType);
		if (HeaderType == 0x60) {
			Packet.insert(Packet.end(), {0x60, 0x00, 0x00, 0x00, 17, 0x20});
			for (int i = 0; i < 32; i++)
				Packet.push_back(static_cast<std::uint8_t>(i + 1));
			Append16(Packet, SourcePort);
			Append16(Packet, DestinationPort);
		}
		Append(Packet, Payload);
		return Packet;
	}

	ByteArray MakeNTP(std::uint64_t Timestamp)
	{
		ByteArray Packet(40, 0);
		Packet[0] = 0x25; // LI 0, version 4, mode 5
		Append32(Packet, static_cast<std::uint32_t>(Timestamp >> 32));
		Append32(Packet, static_cast<std::uint32_t>(Timestamp));
		return Packet;
	}

	// 様々な種類の TLV パケットの列 (複数の列で連番を続けるには FirstSequence を 41 の倍数にする)
	std::vector<ByteArray> MakeTLVPacketList(std::uint32_t FirstSequence = 0)
	{
		std::vector<ByteArray> List;
		std::uint32_t Sequence = FirstSequence;

		List.push_back(MakeTLV(0x02, MakeIPv6UDP(456, 123, MakeNTP(0xE5000000'80000000ULL))));
		List.push_back(MakeTLV(0x03, MakeCompressedIP(0x001, 0, 0x60, 50000, 51216, MakeMMTP(0x0000, Sequence / 41, 0x02, 40))));
		for (int i = 0; i < 40; i++) {
			List.push_back(MakeTLV(0x03, MakeCompressedIP(0x001, static_cast<std::uint8_t>(i + 1), 0x61, 0, 0,
				MakeMMTP(0x0100, Sequence + i, 0x00, 1000 + i * 3, 0x02, (i % 5) == 0))));
			if ((i % 7) == 0)
				List.push_back(MakeTLV(0x03, MakeCompressedIP(0x001, 0, 0x61, 0, 0, MakeMMTP(0x0110, Sequence / 41 * 6 + i / 7, 0x00, 300))));
		}
		List.push_back(MakeTLV(0xFE, ByteArray {0x40, 0xF0, 0x05, 0x00, 0x01, 0xC1, 0x00, 0x00}));
		List.push_back(MakeTLV(0xFF, ByteArray(3000, 0xFF)));
		List.push_back(MakeTLV(0x03, MakeCompressedIP(0x001, 0, 0x61, 0, 0, MakeMMTP(0x0100, Sequence + 40, 0x00, 500))));

		return List;
	}

	ByteArray Concat(const std::vector<ByteArray> &List)
	{
		ByteArray Data;
		for (const ByteArray &e : List)
			Append(Data, e);
		return Data;
	}

	class TLVPacketCollector
		: public LibISDB::TLVPacketParser::PacketHandler
	{
	public:
		std::vector<ByteArray> Packets;
		std::vector<LibISDB::TLVPacket::ParseResult> Results;

		void OnTLVPacket(LibISDB::TLVPacket *pPacket, LibISDB::TLVPacket::ParseResult Result) override
		{
			Packets.emplace_back(pPacket->GetData(), pPacket->GetData() + pPacket->GetSize());
			Results.push_back(Result);
		}
	};

	void InputChunks(LibISDB::TLVPacketParser &Parser, const ByteArray &Data, size_t ChunkSize)
	{
		for (size_t Pos = 0; Pos < Data.size(); Pos += ChunkSize)
			Parser.InputData(&Data[Pos], std::min(ChunkSize, Data.size() - Pos));
	}
}

TEST_CASE("MMTPHeader", "[mmt][packet]")
{
	LibISDB::MMTPHeader Header;

	ByteArray Data = MakeMMTP(0x1234, 0x89ABCDEF, 0x00, 20);
	REQUIRE(Header.Parse(Data.data(), Data.size()));
	CHECK(Header.Version == 0);
	CHECK(Header.RAPFlag);
	CHECK_FALSE(Header.ExtensionFlag);
	CHECK(Header.PayloadType == 0x00);
	CHECK(Header.PacketID == 0x1234_u16);
	CHECK(Header.Timestamp == 0x12345678_u32);
	CHECK(Header.PacketSequenceNumber == 0x89ABCDEF_u32);
	CHECK(Header.HeaderSize == 12);
	CHECK_FALSE(Header.IsScrambled());

	Data = MakeMMTP(0x0100, 1, 0x00, 20, LibISDB::MMT_SCRAMBLING_CONTROL_ODD, true);
	REQUIRE(Header.Parse(Data.data(), Data.size()));
	CHECK(Header.ExtensionFlag);
	CHECK(Header.ExtensionType == 0x0000);
	CHECK(Header.ExtensionLength == 4 + 17);
	CHECK(Header.HasScrambleInfo);
	CHECK(Header.ScramblingControl == LibISDB::MMT_SCRAMBLING_CONTROL_ODD);
	CHECK(Header.IsScrambled());
	CHECK(Header.HasSICV);
	CHECK(Header.SICVOffset == 12 + 4 + 4 + 1);
	CHECK(Data[Header.SICVOffset] == 0xA0);
	CHECK(Header.HeaderSize == 12 + 4 + 4 + 17);

	Data = MakeMMTP(0x0100, 1, 0x00, 20, LibISDB::MMT_SCRAMBLING_CONTROL_EVEN);
	REQUIRE(Header.Parse(Data.data(), Data.size()));
	CHECK(Header.ScramblingControl == LibISDB::MMT_SCRAMBLING_CONTROL_EVEN);
	CHECK_FALSE(Header.HasSICV);

	// 拡張ヘッダが途中で切れている
	CHECK_FALSE(Header.Parse(Data.data(), 12 + 4 + 2));
	// version が 0 以外
	Data[0] |= 0x40;
	CHECK_FALSE(Header.Parse(Data.data(), Data.size()));
	// 短すぎる
	CHECK_FALSE(Header.Parse(Data.data(), 11));
}

TEST_CASE("TLVPacket", "[tlv][packet]")
{
	using LibISDB::TLVPacket;

	LibISDB::IPHeaderContextTable ContextTable;
	LibISDB::MMTPSequenceCounter SequenceCounter;
	TLVPacket Packet;
	ByteArray Data;

	CHECK(Packet.GetTypeID() == TLVPacket::TypeID);

	// IPv6 + UDP + MMTP
	Data = MakeTLV(0x02, MakeIPv6UDP(50000, 51216, MakeMMTP(0x0100, 1, 0x00, 100)));
	Packet.SetData(Data.data(), Data.size());
	REQUIRE(Packet.ParsePacket(&ContextTable, &SequenceCounter) == TLVPacket::ParseResult::OK);
	CHECK(Packet.GetPacketType() == 0x02);
	CHECK(Packet.GetDataLength() == Data.size() - 4);
	CHECK(Packet.IsIPPacket());
	CHECK(Packet.GetIPVersion() == 6);
	CHECK_FALSE(Packet.IsHeaderCompressed());
	CHECK(Packet.HasUDPPorts());
	CHECK(Packet.GetSourcePort() == 50000);
	CHECK(Packet.GetDestinationPort() == 51216);
	CHECK(Packet.GetPayloadKind() == TLVPacket::PayloadKind::MMTP);
	CHECK(Packet.HasMMTPHeader());
	CHECK(Packet.GetPacketID() == 0x0100);
	CHECK(Packet.GetPayloadSize() == 12 + 100);
	CHECK(Packet.GetMMTPPayloadSize() == 100);
	CHECK(Packet.GetMMTPPayloadData() == Packet.GetData() + 4 + 48 + 12);

	// フルヘッダでコンテキストを設定し、圧縮ヘッダで引き継ぐ
	Data = MakeTLV(0x03, MakeCompressedIP(0x123, 0, 0x60, 50000, 51216, MakeMMTP(0x0100, 2, 0x00, 100)));
	Packet.SetData(Data.data(), Data.size());
	REQUIRE(Packet.ParsePacket(&ContextTable, &SequenceCounter) == TLVPacket::ParseResult::OK);
	CHECK(Packet.IsHeaderCompressed());
	CHECK(Packet.GetContextID() == 0x123);
	CHECK(Packet.GetContextHeaderType() == 0x60);
	CHECK(Packet.GetIPVersion() == 6);
	CHECK(Packet.GetDestinationPort() == 51216);
	CHECK(Packet.GetMMTPPayloadSize() == 100);

	Data = MakeTLV(0x03, MakeCompressedIP(0x123, 1, 0x61, 0, 0, MakeMMTP(0x0100, 3, 0x00, 50, 0x02)));
	Packet.SetData(Data.data(), Data.size());
	REQUIRE(Packet.ParsePacket(&ContextTable, &SequenceCounter) == TLVPacket::ParseResult::OK);
	CHECK(Packet.GetContextSequenceNumber() == 1);
	CHECK(Packet.GetContextHeaderType() == 0x61);
	CHECK(Packet.HasUDPPorts());
	CHECK(Packet.GetSourcePort() == 50000);
	CHECK(Packet.GetDestinationPort() == 51216);
	CHECK(Packet.GetPayloadKind() == TLVPacket::PayloadKind::MMTP);
	CHECK(Packet.IsScrambled());
	CHECK(Packet.GetMMTPPayloadSize() == 50);

	// 未知のコンテキストでも MMTP として扱う
	Data = MakeTLV(0x03, MakeCompressedIP(0x456, 0, 0x61, 0, 0, MakeMMTP(0x0200, 0, 0x00, 50)));
	Packet.SetData(Data.data(), Data.size());
	REQUIRE(Packet.ParsePacket(&ContextTable, &SequenceCounter) == TLVPacket::ParseResult::OK);
	CHECK_FALSE(Packet.HasUDPPorts());
	CHECK(Packet.GetPayloadKind() == TLVPacket::PayloadKind::MMTP);

	// packet_sequence_number の不連続
	Data = MakeTLV(0x03, MakeCompressedIP(0x123, 2, 0x61, 0, 0, MakeMMTP(0x0100, 5, 0x00, 50)));
	Packet.SetData(Data.data(), Data.size());
	CHECK(Packet.ParsePacket(&ContextTable, &SequenceCounter) == TLVPacket::ParseResult::ContinuityError);
	CHECK(Packet.GetPayloadKind() == TLVPacket::PayloadKind::MMTP);

	// NTP
	Data = MakeTLV(0x02, MakeIPv6UDP(456, 123, MakeNTP(0xE5000000'80000000ULL)));
	Packet.SetData(Data.data(), Data.size());
	REQUIRE(Packet.ParsePacket(&ContextTable, &SequenceCounter) == TLVPacket::ParseResult::OK);
	CHECK(Packet.GetPayloadKind() == TLVPacket::PayloadKind::NTP);
	CHECK(Packet.GetNTPTransmitTimestamp() == 0xE5000000'80000000ULL);

	// コピーしても解析結果を保持する
	{
		TLVPacket Copy(Packet);
		CHECK(Copy.GetTypeID() == TLVPacket::TypeID);
		CHECK(Copy.GetPayloadKind() == TLVPacket::PayloadKind::NTP);
		CHECK(Copy.GetNTPTransmitTimestamp() == Packet.GetNTPTransmitTimestamp());
		CHECK(Copy.GetPayloadData() != Packet.GetPayloadData());
		TLVPacket Moved(std::move(Copy));
		CHECK(Moved.GetTypeID() == TLVPacket::TypeID);
		CHECK(Moved.GetPayloadKind() == TLVPacket::PayloadKind::NTP);
		CHECK(Moved.GetSize() == Packet.GetSize());
	}

	// TLV-SI / ヌル
	Data = MakeTLV(0xFE, ByteArray {0x40, 0xF0, 0x00});
	Packet.SetData(Data.data(), Data.size());
	REQUIRE(Packet.ParsePacket() == TLVPacket::ParseResult::OK);
	CHECK(Packet.GetPayloadKind() == TLVPacket::PayloadKind::Signaling);
	CHECK(Packet.GetPayloadSize() == 3);
	CHECK(Packet.GetPayloadData()[0] == 0x40);
	CHECK_FALSE(Packet.IsIPPacket());

	Data = MakeTLV(0xFF, ByteArray(2000, 0xFF));
	Packet.SetData(Data.data(), Data.size());
	REQUIRE(Packet.ParsePacket() == TLVPacket::ParseResult::OK);
	CHECK(Packet.GetPayloadKind() == TLVPacket::PayloadKind::Null);

	// IPv6 の UDP 以外
	Data = MakeIPv6UDP(1, 2, ByteArray(10, 0));
	Data[6] = 6; // TCP
	Data = MakeTLV(0x02, Data);
	Packet.SetData(Data.data(), Data.size());
	REQUIRE(Packet.ParsePacket() == TLVPacket::ParseResult::OK);
	CHECK(Packet.GetPayloadKind() == TLVPacket::PayloadKind::Other);

	// フォーマットエラー
	Data = MakeTLV(0x02, MakeIPv6UDP(50000, 51216, MakeMMTP(0x0100, 1, 0x00, 100)));
	Packet.SetData(Data.data(), Data.size() - 1); // length 不一致
	CHECK(Packet.ParsePacket() == TLVPacket::ParseResult::FormatError);
	Data[4 + 5]++; // payload_length 不一致
	Packet.SetData(Data.data(), Data.size());
	CHECK(Packet.ParsePacket() == TLVPacket::ParseResult::FormatError);
	Data = MakeTLV(0x10, ByteArray(10, 0)); // 未定義の packet_type
	Packet.SetData(Data.data(), Data.size());
	CHECK(Packet.ParsePacket() == TLVPacket::ParseResult::FormatError);
	Data = MakeTLV(0x02, MakeIPv6UDP(456, 123, ByteArray(20, 0))); // NTP が短い
	Packet.SetData(Data.data(), Data.size());
	CHECK(Packet.ParsePacket() == TLVPacket::ParseResult::FormatError);
	Data = MakeTLV(0x03, MakeCompressedIP(0x001, 0, 0x30, 0, 0, ByteArray(10, 0))); // 未定義の CID_header_type
	Packet.SetData(Data.data(), Data.size());
	CHECK(Packet.ParsePacket() == TLVPacket::ParseResult::FormatError);
}

TEST_CASE("TLVPacketParser", "[tlv][parser]")
{
	const std::vector<ByteArray> PacketList = MakeTLVPacketList();
	const ByteArray Stream = Concat(PacketList);

	SECTION("Split") {
		// 前に同期位置の候補を含むゴミを付ける
		ByteArray Garbage = MakeFiller(100);
		Garbage.insert(Garbage.begin() + 10, {0x7F, 0x02, 0x00, 0x05});
		Garbage.insert(Garbage.begin() + 50, {0x7F});
		ByteArray Data = Garbage;
		Append(Data, Stream);

		for (const size_t ChunkSize : {1_z, 2_z, 3_z, 5_z, 188_z, 1000_z, 4096_z, Data.size()}) {
			TLVPacketCollector Collector;
			LibISDB::TLVPacketParser Parser(&Collector);

			InputChunks(Parser, Data, ChunkSize);

			INFO("ChunkSize " << ChunkSize);
			REQUIRE(Collector.Packets.size() == PacketList.size());
			CHECK(Collector.Packets == PacketList);
			CHECK(Parser.IsSynced());
			CHECK(Parser.GetPacketCount().Input == PacketList.size());
			CHECK(Parser.GetPacketCount().SkippedBytes == Garbage.size());
			CHECK(Parser.GetPacketCount().SyncLoss == 0);
			CHECK(Parser.GetPacketCount().FormatError == 0);
			CHECK(Parser.GetPacketCount().ContinuityError == 0);
			CHECK(Parser.GetPacketCount().Null == 1);
			CHECK(Parser.GetPacketCount().Signaling == 1);
			CHECK(Parser.GetPacketCount().NTP == 1);
			CHECK(Parser.GetPacketCount().MMTP == PacketList.size() - 3);
			CHECK(Parser.GetPacketCount().Scrambled == 40);
		}
	}

	SECTION("Resync") {
		// パケットの間にゴミが入る
		ByteArray Data;
		const size_t Break = 20;
		for (size_t i = 0; i < PacketList.size(); i++) {
			if (i == Break)
				Append(Data, MakeFiller(333, 5));
			Append(Data, PacketList[i]);
		}

		for (const size_t ChunkSize : {1_z, 7_z, 1500_z, Data.size()}) {
			TLVPacketCollector Collector;
			LibISDB::TLVPacketParser Parser(&Collector);

			InputChunks(Parser, Data, ChunkSize);

			INFO("ChunkSize " << ChunkSize);
			CHECK(Collector.Packets == PacketList);
			CHECK(Parser.GetPacketCount().SyncLoss == 1);
			CHECK(Parser.GetPacketCount().SkippedBytes == 333);
		}
	}

	SECTION("BrokenLength") {
		// length が壊れたパケットは、次の同期が取れるまで読み捨てる
		std::vector<ByteArray> List = PacketList;
		List[10][2] ^= 0x80;
		const std::vector<ByteArray> NextList = MakeTLVPacketList(41);
		ByteArray Data = Concat(List);
		Append(Data, Concat(NextList));

		TLVPacketCollector Collector;
		LibISDB::TLVPacketParser Parser(&Collector);
		InputChunks(Parser, Data, 1500);

		CHECK(Parser.GetPacketCount().SyncLoss <= 1);
		REQUIRE(Collector.Packets.size() >= 10 + 10);
		CHECK(std::equal(PacketList.begin(), PacketList.begin() + 10, Collector.Packets.begin()));
		CHECK(std::equal(NextList.end() - 10, NextList.end(), Collector.Packets.end() - 10));
	}

	SECTION("Reset") {
		TLVPacketCollector Collector;
		LibISDB::TLVPacketParser Parser(&Collector);

		Parser.InputData(Stream.data(), Stream.size() / 2);
		Parser.Reset();
		CHECK_FALSE(Parser.IsSynced());
		CHECK(Parser.GetPacketCount().Input == 0);

		Collector.Packets.clear();
		Parser.InputData(Stream.data(), Stream.size());
		CHECK(Collector.Packets == PacketList);
	}
}

TEST_CASE("StreamFormatDetector", "[tlv][detector]")
{
	using LibISDB::StreamFormat;

	auto MakeTS = [](size_t Interval, size_t Count, size_t Offset) {
		ByteArray Data = MakeFiller(Offset + Interval * Count);
		for (size_t i = 0; i < Count; i++)
			Data[Offset + Interval * i] = 0x47;
		return Data;
	};

	for (const size_t Interval : {188_z, 192_z, 204_z}) {
		LibISDB::StreamFormatDetector Detector;
		const ByteArray Data = MakeTS(Interval, 8, 50);
		INFO("Interval " << Interval);
		CHECK(Detector.InputData(Data.data(), Data.size()) == StreamFormat::TS);
		CHECK(Detector.GetSyncPosition() == 50);
		CHECK(Detector.GetBuffer().GetSize() == Data.size());
	}

	{
		// 少しずつ入力
		LibISDB::StreamFormatDetector Detector;
		const ByteArray Data = MakeTS(188, 8, 0);
		CHECK(Detector.InputData(Data.data(), 188 * 4) == StreamFormat::Unknown);
		CHECK(Detector.InputData(&Data[188 * 4], 1) == StreamFormat::TS);
		CHECK(Detector.GetSyncPosition() == 0);
	}

	{
		LibISDB::StreamFormatDetector Detector;
		ByteArray Data = MakeFiller(30);
		Data.insert(Data.begin() + 5, 0x47); // 単独の 0x47
		const size_t SyncPos = Data.size();
		Append(Data, Concat(MakeTLVPacketList()));
		CHECK(Detector.InputData(Data.data(), 100) == StreamFormat::Unknown);
		CHECK(Detector.InputData(&Data[100], Data.size() - 100) == StreamFormat::TLV);
		CHECK(Detector.GetSyncPosition() == SyncPos);
		CHECK(Detector.GetBuffer().GetSize() == Data.size());

		Detector.Reset();
		CHECK(Detector.GetStreamFormat() == StreamFormat::Unknown);
		CHECK(Detector.GetBuffer().GetSize() == 0);
	}

	{
		// 判別できなければ TS として扱う
		LibISDB::StreamFormatDetector Detector;
		REQUIRE(Detector.SetMaxBufferSize(64 * 1024));
		const ByteArray Data = MakeFiller(16 * 1024);
		CHECK(Detector.InputData(Data.data(), Data.size()) == StreamFormat::Unknown);
		CHECK(Detector.InputData(Data.data(), Data.size()) == StreamFormat::Unknown);
		CHECK(Detector.InputData(Data.data(), Data.size()) == StreamFormat::Unknown);
		CHECK(Detector.InputData(Data.data(), Data.size()) == StreamFormat::TS);
		CHECK(Detector.GetSyncPosition() == 0);
	}

	CHECK(LibISDB::StreamFormatDetector::FindTSSync(MakeTS(188, 8, 100).data(), 188 * 8 + 100) == LibISDB::StreamFormatDetector::SyncResult::Found);
	CHECK(LibISDB::StreamFormatDetector::FindTSSync(MakeFiller(2000).data(), 2000) == LibISDB::StreamFormatDetector::SyncResult::Invalid);
	CHECK(LibISDB::StreamFormatDetector::FindTSSync(MakeTS(188, 3, 0).data(), 188 * 3) == LibISDB::StreamFormatDetector::SyncResult::NeedMoreData);
}

namespace
{
	class StreamCollector
		: public LibISDB::SingleInputFilter
		, public LibISDB::StreamPacketParserFilter::EventListener
	{
	public:
		const LibISDB::CharType * GetObjectName() const noexcept override { return LIBISDB_STR("StreamCollector"); }

		bool ReceiveData(LibISDB::DataStream *pData) override
		{
			if (pData->Is<LibISDB::TSPacket>()) {
				do {
					TSPackets++;
				} while (pData->Next());
			} else if (pData->Is<LibISDB::TLVPacket>()) {
				const LibISDB::TLVPacket *pFirst = pData->Get<LibISDB::TLVPacket>();
				size_t Count = 0;
				do {
					const LibISDB::TLVPacket *pPacket = pData->Get<LibISDB::TLVPacket>();
					if ((pPacket->GetPayloadKind() != pFirst->GetPayloadKind())
							|| (pPacket->HasMMTPHeader() && (pPacket->GetPacketID() != pFirst->GetPacketID())))
						MixedStream = true;
					TLVPackets.emplace_back(pPacket->GetData(), pPacket->GetData() + pPacket->GetSize());
					Count++;
				} while (pData->Next());
				MaxSequence = std::max(MaxSequence, Count);
			}
			return true;
		}

		void OnStreamFormatChanged(LibISDB::StreamPacketParserFilter *pFilter, LibISDB::StreamFormat Format) override
		{
			Formats.push_back(Format);
			FirstDataBeforeNotify.push_back(TSPackets + TLVPackets.size());
		}

		size_t TSPackets = 0;
		std::vector<ByteArray> TLVPackets;
		size_t MaxSequence = 0;
		bool MixedStream = false;
		std::vector<LibISDB::StreamFormat> Formats;
		std::vector<size_t> FirstDataBeforeNotify;
	};

	ByteArray MakeTSStream(size_t Count)
	{
		ByteArray Data;
		for (size_t i = 0; i < Count; i++) {
			Data.insert(Data.end(), {0x47, 0x01, 0x00, static_cast<std::uint8_t>(0x10 | (i & 0x0F))});
			Data.insert(Data.end(), 184, 0xA5);
		}
		return Data;
	}

	void InputFilter(LibISDB::StreamPacketParserFilter &Filter, const ByteArray &Data, size_t ChunkSize)
	{
		for (size_t Pos = 0; Pos < Data.size(); Pos += ChunkSize) {
			LibISDB::DataBuffer Buffer(&Data[Pos], std::min(ChunkSize, Data.size() - Pos));
			LibISDB::SingleDataStream<LibISDB::DataBuffer> Stream(&Buffer);
			Filter.ReceiveData(&Stream);
		}
	}
}

TEST_CASE("StreamPacketParserFilter", "[tlv][filter]")
{
	using LibISDB::StreamFormat;

	LibISDB::StreamPacketParserFilter Filter;
	StreamCollector Collector;
	Filter.SetOutputFilter(&Collector, Collector.GetInputSink());
	Filter.AddEventListener(&Collector);
	REQUIRE(Filter.StartStreaming());

	const std::vector<ByteArray> PacketList = MakeTLVPacketList();
	std::vector<ByteArray> NonNullList;
	for (const ByteArray &e : PacketList) {
		if (e[1] != 0xFF)
			NonNullList.push_back(e);
	}

	SECTION("TS") {
		const ByteArray Data = MakeTSStream(1000);
		InputFilter(Filter, Data, 5000);
		CHECK(Filter.GetStreamFormat() == StreamFormat::TS);
		CHECK(Collector.Formats == std::vector<StreamFormat> {StreamFormat::TS});
		CHECK(Collector.TSPackets == 1000);
		CHECK(Filter.GetPacketCount().Input == 1000);
		CHECK(Filter.GetPacketCount().ContinuityError == 0);
		CHECK(Filter.GetInputBytes() == Data.size());
	}

	SECTION("TLV") {
		const ByteArray Data = Concat(PacketList);
		InputFilter(Filter, Data, 65536);
		CHECK(Filter.GetStreamFormat() == StreamFormat::TLV);
		CHECK(Collector.Formats == std::vector<StreamFormat> {StreamFormat::TLV});
		CHECK(Collector.FirstDataBeforeNotify == std::vector<size_t> {0});
		CHECK(Collector.TSPackets == 0);
		CHECK(Collector.TLVPackets == NonNullList);
		CHECK_FALSE(Collector.MixedStream);
		CHECK(Collector.MaxSequence <= LibISDB::StreamPacketParserFilter::DEFAULT_MAX_TLV_SEQUENCE_PACKET_COUNT);
		CHECK(Collector.MaxSequence > 1);
		CHECK(Filter.GetPacketCount().Input == PacketList.size());
		CHECK(Filter.GetPacketCount().Output == NonNullList.size());
		CHECK(Filter.GetPacketCount().Scrambled == 40);
		CHECK(Filter.GetTLVPacketCount().Null == 1);
		CHECK(Filter.GetInputBytes() == Data.size());
	}

	SECTION("TLVNullPacket") {
		Filter.SetOutputNullPacket(true);
		InputFilter(Filter, Concat(PacketList), 100000);
		CHECK(Collector.TLVPackets == PacketList);
	}

	SECTION("Reset") {
		InputFilter(Filter, Concat(PacketList), 4096);
		Filter.Reset();
		CHECK(Filter.GetStreamFormat() == StreamFormat::Unknown);
		InputFilter(Filter, MakeTSStream(100), 4096);
		CHECK(Filter.GetStreamFormat() == StreamFormat::TS);
		CHECK(Collector.Formats == std::vector<StreamFormat> {StreamFormat::TLV, StreamFormat::TS});
		CHECK(Collector.TSPackets == 100);
		CHECK(Filter.GetPacketCount().Input == 100);
		CHECK(Filter.GetTotalPacketCount().Input == 100 + PacketList.size());
		CHECK(Filter.GetTotalTLVPacketCount().Input == PacketList.size());
	}

	SECTION("TSToTLV") {
		// リセット無しで TS から TLV に変わる
		InputFilter(Filter, MakeTSStream(1000), 16384);
		ByteArray TLVData;
		for (std::uint32_t i = 0; TLVData.size() < 512 * 1024; i++)
			Append(TLVData, Concat(MakeTLVPacketList(i * 41)));
		InputFilter(Filter, TLVData, 16384);
		CHECK(Filter.GetStreamFormat() == StreamFormat::TLV);
		CHECK(Collector.Formats == std::vector<StreamFormat> {StreamFormat::TS, StreamFormat::TLV});
		CHECK(Collector.TLVPackets.size() > 0);
		CHECK_FALSE(Collector.MixedStream);
	}

	SECTION("StaleTS") {
		// 短い TS の後に TLV が続く
		ByteArray Data = MakeTSStream(150);
		std::vector<ByteArray> TLVList;
		for (std::uint32_t i = 0; i < 4; i++) {
			const std::vector<ByteArray> List = MakeTLVPacketList(i * 41);
			TLVList.insert(TLVList.end(), List.begin(), List.end());
		}
		size_t NonNullCount = 0;
		for (const ByteArray &e : TLVList) {
			if (e[1] != 0xFF)
				NonNullCount++;
		}
		Append(Data, Concat(TLVList));

		for (const size_t ChunkSize : {188_z, 4096_z, 16384_z}) {
			auto pChunkFilter = std::make_unique<LibISDB::StreamPacketParserFilter>();
			StreamCollector ChunkCollector;
			pChunkFilter->SetOutputFilter(&ChunkCollector, ChunkCollector.GetInputSink());
			pChunkFilter->AddEventListener(&ChunkCollector);
			InputFilter(*pChunkFilter, Data, ChunkSize);

			INFO("ChunkSize " << ChunkSize);
			CHECK(pChunkFilter->GetStreamFormat() == StreamFormat::TLV);
			CHECK(ChunkCollector.Formats == std::vector<StreamFormat> {StreamFormat::TS, StreamFormat::TLV});
			CHECK(ChunkCollector.TSPackets >= 150);
			CHECK(ChunkCollector.TLVPackets.size() + 20 >= NonNullCount);
			CHECK(ChunkCollector.TLVPackets.back() == TLVList.back());
		}
	}

	SECTION("TLVToTS") {
		ByteArray TLVData = Concat(PacketList);
		InputFilter(Filter, TLVData, 16384);
		InputFilter(Filter, MakeTSStream(3000), 16384);
		CHECK(Filter.GetStreamFormat() == StreamFormat::TS);
		CHECK(Collector.Formats == std::vector<StreamFormat> {StreamFormat::TLV, StreamFormat::TS});
		CHECK(Collector.TSPackets > 0);
	}

	SECTION("Fixed") {
		Filter.SetFixedStreamFormat(StreamFormat::TLV);
		ByteArray Data = MakeFiller(1000);
		Append(Data, Concat(PacketList));
		InputFilter(Filter, Data, 3000);
		CHECK(Filter.GetStreamFormat() == StreamFormat::TLV);
		CHECK(Collector.TLVPackets.size() == NonNullList.size());
	}
}




#include "../LibISDB/MMT/MMTString.hpp"
#include "../LibISDB/MMT/MMTDescriptorBlock.hpp"
#include "../LibISDB/MMT/MMTDescriptors.hpp"
#include "../LibISDB/MMT/MMTTables.hpp"
#include "../LibISDB/MMT/MHEventInfo.hpp"
#include "../LibISDB/TLV/TLVSITables.hpp"
#include "../LibISDB/Filters/MMTAnalyzerFilter.hpp"
#include "../LibISDB/Utilities/CRC.hpp"
#include <string>

namespace
{
	ByteArray UTF8(const Char8 *pString)
	{
		const std::uint8_t *p = reinterpret_cast<const std::uint8_t *>(pString);
		return ByteArray(p, p + std::char_traits<Char8>::length(pString));
	}

	ByteArray HexBytes(const char *pHex)
	{
		ByteArray Data;
		for (; pHex[0] && pHex[1]; pHex += 2)
			Data.push_back(static_cast<std::uint8_t>(std::stoi(std::string(pHex, 2), nullptr, 16)));
		return Data;
	}

	ByteArray MakeMMTDescriptor(std::uint16_t Tag, const ByteArray &Body)
	{
		ByteArray Data;
		Append16(Data, Tag);
		switch (LibISDB::MMTDescriptorBase::GetLengthFieldSize(Tag)) {
		case 1:  Data.push_back(static_cast<std::uint8_t>(Body.size()));  break;
		case 2:  Append16(Data, static_cast<std::uint16_t>(Body.size())); break;
		default: Append32(Data, static_cast<std::uint32_t>(Body.size())); break;
		}
		Append(Data, Body);
		return Data;
	}

	ByteArray WithLength16(std::uint16_t Upper4Bits, const ByteArray &Data)
	{
		ByteArray Result;
		Append16(Result, static_cast<std::uint16_t>((Upper4Bits << 12) | Data.size()));
		Append(Result, Data);
		return Result;
	}

	ByteArray MakeSection(
		std::uint8_t TableID, std::uint16_t TableIDExtension, std::uint8_t Version,
		std::uint8_t SectionNumber, std::uint8_t LastSectionNumber, const ByteArray &Payload)
	{
		ByteArray Section {TableID};
		Append16(Section, static_cast<std::uint16_t>(0xB000 | (5 + Payload.size() + 4)));
		Append16(Section, TableIDExtension);
		Section.push_back(static_cast<std::uint8_t>(0xC1 | (Version << 1)));
		Section.push_back(SectionNumber);
		Section.push_back(LastSectionNumber);
		Append(Section, Payload);
		Append32(Section, LibISDB::CRC32MPEG2::Calc(Section.data(), Section.size()));
		return Section;
	}

	ByteArray MakeShortSection(std::uint8_t TableID, const ByteArray &Payload)
	{
		ByteArray Section {TableID};
		Append16(Section, static_cast<std::uint16_t>(0x7000 | (Payload.size() + 4)));
		Append(Section, Payload);
		Append32(Section, LibISDB::CRC32MPEG2::Calc(Section.data(), Section.size()));
		return Section;
	}

	ByteArray MakeMMTTable(std::uint8_t TableID, std::uint8_t Version, const ByteArray &Payload)
	{
		ByteArray Table {TableID, Version};
		Append16(Table, static_cast<std::uint16_t>(Payload.size()));
		Append(Table, Payload);
		return Table;
	}

	ByteArray MakePAMessage(const ByteArray &Table)
	{
		ByteArray Message;
		Append16(Message, 0x0000);
		Message.push_back(0x00);
		Append32(Message, static_cast<std::uint32_t>(1 + Table.size()));
		Message.push_back(0x00); // number_of_tables
		Append(Message, Table);
		return Message;
	}

	ByteArray MakeMessage16(std::uint16_t MessageID, const ByteArray &Data)
	{
		ByteArray Message;
		Append16(Message, MessageID);
		Message.push_back(0x00);
		Append16(Message, static_cast<std::uint16_t>(Data.size()));
		Append(Message, Data);
		return Message;
	}

	ByteArray MakeSignalingPayload(std::uint8_t FragmentationIndicator, const ByteArray &Data, std::uint8_t FragmentCounter = 0)
	{
		ByteArray Payload {static_cast<std::uint8_t>(FragmentationIndicator << 6), FragmentCounter};
		Append(Payload, Data);
		return Payload;
	}

	ByteArray MakeAggregatedSignalingPayload(const std::vector<ByteArray> &Messages)
	{
		ByteArray Payload {0x01, 0x00};
		for (const ByteArray &e : Messages) {
			Append16(Payload, static_cast<std::uint16_t>(e.size()));
			Append(Payload, e);
		}
		return Payload;
	}

	ByteArray MakeMMTPTLV(std::uint16_t PacketID, std::uint32_t Sequence, std::uint8_t PayloadType, const ByteArray &Payload)
	{
		ByteArray MMTP {0x00, PayloadType};
		Append16(MMTP, PacketID);
		Append32(MMTP, 0x12345678);
		Append32(MMTP, Sequence);
		Append(MMTP, Payload);
		return MakeTLV(0x03, MakeCompressedIP(0x001, 0, 0x60, 50000, 51216, MMTP));
	}

	void ToTLVPacket(const ByteArray &Data, LibISDB::TLVPacket *pPacket)
	{
		LibISDB::IPHeaderContextTable ContextTable;
		pPacket->SetData(Data.data(), Data.size());
		pPacket->ParsePacket(&ContextTable);
	}

	class SignalingCollector
		: public LibISDB::MMTSignalingParser::SignalingHandler
	{
	public:
		std::vector<ByteArray> Tables;
		std::vector<ByteArray> Sections;

		void OnMMTTable(const LibISDB::MMTSignalingParser *pParser, const LibISDB::MMTTable *pTable) override
		{
			Tables.emplace_back(pTable->GetData(), pTable->GetData() + pTable->GetSize());
		}

		void OnPSISection(const LibISDB::MMTSignalingParser *pParser, const LibISDB::PSISection *pSection) override
		{
			Sections.emplace_back(pSection->GetData(), pSection->GetData() + pSection->GetSize());
		}
	};

	void StoreSignaling(LibISDB::MMTSignalingParser &Parser, std::uint16_t PacketID, std::uint32_t Sequence, const ByteArray &Payload)
	{
		LibISDB::TLVPacket Packet;
		ToTLVPacket(MakeMMTPTLV(PacketID, Sequence, 0x02, Payload), &Packet);
		Parser.StorePacket(&Packet);
	}

	// MJD 61342 = 2026/10/29
	const ByteArray StartTime = HexBytes("EF9E050000");

	const LibISDB::String EventNameText = LIBISDB_STR("番組🆞");

	ByteArray MakeEITEvent(std::uint16_t EventID, const ByteArray &Duration, const ByteArray &Descriptors)
	{
		ByteArray Event;
		Append16(Event, EventID);
		Append(Event, StartTime);
		Append(Event, Duration);
		Append16(Event, static_cast<std::uint16_t>(0x0000 | Descriptors.size()));
		Append(Event, Descriptors);
		return Event;
	}

	ByteArray MakeEventDescriptors(std::uint16_t EventID)
	{
		ByteArray Desc;

		ByteArray ShortEvent = UTF8(u8"jpn");
		const ByteArray Name = UTF8(u8"番組🆞");
		ShortEvent.push_back(static_cast<std::uint8_t>(Name.size()));
		Append(ShortEvent, Name);
		const ByteArray Text = UTF8(u8"説明\rです");
		Append16(ShortEvent, static_cast<std::uint16_t>(Text.size()));
		Append(ShortEvent, Text);
		Append(Desc, MakeMMTDescriptor(0xF001, ShortEvent));

		// video component: 2160p 16:9 (パンベクトル無し) / プログレッシブ / component_tag 0x0000
		Append(Desc, MakeMMTDescriptor(0x8010, HexBytes("63E800005F6A706E")));
		// MH-audio component: AAC 5.1ch / component_tag 0x0010 / main、AAC stereo / component_tag 0x0011
		Append(Desc, MakeMMTDescriptor(0x8014, HexBytes("F309001011005F6A706E")));
		ByteArray Audio2 = HexBytes("F303001111001F6A706E");
		Append(Audio2, UTF8(u8"副音声"));
		Append(Desc, MakeMMTDescriptor(0x8014, Audio2));
		// MH-content
		Append(Desc, MakeMMTDescriptor(0x8012, HexBytes("40FF81FF")));
		// MH-event group (イベント共有)
		ByteArray Group {0x11};
		Append16(Group, 0x0401);
		Append16(Group, EventID + 0x100);
		Append(Desc, MakeMMTDescriptor(0x800C, Group));

		// MH-extended event (descriptor_number の逆順に置く)
		auto MakeItem = [](const ByteArray &Description, const ByteArray &Item) {
			ByteArray Data {static_cast<std::uint8_t>(Description.size())};
			Append(Data, Description);
			Append16(Data, static_cast<std::uint16_t>(Item.size()));
			Append(Data, Item);
			return Data;
		};
		auto MakeExtendedEvent = [](std::uint8_t Number, const ByteArray &Items) {
			ByteArray Data {static_cast<std::uint8_t>((Number << 4) | 0x01)};
			Append(Data, UTF8(u8"jpn"));
			Append16(Data, static_cast<std::uint16_t>(Items.size()));
			Append(Data, Items);
			Append16(Data, 0x0000);
			return MakeMMTDescriptor(0xF002, Data);
		};
		ByteArray Items1 = MakeItem(UTF8(u8"出演者"), UTF8(u8"山田\r花子"));
		Append(Items1, MakeItem(ByteArray(), UTF8(u8"無効")));
		Append(Desc, MakeExtendedEvent(1, Items1));
		Append(Desc, MakeExtendedEvent(0, MakeItem(UTF8(u8"番組内容"), UTF8(u8"内容"))));

		// MH-series
		ByteArray Series;
		Append16(Series, 0x0123);
		Series.push_back(0x15); // repeat_label 1 / program_pattern 2 / expire_date_valid_flag 1
		Append16(Series, 0xEF9E);
		Series.insert(Series.end(), {0x00, 0x50, 0x0C}); // episode_number 5 / last_episode_number 12
		Append(Series, UTF8(u8"シリーズ"));
		Append(Desc, MakeMMTDescriptor(0x8016, Series));

		return Desc;
	}

	ByteArray MakeEITPfPayload(std::uint16_t EventID)
	{
		ByteArray Payload;
		Append16(Payload, 0xB110);
		Append16(Payload, 0x000B);
		Payload.push_back(0x01);
		Payload.push_back(0x8B);
		Append(Payload, MakeEITEvent(EventID, HexBytes("013000"), MakeEventDescriptors(EventID)));
		return Payload;
	}

	ByteArray MakePLTPayload(std::uint16_t ServiceID, std::uint16_t MPTPacketID)
	{
		ByteArray Payload {0x01, 0x02};
		Append16(Payload, ServiceID);
		Payload.push_back(0x00);
		Append16(Payload, MPTPacketID);
		Payload.push_back(0x00); // num_of_ip_delivery
		return Payload;
	}

	ByteArray MakeAsset(std::uint16_t PacketID, std::uint32_t AssetType, std::uint16_t ComponentTag, const ByteArray &ExtraDescriptors = ByteArray())
	{
		ByteArray Asset {0x00};
		Append32(Asset, 0x00000000);
		Asset.push_back(0x02);
		Append16(Asset, PacketID);
		Append32(Asset, AssetType);
		Asset.push_back(0xFE);
		Asset.push_back(0x01); // location_count
		Asset.push_back(0x00);
		Append16(Asset, PacketID);
		ByteArray StreamID;
		Append16(StreamID, ComponentTag);
		ByteArray Descriptors = MakeMMTDescriptor(0x8011, StreamID);
		Append(Descriptors, ExtraDescriptors);
		Append16(Asset, static_cast<std::uint16_t>(Descriptors.size()));
		Append(Asset, Descriptors);
		return Asset;
	}

	ByteArray MakeMPTPayload(std::uint16_t ServiceID, std::uint8_t Timestamp, bool WithOtherAsset)
	{
		ByteArray Payload {0xFC, 0x02};
		Append16(Payload, ServiceID);
		const ByteArray AccessControl = MakeMMTDescriptor(0x8004, HexBytes("0005000F10"));
		Append16(Payload, static_cast<std::uint16_t>(AccessControl.size()));
		Append(Payload, AccessControl);
		std::vector<ByteArray> Assets;
		// MPU タイムスタンプ記述子だけが変わる
		ByteArray MPUTimestamp;
		Append32(MPUTimestamp, 1);
		Append32(MPUTimestamp, 0xEE8CD742);
		Append32(MPUTimestamp, Timestamp);
		Assets.push_back(MakeAsset(0x0100, LibISDB::MMT_ASSET_TYPE_HEV1, 0x0000, MakeMMTDescriptor(0x0001, MPUTimestamp)));
		Assets.push_back(MakeAsset(0x0111, LibISDB::MMT_ASSET_TYPE_MP4A, 0x0011));
		Assets.push_back(MakeAsset(0x0110, LibISDB::MMT_ASSET_TYPE_MP4A, 0x0010));
		Assets.push_back(MakeAsset(0x0130, LibISDB::MMT_ASSET_TYPE_STPP, 0x0030));
		Assets.push_back(MakeAsset(0x0140, LibISDB::MMT_ASSET_TYPE_AAPP, 0x0040));
		if (WithOtherAsset)
			Assets.push_back(MakeAsset(0x0150, 0x7A7A7A7A, 0x0050));
		Payload.push_back(static_cast<std::uint8_t>(Assets.size()));
		for (const ByteArray &e : Assets)
			Append(Payload, e);
		return Payload;
	}

	ByteArray MakeSDTPayload(std::uint16_t ServiceID, const ByteArray &ServiceName, bool WithLogo)
	{
		ByteArray Service {0x01};
		const ByteArray Provider = UTF8(u8"ＮＨＫ");
		Service.push_back(static_cast<std::uint8_t>(Provider.size()));
		Append(Service, Provider);
		Service.push_back(static_cast<std::uint8_t>(ServiceName.size()));
		Append(Service, ServiceName);
		ByteArray Descriptors = MakeMMTDescriptor(0x8019, Service);
		if (WithLogo)
			Append(Descriptors, MakeMMTDescriptor(0x8025, HexBytes("01FF23F0010400050001")));

		ByteArray Payload;
		Append16(Payload, 0x000B);
		Payload.push_back(0xFF);
		Append16(Payload, ServiceID);
		Payload.push_back(0xFF);
		Append(Payload, WithLength16(0x8, Descriptors));
		return Payload;
	}

	ByteArray MakeTLVNITPayload()
	{
		ByteArray NetworkDescriptors {0x40};
		const ByteArray Name = UTF8(u8"高度ＢＳデジタル放送");
		NetworkDescriptors.push_back(static_cast<std::uint8_t>(Name.size()));
		Append(NetworkDescriptors, Name);
		Append(NetworkDescriptors, HexBytes("FE020801"));
		Append(NetworkDescriptors, HexBytes("CD0B02010400FFFF0B0097FFFF"));

		ByteArray StreamDescriptors = HexBytes("4103040001");
		Append(StreamDescriptors, HexBytes("430B012034361100EB0337561B"));
		ByteArray Stream;
		Append16(Stream, 0xB110);
		Append16(Stream, 0x000B);
		Append(Stream, WithLength16(0xF, StreamDescriptors));

		ByteArray Payload = WithLength16(0xF, NetworkDescriptors);
		Append(Payload, WithLength16(0xF, Stream));
		return Payload;
	}
}

TEST_CASE("MMTString", "[mmt][string]")
{
	LibISDB::String Text;

	CHECK(LibISDB::DecodeMMTString(UTF8(u8"ABC 日本").data(), UTF8(u8"ABC 日本").size(), &Text));
	CHECK(Text == LIBISDB_STR("ABC 日本"));

	// 補助面の文字
	const ByteArray Enclosed = UTF8(u8"🆞🈑");
	const LibISDB::String EnclosedText = LIBISDB_STR("🆞🈑");
	CHECK(LibISDB::DecodeMMTString(Enclosed.data(), Enclosed.size(), &Text));
	CHECK(Text == EnclosedText);

	// CR は改行、それ以外の制御文字 (LF、タブ、C1) は除く
	const ByteArray Control = HexBytes("410D0A42094300C28044");
	CHECK(LibISDB::DecodeMMTString(Control.data(), Control.size(), &Text));
	CHECK(Text == LIBISDB_STR("A" LIBISDB_NEWLINE "BCD"));

	// 不正な並び
	const ByteArray Invalid = HexBytes("41FF42E697");
	const LibISDB::String InvalidText = LIBISDB_STR("A�B��");
	CHECK(LibISDB::DecodeMMTString(Invalid.data(), Invalid.size(), &Text));
	CHECK(Text == InvalidText);
	const ByteArray Overlong = HexBytes("C0AF");
	const LibISDB::String OverlongText = LIBISDB_STR("�");
	CHECK(LibISDB::DecodeMMTString(Overlong.data(), Overlong.size(), &Text));
	CHECK(Text == OverlongText);

	CHECK(LibISDB::DecodeMMTString(nullptr, 0, &Text));
	CHECK(Text.empty());
}

TEST_CASE("MMTDescriptorBlock", "[mmt][descriptor]")
{
	using namespace LibISDB;

	SECTION("LengthField") {
		CHECK(MMTDescriptorBase::GetLengthFieldSize(0x0001) == 1);
		CHECK(MMTDescriptorBase::GetLengthFieldSize(0x4000) == 2);
		CHECK(MMTDescriptorBase::GetLengthFieldSize(0x7000) == 4);
		CHECK(MMTDescriptorBase::GetLengthFieldSize(0x8019) == 1);
		CHECK(MMTDescriptorBase::GetLengthFieldSize(0xF001) == 2);
	}

	SECTION("Block") {
		ByteArray Data = MakeMMTDescriptor(0x8011, HexBytes("0010"));
		Append(Data, MakeMMTDescriptor(0x4000, ByteArray(300, 0x11)));
		Append(Data, MakeMMTDescriptor(0x7000, ByteArray(20, 0x22)));
		// 内容が不正な記述子は読み飛ばす
		Append(Data, MakeMMTDescriptor(0x8019, HexBytes("01FF")));
		Append(Data, MakeMMTDescriptor(0x8010, HexBytes("63E800005F6A706E")));
		// 範囲が収まらない記述子で終わる
		Append(Data, HexBytes("801105"));

		MMTDescriptorBlock Block;
		CHECK(Block.ParseBlock(Data.data(), Data.size()) == 4);
		CHECK(Block.GetDescriptorByIndex(1)->GetTag() == 0x4000);
		CHECK(Block.GetDescriptorByIndex(1)->GetLength() == 300);
		CHECK(Block.GetDescriptorByIndex(2)->GetSize() == 26);
		CHECK(Block.GetDescriptor<MHServiceDescriptor>() == nullptr);

		const MHStreamIdentifierDescriptor *pStreamID = Block.GetDescriptor<MHStreamIdentifierDescriptor>();
		REQUIRE(pStreamID != nullptr);
		CHECK(pStreamID->GetComponentTag() == 0x0010);

		const VideoComponentDescriptor *pVideo = Block.GetDescriptor<VideoComponentDescriptor>();
		REQUIRE(pVideo != nullptr);
		CHECK(pVideo->GetVideoResolution() == VideoComponentDescriptor::VIDEO_RESOLUTION_2160);
		CHECK(pVideo->GetVideoAspectRatio() == 3);
		CHECK(pVideo->GetVideoScanFlag());
		CHECK(pVideo->GetVideoFrameRate() == 8);
		CHECK(pVideo->GetComponentTag() == 0x0000);
		CHECK(pVideo->GetVideoTransferCharacteristics() == 5);
		CHECK(pVideo->GetLanguageCode() == LANGUAGE_CODE_JPN);

		MMTDescriptorBlock Copy(Block);
		REQUIRE(Copy.GetDescriptorCount() == 4);
		CHECK(Copy.GetDescriptor<VideoComponentDescriptor>() != pVideo);
		CHECK(Copy.GetDescriptor<VideoComponentDescriptor>()->GetVideoFrameRate() == 8);
	}

	SECTION("Descriptors") {
		// 実放送 (BS4K) の記述子
		ByteArray Data = MakeMMTDescriptor(0x8014, HexBytes("F303001011FF5F6A706EE697A5E69CACE8AA9E"));
		Append(Data, MakeMMTDescriptor(0x8025, HexBytes("01FE97F0020097050001060101070202")));
		Append(Data, MakeMMTDescriptor(0x8004, HexBytes("000500A121")));
		Append(Data, MakeMMTDescriptor(0x8020, HexBytes("002030076A706E012A10EE8CD2C0000000003F")));
		const ByteArray Provider = UTF8(u8"ＢＳ朝日");
		ByteArray Service {0x01, static_cast<std::uint8_t>(Provider.size())};
		Append(Service, Provider);
		Service.push_back(0x00);
		Append(Data, MakeMMTDescriptor(0x8019, Service));

		MMTDescriptorBlock Block;
		REQUIRE(Block.ParseBlock(Data.data(), Data.size()) == 5);

		const MHAudioComponentDescriptor *pAudio = Block.GetDescriptor<MHAudioComponentDescriptor>();
		REQUIRE(pAudio != nullptr);
		CHECK(pAudio->GetStreamContent() == 0x03);
		CHECK(pAudio->GetComponentType() == 0x03);
		CHECK(pAudio->GetComponentTag() == 0x0010);
		CHECK(pAudio->GetStreamType() == 0x11);
		CHECK(pAudio->GetSimulcastGroupTag() == 0xFF);
		CHECK_FALSE(pAudio->GetESMultiLingualFlag());
		CHECK(pAudio->GetMainComponentFlag());
		CHECK(pAudio->GetQualityIndicator() == 1);
		CHECK(pAudio->GetSamplingRate() == 7);
		CHECK(pAudio->GetLanguageCode() == LANGUAGE_CODE_JPN);
		String Text;
		CHECK(pAudio->GetText(&Text));
		CHECK(Text == LIBISDB_STR("日本語"));

		const MHLogoTransmissionDescriptor *pLogo = Block.GetDescriptor<MHLogoTransmissionDescriptor>();
		REQUIRE(pLogo != nullptr);
		CHECK(pLogo->GetLogoTransmissionType() == MHLogoTransmissionDescriptor::TRANSMISSION_CDT1);
		CHECK(pLogo->GetLogoID() == 0x097);
		CHECK(pLogo->GetLogoVersion() == 0x002);
		CHECK(pLogo->GetDownloadDataID() == 0x0097);
		REQUIRE(pLogo->GetLogoCount() == 3);
		MHLogoTransmissionDescriptor::LogoInfo Logo;
		CHECK(pLogo->GetLogoInfo(2, &Logo));
		CHECK(Logo.LogoType == 0x07);
		CHECK(Logo.StartSectionNumber == 0x02);
		CHECK(Logo.NumOfSections == 0x02);

		const MMTAccessControlDescriptor *pAccessControl = Block.GetDescriptor<MMTAccessControlDescriptor>();
		REQUIRE(pAccessControl != nullptr);
		CHECK(pAccessControl->GetCASystemID() == 0x0005);
		CHECK(pAccessControl->GetPacketID() == 0xA121);
		CHECK(pAccessControl->GetPrivateData()->GetSize() == 0);

		const MHDataComponentDescriptor *pDataComponent = Block.GetDescriptor<MHDataComponentDescriptor>();
		REQUIRE(pDataComponent != nullptr);
		CHECK(pDataComponent->GetDataComponentID() == MHDataComponentDescriptor::DATA_COMPONENT_ID_CAPTION);
		CHECK(pDataComponent->GetAdditionalDataComponentInfo()->GetSize() == 17);

		const MHServiceDescriptor *pService = Block.GetDescriptor<MHServiceDescriptor>();
		REQUIRE(pService != nullptr);
		CHECK(pService->GetServiceType() == 0x01);
		CHECK(pService->GetProviderName(&Text));
		CHECK(Text == LIBISDB_STR("ＢＳ朝日"));
		CHECK_FALSE(pService->GetServiceName(&Text));
	}
}

TEST_CASE("MMTSignalingParser", "[mmt][signaling]")
{
	using namespace LibISDB;

	SignalingCollector Collector;
	MMTSignalingParser Parser(&Collector);

	const ByteArray PLT = MakeMMTTable(0x80, 3, MakePLTPayload(0x0400, 0x0F00));
	const ByteArray MPT = MakeMMTTable(0x20, 7, MakeMPTPayload(0x0400, 1, true));
	const ByteArray TOT = MakeShortSection(0xA1, HexBytes("EF9E051916F000"));
	const ByteArray SDT = MakeSection(0x9F, 0xB110, 1, 0, 0, MakeSDTPayload(0x0400, UTF8(u8"テスト"), false));

	SECTION("Messages") {
		StoreSignaling(Parser, 0x0000, 0, MakeSignalingPayload(0, MakePAMessage(PLT)));
		StoreSignaling(Parser, 0x0000, 1, MakeSignalingPayload(0, MakeMessage16(0x8002, TOT)));
		StoreSignaling(Parser, 0x0000, 2, MakeSignalingPayload(0, MakeMessage16(0x8000, SDT)));
		StoreSignaling(Parser, 0x0000, 3, MakeSignalingPayload(0, MakeMessage16(0x8001, MakeMMTTable(0x86, 1, HexBytes("800405000500FFF2")))));
		// 未対応のメッセージ
		StoreSignaling(Parser, 0x0000, 4, MakeSignalingPayload(0, MakeMessage16(0x8003, HexBytes("00112233"))));
		// 制御メッセージ以外
		{
			TLVPacket Packet;
			ToTLVPacket(MakeMMTPTLV(0x0000, 5, 0x00, MakeSignalingPayload(0, MakePAMessage(PLT))), &Packet);
			Parser.StorePacket(&Packet);
		}

		REQUIRE(Collector.Tables.size() == 2);
		CHECK(Collector.Tables[0] == PLT);
		CHECK(Collector.Tables[1][0] == 0x86);
		CHECK(Collector.Sections == std::vector<ByteArray> {TOT, SDT});
		CHECK(Parser.GetPacketID() == 0x0000);
	}

	SECTION("Fragmented") {
		const ByteArray Message = MakePAMessage(MPT);
		const size_t Split1 = 40, Split2 = Message.size() - 30;
		StoreSignaling(Parser, 0x0F00, 10, MakeSignalingPayload(1, ByteArray(Message.begin(), Message.begin() + Split1), 2));
		StoreSignaling(Parser, 0x0F00, 11, MakeSignalingPayload(2, ByteArray(Message.begin() + Split1, Message.begin() + Split2), 1));
		CHECK(Collector.Tables.empty());
		StoreSignaling(Parser, 0x0F00, 12, MakeSignalingPayload(3, ByteArray(Message.begin() + Split2, Message.end()), 0));
		REQUIRE(Collector.Tables.size() == 1);
		CHECK(Collector.Tables[0] == MPT);

		// packet_sequence_number が飛んだ場合は破棄する
		StoreSignaling(Parser, 0x0F00, 13, MakeSignalingPayload(1, ByteArray(Message.begin(), Message.begin() + Split1), 1));
		StoreSignaling(Parser, 0x0F00, 15, MakeSignalingPayload(3, ByteArray(Message.begin() + Split1, Message.end()), 0));
		CHECK(Collector.Tables.size() == 1);

		// 先頭の無い断片は破棄する
		StoreSignaling(Parser, 0x0F00, 16, MakeSignalingPayload(3, ByteArray(Message.begin() + Split1, Message.end()), 0));
		CHECK(Collector.Tables.size() == 1);
	}

	SECTION("Aggregated") {
		StoreSignaling(Parser, 0x0000, 0, MakeAggregatedSignalingPayload({MakePAMessage(PLT), MakeMessage16(0x8002, TOT)}));
		CHECK(Collector.Tables == std::vector<ByteArray> {PLT});
		CHECK(Collector.Sections == std::vector<ByteArray> {TOT});
	}

	SECTION("CRCError") {
		ByteArray BrokenSDT = SDT;
		BrokenSDT[12] ^= 0x01;
		StoreSignaling(Parser, 0x8004, 0, MakeSignalingPayload(0, MakeMessage16(0x8000, BrokenSDT)));
		CHECK(Collector.Sections.empty());
		CHECK(Parser.GetCRCErrorCount() == 1);
	}
}

TEST_CASE("MMTTables", "[mmt][table]")
{
	using namespace LibISDB;

	SECTION("PLT/MPT") {
		std::unique_ptr<PLTable> pPLT(new PLTable);
		std::unique_ptr<MPTable> pMPT(new MPTable);
		int MPTUpdated = 0;
		pMPT->SetTableHandler([&](const MMTSingleTable *) { MPTUpdated++; });

		LibISDB::TLVPacket Packet;
		ToTLVPacket(MakeMMTPTLV(0x0000, 0, 0x02,
			MakeSignalingPayload(0, MakePAMessage(MakeMMTTable(0x80, 3, MakePLTPayload(0x0400, 0x0F00))))), &Packet);
		pPLT->StorePacket(&Packet);
		REQUIRE(pPLT->GetPackageCount() == 1);
		CHECK(pPLT->GetVersion() == 3);
		CHECK(pPLT->GetServiceID(0) == 0x0400);
		CHECK(pPLT->GetMPTPacketID(0) == 0x0F00);
		CHECK(pPLT->GetPackageInfo(0)->PackageID == 0x0400);

		ToTLVPacket(MakeMMTPTLV(0x0F00, 0, 0x02,
			MakeSignalingPayload(0, MakePAMessage(MakeMMTTable(0x20, 7, MakeMPTPayload(0x0400, 1, true))))), &Packet);
		pMPT->StorePacket(&Packet);
		pMPT->StorePacket(&Packet);
		CHECK(MPTUpdated == 1);
		CHECK(pMPT->IsTableAcquired());
		CHECK(pMPT->GetVersion() == 7);
		CHECK(pMPT->GetServiceID() == 0x0400);
		CHECK(pMPT->GetMPTMode() == 0);
		REQUIRE(pMPT->GetAssetCount() == 6);
		CHECK(pMPT->GetAssetType(0) == MMT_ASSET_TYPE_HEV1);
		CHECK(pMPT->GetAssetPacketID(1) == 0x0111);
		CHECK(pMPT->GetAssetComponentTag(2) == 0x0010);
		CHECK(pMPT->GetAssetInfo(0)->AssetID == ARIBString {0x01, 0x00});
		CHECK(pMPT->GetAssetDescriptorBlock(0)->GetDescriptorByTag(0x0001) != nullptr);
		const MMTAccessControlDescriptor *pAccessControl = pMPT->GetMPTDescriptorBlock()->GetDescriptor<MMTAccessControlDescriptor>();
		REQUIRE(pAccessControl != nullptr);
		CHECK(pAccessControl->GetPacketID() == 0x0F10);

		// 壊れた MPT は無視する
		ByteArray Broken = MakeMMTTable(0x20, 8, MakeMPTPayload(0x0400, 2, true));
		Broken.resize(Broken.size() - 5);
		Broken[2] = static_cast<std::uint8_t>((Broken.size() - 4) >> 8);
		Broken[3] = static_cast<std::uint8_t>(Broken.size() - 4);
		ToTLVPacket(MakeMMTPTLV(0x0F00, 1, 0x02, MakeSignalingPayload(0, MakePAMessage(Broken))), &Packet);
		pMPT->StorePacket(&Packet);
		CHECK(MPTUpdated == 1);
		CHECK(pMPT->GetVersion() == 7);
	}

	SECTION("MH-EIT") {
		std::unique_ptr<MMTSectionTable> pTable(new MMTSectionTable(new MHEITPfTable));

		LibISDB::TLVPacket Packet;
		ToTLVPacket(MakeMMTPTLV(0x8000, 0, 0x02,
			MakeSignalingPayload(0, MakeMessage16(0x8000, MakeSection(0x8B, 0x0400, 2, 1, 1, MakeEITPfPayload(0x1235))))), &Packet);
		pTable->StorePacket(&Packet);

		const MHEITPfTable *pEITPfTable = pTable->GetTable<MHEITPfTable>();
		REQUIRE(pEITPfTable != nullptr);
		CHECK(pEITPfTable->GetPfTable(0x0400, false) == nullptr);
		const MHEITTable *pEITTable = pEITPfTable->GetPfTable(0x0400, true);
		REQUIRE(pEITTable != nullptr);
		CHECK(pEITTable->GetTLVStreamID() == 0xB110);
		CHECK(pEITTable->GetOriginalNetworkID() == 0x000B);
		CHECK(pEITTable->GetEventID() == 0x1235);
		CHECK(pEITTable->GetDuration() == 90 * 60);

		EventInfo Info;
		REQUIRE(GetMHEventInfo(pEITTable, 0, &Info));
		CHECK(Info.NetworkID == 0x000B);
		CHECK(Info.TransportStreamID == 0xB110);
		CHECK(Info.ServiceID == 0x0400);
		CHECK(Info.EventID == 0x1235);
		CHECK(Info.StartTime.Year == 2026);
		CHECK(Info.StartTime.Month == 10);
		CHECK(Info.StartTime.Day == 29);
		CHECK(Info.StartTime.Hour == 5);
		CHECK(Info.Duration == 90 * 60);
		CHECK(Info.EventName == EventNameText);
		CHECK(Info.EventText == LIBISDB_STR("説明" LIBISDB_NEWLINE "です"));
		CHECK(Info.Type == (EventInfo::TypeFlag::Basic | EventInfo::TypeFlag::Extended | EventInfo::TypeFlag::Following));

		REQUIRE(Info.ExtendedText.size() == 2);
		CHECK(Info.ExtendedText[0].Description == LIBISDB_STR("番組内容"));
		CHECK(Info.ExtendedText[0].Text == LIBISDB_STR("内容"));
		CHECK(Info.ExtendedText[1].Description == LIBISDB_STR("出演者"));
		CHECK(Info.ExtendedText[1].Text == LIBISDB_STR("山田" LIBISDB_NEWLINE "花子"));

		REQUIRE(Info.VideoList.size() == 1);
		CHECK(Info.VideoList[0].StreamContent == 0x09);
		CHECK(Info.VideoList[0].ComponentType == 0x93);
		CHECK(Info.VideoList[0].ComponentTag == 0x00);
		CHECK(Info.VideoList[0].LanguageCode == LANGUAGE_CODE_JPN);

		REQUIRE(Info.AudioList.size() == 2);
		CHECK(Info.AudioList[0].StreamContent == 0x03);
		CHECK(Info.AudioList[0].ComponentType == 0x09);
		CHECK(Info.AudioList[0].ComponentTag == 0x10);
		CHECK(Info.AudioList[0].MainComponentFlag);
		CHECK(Info.AudioList[1].ComponentType == 0x03);
		CHECK(Info.AudioList[1].ComponentTag == 0x11);
		CHECK_FALSE(Info.AudioList[1].MainComponentFlag);
		CHECK(Info.AudioList[1].Text == LIBISDB_STR("副音声"));
		CHECK(Info.GetMainAudioIndex() == 0);

		REQUIRE(Info.ContentNibble.NibbleCount == 2);
		CHECK(Info.ContentNibble.NibbleList[0].ContentNibbleLevel1 == 0x4);
		CHECK(Info.ContentNibble.NibbleList[1].ContentNibbleLevel2 == 0x1);

		REQUIRE(Info.EventGroupList.size() == 1);
		CHECK(Info.EventGroupList[0].GroupType == EventGroupDescriptor::GROUP_TYPE_COMMON);
		CHECK(Info.IsCommonEvent);
		CHECK(Info.CommonEvent.ServiceID == 0x0401);
		CHECK(Info.CommonEvent.EventID == 0x1335);

		EventInfo::SeriesInfo Series;
		REQUIRE(GetMHEventSeriesInfo(pEITTable->GetItemDescriptorBlock(), &Series));
		CHECK(Series.SeriesID == 0x0123);
		CHECK(Series.RepeatLabel == 1);
		CHECK(Series.ProgramPattern == 2);
		CHECK(Series.ExpireDate.Day == 29);
		CHECK(Series.EpisodeNumber == 5);
		CHECK(Series.LastEpisodeNumber == 12);
		CHECK(Series.SeriesName == LIBISDB_STR("シリーズ"));
	}

	SECTION("MH-TOT") {
		std::unique_ptr<MMTSectionTable> pTable(new MMTSectionTable(new MHTOTTable));

		// MH-ローカル時間オフセット記述子: JPN +01:00
		ByteArray Payload = HexBytes("EF9E051916");
		const ByteArray Offset = MakeMMTDescriptor(0x8023, HexBytes("4A504E020100EF9E0000000100"));
		Append(Payload, WithLength16(0xF, Offset));
		LibISDB::TLVPacket Packet;
		ToTLVPacket(MakeMMTPTLV(0x8005, 0, 0x02, MakeSignalingPayload(0, MakeMessage16(0x8002, MakeShortSection(0xA1, Payload)))), &Packet);
		pTable->StorePacket(&Packet);

		const MHTOTTable *pTOT = pTable->GetTable<MHTOTTable>();
		REQUIRE(pTOT != nullptr);
		DateTime Time;
		REQUIRE(pTOT->GetDateTime(&Time));
		CHECK(Time.Hour == 5);
		CHECK(Time.Minute == 19);
		CHECK(Time.Second == 16);
		CHECK(pTOT->GetLocalTimeOffset(LocalTimeOffsetDescriptor::COUNTRY_CODE_JPN) == 60);
		REQUIRE(pTOT->GetOffsetDateTime(&Time, LocalTimeOffsetDescriptor::COUNTRY_CODE_JPN));
		CHECK(Time.Hour == 6);
	}
}

TEST_CASE("TLVSITables", "[tlv][table]")
{
	using namespace LibISDB;

	std::unique_ptr<TLVSITableSet> pTableSet(new TLVSITableSet);
	TLVSIParser Parser(pTableSet.get());

	// AMT: IPv6 の service_id 0x0400
	ByteArray AMTPayload;
	Append16(AMTPayload, (1 << 6) | 0x3F);
	Append16(AMTPayload, 0x0400);
	ByteArray AMTLoop = HexBytes("2401DBC010000000000000000000000280FF3E00000000000000000000A000100080");
	Append16(AMTPayload, static_cast<std::uint16_t>(0x8000 | 0x7C00 | AMTLoop.size()));
	Append(AMTPayload, AMTLoop);

	// 1 つの TLV パケットに 2 つのセクション
	ByteArray SI = MakeSection(0x40, 0x000B, 7, 0, 0, MakeTLVNITPayload());
	Append(SI, MakeSection(0xFE, 0x0000, 4, 0, 0, AMTPayload));
	LibISDB::TLVPacket Packet;
	ToTLVPacket(MakeTLV(0xFE, SI), &Packet);
	Parser.StorePacket(&Packet);
	CHECK(Parser.GetCRCErrorCount() == 0);

	const TLVNITMultiTable *pNITMultiTable = pTableSet->GetNITTable();
	REQUIRE(pNITMultiTable != nullptr);
	REQUIRE(pNITMultiTable->IsNITComplete());
	const TLVNITTable *pNIT = pNITMultiTable->GetNITTable(0);
	REQUIRE(pNIT != nullptr);
	CHECK(pNIT->GetNetworkID() == 0x000B);
	String Name;
	CHECK(pNIT->GetNetworkName(&Name));
	CHECK(Name == LIBISDB_STR("高度ＢＳデジタル放送"));
	const RemoteControlKeyDescriptor *pKey = pNIT->GetNetworkDescriptorBlock()->GetDescriptor<RemoteControlKeyDescriptor>();
	REQUIRE(pKey != nullptr);
	CHECK(pKey->GetKeyCount() == 2);
	CHECK(pKey->GetRemoteControlKeyID(0x0400) == 1);
	CHECK(pKey->GetRemoteControlKeyID(0x0097) == 11);
	CHECK(pKey->GetRemoteControlKeyID(0x0123) == 0);
	const SystemManagementDescriptor *pSystem = pNIT->GetNetworkDescriptorBlock()->GetDescriptor<SystemManagementDescriptor>();
	REQUIRE(pSystem != nullptr);
	CHECK(pSystem->GetBroadcastingID() == 8);
	REQUIRE(pNIT->GetTLVStreamCount() == 1);
	CHECK(pNIT->GetTLVStreamID(0) == 0xB110);
	const SatelliteDeliverySystemDescriptor *pSatellite =
		pNIT->GetItemDescriptorBlock(0)->GetDescriptor<SatelliteDeliverySystemDescriptor>();
	REQUIRE(pSatellite != nullptr);
	CHECK(pSatellite->GetFrequency() == 1203436);
	CHECK(pSatellite->GetModulation() == 11);
	CHECK(pNIT->GetItemDescriptorBlock(0)->GetDescriptor<ServiceListDescriptor>()->GetServiceCount() == 1);

	const AMTTable *pAMT = pTableSet->GetAMTTable();
	REQUIRE(pAMT != nullptr);
	REQUIRE(pAMT->GetServiceCount() == 1);
	const AMTTable::ServiceInfo *pService = pAMT->GetServiceInfo(0);
	CHECK(pService->ServiceID == 0x0400);
	CHECK(pService->IsIPv6);
	CHECK(pService->SourceAddress[0] == 0x24);
	CHECK(pService->SourceAddressMask == 128);
	CHECK(pService->DestinationAddress[0] == 0xFF);
	CHECK(pService->DestinationAddressMask == 128);

	// CRC 誤り
	SI[20] ^= 0x01;
	ToTLVPacket(MakeTLV(0xFE, SI), &Packet);
	Parser.StorePacket(&Packet);
	CHECK(Parser.GetCRCErrorCount() == 1);
}

namespace
{
	class AnalyzerEventRecorder
		: public LibISDB::MMTAnalyzerFilter::EventListener
	{
	public:
		std::vector<std::string> Events;

		void OnPLTUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override { Events.push_back("PLT"); }
		void OnMPTUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer, std::uint16_t ServiceID) override { Events.push_back("MPT"); }
		void OnMHSDTUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override { Events.push_back("SDT"); }
		void OnTLVNITUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override { Events.push_back("NIT"); }
		void OnMHBITUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override { Events.push_back("BIT"); }
		void OnMHEITUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override { Events.push_back("EIT"); }
		void OnMHCATUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override { Events.push_back("CAT"); }
		void OnMHTOTUpdated(LibISDB::MMTAnalyzerFilter *pAnalyzer) override { Events.push_back("TOT"); }
	};

	class PacketCountSink
		: public LibISDB::SingleInputFilter
	{
	public:
		const LibISDB::CharType * GetObjectName() const noexcept override { return LIBISDB_STR("PacketCountSink"); }

		bool ReceiveData(LibISDB::DataStream *pData) override
		{
			do {
				Count++;
			} while (pData->Next());
			return true;
		}

		size_t Count = 0;
	};
}

TEST_CASE("MMTAnalyzerFilter", "[mmt][filter]")
{
	using namespace LibISDB;

	auto pParser = std::make_unique<StreamPacketParserFilter>();
	auto pAnalyzer = std::make_unique<MMTAnalyzerFilter>();
	PacketCountSink Sink;
	AnalyzerEventRecorder Recorder;
	pParser->SetOutputFilter(pAnalyzer.get(), pAnalyzer->GetInputSink());
	pAnalyzer->SetOutputFilter(&Sink, Sink.GetInputSink());
	pAnalyzer->AddEventListener(&Recorder);
	REQUIRE(pParser->StartStreaming());

	std::vector<ByteArray> Packets;
	std::uint32_t Sequence = 0;
	auto Signaling = [&](std::uint16_t PacketID, const ByteArray &Message) {
		Packets.push_back(MakeMMTPTLV(PacketID, Sequence++, 0x02, MakeSignalingPayload(0, Message)));
	};
	auto NTP = [&](std::uint64_t Timestamp) {
		Packets.push_back(MakeTLV(0x02, MakeIPv6UDP(456, 123, MakeNTP(Timestamp))));
	};
	auto MPTMessage = [](std::uint8_t Version, std::uint8_t Timestamp, bool WithOtherAsset) {
		return MakePAMessage(MakeMMTTable(0x20, Version, MakeMPTPayload(0x0400, Timestamp, WithOtherAsset)));
	};

	const std::uint64_t NTPBase = 0xEE8CD740'00000000ULL;

	// PLT より前の MH-EIT は PLT の後に通知する
	Signaling(0x8000, MakeMessage16(0x8000, MakeSection(0x8B, 0x0400, 2, 0, 1, MakeEITPfPayload(0x1234))));
	NTP(NTPBase);
	Signaling(0x0000, MakePAMessage(MakeMMTTable(0x80, 3, MakePLTPayload(0x0400, 0x0F00))));
	{
		// 断片化された MPT
		const ByteArray Message = MPTMessage(7, 1, true);
		const size_t Split = Message.size() / 2;
		Packets.push_back(MakeMMTPTLV(0x0F00, 100, 0x02, MakeSignalingPayload(1, ByteArray(Message.begin(), Message.begin() + Split), 1)));
		Packets.push_back(MakeMMTPTLV(0x0F00, 101, 0x02, MakeSignalingPayload(3, ByteArray(Message.begin() + Split, Message.end()), 0)));
	}
	Signaling(0x8004, MakeMessage16(0x8000, MakeSection(0x9F, 0xB110, 1, 0, 0, MakeSDTPayload(0x0400, UTF8(u8"テスト４Ｋ"), true))));
	Signaling(0x8004, MakeMessage16(0x8000, MakeSection(0xA0, 0xB070, 1, 0, 0, MakeSDTPayload(0x0097, UTF8(u8"他局"), false))));
	Packets.push_back(MakeTLV(0xFE, MakeSection(0x40, 0x000B, 7, 0, 0, MakeTLVNITPayload())));
	Signaling(0x0001, MakeMessage16(0x8001, MakeMMTTable(0x86, 1, HexBytes("800405000500FFF2"))));
	{
		ByteArray BroadcasterDescriptors = MakeMMTDescriptor(0x8018, UTF8(u8"ＮＨＫ"));
		Append(BroadcasterDescriptors, MakeMMTDescriptor(0x800D, HexBytes("040001006601")));
		ByteArray BITPayload = WithLength16(0xE, ByteArray());
		BITPayload.push_back(0x01);
		Append(BITPayload, WithLength16(0xF, BroadcasterDescriptors));
		Signaling(0x8002, MakeMessage16(0x8000, MakeSection(0x9D, 0x000B, 6, 0, 0, BITPayload)));
	}
	Signaling(0x8005, MakeMessage16(0x8002, MakeShortSection(0xA1, HexBytes("EF9E051916F000"))));
	NTP(NTPBase + (3ULL << 32) + 0x80000000ULL);
	// version が同じでタイムスタンプだけ違う MPT は通知しない
	Packets.push_back(MakeMMTPTLV(0x0F00, 102, 0x02, MakeSignalingPayload(0, MPTMessage(7, 2, true))));
	Signaling(0x8000, MakeMessage16(0x8000, MakeSection(0x8B, 0x0400, 2, 1, 1, MakeEITPfPayload(0x1235))));

	ByteArray Data = Concat(Packets);
	InputFilter(*pParser, Data, 4096);

	CHECK(Recorder.Events == std::vector<std::string> {"PLT", "EIT", "MPT", "SDT", "NIT", "CAT", "BIT", "TOT", "EIT"});

	REQUIRE(pAnalyzer->GetServiceCount() == 1);
	MMTAnalyzerFilter::ServiceInfo Service;
	REQUIRE(pAnalyzer->GetServiceInfo(0, &Service));
	CHECK(Service.IsMPTAcquired);
	CHECK(Service.ServiceID == 0x0400);
	CHECK(Service.MPTPacketID == 0x0F00);
	CHECK(Service.MPTVersion == 7);
	CHECK(Service.AssetList.size() == 6);
	REQUIRE(Service.VideoAssetList.size() == 1);
	CHECK(Service.VideoAssetList[0].PacketID == 0x0100);
	REQUIRE(Service.AudioAssetList.size() == 2);
	CHECK(Service.AudioAssetList[0].ComponentTag == 0x0010);
	CHECK(Service.AudioAssetList[1].PacketID == 0x0111);
	CHECK(Service.CaptionAssetList.size() == 1);
	CHECK(Service.ApplicationAssetList.size() == 1);
	CHECK(Service.OtherAssetList.size() == 1);
	REQUIRE(Service.ECMList.size() == 1);
	CHECK(Service.ECMList[0].CASystemID == 0x0005);
	CHECK(Service.ECMList[0].PacketID == 0x0F10);
	CHECK(Service.ServiceName == LIBISDB_STR("テスト４Ｋ"));
	CHECK(Service.ProviderName == LIBISDB_STR("ＮＨＫ"));
	CHECK(Service.ServiceType == 0x01);
	CHECK(Service.LogoID == 0x123);

	CHECK(pAnalyzer->GetTLVStreamID() == 0xB110);
	CHECK(pAnalyzer->GetNetworkID() == 0x000B);
	String Name;
	CHECK(pAnalyzer->GetNetworkName(&Name));
	CHECK(Name == LIBISDB_STR("高度ＢＳデジタル放送"));
	CHECK(pAnalyzer->GetBroadcastingID() == 8);
	CHECK(pAnalyzer->GetRemoteControlKeyID(0x0400) == 1);

	MMTAnalyzerFilter::SDTStreamList SDTStreams;
	REQUIRE(pAnalyzer->GetSDTStreamList(&SDTStreams));
	REQUIRE(SDTStreams.size() == 2);
	CHECK(SDTStreams[0].TLVStreamID == 0xB070);
	CHECK(SDTStreams[0].ServiceList[0].ServiceName == LIBISDB_STR("他局"));
	CHECK(SDTStreams[1].TLVStreamID == 0xB110);

	MMTAnalyzerFilter::NetworkStreamList NetworkStreams;
	REQUIRE(pAnalyzer->GetNetworkStreamList(&NetworkStreams));
	REQUIRE(NetworkStreams.size() == 1);
	CHECK(NetworkStreams[0].ServiceList[0].ServiceID == 0x0400);
	MMTAnalyzerFilter::SatelliteDeliverySystemList Satellites;
	REQUIRE(pAnalyzer->GetSatelliteDeliverySystemList(&Satellites));
	CHECK(Satellites[0].Frequency == 1203436);

	MMTAnalyzerFilter::BITNetworkList BITNetworks;
	REQUIRE(pAnalyzer->GetBITNetworkList(&BITNetworks));
	REQUIRE(BITNetworks[0].BroadcasterList.size() == 1);
	CHECK(BITNetworks[0].BroadcasterList[0].BroadcasterName == LIBISDB_STR("ＮＨＫ"));
	CHECK(BITNetworks[0].BroadcasterList[0].ServiceList.size() == 2);

	MMTAnalyzerFilter::EMMPacketIDList EMMList;
	REQUIRE(pAnalyzer->GetEMMPacketIDList(&EMMList));
	CHECK(EMMList == MMTAnalyzerFilter::EMMPacketIDList {0xFFF2});

	CHECK(pAnalyzer->GetEventID(0) == 0x1234);
	CHECK(pAnalyzer->GetEventID(0, true) == 0x1235);
	EventInfo Event;
	REQUIRE(pAnalyzer->GetEventInfo(0, &Event));
	CHECK(Event.EventName == EventNameText);
	CHECK(Event.IsPresent());
	REQUIRE(pAnalyzer->GetEventInfo(0, &Event, true));
	CHECK(Event.IsFollowing());
	EventInfo::SeriesInfo Series;
	CHECK(pAnalyzer->GetEventSeriesInfo(0, &Series));

	DateTime Time;
	REQUIRE(pAnalyzer->GetTOTTime(&Time));
	CHECK(Time.Second == 16);
	bool Interpolated;
	REQUIRE(pAnalyzer->GetInterpolatedTOTTime(&Time, &Interpolated));
	CHECK(Interpolated);
	CHECK(Time.Second == 19);
	CHECK(Time.Millisecond == 500);

	CHECK(Sink.Count == Packets.size());

	SECTION("MPTVersion") {
		Recorder.Events.clear();
		InputFilter(*pParser,
			MakeMMTPTLV(0x0F00, 103, 0x02, MakeSignalingPayload(0, MPTMessage(8, 3, false))), 4096);
		CHECK(Recorder.Events == std::vector<std::string> {"MPT"});
		REQUIRE(pAnalyzer->GetServiceInfo(0, &Service));
		CHECK(Service.MPTVersion == 8);
		CHECK(Service.AssetList.size() == 5);
		CHECK(Service.OtherAssetList.empty());
	}

	SECTION("Reset") {
		pAnalyzer->Reset();
		CHECK(pAnalyzer->GetServiceCount() == 0);
		CHECK_FALSE(pAnalyzer->IsPLTUpdated());
		CHECK(pAnalyzer->GetTLVStreamID() == TRANSPORT_STREAM_ID_INVALID);
		CHECK_FALSE(pAnalyzer->GetTOTTime(&Time));
	}

	SECTION("TS") {
		auto pTSAnalyzer = std::make_unique<MMTAnalyzerFilter>();
		PacketCountSink TSSink;
		AnalyzerEventRecorder TSRecorder;
		pTSAnalyzer->SetOutputFilter(&TSSink, TSSink.GetInputSink());
		pTSAnalyzer->AddEventListener(&TSRecorder);
		auto pTSParser = std::make_unique<StreamPacketParserFilter>();
		pTSParser->SetOutputFilter(pTSAnalyzer.get(), pTSAnalyzer->GetInputSink());
		REQUIRE(pTSParser->StartStreaming());
		InputFilter(*pTSParser, MakeTSStream(500), 4096);
		CHECK(TSSink.Count == 500);
		CHECK(TSRecorder.Events.empty());
	}
}




#ifdef LIBISDB_TEST_WMAIN

static char * ConvertArg(const wchar_t *arg)
{
	size_t max_length = std::wcslen(arg) * MB_CUR_MAX + 1;
	char *mbs = new char[max_length];

#ifdef _MSC_VER
	size_t dest_length;
	::wcstombs_s(&dest_length, mbs, max_length, arg, max_length);
#else
	std::wcstombs(mbs, arg, max_length);
#endif

	return mbs;
}

static char ** ConvertArgs(int argc, wchar_t **argv)
{
	char **args = new char *[argc + 1];

	for (int i = 0; i < argc; i++) {
		args[i] = ConvertArg(argv[i]);
	}

	args[argc] = nullptr;

	return args;
}

static void FreeArgs(int argc, char **argv)
{
	for (int i = 0; i < argc; i++)
		delete [] argv[i];
	delete [] argv;
}

int wmain(int argc, wchar_t **argv)
{
	std::setlocale(LC_ALL, "");
	char **args = ConvertArgs(argc, argv);
	int result = Catch::Session().run(argc, args);
	FreeArgs(argc, args);

	return result < 0xff ? result : 0xff;
}

#endif
