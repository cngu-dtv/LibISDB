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
 @file   MMTMediaReceiver.cpp
 @brief  MMT の映像・音声の受信
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "MMTMediaReceiver.hpp"
#include "MMTConsts.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


MMTMediaReceiver::MMTMediaReceiver()
	: m_pHandler(nullptr)
{
	Reset();
}


MMTMediaReceiver::~MMTMediaReceiver()
{
	m_PacketIDMapManager.UnmapAllTargets();
}


void MMTMediaReceiver::Reset()
{
	m_PacketIDMapManager.UnmapAllTargets();
	m_MPTPacketIDList.clear();
	m_VideoPacketID = MMT_PACKET_ID_INVALID;
	m_AudioPacketID = MMT_PACKET_ID_INVALID;
	m_LastNTPTime = 0;
	m_LastNTPLeapIndicator = 0;

	m_PacketIDMapManager.MapTarget(
		MMT_PACKET_ID_PA, MMTSingleTable::CreateWithHandler<PLTable>(&MMTMediaReceiver::OnPLTTable, this));
}


void MMTMediaReceiver::StorePacketStream(DataStream *pData)
{
	if (!pData->Is<TLVPacket>())
		return;

	switch (pData->Get<TLVPacket>()->GetPayloadKind()) {
	case TLVPacket::PayloadKind::MMTP:
		m_PacketIDMapManager.StorePacketStream(pData);
		break;

	case TLVPacket::PayloadKind::NTP:
		do {
			OnNTPPacket(pData->Get<TLVPacket>());
		} while (pData->Next());
		break;

	default:
		break;
	}
}


void MMTMediaReceiver::SetVideoPacketID(uint16_t PacketID)
{
	if (PacketID == m_VideoPacketID)
		return;

	LIBISDB_TRACE(LIBISDB_STR("MMTMediaReceiver::SetVideoPacketID() : {:04X} <- {:04X}\n"), PacketID, m_VideoPacketID);

	const uint16_t OldPacketID = m_VideoPacketID;
	m_VideoPacketID = PacketID;
	UnmapAsset(OldPacketID);
	ApplyLastMPT();
}


void MMTMediaReceiver::SetAudioPacketID(uint16_t PacketID)
{
	if (PacketID == m_AudioPacketID)
		return;

	LIBISDB_TRACE(LIBISDB_STR("MMTMediaReceiver::SetAudioPacketID() : {:04X} <- {:04X}\n"), PacketID, m_AudioPacketID);

	const uint16_t OldPacketID = m_AudioPacketID;
	m_AudioPacketID = PacketID;
	UnmapAsset(OldPacketID);
	ApplyLastMPT();
}


int64_t MMTMediaReceiver::GetNTPTime90kHz() const noexcept
{
	return MPUTimestampTable::NTPTimeTo90kHz(m_LastNTPTime);
}


void MMTMediaReceiver::OnNTPPacket(const TLVPacket *pPacket)
{
	const uint64_t Time = pPacket->GetNTPTransmitTimestamp();
	if (Time == 0)
		return;

	/*
		うるう秒の調整 (ARIB TR-B39 第1部 第7編 7.12.2.2)
		挿入では 8:59:59 JST (23:59:59 UTC) が繰り返され、削除では 8:59:59 JST が飛ぶ
		削除の leap_indicator は 8:59:58 まで 2 なので、飛んだ後のパケットでは 0 に戻っていることがある
	*/
	if (m_LastNTPTime != 0) {
		constexpr int64_t SECONDS_PER_DAY = 24 * 60 * 60;
		const int64_t LastSeconds = static_cast<int64_t>(m_LastNTPTime >> 32);
		const int64_t Seconds = static_cast<int64_t>(Time >> 32);
		const int64_t Diff =
			MPUTimestampTable::NTPTimeTo90kHz(Time) - MPUTimestampTable::NTPTimeTo90kHz(m_LastNTPTime);
		const uint8_t LeapIndicator = pPacket->GetNTPLeapIndicator();
		int Step = 0;

		if ((LeapIndicator == 1)
				&& (Diff <= -90000 + 9000) && (Diff >= -90000 - 9000)
				&& (Seconds % SECONDS_PER_DAY == SECONDS_PER_DAY - 1)) {
			Step = -1;
		} else if (((m_LastNTPLeapIndicator == 2) || (LeapIndicator == 2))
				&& (Diff >= 90000) && (Diff <= 90000 + 18000)
				&& (LastSeconds % SECONDS_PER_DAY < SECONDS_PER_DAY - 1)
				&& (Seconds / SECONDS_PER_DAY > LastSeconds / SECONDS_PER_DAY)) {
			Step = 1;
		}

		if ((Step != 0) && (m_pHandler != nullptr)) {
			LIBISDB_TRACE(LIBISDB_STR("MMTMediaReceiver : Leap second {:+}\n"), Step);
			m_pHandler->OnNTPLeapSecond(this, Step);
		}
	}

	m_LastNTPTime = Time;
	m_LastNTPLeapIndicator = pPacket->GetNTPLeapIndicator();
}


const MPUAssembler * MMTMediaReceiver::GetAssembler(MediaType Type) const
{
	const uint16_t PacketID = (Type == MediaType::Video) ? m_VideoPacketID : m_AudioPacketID;
	if (PacketID == MMT_PACKET_ID_INVALID)
		return nullptr;

	const MPUAssemblerTarget *pTarget = m_PacketIDMapManager.GetMapTarget<MPUAssemblerTarget>(PacketID);
	if (pTarget == nullptr)
		return nullptr;

	return &pTarget->GetAssembler();
}


void MMTMediaReceiver::OnAccessUnit(const MPUAssembler *pAssembler, const MPUAssembler::AccessUnitInfo &Info)
{
	if (m_pHandler == nullptr)
		return;

	MediaType Type;
	if (Info.PacketID == m_VideoPacketID)
		Type = MediaType::Video;
	else if (Info.PacketID == m_AudioPacketID)
		Type = MediaType::Audio;
	else
		return;

	if (Info.HasTimestamp && HasNTPTime()) {
		const int64_t NTPTime = GetNTPTime90kHz();
		if ((std::llabs(Info.DTS - NTPTime) > MAX_TIMESTAMP_DEVIATION)
				|| (std::llabs(Info.PTS - NTPTime) > MAX_TIMESTAMP_DEVIATION)) {
			MPUAssembler::AccessUnitInfo NewInfo = Info;
			NewInfo.HasTimestamp = false;
			m_pHandler->OnAccessUnit(this, Type, NewInfo);
			return;
		}
	}

	m_pHandler->OnAccessUnit(this, Type, Info);
}


void MMTMediaReceiver::OnPLTTable(const MMTSingleTable *pTable)
{
	const PLTable *pPLTable = dynamic_cast<const PLTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pPLTable == nullptr))
		return;

	for (const uint16_t PacketID : m_MPTPacketIDList)
		m_PacketIDMapManager.UnmapTarget(PacketID);
	m_MPTPacketIDList.clear();

	for (int i = 0; i < pPLTable->GetPackageCount(); i++) {
		const uint16_t PacketID = pPLTable->GetMPTPacketID(i);

		// 処理中の PLT を破棄しないよう、PLT の packet_id には割り当てない
		if ((PacketID != MMT_PACKET_ID_INVALID) && (PacketID != MMT_PACKET_ID_PA)
				&& (m_PacketIDMapManager.GetMapTarget(PacketID) == nullptr)) {
			m_PacketIDMapManager.MapTarget(
				PacketID, MMTSingleTable::CreateWithHandler<MPTable>(&MMTMediaReceiver::OnMPTTable, this));
			m_MPTPacketIDList.push_back(PacketID);
		}
	}
}


void MMTMediaReceiver::OnMPTTable(const MMTSingleTable *pTable)
{
	const MPTable *pMPTable = dynamic_cast<const MPTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pMPTable == nullptr))
		return;

	UpdateAssets(pMPTable);
}


void MMTMediaReceiver::UpdateAssets(const MPTable *pMPTable)
{
	// MPT はタイムスタンプ記述子が更新される度に内容が変わるので、その都度 MPU の時刻を更新する
	for (int i = 0; i < pMPTable->GetAssetCount(); i++) {
		const uint16_t PacketID = pMPTable->GetAssetPacketID(i);
		if ((PacketID == MMT_PACKET_ID_INVALID) || (PacketID == MMT_PACKET_ID_PA))
			continue;
		if ((PacketID != m_VideoPacketID) && (PacketID != m_AudioPacketID))
			continue;

		const MPUAssembler::MediaType Type = MPUAssembler::GetMediaTypeFromAssetType(pMPTable->GetAssetType(i));
		if ((PacketID == m_VideoPacketID) ? (Type != MPUAssembler::MediaType::Video) : (Type != MPUAssembler::MediaType::Audio))
			continue;

		MPUAssemblerTarget *pTarget = m_PacketIDMapManager.GetMapTarget<MPUAssemblerTarget>(PacketID);
		if ((pTarget != nullptr) && (pTarget->GetAssembler().GetMediaType() != Type)) {
			m_PacketIDMapManager.UnmapTarget(PacketID);
			pTarget = nullptr;
		}
		if (pTarget == nullptr) {
			if (m_PacketIDMapManager.GetMapTarget(PacketID) != nullptr)
				continue;
			pTarget = new MPUAssemblerTarget(Type, this);
			m_PacketIDMapManager.MapTarget(PacketID, pTarget);
		}

		const MPTable::AssetInfo *pAssetInfo = pMPTable->GetAssetInfo(i);
		pTarget->GetAssembler().UpdateTimestamps(
			pMPTable->GetAssetDescriptorBlock(i),
			pAssetInfo->AssetTimescaleFlag ? pAssetInfo->AssetTimescale : 0);
	}
}


void MMTMediaReceiver::ApplyLastMPT()
{
	// 次の MPT を待たずに、選択されたアセットの組み立てを始める
	for (const uint16_t PacketID : m_MPTPacketIDList) {
		const MPTable *pMPTable = m_PacketIDMapManager.GetMapTarget<MPTable>(PacketID);
		if (pMPTable != nullptr)
			UpdateAssets(pMPTable);
	}
}


void MMTMediaReceiver::UnmapAsset(uint16_t PacketID)
{
	if ((PacketID == MMT_PACKET_ID_INVALID) || (PacketID == m_VideoPacketID) || (PacketID == m_AudioPacketID))
		return;

	if (m_PacketIDMapManager.GetMapTarget<MPUAssemblerTarget>(PacketID) != nullptr)
		m_PacketIDMapManager.UnmapTarget(PacketID);
}


} // namespace LibISDB
