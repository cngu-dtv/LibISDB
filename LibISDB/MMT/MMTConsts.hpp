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
 @file   MMTConsts.hpp
 @brief  MMT の定数
 @author cngu-dtv
*/


#ifndef LIBISDB_MMT_CONSTS_H
#define LIBISDB_MMT_CONSTS_H


namespace LibISDB
{

	/*
		MMTP パケット (ARIB STD-B60 6.4.1.1)
	*/
	constexpr size_t MMTP_HEADER_SIZE_MIN = 12;

	/*
		FEC_type
	*/
	constexpr uint8_t MMTP_FEC_TYPE_NONE   = 0_u8; // AL-FEC で保護されていない
	constexpr uint8_t MMTP_FEC_TYPE_SOURCE = 1_u8; // AL-FEC のソースパケット
	constexpr uint8_t MMTP_FEC_TYPE_REPAIR = 2_u8; // AL-FEC のリペアパケット

	/*
		payload_type
	*/
	constexpr uint8_t MMTP_PAYLOAD_TYPE_MPU            = 0x00_u8; // MPU
	constexpr uint8_t MMTP_PAYLOAD_TYPE_GENERIC_OBJECT = 0x01_u8; // 汎用オブジェクト (運用されない)
	constexpr uint8_t MMTP_PAYLOAD_TYPE_SIGNALING      = 0x02_u8; // 制御メッセージ
	constexpr uint8_t MMTP_PAYLOAD_TYPE_REPAIR_SYMBOL  = 0x03_u8; // AL-FEC のリペアシンボル

	/*
		extension_type
	*/
	constexpr uint16_t MMTP_EXTENSION_TYPE_MULTI = 0x0000_u16; // マルチタイプヘッダ拡張

	/*
		hdr_ext_type (ARIB STD-B60 表 6-9)
	*/
	constexpr uint16_t MMTP_HDR_EXT_TYPE_SCRAMBLE      = 0x0001_u16; // スクランブル情報 (ARIB STD-B61)
	constexpr uint16_t MMTP_HDR_EXT_TYPE_DOWNLOAD_ID   = 0x0002_u16; // ダウンロード ID
	constexpr uint16_t MMTP_HDR_EXT_TYPE_FILE_FRAGMENT = 0x0003_u16; // ファイル分割伝送情報

	/*
		MMT_scrambling_control (ARIB STD-B61 表 3-1)
	*/
	constexpr uint8_t MMT_SCRAMBLING_CONTROL_NONE = 0x00_u8; // スクランブルなし
	constexpr uint8_t MMT_SCRAMBLING_CONTROL_EVEN = 0x02_u8; // 偶数鍵でスクランブル
	constexpr uint8_t MMT_SCRAMBLING_CONTROL_ODD  = 0x03_u8; // 奇数鍵でスクランブル

	constexpr size_t MMT_SICV_SIZE = 16;

	/*
		fragment_type (ARIB STD-B60 表 6-2)
	*/
	constexpr uint8_t MPU_FRAGMENT_TYPE_MPU_METADATA      = 0_u8; // MPU メタデータ (運用されない)
	constexpr uint8_t MPU_FRAGMENT_TYPE_FRAGMENT_METADATA = 1_u8; // ムービーフラグメントメタデータ (運用されない)
	constexpr uint8_t MPU_FRAGMENT_TYPE_MFU               = 2_u8; // MFU

	/*
		fragmentation_indicator (ARIB STD-B60 表 6-3)
	*/
	constexpr uint8_t MMT_FRAGMENTATION_INDICATOR_COMPLETE = 0_u8; // 分割されていない
	constexpr uint8_t MMT_FRAGMENTATION_INDICATOR_FIRST    = 1_u8; // 分割の先頭
	constexpr uint8_t MMT_FRAGMENTATION_INDICATOR_MIDDLE   = 2_u8; // 分割の途中
	constexpr uint8_t MMT_FRAGMENTATION_INDICATOR_LAST     = 3_u8; // 分割の最後

	/*
		字幕・文字スーパーの data_type (ARIB STD-B60 表 9-2)
	*/
	constexpr uint8_t MMT_CAPTION_DATA_TYPE_TTML      = 0x0_u8; // TTML 文書
	constexpr uint8_t MMT_CAPTION_DATA_TYPE_PNG       = 0x1_u8; // PNG
	constexpr uint8_t MMT_CAPTION_DATA_TYPE_SVG       = 0x2_u8; // SVG
	constexpr uint8_t MMT_CAPTION_DATA_TYPE_PCM       = 0x3_u8; // PCM (AIFF-C)
	constexpr uint8_t MMT_CAPTION_DATA_TYPE_MP3       = 0x4_u8; // MP3
	constexpr uint8_t MMT_CAPTION_DATA_TYPE_AAC       = 0x5_u8; // MPEG-4 AAC
	constexpr uint8_t MMT_CAPTION_DATA_TYPE_SVG_FONT  = 0x6_u8; // SVG フォント
	constexpr uint8_t MMT_CAPTION_DATA_TYPE_WOFF_FONT = 0x7_u8; // WOFF フォント

	/*
		packet_id (ARIB STD-B60 表 4-12)
	*/
	constexpr uint16_t MMT_PACKET_ID_PA      = 0x0000_u16; // PA メッセージ
	constexpr uint16_t MMT_PACKET_ID_CA      = 0x0001_u16; // CA メッセージ
	constexpr uint16_t MMT_PACKET_ID_AL_FEC  = 0x0002_u16; // AL-FEC メッセージ
	constexpr uint16_t MMT_PACKET_ID_MH_EIT  = 0x8000_u16; // M2 セクションメッセージ (MH-EIT)
	constexpr uint16_t MMT_PACKET_ID_MH_AIT  = 0x8001_u16; // M2 セクションメッセージ (MH-AIT)
	constexpr uint16_t MMT_PACKET_ID_MH_BIT  = 0x8002_u16; // M2 セクションメッセージ (MH-BIT)
	constexpr uint16_t MMT_PACKET_ID_MH_SDTT = 0x8003_u16; // M2 セクションメッセージ (MH-SDTT)
	constexpr uint16_t MMT_PACKET_ID_MH_SDT  = 0x8004_u16; // M2 セクションメッセージ (MH-SDT)
	constexpr uint16_t MMT_PACKET_ID_MH_TOT  = 0x8005_u16; // M2 ショートセクションメッセージ (MH-TOT)
	constexpr uint16_t MMT_PACKET_ID_MH_CDT  = 0x8006_u16; // M2 セクションメッセージ (MH-CDT)
	constexpr uint16_t MMT_PACKET_ID_DATA    = 0x8007_u16; // データ伝送メッセージ
	constexpr uint16_t MMT_PACKET_ID_MH_DIT  = 0x8008_u16; // M2 ショートセクションメッセージ (MH-DIT)
	constexpr uint16_t MMT_PACKET_ID_MH_SIT  = 0x8009_u16; // M2 セクションメッセージ (MH-SIT)
	constexpr uint16_t MMT_PACKET_ID_INVALID = 0xFFFF_u16; // 無効

	/*
		message_id (ARIB STD-B60 表 4-6)
	*/
	constexpr uint16_t MMT_MESSAGE_ID_PA                = 0x0000_u16; // PA メッセージ
	constexpr uint16_t MMT_MESSAGE_ID_M2_SECTION        = 0x8000_u16; // M2 セクションメッセージ
	constexpr uint16_t MMT_MESSAGE_ID_CA                = 0x8001_u16; // CA メッセージ
	constexpr uint16_t MMT_MESSAGE_ID_M2_SHORT_SECTION  = 0x8002_u16; // M2 短セクションメッセージ
	constexpr uint16_t MMT_MESSAGE_ID_DATA_TRANSMISSION = 0x8003_u16; // データ伝送メッセージ

	/*
		table_id (ARIB STD-B60 表 4-8)
	*/
	constexpr uint8_t MMT_TABLE_ID_MPT                  = 0x20_u8; // MPT
	constexpr uint8_t MMT_TABLE_ID_PLT                  = 0x80_u8; // PLT
	constexpr uint8_t MMT_TABLE_ID_LCT                  = 0x81_u8; // LCT
	constexpr uint8_t MMT_TABLE_ID_ECM_0                = 0x82_u8; // ECM
	constexpr uint8_t MMT_TABLE_ID_ECM_1                = 0x83_u8; // ECM
	constexpr uint8_t MMT_TABLE_ID_EMM_0                = 0x84_u8; // EMM
	constexpr uint8_t MMT_TABLE_ID_EMM_1                = 0x85_u8; // EMM
	constexpr uint8_t MMT_TABLE_ID_CAT                  = 0x86_u8; // CAT (MH)
	constexpr uint8_t MMT_TABLE_ID_MH_EIT_PF            = 0x8B_u8; // MH-EIT [p/f]
	constexpr uint8_t MMT_TABLE_ID_MH_EIT_SCHEDULE_FIRST = 0x8C_u8; // MH-EIT [schedule]
	constexpr uint8_t MMT_TABLE_ID_MH_EIT_SCHEDULE_LAST  = 0x9B_u8; // MH-EIT [schedule]
	constexpr uint8_t MMT_TABLE_ID_MH_AIT               = 0x9C_u8; // MH-AIT
	constexpr uint8_t MMT_TABLE_ID_MH_BIT               = 0x9D_u8; // MH-BIT
	constexpr uint8_t MMT_TABLE_ID_MH_SDTT              = 0x9E_u8; // MH-SDTT
	constexpr uint8_t MMT_TABLE_ID_MH_SDT_ACTUAL        = 0x9F_u8; // MH-SDT (自ストリーム)
	constexpr uint8_t MMT_TABLE_ID_MH_SDT_OTHER         = 0xA0_u8; // MH-SDT (他ストリーム)
	constexpr uint8_t MMT_TABLE_ID_MH_TOT               = 0xA1_u8; // MH-TOT
	constexpr uint8_t MMT_TABLE_ID_MH_CDT               = 0xA2_u8; // MH-CDT
	constexpr uint8_t MMT_TABLE_ID_MH_DIT               = 0xA7_u8; // MH-DIT
	constexpr uint8_t MMT_TABLE_ID_MH_SIT               = 0xA8_u8; // MH-SIT

	/*
		asset_type (ARIB STD-B60 表 7-8)
	*/
	constexpr uint32_t MMT_ASSET_TYPE_HEV1 = 0x68657631_u32; // 'hev1' HEVC (VPS/SPS/PPS を MFU に含む)
	constexpr uint32_t MMT_ASSET_TYPE_HVC1 = 0x68766331_u32; // 'hvc1' HEVC (VPS/SPS/PPS は MPU メタデータのみ)
	constexpr uint32_t MMT_ASSET_TYPE_MP4A = 0x6D703461_u32; // 'mp4a' MPEG-4 オーディオ
	constexpr uint32_t MMT_ASSET_TYPE_STPP = 0x73747070_u32; // 'stpp' 字幕・文字スーパー
	constexpr uint32_t MMT_ASSET_TYPE_AAPP = 0x61617070_u32; // 'aapp' アプリケーション
	constexpr uint32_t MMT_ASSET_TYPE_ASGD = 0x61736764_u32; // 'asgd' 同期型汎用データ
	constexpr uint32_t MMT_ASSET_TYPE_AAGD = 0x61616764_u32; // 'aagd' 非同期型汎用データ

	constexpr uint16_t MMT_COMPONENT_TAG_INVALID = 0xFFFF_u16;

	/*
		字幕・文字スーパーの component_tag (ARIB TR-B39 第1部 第3編 8.2.3.1, 9.2.3.1)
	*/
	constexpr uint16_t MMT_COMPONENT_TAG_CAPTION_FIRST     = 0x0030_u16; // 字幕 (既定)
	constexpr uint16_t MMT_COMPONENT_TAG_CAPTION_LAST      = 0x0037_u16;
	constexpr uint16_t MMT_COMPONENT_TAG_SUPERIMPOSE_FIRST = 0x0038_u16; // 文字スーパー (既定)
	constexpr uint16_t MMT_COMPONENT_TAG_SUPERIMPOSE_LAST  = 0x003F_u16;

	/*
		HEVC の nal_unit_type (ITU-T H.265 表 7-1)
	*/
	constexpr uint8_t HEVC_NAL_UNIT_TYPE_IRAP_FIRST = 16_u8; // BLA_W_LP
	constexpr uint8_t HEVC_NAL_UNIT_TYPE_IRAP_LAST  = 23_u8; // RSV_IRAP_VCL23
	constexpr uint8_t HEVC_NAL_UNIT_TYPE_AUD        = 35_u8; // アクセスユニットデリミタ

} // namespace LibISDB


#endif // ifndef LIBISDB_MMT_CONSTS_H
