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
 @file   MMTMediaReceiver.hpp
 @brief  MMT の映像・音声の受信
 @author cngu-dtv
*/


#ifndef LIBISDB_MMT_MEDIA_RECEIVER_H
#define LIBISDB_MMT_MEDIA_RECEIVER_H


#include "MPUAssembler.hpp"
#include "MMTTables.hpp"
#include "../Base/DataStream.hpp"
#include <vector>


namespace LibISDB
{

	/**
	 MMT の映像・音声アセットの受信クラス

	 選択された映像・音声アセットのアクセスユニットを組み立てて出力する。
	 時刻の基準には TLV ストリームの NTP パケットの時刻を使う。
	*/
	class MMTMediaReceiver
		: protected MPUAssembler::Handler
	{
	public:
		enum class MediaType {
			Video,
			Audio,
		};

		class Handler
		{
		public:
			virtual ~Handler() = default;

			// データはこの呼び出しの間のみ有効
			// NTP の時刻から MAX_TIMESTAMP_DEVIATION 以上離れた時刻は無効 (HasTimestamp = false) にする
			virtual void OnAccessUnit(
				const MMTMediaReceiver *pReceiver, MediaType Type, const MPUAssembler::AccessUnitInfo &Info) {}
			// NTP の時刻がうるう秒の調整で Seconds 秒 (-1 / +1) ずれた
			virtual void OnNTPLeapSecond(const MMTMediaReceiver *pReceiver, int Seconds) {}
		};

		/** NTP の時刻との差の上限 (90kHz) */
		static constexpr int64_t MAX_TIMESTAMP_DEVIATION = 10 * 90000;

		MMTMediaReceiver();
		~MMTMediaReceiver();

		void Reset();
		// TLVPacket 以外は無視する
		void StorePacketStream(DataStream *pData);

		void SetVideoPacketID(uint16_t PacketID);
		uint16_t GetVideoPacketID() const noexcept { return m_VideoPacketID; }
		void SetAudioPacketID(uint16_t PacketID);
		uint16_t GetAudioPacketID() const noexcept { return m_AudioPacketID; }

		void SetHandler(Handler *pHandler) noexcept { m_pHandler = pHandler; }

		// 最後に受信した NTP パケットの時刻
		bool HasNTPTime() const noexcept { return m_LastNTPTime != 0; }
		uint64_t GetNTPTime() const noexcept { return m_LastNTPTime; }
		// GetNTPTime() を 90kHz にしたもの (アクセスユニットの DTS / PTS と同じ基準)
		int64_t GetNTPTime90kHz() const noexcept;

		const MPUAssembler * GetAssembler(MediaType Type) const;

	protected:
	// MPUAssembler::Handler
		void OnAccessUnit(const MPUAssembler *pAssembler, const MPUAssembler::AccessUnitInfo &Info) override;

		void OnNTPPacket(const TLVPacket *pPacket);
		void OnPLTTable(const MMTSingleTable *pTable);
		void OnMPTTable(const MMTSingleTable *pTable);
		void UpdateAssets(const MPTable *pMPTable);
		void ApplyLastMPT();
		void UnmapAsset(uint16_t PacketID);

		MMTPacketIDMapManager m_PacketIDMapManager;
		std::vector<uint16_t> m_MPTPacketIDList;
		uint16_t m_VideoPacketID;
		uint16_t m_AudioPacketID;
		uint64_t m_LastNTPTime;
		Handler *m_pHandler;
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_MMT_MEDIA_RECEIVER_H
