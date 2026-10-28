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
