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
 @file   MMTSourceFilter.hpp
 @brief  MMT ソースフィルタ
 @author cngu-dtv
*/


#ifndef LIBISDB_MMT_SOURCE_FILTER_H
#define LIBISDB_MMT_SOURCE_FILTER_H


#include "../DirectShowBase.hpp"
#include "../../../../MMT/MPUAssembler.hpp"
#include "../../../../Utilities/Lock.hpp"


namespace LibISDB::DirectShow
{

	class MMTSourcePin;

	/**
	 MMT ソースフィルタクラス

	 映像 (HEVC の Annex B バイトストリーム) と音声 (AudioMuxElement(1)) のアクセスユニットを
	 それぞれのピンからサンプルとして出力する。
	 サンプルの時刻は、TLV ストリームの NTP の時刻をストリーム時間に対応付けて PTS から求める。
	*/
	class __declspec(uuid("5C2E8F3A-7D41-4B6E-9A0C-2F8B1D6E4A73")) MMTSourceFilter
		: public CBaseFilter
	{
		friend MMTSourcePin;

	public:
		enum class PinType {
			Video,
			Audio,
		};

		/** NTP の時刻と提示時刻の間に置く既定の遅延 (100ns) */
		static constexpr REFERENCE_TIME DEFAULT_PRESENTATION_DELAY = 5000000;
		/** 遅延の上限 (100ns) */
		static constexpr REFERENCE_TIME MAX_PRESENTATION_DELAY = 100000000;
		/** ストリーム時間が NTP の時刻に対応する時間より遅れてもよい上限 (100ns、超えたら合わせ直す) */
		static constexpr REFERENCE_TIME MAX_CLOCK_LEAD = 10000000;

		static IBaseFilter * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

		DECLARE_IUNKNOWN

	// CBaseFilter
		STDMETHODIMP Stop() override;
		STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State) override;
		int GetPinCount() override;
		CBasePin *GetPin(int n) override;

	// MMTSourceFilter
		void SetPinMediaType(PinType Type, const CMediaType &MediaType);
		/**
		 アクセスユニットを入力する

		 NTPTime は入力時点の NTP の時刻 (90kHz、Info の DTS / PTS と同じ基準)。
		 映像は時刻の分かるランダムアクセスポイントから出力する。
		*/
		bool InputAccessUnit(PinType Type, const MPUAssembler::AccessUnitInfo &Info, int64_t NTPTime);
		// NTP の時刻がうるう秒の調整で Seconds 秒ずれた (以降の NTP の時刻と PTS は同じだけずれる)
		void OnLeapSecond(int Seconds);
		void Reset();
		void Flush();
		void SetOutputWhenPaused(bool Output);
		bool SetPresentationDelay(REFERENCE_TIME Delay);
		REFERENCE_TIME GetPresentationDelay() const noexcept { return m_PresentationDelay; }
		bool SetInputWait(DWORD Wait);
		int GetBufferFillPercentage() const;

	protected:
		MMTSourceFilter(LPUNKNOWN pUnk, HRESULT *phr);
		virtual ~MMTSourceFilter();

		MMTSourcePin * GetPinByType(PinType Type) const noexcept;
		bool CanOutput();
		/*
			グラフのクロックは送出スレッドからのみ読む
			(音声レンダラのクロックは初期化中などに長く待たされることがあり、入力側のスレッドを止めないため)
		*/
		void UpdateStreamClock();
		// UpdateStreamClock() で読んだ時刻から現在のストリーム時間を推定する
		bool GetStreamTimeNow(REFERENCE_TIME *pTime);
		void ResetSync();

		static constexpr REFERENCE_TIME ToReferenceTime(int64_t Time90kHz) noexcept
		{
			return Time90kHz * 1000 / 9;
		}

		MMTSourcePin *m_pVideoPin;
		MMTSourcePin *m_pAudioPin;
		CCritSec m_cStateLock;
		bool m_OutputWhenPaused;
		DWORD m_InputWait;

		MutexLock m_SyncLock;
		bool m_IsSynced;
		REFERENCE_TIME m_TimeOffset;
		REFERENCE_TIME m_PresentationDelay;
		bool m_VideoWaitRAP;
		bool m_VideoDiscontinuity;
		bool m_AudioDiscontinuity;

		MutexLock m_ClockLock;
		bool m_HasStreamClock;
		REFERENCE_TIME m_StreamClockTime;
		LONGLONG m_StreamClockCounter;
		LONGLONG m_CounterFrequency;
	};

} // namespace LibISDB::DirectShow


#endif // ifndef LIBISDB_MMT_SOURCE_FILTER_H
