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
 @file   MPUAssembler.cpp
 @brief  MPU の組み立て
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "MPUAssembler.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


namespace
{

int64_t TimescaleTo90kHz(int64_t Value, uint32_t Timescale)
{
	const int64_t v = Value * 90000;
	if (v >= 0)
		return v / Timescale;
	return -((-v + Timescale - 1) / Timescale);
}

}




MPUTimestampTable::MPUTimestampTable()
	: m_HasLatestSequenceNumber(false)
	, m_LatestSequenceNumber(0)
{
}


void MPUTimestampTable::Reset()
{
	m_MPUMap.clear();
	m_HasLatestSequenceNumber = false;
	m_LatestSequenceNumber = 0;
}


void MPUTimestampTable::Update(const MMTDescriptorBlock *pDescBlock, uint32_t AssetTimescale)
{
	if (pDescBlock == nullptr)
		return;

	// MPU_sequence_number は送出設備の切り替え等で 0 に戻ることがあるため (ARIB TR-B39 第1部 第4編 34.1.3.10)、
	// 今回の記述子の中で最も新しいものを基準にする
	bool HasLatest = false;
	uint32_t Latest = 0;
	const auto UpdateLatest =
		[&](uint32_t SequenceNumber) {
			if (!HasLatest || (static_cast<int32_t>(SequenceNumber - Latest) > 0)) {
				Latest = SequenceNumber;
				HasLatest = true;
			}
		};

	pDescBlock->EnumDescriptors<MPUTimestampDescriptor>(
		[&](const MPUTimestampDescriptor *pDesc) {
			for (int i = 0; i < pDesc->GetTimestampCount(); i++) {
				const MPUTimestampDescriptor::TimestampInfo *pInfo = pDesc->GetTimestampInfo(i);
				MPUEntry &Entry = m_MPUMap[pInfo->MPUSequenceNumber];
				Entry.HasPresentationTime = true;
				Entry.PresentationTime = pInfo->MPUPresentationTime;
				UpdateLatest(pInfo->MPUSequenceNumber);
			}
		});

	pDescBlock->EnumDescriptors<MPUExtendedTimestampDescriptor>(
		[&](const MPUExtendedTimestampDescriptor *pDesc) {
			// pts_offset_type 0 の固定値は規定されていない
			if (pDesc->GetPTSOffsetType() == MPUExtendedTimestampDescriptor::PTS_OFFSET_TYPE_FIXED)
				return;
			const uint32_t Timescale = pDesc->GetTimescaleFlag() ? pDesc->GetTimescale() : AssetTimescale;
			if (Timescale == 0)
				return;

			for (int i = 0; i < pDesc->GetMPUCount(); i++) {
				const MPUExtendedTimestampDescriptor::MPUInfo *pInfo = pDesc->GetMPUInfo(i);
				MPUEntry &Entry = m_MPUMap[pInfo->MPUSequenceNumber];
				Entry.HasExtendedTimestamp = true;
				Entry.Timescale = Timescale;
				Entry.DecodingTimeOffset = pInfo->MPUDecodingTimeOffset;
				Entry.AccessUnitList = pInfo->AccessUnitList;
				UpdateLatest(pInfo->MPUSequenceNumber);
			}
		});

	if (HasLatest) {
		m_LatestSequenceNumber = Latest;
		m_HasLatestSequenceNumber = true;
		Prune();
	}
}


bool MPUTimestampTable::GetPresentationTime(uint32_t MPUSequenceNumber, ReturnArg<uint64_t> Time) const
{
	if (!Time)
		return false;

	auto it = m_MPUMap.find(MPUSequenceNumber);
	if ((it == m_MPUMap.end()) || !it->second.HasPresentationTime)
		return false;

	*Time = it->second.PresentationTime;

	return true;
}


bool MPUTimestampTable::GetAccessUnitTime(
	uint32_t MPUSequenceNumber, uint32_t Index,
	ReturnArg<int64_t> DTS, ReturnArg<int64_t> PTS) const
{
	auto it = m_MPUMap.find(MPUSequenceNumber);
	if (it == m_MPUMap.end())
		return false;

	const MPUEntry &Entry = it->second;
	if (!Entry.HasPresentationTime || !Entry.HasExtendedTimestamp
			|| (Index >= Entry.AccessUnitList.size()))
		return false;

	// DTS(m) = mpu_presentation_time - mpu_decoding_time_offset + Σ(l < m) pts_offset(l)
	// PTS(m) = DTS(m) + dts_pts_offset(m)
	int64_t Ticks = -static_cast<int64_t>(Entry.DecodingTimeOffset);
	for (uint32_t i = 0; i < Index; i++)
		Ticks += Entry.AccessUnitList[i].PTSOffset;

	const int64_t Base = NTPTimeTo90kHz(Entry.PresentationTime);

	if (DTS)
		*DTS = Base + TimescaleTo90kHz(Ticks, Entry.Timescale);
	if (PTS)
		*PTS = Base + TimescaleTo90kHz(Ticks + Entry.AccessUnitList[Index].DTSPTSOffset, Entry.Timescale);

	return true;
}


int64_t MPUTimestampTable::NTPTimeTo90kHz(uint64_t Time) noexcept
{
	return static_cast<int64_t>(Time >> 32) * 90000
		+ static_cast<int64_t>(((Time & 0xFFFFFFFF_u64) * 90000) >> 32);
}


void MPUTimestampTable::Prune()
{
	for (auto it = m_MPUMap.begin(); it != m_MPUMap.end();) {
		const int32_t Distance = static_cast<int32_t>(it->first - m_LatestSequenceNumber);
		if ((Distance > MAX_SEQUENCE_DISTANCE) || (Distance < -MAX_SEQUENCE_DISTANCE))
			it = m_MPUMap.erase(it);
		else
			++it;
	}
}




MPUAssembler::MPUAssembler(MediaType Type, Handler *pHandler)
	: m_MediaType(Type)
	, m_pHandler(pHandler)
{
	Reset();
}


void MPUAssembler::Reset()
{
	m_PacketID = MMT_PACKET_ID_INVALID;
	m_TimestampTable.Reset();
	m_Statistics = StatisticsInfo();

	m_HasSequenceNumber = false;
	m_NextSequenceNumber = 0;
	m_Discontinuity = false;

	m_IsFragmentStoring = false;
	m_FragmentMPUSequenceNumber = 0;
	m_FragmentBuffer.ClearSize();

	m_HasMPU = false;
	m_MPUSequenceNumber = 0;
	m_MPUIndexValid = false;
	m_MPUUnitCount = 0;

	m_UnitStarted = false;
	m_UnitValid = false;
	m_UnitIndexValid = false;
	m_UnitIndex = 0;
	m_UnitMPUSequenceNumber = 0;
	m_UnitRandomAccess = false;
	m_UnitBuffer.ClearSize();

	m_CaptionHeader = CaptionMFUHeader();
	m_CaptionNextSubsample = 0;
	m_CaptionSubsampleList.clear();
	m_CaptionSubsampleOffset.clear();
}


void MPUAssembler::StorePacket(const TLVPacket *pPacket)
{
	if ((pPacket == nullptr) || !pPacket->HasMMTPHeader())
		return;

	const MMTPHeader &Header = pPacket->GetMMTPHeader();
	if (Header.PayloadType != MMTP_PAYLOAD_TYPE_MPU)
		return;

	m_PacketID = Header.PacketID;
	m_Statistics.PacketCount++;

	bool Lost = false;
	if (m_HasSequenceNumber) {
		if (Header.PacketSequenceNumber == m_NextSequenceNumber - 1)
			return;
		if (Header.PacketSequenceNumber != m_NextSequenceNumber) {
			m_Statistics.LossCount++;
			OnLoss();
			Lost = true;
		}
	}
	m_HasSequenceNumber = true;
	m_NextSequenceNumber = Header.PacketSequenceNumber + 1;

	if (pPacket->IsScrambled()) {
		m_Statistics.ScrambledCount++;
		OnLoss();
		return;
	}

	MPUPayloadHeader PayloadHeader;
	if (!ParseMPUPayload(
			pPacket->GetMMTPPayloadData(), pPacket->GetMMTPPayloadSize(),
			&PayloadHeader, &m_DataUnitList)) {
		OnLoss();
		return;
	}
	if ((PayloadHeader.FragmentType != MPU_FRAGMENT_TYPE_MFU) || !PayloadHeader.TimedFlag)
		return;

	if (!m_HasMPU || (PayloadHeader.MPUSequenceNumber != m_MPUSequenceNumber)) {
		// MPU の先頭のパケットは RAP_flag が 1 (ARIB TR-B39 第1部 第7編 5.6.2)
		OnMPUChanged(PayloadHeader.MPUSequenceNumber, Header.RAPFlag && !Lost);
	}

	if (PayloadHeader.AggregationFlag) {
		if (m_IsFragmentStoring)
			OnLoss();
		for (const MFUDataUnit &Unit : m_DataUnitList)
			OnMFU(Unit.pData, Unit.Size);
		return;
	}

	const MFUDataUnit &Unit = m_DataUnitList.front();

	switch (PayloadHeader.FragmentationIndicator) {
	case MMT_FRAGMENTATION_INDICATOR_COMPLETE:
		if (m_IsFragmentStoring)
			OnLoss();
		OnMFU(Unit.pData, Unit.Size);
		break;

	case MMT_FRAGMENTATION_INDICATOR_FIRST:
		if (m_IsFragmentStoring)
			OnLoss();
		m_FragmentBuffer.SetData(Unit.pData, Unit.Size);
		m_FragmentMPUSequenceNumber = PayloadHeader.MPUSequenceNumber;
		m_IsFragmentStoring = true;
		break;

	case MMT_FRAGMENTATION_INDICATOR_MIDDLE:
	case MMT_FRAGMENTATION_INDICATOR_LAST:
		if (!m_IsFragmentStoring)
			break;
		if ((m_FragmentMPUSequenceNumber != PayloadHeader.MPUSequenceNumber)
				|| (m_FragmentBuffer.GetSize() + Unit.Size > MAX_UNIT_SIZE)) {
			OnLoss();
			break;
		}
		m_FragmentBuffer.AddData(Unit.pData, Unit.Size);
		if (PayloadHeader.FragmentationIndicator == MMT_FRAGMENTATION_INDICATOR_LAST) {
			m_IsFragmentStoring = false;
			OnMFU(m_FragmentBuffer.GetData(), m_FragmentBuffer.GetSize());
		}
		break;
	}
}


void MPUAssembler::Flush()
{
	if (m_MediaType == MediaType::Video)
		OutputVideoAccessUnit();
}


void MPUAssembler::UpdateTimestamps(const MMTDescriptorBlock *pDescBlock, uint32_t AssetTimescale)
{
	m_TimestampTable.Update(pDescBlock, AssetTimescale);
}


MPUAssembler::MediaType MPUAssembler::GetMediaTypeFromAssetType(uint32_t AssetType) noexcept
{
	switch (AssetType) {
	case MMT_ASSET_TYPE_HEV1:
	case MMT_ASSET_TYPE_HVC1:
		return MediaType::Video;
	case MMT_ASSET_TYPE_MP4A:
		return MediaType::Audio;
	case MMT_ASSET_TYPE_STPP:
		return MediaType::Caption;
	}

	return MediaType::Generic;
}


void MPUAssembler::OnLoss()
{
	m_IsFragmentStoring = false;
	m_Discontinuity = true;
	m_MPUIndexValid = false;
	if (m_UnitStarted)
		m_UnitValid = false;
}


void MPUAssembler::OnMPUChanged(uint32_t MPUSequenceNumber, bool RandomAccess)
{
	if (m_IsFragmentStoring) {
		m_IsFragmentStoring = false;
		m_Discontinuity = true;
	}

	if (m_MediaType == MediaType::Video) {
		OutputVideoAccessUnit();
	} else if (m_MediaType == MediaType::Caption) {
		if (m_UnitStarted)
			DropCaptionUnit();
	}

	m_HasMPU = true;
	m_MPUSequenceNumber = MPUSequenceNumber;
	m_MPUIndexValid = RandomAccess;
	m_MPUUnitCount = 0;
}


void MPUAssembler::OnMFU(const uint8_t *pData, size_t Size)
{
	switch (m_MediaType) {
	case MediaType::Video:
		OnVideoNALUnit(pData, Size);
		break;
	case MediaType::Audio:
		OnAudioFrame(pData, Size);
		break;
	case MediaType::Caption:
		OnCaptionSubsample(pData, Size);
		break;
	case MediaType::Generic:
		OnGenericMFU(pData, Size);
		break;
	}
}


void MPUAssembler::OnVideoNALUnit(const uint8_t *pData, size_t Size)
{
	// MFU は 32 ビットの長さの後に NAL ユニット (ARIB STD-B60 8.1.1)
	if (Size < 4 + 2) {
		if (m_UnitStarted)
			m_UnitValid = false;
		return;
	}

	const uint8_t *pNALUnit = pData + 4;
	const size_t NALUnitSize = Size - 4;
	const uint8_t NALUnitType = (pNALUnit[0] >> 1) & 0x3F;

	if (NALUnitType == HEVC_NAL_UNIT_TYPE_AUD) {
		OutputVideoAccessUnit();

		m_UnitStarted = true;
		m_UnitValid = true;
		m_UnitIndexValid = m_MPUIndexValid;
		m_UnitIndex = m_MPUUnitCount++;
		m_UnitMPUSequenceNumber = m_MPUSequenceNumber;
		m_UnitRandomAccess = false;
		m_UnitBuffer.ClearSize();
	} else if (m_MPUUnitCount == 0) {
		// MPU の先頭が AUD でなければ途中から受け取っているため、MPU 内の順番が分からない
		m_MPUIndexValid = false;
	}

	if (!m_UnitStarted || !m_UnitValid)
		return;

	if ((NALUnitType >= HEVC_NAL_UNIT_TYPE_IRAP_FIRST) && (NALUnitType <= HEVC_NAL_UNIT_TYPE_IRAP_LAST))
		m_UnitRandomAccess = true;

	if (m_UnitBuffer.GetSize() + 4 + NALUnitSize > MAX_UNIT_SIZE) {
		m_UnitValid = false;
		return;
	}

	static const uint8_t StartCode[4] = {0x00, 0x00, 0x00, 0x01};
	m_UnitBuffer.AddData(StartCode, sizeof(StartCode));
	m_UnitBuffer.AddData(pNALUnit, NALUnitSize);
}


void MPUAssembler::OutputVideoAccessUnit()
{
	if (!m_UnitStarted)
		return;

	m_UnitStarted = false;

	if (!m_UnitValid || (m_UnitBuffer.GetSize() == 0)) {
		m_Statistics.DroppedCount++;
		m_Discontinuity = true;
		return;
	}

	OutputAccessUnit(
		m_UnitMPUSequenceNumber, m_UnitIndexValid, m_UnitIndex, m_UnitRandomAccess,
		m_UnitBuffer.GetData(), m_UnitBuffer.GetSize());
}


void MPUAssembler::OnAudioFrame(const uint8_t *pData, size_t Size)
{
	OutputAccessUnit(m_MPUSequenceNumber, m_MPUIndexValid, m_MPUUnitCount++, true, pData, Size);
}


void MPUAssembler::OnGenericMFU(const uint8_t *pData, size_t Size)
{
	OutputAccessUnit(m_MPUSequenceNumber, m_MPUIndexValid, m_MPUUnitCount++, false, pData, Size);
}


void MPUAssembler::OutputAccessUnit(
	uint32_t MPUSequenceNumber, bool IndexValid, uint32_t Index, bool RandomAccess,
	const uint8_t *pData, size_t Size)
{
	AccessUnitInfo Info;

	Info.PacketID = m_PacketID;
	Info.MPUSequenceNumber = MPUSequenceNumber;
	Info.Index = Index;
	Info.HasTimestamp =
		IndexValid && m_TimestampTable.GetAccessUnitTime(MPUSequenceNumber, Index, &Info.DTS, &Info.PTS);
	if (!Info.HasTimestamp) {
		Info.DTS = 0;
		Info.PTS = 0;
		m_Statistics.NoTimestampCount++;
	}
	Info.RandomAccess = RandomAccess;
	Info.Discontinuity = m_Discontinuity;
	Info.pData = pData;
	Info.DataSize = Size;

	m_Discontinuity = false;
	m_Statistics.OutputCount++;

	if (m_pHandler != nullptr)
		m_pHandler->OnAccessUnit(this, Info);
}


void MPUAssembler::OnCaptionSubsample(const uint8_t *pData, size_t Size)
{
	CaptionMFUHeader Header;

	if (!Header.Parse(pData, Size)) {
		if (m_UnitStarted)
			DropCaptionUnit();
		m_Discontinuity = true;
		return;
	}

	if (Header.SubsampleNumber == 0) {
		if (m_UnitStarted)
			DropCaptionUnit();
		m_UnitStarted = true;
		m_UnitValid = true;
		m_UnitMPUSequenceNumber = m_MPUSequenceNumber;
		m_UnitBuffer.ClearSize();
		m_CaptionHeader = Header;
		m_CaptionSubsampleList.clear();
		m_CaptionSubsampleOffset.clear();
	} else if (!m_UnitStarted) {
		return;
	} else if (Header.SubsampleNumber != m_CaptionNextSubsample) {
		DropCaptionUnit();
		return;
	}

	if (m_UnitBuffer.GetSize() + Header.DataSize > MAX_UNIT_SIZE) {
		m_UnitValid = false;
	} else {
		SubsampleInfo &Subsample = m_CaptionSubsampleList.emplace_back();
		Subsample.DataType = Header.DataType;
		Subsample.pData = nullptr;
		Subsample.DataSize = Header.DataSize;
		m_CaptionSubsampleOffset.push_back(m_UnitBuffer.GetSize());
		m_UnitBuffer.AddData(pData + Header.HeaderSize, Header.DataSize);
	}

	m_CaptionNextSubsample = Header.SubsampleNumber + 1;

	if (Header.SubsampleNumber < m_CaptionHeader.LastSubsampleNumber)
		return;

	m_UnitStarted = false;

	if (!m_UnitValid) {
		m_Statistics.DroppedCount++;
		m_Discontinuity = true;
		return;
	}

	CaptionUnitInfo Info;

	Info.PacketID = m_PacketID;
	Info.MPUSequenceNumber = m_UnitMPUSequenceNumber;
	Info.SubtitleTag = m_CaptionHeader.SubtitleTag;
	Info.SubtitleSequenceNumber = m_CaptionHeader.SubtitleSequenceNumber;
	Info.HasPresentationTime = m_TimestampTable.GetPresentationTime(m_UnitMPUSequenceNumber, &Info.PresentationTime);
	if (!Info.HasPresentationTime)
		Info.PresentationTime = 0;
	Info.Discontinuity = m_Discontinuity;
	Info.SubsampleList = m_CaptionSubsampleList;
	for (size_t i = 0; i < Info.SubsampleList.size(); i++)
		Info.SubsampleList[i].pData = m_UnitBuffer.GetData() + m_CaptionSubsampleOffset[i];

	m_Discontinuity = false;
	m_Statistics.OutputCount++;

	if (m_pHandler != nullptr)
		m_pHandler->OnCaptionUnit(this, Info);
}


void MPUAssembler::DropCaptionUnit()
{
	m_UnitStarted = false;
	m_Statistics.DroppedCount++;
	m_Discontinuity = true;
}




MPUAssemblerTarget::MPUAssemblerTarget(MPUAssembler::MediaType Type, MPUAssembler::Handler *pHandler)
	: m_Assembler(Type, pHandler)
{
}


bool MPUAssemblerTarget::StorePacket(const TLVPacket *pPacket)
{
	m_Assembler.StorePacket(pPacket);
	return false;
}


void MPUAssemblerTarget::OnPacketIDUnmapped(uint16_t PacketID)
{
	delete this;
}


} // namespace LibISDB
