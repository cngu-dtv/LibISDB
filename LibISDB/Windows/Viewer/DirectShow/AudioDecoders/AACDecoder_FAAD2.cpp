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
 @file   AACDecoder_FAAD2.cpp
 @brief  AAC デコーダ (FAAD2)
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "AACDecoder_FAAD2.hpp"

#include "../../../../../Thirdparty/faad2/include/neaacdec.h"
// ダウンミックス係数のパラメータ取得にFAAD2の内部情報が必要
#define HAVE_CONFIG_H
#pragma include_alias("neaacdec.h", "../../../../../Thirdparty/faad2/include/neaacdec.h")
#include "../../../../../Thirdparty/faad2/libfaad/common.h"
#include "../../../../../Thirdparty/faad2/libfaad/structs.h"

#include "../../../../Base/DebugDef.hpp"


#pragma comment(lib, "libfaad.lib")


namespace LibISDB::DirectShow
{


void AACDecoder_FAAD2::GetVersion(std::string *pVersion)
{
	char *pID;
	::NeAACDecGetVersion(&pID, nullptr);
	*pVersion = pID;
}


AACDecoder_FAAD2::AACDecoder_FAAD2(TransportType Transport) noexcept
	: AACDecoder(Transport)
	, m_hDecoder(nullptr)
	, m_InitRequest(false)
	, m_LastChannelConfig(0xFF)
{
}


AACDecoder_FAAD2::~AACDecoder_FAAD2()
{
	Close();
}


bool AACDecoder_FAAD2::IsOpened() const
{
	return m_hDecoder != nullptr;
}


bool AACDecoder_FAAD2::GetChannelMap(int Channels, int *pMap) const
{
	switch (Channels) {
	case 2:
		pMap[CHANNEL_2_L] = 0;
		pMap[CHANNEL_2_R] = 1;
		break;

	case 6:
		pMap[CHANNEL_6_FL]  = 1;
		pMap[CHANNEL_6_FR]  = 2;
		pMap[CHANNEL_6_FC]  = 0;
		pMap[CHANNEL_6_LFE] = 5;
		pMap[CHANNEL_6_BL]  = 3;
		pMap[CHANNEL_6_BR]  = 4;
		break;

	default:
		return false;
	}

	return true;
}


bool AACDecoder_FAAD2::GetDownmixInfo(ReturnArg<DownmixInfo> Info) const
{
	if (!Info)
		return false;

	// 5.1chダウンミックス設定
	/*
	ダウンミックス計算式 (STD-B21 6.2.1)
	┌─┬──┬─┬───┬───────────┐
	│*1│*2  │*3│kの値 │計算式(*4)            │
	├─┼──┼─┼───┼───────────┤
	│ 1│ 0/1│ 0│1/√2 │Set1                  │
	│  │    │ 1│1/2   │Lt=L+1/√2*C＋k*Ls    │
	│  │    │ 2│1/2√2│Rt=R+1/√2*C＋k*Rs    │
	│  │    │ 3│0     │                      │
	├─┼──┼─┼───┼───────────┤
	│ 0│    │  │      │Set3                  │
	│  │    │  │      │Lt=L+1/√2*C＋1/√2*Ls│
	│  │    │  │      │Rt=R+1/√2*C＋1/√2*Rs│
	└─┴──┴─┴───┴───────────┘
	*1 matrix_mixdown_idx_present
	*2 pseudo_surround_enable
	*3 matrix_mixdown_idx
	*4 L=Left, R=Right, C=Center, Ls=Rear left, Rs=Rear right

	STD-B21 5.3版より前は、最終的に Lt/Rt に 1/√2 を乗じる規定(STD-B21 付属4参照)
	ただしTVTestでは元々この規定は無視していた
	*/

	constexpr double PSQR = 1.0 / 1.4142135623730950488016887242097;

	Info->Center = PSQR;
	Info->Front  = 1.0;
	Info->Rear   = PSQR;
	Info->LFE    = 0.0;

	// 5.1ch を超える音声モードでは係数は DSE で送られる (STD-B32 第 2 部 6.2.3 (3) (e))
	if (m_DownmixCoefficients.HasMixLevels) {
		Info->Center = AACDownmixLevelToGain(m_DownmixCoefficients.CenterMixLevel);
		Info->Rear = AACDownmixLevelToGain(m_DownmixCoefficients.SurroundMixLevel);
	} else if (m_hDecoder) {
		const NeAACDecStruct *pDec = static_cast<const NeAACDecStruct *>(m_hDecoder);

		if (pDec->pce.matrix_mixdown_idx_present) {
			static const double k[4] = {PSQR, 0.5, 0.5 * PSQR, 0.0};
			Info->Rear = pDec->pce.matrix_mixdown_idx <= 3 ? k[pDec->pce.matrix_mixdown_idx] : PSQR;
		}
	}

	return true;
}


bool AACDecoder_FAAD2::OpenDecoder()
{
	CloseDecoder();

	// FAAD2オープン
	m_hDecoder = ::NeAACDecOpen();
	if (m_hDecoder == nullptr)
		return false;

	// デフォルト設定取得
	NeAACDecConfigurationPtr pDecodeConfig = ::NeAACDecGetCurrentConfiguration(m_hDecoder);

	// デコーダ設定
	pDecodeConfig->defObjectType = LC;
	pDecodeConfig->defSampleRate = 48000UL;
	pDecodeConfig->outputFormat = FAAD_FMT_16BIT;
	pDecodeConfig->downMatrix = 0;
	pDecodeConfig->useOldADTSFormat = 0;

	if (!::NeAACDecSetConfiguration(m_hDecoder, pDecodeConfig)) {
		Close();
		return false;
	}

	m_InitRequest = true;
	m_LastChannelConfig = 0xFF;
	m_DecodeError = false;
	m_DownmixCoefficients = AACDownmixCoefficients();

	return true;
}


void AACDecoder_FAAD2::CloseDecoder()
{
	// FAAD2クローズ
	if (m_hDecoder != nullptr) {
		::NeAACDecClose(m_hDecoder);
		m_hDecoder = nullptr;
	}
}


bool AACDecoder_FAAD2::DecodeFrame(const ADTSFrame *pFrame, ReturnArg<DecodeFrameInfo> Info)
{
	if (m_hDecoder == nullptr) {
		return false;
	}

	// 初回フレーム解析
	if (m_InitRequest || (pFrame->GetChannelConfig() != m_LastChannelConfig)) {
		if (!m_InitRequest) {
			// チャンネル設定が変化した、デコーダリセット
			LIBISDB_TRACE(
				LIBISDB_STR("AACDecoder_FAAD2::DecodeFrame() Channel config changed {} -> {}\n"),
				m_LastChannelConfig,
				pFrame->GetChannelConfig());
			if (!ResetDecoder())
				return false;
		}

		unsigned long SampleRate;
		unsigned char Channels;
		if (::NeAACDecInit(m_hDecoder,
				const_cast<unsigned char *>(pFrame->GetData()),
				static_cast<unsigned long>(pFrame->GetSize()),
				&SampleRate, &Channels) < 0) {
			return false;
		}

		m_InitRequest = false;
		m_LastChannelConfig = pFrame->GetChannelConfig();
	}

	return DecodeData(pFrame->GetData(), pFrame->GetSize(), Info);
}


bool AACDecoder_FAAD2::DecodeLATMFrame(const uint8_t *pData, size_t Size, ReturnArg<DecodeFrameInfo> Info)
{
	if (m_hDecoder == nullptr) {
		return false;
	}

	const LATMParser::AudioConfig &Config = m_LATMParser.GetConfig();

	if (m_InitRequest || m_LATMParser.IsConfigChanged()) {
		if (!m_InitRequest) {
			LIBISDB_TRACE(
				LIBISDB_STR("AACDecoder_FAAD2::DecodeLATMFrame() Config changed (channel config {} -> {})\n"),
				m_LastChannelConfig,
				Config.ChannelConfiguration);
			if (!ResetDecoder())
				return false;
		}

		std::vector<uint8_t> SpecificConfig = Config.SpecificConfig;
		unsigned long SampleRate;
		unsigned char Channels;
		if (SpecificConfig.empty()
				|| (::NeAACDecInit2(
					m_hDecoder,
					SpecificConfig.data(), static_cast<unsigned long>(SpecificConfig.size()),
					&SampleRate, &Channels) < 0)) {
			return false;
		}

		m_InitRequest = false;
		m_LastChannelConfig = Config.ChannelConfiguration;
	}

	// AudioMuxElement() から取り出した raw_data_block() を復号する
	return DecodeData(m_LATMParser.GetPayloadData(), m_LATMParser.GetPayloadSize(), Info);
}


bool AACDecoder_FAAD2::DecodeData(const uint8_t *pData, size_t Size, ReturnArg<DecodeFrameInfo> Info)
{
	// デコード
	NeAACDecFrameInfo FrameInfo;

	const uint8_t *pPCMBuffer =
		static_cast<uint8_t *>(
			::NeAACDecDecode(
				m_hDecoder, &FrameInfo,
				const_cast<unsigned char *>(pData),
				static_cast<unsigned long>(Size)));

	bool OK = false;

	if (FrameInfo.error == 0) {
		int Channels = FrameInfo.channels;

		// ダウンミックスの係数は DSE で送られる (STD-B32 第 2 部 6.2.3 (3)、送られるまでは既定値)
		const NeAACDecStruct *pDec = static_cast<const NeAACDecStruct *>(m_hDecoder);
		if (pDec->dse_bytes > 0)
			ParseAACAncillaryData(pDec->dse_data, pDec->dse_bytes, &m_DownmixCoefficients);

		// 22.2ch は 5.1ch にして出力する
		if ((m_LastChannelConfig == 13) && (Channels == AAC_22_2CH_CHANNEL_COUNT) && (FrameInfo.samples > 0)) {
			const size_t SampleCount = FrameInfo.samples / AAC_22_2CH_CHANNEL_COUNT;
			m_DownmixBuffer.resize(SampleCount * 6);
			DownmixAAC22_2chTo5_1ch(
				reinterpret_cast<const int16_t *>(pPCMBuffer), m_DownmixBuffer.data(), SampleCount,
				m_DownmixCoefficients);
			pPCMBuffer = reinterpret_cast<const uint8_t *>(m_DownmixBuffer.data());
			Channels = 6;
		}

		m_AudioInfo.Frequency = FrameInfo.samplerate;
		m_AudioInfo.ChannelCount = Channels;
		// FAADではモノラルが2chにデコードされる
		if ((FrameInfo.channels == 2) && (m_LastChannelConfig == 1))
			m_AudioInfo.OriginalChannelCount = 1;
		else
			m_AudioInfo.OriginalChannelCount = FrameInfo.channels;
		m_AudioInfo.DualMono = (FrameInfo.channels == 2) && (m_LastChannelConfig == 0);
		if (FrameInfo.samples > 0) {
			Info->pData = pPCMBuffer;
			Info->SampleCount = FrameInfo.samples / FrameInfo.channels;
			Info->Info = m_AudioInfo;
			Info->Discontinuity = m_DecodeError;
			OK = true;
		}
	} else {
		// エラー発生
		LIBISDB_TRACE(
			"NeAACDecDecode() error \"{}\"\n",
			NeAACDecGetErrorMessage(FrameInfo.error));

		// リセットする
		ResetDecoder();
	}

	return OK;
}


} // namespace LibISDB::DirectShow
