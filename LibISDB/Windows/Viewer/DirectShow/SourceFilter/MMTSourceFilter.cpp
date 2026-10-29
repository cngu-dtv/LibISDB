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
 @file   MMTSourceFilter.cpp
 @brief  MMT ソースフィルタ
 @author cngu-dtv
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "MMTSourceFilter.hpp"
#include "MMTSourcePin.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


namespace
{

// 映像は 60 フレーム/秒で約 10 秒分、音声は 48kHz で約 10 秒分
constexpr size_t VIDEO_MAX_QUEUE_COUNT = 600;
constexpr size_t AUDIO_MAX_QUEUE_COUNT = 480;
constexpr long VIDEO_SAMPLE_BUFFER_SIZE = 1024 * 1024;
constexpr long VIDEO_SAMPLE_BUFFER_COUNT = 4;
constexpr long AUDIO_SAMPLE_BUFFER_SIZE = 8 * 1024;
constexpr long AUDIO_SAMPLE_BUFFER_COUNT = 16;

}




MMTSourceFilter::MMTSourceFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CBaseFilter(TEXT("MMT Source Filter"), pUnk, &m_cStateLock, __uuidof(MMTSourceFilter))
	, m_pVideoPin(nullptr)
	, m_pAudioPin(nullptr)
	, m_OutputWhenPaused(false)
	, m_InputWait(0)
	, m_PresentationDelay(DEFAULT_PRESENTATION_DELAY)
	, m_HasStreamClock(false)
	, m_StreamClockTime(0)
	, m_StreamClockCounter(0)
{
	LIBISDB_TRACE(LIBISDB_STR("MMTSourceFilter::MMTSourceFilter {}\n"), static_cast<void *>(this));

	LARGE_INTEGER Frequency;
	::QueryPerformanceFrequency(&Frequency);
	m_CounterFrequency = Frequency.QuadPart;

	ResetSync();

	m_pVideoPin = new MMTSourcePin(
		phr, this, L"Video", VIDEO_MAX_QUEUE_COUNT, VIDEO_SAMPLE_BUFFER_SIZE, VIDEO_SAMPLE_BUFFER_COUNT);
	if (SUCCEEDED(*phr)) {
		m_pAudioPin = new MMTSourcePin(
			phr, this, L"Audio", AUDIO_MAX_QUEUE_COUNT, AUDIO_SAMPLE_BUFFER_SIZE, AUDIO_SAMPLE_BUFFER_COUNT);
	}
}


MMTSourceFilter::~MMTSourceFilter()
{
	delete m_pVideoPin;
	delete m_pAudioPin;
}


IBaseFilter * WINAPI MMTSourceFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	MMTSourceFilter *pNewFilter = new MMTSourceFilter(pUnk, phr);
	if (FAILED(*phr)) {
		delete pNewFilter;
		return nullptr;
	}

	IBaseFilter *pFilter;
	*phr = pNewFilter->QueryInterface(IID_PPV_ARGS(&pFilter));
	if (FAILED(*phr)) {
		delete pNewFilter;
		return nullptr;
	}

	return pFilter;
}


STDMETHODIMP MMTSourceFilter::Stop()
{
	LIBISDB_TRACE(LIBISDB_STR("MMTSourceFilter::Stop()\n"));

	const HRESULT hr = CBaseFilter::Stop();

	{
		BlockLock Lock(m_SyncLock);
		ResetSync();
	}

	BlockLock Lock(m_ClockLock);
	m_HasStreamClock = false;

	return hr;
}


STDMETHODIMP MMTSourceFilter::GetState(DWORD dw, FILTER_STATE *pState)
{
	CheckPointer(pState, E_POINTER);

	*pState = m_State;
	if ((m_State == State_Paused) && !m_OutputWhenPaused)
		return VFW_S_CANT_CUE;
	return S_OK;
}


int MMTSourceFilter::GetPinCount()
{
	return 2;
}


CBasePin * MMTSourceFilter::GetPin(int n)
{
	switch (n) {
	case 0: return m_pVideoPin;
	case 1: return m_pAudioPin;
	}

	return nullptr;
}


void MMTSourceFilter::SetPinMediaType(PinType Type, const CMediaType &MediaType)
{
	GetPinByType(Type)->SetPinMediaType(MediaType);
}


bool MMTSourceFilter::InputAccessUnit(PinType Type, const MPUAssembler::AccessUnitInfo &Info, int64_t NTPTime)
{
	if (!CanOutput())
		return false;

	MMTSourcePin *pPin = GetPinByType(Type);
	if (!pPin->IsActive())
		return false;

	MMTSourcePin::SampleInfo Sample;

	{
		BlockLock Lock(m_SyncLock);

		REFERENCE_TIME Now;
		const bool Running = GetStreamTimeNow(&Now);
		const REFERENCE_TIME NTP = ToReferenceTime(NTPTime);

		if (m_IsSynced && Running) {
			/*
				NTP の時刻に対応するストリーム時間と実際のストリーム時間のずれが大きくなったら合わせ直す
				(グラフのクロックと送出側のクロックの差の蓄積や、NTP の時刻の飛び)
			*/
			const REFERENCE_TIME Deviation = Now - (NTP + m_TimeOffset - m_PresentationDelay);
			if ((Deviation > m_PresentationDelay) || (Deviation < -MAX_CLOCK_LEAD)) {
				LIBISDB_TRACE(
					LIBISDB_STR("MMTSourceFilter : Resync (deviation {} ms)\n"),
					Deviation / 10000);
				ResetSync();
				m_pVideoPin->ClearQueue();
				m_pAudioPin->ClearQueue();
			}
		}

		if (!m_IsSynced) {
			m_TimeOffset = Now + m_PresentationDelay - NTP;
			m_IsSynced = true;
		}

		if (Type == PinType::Video) {
			if (Info.Discontinuity)
				m_VideoWaitRAP = true;
			if (m_VideoWaitRAP) {
				if (!Info.RandomAccess || !Info.HasTimestamp)
					return true;
				m_VideoWaitRAP = false;
				m_VideoDiscontinuity = true;
			}
			Sample.SyncPoint = Info.RandomAccess;
			Sample.Discontinuity = m_VideoDiscontinuity;
			m_VideoDiscontinuity = false;
		} else {
			if (Info.Discontinuity)
				m_AudioDiscontinuity = true;
			// 欠落の直後は時刻の分かるフレームから出力する
			if (m_AudioDiscontinuity && !Info.HasTimestamp)
				return true;
			Sample.SyncPoint = true;
			Sample.Discontinuity = m_AudioDiscontinuity;
			m_AudioDiscontinuity = false;
		}

		if (Info.HasTimestamp) {
			Sample.HasTime = true;
			Sample.Time = ToReferenceTime(Info.PTS) + m_TimeOffset;
		}
	}

	if (pPin->InputSample(Info.pData, Info.DataSize, Sample))
		return true;

	// キューが一杯
	const DWORD Wait = m_InputWait;
	if (Wait != 0) {
		const DWORD BeginTime = ::GetTickCount();
		do {
			::Sleep(10);
			if (!pPin->IsQueueFull())
				break;
		} while (static_cast<DWORD>(::GetTickCount() - BeginTime) < Wait);

		if (pPin->InputSample(Info.pData, Info.DataSize, Sample))
			return true;
	}

	LIBISDB_TRACE(LIBISDB_STR("MMTSourceFilter : {} queue full\n"), (Type == PinType::Video) ? LIBISDB_STR("Video") : LIBISDB_STR("Audio"));

	BlockLock Lock(m_SyncLock);

	if (Type == PinType::Video) {
		pPin->ClearQueue();
		m_VideoWaitRAP = true;
	} else {
		m_AudioDiscontinuity = true;
	}

	return false;
}


void MMTSourceFilter::OnLeapSecond(int Seconds)
{
	BlockLock Lock(m_SyncLock);

	if (m_IsSynced)
		m_TimeOffset -= static_cast<REFERENCE_TIME>(Seconds) * 10000000;
}


void MMTSourceFilter::Reset()
{
	m_pVideoPin->ClearQueue();
	m_pAudioPin->ClearQueue();

	BlockLock Lock(m_SyncLock);
	ResetSync();
}


void MMTSourceFilter::Flush()
{
	CAutoLock Lock(m_pLock);

	{
		BlockLock SyncLock(m_SyncLock);
		ResetSync();
	}

	m_pVideoPin->Flush();
	m_pAudioPin->Flush();
}


void MMTSourceFilter::SetOutputWhenPaused(bool Output)
{
	m_OutputWhenPaused = Output;
}


bool MMTSourceFilter::SetPresentationDelay(REFERENCE_TIME Delay)
{
	if ((Delay < 0) || (Delay > MAX_PRESENTATION_DELAY))
		return false;

	BlockLock Lock(m_SyncLock);

	if (m_PresentationDelay != Delay) {
		m_PresentationDelay = Delay;
		ResetSync();
	}

	return true;
}


bool MMTSourceFilter::SetInputWait(DWORD Wait)
{
	m_InputWait = Wait;
	return true;
}


int MMTSourceFilter::GetBufferFillPercentage() const
{
	if (m_pVideoPin->IsConnected())
		return m_pVideoPin->GetFillPercentage();
	return m_pAudioPin->GetFillPercentage();
}


MMTSourcePin * MMTSourceFilter::GetPinByType(PinType Type) const noexcept
{
	return (Type == PinType::Video) ? m_pVideoPin : m_pAudioPin;
}


bool MMTSourceFilter::CanOutput()
{
	CAutoLock Lock(&m_cStateLock);

	return (m_State == State_Running)
		|| ((m_State == State_Paused) && m_OutputWhenPaused);
}


void MMTSourceFilter::UpdateStreamClock()
{
	/*
		ロックは取らずに状態を見る
		(停止時はフィルタのロックを持ったまま送出スレッドの終了を待つため)
	*/
	if (m_State != State_Running) {
		BlockLock Lock(m_ClockLock);
		m_HasStreamClock = false;
		return;
	}

	CRefTime Time;
	if (FAILED(StreamTime(Time)))
		return;

	LARGE_INTEGER Counter;
	::QueryPerformanceCounter(&Counter);

	BlockLock Lock(m_ClockLock);
	m_StreamClockTime = Time;
	m_StreamClockCounter = Counter.QuadPart;
	m_HasStreamClock = true;
}


bool MMTSourceFilter::GetStreamTimeNow(REFERENCE_TIME *pTime)
{
	LARGE_INTEGER Counter;
	::QueryPerformanceCounter(&Counter);

	BlockLock Lock(m_ClockLock);

	if (!m_HasStreamClock) {
		// 再生開始直後はストリーム時間 0 から始まるものとする
		*pTime = 0;
		return false;
	}

	*pTime = m_StreamClockTime
		+ static_cast<REFERENCE_TIME>((Counter.QuadPart - m_StreamClockCounter) * 10000000 / m_CounterFrequency);

	return true;
}


void MMTSourceFilter::ResetSync()
{
	m_IsSynced = false;
	m_TimeOffset = 0;
	m_VideoWaitRAP = true;
	m_VideoDiscontinuity = true;
	m_AudioDiscontinuity = true;
}


} // namespace LibISDB::DirectShow
