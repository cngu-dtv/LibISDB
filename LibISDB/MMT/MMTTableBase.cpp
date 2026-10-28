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
 @file   MMTTableBase.cpp
 @brief  MMT-SI テーブルの基底
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "MMTTableBase.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


MMTTableBase::MMTTableBase()
	: m_SignalingParser(this)
{
}


void MMTTableBase::Reset()
{
	m_SignalingParser.Reset();
}


unsigned long MMTTableBase::GetCRCErrorCount() const noexcept
{
	return m_SignalingParser.GetCRCErrorCount();
}


bool MMTTableBase::StorePacket(const TLVPacket *pPacket)
{
	if (pPacket == nullptr)
		return false;

	m_SignalingParser.StorePacket(pPacket);

	return true;
}


void MMTTableBase::OnPacketIDUnmapped(uint16_t PacketID)
{
	delete this;
}




MMTSingleTable::MMTSingleTable(uint8_t TableID)
	: m_TableID(TableID)
{
}


void MMTSingleTable::Reset()
{
	MMTTableBase::Reset();
	m_CurTable.Reset();
}


void MMTSingleTable::SetTableHandler(const TableHandler &Handler)
{
	m_TableHandler = Handler;
}


void MMTSingleTable::OnMMTTable(const MMTSignalingParser *pParser, const MMTTable *pTable)
{
	if (pTable->GetTableID() != m_TableID)
		return;

	if (*pTable != m_CurTable) {
		if (OnTableUpdate(pTable, &m_CurTable)) {
			m_CurTable = *pTable;
			if (m_TableHandler)
				m_TableHandler(this);
		}
	}
}


bool MMTSingleTable::OnTableUpdate(const MMTTable *pCurTable, const MMTTable *pOldTable)
{
	return true;
}




MMTSectionTable::MMTSectionTable(PSITableBase *pTable)
	: m_Table(pTable)
{
}


void MMTSectionTable::Reset()
{
	MMTTableBase::Reset();
	if (m_Table)
		m_Table->Reset();
}


void MMTSectionTable::OnPSISection(const MMTSignalingParser *pParser, const PSISection *pSection)
{
	if (m_Table)
		m_Table->OnPSISection(nullptr, pSection);
}


} // namespace LibISDB
