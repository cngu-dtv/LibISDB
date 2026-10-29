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
 @file   EPGDatabaseFilter.cpp
 @brief  番組情報フィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "EPGDatabaseFilter.hpp"
#include <algorithm>
#include "../TS/Tables.hpp"
#include "../MMT/MHEventInfo.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


EPGDatabaseFilter::EPGDatabaseFilter()
	: m_pEPGDatabase(nullptr)
	, m_SourceID()
{
	Reset();
}


void EPGDatabaseFilter::Reset()
{
	BlockLock Lock(m_FilterLock);

	m_PIDMapManager.UnmapAllTargets();

	// H-EIT
	m_PIDMapManager.MapTarget(PID_HEIT, PSITableBase::CreateWithHandler<EITPfScheduleTable>(&EPGDatabaseFilter::OnEITSection, this));

	// L-EIT
	m_PIDMapManager.MapTarget(PID_LEIT, PSITableBase::CreateWithHandler<EITPfScheduleTable>(&EPGDatabaseFilter::OnEITSection, this));

	// TOT
	m_PIDMapManager.MapTarget(PID_TOT, PSITableBase::CreateWithHandler<TOTTable>(&EPGDatabaseFilter::OnTOTSection, this));

	m_MMTPacketIDMapManager.UnmapAllTargets();

	// MH-EIT
	m_MMTPacketIDMapManager.MapTarget(
		MMT_PACKET_ID_MH_EIT,
		MMTSectionTable::CreateWithHandler<MHEITPfScheduleTable>(&EPGDatabaseFilter::OnMHEITSection, this));

	// MH-TOT
	m_MMTPacketIDMapManager.MapTarget(
		MMT_PACKET_ID_MH_TOT,
		MMTSectionTable::CreateWithHandler<MHTOTTable>(&EPGDatabaseFilter::OnMHTOTSection, this));

	if (m_pEPGDatabase != nullptr)
		m_pEPGDatabase->ResetTOTTime();
}


bool EPGDatabaseFilter::ProcessData(DataStream *pData)
{
	if (pData->Is<TSPacket>())
		m_PIDMapManager.StorePacketStream(pData);
	else if (pData->Is<TLVPacket>())
		m_MMTPacketIDMapManager.StorePacketStream(pData);

	return true;
}


void EPGDatabaseFilter::SetEPGDatabase(EPGDatabase *pDatabase)
{
	BlockLock Lock(m_FilterLock);

	if (m_pEPGDatabase != nullptr)
		m_pEPGDatabase->RemoveEventListener(this);

	m_pEPGDatabase = pDatabase;

	if (m_pEPGDatabase != nullptr)
		m_pEPGDatabase->AddEventListener(this);
}


EPGDatabase * EPGDatabaseFilter::GetEPGDatabase() const
{
	return m_pEPGDatabase;
}


void EPGDatabaseFilter::SetSourceID(EventInfo::SourceIDType ID)
{
	BlockLock Lock(m_FilterLock);

	m_SourceID = ID;
}


EventInfo::SourceIDType EPGDatabaseFilter::GetSourceID() const
{
	return m_SourceID;
}


void EPGDatabaseFilter::OnScheduleStatusReset(
	EPGDatabase *pEPGDatabase,
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID)
{
	m_ResetTable = true;
}


void EPGDatabaseFilter::OnEITSection(const PSITableBase *pTable, const PSISection *pSection)
{
	if (m_pEPGDatabase != nullptr) {
		const EITPfScheduleTable *pEITScheduleTable = dynamic_cast<const EITPfScheduleTable *>(pTable);

		if (pEITScheduleTable != nullptr) {
			const EITTable *pEITTable = pEITScheduleTable->GetLastUpdatedEITTable();

			if (pEITTable != nullptr) {
				m_ResetTable = false;

				m_pEPGDatabase->UpdateSection(pEITScheduleTable, pEITTable, m_SourceID);

				if (m_ResetTable) {
					const uint16_t NetworkID = pEITTable->GetOriginalNetworkID();
					const uint16_t TransportStreamID = pEITTable->GetTransportStreamID();
					const uint16_t ServiceID = pEITTable->GetServiceID();

					if (EITPfScheduleTable *pHEITTable = m_PIDMapManager.GetMapTarget<EITPfScheduleTable>(PID_HEIT);
							pHEITTable != nullptr)
						pHEITTable->ResetScheduleService(NetworkID, TransportStreamID, ServiceID);

					if (EITPfScheduleTable *pLEITTable = m_PIDMapManager.GetMapTarget<EITPfScheduleTable>(PID_LEIT);
							pLEITTable != nullptr)
						pLEITTable->ResetScheduleService(NetworkID, TransportStreamID, ServiceID);
				}
			}
		}
	}
}


void EPGDatabaseFilter::OnTOTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	if (m_pEPGDatabase != nullptr) {
		const TOTTable *pTOTTable = dynamic_cast<const TOTTable *>(pTable);

		if (pTOTTable != nullptr)
			m_pEPGDatabase->UpdateTOT(pTOTTable);
	}
}


void EPGDatabaseFilter::OnMHEITSection(const PSITableBase *pTable, const PSISection *pSection)
{
	if (m_pEPGDatabase == nullptr)
		return;

	const MHEITPfScheduleTable *pScheduleTable = dynamic_cast<const MHEITPfScheduleTable *>(pTable);
	if (pScheduleTable == nullptr)
		return;

	const MHEITTable *pEITTable = pScheduleTable->GetLastUpdatedEITTable();
	EPGDatabase::EventSectionInfo Section;
	if (!GetMHEITSectionInfo(pEITTable, &Section))
		return;

	m_ResetTable = false;

	m_pEPGDatabase->UpdateEvents(Section, MHEITEventSectionSource(pEITTable), m_SourceID);

	if (m_ResetTable) {
		MMTSectionTable *pSectionTable = m_MMTPacketIDMapManager.GetMapTarget<MMTSectionTable>(MMT_PACKET_ID_MH_EIT);
		if (pSectionTable != nullptr) {
			MHEITPfScheduleTable *pMHEITTable = dynamic_cast<MHEITPfScheduleTable *>(pSectionTable->GetTable());
			if (pMHEITTable != nullptr)
				pMHEITTable->ResetScheduleService(Section.NetworkID, Section.TransportStreamID, Section.ServiceID);
		}
	}
}


void EPGDatabaseFilter::OnMHTOTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	if (m_pEPGDatabase == nullptr)
		return;

	const MHTOTTable *pTOTTable = dynamic_cast<const MHTOTTable *>(pTable);
	DateTime Time;
	if ((pTOTTable != nullptr) && pTOTTable->GetDateTime(&Time))
		m_pEPGDatabase->UpdateTOTTime(Time);
}


} // namespace LibISDB
