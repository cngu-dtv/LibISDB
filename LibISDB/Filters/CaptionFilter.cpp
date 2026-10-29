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
 @file   CaptionFilter.cpp
 @brief  字幕フィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "CaptionFilter.hpp"
#include "../TS/Tables.hpp"
#include <algorithm>
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


namespace
{


class CaptionStream : public PIDMapTarget
{
// PIDMapTarget
	bool StorePacket(const TSPacket *pPacket) override
	{
		m_CaptionParser.StorePacket(pPacket);
		return false;
	}

	void OnPIDUnmapped(uint16_t PID) override
	{
		delete this;
	}

	CaptionParser m_CaptionParser;

public:
	CaptionStream(bool OneSeg)
		: m_CaptionParser(OneSeg)
	{
	}

	void SetCaptionHandler(CaptionParser::CaptionHandler *pHandler)
	{
		m_CaptionParser.SetCaptionHandler(pHandler);
	}

	void SetDRCSMap(CaptionParser::DRCSMap *pDRCSMap)
	{
		m_CaptionParser.SetDRCSMap(pDRCSMap);
	}

	CaptionParser * GetParser() { return &m_CaptionParser; }
};


}




CaptionFilter::CaptionFilter()
	: m_FollowActiveService(true)
	, m_TargetServiceID(SERVICE_ID_INVALID)
	, m_TargetComponentTag(0xFF)
	, m_TargetESPID(PID_INVALID)

	, m_pCaptionHandler(nullptr)
	, m_pDRCSMap(nullptr)

	, m_TargetPacketID(MMT_PACKET_ID_INVALID)
	, m_NotifiedPacketID(MMT_PACKET_ID_INVALID)
	, m_NotifiedLanguageCode(LANGUAGE_CODE_INVALID)
{
	Reset();
}


void CaptionFilter::Reset()
{
	BlockLock Lock(m_FilterLock);

	m_ServiceList.clear();

	m_TargetESPID = PID_INVALID;

	m_PIDMapManager.UnmapAllTargets();

	m_PIDMapManager.MapTarget(
		PID_PAT, PSITableBase::CreateWithHandler<PATTable>(&CaptionFilter::OnPATSection, this));

	m_MMTServiceList.clear();
	m_TargetPacketID = MMT_PACKET_ID_INVALID;
	m_NotifiedPacketID = MMT_PACKET_ID_INVALID;
	m_NotifiedLanguageCode = LANGUAGE_CODE_INVALID;

	m_MMTPacketIDMapManager.UnmapAllTargets();

	m_MMTPacketIDMapManager.MapTarget(
		MMT_PACKET_ID_PA, MMTSingleTable::CreateWithHandler<PLTable>(&CaptionFilter::OnPLTTable, this));
}


void CaptionFilter::SetActiveServiceID(uint16_t ServiceID)
{
	if (m_FollowActiveService)
		SetTargetStream(ServiceID);
}


bool CaptionFilter::ProcessData(DataStream *pData)
{
	if (pData->Is<TSPacket>())
		m_PIDMapManager.StorePacketStream(pData);
	else if (pData->Is<TLVPacket>())
		m_MMTPacketIDMapManager.StorePacketStream(pData);

	return true;
}


bool CaptionFilter::SetTargetStream(uint16_t ServiceID, uint8_t ComponentTag)
{
	BlockLock Lock(m_FilterLock);

	LIBISDB_TRACE(LIBISDB_STR("Select caption : service_id {:04X} / component_tag {:02X}\n"), ServiceID, ComponentTag);

	if (m_TargetESPID != PID_INVALID) {
		CaptionStream *pStream = m_PIDMapManager.GetMapTarget<CaptionStream>(m_TargetESPID);

		if (pStream != nullptr) {
			pStream->SetCaptionHandler(nullptr);
			pStream->SetDRCSMap(nullptr);
		}

		m_TargetESPID = PID_INVALID;
	}

	const int Index = GetServiceIndexByID(ServiceID);
	if (Index >= 0) {
		uint16_t TargetESPID = PID_INVALID;

		if (ComponentTag == 0xFF) {
			if (!m_ServiceList[Index].CaptionESList.empty())
				TargetESPID = m_ServiceList[Index].CaptionESList[0].PID;
		} else {
			for (auto const &e : m_ServiceList[Index].CaptionESList) {
				if (e.ComponentTag == ComponentTag) {
					TargetESPID = e.PID;
					break;
				}
			}
		}

		if (TargetESPID != PID_INVALID) {
			CaptionStream *pStream = m_PIDMapManager.GetMapTarget<CaptionStream>(TargetESPID);

			if (pStream != nullptr) {
				pStream->SetCaptionHandler(this);
				pStream->SetDRCSMap(m_pDRCSMap);
				m_TargetESPID = TargetESPID;
			}
		}
	}

	m_TargetServiceID = ServiceID;
	m_TargetComponentTag = ComponentTag;

	SelectMMTTargetAsset();

	return true;
}


void CaptionFilter::SetFollowActiveService(bool Follow)
{
	BlockLock Lock(m_FilterLock);

	m_FollowActiveService = Follow;
}


void CaptionFilter::SetCaptionHandler(Handler *pHandler)
{
	BlockLock Lock(m_FilterLock);

	m_pCaptionHandler = pHandler;
}


void CaptionFilter::SetDRCSMap(DRCSMap *pDRCSMap)
{
	BlockLock Lock(m_FilterLock);

	m_pDRCSMap = pDRCSMap;

	CaptionParser *pParser = GetCurrentCaptionParser();
	if (pParser != nullptr)
		pParser->SetDRCSMap(pDRCSMap);
}


int CaptionFilter::GetLanguageCount() const
{
	BlockLock Lock(m_FilterLock);

	// TLV/MMT では 1 アセット 1 言語 (ARIB STD-B60 9.1)
	if (GetMMTTargetAsset() != nullptr)
		return 1;

	const CaptionParser *pParser = GetCurrentCaptionParser();
	if (pParser == nullptr)
		return 0;

	return pParser->GetLanguageCount();
}


uint32_t CaptionFilter::GetLanguageCode(uint8_t LanguageTag) const
{
	BlockLock Lock(m_FilterLock);

	if (const MMTCaptionAssetInfo *pAsset = GetMMTTargetAsset(); pAsset != nullptr) {
		if ((LanguageTag != 0) || !pAsset->HasSubtitleInfo)
			return 0;
		return pAsset->SubtitleInfo.LanguageCode;
	}

	const CaptionParser *pParser = GetCurrentCaptionParser();
	if (pParser == nullptr)
		return 0;

	return pParser->GetLanguageCodeByTag(LanguageTag);
}


void CaptionFilter::OnLanguageUpdate(CaptionParser *pParser)
{
	if (m_pCaptionHandler != nullptr)
		m_pCaptionHandler->OnLanguageUpdate(this, pParser);
}


void CaptionFilter::OnCaption(
	CaptionParser *pParser, uint8_t Language, const CharType *pText,
	const ARIBStringDecoder::FormatList *pFormatList)
{
	if (m_pCaptionHandler != nullptr)
		m_pCaptionHandler->OnCaption(this, pParser, Language, pText, pFormatList);
}


int CaptionFilter::GetServiceIndexByID(uint16_t ServiceID) const
{
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i].ServiceID == ServiceID)
			return static_cast<int>(i);
	}
	return -1;
}


CaptionParser * CaptionFilter::GetCurrentCaptionParser() const
{
	if (m_TargetESPID != PID_INVALID) {
		CaptionStream *pStream = m_PIDMapManager.GetMapTarget<CaptionStream>(m_TargetESPID);

		if (pStream != nullptr)
			return pStream->GetParser();
	}

	return nullptr;
}


void CaptionFilter::OnPATSection(const PSITableBase *pTable, const PSISection *pSection)
{
	const PATTable *pPATTable = dynamic_cast<const PATTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pPATTable == nullptr))
		return;

	// 現PMTのPIDをアンマップする
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		m_PIDMapManager.UnmapTarget(m_ServiceList[i].PMTPID);
		for (size_t j = 0; j < m_ServiceList[i].CaptionESList.size(); j++)
			m_PIDMapManager.UnmapTarget(m_ServiceList[i].CaptionESList[j].PID);
	}

	m_TargetESPID = PID_INVALID;

	m_ServiceList.resize(pPATTable->GetProgramCount());

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		const uint16_t PMTPID = pPATTable->GetPMTPID(static_cast<uint16_t>(i));

		m_ServiceList[i].ServiceID = pPATTable->GetProgramNumber(static_cast<uint16_t>(i));
		m_ServiceList[i].PMTPID = PMTPID;
		m_ServiceList[i].CaptionESList.clear();

		m_PIDMapManager.MapTarget(PMTPID, PSITableBase::CreateWithHandler<PMTTable>(&CaptionFilter::OnPMTSection, this));
	}
}


void CaptionFilter::OnPMTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	const PMTTable *pPMTTable = dynamic_cast<const PMTTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pPMTTable == nullptr))
		return;

	const int ServiceIndex = GetServiceIndexByID(pPMTTable->GetProgramNumberID());
	if (ServiceIndex < 0)
		return;

	ServiceInfo &Info = m_ServiceList[ServiceIndex];

	Info.CaptionESList.clear();

	for (int ESIndex = 0; ESIndex < pPMTTable->GetESCount(); ESIndex++) {
		const uint8_t StreamType = pPMTTable->GetStreamType(ESIndex);

		if (StreamType == STREAM_TYPE_CAPTION) {
			CaptionESInfo CaptionInfo;

			CaptionInfo.PID = pPMTTable->GetESPID(ESIndex);
			CaptionInfo.ComponentTag = 0xFF;

			const DescriptorBlock *pDescBlock = pPMTTable->GetItemDescriptorBlock(ESIndex);
			if (pDescBlock != nullptr) {
				const StreamIDDescriptor *pStreamIDDesc = pDescBlock->GetDescriptor<StreamIDDescriptor>();

				if (pStreamIDDesc != nullptr)
					CaptionInfo.ComponentTag = pStreamIDDesc->GetComponentTag();
			}

			Info.CaptionESList.push_back(CaptionInfo);

			CaptionStream *pStream = new CaptionStream(Is1SegPMTPID(Info.PMTPID));
			m_PIDMapManager.MapTarget(CaptionInfo.PID, pStream);
		}
	}

	SetTargetStream(m_TargetServiceID, m_TargetComponentTag);
}


void CaptionFilter::OnCaptionUnit(const MPUAssembler *pAssembler, const MPUAssembler::CaptionUnitInfo &Unit)
{
	if (m_pCaptionHandler == nullptr)
		return;

	const MMTCaptionAssetInfo *pAsset = GetMMTTargetAsset();
	if ((pAsset == nullptr) || (pAsset->PacketID != Unit.PacketID))
		return;

	CheckMMTLanguageUpdate();

	TTMLCaptionInfo Info;

	Info.PacketID = Unit.PacketID;
	Info.ComponentTag = pAsset->ComponentTag;
	if ((pAsset->ComponentTag >= MMT_COMPONENT_TAG_SUPERIMPOSE_FIRST)
			&& (pAsset->ComponentTag <= MMT_COMPONENT_TAG_SUPERIMPOSE_LAST))
		Info.Superimpose = true;
	else if ((pAsset->ComponentTag >= MMT_COMPONENT_TAG_CAPTION_FIRST)
			&& (pAsset->ComponentTag <= MMT_COMPONENT_TAG_CAPTION_LAST))
		Info.Superimpose = false;
	else
		Info.Superimpose =
			pAsset->HasSubtitleInfo
			&& (pAsset->SubtitleInfo.Type == MHDataComponentDescriptor::SUBTITLE_TYPE_SUPERIMPOSE);
	Info.HasSubtitleInfo = pAsset->HasSubtitleInfo;
	Info.SubtitleInfo = pAsset->SubtitleInfo;
	Info.MPUSequenceNumber = Unit.MPUSequenceNumber;
	Info.HasPresentationTime = Unit.HasPresentationTime;
	Info.PresentationTime = Unit.PresentationTime;
	Info.Discontinuity = Unit.Discontinuity;
	Info.pDocument = nullptr;
	Info.DocumentSize = 0;
	// TTML 文書は MPU の最初の MFU (ARIB STD-B60 9.2.1)
	if (!Unit.SubsampleList.empty() && (Unit.SubsampleList.front().DataType == MMT_CAPTION_DATA_TYPE_TTML)) {
		Info.pDocument = Unit.SubsampleList.front().pData;
		Info.DocumentSize = Unit.SubsampleList.front().DataSize;
	}
	Info.pSubsampleList = &Unit.SubsampleList;

	m_pCaptionHandler->OnTTMLCaption(this, Info);
}


int CaptionFilter::GetMMTServiceIndexByID(uint16_t ServiceID) const
{
	for (size_t i = 0; i < m_MMTServiceList.size(); i++) {
		if (m_MMTServiceList[i].ServiceID == ServiceID)
			return static_cast<int>(i);
	}
	return -1;
}


const CaptionFilter::MMTCaptionAssetInfo * CaptionFilter::GetMMTTargetAsset() const
{
	if (m_TargetPacketID == MMT_PACKET_ID_INVALID)
		return nullptr;

	for (const MMTServiceInfo &Service : m_MMTServiceList) {
		for (const MMTCaptionAssetInfo &Asset : Service.CaptionAssetList) {
			if (Asset.PacketID == m_TargetPacketID)
				return &Asset;
		}
	}

	return nullptr;
}


void CaptionFilter::SelectMMTTargetAsset()
{
	if (m_TargetPacketID != MMT_PACKET_ID_INVALID) {
		MPUAssemblerTarget *pTarget = m_MMTPacketIDMapManager.GetMapTarget<MPUAssemblerTarget>(m_TargetPacketID);
		if (pTarget != nullptr)
			pTarget->GetAssembler().SetHandler(nullptr);
		m_TargetPacketID = MMT_PACKET_ID_INVALID;
	}

	const int Index = GetMMTServiceIndexByID(m_TargetServiceID);
	if (Index < 0)
		return;

	for (const MMTCaptionAssetInfo &Asset : m_MMTServiceList[Index].CaptionAssetList) {
		if ((m_TargetComponentTag == 0xFF) || (Asset.ComponentTag == m_TargetComponentTag)) {
			MPUAssemblerTarget *pTarget = m_MMTPacketIDMapManager.GetMapTarget<MPUAssemblerTarget>(Asset.PacketID);
			if (pTarget != nullptr) {
				pTarget->GetAssembler().SetHandler(this);
				m_TargetPacketID = Asset.PacketID;
			}
			break;
		}
	}
}


void CaptionFilter::CheckMMTLanguageUpdate()
{
	const MMTCaptionAssetInfo *pAsset = GetMMTTargetAsset();
	const uint16_t PacketID = (pAsset != nullptr) ? pAsset->PacketID : MMT_PACKET_ID_INVALID;
	const uint32_t LanguageCode =
		((pAsset != nullptr) && pAsset->HasSubtitleInfo) ? pAsset->SubtitleInfo.LanguageCode : LANGUAGE_CODE_INVALID;

	if ((PacketID != m_NotifiedPacketID) || (LanguageCode != m_NotifiedLanguageCode)) {
		m_NotifiedPacketID = PacketID;
		m_NotifiedLanguageCode = LanguageCode;
		if ((PacketID != MMT_PACKET_ID_INVALID) && (m_pCaptionHandler != nullptr))
			m_pCaptionHandler->OnLanguageUpdate(this, nullptr);
	}
}


void CaptionFilter::OnPLTTable(const MMTSingleTable *pTable)
{
	const PLTable *pPLTable = dynamic_cast<const PLTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pPLTable == nullptr))
		return;

	for (const MMTServiceInfo &Service : m_MMTServiceList) {
		if ((Service.MPTPacketID != MMT_PACKET_ID_INVALID) && (Service.MPTPacketID != MMT_PACKET_ID_PA))
			m_MMTPacketIDMapManager.UnmapTarget(Service.MPTPacketID);
		for (const MMTCaptionAssetInfo &Asset : Service.CaptionAssetList)
			m_MMTPacketIDMapManager.UnmapTarget(Asset.PacketID);
	}

	m_TargetPacketID = MMT_PACKET_ID_INVALID;

	m_MMTServiceList.resize(pPLTable->GetPackageCount());

	for (int i = 0; i < pPLTable->GetPackageCount(); i++) {
		MMTServiceInfo &Service = m_MMTServiceList[i];

		Service.ServiceID = pPLTable->GetServiceID(i);
		Service.MPTPacketID = pPLTable->GetMPTPacketID(i);
		Service.CaptionAssetList.clear();

		// 処理中の PLT を破棄しないよう、PLT の packet_id には割り当てない
		if ((Service.MPTPacketID != MMT_PACKET_ID_INVALID) && (Service.MPTPacketID != MMT_PACKET_ID_PA)) {
			m_MMTPacketIDMapManager.MapTarget(
				Service.MPTPacketID,
				MMTSingleTable::CreateWithHandler<MPTable>(&CaptionFilter::OnMPTTable, this));
		}
	}
}


void CaptionFilter::OnMPTTable(const MMTSingleTable *pTable)
{
	const MPTable *pMPTable = dynamic_cast<const MPTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pMPTable == nullptr))
		return;

	const int ServiceIndex = GetMMTServiceIndexByID(pMPTable->GetServiceID());
	if (ServiceIndex < 0)
		return;

	MMTServiceInfo &Service = m_MMTServiceList[ServiceIndex];
	std::vector<MMTCaptionAssetInfo> AssetList;

	// MPT はタイムスタンプ記述子が更新される度に内容が変わるので、その都度 MPU の時刻を更新する
	for (int i = 0; i < pMPTable->GetAssetCount(); i++) {
		if (pMPTable->GetAssetType(i) != MMT_ASSET_TYPE_STPP)
			continue;

		const uint16_t PacketID = pMPTable->GetAssetPacketID(i);
		if ((PacketID == MMT_PACKET_ID_INVALID) || (PacketID == MMT_PACKET_ID_PA))
			continue;

		MPUAssemblerTarget *pTarget = m_MMTPacketIDMapManager.GetMapTarget<MPUAssemblerTarget>(PacketID);
		if (pTarget == nullptr) {
			if (m_MMTPacketIDMapManager.GetMapTarget(PacketID) != nullptr)
				continue;
			pTarget = new MPUAssemblerTarget(MPUAssembler::MediaType::Caption);
			m_MMTPacketIDMapManager.MapTarget(PacketID, pTarget);
		}

		const MMTDescriptorBlock *pDescBlock = pMPTable->GetAssetDescriptorBlock(i);
		const MPTable::AssetInfo *pAssetInfo = pMPTable->GetAssetInfo(i);
		pTarget->GetAssembler().UpdateTimestamps(
			pDescBlock, pAssetInfo->AssetTimescaleFlag ? pAssetInfo->AssetTimescale : 0);

		MMTCaptionAssetInfo &Asset = AssetList.emplace_back();
		Asset.PacketID = PacketID;
		Asset.ComponentTag = pMPTable->GetAssetComponentTag(i);
		Asset.HasSubtitleInfo = false;
		Asset.SubtitleInfo = {};
		pDescBlock->EnumDescriptors<MHDataComponentDescriptor>(
			[&](const MHDataComponentDescriptor *pDesc) {
				if (!Asset.HasSubtitleInfo)
					Asset.HasSubtitleInfo = pDesc->GetAribSubtitleInfo(&Asset.SubtitleInfo);
			});
	}

	std::ranges::stable_sort(
		AssetList,
		[](const MMTCaptionAssetInfo &Asset1, const MMTCaptionAssetInfo &Asset2) {
			return Asset1.ComponentTag < Asset2.ComponentTag;
		});

	bool Changed = AssetList.size() != Service.CaptionAssetList.size();

	for (size_t i = 0; i < Service.CaptionAssetList.size(); i++) {
		const MMTCaptionAssetInfo &OldAsset = Service.CaptionAssetList[i];
		if (!Changed
				&& ((OldAsset.PacketID != AssetList[i].PacketID)
					|| (OldAsset.ComponentTag != AssetList[i].ComponentTag)))
			Changed = true;
		if (std::ranges::find(AssetList, OldAsset.PacketID, &MMTCaptionAssetInfo::PacketID) == AssetList.end())
			m_MMTPacketIDMapManager.UnmapTarget(OldAsset.PacketID);
	}

	Service.CaptionAssetList = std::move(AssetList);

	if (Changed)
		SelectMMTTargetAsset();

	CheckMMTLanguageUpdate();
}


} // namespace LibISDB
