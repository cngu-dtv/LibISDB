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
 @file   MMTDescriptors.hpp
 @brief  MMT-SI 記述子
 @author cngu-dtv
*/


#ifndef LIBISDB_MMT_DESCRIPTORS_H
#define LIBISDB_MMT_DESCRIPTORS_H


#include "MMTDescriptorBase.hpp"
#include "MMTGeneralLocationInfo.hpp"
#include "../TS/Descriptors.hpp"
#include "../Base/DataBuffer.hpp"
#include "../Base/DateTime.hpp"
#include "../Base/ARIBString.hpp"
#include <vector>


namespace LibISDB
{

	// 文字列は UTF-8 のまま保持し、String を返す関数で変換する

	/** MPU タイムスタンプ記述子クラス (ARIB STD-B60 7.4.3.5) */
	class MPUTimestampDescriptor
		: public MMTDescriptorTemplate<MPUTimestampDescriptor, 0x0001>
	{
	public:
		struct TimestampInfo {
			uint32_t MPUSequenceNumber;   /**< mpu_sequence_number */
			uint64_t MPUPresentationTime; /**< mpu_presentation_time (NTP 64 ビット形式) */
		};

		MPUTimestampDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MPUTimestampDescriptor
		int GetTimestampCount() const noexcept { return static_cast<int>(m_TimestampList.size()); }
		const TimestampInfo * GetTimestampInfo(int Index) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		std::vector<TimestampInfo> m_TimestampList;
	};

	/** アクセス制御記述子クラス (ARIB STD-B60 7.4.3.7) */
	class MMTAccessControlDescriptor
		: public MMTDescriptorTemplate<MMTAccessControlDescriptor, 0x8004>
	{
	public:
		MMTAccessControlDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MMTAccessControlDescriptor
		uint16_t GetCASystemID() const noexcept { return m_CASystemID; }
		const MMTGeneralLocationInfo & GetLocationInfo() const noexcept { return m_LocationInfo; }
		// location_type が 0x00 以外の場合は MMT_PACKET_ID_INVALID
		uint16_t GetPacketID() const noexcept;
		const DataBuffer * GetPrivateData() const noexcept { return &m_PrivateData; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint16_t m_CASystemID;                 /**< CA_system_ID */
		MMTGeneralLocationInfo m_LocationInfo; /**< MMT_general_location_info */
		DataBuffer m_PrivateData;              /**< private_data */
	};

	/** 映像コンポーネント記述子クラス (ARIB STD-B60 7.4.3.19) */
	class VideoComponentDescriptor
		: public MMTDescriptorTemplate<VideoComponentDescriptor, 0x8010>
	{
	public:
		// video_resolution
		static constexpr uint8_t VIDEO_RESOLUTION_UNSPECIFIED = 0_u8;
		static constexpr uint8_t VIDEO_RESOLUTION_180         = 1_u8;
		static constexpr uint8_t VIDEO_RESOLUTION_240         = 2_u8;
		static constexpr uint8_t VIDEO_RESOLUTION_480         = 3_u8;
		static constexpr uint8_t VIDEO_RESOLUTION_720         = 4_u8;
		static constexpr uint8_t VIDEO_RESOLUTION_1080        = 5_u8;
		static constexpr uint8_t VIDEO_RESOLUTION_2160        = 6_u8;
		static constexpr uint8_t VIDEO_RESOLUTION_4320        = 7_u8;

		VideoComponentDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// VideoComponentDescriptor
		uint8_t GetVideoResolution() const noexcept { return m_VideoResolution; }
		uint8_t GetVideoAspectRatio() const noexcept { return m_VideoAspectRatio; }
		bool GetVideoScanFlag() const noexcept { return m_VideoScanFlag; }
		uint8_t GetVideoFrameRate() const noexcept { return m_VideoFrameRate; }
		uint16_t GetComponentTag() const noexcept { return m_ComponentTag; }
		uint8_t GetVideoTransferCharacteristics() const noexcept { return m_VideoTransferCharacteristics; }
		uint32_t GetLanguageCode() const noexcept { return m_LanguageCode; }
		bool GetText(ReturnArg<String> Text) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_VideoResolution;              /**< video_resolution */
		uint8_t m_VideoAspectRatio;             /**< video_aspect_ratio */
		bool m_VideoScanFlag;                   /**< video_scan_flag (true: プログレッシブ) */
		uint8_t m_VideoFrameRate;               /**< video_frame_rate */
		uint16_t m_ComponentTag;                /**< component_tag */
		uint8_t m_VideoTransferCharacteristics; /**< video_transfer_characteristics */
		uint32_t m_LanguageCode;                /**< ISO_639_language_code */
		ARIBString m_Text;                      /**< text_char */
	};

	/** MH-ストリーム識別記述子クラス (ARIB STD-B60 7.4.3.20) */
	class MHStreamIdentifierDescriptor
		: public MMTDescriptorTemplate<MHStreamIdentifierDescriptor, 0x8011>
	{
	public:
		MHStreamIdentifierDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MHStreamIdentifierDescriptor
		uint16_t GetComponentTag() const noexcept { return m_ComponentTag; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint16_t m_ComponentTag; /**< component_tag */
	};

	/** MH-コンテント記述子クラス (ARIB STD-B60 7.4.3.21) */
	class MHContentDescriptor
		: public MMTDescriptorTemplate<MHContentDescriptor, 0x8012>
	{
	public:
		typedef ContentDescriptor::NibbleInfo NibbleInfo;

		MHContentDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MHContentDescriptor
		int GetNibbleCount() const noexcept { return static_cast<int>(m_NibbleList.size()); }
		bool GetNibble(int Index, ReturnArg<NibbleInfo> Nibble) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		std::vector<NibbleInfo> m_NibbleList;
	};

	/** MH-音声コンポーネント記述子クラス (ARIB STD-B60 7.4.3.23) */
	class MHAudioComponentDescriptor
		: public MMTDescriptorTemplate<MHAudioComponentDescriptor, 0x8014>
	{
	public:
		MHAudioComponentDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MHAudioComponentDescriptor
		uint8_t GetStreamContent() const noexcept { return m_StreamContent; }
		uint8_t GetComponentType() const noexcept { return m_ComponentType; }
		uint16_t GetComponentTag() const noexcept { return m_ComponentTag; }
		uint8_t GetStreamType() const noexcept { return m_StreamType; }
		uint8_t GetSimulcastGroupTag() const noexcept { return m_SimulcastGroupTag; }
		bool GetESMultiLingualFlag() const noexcept { return m_ESMultiLingualFlag; }
		bool GetMainComponentFlag() const noexcept { return m_MainComponentFlag; }
		uint8_t GetQualityIndicator() const noexcept { return m_QualityIndicator; }
		uint8_t GetSamplingRate() const noexcept { return m_SamplingRate; }
		uint32_t GetLanguageCode() const noexcept { return m_LanguageCode; }
		uint32_t GetLanguageCode2() const noexcept { return m_LanguageCode2; }
		bool GetText(ReturnArg<String> Text) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_StreamContent;     /**< stream_content */
		uint8_t m_ComponentType;     /**< component_type */
		uint16_t m_ComponentTag;     /**< component_tag */
		uint8_t m_StreamType;        /**< stream_type */
		uint8_t m_SimulcastGroupTag; /**< simulcast_group_tag */
		bool m_ESMultiLingualFlag;   /**< ES_multi_lingual_flag */
		bool m_MainComponentFlag;    /**< main_component_flag */
		uint8_t m_QualityIndicator;  /**< quality_indicator */
		uint8_t m_SamplingRate;      /**< sampling_rate */
		uint32_t m_LanguageCode;     /**< ISO_639_language_code */
		uint32_t m_LanguageCode2;    /**< ISO_639_language_code_2 */
		ARIBString m_Text;           /**< text_char */
	};

	/** MH-シリーズ記述子クラス (ARIB STD-B60 7.4.3.25) */
	class MHSeriesDescriptor
		: public MMTDescriptorTemplate<MHSeriesDescriptor, 0x8016>
	{
	public:
		MHSeriesDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MHSeriesDescriptor
		uint16_t GetSeriesID() const noexcept { return m_SeriesID; }
		uint8_t GetRepeatLabel() const noexcept { return m_RepeatLabel; }
		uint8_t GetProgramPattern() const noexcept { return m_ProgramPattern; }
		bool IsExpireDateValid() const noexcept { return m_ExpireDateValidFlag; }
		bool GetExpireDate(ReturnArg<DateTime> Date) const;
		uint16_t GetEpisodeNumber() const noexcept { return m_EpisodeNumber; }
		uint16_t GetLastEpisodeNumber() const noexcept { return m_LastEpisodeNumber; }
		bool GetSeriesName(ReturnArg<String> Name) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint16_t m_SeriesID;          /**< series_id */
		uint8_t m_RepeatLabel;        /**< repeat_label */
		uint8_t m_ProgramPattern;     /**< program_pattern */
		bool m_ExpireDateValidFlag;   /**< expire_date_valid_flag */
		DateTime m_ExpireDate;        /**< expire_date */
		uint16_t m_EpisodeNumber;     /**< episode_number */
		uint16_t m_LastEpisodeNumber; /**< last_episode_number */
		ARIBString m_SeriesName;      /**< series_name_char */
	};

	/** MH-イベントグループ記述子クラス (ARIB STD-B60 7.4.3.15) */
	class MHEventGroupDescriptor
		: public MMTDescriptorTemplate<MHEventGroupDescriptor, 0x800C>
	{
	public:
		// TransportStreamID には tlv_stream_id が入る
		typedef EventGroupDescriptor::EventInfo EventInfo;

		MHEventGroupDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MHEventGroupDescriptor
		uint8_t GetGroupType() const noexcept { return m_GroupType; }
		int GetEventCount() const noexcept { return static_cast<int>(m_EventList.size()); }
		bool GetEventInfo(int Index, ReturnArg<EventInfo> Info) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_GroupType; /**< group_type */
		std::vector<EventInfo> m_EventList;
	};

	/** MH-サービスリスト記述子クラス (ARIB STD-B60 7.4.3.16) */
	class MHServiceListDescriptor
		: public MMTDescriptorTemplate<MHServiceListDescriptor, 0x800D>
	{
	public:
		typedef ServiceListDescriptor::ServiceInfo ServiceInfo;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MHServiceListDescriptor
		int GetServiceCount() const noexcept { return static_cast<int>(m_ServiceList.size()); }
		bool GetServiceInfo(int Index, ReturnArg<ServiceInfo> Info) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		std::vector<ServiceInfo> m_ServiceList;
	};

	/** MH-事業者名記述子クラス (ARIB STD-B60 7.4.3.27) */
	class MHBroadcasterNameDescriptor
		: public MMTDescriptorTemplate<MHBroadcasterNameDescriptor, 0x8018>
	{
	public:
	// MMTDescriptorBase
		void Reset() noexcept override;

	// MHBroadcasterNameDescriptor
		bool GetBroadcasterName(ReturnArg<String> Name) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		ARIBString m_BroadcasterName; /**< char */
	};

	/** MH-サービス記述子クラス (ARIB STD-B60 7.4.3.28) */
	class MHServiceDescriptor
		: public MMTDescriptorTemplate<MHServiceDescriptor, 0x8019>
	{
	public:
		MHServiceDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MHServiceDescriptor
		uint8_t GetServiceType() const noexcept { return m_ServiceType; }
		bool GetProviderName(ReturnArg<String> Name) const;
		bool GetServiceName(ReturnArg<String> Name) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_ServiceType;     /**< service_type */
		ARIBString m_ProviderName; /**< service_provider_name */
		ARIBString m_ServiceName;  /**< service_name */
	};

	/** MH-データ符号化方式記述子クラス (ARIB STD-B60 7.4.3.31) */
	class MHDataComponentDescriptor
		: public MMTDescriptorTemplate<MHDataComponentDescriptor, 0x8020>
	{
	public:
		// data_component_id (ARIB STD-B60 表 7-74)
		static constexpr uint16_t DATA_COMPONENT_ID_CAPTION    = 0x0020_u16; // 字幕
		static constexpr uint16_t DATA_COMPONENT_ID_MULTIMEDIA = 0x0021_u16; // マルチメディア
		static constexpr uint16_t DATA_COMPONENT_ID_INVALID    = 0xFFFF_u16;

		/** 字幕・文字スーパーの付加識別情報 (ARIB STD-B60 表 9-3) */
		struct AribSubtitleInfo {
			uint8_t SubtitleTag;                     /**< subtitle_tag */
			uint8_t SubtitleInfoVersion;             /**< subtitle_info_version */
			bool StartMPUSequenceNumberFlag;         /**< start_mpu_sequence_number_flag */
			uint32_t LanguageCode;                   /**< ISO_639_language_code */
			uint8_t Type;                            /**< type */
			uint8_t SubtitleFormat;                  /**< subtitle_format */
			uint8_t OPM;                             /**< OPM */
			uint8_t TMD;                             /**< TMD */
			uint8_t DMF;                             /**< DMF */
			uint8_t Resolution;                      /**< resolution */
			uint8_t CompressionType;                 /**< compression_type */
			uint32_t StartMPUSequenceNumber;         /**< start_mpu_sequence_number */
			uint64_t ReferenceStartTime;             /**< reference_start_time (NTP 64 ビット形式) */
			uint8_t ReferenceStartTimeLeapIndicator; /**< reference_start_time_leap_indicator */
		};

		// type (ARIB STD-B60 表 9-4)
		static constexpr uint8_t SUBTITLE_TYPE_CAPTION     = 0x0_u8; // 字幕
		static constexpr uint8_t SUBTITLE_TYPE_SUPERIMPOSE = 0x1_u8; // 文字スーパー

		// TMD (ARIB STD-B60 表 9-7)
		static constexpr uint8_t TMD_UTC                  = 0x0_u8; // TTML 記述 (UTC)
		static constexpr uint8_t TMD_EIT_START_TIME       = 0x1_u8; // TTML 記述 (MH-EIT の開始時刻起点)
		static constexpr uint8_t TMD_REFERENCE_START_TIME = 0x2_u8; // TTML 記述 (reference_start_time 起点)
		static constexpr uint8_t TMD_MPU_TIMESTAMP_BASE   = 0x3_u8; // TTML 記述 (MPU タイムスタンプ起点)
		static constexpr uint8_t TMD_NPT                  = 0x4_u8; // TTML 記述 (NPT)
		static constexpr uint8_t TMD_MPU_TIMESTAMP        = 0x8_u8; // MPU タイムスタンプ
		static constexpr uint8_t TMD_NO_TIME_CONTROL      = 0xF_u8; // 時刻制御なし

		// compression_type (ARIB STD-B60 表 9-10)
		static constexpr uint8_t COMPRESSION_TYPE_NONE = 0x0_u8; // 非圧縮

		MHDataComponentDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MHDataComponentDescriptor
		uint16_t GetDataComponentID() const noexcept { return m_DataComponentID; }
		const DataBuffer * GetAdditionalDataComponentInfo() const noexcept { return &m_AdditionalDataComponentInfo; }
		// data_component_id が字幕 (0x0020) で付加識別情報が正しい場合のみ true
		bool GetAribSubtitleInfo(ReturnArg<AribSubtitleInfo> Info) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint16_t m_DataComponentID;               /**< data_component_id */
		DataBuffer m_AdditionalDataComponentInfo; /**< additional_data_component_info */
	};

	/** MH-ローカル時間オフセット記述子クラス (ARIB STD-B60 7.4.3.32) */
	class MHLocalTimeOffsetDescriptor
		: public MMTDescriptorTemplate<MHLocalTimeOffsetDescriptor, 0x8023>
	{
	public:
		typedef LocalTimeOffsetDescriptor::TimeOffsetInfo TimeOffsetInfo;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MHLocalTimeOffsetDescriptor
		int GetTimeOffsetInfoCount() const noexcept { return static_cast<int>(m_TimeOffsetList.size()); }
		bool GetTimeOffsetInfo(int Index, ReturnArg<TimeOffsetInfo> Info) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		std::vector<TimeOffsetInfo> m_TimeOffsetList;
	};

	/** MH-ロゴ伝送記述子クラス (ARIB STD-B60 7.4.3.34) */
	class MHLogoTransmissionDescriptor
		: public MMTDescriptorTemplate<MHLogoTransmissionDescriptor, 0x8025>
	{
	public:
		// logo_transmission_type
		static constexpr uint8_t TRANSMISSION_CDT1 = 0x01_u8; // CDT 伝送方式 1
		static constexpr uint8_t TRANSMISSION_CDT2 = 0x02_u8; // CDT 伝送方式 2
		static constexpr uint8_t TRANSMISSION_CHAR = 0x03_u8; // 簡易ロゴ方式

		static constexpr uint16_t LOGO_ID_INVALID      = LogoTransmissionDescriptor::LOGO_ID_INVALID;
		static constexpr uint16_t LOGO_VERSION_INVALID = LogoTransmissionDescriptor::LOGO_VERSION_INVALID;
		static constexpr uint16_t DATA_ID_INVALID      = LogoTransmissionDescriptor::DATA_ID_INVALID;

		struct LogoInfo {
			uint8_t LogoType;           /**< logo_type */
			uint8_t StartSectionNumber; /**< start_section_number */
			uint8_t NumOfSections;      /**< num_of_sections */
		};

		MHLogoTransmissionDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MHLogoTransmissionDescriptor
		uint8_t GetLogoTransmissionType() const noexcept { return m_LogoTransmissionType; }
		uint16_t GetLogoID() const noexcept { return m_LogoID; }
		uint16_t GetLogoVersion() const noexcept { return m_LogoVersion; }
		uint16_t GetDownloadDataID() const noexcept { return m_DownloadDataID; }
		int GetLogoCount() const noexcept { return static_cast<int>(m_LogoList.size()); }
		bool GetLogoInfo(int Index, ReturnArg<LogoInfo> Info) const;
		bool GetLogoChar(ReturnArg<String> Char) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_LogoTransmissionType; /**< logo_transmission_type */
		uint16_t m_LogoID;              /**< logo_id */
		uint16_t m_LogoVersion;         /**< logo_version */
		uint16_t m_DownloadDataID;      /**< download_data_id */
		std::vector<LogoInfo> m_LogoList;
		ARIBString m_LogoChar;          /**< logo_char */
	};

	/** MPU 拡張タイムスタンプ記述子クラス (ARIB STD-B60 7.4.3.35) */
	class MPUExtendedTimestampDescriptor
		: public MMTDescriptorTemplate<MPUExtendedTimestampDescriptor, 0x8026>
	{
	public:
		// pts_offset_type
		static constexpr uint8_t PTS_OFFSET_TYPE_FIXED      = 0_u8; // 規定の固定値
		static constexpr uint8_t PTS_OFFSET_TYPE_DEFAULT    = 1_u8; // default_pts_offset
		static constexpr uint8_t PTS_OFFSET_TYPE_INDIVIDUAL = 2_u8; // アクセスユニット毎の pts_offset

		struct AccessUnitInfo {
			uint16_t DTSPTSOffset; /**< dts_pts_offset */
			uint16_t PTSOffset;    /**< pts_offset (pts_offset_type 1 では default_pts_offset) */
		};

		struct MPUInfo {
			uint32_t MPUSequenceNumber;                 /**< mpu_sequence_number */
			uint8_t MPUPresentationTimeLeapIndicator;   /**< mpu_presentation_time_leap_indicator */
			uint16_t MPUDecodingTimeOffset;             /**< mpu_decoding_time_offset */
			std::vector<AccessUnitInfo> AccessUnitList; /**< 復号順 */
		};

		MPUExtendedTimestampDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MPUExtendedTimestampDescriptor
		uint8_t GetPTSOffsetType() const noexcept { return m_PTSOffsetType; }
		bool GetTimescaleFlag() const noexcept { return m_TimescaleFlag; }
		uint32_t GetTimescale() const noexcept { return m_Timescale; }
		uint16_t GetDefaultPTSOffset() const noexcept { return m_DefaultPTSOffset; }
		int GetMPUCount() const noexcept { return static_cast<int>(m_MPUList.size()); }
		const MPUInfo * GetMPUInfo(int Index) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_PTSOffsetType;     /**< pts_offset_type */
		bool m_TimescaleFlag;        /**< timescale_flag */
		uint32_t m_Timescale;        /**< timescale */
		uint16_t m_DefaultPTSOffset; /**< default_pts_offset */
		std::vector<MPUInfo> m_MPUList;
	};

	/** アプリケーションサービス記述子クラス (ARIB STD-B60 7.4.3.39) */
	class ApplicationServiceDescriptor
		: public MMTDescriptorTemplate<ApplicationServiceDescriptor, 0x8034>
	{
	public:
		// application_format
		static constexpr uint8_t APPLICATION_FORMAT_HTML5 = 0x1_u8; // ARIB-HTML5

		// document_resolution (ARIB STD-B60 表 7-88)
		static constexpr uint8_t DOCUMENT_RESOLUTION_1920x1080 = 0x0_u8;
		static constexpr uint8_t DOCUMENT_RESOLUTION_3840x2160 = 0x1_u8;
		static constexpr uint8_t DOCUMENT_RESOLUTION_7680x4320 = 0x2_u8;

		struct EMTInfo {
			uint8_t EMTTag;                      /**< EMT_tag */
			MMTGeneralLocationInfo LocationInfo; /**< EMT_location_info */
		};

		ApplicationServiceDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// ApplicationServiceDescriptor
		uint8_t GetApplicationFormat() const noexcept { return m_ApplicationFormat; }
		uint8_t GetDocumentResolution() const noexcept { return m_DocumentResolution; }
		bool GetDefaultAITFlag() const noexcept { return m_DefaultAITFlag; }
		bool GetDTMessageFlag() const noexcept { return m_DTMessageFlag; }
		const MMTGeneralLocationInfo & GetAITLocationInfo() const noexcept { return m_AITLocationInfo; }
		const MMTGeneralLocationInfo & GetDTMessageLocationInfo() const noexcept { return m_DTMessageLocationInfo; }
		int GetEMTCount() const noexcept { return static_cast<int>(m_EMTList.size()); }
		const EMTInfo * GetEMTInfo(int Index) const;
		// location_type が 0x00 以外の場合 (データ伝送メッセージが無い場合も) は MMT_PACKET_ID_INVALID
		uint16_t GetAITPacketID() const noexcept;
		uint16_t GetDTMessagePacketID() const noexcept;
		uint16_t GetEMTPacketID(int Index) const noexcept;
		const DataBuffer * GetPrivateData() const noexcept { return &m_PrivateData; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_ApplicationFormat;                     /**< application_format */
		uint8_t m_DocumentResolution;                    /**< document_resolution */
		bool m_DefaultAITFlag;                           /**< default_AIT_flag */
		bool m_DTMessageFlag;                            /**< DT_message_flag */
		MMTGeneralLocationInfo m_AITLocationInfo;        /**< AIT_location_info */
		MMTGeneralLocationInfo m_DTMessageLocationInfo;  /**< DT_message_location_info */
		std::vector<EMTInfo> m_EMTList;
		DataBuffer m_PrivateData;                        /**< private_data */
	};

	/** MH-短形式イベント記述子クラス (ARIB STD-B60 7.4.3.17) */
	class MHShortEventDescriptor
		: public MMTDescriptorTemplate<MHShortEventDescriptor, 0xF001>
	{
	public:
		MHShortEventDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MHShortEventDescriptor
		uint32_t GetLanguageCode() const noexcept { return m_LanguageCode; }
		bool GetEventName(ReturnArg<String> Name) const;
		bool GetEventDescription(ReturnArg<String> Description) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint32_t m_LanguageCode;       /**< ISO_639_language_code */
		ARIBString m_EventName;        /**< event_name_char */
		ARIBString m_EventDescription; /**< text_char */
	};

	/** MH-拡張形式イベント記述子クラス (ARIB STD-B60 7.4.3.18) */
	class MHExtendedEventDescriptor
		: public MMTDescriptorTemplate<MHExtendedEventDescriptor, 0xF002>
	{
	public:
		struct ItemInfo {
			ARIBString Description; /**< item_description_char */
			ARIBString ItemChar;    /**< item_char */
		};

		MHExtendedEventDescriptor() noexcept;

	// MMTDescriptorBase
		void Reset() noexcept override;

	// MHExtendedEventDescriptor
		uint8_t GetDescriptorNumber() const noexcept { return m_DescriptorNumber; }
		uint8_t GetLastDescriptorNumber() const noexcept { return m_LastDescriptorNumber; }
		uint32_t GetLanguageCode() const noexcept { return m_LanguageCode; }
		int GetItemCount() const noexcept { return static_cast<int>(m_ItemList.size()); }
		const ItemInfo * GetItem(int Index) const;
		bool GetText(ReturnArg<String> Text) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_DescriptorNumber;     /**< descriptor_number */
		uint8_t m_LastDescriptorNumber; /**< last_descriptor_number */
		uint32_t m_LanguageCode;        /**< ISO_639_language_code */
		std::vector<ItemInfo> m_ItemList;
		ARIBString m_Text;              /**< text_char */
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_MMT_DESCRIPTORS_H
