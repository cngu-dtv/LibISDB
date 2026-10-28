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
 @file   MMTTableBase.hpp
 @brief  MMT-SI テーブルの基底
 @author cngu-dtv
*/


#ifndef LIBISDB_MMT_TABLE_BASE_H
#define LIBISDB_MMT_TABLE_BASE_H


#include "MMTSignaling.hpp"
#include "MMTPacketIDMap.hpp"
#include "../TS/PSITable.hpp"
#include <memory>
#include <functional>


namespace LibISDB
{

	/** MMT-SI テーブル基底クラス (packet_id 毎にマップする) */
	class MMTTableBase
		: public MMTPacketIDMapTarget
		, protected MMTSignalingParser::SignalingHandler
	{
	public:
		MMTTableBase();
		virtual ~MMTTableBase() = default;

		virtual void Reset();

		unsigned long GetCRCErrorCount() const noexcept;

	// MMTPacketIDMapTarget
		bool StorePacket(const TLVPacket *pPacket) override;
		void OnPacketIDUnmapped(uint16_t PacketID) override;

	protected:
		MMTSignalingParser m_SignalingParser;
	};

	/** MMT-SI 単独テーブルクラス (PA メッセージ / CA メッセージで伝送されるテーブル) */
	class MMTSingleTable
		: public MMTTableBase
	{
	public:
		typedef std::function<void(const MMTSingleTable *pTable)> TableHandler;

		template<typename TTable, typename THandler> static TTable * CreateWithHandler(
			void (THandler::*pFunc)(const MMTSingleTable *), THandler *pObject)
		{
			TTable *pTable = new TTable;
			pTable->SetTableHandler(std::bind(pFunc, pObject, std::placeholders::_1));
			return pTable;
		}

		MMTSingleTable(uint8_t TableID);

	// MMTTableBase
		void Reset() override;

	// MMTSingleTable
		uint8_t GetTableID() const noexcept { return m_TableID; }
		uint8_t GetVersion() const noexcept { return m_CurTable.GetVersion(); }
		bool IsTableAcquired() const noexcept { return m_CurTable.GetSize() > 0; }
		void SetTableHandler(const TableHandler &Handler);

	protected:
	// MMTSignalingParser::SignalingHandler
		void OnMMTTable(const MMTSignalingParser *pParser, const MMTTable *pTable) override;

		// 内容が変わった時に呼ばれる
		virtual bool OnTableUpdate(const MMTTable *pCurTable, const MMTTable *pOldTable);

		uint8_t m_TableID;
		MMTTable m_CurTable;
		TableHandler m_TableHandler;
	};

	/** MMT-SI セクションテーブルクラス (M2 セクションメッセージ / M2 短セクションメッセージで伝送されるテーブル) */
	class MMTSectionTable
		: public MMTTableBase
	{
	public:
		template<typename TTable, typename THandler> static MMTSectionTable * CreateWithHandler(
			void (THandler::*pFunc)(const PSITableBase *, const PSISection *), THandler *pObject)
		{
			return new MMTSectionTable(PSITableBase::CreateWithHandler<TTable>(pFunc, pObject));
		}

		// pTable の所有権を受け取る
		MMTSectionTable(PSITableBase *pTable);

	// MMTTableBase
		void Reset() override;

	// MMTSectionTable
		PSITableBase * GetTable() noexcept { return m_Table.get(); }
		const PSITableBase * GetTable() const noexcept { return m_Table.get(); }
		template<typename T> const T * GetTable() const { return dynamic_cast<const T *>(m_Table.get()); }

	protected:
	// MMTSignalingParser::SignalingHandler
		void OnPSISection(const MMTSignalingParser *pParser, const PSISection *pSection) override;

		std::unique_ptr<PSITableBase> m_Table;
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_MMT_TABLE_BASE_H
