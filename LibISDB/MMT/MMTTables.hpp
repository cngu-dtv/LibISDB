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
 @file   MMTTables.hpp
 @brief  MMT-SI テーブル
 @author cngu-dtv
*/


#ifndef LIBISDB_MMT_TABLES_H
#define LIBISDB_MMT_TABLES_H


#include "MMTTableBase.hpp"
#include "MMTDescriptorBlock.hpp"
#include "MMTDescriptors.hpp"
#include "MMTGeneralLocationInfo.hpp"
#include <vector>


namespace LibISDB
{

	/** PLT クラス (ARIB STD-B60 7.3.3.2) */
	class PLTable
		: public MMTSingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID = MMT_TABLE_ID_PLT;

		struct PackageInfo {
			uint64_t PackageID;                  /**< MMT_package_id_byte (末尾 8 バイトまで) */
			MMTGeneralLocationInfo LocationInfo; /**< PA メッセージの位置 */
		};

		PLTable();

	// MMTSingleTable
		void Reset() override;

	// PLTable
		int GetPackageCount() const;
		const PackageInfo * GetPackageInfo(int Index) const;
		// package_id の下位 16 ビット (service_id)
		uint16_t GetServiceID(int Index) const;
		// MPT の PA メッセージが同じ IP データフローに無い場合は MMT_PACKET_ID_INVALID
		uint16_t GetMPTPacketID(int Index) const;

	protected:
		bool OnTableUpdate(const MMTTable *pCurTable, const MMTTable *pOldTable) override;

		std::vector<PackageInfo> m_PackageList;
	};

	/** MPT クラス (ARIB STD-B60 7.3.3.1) */
	class MPTable
		: public MMTSingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID = MMT_TABLE_ID_MPT;

		struct AssetInfo {
			uint8_t IdentifierType;                          /**< identifier_type */
			uint32_t AssetIDScheme;                          /**< asset_id_scheme */
			ARIBString AssetID;                              /**< asset_id_byte */
			uint32_t AssetType;                              /**< asset_type */
			bool AssetClockRelationFlag;                     /**< asset_clock_relation_flag */
			uint8_t AssetClockRelationID;                    /**< asset_clock_relation_id */
			bool AssetTimescaleFlag;                         /**< asset_timescale_flag */
			uint32_t AssetTimescale;                         /**< asset_timescale */
			std::vector<MMTGeneralLocationInfo> LocationList; /**< MMT_general_location_info */
			MMTDescriptorBlock Descriptors;                  /**< asset_descriptors */
		};

		MPTable();

	// MMTSingleTable
		void Reset() override;

	// MPTable
		uint8_t GetMPTMode() const noexcept { return m_MPTMode; }
		uint64_t GetPackageID() const noexcept { return m_PackageID; }
		// package_id の下位 16 ビット (service_id)
		uint16_t GetServiceID() const noexcept { return static_cast<uint16_t>(m_PackageID & 0xFFFF); }
		const MMTDescriptorBlock * GetMPTDescriptorBlock() const noexcept { return &m_DescriptorBlock; }
		int GetAssetCount() const;
		const AssetInfo * GetAssetInfo(int Index) const;
		uint32_t GetAssetType(int Index) const;
		// 最初の location_type 0x00 の packet_id
		uint16_t GetAssetPacketID(int Index) const;
		// MH-ストリーム識別記述子の component_tag
		uint16_t GetAssetComponentTag(int Index) const;
		const MMTDescriptorBlock * GetAssetDescriptorBlock(int Index) const;

	protected:
		bool OnTableUpdate(const MMTTable *pCurTable, const MMTTable *pOldTable) override;

		uint8_t m_MPTMode;                    /**< MPT_mode */
		uint64_t m_PackageID;                 /**< MMT_package_id_byte (末尾 8 バイトまで) */
		MMTDescriptorBlock m_DescriptorBlock; /**< MPT_descriptors */
		std::vector<AssetInfo> m_AssetList;
	};

	/** CAT (MH) クラス (ARIB STD-B60 7.3.3.8) */
	class MHCATTable
		: public MMTSingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID = MMT_TABLE_ID_CAT;

		MHCATTable();

	// MMTSingleTable
		void Reset() override;

	// MHCATTable
		const MMTDescriptorBlock * GetDescriptorBlock() const noexcept { return &m_DescriptorBlock; }

	protected:
		bool OnTableUpdate(const MMTTable *pCurTable, const MMTTable *pOldTable) override;

		MMTDescriptorBlock m_DescriptorBlock;
	};

	/** MH-SDT クラス (ARIB STD-B60 7.3.3.13) */
	class MHSDTTable
		: public PSISingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID_ACTUAL = MMT_TABLE_ID_MH_SDT_ACTUAL;
		static constexpr uint8_t TABLE_ID_OTHER  = MMT_TABLE_ID_MH_SDT_OTHER;

		MHSDTTable(uint8_t TableID = TABLE_ID_ACTUAL);

	// PSISingleTable
		void Reset() override;

	// MHSDTTable
		uint8_t GetTableID() const noexcept { return m_TableID; }
		uint16_t GetTLVStreamID() const;
		uint16_t GetOriginalNetworkID() const noexcept { return m_OriginalNetworkID; }
		int GetServiceCount() const;
		int GetServiceIndexByID(uint16_t ServiceID) const;
		uint16_t GetServiceID(int Index) const;
		uint8_t GetEITUserDefinedFlags(int Index) const;
		bool GetEITScheduleFlag(int Index) const;
		bool GetEITPresentFollowingFlag(int Index) const;
		uint8_t GetRunningStatus(int Index) const;
		bool GetFreeCAMode(int Index) const;
		const MMTDescriptorBlock * GetItemDescriptorBlock(int Index) const;

	protected:
		bool OnPSISection(const PSISectionParser *pSectionParser, const PSISection *pSection) override;
		bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection) override;

		struct ServiceInfo {
			uint16_t ServiceID;             /**< service_id */
			uint8_t EITUserDefinedFlags;    /**< EIT_user_defined_flags */
			bool EITScheduleFlag;           /**< EIT_schedule_flag */
			bool EITPresentFollowingFlag;   /**< EIT_present_following_flag */
			uint8_t RunningStatus;          /**< running_status */
			bool FreeCAMode;                /**< free_CA_mode */
			MMTDescriptorBlock Descriptors; /**< 記述子 */
		};

		uint8_t m_TableID;
		uint16_t m_OriginalNetworkID;
		std::vector<ServiceInfo> m_ServiceList;
	};

	/** MH-SDT (他ストリーム) クラス */
	class MHSDTOtherTable
		: public PSITable
	{
	protected:
	// PSITable
		PSITableBase * CreateSectionTable(const PSISection *pSection) override;
	};

	/** MH-SDT 集合クラス */
	class MHSDTTableSet
		: public PSITableSet
	{
	public:
		MHSDTTableSet();

		const MHSDTTable * GetActualSDTTable() const;
		const MHSDTOtherTable * GetOtherSDTTable() const;
	};

	/** MH-EIT クラス (ARIB STD-B60 7.3.3.9) */
	class MHEITTable
		: public PSISingleTable
	{
	public:
		struct EventInfo {
			uint16_t EventID;               /**< event_id */
			DateTime StartTime;             /**< start_time */
			uint32_t Duration;              /**< duration */
			uint8_t RunningStatus;          /**< running_status */
			bool FreeCAMode;                /**< free_CA_mode */
			MMTDescriptorBlock Descriptors; /**< 記述子 */
		};

		MHEITTable();

	// PSISingleTable
		void Reset() override;

	// MHEITTable
		uint16_t GetServiceID() const noexcept { return m_ServiceID; }
		uint16_t GetTLVStreamID() const noexcept { return m_TLVStreamID; }
		uint16_t GetOriginalNetworkID() const noexcept { return m_OriginalNetworkID; }
		uint8_t GetSegmentLastSectionNumber() const noexcept { return m_SegmentLastSectionNumber; }
		uint8_t GetLastTableID() const noexcept { return m_LastTableID; }
		int GetEventCount() const;
		const EventInfo * GetEventInfo(int Index = 0) const;
		uint16_t GetEventID(int Index = 0) const;
		const DateTime * GetStartTime(int Index = 0) const;
		uint32_t GetDuration(int Index = 0) const;
		uint8_t GetRunningStatus(int Index = 0) const;
		bool GetFreeCAMode(int Index = 0) const;
		const MMTDescriptorBlock * GetItemDescriptorBlock(int Index = 0) const;

	protected:
		bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection) override;

		uint16_t m_ServiceID;               /**< service_id */
		uint16_t m_TLVStreamID;             /**< tlv_stream_id */
		uint16_t m_OriginalNetworkID;       /**< original_network_id */
		uint8_t m_SegmentLastSectionNumber; /**< segment_last_section_number */
		uint8_t m_LastTableID;              /**< last_table_id */
		std::vector<EventInfo> m_EventList;
	};

	/** MH-EIT 集合クラス */
	class MHEITMultiTable
		: public PSITable
	{
	public:
		const MHEITTable * GetEITTableByServiceID(uint16_t ServiceID, uint16_t SectionNumber) const;
		uint16_t GetServiceID(int Index) const;

		static unsigned long long MakeTableUniqueID(
			uint16_t NetworkID, uint16_t TLVStreamID, uint16_t ServiceID);

	protected:
	// PSITable
		PSITableBase * CreateSectionTable(const PSISection *pSection) override;
		unsigned long long GetSectionTableUniqueID(const PSISection *pSection) override;
	};

	/** MH-EIT[p/f] クラス */
	class MHEITPfTable
		: public PSITableSet
	{
	public:
		MHEITPfTable();

		const MHEITMultiTable * GetPfTable() const;
		const MHEITTable * GetPfTable(uint16_t ServiceID, bool Following = false) const;
	};

	/** MH-BIT クラス (ARIB STD-B60 7.3.3.11) */
	class MHBITTable
		: public PSISingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID = MMT_TABLE_ID_MH_BIT;

		MHBITTable();

	// PSISingleTable
		void Reset() override;

	// MHBITTable
		uint16_t GetOriginalNetworkID() const noexcept { return m_OriginalNetworkID; }
		bool GetBroadcastViewPropriety() const noexcept { return m_BroadcastViewPropriety; }
		const MMTDescriptorBlock * GetBITDescriptorBlock() const noexcept { return &m_DescriptorBlock; }
		int GetBroadcasterCount() const;
		uint8_t GetBroadcasterID(int Index) const;
		const MMTDescriptorBlock * GetBroadcasterDescriptorBlock(int Index) const;

	protected:
		bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection) override;

		struct BroadcasterInfo {
			uint8_t BroadcasterID;          /**< broadcaster_id */
			MMTDescriptorBlock Descriptors; /**< 記述子 */
		};

		uint16_t m_OriginalNetworkID;         /**< original_network_id */
		bool m_BroadcastViewPropriety;        /**< broadcast_view_propriety */
		MMTDescriptorBlock m_DescriptorBlock; /**< 記述子 */
		std::vector<BroadcasterInfo> m_BroadcasterList;
	};

	/** MH-BIT 集合クラス */
	class MHBITMultiTable
		: public PSITable
	{
	public:
		uint16_t GetBITSectionCount() const;
		const MHBITTable * GetBITTable(uint16_t SectionNumber) const;
		bool IsBITComplete() const;

	protected:
	// PSITable
		PSITableBase * CreateSectionTable(const PSISection *pSection) override;
	};

	/** MH-TOT クラス (ARIB STD-B60 7.3.3.14) */
	class MHTOTTable
		: public PSISingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID = MMT_TABLE_ID_MH_TOT;

		MHTOTTable();

	// PSISingleTable
		void Reset() override;

	// MHTOTTable
		bool GetDateTime(ReturnArg<DateTime> Time) const;
		bool GetOffsetDateTime(ReturnArg<DateTime> Time, uint32_t CountryCode, uint8_t CountryRegionID = 0) const;
		int GetLocalTimeOffset(uint32_t CountryCode, uint8_t CountryRegionID = 0) const;
		const MMTDescriptorBlock * GetTOTDescriptorBlock() const noexcept { return &m_DescriptorBlock; }

	protected:
		bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection) override;

		DateTime m_DateTime;                  /**< JST_time */
		MMTDescriptorBlock m_DescriptorBlock; /**< 記述子 */
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_MMT_TABLES_H
