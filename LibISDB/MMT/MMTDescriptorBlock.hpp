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
 @file   MMTDescriptorBlock.hpp
 @brief  MMT-SI 記述子ブロック
 @author cngu-dtv
*/


#ifndef LIBISDB_MMT_DESCRIPTOR_BLOCK_H
#define LIBISDB_MMT_DESCRIPTOR_BLOCK_H


#include "MMTDescriptorBase.hpp"
#include <vector>
#include <memory>


namespace LibISDB
{

	/** MMT-SI 記述子ブロッククラス */
	class MMTDescriptorBlock
	{
	public:
		MMTDescriptorBlock() = default;
		MMTDescriptorBlock(const MMTDescriptorBlock &Src);
		MMTDescriptorBlock(MMTDescriptorBlock &&Src) noexcept;
		virtual ~MMTDescriptorBlock() = default;
		MMTDescriptorBlock & operator = (const MMTDescriptorBlock &Src);
		MMTDescriptorBlock & operator = (MMTDescriptorBlock &&Src) noexcept;

		// 内容が不正な記述子は読み飛ばし、記述子の範囲がデータに収まらなくなった所で終わる
		int ParseBlock(const uint8_t *pData, size_t DataLength);

		virtual void Reset() noexcept;

		int GetDescriptorCount() const;
		const MMTDescriptorBase * GetDescriptorByIndex(int Index) const;
		const MMTDescriptorBase * GetDescriptorByTag(uint16_t Tag) const;
		template<typename T> const T * GetDescriptor() const {
			return dynamic_cast<const T *>(GetDescriptorByTag(T::TAG));
		}

		template<typename TDesc, typename TPred> void EnumDescriptors(TPred Pred) const
		{
			for (auto &e : m_DescriptorList) {
				if (e->GetTag() == TDesc::TAG) {
					const TDesc *pDesc = dynamic_cast<const TDesc *>(e.get());
					if (pDesc != nullptr)
						Pred(pDesc);
				}
			}
		}

	protected:
		static MMTDescriptorBase * CreateDescriptorInstance(uint16_t Tag);

		std::vector<std::unique_ptr<MMTDescriptorBase>> m_DescriptorList;
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_MMT_DESCRIPTOR_BLOCK_H
