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
 @file   AACDecoder.hpp
 @brief  AAC デコーダ
 @author DBCTRADO
*/


#ifndef LIBISDB_AAC_DECODER_H
#define LIBISDB_AAC_DECODER_H


#include "AudioDecoder.hpp"
#include "../../../../MediaParsers/ADTSParser.hpp"
#include "../../../../MediaParsers/LATMParser.hpp"


namespace LibISDB::DirectShow
{

	/** AAC デコーダ基底クラス */
	class AACDecoder
		: public AudioDecoder
	{
	public:
		enum class TransportType {
			ADTS, /**< ADTS のバイトストリーム */
			LATM, /**< 1 回の入力が AudioMuxElement(1) 1 つ (MMT の MFU) */
		};

		AACDecoder(TransportType Transport = TransportType::ADTS) noexcept;
		~AACDecoder();

	// AudioDecoder
		bool Open() override;
		void Close() override;
		bool Reset() override;
		bool Decode(const uint8_t *pData, size_t *pDataSize, ReturnArg<DecodeFrameInfo> Info) override;

		// S/PDIF には ADTS のフレームを出力するため、LATM の場合は非対応
		bool IsSPDIFSupported() const override { return m_Transport == TransportType::ADTS; }
		bool GetSPDIFFrameInfo(ReturnArg<SPDIFFrameInfo> Info) const override;
		int GetSPDIFBurstPayload(uint8_t *pBuffer, size_t BufferSize) const override;

	// AACDecoder
		TransportType GetTransportType() const noexcept { return m_Transport; }

	protected:
		virtual bool OpenDecoder() = 0;
		virtual void CloseDecoder() = 0;
		bool ResetDecoder();
		virtual bool DecodeFrame(const ADTSFrame *pFrame, ReturnArg<DecodeFrameInfo> Info) = 0;
		// m_LATMParser で解析済みの AudioMuxElement(1) を復号する
		virtual bool DecodeLATMFrame(const uint8_t *pData, size_t Size, ReturnArg<DecodeFrameInfo> Info) = 0;

		const TransportType m_Transport;
		ADTSParser m_ADTSParser;
		const ADTSFrame *m_pADTSFrame;
		LATMParser m_LATMParser;
		bool m_DecodeError;
	};

} // namespace LibISDB::DirectShow


#endif // ifndef LIBISDB_AAC_DECODER_H
