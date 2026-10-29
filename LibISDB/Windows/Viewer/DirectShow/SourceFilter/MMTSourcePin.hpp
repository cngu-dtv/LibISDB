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
 @file   MMTSourcePin.hpp
 @brief  MMT ソースピン
 @author cngu-dtv
*/


#ifndef LIBISDB_MMT_SOURCE_PIN_H
#define LIBISDB_MMT_SOURCE_PIN_H


#include "../DirectShowBase.hpp"
#include "../../../../Base/StreamingThread.hpp"
#include "../../../../Utilities/Lock.hpp"
#include <deque>
#include <vector>
#include <atomic>


namespace LibISDB::DirectShow
{

	class MMTSourceFilter;

	/** MMT ソースピンクラス (1 サンプル = 1 アクセスユニット) */
	class MMTSourcePin
		: public CBaseOutputPin
		, protected StreamingThread
	{
	public:
		struct SampleInfo {
			bool HasTime = false;
			REFERENCE_TIME Time = 0;
			bool SyncPoint = false;
			bool Discontinuity = false;
		};

		MMTSourcePin(
			HRESULT *phr, MMTSourceFilter *pFilter, LPCWSTR pName,
			size_t MaxQueueCount, long SampleBufferSize, long SampleBufferCount);
		~MMTSourcePin();

	// IQualityControl
		STDMETHODIMP Notify(IBaseFilter *pSender, Quality q) override;

	// CBasePin
		HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) override;
		HRESULT CheckMediaType(const CMediaType *pMediaType) override;

		HRESULT Active() override;
		HRESULT Inactive() override;

	// CBaseOutputPin
		HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest) override;

	// MMTSourcePin
		void SetPinMediaType(const CMediaType &MediaType);
		bool IsActive() const noexcept { return m_IsActive.load(std::memory_order_acquire); }
		// キューが一杯の場合は false を返す
		bool InputSample(const uint8_t *pData, size_t Size, const SampleInfo &Info);
		bool IsQueueFull();
		void ClearQueue();
		void Flush();
		int GetFillPercentage();

	protected:
		// 再利用するバッファの上限 (大きなアクセスユニットのバッファを溜め込まないようにする)
		static constexpr size_t MAX_FREE_BUFFERS = 16;

		struct QueuedSample {
			std::vector<uint8_t> Data;
			SampleInfo Info;
		};

	// Thread
		const CharType * GetThreadName() const noexcept override { return LIBISDB_STR("MMTSourcePin"); }

	// StreamingThread
		void StreamingLoop() override;
		bool ProcessStream() override;

		MMTSourceFilter *m_pFilter;
		CMediaType m_MediaType;
		const size_t m_MaxQueueCount;
		const long m_SampleBufferSize;
		const long m_SampleBufferCount;

		MutexLock m_QueueLock;
		std::deque<QueuedSample> m_Queue;
		std::vector<std::vector<uint8_t>> m_FreeBufferList;
		std::atomic<bool> m_IsActive;
		std::atomic<bool> m_NewSegment;
	};

} // namespace LibISDB::DirectShow


#endif // ifndef LIBISDB_MMT_SOURCE_PIN_H
