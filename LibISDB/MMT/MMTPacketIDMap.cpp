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
 @file   MMTPacketIDMap.cpp
 @brief  MMTP パケットの packet_id マップ
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "MMTPacketIDMap.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


MMTPacketIDMapManager::~MMTPacketIDMapManager()
{
	UnmapAllTargets();
}


bool MMTPacketIDMapManager::StorePacket(const TLVPacket *pPacket)
{
	if (!pPacket->HasMMTPHeader())
		return false;

	MMTPacketIDMapTarget *pTarget = GetMapTarget(pPacket->GetPacketID());

	if (pTarget == nullptr)
		return false;

	return pTarget->StorePacket(pPacket);
}


bool MMTPacketIDMapManager::StorePacketStream(DataStream *pPacketStream)
{
	const TLVPacket *pFirstPacket = pPacketStream->Get<TLVPacket>();
	if (!pFirstPacket->HasMMTPHeader())
		return false;

	const uint16_t PacketID = pFirstPacket->GetPacketID();

	if (GetMapTarget(PacketID) == nullptr)
		return false;

	do {
		const TLVPacket *pPacket = pPacketStream->Get<TLVPacket>();

		LIBISDB_ASSERT(pPacket->HasMMTPHeader() && (pPacket->GetPacketID() == PacketID));

		// 処理中に割り当てが変わることがあるため毎回取得する
		MMTPacketIDMapTarget *pTarget = GetMapTarget(PacketID);

		if (pTarget == nullptr)
			break;

		pTarget->StorePacket(pPacket);
	} while (pPacketStream->Next());

	return true;
}


bool MMTPacketIDMapManager::MapTarget(uint16_t PacketID, MMTPacketIDMapTarget *pMapTarget)
{
	if (pMapTarget == nullptr)
		return false;

	UnmapTarget(PacketID);

	m_PacketIDMap.emplace(PacketID, pMapTarget);

	pMapTarget->OnPacketIDMapped(PacketID);

	return true;
}


bool MMTPacketIDMapManager::UnmapTarget(uint16_t PacketID)
{
	auto it = m_PacketIDMap.find(PacketID);

	if (it == m_PacketIDMap.end())
		return false;

	MMTPacketIDMapTarget *pTarget = it->second;

	m_PacketIDMap.erase(it);

	pTarget->OnPacketIDUnmapped(PacketID);

	return true;
}


void MMTPacketIDMapManager::UnmapAllTargets()
{
	while (!m_PacketIDMap.empty())
		UnmapTarget(m_PacketIDMap.begin()->first);
}


MMTPacketIDMapTarget * MMTPacketIDMapManager::GetMapTarget(uint16_t PacketID) const
{
	auto it = m_PacketIDMap.find(PacketID);

	if (it == m_PacketIDMap.end())
		return nullptr;

	return it->second;
}


} // namespace LibISDB
