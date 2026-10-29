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
 @file   MMTSourcePin.cpp
 @brief  MMT ソースピン
 @author cngu-dtv
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "MMTSourcePin.hpp"
#include "MMTSourceFilter.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


MMTSourcePin::MMTSourcePin(
	HRESULT *phr, MMTSourceFilter *pFilter, LPCWSTR pName,
	size_t MaxQueueCount, long SampleBufferSize, long SampleBufferCount)
	: CBaseOutputPin(TEXT("MMTSourcePin"), pFilter, pFilter->m_pLock, phr, pName)
	, m_pFilter(pFilter)
	, m_MaxQueueCount(MaxQueueCount)
	, m_SampleBufferSize(SampleBufferSize)
	, m_SampleBufferCount(SampleBufferCount)
	, m_IsActive(false)
	, m_NewSegment(true)
{
	LIBISDB_TRACE(LIBISDB_STR("MMTSourcePin::MMTSourcePin() {}\n"), static_cast<void *>(this));

	m_MediaType.InitMediaType();

	*phr = S_OK;
}


MMTSourcePin::~MMTSourcePin()
{
	LIBISDB_TRACE(LIBISDB_STR("MMTSourcePin::~MMTSourcePin()\n"));

	StopStreamingThread();
}


STDMETHODIMP MMTSourcePin::Notify(IBaseFilter *pSender, Quality q)
{
	// 品質制御は行わない (遅れたサンプルはレンダラに破棄させる)
	// CBasePin の既定の実装は Debug ビルドでアサートするので置き換える
	return E_NOTIMPL;
}


HRESULT MMTSourcePin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);

	if (iPosition < 0)
		return E_INVALIDARG;
	if ((iPosition > 0) || (*m_MediaType.Type() == GUID_NULL))
		return VFW_S_NO_MORE_ITEMS;

	*pMediaType = m_MediaType;

	return S_OK;
}


HRESULT MMTSourcePin::CheckMediaType(const CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);

	return (*pMediaType == m_MediaType) ? S_OK : E_FAIL;
}


HRESULT MMTSourcePin::Active()
{
	LIBISDB_TRACE(LIBISDB_STR("MMTSourcePin::Active()\n"));

	const HRESULT hr = CBaseOutputPin::Active();
	if (FAILED(hr))
		return hr;

	if (IsStarted())
		return E_UNEXPECTED;

	ClearQueue();
	m_NewSegment.store(true, std::memory_order_release);

	if (!StartStreamingThread())
		return E_FAIL;

	m_IsActive.store(true, std::memory_order_release);

	return S_OK;
}


HRESULT MMTSourcePin::Inactive()
{
	LIBISDB_TRACE(LIBISDB_STR("MMTSourcePin::Inactive()\n"));

	m_IsActive.store(false, std::memory_order_release);

	const HRESULT hr = CBaseOutputPin::Inactive();

	StopStreamingThread();
	ClearQueue();

	return hr;
}


HRESULT MMTSourcePin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest)
{
	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(pRequest, E_POINTER);

	if (pRequest->cBuffers < m_SampleBufferCount)
		pRequest->cBuffers = m_SampleBufferCount;

	if (pRequest->cbBuffer < m_SampleBufferSize)
		pRequest->cbBuffer = m_SampleBufferSize;

	ALLOCATOR_PROPERTIES Actual;
	const HRESULT hr = pAlloc->SetProperties(pRequest, &Actual);
	if (FAILED(hr))
		return hr;

	if ((Actual.cBuffers < pRequest->cBuffers)
			|| (Actual.cbBuffer < pRequest->cbBuffer))
		return E_FAIL;

	return S_OK;
}


void MMTSourcePin::SetPinMediaType(const CMediaType &MediaType)
{
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);

	m_MediaType = MediaType;
}


bool MMTSourcePin::InputSample(const uint8_t *pData, size_t Size, const SampleInfo &Info)
{
	{
		BlockLock Lock(m_QueueLock);

		if (m_Queue.size() >= m_MaxQueueCount)
			return false;

		QueuedSample &Sample = m_Queue.emplace_back();
		if (!m_FreeBufferList.empty()) {
			Sample.Data = std::move(m_FreeBufferList.back());
			m_FreeBufferList.pop_back();
		}
		Sample.Data.assign(pData, pData + Size);
		Sample.Info = Info;
	}

	m_StreamingThreadCondition.NotifyOne();

	return true;
}


bool MMTSourcePin::IsQueueFull()
{
	BlockLock Lock(m_QueueLock);

	return m_Queue.size() >= m_MaxQueueCount;
}


void MMTSourcePin::ClearQueue()
{
	BlockLock Lock(m_QueueLock);

	while (!m_Queue.empty()) {
		if (m_FreeBufferList.size() < MAX_FREE_BUFFERS)
			m_FreeBufferList.emplace_back(std::move(m_Queue.front().Data));
		m_Queue.pop_front();
	}
}


void MMTSourcePin::Flush()
{
	LIBISDB_TRACE(LIBISDB_STR("MMTSourcePin::Flush()\n"));

	ClearQueue();
	m_NewSegment.store(true, std::memory_order_release);

	if (IsConnected()) {
		DeliverBeginFlush();
		DeliverEndFlush();
	}
}


int MMTSourcePin::GetFillPercentage()
{
	BlockLock Lock(m_QueueLock);

	return static_cast<int>(m_Queue.size() * 100 / m_MaxQueueCount);
}


void MMTSourcePin::StreamingLoop()
{
	::CoInitialize(nullptr);

	m_NewSegment.store(true, std::memory_order_release);

	StreamingThread::StreamingLoop();

	DeliverEndOfStream();

	::CoUninitialize();
}


bool MMTSourcePin::ProcessStream()
{
	m_pFilter->UpdateStreamClock();

	QueuedSample Sample;

	{
		BlockLock Lock(m_QueueLock);

		if (m_Queue.empty())
			return false;

		Sample = std::move(m_Queue.front());
		m_Queue.pop_front();
	}

	if (m_NewSegment.exchange(false, std::memory_order_acq_rel)) {
		DeliverNewSegment(0, std::numeric_limits<REFERENCE_TIME>::max(), 1.0);
		Sample.Info.Discontinuity = true;
	}

	// アロケータのバッファに収まらない場合は分割する (先頭のサンプルにのみ時刻を設定する)
	const size_t DataSize = Sample.Data.size();
	size_t Pos = 0;

	while (Pos < DataSize) {
		IMediaSample *pMediaSample = nullptr;
		HRESULT hr = GetDeliveryBuffer(&pMediaSample, nullptr, nullptr, 0);
		if (FAILED(hr))
			break;

		BYTE *pBuffer = nullptr;
		hr = pMediaSample->GetPointer(&pBuffer);
		if (SUCCEEDED(hr)) {
			const size_t Size = std::min(DataSize - Pos, static_cast<size_t>(pMediaSample->GetSize()));

			std::memcpy(pBuffer, Sample.Data.data() + Pos, Size);
			pMediaSample->SetActualDataLength(static_cast<long>(Size));

			if (Pos == 0) {
				REFERENCE_TIME Time = Sample.Info.Time;
				pMediaSample->SetTime(Sample.Info.HasTime ? &Time : nullptr, nullptr);
				pMediaSample->SetSyncPoint(Sample.Info.SyncPoint);
				pMediaSample->SetDiscontinuity(Sample.Info.Discontinuity);
			} else {
				pMediaSample->SetTime(nullptr, nullptr);
				pMediaSample->SetSyncPoint(FALSE);
				pMediaSample->SetDiscontinuity(FALSE);
			}

			hr = Deliver(pMediaSample);
			Pos += Size;
		}

		pMediaSample->Release();

		if (FAILED(hr))
			break;
	}

	{
		BlockLock Lock(m_QueueLock);

		if (m_FreeBufferList.size() < MAX_FREE_BUFFERS) {
			Sample.Data.clear();
			m_FreeBufferList.emplace_back(std::move(Sample.Data));
		}
	}

	return true;
}


} // namespace LibISDB::DirectShow
