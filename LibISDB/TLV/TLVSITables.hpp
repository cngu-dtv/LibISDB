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
 @file   TLVSITables.hpp
 @brief  TLV-SI テーブル
 @author cngu-dtv
*/


#ifndef LIBISDB_TLV_SI_TABLES_H
#define LIBISDB_TLV_SI_TABLES_H


#include "TLVPacket.hpp"
#include "TLVSIDescriptors.hpp"
#include "../TS/PSITable.hpp"
#include <vector>
#include <array>


namespace LibISDB
{

	/** TLV-NIT クラス (ARIB STD-B60 5.2.1.1) */
	class TLVNITTable
		: public PSISingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID = TLV_TABLE_ID_NIT_ACTUAL;

		TLVNITTable();

	// PSISingleTable
		void Reset() override;

	// TLVNITTable
		uint16_t GetNetworkID() const noexcept { return m_NetworkID; }
		const DescriptorBlock * GetNetworkDescriptorBlock() const noexcept { return &m_NetworkDescriptorBlock; }
		bool GetNetworkName(ReturnArg<String> Name) const;
		int GetTLVStreamCount() const;
		uint16_t GetTLVStreamID(int Index) const;
		uint16_t GetOriginalNetworkID(int Index) const;
		const DescriptorBlock * GetItemDescriptorBlock(int Index) const;

	protected:
		bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection) override;

		struct TLVStreamInfo {
			uint16_t TLVStreamID;             /**< tlv_stream_id */
			uint16_t OriginalNetworkID;       /**< original_network_id */
			TLVSIDescriptorBlock Descriptors; /**< 記述子 */
		};

		uint16_t m_NetworkID;                          /**< network_id */
		TLVSIDescriptorBlock m_NetworkDescriptorBlock; /**< ネットワーク記述子 */
		std::vector<TLVStreamInfo> m_TLVStreamList;
	};

	/** TLV-NIT 集合クラス */
	class TLVNITMultiTable
		: public PSITable
	{
	public:
		uint16_t GetNITSectionCount() const;
		const TLVNITTable * GetNITTable(uint16_t SectionNumber) const;
		bool IsNITComplete() const;

	protected:
	// PSITable
		PSITableBase * CreateSectionTable(const PSISection *pSection) override;
	};

	/** AMT クラス (ARIB STD-B60 5.2.1.2) */
	class AMTTable
		: public PSISingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID = TLV_TABLE_ID_AMT;

		struct ServiceInfo {
			uint16_t ServiceID;                         /**< service_id */
			bool IsIPv6;                                /**< ip_version */
			std::array<uint8_t, 16> SourceAddress;      /**< src_address (IPv4 は先頭 4 バイト) */
			uint8_t SourceAddressMask;                  /**< src_address_mask */
			std::array<uint8_t, 16> DestinationAddress; /**< dst_address (IPv4 は先頭 4 バイト) */
			uint8_t DestinationAddressMask;             /**< dst_address_mask */
		};

		AMTTable();

	// PSISingleTable
		void Reset() override;

	// AMTTable
		int GetServiceCount() const;
		const ServiceInfo * GetServiceInfo(int Index) const;

	protected:
		bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection) override;

		std::vector<ServiceInfo> m_ServiceList;
	};

	/** TLV-SI テーブル集合クラス */
	class TLVSITableSet
		: public PSITableSet
	{
	public:
		TLVSITableSet();

		const TLVNITMultiTable * GetNITTable() const;
		const AMTTable * GetAMTTable() const;
	};

	/** TLV-SI 解析クラス */
	class TLVSIParser
	{
	public:
		TLVSIParser(PSISectionParser::PSISectionHandler *pHandler);

		// TLV-SI 以外のパケットは無視する
		void StorePacket(const TLVPacket *pPacket);
		void Reset() noexcept;

		unsigned long GetCRCErrorCount() const noexcept { return m_CRCErrorCount; }

	private:
		PSISectionParser::PSISectionHandler *m_pHandler;
		PSISection m_Section;
		unsigned long m_CRCErrorCount;
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_TLV_SI_TABLES_H
