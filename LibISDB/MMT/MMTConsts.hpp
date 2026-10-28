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

} // namespace LibISDB


#endif // ifndef LIBISDB_MMT_CONSTS_H
