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
 @file   MPUAssembler.hpp
 @brief  MPU の組み立て
 @author cngu-dtv
*/


#ifndef LIBISDB_MPU_ASSEMBLER_H
#define LIBISDB_MPU_ASSEMBLER_H


#include "MMTPPayload.hpp"
#include "MMTPacketIDMap.hpp"
#include "MMTDescriptorBlock.hpp"
#include "MMTDescriptors.hpp"
#include <map>
#include <vector>


namespace LibISDB
{

	/** MPU のタイムスタンプ表 (ARIB STD-B60 解説 2) */
	class MPUTimestampTable
	{
	public:
		MPUTimestampTable();

		void Reset();
		// MPT のアセット記述子領域の MPU タイムスタンプ記述子と MPU 拡張タイムスタンプ記述子を取り込む
		// AssetTimescale は MPU 拡張タイムスタンプ記述子に timescale が無い場合に使う (0 なら不明)
		void Update(const MMTDescriptorBlock *pDescBlock, uint32_t AssetTimescale = 0);
		// MPU の提示時刻 (NTP 64 ビット形式)
		bool GetPresentationTime(uint32_t MPUSequenceNumber, ReturnArg<uint64_t> Time) const;
		// MPU 内の復号順で Index 番目のアクセスユニットの時刻 (90kHz、NTP の起点からの値)
		bool GetAccessUnitTime(
			uint32_t MPUSequenceNumber, uint32_t Index,
			ReturnArg<int64_t> DTS, ReturnArg<int64_t> PTS) const;
		size_t GetMPUCount() const noexcept { return m_MPUMap.size(); }

		static int64_t NTPTimeTo90kHz(uint64_t Time) noexcept;

	private:
		struct MPUEntry {
			bool HasPresentationTime = false;
			uint64_t PresentationTime = 0;
			bool HasExtendedTimestamp = false;
			uint32_t Timescale = 0;
			uint16_t DecodingTimeOffset = 0;
			std::vector<MPUExtendedTimestampDescriptor::AccessUnitInfo> AccessUnitList;
		};

		// 最新の MPU からこれ以上離れた MPU の情報は捨てる
		static constexpr int32_t MAX_SEQUENCE_DISTANCE = 32;

		void Prune();

		std::map<uint32_t, MPUEntry> m_MPUMap;
		bool m_HasLatestSequenceNumber;
		uint32_t m_LatestSequenceNumber;
	};

	/** MPU 組み立てクラス (packet_id 1 つ分の MFU からアクセスユニット等を組み立てる) */
	class MPUAssembler
	{
	public:
		enum class MediaType {
			Video,   /**< HEVC (1 MFU = 1 NAL ユニット) */
			Audio,   /**< MPEG-4 オーディオ (1 MFU = 1 AudioMuxElement()) */
			Caption, /**< 字幕・文字スーパー (1 MPU = TTML 文書と参照されるリソース) */
			Generic, /**< その他の時間指定 MFU (1 MFU をそのまま出力する) */
		};

		struct AccessUnitInfo {
			uint16_t PacketID;
			uint32_t MPUSequenceNumber;
			uint32_t Index;          /**< MPU 内の復号順の番号 */
			bool HasTimestamp;
			int64_t DTS;             /**< 90kHz */
			int64_t PTS;             /**< 90kHz */
			bool RandomAccess;       /**< IRAP ピクチャ (映像のみ) */
			bool Discontinuity;      /**< 直前に出力したものとの間でデータが欠けた */
			const uint8_t *pData;    /**< 映像は Annex B のバイトストリーム、音声は AudioMuxElement() */
			size_t DataSize;
		};

		struct SubsampleInfo {
			uint8_t DataType;        /**< data_type (ARIB STD-B60 表 9-2) */
			const uint8_t *pData;
			size_t DataSize;
		};

		struct CaptionUnitInfo {
			uint16_t PacketID;
			uint32_t MPUSequenceNumber;
			uint8_t SubtitleTag;
			uint8_t SubtitleSequenceNumber;
			bool HasPresentationTime;
			uint64_t PresentationTime;                /**< MPU の提示時刻 (NTP 64 ビット形式) */
			bool Discontinuity;                       /**< 直前に出力したものとの間でデータが欠けた */
			std::vector<SubsampleInfo> SubsampleList; /**< subsample_number 順 */
		};

		struct StatisticsInfo {
			unsigned long long PacketCount = 0;        /**< 入力された MPU パケット */
			unsigned long long ScrambledCount = 0;     /**< スクランブルされていて捨てたパケット */
			unsigned long long LossCount = 0;          /**< packet_sequence_number の不連続 */
			unsigned long long OutputCount = 0;        /**< 出力したアクセスユニット / 字幕 MPU */
			unsigned long long DroppedCount = 0;       /**< 欠けていて捨てたアクセスユニット / 字幕 MPU */
			unsigned long long NoTimestampCount = 0;   /**< 時刻が分からなかったアクセスユニット */
		};

		class Handler
		{
		public:
			virtual ~Handler() = default;

			// データはこの呼び出しの間のみ有効
			virtual void OnAccessUnit(const MPUAssembler *pAssembler, const AccessUnitInfo &Info) {}
			virtual void OnCaptionUnit(const MPUAssembler *pAssembler, const CaptionUnitInfo &Info) {}
		};

		static constexpr size_t MAX_UNIT_SIZE = 16 * 1024 * 1024;

		MPUAssembler(MediaType Type, Handler *pHandler = nullptr);

		void Reset();
		// payload_type が MPU 以外のパケットは無視する
		void StorePacket(const TLVPacket *pPacket);
		// 組み立て途中の映像アクセスユニットを出力する
		void Flush();
		void UpdateTimestamps(const MMTDescriptorBlock *pDescBlock, uint32_t AssetTimescale = 0);

		MediaType GetMediaType() const noexcept { return m_MediaType; }
		uint16_t GetPacketID() const noexcept { return m_PacketID; }
		void SetHandler(Handler *pHandler) noexcept { m_pHandler = pHandler; }
		Handler * GetHandler() const noexcept { return m_pHandler; }
		const MPUTimestampTable & GetTimestampTable() const noexcept { return m_TimestampTable; }
		const StatisticsInfo & GetStatistics() const noexcept { return m_Statistics; }

		// hev1 / hvc1 は Video、mp4a は Audio、stpp は Caption、それ以外は Generic
		static MediaType GetMediaTypeFromAssetType(uint32_t AssetType) noexcept;

	private:
		void OnLoss();
		void OnMPUChanged(uint32_t MPUSequenceNumber, bool RandomAccess);
		void OnMFU(const uint8_t *pData, size_t Size);
		void OnVideoNALUnit(const uint8_t *pData, size_t Size);
		void OutputVideoAccessUnit();
		void OnAudioFrame(const uint8_t *pData, size_t Size);
		void OnCaptionSubsample(const uint8_t *pData, size_t Size);
		void OnGenericMFU(const uint8_t *pData, size_t Size);
		void OutputAccessUnit(
			uint32_t MPUSequenceNumber, bool IndexValid, uint32_t Index, bool RandomAccess,
			const uint8_t *pData, size_t Size);
		void DropCaptionUnit();

		MediaType m_MediaType;
		Handler *m_pHandler;
		uint16_t m_PacketID;
		MPUTimestampTable m_TimestampTable;
		StatisticsInfo m_Statistics;
		std::vector<MFUDataUnit> m_DataUnitList;

		bool m_HasSequenceNumber;
		uint32_t m_NextSequenceNumber;
		bool m_Discontinuity;

		bool m_IsFragmentStoring;
		uint32_t m_FragmentMPUSequenceNumber;
		DataBuffer m_FragmentBuffer;

		bool m_HasMPU;
		uint32_t m_MPUSequenceNumber;
		bool m_MPUIndexValid;
		uint32_t m_MPUUnitCount;

		bool m_UnitStarted;
		bool m_UnitValid;
		bool m_UnitIndexValid;
		uint32_t m_UnitIndex;
		uint32_t m_UnitMPUSequenceNumber;
		bool m_UnitRandomAccess;
		DataBuffer m_UnitBuffer;

		CaptionMFUHeader m_CaptionHeader;
		uint8_t m_CaptionNextSubsample;
		std::vector<SubsampleInfo> m_CaptionSubsampleList;
		std::vector<size_t> m_CaptionSubsampleOffset;
	};

	/** packet_id マップ用の MPU 組み立てクラス (アンマップ時に自身を削除する) */
	class MPUAssemblerTarget
		: public MMTPacketIDMapTarget
	{
	public:
		MPUAssemblerTarget(MPUAssembler::MediaType Type, MPUAssembler::Handler *pHandler = nullptr);

		bool StorePacket(const TLVPacket *pPacket) override;
		void OnPacketIDUnmapped(uint16_t PacketID) override;

		MPUAssembler & GetAssembler() noexcept { return m_Assembler; }
		const MPUAssembler & GetAssembler() const noexcept { return m_Assembler; }

	private:
		MPUAssembler m_Assembler;
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_MPU_ASSEMBLER_H
