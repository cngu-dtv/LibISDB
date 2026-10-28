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
 @file   TLVSIDescriptors.hpp
 @brief  TLV-SI 記述子
 @author cngu-dtv
*/


#ifndef LIBISDB_TLV_SI_DESCRIPTORS_H
#define LIBISDB_TLV_SI_DESCRIPTORS_H


#include "../TS/DescriptorBlock.hpp"
#include "../TS/Descriptors.hpp"
#include <vector>


namespace LibISDB
{

	/** リモコンキー記述子クラス (ARIB STD-B60 5.3.1.5) */
	class RemoteControlKeyDescriptor
		: public DescriptorTemplate<RemoteControlKeyDescriptor, 0xCD>
	{
	public:
		struct KeyInfo {
			uint8_t RemoteControlKeyID; /**< remote_control_key_id */
			uint16_t ServiceID;         /**< service_id */
		};

	// DescriptorBase
		void Reset() noexcept override;

	// RemoteControlKeyDescriptor
		int GetKeyCount() const noexcept { return static_cast<int>(m_KeyList.size()); }
		bool GetKeyInfo(int Index, ReturnArg<KeyInfo> Info) const;
		// 割り当てが無い場合は 0
		uint8_t GetRemoteControlKeyID(uint16_t ServiceID) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		std::vector<KeyInfo> m_KeyList;
	};

	/**
		TLV-SI 記述子ブロッククラス

		0x40 / 0x41 / 0x43 / 0xFE は TS と同じ形式なので TS の記述子クラスを使う。
		0xCD は TS とは意味が違うためリモコンキー記述子として解析する。
	*/
	class TLVSIDescriptorBlock
		: public DescriptorBlock
	{
	public:
		// 内容が不正な記述子は読み飛ばし、記述子の範囲がデータに収まらなくなった所で終わる
		int ParseBlock(const uint8_t *pData, size_t DataLength);

	private:
		static DescriptorBase * CreateDescriptorInstance(uint8_t Tag);
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_TLV_SI_DESCRIPTORS_H
