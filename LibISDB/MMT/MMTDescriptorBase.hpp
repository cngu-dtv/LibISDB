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
 @file   MMTDescriptorBase.hpp
 @brief  MMT-SI 記述子の基底
 @author cngu-dtv
*/


#ifndef LIBISDB_MMT_DESCRIPTOR_BASE_H
#define LIBISDB_MMT_DESCRIPTOR_BASE_H


namespace LibISDB
{

	/** MMT-SI 記述子基底クラス */
	class MMTDescriptorBase
	{
	public:
		MMTDescriptorBase() noexcept;
		virtual ~MMTDescriptorBase() = default;

		virtual void CopyFrom(const MMTDescriptorBase *pSrc);
		virtual MMTDescriptorBase * Clone() const;

		// pData は descriptor_tag の位置。記述子の範囲が分かれば内容が不正でも GetSize() は有効になる
		bool Parse(const uint8_t *pData, size_t DataLength);

		bool IsValid() const noexcept { return m_IsValid; }
		uint16_t GetTag() const noexcept { return m_Tag; }
		uint32_t GetLength() const noexcept { return m_Length; }
		size_t GetSize() const noexcept { return m_Size; }

		virtual void Reset() noexcept;

		// descriptor_length のバイト数 (ARIB STD-B60 表 4-10)
		static size_t GetLengthFieldSize(uint16_t Tag) noexcept;

	protected:
		virtual bool StoreContents(const uint8_t *pPayload);

		uint16_t m_Tag;    /**< descriptor_tag */
		uint32_t m_Length; /**< descriptor_length */
		size_t m_Size;     /**< 記述子全体のサイズ */
		bool m_IsValid;    /**< 解析結果 */
	};

	/** MMT-SI 記述子テンプレートクラス */
	template<typename T, uint16_t Tag> class MMTDescriptorTemplate
		: public MMTDescriptorBase
	{
	public:
		static constexpr uint16_t TAG = Tag;

		void CopyFrom(const MMTDescriptorBase *pSrc) override
		{
			if (pSrc != this) {
				const T *pSrcDescriptor = dynamic_cast<const T *>(pSrc);

				if (pSrcDescriptor != nullptr) {
					*static_cast<T *>(this) = *pSrcDescriptor;
				} else {
					MMTDescriptorBase::CopyFrom(pSrc);
				}
			}
		}

		MMTDescriptorBase * Clone() const override
		{
			return new T(*static_cast<const T *>(this));
		}
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_MMT_DESCRIPTOR_BASE_H
