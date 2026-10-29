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
 @file   ViewerEngine.cpp
 @brief  ビューアエンジン
 @author DBCTRADO
*/


#include "../../LibISDBPrivate.hpp"
#include "../../LibISDBWindows.hpp"
#include "ViewerEngine.hpp"
#include "../../Base/DebugDef.hpp"


namespace LibISDB
{


ViewerEngine::ViewerEngine() noexcept
	: m_pViewer(nullptr)
	, m_PlayRadio(true)
{
}


bool ViewerEngine::BuildViewer(const ViewerFilter::OpenSettings &Settings)
{
	const ViewerFilter::OpenSettings OpenSettings = GetOpenSettings(Settings);
	const FilterGraph::IDType FilterID = m_FilterGraph.GetFilterID(m_pViewer);
	if (FilterID == 0) {
		return false;
	}

	m_EngineLock.Lock();
	m_FilterGraph.DisconnectFilter(FilterID, FilterGraph::ConnectDirection::Upstream);
	m_EngineLock.Unlock();

	const bool OK = m_pViewer->OpenViewer(OpenSettings);
	if (!OK)
		SetError(m_pViewer->GetLastErrorDescription());

	m_EngineLock.Lock();
	m_FilterGraph.ConnectFilter(FilterID, FilterGraph::ConnectDirection::Upstream);
	m_EngineLock.Unlock();

	UpdateVideoAndAudioPID();

	return OK;
}


bool ViewerEngine::RebuildViewer(const ViewerFilter::OpenSettings &Settings)
{
	const ViewerFilter::OpenSettings OpenSettings = GetOpenSettings(Settings);
	const FilterGraph::IDType FilterID = m_FilterGraph.GetFilterID(m_pViewer);
	if (FilterID == 0) {
		return false;
	}

	EnableViewer(false);

	m_EngineLock.Lock();
	m_FilterGraph.DisconnectFilter(FilterID, FilterGraph::ConnectDirection::Upstream);
	m_EngineLock.Unlock();

	m_pViewer->CloseViewer();
	const bool OK = m_pViewer->OpenViewer(OpenSettings);
	if (!OK)
		SetError(m_pViewer->GetLastErrorDescription());

	m_EngineLock.Lock();
	m_FilterGraph.ConnectFilter(FilterID, FilterGraph::ConnectDirection::Upstream);
	m_EngineLock.Unlock();

	UpdateVideoAndAudioPID();

	return OK;
}


bool ViewerEngine::CloseViewer()
{
	if (m_pViewer != nullptr)
		m_pViewer->CloseViewer();
	return true;
}


bool ViewerEngine::IsViewerOpen() const
{
	return (m_pViewer != nullptr) && m_pViewer->IsOpen();
}


bool ViewerEngine::ResetViewer()
{
	if (!IsViewerOpen())
		return false;

	m_pViewer->Reset();

	UpdateVideoAndAudioPID();

	return true;
}


bool ViewerEngine::EnableViewer(bool Enable)
{
	if ((m_pViewer == nullptr) || !m_pViewer->IsOpen())
		return false;

	bool OK;

	if (Enable) {
		// ビューア有効
		OK = m_pViewer->Play();
	} else {
		// ビューア無効
		OK = m_pViewer->Stop();
	}

	return OK;
}


bool ViewerEngine::SetStreamTypePlayable(uint8_t StreamType, bool Playable)
{
	if (StreamType >= m_PlayableStreamType.size())
		return false;
	m_PlayableStreamType[StreamType] = Playable;
	return true;
}


bool ViewerEngine::IsStreamTypePlayable(uint8_t StreamType) const
{
	if (StreamType >= m_PlayableStreamType.size())
		return false;
	return m_PlayableStreamType[StreamType];
}


bool ViewerEngine::IsSelectableService(int Index) const
{
	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return false;

	if ((Index < 0) || (Index >= pSource->GetServiceCount()))
		return false;

	if (pSource->GetVideoStreamCount(Index) > 0) {
		if (IsStreamTypePlayable(pSource->GetVideoStreamType(Index, 0)))
			return true;
	} else if (m_PlayRadio) {
		if (IsStreamTypePlayable(pSource->GetAudioStreamType(Index, 0)))
			return true;
	}

	return false;
}


int ViewerEngine::GetSelectableServiceCount() const
{
	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return 0;

	const int ServiceCount = pSource->GetServiceCount();
	int Count = 0;

	for (int i = 0; i < ServiceCount; i++) {
		if (IsSelectableService(i))
			Count++;
	}

	return Count;
}


uint16_t ViewerEngine::GetSelectableServiceID(int Index) const
{
	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return SERVICE_ID_INVALID;

	const int ServiceCount = pSource->GetServiceCount();
	int Count = 0;

	for (int i = 0; i < ServiceCount; i++) {
		if (IsSelectableService(i)) {
			if (Count == Index) {
				return pSource->GetServiceID(i);
			}
			Count++;
		}
	}

	return SERVICE_ID_INVALID;
}


uint16_t ViewerEngine::GetDefaultServiceID() const
{
	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return SERVICE_ID_INVALID;

	if (pSource->Is1SegStream())
		return pSource->GetServiceID(-1);

	const int ServiceCount = pSource->GetServiceCount();

	for (int i = 0; i < ServiceCount; i++) {
		if (!pSource->IsServiceInfoAcquired(i))
			return SERVICE_ID_INVALID;
		if (IsSelectableService(i)) {
			return pSource->GetServiceID(i);
		}
	}

	return SERVICE_ID_INVALID;
}


int ViewerEngine::GetSelectableServiceIndexByID(uint16_t ServiceID) const
{
	const ServiceSource *pSource = GetServiceSource();
	if (pSource == nullptr)
		return -1;

	const int ServiceCount = pSource->GetServiceCount();
	int Count = 0;

	for (int i = 0; i < ServiceCount; i++) {
		if (IsSelectableService(i)) {
			if (pSource->GetServiceID(i) == ServiceID)
				return Count;
			Count++;
		}
	}

	return -1;
}


bool ViewerEngine::GetSelectableServiceList(AnalyzerFilter::ServiceList *pList) const
{
	if (pList == nullptr)
		return false;

	pList->clear();

	if ((m_pAnalyzer == nullptr) || IsMMTStream())
		return false;

	const int ServiceCount = m_pAnalyzer->GetServiceCount();

	for (int i = 0; i < ServiceCount; i++) {
		if (IsSelectableService(i))
			m_pAnalyzer->GetServiceInfo(i, &pList->emplace_back());
	}

	return true;
}


bool ViewerEngine::GetSelectableMMTServiceList(MMTAnalyzerFilter::ServiceList *pList) const
{
	if (pList == nullptr)
		return false;

	pList->clear();

	if (!IsMMTStream())
		return false;

	const int ServiceCount = m_pMMTAnalyzer->GetServiceCount();

	for (int i = 0; i < ServiceCount; i++) {
		if (IsSelectableService(i))
			m_pMMTAnalyzer->GetServiceInfo(i, &pList->emplace_back());
	}

	return true;
}


void ViewerEngine::OnFilterRegistered(FilterBase *pFilter, FilterGraph::IDType ID)
{
	TSEngine::OnFilterRegistered(pFilter, ID);

	ViewerFilter *pViewer = dynamic_cast<ViewerFilter *>(pFilter);
	if (pViewer != nullptr)
		m_pViewer = pViewer;
}


void ViewerEngine::OnServiceChanged(uint16_t ServiceID)
{
	const ServiceSource *pSource = GetServiceSource();

	if ((m_pViewer != nullptr) && (pSource != nullptr)) {
		m_pViewer->Set1SegMode(pSource->Is1SegService(pSource->GetServiceIndexByID(ServiceID)));
	}
}


void ViewerEngine::OnVideoStreamTypeChanged(uint8_t StreamType)
{
}


void ViewerEngine::OnAudioStreamTypeChanged(uint8_t StreamType)
{
	if (m_pViewer != nullptr)
		m_pViewer->SetAudioStreamType(StreamType);
}


bool ViewerEngine::IsAudioPlayable(uint8_t StreamType, const EventInfo::AudioInfo *pInfo) const
{
	if (!IsStreamTypePlayable(StreamType))
		return false;

	if ((pInfo != nullptr) && (m_pViewer != nullptr))
		return m_pViewer->IsMMTAudioPlayable(pInfo->StreamContent, pInfo->ComponentType);

	return true;
}


bool ViewerEngine::IsStereoAudioPreferred() const
{
	return (m_pViewer != nullptr) && m_pViewer->GetDownMixSurround();
}


void ViewerEngine::UpdateVideoAndAudioPID()
{
	BlockLock Lock(m_EngineLock);

	const ServiceSource *pSource = GetServiceSource();
	const int ServiceIndex = GetServiceIndex();

	if ((pSource != nullptr) && (ServiceIndex >= 0)) {
		const uint16_t InvalidID = pSource->GetInvalidStreamID();
		const uint16_t VideoID = pSource->GetVideoStreamID(ServiceIndex, m_CurVideoStream);
		if (VideoID != InvalidID) {
			if (pSource->IsMMT())
				m_pViewer->SetActiveVideoPacketID(VideoID, false);
			else
				m_pViewer->SetActiveVideoPID(VideoID, false);
		}
		const uint16_t AudioID = pSource->GetAudioStreamID(ServiceIndex, m_CurAudioStream);
		if (AudioID != InvalidID) {
			if (pSource->IsMMT())
				m_pViewer->SetActiveAudioPacketID(AudioID, false);
			else
				m_pViewer->SetActiveAudioPID(AudioID, false);
		}
	}
}


ViewerFilter::OpenSettings ViewerEngine::GetOpenSettings(const ViewerFilter::OpenSettings &Settings) const
{
	ViewerFilter::OpenSettings NewSettings = Settings;

	NewSettings.Format = IsMMTStream() ? StreamFormat::TLV : StreamFormat::TS;

	return NewSettings;
}


} // namespace LibISDB
