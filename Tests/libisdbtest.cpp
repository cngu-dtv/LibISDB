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

	// \u8ffd\u52a0\u8a18\u53f7\u306e Unicode \u304b\u3089\u65e2\u5b9a\u306e\u6587\u5b57\u5217\u3078\u306e\u9006\u5f15\u304d
	typedef LibISDB::ARIBStringDecoder::InternalString InternalString;
	REQUIRE(LibISDB::ARIBStringDecoder::GetSymbolAlternativeText(0x1F211) != nullptr);
	CHECK(InternalString(LibISDB::ARIBStringDecoder::GetSymbolAlternativeText(0x1F211)) == LIBISDB_ARIB_STR("[\u5b57]"));
	REQUIRE(LibISDB::ARIBStringDecoder::GetSymbolAlternativeText(0x1F214) != nullptr);
	CHECK(InternalString(LibISDB::ARIBStringDecoder::GetSymbolAlternativeText(0x1F214)) == LIBISDB_ARIB_STR("[\u4e8c]"));  // 90 \u533a\u3092\u63a1\u308b
	REQUIRE(LibISDB::ARIBStringDecoder::GetSymbolAlternativeText(0x322A) != nullptr);
	CHECK(InternalString(LibISDB::ARIBStringDecoder::GetSymbolAlternativeText(0x322A)) == LIBISDB_ARIB_STR("(\u6708)"));
	CHECK(LibISDB::ARIBStringDecoder::GetSymbolAlternativeText(0x5E74) == nullptr);   // \u5e74 (\u65e2\u5b9a\u3068\u540c\u3058)
	CHECK(LibISDB::ARIBStringDecoder::GetSymbolAlternativeText(0x1F19E) == nullptr);  // STD-B24 \u306b\u306a\u3044
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

	ByteArray MakeNTP(std::uint64_t Timestamp, std::uint8_t LeapIndicator = 0)
	{
		ByteArray Packet(40, 0);
		Packet[0] = static_cast<std::uint8_t>((LeapIndicator << 6) | 0x25); // version 4, mode 5
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

		LibISDB::StreamPacketParserFilter::MMTPacketCountList CountList;
		REQUIRE(Filter.GetMMTPacketCountList(&CountList));
		REQUIRE(CountList.size() == 3);
		CHECK(CountList[0].PacketID == 0x0000);
		CHECK(CountList[0].Count.Input == 1);
		CHECK(CountList[0].Count.Output == 1);
		CHECK(CountList[1].PacketID == 0x0100);
		CHECK(CountList[1].Count.Input == 41);
		CHECK(CountList[1].Count.Output == 41);
		CHECK(CountList[1].Count.Scrambled == 40);
		CHECK(CountList[2].PacketID == 0x0110);
		CHECK(CountList[2].Count.Input == 6);
		CHECK(CountList[2].Count.Scrambled == 0);
	}

	SECTION("MMTPacketCount") {
		std::vector<ByteArray> NextList = MakeTLVPacketList(41);
		NextList.erase(NextList.begin() + 4); // 0x0100 の packet_sequence_number 42
		InputFilter(Filter, Concat(PacketList), 4096);
		InputFilter(Filter, Concat(NextList), 4096);

		LibISDB::StreamPacketParserFilter::MMTPacketCountList CountList;
		REQUIRE(Filter.GetMMTPacketCountList(&CountList));
		REQUIRE(CountList.size() == 3);
		CHECK(CountList[0].Count.Input == 2);
		CHECK(CountList[0].Count.ContinuityError == 0);
		CHECK(CountList[1].Count.Input == 81);
		CHECK(CountList[1].Count.ContinuityError == 1);
		CHECK(CountList[2].Count.Input == 12);
		CHECK(CountList[2].Count.ContinuityError == 0);
		CHECK(Filter.GetPacketCount().ContinuityError == 1);

		Filter.ResetErrorPacketCount();
		REQUIRE(Filter.GetMMTPacketCountList(&CountList));
		CHECK(CountList[1].Count.ContinuityError == 1);

		Filter.Reset();
		REQUIRE(Filter.GetMMTPacketCountList(&CountList));
		CHECK(CountList.empty());
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

	// 🆞 は既定で [4K] になる
	const LibISDB::String EventNameText = LIBISDB_STR("番組[4K]");

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
		ByteArray MPTDescriptors = MakeMMTDescriptor(0x8004, HexBytes("0005000F10"));
		Append(MPTDescriptors, MakeMMTDescriptor(0x8034, HexBytes("1F1FF100FFA100FFA04000F170")));
		Append16(Payload, static_cast<std::uint16_t>(MPTDescriptors.size()));
		Append(Payload, MPTDescriptors);
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

	// 追加記号は既定で ARIBStringDecoder の既定と同じ文字列、UnicodeSymbol で符号位置のまま
	const ByteArray Enclosed = UTF8(u8"空港🆞🈑🆧ピアノ㈪㋿");
	const LibISDB::String EnclosedText = LIBISDB_STR("空港[4K][字][HDR]ピアノ(月)㋿");
	const LibISDB::String EnclosedUnicodeText = LIBISDB_STR("空港🆞🈑🆧ピアノ㈪㋿");
	CHECK(LibISDB::DecodeMMTString(Enclosed.data(), Enclosed.size(), &Text));
	CHECK(Text == EnclosedText);
	CHECK(LibISDB::DecodeMMTString(Enclosed.data(), Enclosed.size(), &Text, LibISDB::ARIBStringDecoder::DecodeFlag::UnicodeSymbol));
	CHECK(Text == EnclosedUnicodeText);

	// LATIN-1 SUPPLEMENT は変換しない
	const ByteArray Latin1 = UTF8(u8"m²½");
	const LibISDB::String Latin1Text = LIBISDB_STR("m²½");
	CHECK(LibISDB::DecodeMMTString(Latin1.data(), Latin1.size(), &Text));
	CHECK(Text == Latin1Text);

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
		// 依存記述子 (ARIB STD-B60 表 7-32)
		CHECK(MMTDescriptorBase::GetLengthFieldSize(0x0002) == 2);
		CHECK(MMTDescriptorBase::GetLengthFieldSize(0x0003) == 1);
		CHECK(MMTDescriptorBase::GetLengthFieldSize(0x4000) == 2);
		CHECK(MMTDescriptorBase::GetLengthFieldSize(0x7000) == 4);
		CHECK(MMTDescriptorBase::GetLengthFieldSize(0x8019) == 1);
		CHECK(MMTDescriptorBase::GetLengthFieldSize(0xF001) == 2);
	}

	SECTION("Block") {
		// 依存記述子は長さが 16 ビット
		ByteArray Data = HexBytes("0002000101");
		Append(Data, MakeMMTDescriptor(0x8011, HexBytes("0010")));
		Append(Data, MakeMMTDescriptor(0x4000, ByteArray(300, 0x11)));
		Append(Data, MakeMMTDescriptor(0x7000, ByteArray(20, 0x22)));
		// 内容が不正な記述子は読み飛ばす
		Append(Data, MakeMMTDescriptor(0x8019, HexBytes("01FF")));
		Append(Data, MakeMMTDescriptor(0x8010, HexBytes("63E800005F6A706E")));
		// 範囲が収まらない記述子で終わる
		Append(Data, HexBytes("801105"));

		MMTDescriptorBlock Block;
		CHECK(Block.ParseBlock(Data.data(), Data.size()) == 5);
		CHECK(Block.GetDescriptorByIndex(0)->GetTag() == 0x0002);
		CHECK(Block.GetDescriptorByIndex(0)->GetSize() == 5);
		CHECK(Block.GetDescriptorByIndex(2)->GetTag() == 0x4000);
		CHECK(Block.GetDescriptorByIndex(2)->GetLength() == 300);
		CHECK(Block.GetDescriptorByIndex(3)->GetSize() == 26);
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
		REQUIRE(Copy.GetDescriptorCount() == 5);
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

	SECTION("ApplicationService") {
		// 実放送 (NHK BSP4K / 4K QVC) の MPT の記述子
		ByteArray Data = MakeMMTDescriptor(0x8034, HexBytes(
			"1F1FF900FFA200FFD24000FEE16000FEE26100FEE36200FEE47000FEE57100FEE67200FEE75000FEE9FF00FFEB"));
		Append(Data, MakeMMTDescriptor(0x8034, HexBytes("1F1FF100FFA100FFA04000F170")));
		// データ伝送メッセージ無し、AIT は packet_id 以外の位置、私的データ付き
		Append(Data, MakeMMTDescriptor(0x8034, HexBytes("100030" "02""20010DB8000000000000000000000001" "20010DB8000000000000000000000002" "1F90" "FFA3" "AB")));
		// EMT が足りない
		Append(Data, MakeMMTDescriptor(0x8034, HexBytes("1F1FC200FFA200FFD24000FEE1")));

		MMTDescriptorBlock Block;
		REQUIRE(Block.ParseBlock(Data.data(), Data.size()) == 3);

		std::vector<const ApplicationServiceDescriptor *> List;
		Block.EnumDescriptors<ApplicationServiceDescriptor>(
			[&](const ApplicationServiceDescriptor *pDesc) { List.push_back(pDesc); });
		REQUIRE(List.size() == 3);

		CHECK(List[0]->GetApplicationFormat() == ApplicationServiceDescriptor::APPLICATION_FORMAT_HTML5);
		CHECK(List[0]->GetDocumentResolution() == ApplicationServiceDescriptor::DOCUMENT_RESOLUTION_3840x2160);
		CHECK(List[0]->GetDefaultAITFlag());
		CHECK(List[0]->GetDTMessageFlag());
		CHECK(List[0]->GetAITPacketID() == 0xFFA2);
		CHECK(List[0]->GetDTMessagePacketID() == 0xFFD2);
		REQUIRE(List[0]->GetEMTCount() == 9);
		CHECK(List[0]->GetEMTInfo(0)->EMTTag == 0x40);
		CHECK(List[0]->GetEMTPacketID(0) == 0xFEE1);
		CHECK(List[0]->GetEMTInfo(8)->EMTTag == 0xFF);
		CHECK(List[0]->GetEMTPacketID(8) == 0xFFEB);
		CHECK(List[0]->GetEMTInfo(9) == nullptr);
		CHECK(List[0]->GetEMTPacketID(9) == MMT_PACKET_ID_INVALID);
		CHECK(List[0]->GetPrivateData()->GetSize() == 0);

		CHECK(List[1]->GetAITPacketID() == 0xFFA1);
		CHECK(List[1]->GetDTMessagePacketID() == 0xFFA0);
		REQUIRE(List[1]->GetEMTCount() == 1);
		CHECK(List[1]->GetEMTInfo(0)->EMTTag == 0x40);
		CHECK(List[1]->GetEMTPacketID(0) == 0xF170);

		CHECK(List[2]->GetDocumentResolution() == ApplicationServiceDescriptor::DOCUMENT_RESOLUTION_1920x1080);
		CHECK_FALSE(List[2]->GetDefaultAITFlag());
		CHECK_FALSE(List[2]->GetDTMessageFlag());
		CHECK(List[2]->GetAITLocationInfo().LocationType == MMTGeneralLocationInfo::LOCATION_TYPE_IPV6);
		CHECK(List[2]->GetAITLocationInfo().PacketID == 0xFFA3);
		CHECK(List[2]->GetAITPacketID() == MMT_PACKET_ID_INVALID);
		CHECK(List[2]->GetDTMessagePacketID() == MMT_PACKET_ID_INVALID);
		CHECK(List[2]->GetEMTCount() == 0);
		CHECK(List[2]->GetPrivateData()->GetSize() == 1);
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
	REQUIRE(Service.ApplicationServiceList.size() == 1);
	CHECK(Service.ApplicationServiceList[0].ApplicationFormat == ApplicationServiceDescriptor::APPLICATION_FORMAT_HTML5);
	CHECK(Service.ApplicationServiceList[0].DocumentResolution == ApplicationServiceDescriptor::DOCUMENT_RESOLUTION_3840x2160);
	CHECK(Service.ApplicationServiceList[0].DefaultAITFlag);
	CHECK(Service.ApplicationServiceList[0].AITPacketID == 0xFFA1);
	CHECK(Service.ApplicationServiceList[0].DTMessagePacketID == 0xFFA0);
	REQUIRE(Service.ApplicationServiceList[0].EMTList.size() == 1);
	CHECK(Service.ApplicationServiceList[0].EMTList[0].EMTTag == 0x40);
	CHECK(Service.ApplicationServiceList[0].EMTList[0].PacketID == 0xF170);
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


#include "../LibISDB/MMT/MMTPPayload.hpp"
#include "../LibISDB/MMT/MPUAssembler.hpp"
#include "../LibISDB/MMT/TTMLCaption.hpp"
#include "../LibISDB/Filters/CaptionFilter.hpp"
#include "../LibISDB/Filters/EPGDatabaseFilter.hpp"
#include <cstring>

namespace
{
	struct DataUnit {
		ByteArray Data;
		std::uint32_t ItemID = 0;
	};

	// MPU ペイロード (ARIB STD-B60 表 6-1)
	ByteArray MakeMPUPayload(
		std::uint8_t FragmentationIndicator, std::uint32_t MPUSequenceNumber,
		const std::vector<ByteArray> &Units, bool Timed = true)
	{
		const bool Aggregation = Units.size() > 1;
		ByteArray Body;
		Body.push_back(static_cast<std::uint8_t>(
			(LibISDB::MPU_FRAGMENT_TYPE_MFU << 4) | (Timed ? 0x08 : 0x00) | (FragmentationIndicator << 1) | (Aggregation ? 0x01 : 0x00)));
		Body.push_back(0x00);
		Append32(Body, MPUSequenceNumber);
		const ByteArray DUHeader(Timed ? LibISDB::MFU_TIMED_HEADER_SIZE : LibISDB::MFU_NON_TIMED_HEADER_SIZE, 0x00);
		for (const ByteArray &Unit : Units) {
			if (Aggregation)
				Append16(Body, static_cast<std::uint16_t>(DUHeader.size() + Unit.size()));
			Append(Body, DUHeader);
			Append(Body, Unit);
		}
		ByteArray Payload;
		Append16(Payload, static_cast<std::uint16_t>(Body.size()));
		Append(Payload, Body);
		return Payload;
	}

	// ScramblingControl が負の場合はヘッダ拡張を付けない
	ByteArray MakeMPUTLV(
		std::uint16_t PacketID, std::uint32_t Sequence, bool RAP, const ByteArray &Payload, int ScramblingControl = 0)
	{
		ByteArray MMTP {static_cast<std::uint8_t>((ScramblingControl >= 0 ? 0x02 : 0x00) | (RAP ? 0x01 : 0x00)), 0x00};
		Append16(MMTP, PacketID);
		Append32(MMTP, 0x12345678);
		Append32(MMTP, Sequence);
		if (ScramblingControl >= 0) {
			Append16(MMTP, 0x0000); // extension_type
			Append16(MMTP, 5);
			Append16(MMTP, 0x8001); // 最後のヘッダ拡張 / hdr_ext_type 0x0001
			Append16(MMTP, 1);
			MMTP.push_back(static_cast<std::uint8_t>(0xE0 | (ScramblingControl << 3)));
		}
		Append(MMTP, Payload);
		return MakeTLV(0x03, MakeCompressedIP(0x001, 0, 0x60, 50000, 51216, MMTP));
	}

	ByteArray MakeNALUnit(std::uint8_t Type, std::uint8_t Fill, size_t Size)
	{
		ByteArray NAL {static_cast<std::uint8_t>(Type << 1), 0x01};
		NAL.resize(Size, Fill);
		return NAL;
	}

	// 長さ前置の MFU (ARIB STD-B60 8.1.1)
	ByteArray MakeNALUnitMFU(const ByteArray &NALUnit)
	{
		ByteArray MFU;
		Append32(MFU, static_cast<std::uint32_t>(NALUnit.size()));
		Append(MFU, NALUnit);
		return MFU;
	}

	ByteArray ToAnnexB(const std::vector<ByteArray> &NALUnits)
	{
		ByteArray Data;
		for (const ByteArray &NAL : NALUnits) {
			Data.insert(Data.end(), {0x00, 0x00, 0x00, 0x01});
			Append(Data, NAL);
		}
		return Data;
	}

	ByteArray MakeCaptionMFU(
		std::uint8_t SubsampleNumber, std::uint8_t LastSubsampleNumber, std::uint8_t DataType,
		const ByteArray &Data, std::uint8_t SequenceNumber = 0)
	{
		ByteArray MFU {0x30, SequenceNumber, SubsampleNumber, LastSubsampleNumber, static_cast<std::uint8_t>(DataType << 4)};
		Append16(MFU, static_cast<std::uint16_t>(Data.size()));
		Append(MFU, Data);
		return MFU;
	}

	constexpr std::uint32_t TEST_NTP_SECONDS = 0xEE8CD742;

	ByteArray MakeMPUTimestampDescriptor(std::uint32_t MPUSequenceNumber, std::uint64_t Time)
	{
		ByteArray Body;
		Append32(Body, MPUSequenceNumber);
		Append32(Body, static_cast<std::uint32_t>(Time >> 32));
		Append32(Body, static_cast<std::uint32_t>(Time));
		return MakeMMTDescriptor(0x0001, Body);
	}

	// timescale 90000 / default_pts_offset 1500 / mpu_decoding_time_offset 3000
	ByteArray MakeMPUExtendedTimestampDescriptor(
		std::uint32_t MPUSequenceNumber, const std::vector<std::uint16_t> &DTSPTSOffsets, std::uint8_t LeapIndicator = 0)
	{
		ByteArray Body {0xFB};
		Append32(Body, 90000);
		Append16(Body, 1500);
		Append32(Body, MPUSequenceNumber);
		Body.push_back(static_cast<std::uint8_t>((LeapIndicator << 6) | 0x3F));
		Append16(Body, 3000);
		Body.push_back(static_cast<std::uint8_t>(DTSPTSOffsets.size()));
		for (std::uint16_t Offset : DTSPTSOffsets)
			Append16(Body, Offset);
		return MakeMMTDescriptor(0x8026, Body);
	}

	class AssemblerRecorder
		: public LibISDB::MPUAssembler::Handler
	{
	public:
		struct AccessUnit {
			std::uint32_t MPUSequenceNumber;
			std::uint32_t Index;
			bool HasTimestamp;
			std::int64_t DTS;
			std::int64_t PTS;
			bool RandomAccess;
			bool Discontinuity;
			ByteArray Data;
		};

		struct CaptionUnit {
			std::uint32_t MPUSequenceNumber;
			bool HasPresentationTime;
			std::uint64_t PresentationTime;
			bool Discontinuity;
			std::vector<std::pair<std::uint8_t, ByteArray>> Subsamples;
		};

		std::vector<AccessUnit> AccessUnits;
		std::vector<CaptionUnit> CaptionUnits;

		void OnAccessUnit(const LibISDB::MPUAssembler *pAssembler, const LibISDB::MPUAssembler::AccessUnitInfo &Info) override
		{
			AccessUnits.push_back({
				Info.MPUSequenceNumber, Info.Index, Info.HasTimestamp, Info.DTS, Info.PTS,
				Info.RandomAccess, Info.Discontinuity, ByteArray(Info.pData, Info.pData + Info.DataSize)});
		}

		void OnCaptionUnit(const LibISDB::MPUAssembler *pAssembler, const LibISDB::MPUAssembler::CaptionUnitInfo &Info) override
		{
			CaptionUnit &Unit = CaptionUnits.emplace_back();
			Unit.MPUSequenceNumber = Info.MPUSequenceNumber;
			Unit.HasPresentationTime = Info.HasPresentationTime;
			Unit.PresentationTime = Info.PresentationTime;
			Unit.Discontinuity = Info.Discontinuity;
			for (const auto &e : Info.SubsampleList)
				Unit.Subsamples.emplace_back(e.DataType, ByteArray(e.pData, e.pData + e.DataSize));
		}
	};

	void StoreMPU(LibISDB::MPUAssembler &Assembler, const ByteArray &Data)
	{
		LibISDB::TLVPacket Packet;
		ToTLVPacket(Data, &Packet);
		Assembler.StorePacket(&Packet);
	}

	const Char8 TestTTMLDocument[] =
		u8"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		"<tt xml:lang=\"ja\" xmlns=\"http://www.w3.org/ns/ttml\" xmlns:tts=\"http://www.w3.org/ns/ttml#styling\""
		" xmlns:arib-tt=\"http://www.arib.or.jp/ns/arib-tt\">"
		"<head><styling><style xml:id=\"s1\" tts:fontSize=\"120px 120px\"/></styling>"
		"<layout><region xml:id=\"r1\" tts:origin=\"968px 1516px\"/><region xml:id=\"r2\"/></layout></head>"
		"<body><div begin=\"00:51:55.994\">"
		"<p xml:id=\"p1\" region=\"r1\"><span style=\"s1\">皮膚の&amp;乾燥</span><br/><span>&#x3042;&#12354;</span></p>"
		"<!-- comment <p>ignored</p> -->"
		"<p region=\"r2\" begin=\"1.5s\" end=\"00:00:04.000\">\n  <span xml:id=\"base\">漢字</span>"
		"<span arib-tt:ruby=\"base\">かんじ</span><span> a  b </span></p>"
		"<p><span/></p>"
		"</div></body></tt>";
}

TEST_CASE("MMTPPayload", "[mmt][mpu]")
{
	using namespace LibISDB;

	MPUPayloadHeader Header;
	std::vector<MFUDataUnit> Units;

	SECTION("Single") {
		const ByteArray Payload = MakeMPUPayload(1, 0x01020304, {HexBytes("A1A2A3")});
		REQUIRE(ParseMPUPayload(Payload.data(), Payload.size(), &Header, &Units));
		CHECK(Header.FragmentType == MPU_FRAGMENT_TYPE_MFU);
		CHECK(Header.TimedFlag);
		CHECK(Header.FragmentationIndicator == MMT_FRAGMENTATION_INDICATOR_FIRST);
		CHECK_FALSE(Header.AggregationFlag);
		CHECK(Header.MPUSequenceNumber == 0x01020304);
		REQUIRE(Units.size() == 1);
		CHECK(ByteArray(Units[0].pData, Units[0].pData + Units[0].Size) == HexBytes("A1A2A3"));
	}

	SECTION("Aggregated") {
		ByteArray Payload = MakeMPUPayload(0, 7, {HexBytes("01"), HexBytes("0203"), ByteArray()});
		// payload_length より後ろは無視する
		Payload.push_back(0xEE);
		REQUIRE(ParseMPUPayload(Payload.data(), Payload.size(), &Header, &Units));
		CHECK(Header.AggregationFlag);
		REQUIRE(Units.size() == 3);
		CHECK(Units[1].Size == 2);
		CHECK(Units[1].pData[1] == 0x03);
		CHECK(Units[2].Size == 0);

		// data_unit_length が足りない
		ByteArray Broken = Payload;
		Broken[MPU_PAYLOAD_HEADER_SIZE + 1] = 0xFF;
		CHECK_FALSE(ParseMPUPayload(Broken.data(), Broken.size(), &Header, &Units));
	}

	SECTION("NonTimed") {
		ByteArray Payload = MakeMPUPayload(0, 1, {HexBytes("0000002AFF")}, false);
		REQUIRE(ParseMPUPayload(Payload.data(), Payload.size(), &Header, &Units));
		CHECK_FALSE(Header.TimedFlag);
		REQUIRE(Units.size() == 1);
		// item_id の後ろがデータ (MFU_NON_TIMED_HEADER_SIZE の 0 埋めの後ろに置いたもの)
		CHECK(Units[0].Size == 5);
	}

	SECTION("Short") {
		const ByteArray Payload = HexBytes("000420000000");
		CHECK_FALSE(ParseMPUPayload(Payload.data(), Payload.size(), &Header, &Units));
	}

	SECTION("CaptionHeader") {
		CaptionMFUHeader Caption;
		ByteArray MFU = MakeCaptionMFU(0, 0, MMT_CAPTION_DATA_TYPE_TTML, HexBytes("3C74743E"), 5);
		REQUIRE(Caption.Parse(MFU.data(), MFU.size()));
		CHECK(Caption.SubtitleTag == 0x30);
		CHECK(Caption.SubtitleSequenceNumber == 5);
		CHECK(Caption.DataSize == 4);
		CHECK(Caption.HeaderSize == 7);
		CHECK_FALSE(Caption.Parse(MFU.data(), MFU.size() - 1));

		// 32 ビット長 + サブサンプル情報
		ByteArray Extended {0x30, 0x01, 0x00, 0x02, 0x0C};
		Append32(Extended, 2);
		Extended.insert(Extended.end(), {0x10, 0x00, 0x00, 0x00, 0x09, 0x30, 0x00, 0x00, 0x00, 0x05});
		Extended.insert(Extended.end(), {0xAA, 0xBB});
		REQUIRE(Caption.Parse(Extended.data(), Extended.size()));
		CHECK(Caption.LengthExtensionFlag);
		CHECK(Caption.SubsampleInfoListFlag);
		CHECK(Caption.LastSubsampleNumber == 2);
		CHECK(Caption.HeaderSize == 19);
		CHECK(Caption.DataSize == 2);
	}
}

TEST_CASE("MPUTimestamp", "[mmt][mpu]")
{
	using namespace LibISDB;

	const std::uint64_t Time = static_cast<std::uint64_t>(TEST_NTP_SECONDS) << 32 | 0x80000000;
	const std::int64_t Base = static_cast<std::int64_t>(TEST_NTP_SECONDS) * 90000 + 45000;
	CHECK(MPUTimestampTable::NTPTimeTo90kHz(Time) == Base);

	ByteArray Descriptors = MakeMPUTimestampDescriptor(1005, Time);
	Append(Descriptors, MakeMPUTimestampDescriptor(1006, Time + (1ULL << 32)));
	Append(Descriptors, MakeMPUExtendedTimestampDescriptor(1005, {4500, 1500, 1500}));

	MMTDescriptorBlock Block;
	REQUIRE(Block.ParseBlock(Descriptors.data(), Descriptors.size()) == 3);
	const MPUTimestampDescriptor *pTimestamp = Block.GetDescriptor<MPUTimestampDescriptor>();
	REQUIRE(pTimestamp != nullptr);
	REQUIRE(pTimestamp->GetTimestampCount() == 1);
	CHECK(pTimestamp->GetTimestampInfo(0)->MPUPresentationTime == Time);
	const MPUExtendedTimestampDescriptor *pExtended = Block.GetDescriptor<MPUExtendedTimestampDescriptor>();
	REQUIRE(pExtended != nullptr);
	CHECK(pExtended->GetPTSOffsetType() == MPUExtendedTimestampDescriptor::PTS_OFFSET_TYPE_DEFAULT);
	CHECK(pExtended->GetTimescale() == 90000);
	REQUIRE(pExtended->GetMPUCount() == 1);
	CHECK(pExtended->GetMPUInfo(0)->MPUDecodingTimeOffset == 3000);
	REQUIRE(pExtended->GetMPUInfo(0)->AccessUnitList.size() == 3);
	CHECK(pExtended->GetMPUInfo(0)->AccessUnitList[2].PTSOffset == 1500);

	MPUTimestampTable Table;
	Table.Update(&Block);
	CHECK(Table.GetMPUCount() == 2);

	std::int64_t DTS, PTS;
	// DTS = 提示時刻 - 3000 + 1500 * n、PTS = DTS + dts_pts_offset
	REQUIRE(Table.GetAccessUnitTime(1005, 0, &DTS, &PTS));
	CHECK(DTS == Base - 3000);
	CHECK(PTS == Base + 1500);
	REQUIRE(Table.GetAccessUnitTime(1005, 2, &DTS, &PTS));
	CHECK(DTS == Base);
	CHECK(PTS == Base + 1500);
	CHECK_FALSE(Table.GetAccessUnitTime(1005, 3, &DTS, &PTS));
	// MPU 1006 は拡張タイムスタンプが無い
	CHECK_FALSE(Table.GetAccessUnitTime(1006, 0, &DTS, &PTS));
	std::uint64_t PresentationTime;
	REQUIRE(Table.GetPresentationTime(1006, &PresentationTime));
	CHECK(PresentationTime == Time + (1ULL << 32));

	// MPU_sequence_number が 0 に戻ったら古い MPU は捨てる
	ByteArray Reset = MakeMPUTimestampDescriptor(0, Time);
	MMTDescriptorBlock ResetBlock;
	ResetBlock.ParseBlock(Reset.data(), Reset.size());
	Table.Update(&ResetBlock);
	CHECK(Table.GetMPUCount() == 1);
	CHECK_FALSE(Table.GetPresentationTime(1005, &PresentationTime));
	CHECK(Table.GetPresentationTime(0, &PresentationTime));

	// うるう秒 (ARIB TR-B39 第1部 第7編 付録 1): 提示時刻は 23:59:59.5 UTC (8:59:59.5 JST)
	const std::int64_t DaySeconds = 24 * 60 * 60;
	const std::uint64_t LeapSeconds = (static_cast<std::uint64_t>(TEST_NTP_SECONDS) / DaySeconds + 1) * DaySeconds - 1;
	const std::uint64_t LeapTime = (LeapSeconds << 32) | 0x80000000;
	const std::int64_t LeapBase = static_cast<std::int64_t>(LeapSeconds) * 90000 + 45000;
	const std::int64_t Boundary = LeapBase + 45000;
	std::vector<std::uint16_t> Offsets(40, 1500);

	for (std::uint8_t Leap = 0; Leap <= 3; Leap++) {
		ByteArray LeapDesc = MakeMPUTimestampDescriptor(2000, LeapTime);
		Append(LeapDesc, MakeMPUExtendedTimestampDescriptor(2000, Offsets, Leap));
		MMTDescriptorBlock LeapBlock;
		LeapBlock.ParseBlock(LeapDesc.data(), LeapDesc.size());
		MPUTimestampTable LeapTable;
		LeapTable.Update(&LeapBlock);

		// DTS = 境界 - 48000 + 1500 * n、PTS = DTS + 1500
		REQUIRE(LeapTable.GetAccessUnitTime(2000, 0, &DTS, &PTS));
		CHECK(DTS == LeapBase - 3000 + ((Leap == 2) ? 90000 : 0));
		REQUIRE(LeapTable.GetAccessUnitTime(2000, 31, &DTS, &PTS));
		// 挿入では 0:00:00 UTC 以後の時刻から 1 秒引く、削除では 23:59:59 UTC 以後の時刻に 1 秒足す
		CHECK(DTS == Boundary - 1500 + ((Leap == 2) ? 90000 : 0));
		CHECK(PTS == Boundary + ((Leap == 1) ? -90000 : (Leap == 2) ? 90000 : 0));
		REQUIRE(LeapTable.GetAccessUnitTime(2000, 32, &DTS, &PTS));
		CHECK(DTS == Boundary + ((Leap == 1) ? -90000 : (Leap == 2) ? 90000 : 0));
	}
}

TEST_CASE("MPUAssembler", "[mmt][mpu]")
{
	using namespace LibISDB;

	AssemblerRecorder Recorder;
	const std::uint64_t Time = static_cast<std::uint64_t>(TEST_NTP_SECONDS) << 32;
	const std::int64_t Base = static_cast<std::int64_t>(TEST_NTP_SECONDS) * 90000;

	SECTION("Video") {
		MPUAssembler Assembler(MPUAssembler::MediaType::Video, &Recorder);
		ByteArray Descriptors = MakeMPUTimestampDescriptor(10, Time);
		Append(Descriptors, MakeMPUExtendedTimestampDescriptor(10, {4500, 1500}));
		MMTDescriptorBlock Block;
		Block.ParseBlock(Descriptors.data(), Descriptors.size());
		Assembler.UpdateTimestamps(&Block);

		const ByteArray AUD = MakeNALUnit(35, 0x50, 3);
		const ByteArray VPS = MakeNALUnit(32, 0x11, 10);
		const ByteArray SPS = MakeNALUnit(33, 0x12, 20);
		const ByteArray IDR = MakeNALUnit(19, 0x13, 3000);
		const ByteArray Trail = MakeNALUnit(1, 0x14, 100);
		const ByteArray IDRMFU = MakeNALUnitMFU(IDR);

		std::uint32_t Sequence = 1000;
		StoreMPU(Assembler, MakeMPUTLV(0x0100, Sequence++, true,
			MakeMPUPayload(0, 10, {MakeNALUnitMFU(AUD), MakeNALUnitMFU(VPS), MakeNALUnitMFU(SPS)})));
		// スライスは 3 つに分割
		StoreMPU(Assembler, MakeMPUTLV(0x0100, Sequence++, false,
			MakeMPUPayload(1, 10, {ByteArray(IDRMFU.begin(), IDRMFU.begin() + 1000)})));
		StoreMPU(Assembler, MakeMPUTLV(0x0100, Sequence++, false,
			MakeMPUPayload(2, 10, {ByteArray(IDRMFU.begin() + 1000, IDRMFU.begin() + 2000)})));
		StoreMPU(Assembler, MakeMPUTLV(0x0100, Sequence++, false,
			MakeMPUPayload(3, 10, {ByteArray(IDRMFU.begin() + 2000, IDRMFU.end())})));
		StoreMPU(Assembler, MakeMPUTLV(0x0100, Sequence++, false,
			MakeMPUPayload(0, 10, {MakeNALUnitMFU(AUD)})));
		StoreMPU(Assembler, MakeMPUTLV(0x0100, Sequence++, false,
			MakeMPUPayload(0, 10, {MakeNALUnitMFU(Trail)})));
		REQUIRE(Recorder.AccessUnits.size() == 1);

		SECTION("Normal") {
			// 次の MPU が始まると前の MPU の最後のアクセスユニットが出力される
			StoreMPU(Assembler, MakeMPUTLV(0x0100, Sequence++, true,
				MakeMPUPayload(0, 11, {MakeNALUnitMFU(AUD)})));
			REQUIRE(Recorder.AccessUnits.size() == 2);

			const AssemblerRecorder::AccessUnit &AU0 = Recorder.AccessUnits[0];
			CHECK(AU0.MPUSequenceNumber == 10);
			CHECK(AU0.Index == 0);
			CHECK(AU0.RandomAccess);
			CHECK_FALSE(AU0.Discontinuity);
			CHECK(AU0.Data == ToAnnexB({AUD, VPS, SPS, IDR}));
			REQUIRE(AU0.HasTimestamp);
			CHECK(AU0.DTS == Base - 3000);
			CHECK(AU0.PTS == Base + 1500);

			const AssemblerRecorder::AccessUnit &AU1 = Recorder.AccessUnits[1];
			CHECK(AU1.Index == 1);
			CHECK_FALSE(AU1.RandomAccess);
			CHECK(AU1.Data == ToAnnexB({AUD, Trail}));
			REQUIRE(AU1.HasTimestamp);
			CHECK(AU1.DTS == Base - 1500);
			CHECK(AU1.PTS == Base);

			// MPU 11 の時刻は無い
			Assembler.Flush();
			REQUIRE(Recorder.AccessUnits.size() == 3);
			CHECK_FALSE(Recorder.AccessUnits[2].HasTimestamp);
			CHECK(Assembler.GetStatistics().OutputCount == 3);
			CHECK(Assembler.GetStatistics().NoTimestampCount == 1);
		}

		SECTION("Loss") {
			// packet_sequence_number が飛ぶと組み立て中のアクセスユニットは捨てる
			Sequence++;
			StoreMPU(Assembler, MakeMPUTLV(0x0100, Sequence++, false,
				MakeMPUPayload(0, 10, {MakeNALUnitMFU(AUD)})));
			StoreMPU(Assembler, MakeMPUTLV(0x0100, Sequence++, false,
				MakeMPUPayload(0, 10, {MakeNALUnitMFU(Trail)})));
			StoreMPU(Assembler, MakeMPUTLV(0x0100, Sequence++, true,
				MakeMPUPayload(0, 11, {MakeNALUnitMFU(AUD)})));
			REQUIRE(Recorder.AccessUnits.size() == 2);
			// MPU 内の順番が分からなくなるので時刻は付けない
			CHECK(Recorder.AccessUnits[1].Discontinuity);
			CHECK_FALSE(Recorder.AccessUnits[1].HasTimestamp);
			CHECK(Assembler.GetStatistics().LossCount == 1);
			CHECK(Assembler.GetStatistics().DroppedCount == 1);

			// 欠落の直後でも、先頭 (RAP_flag) から届いた MPU の順番は分かる
			ByteArray Descriptors11 = MakeMPUTimestampDescriptor(11, Time + (1ULL << 32));
			Append(Descriptors11, MakeMPUExtendedTimestampDescriptor(11, {4500, 1500}));
			MMTDescriptorBlock Block11;
			Block11.ParseBlock(Descriptors11.data(), Descriptors11.size());
			Assembler.UpdateTimestamps(&Block11);
			StoreMPU(Assembler, MakeMPUTLV(0x0100, Sequence++, false,
				MakeMPUPayload(0, 11, {MakeNALUnitMFU(AUD)})));
			REQUIRE(Recorder.AccessUnits.size() == 3);
			CHECK(Recorder.AccessUnits[2].MPUSequenceNumber == 11);
			CHECK(Recorder.AccessUnits[2].Index == 0);
			CHECK_FALSE(Recorder.AccessUnits[2].Discontinuity);
			REQUIRE(Recorder.AccessUnits[2].HasTimestamp);
			CHECK(Recorder.AccessUnits[2].DTS == Base + 90000 - 3000);
		}

		SECTION("Scrambled") {
			StoreMPU(Assembler, MakeMPUTLV(0x0100, Sequence++, false,
				MakeMPUPayload(0, 10, {MakeNALUnitMFU(Trail)}), MMT_SCRAMBLING_CONTROL_EVEN));
			StoreMPU(Assembler, MakeMPUTLV(0x0100, Sequence++, true,
				MakeMPUPayload(0, 11, {MakeNALUnitMFU(AUD)}), MMT_SCRAMBLING_CONTROL_NONE));
			CHECK(Recorder.AccessUnits.size() == 1);
			CHECK(Assembler.GetStatistics().ScrambledCount == 1);
			CHECK(Assembler.GetStatistics().DroppedCount == 1);
		}

		SECTION("Reset") {
			Assembler.Reset();
			Assembler.Flush();
			CHECK(Recorder.AccessUnits.size() == 1);
			CHECK(Assembler.GetTimestampTable().GetMPUCount() == 0);
		}
	}

	SECTION("JoinInMiddle") {
		MPUAssembler Assembler(MPUAssembler::MediaType::Video, &Recorder);
		ByteArray Descriptors = MakeMPUTimestampDescriptor(10, Time);
		Append(Descriptors, MakeMPUExtendedTimestampDescriptor(10, {4500, 1500}));
		MMTDescriptorBlock Block;
		Block.ParseBlock(Descriptors.data(), Descriptors.size());
		Assembler.UpdateTimestamps(&Block);

		// RAP_flag の無いパケットから始まった MPU では MPU 内の順番が分からない
		StoreMPU(Assembler, MakeMPUTLV(0x0100, 1, false, MakeMPUPayload(0, 10, {MakeNALUnitMFU(MakeNALUnit(1, 0, 10))})));
		StoreMPU(Assembler, MakeMPUTLV(0x0100, 2, false, MakeMPUPayload(0, 10, {MakeNALUnitMFU(MakeNALUnit(35, 0x50, 3))})));
		StoreMPU(Assembler, MakeMPUTLV(0x0100, 3, false, MakeMPUPayload(0, 10, {MakeNALUnitMFU(MakeNALUnit(1, 0, 10))})));
		Assembler.Flush();
		REQUIRE(Recorder.AccessUnits.size() == 1);
		CHECK_FALSE(Recorder.AccessUnits[0].HasTimestamp);
	}

	SECTION("Audio") {
		MPUAssembler Assembler(MPUAssembler::MediaType::Audio, &Recorder);
		ByteArray Descriptors = MakeMPUTimestampDescriptor(3, Time);
		ByteArray Extended {0xFB};
		Append32(Extended, 48000);
		Append16(Extended, 1024);
		Append32(Extended, 3);
		Extended.insert(Extended.end(), {0x3F, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
		Append(Descriptors, MakeMMTDescriptor(0x8026, Extended));
		MMTDescriptorBlock Block;
		Block.ParseBlock(Descriptors.data(), Descriptors.size());
		Assembler.UpdateTimestamps(&Block);

		const ByteArray Frame = HexBytes("20000A0B0C0D0E0F");
		StoreMPU(Assembler, MakeMPUTLV(0x0110, 50, true, MakeMPUPayload(0, 3, {Frame})));
		StoreMPU(Assembler, MakeMPUTLV(0x0110, 51, false, MakeMPUPayload(1, 3, {ByteArray(Frame.begin(), Frame.begin() + 3)})));
		StoreMPU(Assembler, MakeMPUTLV(0x0110, 52, false, MakeMPUPayload(3, 3, {ByteArray(Frame.begin() + 3, Frame.end())})));
		StoreMPU(Assembler, MakeMPUTLV(0x0110, 53, false, MakeMPUPayload(0, 3, {Frame})));
		REQUIRE(Recorder.AccessUnits.size() == 3);
		for (int i = 0; i < 3; i++) {
			const AssemblerRecorder::AccessUnit &AU = Recorder.AccessUnits[i];
			CHECK(AU.Index == static_cast<std::uint32_t>(i));
			CHECK(AU.Data == Frame);
			REQUIRE(AU.HasTimestamp);
			CHECK(AU.PTS == Base + i * 1920);
			CHECK(AU.DTS == AU.PTS);
		}
	}

	SECTION("Caption") {
		MPUAssembler Assembler(MPUAssembler::MediaType::Caption, &Recorder);
		ByteArray Descriptors = MakeMPUTimestampDescriptor(20, Time);
		MMTDescriptorBlock Block;
		Block.ParseBlock(Descriptors.data(), Descriptors.size());
		Assembler.UpdateTimestamps(&Block);

		const ByteArray Document = UTF8(u8"<tt><body><div><p><span>字幕</span></p></div></body></tt>");
		const ByteArray Image = HexBytes("89504E47");
		const ByteArray DocumentMFU = MakeCaptionMFU(0, 1, MMT_CAPTION_DATA_TYPE_TTML, Document);
		StoreMPU(Assembler, MakeMPUTLV(0x0130, 7, true,
			MakeMPUPayload(1, 20, {ByteArray(DocumentMFU.begin(), DocumentMFU.begin() + 10)})));
		StoreMPU(Assembler, MakeMPUTLV(0x0130, 8, false,
			MakeMPUPayload(3, 20, {ByteArray(DocumentMFU.begin() + 10, DocumentMFU.end())})));
		CHECK(Recorder.CaptionUnits.empty());
		StoreMPU(Assembler, MakeMPUTLV(0x0130, 9, false,
			MakeMPUPayload(0, 20, {MakeCaptionMFU(1, 1, MMT_CAPTION_DATA_TYPE_PNG, Image)})));
		REQUIRE(Recorder.CaptionUnits.size() == 1);
		const AssemblerRecorder::CaptionUnit &Unit = Recorder.CaptionUnits[0];
		CHECK(Unit.MPUSequenceNumber == 20);
		CHECK(Unit.HasPresentationTime);
		CHECK(Unit.PresentationTime == Time);
		REQUIRE(Unit.Subsamples.size() == 2);
		CHECK(Unit.Subsamples[0].first == MMT_CAPTION_DATA_TYPE_TTML);
		CHECK(Unit.Subsamples[0].second == Document);
		CHECK(Unit.Subsamples[1].first == MMT_CAPTION_DATA_TYPE_PNG);
		CHECK(Unit.Subsamples[1].second == Image);

		// サブサンプルが欠けた MPU は捨てる
		StoreMPU(Assembler, MakeMPUTLV(0x0130, 10, true,
			MakeMPUPayload(0, 21, {MakeCaptionMFU(0, 1, MMT_CAPTION_DATA_TYPE_TTML, Document)})));
		StoreMPU(Assembler, MakeMPUTLV(0x0130, 11, true,
			MakeMPUPayload(0, 22, {MakeCaptionMFU(0, 0, MMT_CAPTION_DATA_TYPE_TTML, Document)})));
		REQUIRE(Recorder.CaptionUnits.size() == 2);
		CHECK(Recorder.CaptionUnits[1].MPUSequenceNumber == 22);
		CHECK(Recorder.CaptionUnits[1].Discontinuity);
		CHECK_FALSE(Recorder.CaptionUnits[1].HasPresentationTime);
		CHECK(Assembler.GetStatistics().DroppedCount == 1);
	}

	SECTION("MediaType") {
		CHECK(MPUAssembler::GetMediaTypeFromAssetType(MMT_ASSET_TYPE_HVC1) == MPUAssembler::MediaType::Video);
		CHECK(MPUAssembler::GetMediaTypeFromAssetType(MMT_ASSET_TYPE_MP4A) == MPUAssembler::MediaType::Audio);
		CHECK(MPUAssembler::GetMediaTypeFromAssetType(MMT_ASSET_TYPE_STPP) == MPUAssembler::MediaType::Caption);
		CHECK(MPUAssembler::GetMediaTypeFromAssetType(MMT_ASSET_TYPE_AAPP) == MPUAssembler::MediaType::Generic);
	}
}

TEST_CASE("TTMLCaptionDecoder", "[mmt][caption]")
{
	using namespace LibISDB;

	const std::uint8_t *pDocument = reinterpret_cast<const std::uint8_t *>(TestTTMLDocument);
	const size_t DocumentSize = sizeof(TestTTMLDocument) - 1;

	TTMLCaptionDecoder::ParagraphList List;
	REQUIRE(TTMLCaptionDecoder::Decode(pDocument, DocumentSize, &List));
	REQUIRE(List.size() == 2);

	const String Line1 = LIBISDB_STR("皮膚の&乾燥") LIBISDB_STR(LIBISDB_NEWLINE) LIBISDB_STR("ああ");
	CHECK(List[0].Text == Line1);
	CHECK(List[0].Region == LIBISDB_STR("r1"));
	CHECK(List[0].Begin == 3115994);
	CHECK(List[0].End == TTMLCaptionDecoder::TIME_INVALID);

	// ルビは除き、空白はまとめる
	const String Line2 = LIBISDB_STR("漢字 a b");
	CHECK(List[1].Text == Line2);
	CHECK(List[1].Region == LIBISDB_STR("r2"));
	CHECK(List[1].Begin == 3115994 + 1500);
	CHECK(List[1].End == 3115994 + 4000);

	String Text;
	REQUIRE(TTMLCaptionDecoder::DecodeText(pDocument, DocumentSize, &Text));
	CHECK(Text == Line1 + LIBISDB_STR(LIBISDB_NEWLINE) + Line2);

	auto Decode = [&List](const char *pText) {
		return TTMLCaptionDecoder::Decode(reinterpret_cast<const std::uint8_t *>(pText), std::strlen(pText), &List);
	};

	// 消去 (ARIB STD-B62 第1編 第3部 3.8、ARIB TR-B39 第1部 第3編 8.11.5)
	CHECK(Decode("<?xml version=\"1.0\"?><tt></tt>"));
	CHECK(List.empty());
	CHECK(Decode("<tt><head/><body><div><p><span/></p></div></body></tt>"));
	CHECK(List.empty());

	CHECK_FALSE(Decode("<tt><p a=\"1></p></tt>"));
	CHECK_FALSE(Decode("not xml"));

	std::int64_t Time;
	CHECK(TTMLCaptionDecoder::ParseTime("01:02:03.4", &Time));
	CHECK(Time == 3723400);
	CHECK(TTMLCaptionDecoder::ParseTime("00:00:01", &Time));
	CHECK(Time == 1000);
	CHECK(TTMLCaptionDecoder::ParseTime("2.25s", &Time));
	CHECK(Time == 2250);
	CHECK(TTMLCaptionDecoder::ParseTime("1.5m", &Time));
	CHECK(Time == 90000);
	CHECK(TTMLCaptionDecoder::ParseTime("40ms", &Time));
	CHECK(Time == 40);
	CHECK_FALSE(TTMLCaptionDecoder::ParseTime("10f", &Time));
	CHECK_FALSE(TTMLCaptionDecoder::ParseTime("1:2", &Time));
}

namespace
{
	class TTMLCaptionRecorder
		: public LibISDB::CaptionFilter::Handler
	{
	public:
		int LanguageUpdateCount = 0;
		std::vector<LibISDB::CaptionFilter::TTMLCaptionInfo> Captions;
		std::vector<ByteArray> Documents;

		void OnLanguageUpdate(LibISDB::CaptionFilter *pFilter, LibISDB::CaptionParser *pParser) override
		{
			CHECK(pParser == nullptr);
			LanguageUpdateCount++;
		}

		void OnTTMLCaption(LibISDB::CaptionFilter *pFilter, const LibISDB::CaptionFilter::TTMLCaptionInfo &Info) override
		{
			Captions.push_back(Info);
			Documents.emplace_back(Info.pDocument, Info.pDocument + Info.DocumentSize);
		}
	};

	// 字幕付加識別情報 (ARIB STD-B60 表 9-3): jpn / type 0 / OPM 01 / TMD 0010 / DMF 1010 / 3840x2160
	ByteArray MakeSubtitleInfoDescriptor(std::uint8_t SubtitleTag)
	{
		ByteArray Body;
		Append16(Body, 0x0020);
		Body.push_back(SubtitleTag);
		Body.push_back(0x00);
		Append(Body, UTF8(u8"jpn"));
		Body.push_back(0x01);
		Body.push_back(0x2A);
		Body.push_back(0x10);
		Append32(Body, TEST_NTP_SECONDS);
		Append32(Body, 0x00000000);
		Body.push_back(0x00);
		return MakeMMTDescriptor(0x8020, Body);
	}
}

TEST_CASE("CaptionFilterMMT", "[mmt][caption][filter]")
{
	using namespace LibISDB;

	auto pParser = std::make_unique<StreamPacketParserFilter>();
	auto pCaption = std::make_unique<CaptionFilter>();
	TTMLCaptionRecorder Recorder;
	pParser->SetOutputFilter(pCaption.get(), pCaption->GetInputSink());
	pCaption->SetCaptionHandler(&Recorder);
	REQUIRE(pParser->StartStreaming());

	std::vector<ByteArray> Packets;
	std::uint32_t Sequence = 0;

	ByteArray MPT {0xFC, 0x02};
	Append16(MPT, 0x0400);
	Append16(MPT, 0x0000);
	MPT.push_back(3);
	Append(MPT, MakeAsset(0x0100, MMT_ASSET_TYPE_HEV1, 0x0000));
	Append(MPT, MakeAsset(0x0138, MMT_ASSET_TYPE_STPP, 0x0038, MakeSubtitleInfoDescriptor(0x38)));
	Append(MPT, MakeAsset(0x0130, MMT_ASSET_TYPE_STPP, 0x0030,
		Concat({MakeMPUTimestampDescriptor(40, static_cast<std::uint64_t>(TEST_NTP_SECONDS) << 32), MakeSubtitleInfoDescriptor(0x30)})));

	Packets.push_back(MakeMMTPTLV(0x0000, Sequence++, 0x02,
		MakeSignalingPayload(0, MakePAMessage(MakeMMTTable(0x80, 1, MakePLTPayload(0x0400, 0x0F00))))));
	Packets.push_back(MakeMMTPTLV(0x0F00, Sequence++, 0x02,
		MakeSignalingPayload(0, MakePAMessage(MakeMMTTable(0x20, 1, MPT)))));

	const ByteArray Document1 = ByteArray(TestTTMLDocument, TestTTMLDocument + sizeof(TestTTMLDocument) - 1);
	const ByteArray Document2 = UTF8(u8"<tt><body><div><p><span>スーパー</span></p></div></body></tt>");
	Packets.push_back(MakeMPUTLV(0x0130, 100, true,
		MakeMPUPayload(0, 40, {MakeCaptionMFU(0, 0, MMT_CAPTION_DATA_TYPE_TTML, Document1)})));
	Packets.push_back(MakeMPUTLV(0x0138, 200, true,
		MakeMPUPayload(0, 50, {MakeCaptionMFU(0, 0, MMT_CAPTION_DATA_TYPE_TTML, Document2)})));

	SECTION("Default") {
		pCaption->SetActiveServiceID(0x0400);
		InputFilter(*pParser, Concat(Packets), 4096);

		CHECK(Recorder.LanguageUpdateCount == 1);
		CHECK(pCaption->GetLanguageCount() == 1);
		CHECK(pCaption->GetLanguageCode(0) == LANGUAGE_CODE_JPN);

		// 既定は component_tag の最も小さい字幕、文字スーパーは同時に扱う
		REQUIRE(Recorder.Captions.size() == 2);
		const CaptionFilter::TTMLCaptionInfo &Info = Recorder.Captions[0];
		CHECK(Info.PacketID == 0x0130);
		CHECK(Info.ComponentTag == 0x0030);
		CHECK_FALSE(Info.Superimpose);
		CHECK(Recorder.Captions[1].PacketID == 0x0138);
		CHECK(Recorder.Captions[1].Superimpose);
		CHECK(Recorder.Documents[1] == Document2);
		REQUIRE(Info.HasSubtitleInfo);
		CHECK(Info.SubtitleInfo.TMD == MHDataComponentDescriptor::TMD_REFERENCE_START_TIME);
		CHECK(Info.SubtitleInfo.DMF == 0xA);
		CHECK(Info.SubtitleInfo.Resolution == 1);
		CHECK(Info.SubtitleInfo.ReferenceStartTime == static_cast<std::uint64_t>(TEST_NTP_SECONDS) << 32);
		CHECK(Info.MPUSequenceNumber == 40);
		CHECK(Info.HasPresentationTime);
		CHECK(Recorder.Documents[0] == Document1);
	}

	SECTION("Superimpose") {
		pCaption->SetTargetStream(0x0400, 0x38);
		InputFilter(*pParser, Concat(Packets), 4096);

		REQUIRE(Recorder.Captions.size() == 1);
		CHECK(Recorder.Captions[0].PacketID == 0x0138);
		CHECK(Recorder.Captions[0].Superimpose);
		CHECK_FALSE(Recorder.Captions[0].HasPresentationTime);
		CHECK(Recorder.Documents[0] == Document2);
	}

	SECTION("SuperimposeDisabled") {
		pCaption->SetSuperimposeEnabled(false);
		pCaption->SetActiveServiceID(0x0400);
		InputFilter(*pParser, Concat(Packets), 4096);

		REQUIRE(Recorder.Captions.size() == 1);
		CHECK(Recorder.Captions[0].PacketID == 0x0130);

		// 有効にすると文字スーパーも届く
		pCaption->SetSuperimposeEnabled(true);
		InputFilter(*pParser, MakeMPUTLV(0x0138, 201, true,
			MakeMPUPayload(0, 51, {MakeCaptionMFU(0, 0, MMT_CAPTION_DATA_TYPE_TTML, Document2)})), 4096);
		REQUIRE(Recorder.Captions.size() == 2);
		CHECK(Recorder.Captions[1].PacketID == 0x0138);
		CHECK(Recorder.Captions[1].MPUSequenceNumber == 51);
	}

	SECTION("NoTarget") {
		InputFilter(*pParser, Concat(Packets), 4096);
		CHECK(Recorder.Captions.empty());
		CHECK(Recorder.LanguageUpdateCount == 0);
		CHECK(pCaption->GetLanguageCount() == 0);
	}

	SECTION("Reset") {
		pCaption->SetActiveServiceID(0x0400);
		InputFilter(*pParser, Concat(Packets), 4096);
		pCaption->Reset();
		CHECK(pCaption->GetLanguageCount() == 0);
		InputFilter(*pParser, Packets[2], 4096);
		CHECK(Recorder.Captions.size() == 2);
	}
}

namespace
{
	class EPGEventRecorder
		: public LibISDB::EPGDatabase::EventListener
	{
	public:
		std::vector<std::string> Events;

		void OnServiceCompleted(
			LibISDB::EPGDatabase *pEPGDatabase,
			std::uint16_t NetworkID, std::uint16_t TransportStreamID, std::uint16_t ServiceID,
			bool IsExtended) override
		{
			Events.push_back(IsExtended ? "extended" : "basic");
		}
	};

	ByteArray MakeEITScheduleSection(
		std::uint8_t TableID, std::uint8_t SectionNumber, std::uint8_t LastSectionNumber,
		std::uint8_t LastTableID, const ByteArray &Events)
	{
		ByteArray Payload;
		Append16(Payload, 0xB110);
		Append16(Payload, 0x000B);
		Payload.push_back(SectionNumber);
		Payload.push_back(LastTableID);
		Append(Payload, Events);
		return MakeSection(TableID, 0x0400, 4, SectionNumber, LastSectionNumber, Payload);
	}
}

TEST_CASE("EPGDatabaseMMT", "[mmt][epg]")
{
	using namespace LibISDB;

	auto pParser = std::make_unique<StreamPacketParserFilter>();
	auto pEPGFilter = std::make_unique<EPGDatabaseFilter>();
	EPGDatabase Database;
	EPGEventRecorder Recorder;
	Database.SetNoPastEvents(false);
	Database.AddEventListener(&Recorder);
	pEPGFilter->SetEPGDatabase(&Database);
	pParser->SetOutputFilter(pEPGFilter.get(), pEPGFilter->GetInputSink());
	REQUIRE(pParser->StartStreaming());

	std::vector<ByteArray> Packets;
	std::uint32_t Sequence = 0;
	auto Signaling = [&](std::uint16_t PacketID, const ByteArray &Message) {
		Packets.push_back(MakeMMTPTLV(PacketID, Sequence++, 0x02, MakeSignalingPayload(0, Message)));
	};

	// 2026/10/29 05:19:16
	Signaling(0x8005, MakeMessage16(0x8002, MakeShortSection(0xA1, HexBytes("EF9E051916F000"))));
	Signaling(0x8000, MakeMessage16(0x8000, MakeSection(0x8B, 0x0400, 2, 0, 1, MakeEITPfPayload(0x1234))));

	// schedule basic : 現在時刻のセグメント (1) から 31 まで 1 セクションずつ
	ByteArray ScheduleEvent;
	Append16(ScheduleEvent, 0x2000);
	Append(ScheduleEvent, HexBytes("EF9E070000"));
	Append(ScheduleEvent, HexBytes("003000"));
	ByteArray ShortEvent = UTF8(u8"jpn");
	const ByteArray ScheduleName = UTF8(u8"予定");
	ShortEvent.push_back(static_cast<std::uint8_t>(ScheduleName.size()));
	Append(ShortEvent, ScheduleName);
	Append16(ShortEvent, 0x0000);
	ByteArray ScheduleDescriptors = MakeMMTDescriptor(0xF001, ShortEvent);
	Append16(ScheduleEvent, static_cast<std::uint16_t>(ScheduleDescriptors.size()));
	Append(ScheduleEvent, ScheduleDescriptors);
	for (std::uint8_t Segment = 1; Segment < 32; Segment++) {
		Signaling(0x8000, MakeMessage16(0x8000, MakeEITScheduleSection(
			0x8C, Segment * 8, 31 * 8, 0x8C, (Segment == 2) ? ScheduleEvent : ByteArray())));
	}

	// schedule extended : 拡張形式イベント記述子だけ
	ByteArray ExtendedEvent;
	Append16(ExtendedEvent, 0x2000);
	Append(ExtendedEvent, HexBytes("EF9E070000"));
	Append(ExtendedEvent, HexBytes("003000"));
	ByteArray ExtendedBody {0x00};
	Append(ExtendedBody, UTF8(u8"jpn"));
	ByteArray Item {0x06};
	Append(Item, UTF8(u8"内容"));
	Append16(Item, 6);
	Append(Item, UTF8(u8"詳細"));
	Append16(ExtendedBody, static_cast<std::uint16_t>(Item.size()));
	Append(ExtendedBody, Item);
	Append16(ExtendedBody, 0x0000);
	const ByteArray ExtendedDescriptor = MakeMMTDescriptor(0xF002, ExtendedBody);
	Append16(ExtendedEvent, static_cast<std::uint16_t>(ExtendedDescriptor.size()));
	Append(ExtendedEvent, ExtendedDescriptor);
	Signaling(0x8000, MakeMessage16(0x8000, MakeEITScheduleSection(0x94, 16, 16, 0x94, ExtendedEvent)));

	InputFilter(*pParser, Concat(Packets), 4096);

	EPGDatabase::ServiceList Services;
	REQUIRE(Database.GetServiceList(&Services));
	REQUIRE(Services.size() == 1);
	CHECK(Services[0].NetworkID == 0x000B);
	CHECK(Services[0].TransportStreamID == 0xB110);
	CHECK(Services[0].ServiceID == 0x0400);

	CHECK(Database.IsScheduleComplete(0x000B, 0xB110, 0x0400, false));
	CHECK(Database.HasSchedule(0x000B, 0xB110, 0x0400, true));
	CHECK(Recorder.Events == std::vector<std::string> {"basic"});

	// p/f のイベントは GetMHEventInfo と同じ内容になる
	EventInfo Event;
	REQUIRE(Database.GetEventInfo(0x000B, 0xB110, 0x0400, 0x1234, &Event));
	std::unique_ptr<MMTSectionTable> pPfTable(new MMTSectionTable(new MHEITPfTable));
	{
		TLVPacket Packet;
		ToTLVPacket(MakeMMTPTLV(0x8000, 0, 0x02,
			MakeSignalingPayload(0, MakeMessage16(0x8000, MakeSection(0x8B, 0x0400, 2, 0, 1, MakeEITPfPayload(0x1234))))), &Packet);
		pPfTable->StorePacket(&Packet);
	}
	EventInfo Expected;
	REQUIRE(GetMHEventInfo(pPfTable->GetTable<MHEITPfTable>()->GetPfTable(0x0400), 0, &Expected));
	CHECK(Event.EventName == Expected.EventName);
	CHECK(Event.EventText == Expected.EventText);
	CHECK(Event.ExtendedText == Expected.ExtendedText);
	CHECK(Event.VideoList == Expected.VideoList);
	CHECK(Event.AudioList == Expected.AudioList);
	CHECK(Event.ContentNibble == Expected.ContentNibble);
	CHECK(Event.EventGroupList == Expected.EventGroupList);
	CHECK(Event.IsCommonEvent == Expected.IsCommonEvent);
	CHECK(Event.IsPresent());

	// schedule の基本情報と拡張情報がまとまる
	REQUIRE(Database.GetEventInfo(0x000B, 0xB110, 0x0400, 0x2000, &Event));
	CHECK(Event.EventName == LIBISDB_STR("予定"));
	CHECK(Event.StartTime.Hour == 7);
	CHECK(Event.StartTime.Minute == 0);
	CHECK(Event.Duration == 30 * 60);
	REQUIRE(Event.ExtendedText.size() == 1);
	CHECK(Event.ExtendedText[0].Description == LIBISDB_STR("内容"));
	CHECK(Event.ExtendedText[0].Text == LIBISDB_STR("詳細"));
	CHECK(!!(Event.Type & EventInfo::TypeFlag::Basic));
	CHECK(!!(Event.Type & EventInfo::TypeFlag::Extended));

	SECTION("TableID") {
		CHECK(MHEITTableIDToEITTableID(0x8B) == 0x4E);
		CHECK(MHEITTableIDToEITTableID(0x8C) == 0x50);
		CHECK(MHEITTableIDToEITTableID(0x94) == 0x58);
		CHECK(MHEITTableIDToEITTableID(0x9B) == 0x5F);
		CHECK(MHEITTableIDToEITTableID(0x9C) == 0x00);
	}

	SECTION("TS") {
		auto pTSParser = std::make_unique<StreamPacketParserFilter>();
		auto pTSEPGFilter = std::make_unique<EPGDatabaseFilter>();
		EPGDatabase TSDatabase;
		pTSEPGFilter->SetEPGDatabase(&TSDatabase);
		pTSParser->SetOutputFilter(pTSEPGFilter.get(), pTSEPGFilter->GetInputSink());
		REQUIRE(pTSParser->StartStreaming());
		InputFilter(*pTSParser, MakeTSStream(500), 4096);
		CHECK(TSDatabase.GetServiceCount() == 0);
	}
}


#include "../LibISDB/MediaParsers/LATMParser.hpp"
#include "../LibISDB/MMT/MMTMediaReceiver.hpp"
#include "../LibISDB/Engine/TSEngine.hpp"

namespace
{
	class BitWriter
	{
	public:
		void Put(std::uint32_t Value, int Bits)
		{
			while (Bits-- > 0) {
				if ((m_BitCount & 7) == 0)
					m_Data.push_back(0);
				if ((Value >> Bits) & 1)
					m_Data.back() |= static_cast<std::uint8_t>(0x80 >> (m_BitCount & 7));
				m_BitCount++;
			}
		}

		void PutBytes(const ByteArray &Data)
		{
			for (std::uint8_t Byte : Data)
				Put(Byte, 8);
		}

		size_t GetBitCount() const noexcept { return m_BitCount; }
		const ByteArray & GetData() const noexcept { return m_Data; }

	private:
		ByteArray m_Data;
		size_t m_BitCount = 0;
	};

	// AudioSpecificConfig() (AAC LC、GASpecificConfig() の拡張無し)
	void PutAudioSpecificConfig(BitWriter &Writer, std::uint8_t SamplingFrequencyIndex, std::uint8_t ChannelConfiguration)
	{
		Writer.Put(2, 5);
		Writer.Put(SamplingFrequencyIndex, 4);
		Writer.Put(ChannelConfiguration, 4);
		Writer.Put(0, 3); // frameLengthFlag, dependsOnCoreCoder, extensionFlag
	}

	// StreamMuxConfig() (audioMuxVersion = 0) の後の部分
	void PutStreamMuxConfigTail(BitWriter &Writer)
	{
		Writer.Put(0, 3);    // frameLengthType
		Writer.Put(0xFF, 8); // latmBufferFullness
		Writer.Put(0, 1);    // otherDataPresent
		Writer.Put(0, 1);    // crcCheckPresent
	}

	void PutPayload(BitWriter &Writer, const ByteArray &Payload)
	{
		size_t Length = Payload.size();
		while (Length >= 255) {
			Writer.Put(255, 8);
			Length -= 255;
		}
		Writer.Put(static_cast<std::uint32_t>(Length), 8);
		Writer.PutBytes(Payload);
	}

	ByteArray MakeAudioMuxElement(std::uint8_t ChannelConfiguration, const ByteArray &Payload, bool UseSameStreamMux = false)
	{
		BitWriter Writer;
		Writer.Put(UseSameStreamMux ? 1 : 0, 1);
		if (!UseSameStreamMux) {
			Writer.Put(0, 1);  // audioMuxVersion
			Writer.Put(1, 1);  // allStreamsSameTimeFraming
			Writer.Put(0, 6);  // numSubFrames
			Writer.Put(0, 4);  // numProgram
			Writer.Put(0, 3);  // numLayer
			PutAudioSpecificConfig(Writer, 3, ChannelConfiguration);
			PutStreamMuxConfigTail(Writer);
		}
		PutPayload(Writer, Payload);
		return Writer.GetData();
	}
}

TEST_CASE("LATMParser", "[mediaparser][latm]")
{
	using namespace LibISDB;

	LATMParser Parser;
	const ByteArray Payload = HexBytes("ABCDEF");

	// 48kHz / 2ch の AudioSpecificConfig() は 0x1190
	const ByteArray Element = MakeAudioMuxElement(2, Payload);
	REQUIRE(Parser.ParseAudioMuxElement(Element.data(), Element.size()));
	CHECK(Parser.HasConfig());
	CHECK(Parser.IsConfigChanged());
	CHECK(Parser.GetConfig().AudioObjectType == 2);
	CHECK(Parser.GetConfig().SamplingFrequencyIndex == 3);
	CHECK(Parser.GetConfig().SamplingFrequency == 48000);
	CHECK(Parser.GetConfig().ChannelConfiguration == 2);
	CHECK(Parser.GetConfig().SpecificConfig == HexBytes("1190"));
	// ペイロードはバイト境界から始まらない (53 ビット目から)
	CHECK(ByteArray(Parser.GetPayloadData(), Parser.GetPayloadData() + Parser.GetPayloadSize()) == Payload);

	SECTION("SameStreamMux") {
		const ByteArray Same = MakeAudioMuxElement(2, HexBytes("0102"), true);
		REQUIRE(Parser.ParseAudioMuxElement(Same.data(), Same.size()));
		CHECK_FALSE(Parser.IsConfigChanged());
		CHECK(ByteArray(Parser.GetPayloadData(), Parser.GetPayloadData() + Parser.GetPayloadSize()) == HexBytes("0102"));

		// 同じ設定を再び送っても変化ではない
		REQUIRE(Parser.ParseAudioMuxElement(Element.data(), Element.size()));
		CHECK_FALSE(Parser.IsConfigChanged());

		const ByteArray Surround = MakeAudioMuxElement(6, Payload);
		REQUIRE(Parser.ParseAudioMuxElement(Surround.data(), Surround.size()));
		CHECK(Parser.IsConfigChanged());
		CHECK(Parser.GetConfig().ChannelConfiguration == 6);
	}

	SECTION("LongPayload") {
		ByteArray Long(300);
		for (size_t i = 0; i < Long.size(); i++)
			Long[i] = static_cast<std::uint8_t>(i * 7);
		const ByteArray LongElement = MakeAudioMuxElement(2, Long);
		REQUIRE(Parser.ParseAudioMuxElement(LongElement.data(), LongElement.size()));
		CHECK(ByteArray(Parser.GetPayloadData(), Parser.GetPayloadData() + Parser.GetPayloadSize()) == Long);
	}

	SECTION("ProgramConfigElement") {
		// channelConfiguration = 0 + 2 つの SCE の program_config_element() (二重音声)
		BitWriter Writer;
		Writer.Put(0, 1);
		Writer.Put(0, 1);
		Writer.Put(1, 1);
		Writer.Put(0, 6 + 4 + 3);
		const size_t ConfigPos = Writer.GetBitCount();
		Writer.Put(2, 5);
		Writer.Put(3, 4);
		Writer.Put(0, 4);
		Writer.Put(0, 3);
		Writer.Put(0, 4);   // element_instance_tag
		Writer.Put(1, 2);   // object_type
		Writer.Put(3, 4);   // sampling_frequency_index
		Writer.Put(2, 4);   // num_front_channel_elements
		Writer.Put(0, 4 + 4 + 2 + 3 + 4);
		Writer.Put(0, 3);   // mono / stereo / matrix mixdown
		Writer.Put(0x00, 5);
		Writer.Put(0x01, 5);
		// byte_alignment() は AudioSpecificConfig() の先頭から
		while ((Writer.GetBitCount() - ConfigPos) % 8 != 0)
			Writer.Put(0, 1);
		Writer.Put(1, 8);   // comment_field_bytes
		Writer.Put(0x41, 8);
		const size_t ConfigBits = Writer.GetBitCount() - ConfigPos;
		PutStreamMuxConfigTail(Writer);
		PutPayload(Writer, Payload);

		REQUIRE(Parser.ParseAudioMuxElement(Writer.GetData().data(), Writer.GetData().size()));
		CHECK(Parser.IsConfigChanged());
		CHECK(Parser.GetConfig().ChannelConfiguration == 0);
		CHECK(Parser.GetConfig().SpecificConfig.size() == (ConfigBits + 7) / 8);
		CHECK(ByteArray(Parser.GetPayloadData(), Parser.GetPayloadData() + Parser.GetPayloadSize()) == Payload);
	}

	SECTION("AudioMuxVersion1") {
		BitWriter Writer;
		Writer.Put(0, 1);
		Writer.Put(1, 1);   // audioMuxVersion
		Writer.Put(0, 1);   // audioMuxVersionA
		Writer.Put(0, 2);   // taraBufferFullness (bytesForValue)
		Writer.Put(0xFF, 8);
		Writer.Put(1, 1);
		Writer.Put(0, 6 + 4 + 3);
		Writer.Put(0, 2);   // ascLen (bytesForValue)
		Writer.Put(20, 8);  // 16 ビット + fillBits 4 ビット
		PutAudioSpecificConfig(Writer, 3, 1);
		Writer.Put(0, 4);
		PutStreamMuxConfigTail(Writer);
		PutPayload(Writer, Payload);

		REQUIRE(Parser.ParseAudioMuxElement(Writer.GetData().data(), Writer.GetData().size()));
		CHECK(Parser.GetConfig().ChannelConfiguration == 1);
		CHECK(Parser.GetConfig().SpecificConfig == HexBytes("1188"));
		CHECK(ByteArray(Parser.GetPayloadData(), Parser.GetPayloadData() + Parser.GetPayloadSize()) == Payload);
	}

	SECTION("Error") {
		LATMParser NewParser;
		const ByteArray Same = MakeAudioMuxElement(2, Payload, true);
		// 設定がまだ無い
		CHECK_FALSE(NewParser.ParseAudioMuxElement(Same.data(), Same.size()));
		// ペイロードが足りない
		CHECK_FALSE(NewParser.ParseAudioMuxElement(Element.data(), Element.size() - 1));
		CHECK_FALSE(NewParser.ParseAudioMuxElement(Element.data(), 2));
		// numSubFrames が 0 でない
		BitWriter Writer;
		Writer.Put(0, 2);
		Writer.Put(1, 1);
		Writer.Put(1, 6);
		Writer.Put(0, 7);
		PutAudioSpecificConfig(Writer, 3, 2);
		PutStreamMuxConfigTail(Writer);
		PutPayload(Writer, Payload);
		CHECK_FALSE(NewParser.ParseAudioMuxElement(Writer.GetData().data(), Writer.GetData().size()));
		CHECK_FALSE(NewParser.HasConfig());
		CHECK_FALSE(NewParser.ParseAudioMuxElement(nullptr, 0));
	}
}

namespace
{
	ByteArray MakeAudioComponentDescriptor(std::uint16_t ComponentTag, std::uint8_t ComponentType)
	{
		ByteArray Body {0xF3, ComponentType};
		Append16(Body, ComponentTag);
		Append(Body, HexBytes("11FF5F6A706E"));
		return MakeMMTDescriptor(0x8014, Body);
	}

	// 映像 0x0100 / 音声 0x0110, 0x0111 の MPT (MPU 10 / 3 の時刻を持つ)
	ByteArray MakeMediaMPTPayload(std::uint64_t Time)
	{
		ByteArray Payload {0xFC, 0x02};
		Append16(Payload, 0x0400);
		Append16(Payload, 0x0000);
		ByteArray Video = MakeMPUTimestampDescriptor(10, Time);
		Append(Video, MakeMPUExtendedTimestampDescriptor(10, {4500, 1500}));
		ByteArray Audio = MakeMPUTimestampDescriptor(3, Time);
		ByteArray AudioExtended {0xFB};
		Append32(AudioExtended, 48000);
		Append16(AudioExtended, 1024);
		Append32(AudioExtended, 3);
		AudioExtended.insert(AudioExtended.end(), {0x3F, 0x00, 0x00, 0x01, 0x00, 0x00});
		Append(Audio, MakeMMTDescriptor(0x8026, AudioExtended));
		Append(Audio, MakeAudioComponentDescriptor(0x0010, 0x03));
		ByteArray Audio2 = MakeAudioComponentDescriptor(0x0011, 0x09);
		const ByteArray Assets[] = {
			MakeAsset(0x0100, LibISDB::MMT_ASSET_TYPE_HEV1, 0x0000, Video),
			MakeAsset(0x0111, LibISDB::MMT_ASSET_TYPE_MP4A, 0x0011, Audio2),
			MakeAsset(0x0110, LibISDB::MMT_ASSET_TYPE_MP4A, 0x0010, Audio),
		};
		Payload.push_back(static_cast<std::uint8_t>(std::size(Assets)));
		for (const ByteArray &e : Assets)
			Append(Payload, e);
		return Payload;
	}

	class MediaReceiverRecorder
		: public LibISDB::MMTMediaReceiver::Handler
	{
	public:
		struct AccessUnit {
			LibISDB::MMTMediaReceiver::MediaType Type;
			std::uint16_t PacketID;
			bool HasTimestamp;
			std::int64_t PTS;
			bool RandomAccess;
			std::int64_t NTPTime;
			ByteArray Data;
		};

		std::vector<AccessUnit> AccessUnits;
		std::vector<int> LeapSeconds;

		void OnAccessUnit(
			const LibISDB::MMTMediaReceiver *pReceiver, LibISDB::MMTMediaReceiver::MediaType Type,
			const LibISDB::MPUAssembler::AccessUnitInfo &Info) override
		{
			AccessUnits.push_back({
				Type, Info.PacketID, Info.HasTimestamp, Info.PTS, Info.RandomAccess,
				pReceiver->GetNTPTime90kHz(), ByteArray(Info.pData, Info.pData + Info.DataSize)});
		}

		void OnNTPLeapSecond(const LibISDB::MMTMediaReceiver *pReceiver, int Seconds) override
		{
			LeapSeconds.push_back(Seconds);
		}
	};

	class MediaReceiverSink
		: public LibISDB::SingleInputFilter
	{
	public:
		MediaReceiverSink(LibISDB::MMTMediaReceiver &Receiver) : m_Receiver(Receiver) {}

		const LibISDB::CharType * GetObjectName() const noexcept override { return LIBISDB_STR("MediaReceiverSink"); }

		bool ReceiveData(LibISDB::DataStream *pData) override
		{
			m_Receiver.StorePacketStream(pData);
			return true;
		}

	private:
		LibISDB::MMTMediaReceiver &m_Receiver;
	};

	// MPT / NTP と、映像・音声の MPU の TLV パケット列
	struct MediaStreamPackets {
		ByteArray Signaling;
		ByteArray Media;
	};

	MediaStreamPackets MakeMediaStreamPackets(std::uint64_t MPTTime, std::uint64_t NTPTime)
	{
		MediaStreamPackets Packets;
		std::vector<ByteArray> Signaling;
		Signaling.push_back(MakeMMTPTLV(0x0000, 0, 0x02,
			MakeSignalingPayload(0, MakePAMessage(MakeMMTTable(0x80, 1, MakePLTPayload(0x0400, 0x0F00))))));
		Signaling.push_back(MakeMMTPTLV(0x0F00, 0, 0x02,
			MakeSignalingPayload(0, MakePAMessage(MakeMMTTable(0x20, 1, MakeMediaMPTPayload(MPTTime))))));
		Signaling.push_back(MakeTLV(0x02, MakeIPv6UDP(456, 123, MakeNTP(NTPTime))));
		// 最後のパケットは次の TLV パケットの同期バイトを見るまで出力されない
		for (int i = 0; i < 4; i++)
			Signaling.push_back(MakeTLV(0xFF, ByteArray(16, 0xFF)));
		Packets.Signaling = Concat(Signaling);

		const ByteArray AUD = MakeNALUnit(35, 0x50, 3);
		std::vector<ByteArray> Media;
		Media.push_back(MakeMPUTLV(0x0100, 1000, true,
			MakeMPUPayload(0, 10, {MakeNALUnitMFU(AUD), MakeNALUnitMFU(MakeNALUnit(19, 0x13, 300))})));
		Media.push_back(MakeMPUTLV(0x0100, 1001, false,
			MakeMPUPayload(0, 10, {MakeNALUnitMFU(AUD), MakeNALUnitMFU(MakeNALUnit(1, 0x14, 100))})));
		Media.push_back(MakeMPUTLV(0x0110, 50, true, MakeMPUPayload(0, 3, {HexBytes("20000A0B")})));
		Media.push_back(MakeMPUTLV(0x0111, 70, true, MakeMPUPayload(0, 3, {HexBytes("20000C0D")})));
		Media.push_back(MakeMPUTLV(0x0100, 1002, true,
			MakeMPUPayload(0, 11, {MakeNALUnitMFU(AUD)})));
		for (int i = 0; i < 4; i++)
			Media.push_back(MakeTLV(0xFF, ByteArray(16, 0xFF)));
		Packets.Media = Concat(Media);

		return Packets;
	}
}

TEST_CASE("MMTMediaReceiver", "[mmt][viewer]")
{
	using namespace LibISDB;

	auto pParser = std::make_unique<StreamPacketParserFilter>();
	MMTMediaReceiver Receiver;
	MediaReceiverRecorder Recorder;
	MediaReceiverSink Sink(Receiver);
	Receiver.SetHandler(&Recorder);
	pParser->SetOutputFilter(&Sink, Sink.GetInputSink());
	REQUIRE(pParser->StartStreaming());

	const std::uint64_t Time = static_cast<std::uint64_t>(TEST_NTP_SECONDS) << 32;
	const std::int64_t Base = static_cast<std::int64_t>(TEST_NTP_SECONDS) * 90000;
	// NTP の時刻は提示時刻の 0.5 秒前
	const MediaStreamPackets Packets = MakeMediaStreamPackets(Time, Time - 0x80000000ULL);

	SECTION("NoTarget") {
		InputFilter(*pParser, Packets.Signaling, 4096);
		InputFilter(*pParser, Packets.Media, 4096);
		CHECK(Recorder.AccessUnits.empty());
		CHECK(Receiver.HasNTPTime());
		CHECK(Receiver.GetNTPTime90kHz() == Base - 45000);
	}

	SECTION("Selected") {
		Receiver.SetVideoPacketID(0x0100);
		Receiver.SetAudioPacketID(0x0110);
		CHECK(Receiver.GetAssembler(MMTMediaReceiver::MediaType::Video) == nullptr);
		InputFilter(*pParser, Packets.Signaling, 4096);
		REQUIRE(Receiver.GetAssembler(MMTMediaReceiver::MediaType::Video) != nullptr);
		CHECK(Receiver.GetAssembler(MMTMediaReceiver::MediaType::Video)->GetMediaType() == MPUAssembler::MediaType::Video);
		REQUIRE(Receiver.GetAssembler(MMTMediaReceiver::MediaType::Audio) != nullptr);
		CHECK(Receiver.GetAssembler(MMTMediaReceiver::MediaType::Audio)->GetMediaType() == MPUAssembler::MediaType::Audio);
		InputFilter(*pParser, Packets.Media, 4096);

		// 映像のアクセスユニットは次の AUD で確定する
		REQUIRE(Recorder.AccessUnits.size() == 3);
		CHECK(Recorder.AccessUnits[0].Type == MMTMediaReceiver::MediaType::Video);
		CHECK(Recorder.AccessUnits[0].PacketID == 0x0100);
		CHECK(Recorder.AccessUnits[0].RandomAccess);
		CHECK(Recorder.AccessUnits[0].HasTimestamp);
		CHECK(Recorder.AccessUnits[0].PTS == Base + 1500);
		CHECK(Recorder.AccessUnits[0].NTPTime == Base - 45000);
		CHECK(Recorder.AccessUnits[1].Type == MMTMediaReceiver::MediaType::Audio);
		CHECK(Recorder.AccessUnits[1].PacketID == 0x0110);
		CHECK(Recorder.AccessUnits[1].Data == HexBytes("20000A0B"));
		CHECK(Recorder.AccessUnits[1].HasTimestamp);
		CHECK(Recorder.AccessUnits[1].PTS == Base);
		CHECK(Recorder.AccessUnits[2].Type == MMTMediaReceiver::MediaType::Video);
		CHECK_FALSE(Recorder.AccessUnits[2].RandomAccess);

		// 音声を切り替えると、前のアセットの組み立ては止め、最後の MPT で新しいアセットの組み立てを始める
		Recorder.AccessUnits.clear();
		Receiver.SetAudioPacketID(0x0111);
		REQUIRE(Receiver.GetAssembler(MMTMediaReceiver::MediaType::Audio) != nullptr);
		CHECK(Receiver.GetAssembler(MMTMediaReceiver::MediaType::Audio)->GetPacketID() != 0x0110);
		InputFilter(*pParser, Concat({
			MakeMPUTLV(0x0110, 51, true, MakeMPUPayload(0, 3, {HexBytes("20000A0B")})),
			MakeMPUTLV(0x0111, 71, true, MakeMPUPayload(0, 3, {HexBytes("20000C0D")})),
			MakeTLV(0xFF, ByteArray(16, 0xFF))}), 4096);
		REQUIRE(Recorder.AccessUnits.size() == 1);
		CHECK(Recorder.AccessUnits[0].PacketID == 0x0111);
		// MPU 3 の時刻は 0x0111 には無い
		CHECK_FALSE(Recorder.AccessUnits[0].HasTimestamp);
	}

	SECTION("TimestampDeviation") {
		// NTP の時刻から 10 秒以上離れた時刻は無効にする
		const MediaStreamPackets Far = MakeMediaStreamPackets(Time, Time - (11ULL << 32));
		Receiver.SetAudioPacketID(0x0110);
		InputFilter(*pParser, Far.Signaling, 4096);
		InputFilter(*pParser, Far.Media, 4096);
		REQUIRE(Recorder.AccessUnits.size() == 1);
		CHECK_FALSE(Recorder.AccessUnits[0].HasTimestamp);
	}

	SECTION("LeapSecond") {
		// 23:59:59 UTC (8:59:59 JST) の繰り返しと飛びを NTP の leap_indicator と合わせて検出する
		const std::uint64_t DaySeconds = 24 * 60 * 60;
		const std::uint64_t LastSecond = (static_cast<std::uint64_t>(TEST_NTP_SECONDS) / DaySeconds + 1) * DaySeconds - 1;
		const auto InputNTP =
			[&](std::uint64_t Seconds, std::uint32_t Fraction, std::uint8_t LeapIndicator) {
				InputFilter(*pParser, Concat({
					MakeTLV(0x02, MakeIPv6UDP(456, 123, MakeNTP((Seconds << 32) | Fraction, LeapIndicator))),
					MakeTLV(0xFF, ByteArray(16, 0xFF))}), 4096);
			};

		// 挿入: 23:59:59.97 -> 23:59:59.00
		InputNTP(LastSecond, 0xF7000000, 1);
		InputNTP(LastSecond, 0x00000000, 1);
		CHECK(Recorder.LeapSeconds == std::vector<int> {-1});
		// 削除: 23:59:58.97 -> 0:00:00.00
		InputNTP(LastSecond - 1, 0xF7000000, 2);
		InputNTP(LastSecond + 1, 0x00000000, 2);
		CHECK(Recorder.LeapSeconds == std::vector<int> {-1, 1});
		// leap_indicator が無ければ時刻の飛びは調整ではない
		InputNTP(LastSecond, 0xF7000000, 0);
		InputNTP(LastSecond, 0x00000000, 0);
		InputNTP(LastSecond - 1, 0xF7000000, 0);
		InputNTP(LastSecond + 1, 0x00000000, 0);
		// 日の境界でない繰り返しは調整ではない
		InputNTP(LastSecond - 10, 0xF7000000, 1);
		InputNTP(LastSecond - 10, 0x00000000, 1);
		CHECK(Recorder.LeapSeconds == std::vector<int> {-1, 1});
		CHECK(Receiver.GetNTPTime() == ((LastSecond - 10) << 32));
	}

	SECTION("Reset") {
		Receiver.SetVideoPacketID(0x0100);
		InputFilter(*pParser, Packets.Signaling, 4096);
		Receiver.Reset();
		CHECK(Receiver.GetVideoPacketID() == MMT_PACKET_ID_INVALID);
		CHECK_FALSE(Receiver.HasNTPTime());
		CHECK(Receiver.GetAssembler(MMTMediaReceiver::MediaType::Video) == nullptr);
		InputFilter(*pParser, Packets.Media, 4096);
		CHECK(Recorder.AccessUnits.empty());
	}
}

namespace
{
	class StreamIDRecorder
		: public LibISDB::SingleIOFilter
	{
	public:
		const LibISDB::CharType * GetObjectName() const noexcept override { return LIBISDB_STR("StreamIDRecorder"); }

		void SetActiveServiceID(std::uint16_t ServiceID) override { this->ServiceID = ServiceID; }
		void SetActiveVideoPID(std::uint16_t PID, bool ServiceChanged) override { VideoPID = PID; }
		void SetActiveAudioPID(std::uint16_t PID, bool ServiceChanged) override { AudioPID = PID; }
		void SetActiveVideoPacketID(std::uint16_t PacketID, bool ServiceChanged) override { VideoPacketID = PacketID; }
		void SetActiveAudioPacketID(std::uint16_t PacketID, bool ServiceChanged) override { AudioPacketID = PacketID; }

		std::uint16_t ServiceID = LibISDB::SERVICE_ID_INVALID;
		std::uint16_t VideoPID = 0;
		std::uint16_t AudioPID = 0;
		std::uint16_t VideoPacketID = 0;
		std::uint16_t AudioPacketID = 0;
	};

	class TestEngine
		: public LibISDB::TSEngine
	{
	public:
		std::vector<std::uint8_t> VideoStreamTypes;
		std::vector<std::uint8_t> AudioStreamTypes;
		std::vector<std::uint16_t> Services;

	protected:
		void OnServiceChanged(std::uint16_t ServiceID) override { Services.push_back(ServiceID); }
		void OnVideoStreamTypeChanged(std::uint8_t StreamType) override { VideoStreamTypes.push_back(StreamType); }
		void OnAudioStreamTypeChanged(std::uint8_t StreamType) override { AudioStreamTypes.push_back(StreamType); }
	};
}

TEST_CASE("TSEngineMMT", "[mmt][engine]")
{
	using namespace LibISDB;

	StreamPacketParserFilter *pParser = new StreamPacketParserFilter;
	AnalyzerFilter *pAnalyzer = new AnalyzerFilter;
	MMTAnalyzerFilter *pMMTAnalyzer = new MMTAnalyzerFilter;
	StreamIDRecorder *pRecorder = new StreamIDRecorder;
	TestEngine Engine;
	REQUIRE(Engine.BuildEngine({pParser, pAnalyzer, pMMTAnalyzer, pRecorder}));
	REQUIRE(pParser->StartStreaming());

	const std::uint64_t Time = static_cast<std::uint64_t>(TEST_NTP_SECONDS) << 32;
	const MediaStreamPackets Packets = MakeMediaStreamPackets(Time, Time);
	InputFilter(*pParser, Packets.Signaling, 4096);

	CHECK(Engine.GetStreamFormat() == StreamFormat::TLV);
	CHECK(Engine.GetServiceID() == 0x0400);
	CHECK(Engine.Services == std::vector<std::uint16_t> {0x0400});
	CHECK(pRecorder->ServiceID == 0x0400);
	// 既定は component_tag の最も小さいアセット
	CHECK(pRecorder->VideoPacketID == 0x0100);
	CHECK(pRecorder->AudioPacketID == 0x0110);
	// MMT では PID は通知しない
	CHECK(pRecorder->VideoPID == 0);
	CHECK(pRecorder->AudioPID == 0);
	CHECK(Engine.GetVideoStreamType() == STREAM_TYPE_H265);
	// 音声は MH-音声コンポーネント記述子の stream_type
	CHECK(Engine.GetAudioStreamType() == STREAM_TYPE_MPEG4_AUDIO);
	// PLT の時点ではアセットが分からないので、MPT で stream_type が変わる
	CHECK(Engine.VideoStreamTypes == std::vector<std::uint8_t> {STREAM_TYPE_INVALID, STREAM_TYPE_H265});
	CHECK(Engine.AudioStreamTypes == std::vector<std::uint8_t> {STREAM_TYPE_INVALID, STREAM_TYPE_MPEG4_AUDIO});
	CHECK(Engine.GetVideoStreamCount() == 1);
	CHECK(Engine.GetAudioStreamCount() == 2);
	CHECK(Engine.GetVideoComponentTag() == 0x00);
	CHECK(Engine.GetAudioComponentTag() == 0x10);
	// tlv_stream_id は MH-SDT が来るまで分からない
	CHECK(Engine.GetTransportStreamID() == TRANSPORT_STREAM_ID_INVALID);

	AnalyzerFilter::ServiceList TSList;
	CHECK_FALSE(Engine.GetSelectableServiceList(&TSList));
	MMTAnalyzerFilter::ServiceList MMTList;
	REQUIRE(Engine.GetSelectableMMTServiceList(&MMTList));
	REQUIRE(MMTList.size() == 1);
	CHECK(MMTList[0].ServiceID == 0x0400);

	SECTION("AudioStream") {
		REQUIRE(Engine.SetAudioStream(1));
		CHECK(pRecorder->AudioPacketID == 0x0111);
		CHECK(Engine.GetAudioComponentTag() == 0x11);
		CHECK_FALSE(Engine.SetAudioStream(2));
	}

	SECTION("MHSDT") {
		InputFilter(*pParser,
			MakeMMTPTLV(0x8004, 0, 0x02, MakeSignalingPayload(0,
				MakeMessage16(0x8000, MakeSection(0x9F, 0xB110, 1, 0, 0, MakeSDTPayload(0x0400, UTF8(u8"テスト"), false))))),
			4096);
		CHECK(Engine.GetTransportStreamID() == 0xB110);
		CHECK(Engine.GetNetworkID() == 0x000B);
		// tlv_stream_id が分かっても選局し直さない
		CHECK(Engine.Services == std::vector<std::uint16_t> {0x0400});
		InputFilter(*pParser, Packets.Signaling, 4096);
		CHECK(Engine.Services == std::vector<std::uint16_t> {0x0400});
	}

	SECTION("FormatChange") {
		pRecorder->AudioPID = 0x1234;
		InputFilter(*pParser, MakeTSStream(500), 4096);
		CHECK(Engine.GetStreamFormat() == StreamFormat::TS);
		CHECK(pRecorder->VideoPacketID == MMT_PACKET_ID_INVALID);
		CHECK(pRecorder->AudioPacketID == MMT_PACKET_ID_INVALID);
		CHECK(pRecorder->AudioPID == PID_INVALID);
		CHECK(Engine.GetServiceID() == SERVICE_ID_INVALID);
		CHECK(Engine.GetVideoStreamType() == STREAM_TYPE_UNINITIALIZED);
		CHECK_FALSE(Engine.GetSelectableMMTServiceList(&MMTList));
	}
}

namespace
{
	struct TestAudioAsset {
		std::uint16_t PacketID;
		std::uint16_t ComponentTag;
		std::uint8_t StreamContent;
		std::uint8_t ComponentType;
		std::uint8_t SimulcastGroupTag;
	};

	// PLT と、映像 0x0100 と指定した音声アセットの MPT
	ByteArray MakeAudioSelectionSignaling(std::uint8_t MPTVersion, std::initializer_list<TestAudioAsset> AudioAssets)
	{
		ByteArray MPT {0xFC, 0x02};
		Append16(MPT, 0x0400);
		Append16(MPT, 0x0000);
		MPT.push_back(static_cast<std::uint8_t>(1 + AudioAssets.size()));
		Append(MPT, MakeAsset(0x0100, LibISDB::MMT_ASSET_TYPE_HEV1, 0x0000));
		for (const TestAudioAsset &e : AudioAssets) {
			ByteArray Body {static_cast<std::uint8_t>(0xF0 | e.StreamContent), e.ComponentType};
			Append16(Body, e.ComponentTag);
			Body.push_back(0x11);
			Body.push_back(e.SimulcastGroupTag);
			Append(Body, HexBytes("5F6A706E"));
			Append(MPT, MakeAsset(e.PacketID, LibISDB::MMT_ASSET_TYPE_MP4A, e.ComponentTag, MakeMMTDescriptor(0x8014, Body)));
		}

		std::vector<ByteArray> Packets;
		Packets.push_back(MakeMMTPTLV(0x0000, 0, 0x02,
			MakeSignalingPayload(0, MakePAMessage(MakeMMTTable(0x80, 1, MakePLTPayload(0x0400, 0x0F00))))));
		Packets.push_back(MakeMMTPTLV(0x0F00, MPTVersion, 0x02,
			MakeSignalingPayload(0, MakePAMessage(MakeMMTTable(0x20, MPTVersion, MPT)))));
		for (int i = 0; i < 4; i++)
			Packets.push_back(MakeTLV(0xFF, ByteArray(16, 0xFF)));
		return Concat(Packets);
	}

	class AudioSelectionEngine
		: public LibISDB::TSEngine
	{
	public:
		// ViewerFilter の FDK と同じく 22.2ch を再生できないものとする
		bool Play22_2ch = true;
		bool Stereo = false;

	protected:
		bool IsAudioPlayable(std::uint8_t StreamType, const LibISDB::EventInfo::AudioInfo *pInfo) const override
		{
			if (pInfo == nullptr)
				return true;
			if (pInfo->StreamContent != LibISDB::MMT_AUDIO_STREAM_CONTENT_AAC)
				return false;
			if ((pInfo->ComponentType & LibISDB::MMT_AUDIO_MODE_MASK) == LibISDB::MMT_AUDIO_MODE_22_2)
				return Play22_2ch;
			return true;
		}

		bool IsStereoAudioPreferred() const override { return Stereo; }
	};

	constexpr std::uint8_t AAC = LibISDB::MMT_AUDIO_STREAM_CONTENT_AAC;
	constexpr std::uint8_t ALS = LibISDB::MMT_AUDIO_STREAM_CONTENT_ALS;
	constexpr std::uint8_t MODE_2CH = LibISDB::MMT_AUDIO_MODE_2_0;
	constexpr std::uint8_t MODE_5_1CH = LibISDB::MMT_AUDIO_MODE_3_2_1;
	constexpr std::uint8_t MODE_22_2CH = LibISDB::MMT_AUDIO_MODE_22_2;
}

TEST_CASE("TSEngineMMTAudioSelection", "[mmt][engine]")
{
	using namespace LibISDB;

	StreamPacketParserFilter *pParser = new StreamPacketParserFilter;
	MMTAnalyzerFilter *pMMTAnalyzer = new MMTAnalyzerFilter;
	StreamIDRecorder *pRecorder = new StreamIDRecorder;
	AudioSelectionEngine Engine;
	REQUIRE(Engine.BuildEngine({pParser, pMMTAnalyzer, pRecorder}));
	REQUIRE(pParser->StartStreaming());

	// NHK BS8K の 22.2ch 番組の構成
	const auto Input22_2ch =
		[&](std::uint8_t Version) {
			InputFilter(*pParser, MakeAudioSelectionSignaling(Version, {
				{0x0110, 0x0010, AAC, MODE_22_2CH, 0x00},
				{0x0111, 0x0011, AAC, MODE_5_1CH, 0x00},
				{0x0112, 0x0012, AAC, MODE_2CH, 0x00},
			}), 4096);
		};

	SECTION("Playable") {
		Input22_2ch(1);
		CHECK(pRecorder->AudioPacketID == 0x0110);
		CHECK(Engine.GetAudioComponentTag() == 0x10);
		CHECK(Engine.GetDefaultAudioStream() == 0);
		CHECK(Engine.IsAudioStreamPlayable(0));
		CHECK_FALSE(Engine.IsAudioStreamPlayable(3));

		EventInfo::AudioInfo Info;
		REQUIRE(Engine.GetAudioComponentInfo(1, &Info));
		CHECK(Info.StreamContent == AAC);
		CHECK(Info.ComponentType == MODE_5_1CH);
		CHECK(Info.ComponentTag == 0x11);
		CHECK(Info.SimulcastGroupTag == 0x00);
		CHECK(Info.LanguageCode == 0x6A706E);
		CHECK_FALSE(Engine.GetAudioComponentInfo(3, &Info));

		Engine.Play22_2ch = false;
		CHECK_FALSE(Engine.IsAudioStreamPlayable(0));
		CHECK(Engine.GetDefaultAudioStream() == 1);
		Engine.Stereo = true;
		CHECK(Engine.GetDefaultAudioStream() == 2);
	}

	SECTION("NotPlayable") {
		// 選局時は再生できるもののうち component_tag が最小のもの
		Engine.Play22_2ch = false;
		Input22_2ch(1);
		CHECK(pRecorder->AudioPacketID == 0x0111);
		CHECK(Engine.GetAudioStream() == 1);
		CHECK(Engine.GetAudioComponentTag() == 0x11);

		// 利用者が選んだものはそのまま
		REQUIRE(Engine.SetAudioStream(0));
		CHECK(pRecorder->AudioPacketID == 0x0110);
	}

	SECTION("Stereo") {
		// 2ch で再生する場合はサイマル音声の AAC 2ch
		Engine.Stereo = true;
		Input22_2ch(1);
		CHECK(pRecorder->AudioPacketID == 0x0112);
		CHECK(Engine.GetAudioComponentTag() == 0x12);
	}

	SECTION("StereoNotSimulcast") {
		// サイマル音声でない 2ch (解説音声など) は選ばない
		Engine.Stereo = true;
		InputFilter(*pParser, MakeAudioSelectionSignaling(1, {
			{0x0110, 0x0010, AAC, MODE_5_1CH, 0xFF},
			{0x0111, 0x0011, AAC, MODE_2CH, 0xFF},
		}), 4096);
		CHECK(pRecorder->AudioPacketID == 0x0110);
	}

	SECTION("ALS") {
		InputFilter(*pParser, MakeAudioSelectionSignaling(1, {
			{0x0110, 0x0010, ALS, MODE_2CH, 0x00},
			{0x0111, 0x0011, AAC, MODE_2CH, 0x00},
		}), 4096);
		CHECK(pRecorder->AudioPacketID == 0x0111);
		CHECK_FALSE(Engine.IsAudioStreamPlayable(0));
	}

	SECTION("NoPlayableAudio") {
		InputFilter(*pParser, MakeAudioSelectionSignaling(1, {
			{0x0110, 0x0010, ALS, MODE_2CH, 0x00},
		}), 4096);
		CHECK(pRecorder->AudioPacketID == 0x0110);
	}

	SECTION("KeepSelection") {
		// 音声モードだけの変化では利用者の選択を保つ (ARIB TR-B39 第1部 第2編 4.7.2 図 4-2)
		InputFilter(*pParser, MakeAudioSelectionSignaling(1, {
			{0x0110, 0x0010, AAC, MODE_5_1CH, 0x00},
			{0x0111, 0x0011, AAC, MODE_2CH, 0x00},
			{0x0112, 0x0012, AAC, MODE_2CH, 0xFF},
		}), 4096);
		REQUIRE(Engine.SetAudioStream(2));
		InputFilter(*pParser, MakeAudioSelectionSignaling(2, {
			{0x0110, 0x0010, AAC, MODE_22_2CH, 0x00},
			{0x0111, 0x0011, AAC, MODE_2CH, 0x00},
			{0x0112, 0x0012, AAC, MODE_2CH, 0xFF},
		}), 4096);
		CHECK(pRecorder->AudioPacketID == 0x0112);

		// 選んでいた音声が再生できなくなったら既定の音声にする
		Engine.Play22_2ch = false;
		InputFilter(*pParser, MakeAudioSelectionSignaling(3, {
			{0x0110, 0x0010, AAC, MODE_5_1CH, 0x00},
			{0x0111, 0x0011, AAC, MODE_2CH, 0x00},
			{0x0112, 0x0012, AAC, MODE_2CH, 0xFF},
		}), 4096);
		CHECK(pRecorder->AudioPacketID == 0x0112);
		REQUIRE(Engine.SetAudioStream(0));
		InputFilter(*pParser, MakeAudioSelectionSignaling(4, {
			{0x0110, 0x0010, AAC, MODE_22_2CH, 0x00},
			{0x0111, 0x0011, AAC, MODE_2CH, 0x00},
			{0x0112, 0x0012, AAC, MODE_2CH, 0xFF},
		}), 4096);
		CHECK(pRecorder->AudioPacketID == 0x0111);
	}

	SECTION("AudioChanged") {
		Engine.Play22_2ch = false;

		// 番組の冒頭は 2ch のみ
		InputFilter(*pParser, MakeAudioSelectionSignaling(1, {
			{0x0110, 0x0010, AAC, MODE_2CH, 0xFF},
		}), 4096);
		CHECK(pRecorder->AudioPacketID == 0x0110);

		// 同じ component_tag が 22.2ch になったので選び直す
		Input22_2ch(2);
		CHECK(pRecorder->AudioPacketID == 0x0111);

		// 構成が同じなら利用者の選んだものを保つ
		REQUIRE(Engine.SetAudioStream(2));
		CHECK(pRecorder->AudioPacketID == 0x0112);
		Input22_2ch(3);
		CHECK(pRecorder->AudioPacketID == 0x0112);
		CHECK(Engine.GetAudioComponentTag() == 0x12);

		// 構成が変わったら既定の音声に戻す
		InputFilter(*pParser, MakeAudioSelectionSignaling(4, {
			{0x0110, 0x0010, AAC, MODE_5_1CH, 0x00},
			{0x0111, 0x0011, AAC, MODE_2CH, 0x00},
			{0x0112, 0x0012, AAC, MODE_2CH, 0xFF},
		}), 4096);
		CHECK(pRecorder->AudioPacketID == 0x0110);
		CHECK(Engine.GetAudioComponentTag() == 0x10);
	}
}

#include "../LibISDB/MediaParsers/AACDownmix.hpp"

TEST_CASE("AACDownmix", "[mediaparser][aac]")
{
	using namespace LibISDB;

	enum {
		FC, FLc, FRc, FL, FR, SiL, SiR, BL, BR, BC, LFE1, LFE2,
		TpFC, TpFL, TpFR, TpSiL, TpSiR, TpC, TpBL, TpBR, TpBC, BtFC, BtFL, BtFR,
	};

	const auto Downmix =
		[](std::initializer_list<std::pair<int, std::int16_t>> Values, const AACDownmixCoefficients &Coefficients = AACDownmixCoefficients()) {
			std::int16_t Src[AAC_22_2CH_CHANNEL_COUNT * 2] = {};
			for (const auto &e : Values) {
				Src[e.first] = e.second;
				Src[AAC_22_2CH_CHANNEL_COUNT + e.first] = e.second;
			}
			std::vector<std::int16_t> Dst(12, 0x7777);
			DownmixAAC22_2chTo5_1ch(Src, Dst.data(), 2, Coefficients);
			CHECK(std::equal(Dst.begin(), Dst.begin() + 6, Dst.begin() + 6));
			Dst.resize(6);
			return Dst;
		};

	// ISO/IEC 14496-3 AMD4 表 AMD4.8 / AMD4.9
	CHECK(AACDownmixLevelToGain(0) == 1.0f);
	CHECK(AACDownmixLevelToGain(2) == 0.707f);
	CHECK(AACDownmixLevelToGain(7) == 0.0f);
	CHECK(AACDownmixLFELevelToGain(5) == 1.0f);
	CHECK(AACDownmixLFELevelToGain(7) == 0.707f);
	CHECK(AACDownmixLFELevelToGain(15) == 0.0f);

	// DSE の MPEG4_ancillary_data() (NHK BS8K の 22.2ch 番組の実データ、全て既定値)
	AACDownmixCoefficients Parsed;
	const ByteArray RealDSE = HexBytes("BCC01CAA010000BDB62270");
	REQUIRE(ParseAACAncillaryData(RealDSE.data(), RealDSE.size(), &Parsed));
	CHECK(Parsed.HasMixLevels);
	CHECK(Parsed.CenterMixLevel == 2);
	CHECK(Parsed.SurroundMixLevel == 2);
	CHECK(Parsed.DmixC == 3);
	CHECK(Parsed.DmixD == 3);
	CHECK(Parsed.DmixE == 0);
	CHECK(Parsed.DmixF == 4);
	CHECK(Parsed.DmixG == 2);
	CHECK(Parsed.DmixLFE == 7);
	// 既定値以外: center 4 / surround 7、dmix_a 1 / dmix_b 5、c..g = 1,2,3,4,5、l = 15
	const ByteArray CustomDSE = HexBytes("BCC018CF4034BD94E5F0");
	REQUIRE(ParseAACAncillaryData(CustomDSE.data(), CustomDSE.size(), &Parsed));
	CHECK(Parsed.CenterMixLevel == 4);
	CHECK(Parsed.SurroundMixLevel == 7);
	CHECK(Parsed.DmixA == 1);
	CHECK(Parsed.DmixB == 5);
	CHECK(Parsed.DmixC == 1);
	CHECK(Parsed.DmixD == 2);
	CHECK(Parsed.DmixE == 3);
	CHECK(Parsed.DmixF == 4);
	CHECK(Parsed.DmixG == 5);
	CHECK(Parsed.DmixLFE == 15);
	// ARIB 拡張が無ければ 22.2ch の係数は変わらない
	const ByteArray NoExtDSE = HexBytes("BCC000");
	REQUIRE(ParseAACAncillaryData(NoExtDSE.data(), NoExtDSE.size(), &Parsed));
	CHECK(Parsed.DmixC == 1);
	CHECK(Parsed.CenterMixLevel == 4);
	CHECK_FALSE(ParseAACAncillaryData(HexBytes("BB0000").data(), 3, &Parsed));
	CHECK_FALSE(ParseAACAncillaryData(RealDSE.data(), 2, &Parsed));

	// 係数の指定: FLc を -∞ (7)、LFE を 0 dB (5)
	AACDownmixCoefficients Custom;
	Custom.DmixC = 7;
	Custom.DmixLFE = 5;
	CHECK(Downmix({{FLc, 1000}, {LFE1, 1000}}, Custom) == std::vector<std::int16_t> {0, 0, 0, 0, 0, 1000});

	// 出力は C, L, R, Ls, Rs, LFE
	CHECK(Downmix({{FC, 1000}}) == std::vector<std::int16_t> {1000, 0, 0, 0, 0, 0});
	CHECK(Downmix({{FL, 1000}, {FR, -1000}, {BL, 300}, {BR, -300}}) == std::vector<std::int16_t> {0, 1000, -1000, 300, -300, 0});
	// -4.5 dB
	CHECK(Downmix({{FLc, 1000}}) == std::vector<std::int16_t> {596, 596, 0, 0, 0, 0});
	CHECK(Downmix({{SiR, 1000}}) == std::vector<std::int16_t> {0, 0, 596, 0, 596, 0});
	CHECK(Downmix({{TpSiL, 1000}}) == std::vector<std::int16_t> {0, 596, 0, 596, 0, 0});
	// 上層・下層は 0 dB
	CHECK(Downmix({{TpFL, 1000}, {BtFR, 1000}, {TpFC, 100}, {BtFC, 100}}) == std::vector<std::int16_t> {200, 1000, 1000, 0, 0, 0});
	CHECK(Downmix({{TpBL, 1000}, {TpBR, -1000}}) == std::vector<std::int16_t> {0, 0, 0, 1000, -1000, 0});
	// TpC は C に -6 dB、Ls/Rs に -6 dB
	CHECK(Downmix({{TpC, 1000}}) == std::vector<std::int16_t> {500, 0, 0, 500, 500, 0});
	// BC と TpBC は Ls/Rs に -3 dB
	CHECK(Downmix({{BC, 1000}}) == std::vector<std::int16_t> {0, 0, 0, 707, 707, 0});
	CHECK(Downmix({{TpBC, 1000}}) == std::vector<std::int16_t> {0, 0, 0, 707, 707, 0});
	// LFE は 2 つの和に -3 dB
	CHECK(Downmix({{LFE1, 1000}, {LFE2, 1000}}) == std::vector<std::int16_t> {0, 0, 0, 0, 0, 1414});
	// 飽和
	CHECK(Downmix({{FL, 32767}, {TpFL, 32767}, {FR, -32768}, {BtFR, -32768}}) == std::vector<std::int16_t> {0, 32767, -32768, 0, 0, 0});
}

#include "../LibISDB/Filters/LogoDownloaderFilter.hpp"

namespace
{
	class LogoRecorder
		: public LibISDB::LogoDownloaderFilter::LogoHandler
	{
	public:
		struct Logo {
			std::uint16_t NetworkID;
			std::vector<LibISDB::LogoDownloaderFilter::LogoService> ServiceList;
			std::uint16_t LogoID;
			std::uint16_t LogoVersion;
			std::uint8_t LogoType;
			ByteArray Data;
			LibISDB::DateTime Time;
		};

		std::vector<Logo> Logos;

		void OnLogoDownloaded(const LibISDB::LogoDownloaderFilter::LogoData &Data) override
		{
			Logo &Info = Logos.emplace_back();
			Info.NetworkID = Data.NetworkID;
			Info.ServiceList = Data.ServiceList;
			Info.LogoID = Data.LogoID;
			Info.LogoVersion = Data.LogoVersion;
			Info.LogoType = Data.LogoType;
			Info.Data.assign(Data.pData, Data.pData + Data.DataSize);
			Info.Time = Data.Time;
		}
	};

	ByteArray MakeLogoCDTSection(
		std::uint16_t DownloadDataID, std::uint8_t Version, std::uint8_t SectionNumber, std::uint8_t LastSectionNumber,
		std::uint8_t LogoType, std::uint16_t LogoID, std::uint16_t LogoVersion, const ByteArray &Data)
	{
		ByteArray Payload;
		Append16(Payload, 0x000B);
		Payload.push_back(0x01);
		Append16(Payload, 0xF000);
		Payload.push_back(LogoType);
		Append16(Payload, static_cast<std::uint16_t>(0xFE00 | LogoID));
		Append16(Payload, static_cast<std::uint16_t>(0xF000 | LogoVersion));
		Append16(Payload, static_cast<std::uint16_t>(Data.size()));
		Append(Payload, Data);
		return MakeSection(0xA2, DownloadDataID, Version, SectionNumber, LastSectionNumber, Payload);
	}

	// LogoList は logo_type, start_section_number, num_of_sections の並び
	ByteArray MakeLogoCDT1Descriptor(
		std::uint16_t LogoID, std::uint16_t LogoVersion, std::uint16_t DownloadDataID, const ByteArray &LogoList)
	{
		ByteArray Body {0x01};
		Append16(Body, static_cast<std::uint16_t>(0xFE00 | LogoID));
		Append16(Body, static_cast<std::uint16_t>(0xF000 | LogoVersion));
		Append16(Body, DownloadDataID);
		Append(Body, LogoList);
		return Body;
	}

	ByteArray MakeLogoSDTPayload(const std::vector<std::pair<std::uint16_t, ByteArray>> &ServiceList)
	{
		ByteArray Payload;
		Append16(Payload, 0x000B);
		Payload.push_back(0xFF);
		for (const auto &e : ServiceList) {
			Append16(Payload, e.first);
			Payload.push_back(0xFF);
			Append(Payload, WithLength16(0x8, MakeMMTDescriptor(0x8025, e.second)));
		}
		return Payload;
	}
}

TEST_CASE("LogoDownloaderMMT", "[mmt][logo][filter]")
{
	using namespace LibISDB;

	auto pParser = std::make_unique<StreamPacketParserFilter>();
	auto pLogo = std::make_unique<LogoDownloaderFilter>();
	LogoRecorder Recorder;
	pLogo->SetLogoHandler(&Recorder);
	pParser->SetOutputFilter(pLogo.get(), pLogo->GetInputSink());
	REQUIRE(pParser->StartStreaming());

	std::uint32_t Sequence = 0;
	auto Signaling = [&](std::uint16_t PacketID, std::uint16_t MessageID, const ByteArray &Section) {
		return MakeMMTPTLV(PacketID, Sequence++, 0x02, MakeSignalingPayload(0, MakeMessage16(MessageID, Section)));
	};
	auto CDT = [&](std::uint8_t Version, std::uint8_t SectionNumber, std::uint8_t LastSectionNumber,
			std::uint8_t LogoType, std::uint16_t LogoVersion, const ByteArray &Data) {
		return Signaling(0x8006, 0x8000,
			MakeLogoCDTSection(0x0123, Version, SectionNumber, LastSectionNumber, LogoType, 0x0123, LogoVersion, Data));
	};
	auto SDT = [&](std::uint8_t Version, const std::vector<std::pair<std::uint16_t, ByteArray>> &ServiceList) {
		return Signaling(0x8004, 0x8000, MakeSection(0x9F, 0xB110, Version, 0, 0, MakeLogoSDTPayload(ServiceList)));
	};

	const ByteArray Logo2K(10, 0x05_u8);
	const ByteArray LogoSmall(20, 0x06_u8);
	const ByteArray LogoLarge1(30, 0x07_u8);
	const ByteArray LogoLarge2(5, 0x17_u8);
	const ByteArray Descriptor = MakeLogoCDT1Descriptor(0x0123, 2, 0x0123, {0x05, 0, 1, 0x06, 1, 1, 0x07, 2, 2});
	const ByteArray TOT = Signaling(0x8005, 0x8002, MakeShortSection(0xA1, HexBytes("EF9E051916F000")));
	const std::vector<ByteArray> CDTPackets {
		CDT(4, 0, 3, 0x05, 2, Logo2K),
		CDT(4, 1, 3, 0x06, 2, LogoSmall),
		CDT(4, 2, 3, 0x07, 2, LogoLarge1),
		CDT(4, 3, 3, 0x07, 2, LogoLarge2),
	};

	SECTION("MHCDTTable") {
		std::unique_ptr<MMTSectionTable> pTable(new MMTSectionTable(new MHCDTMultiTable));
		TLVPacket Packet;
		ToTLVPacket(CDTPackets[2], &Packet);
		pTable->StorePacket(&Packet);

		// data_size がモジュールの残りより大きい
		ByteArray Broken;
		Append(Broken, HexBytes("000B01F00005FF24F0000010"));
		Append(Broken, HexBytes("01020304"));
		ToTLVPacket(Signaling(0x8006, 0x8000, MakeSection(0xA2, 0x0124, 0, 0, 0, Broken)), &Packet);
		pTable->StorePacket(&Packet);

		const MHCDTMultiTable *pCDT = pTable->GetTable<MHCDTMultiTable>();
		REQUIRE(pCDT != nullptr);
		CHECK(pCDT->GetDownloadDataIndex(0x0999) < 0);
		const int Index = pCDT->GetDownloadDataIndex(0x0123);
		REQUIRE(Index >= 0);
		CHECK(pCDT->GetSectionCount(Index) == 4);
		CHECK(pCDT->GetCDTTable(Index, 0) == nullptr);
		const MHCDTTable *pSection = pCDT->GetCDTTable(Index, 2);
		REQUIRE(pSection != nullptr);
		CHECK(pSection->GetDownloadDataID() == 0x0123);
		CHECK(pSection->GetVersionNumber() == 4);
		CHECK(pSection->GetOriginalNetworkID() == 0x000B);
		CHECK(pSection->GetDataType() == MHCDTTable::DATA_TYPE_LOGO);
		MHCDTTable::LogoModuleInfo Module;
		REQUIRE(pSection->GetLogoModule(&Module));
		CHECK(Module.LogoType == 0x07);
		CHECK(Module.LogoID == 0x0123);
		CHECK(Module.LogoVersion == 2);
		CHECK(ByteArray(Module.pData, Module.pData + Module.DataSize) == LogoLarge1);

		const MHCDTTable *pBroken = pCDT->GetCDTTable(pCDT->GetDownloadDataIndex(0x0124), 0);
		REQUIRE(pBroken != nullptr);
		CHECK_FALSE(pBroken->GetLogoModule(&Module));
	}

	SECTION("CDT1") {
		// MH-SDT より先に届いた MH-CDT も使う
		InputFilter(*pParser, Concat({TOT, CDTPackets[0], CDTPackets[1], CDTPackets[2]}), 4096);
		CHECK(Recorder.Logos.empty());

		InputFilter(*pParser, SDT(1, {{0x0400, Descriptor}}), 4096);
		REQUIRE(Recorder.Logos.size() == 2);
		CHECK(Recorder.Logos[0].NetworkID == 0x000B);
		CHECK(Recorder.Logos[0].LogoID == 0x0123);
		CHECK(Recorder.Logos[0].LogoVersion == 2);
		CHECK(Recorder.Logos[0].LogoType == 0x05);
		CHECK(Recorder.Logos[0].Data == Logo2K);
		REQUIRE(Recorder.Logos[0].ServiceList.size() == 1);
		CHECK(Recorder.Logos[0].ServiceList[0].NetworkID == 0x000B);
		CHECK(Recorder.Logos[0].ServiceList[0].TransportStreamID == 0xB110);
		CHECK(Recorder.Logos[0].ServiceList[0].ServiceID == 0x0400);
		CHECK(Recorder.Logos[0].Time.Hour == 5);
		CHECK(Recorder.Logos[0].Time.Minute == 19);
		CHECK(Recorder.Logos[1].LogoType == 0x06);
		CHECK(Recorder.Logos[1].Data == LogoSmall);

		// 複数セクションのロゴは全部揃ってから連結する
		InputFilter(*pParser, CDTPackets[3], 4096);
		REQUIRE(Recorder.Logos.size() == 3);
		CHECK(Recorder.Logos[2].LogoType == 0x07);
		CHECK(Recorder.Logos[2].Data == Concat({LogoLarge1, LogoLarge2}));

		// ロゴに関係しない MH-SDT の更新では再通知しない
		InputFilter(*pParser, Concat({SDT(2, {{0x0400, Descriptor}}), Concat(CDTPackets)}), 4096);
		CHECK(Recorder.Logos.size() == 3);

		pLogo->Reset();
		InputFilter(*pParser, Concat({SDT(2, {{0x0400, Descriptor}}), Concat(CDTPackets)}), 4096);
		CHECK(Recorder.Logos.size() == 6);
	}

	SECTION("Mismatch") {
		// 記述子と data_module_byte の logo_version が違う間は通知しない
		InputFilter(*pParser, Concat({
			TOT,
			SDT(1, {{0x0400, MakeLogoCDT1Descriptor(0x0123, 3, 0x0123, {0x05, 0, 1})}}),
			CDTPackets[0],
			CDTPackets[1]}), 4096);
		CHECK(Recorder.Logos.empty());

		InputFilter(*pParser, CDT(5, 0, 0, 0x05, 3, Logo2K), 4096);
		REQUIRE(Recorder.Logos.size() == 1);
		CHECK(Recorder.Logos[0].LogoVersion == 3);
		CHECK(Recorder.Logos[0].LogoType == 0x05);
	}

	SECTION("CDT2") {
		// CDT 伝送方式 2 で同じ logo_id を参照するサービスも対象に含める
		ByteArray CDT2 {0x02};
		Append16(CDT2, 0xFF23);
		InputFilter(*pParser, Concat({
			TOT,
			SDT(1, {{0x0400, MakeLogoCDT1Descriptor(0x0123, 2, 0x0123, {0x05, 0, 1})}, {0x0401, CDT2}}),
			CDTPackets[0],
			CDTPackets[1]}), 4096);
		REQUIRE(Recorder.Logos.size() == 1);
		REQUIRE(Recorder.Logos[0].ServiceList.size() == 2);
		CHECK(Recorder.Logos[0].ServiceList[0].ServiceID == 0x0400);
		CHECK(Recorder.Logos[0].ServiceList[1].ServiceID == 0x0401);
	}
}




#include "../LibISDB/Filters/TSPacketCounterFilter.hpp"
#include <map>
#include <chrono>
#include <thread>

namespace
{
	class TestPacketCounterFilter
		: public LibISDB::TSPacketCounterFilter
	{
	public:
		void SetBitRateUpdateInterval(LibISDB::TickClock::ClockType Interval)
		{
			m_VideoBitRate.SetUpdateInterval(Interval);
			m_AudioBitRate.SetUpdateInterval(Interval);
		}
	};
}

TEST_CASE("TSPacketCounterFilterMMT", "[mmt][filter]")
{
	using namespace LibISDB;

	auto pParser = std::make_unique<StreamPacketParserFilter>();
	auto pCounter = std::make_unique<TestPacketCounterFilter>();
	pParser->SetOutputFilter(pCounter.get(), pCounter->GetInputSink());
	REQUIRE(pParser->StartStreaming());

	std::map<std::uint16_t, std::uint32_t> SequenceMap;
	auto Signaling = [&](std::uint16_t PacketID, const ByteArray &Message) {
		return MakeMMTPTLV(PacketID, SequenceMap[PacketID]++, 0x02, MakeSignalingPayload(0, Message));
	};
	auto Media = [&](std::uint16_t PacketID, size_t Count, bool Scrambled) {
		std::vector<ByteArray> List;
		for (size_t i = 0; i < Count; i++) {
			List.push_back(MakeTLV(0x03, MakeCompressedIP(0x001, 0, 0x60, 50000, 51216,
				MakeMMTP(PacketID, SequenceMap[PacketID]++, 0x00, 1000, Scrambled ? MMT_SCRAMBLING_CONTROL_EVEN : -1))));
		}
		return Concat(List);
	};

	ByteArray PLT {0x02};
	const std::pair<std::uint16_t, std::uint16_t> PackageList[] = {{0x0400, 0x0F00}, {0x0401, 0x0F01}};
	for (const auto &[ServiceID, MPTPacketID] : PackageList) {
		PLT.push_back(0x02);
		Append16(PLT, ServiceID);
		PLT.push_back(0x00);
		Append16(PLT, MPTPacketID);
	}
	PLT.push_back(0x00); // num_of_ip_delivery

	ByteArray OtherMPT {0xFC, 0x02};
	Append16(OtherMPT, 0x0401);
	Append16(OtherMPT, 0x0000); // MPT_descriptors_length
	OtherMPT.push_back(0x02);
	Append(OtherMPT, MakeAsset(0x0200, MMT_ASSET_TYPE_HEV1, 0x0000));
	Append(OtherMPT, MakeAsset(0x0210, MMT_ASSET_TYPE_MP4A, 0x0010));

	// 0x0400 のアセットは 0x0100 / 0x0110 / 0x0111 / 0x0130 / 0x0140、0x0401 のアセットは 0x0200 / 0x0210
	auto MediaData = [&] {
		return Concat({
			Media(0x0100, 10, true),
			Media(0x0110, 4, true),
			Media(0x0130, 2, true),
			Media(0x0140, 2, false),
			Media(0x0200, 5, true),
			Media(0x0210, 3, true),
			Media(0x0300, 2, true)});
	};

	InputFilter(*pParser, Concat({
		Signaling(0x0000, MakePAMessage(MakeMMTTable(0x80, 1, PLT))),
		Signaling(0x0F00, MakePAMessage(MakeMMTTable(0x20, 1, MakeMPTPayload(0x0400, 1, false)))),
		Signaling(0x0F01, MakePAMessage(MakeMMTTable(0x20, 1, OtherMPT)))}), 4096);
	InputFilter(*pParser, MediaData(), 4096);
	// サービスが選択されていなければ全て数える
	CHECK(pCounter->GetScrambledPacketCount() == 26);
	CHECK(pCounter->GetInputPacketCount() == 3 + 28);

	pCounter->ResetScrambledPacketCount();
	pCounter->SetActiveServiceID(0x0400);
	InputFilter(*pParser, MediaData(), 4096);
	CHECK(pCounter->GetScrambledPacketCount() == 16);

	pCounter->ResetScrambledPacketCount();
	pCounter->SetActiveServiceID(0x0401);
	InputFilter(*pParser, MediaData(), 4096);
	CHECK(pCounter->GetScrambledPacketCount() == 8);

	SECTION("MPTUpdate") {
		pCounter->SetActiveServiceID(0x0400);
		pCounter->ResetScrambledPacketCount();
		InputFilter(*pParser, Concat({
			Signaling(0x0F00, MakePAMessage(MakeMMTTable(0x20, 2, MakeMPTPayload(0x0400, 2, true)))),
			Media(0x0150, 3, true)}), 4096);
		CHECK(pCounter->GetScrambledPacketCount() == 3);
	}

	SECTION("PLTUpdate") {
		pCounter->SetActiveServiceID(0x0400);
		pCounter->ResetScrambledPacketCount();
		InputFilter(*pParser, Concat({
			Signaling(0x0000, MakePAMessage(MakeMMTTable(0x80, 2, MakePLTPayload(0x0401, 0x0F01)))),
			MediaData()}), 4096);
		CHECK(pCounter->GetScrambledPacketCount() == 0);
	}

	SECTION("Reset") {
		pCounter->Reset();
		InputFilter(*pParser, MediaData(), 4096);
		CHECK(pCounter->GetScrambledPacketCount() == 26);
		CHECK(pCounter->GetInputPacketCount() == 28);
	}

	SECTION("BitRate") {
		pCounter->SetBitRateUpdateInterval(TickClock::ClocksPerSec / 10);
		pCounter->SetActiveServiceID(0x0400);
		// 0x0210 は選択されていないサービスのアセット
		pCounter->SetActiveVideoPacketID(0x0100, true);
		pCounter->SetActiveAudioPacketID(0x0210, true);
		InputFilter(*pParser, MediaData(), 4096);
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		InputFilter(*pParser, MediaData(), 4096);
		CHECK(pCounter->GetVideoBitRate() > 0);
		CHECK(pCounter->GetAudioBitRate() == 0);
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
