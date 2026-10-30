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
 @file   MMTPacketIDMap.hpp
 @brief  MMTP パケットの packet_id マップ
 @author cngu-dtv
*/


#ifndef LIBISDB_MMT_PACKET_ID_MAP_H
#define LIBISDB_MMT_PACKET_ID_MAP_H


#include "../TLV/TLVPacket.hpp"
#include "../Base/DataStream.hpp"
#include <map>


namespace LibISDB
{

	/** packet_id マップ対象クラス */
	class MMTPacketIDMapTarget
	{
	public:
		virtual ~MMTPacketIDMapTarget() = default;

		virtual bool StorePacket(const TLVPacket *pPacket) = 0;
		virtual void OnPacketIDMapped(uint16_t PacketID) {}
		virtual void OnPacketIDUnmapped(uint16_t PacketID) {}
	};

	/**
		packet_id マップ管理クラス

		外した対象は OnPacketIDUnmapped() で即座に破棄される (テーブルは自身を delete する)。
		StorePacket() の処理中に、処理中の対象の packet_id を UnmapTarget() / MapTarget() で
		外してはならない。
	*/
	class MMTPacketIDMapManager
	{
	public:
		~MMTPacketIDMapManager();

		bool StorePacket(const TLVPacket *pPacket);
		// 1 つのストリームには同じ packet_id の MMTP パケットだけが含まれていること
		bool StorePacketStream(DataStream *pPacketStream);

		// 既に割り当てがあればそれを外してから割り当てる
		bool MapTarget(uint16_t PacketID, MMTPacketIDMapTarget *pMapTarget);
		bool UnmapTarget(uint16_t PacketID);
		void UnmapAllTargets();

		MMTPacketIDMapTarget * GetMapTarget(uint16_t PacketID) const;
		template<typename T> T * GetMapTarget(uint16_t PacketID) const { return dynamic_cast<T *>(GetMapTarget(PacketID)); }
		size_t GetMapCount() const noexcept { return m_PacketIDMap.size(); }

	protected:
		std::map<uint16_t, MMTPacketIDMapTarget *> m_PacketIDMap;
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_MMT_PACKET_ID_MAP_H
