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
 @file   TTMLCaption.hpp
 @brief  ARIB-TTML 字幕
 @author cngu-dtv
*/


#ifndef LIBISDB_TTML_CAPTION_H
#define LIBISDB_TTML_CAPTION_H


#include <vector>


namespace LibISDB
{

	/** ARIB-TTML 字幕のテキスト取り出しクラス (ARIB STD-B62 第1編 第3部、ARIB TR-B39 第1部 第3編 8.5) */
	class TTMLCaptionDecoder
	{
	public:
		static constexpr int64_t TIME_INVALID = -1;

		struct ParagraphInfo {
			String Text;      /**< 改行 (br 要素) は LIBISDB_NEWLINE */
			String Region;    /**< region 属性 */
			int64_t Begin;    /**< 開始時刻 (ミリ秒、時刻の起点からの値)。指定が無ければ 0 */
			int64_t End;      /**< 終了時刻 (ミリ秒)。指定が無ければ TIME_INVALID */
		};

		typedef std::vector<ParagraphInfo> ParagraphList;

		// テキストの無い p 要素は含めない。消去の文書 (ARIB STD-B62 第1編 第3部 3.8) は空のリストを返す
		// XML として読めない場合は false
		static bool Decode(const uint8_t *pData, size_t Size, ReturnArg<ParagraphList> List);
		// p 要素ごとに改行で区切ったテキスト
		static bool DecodeText(const uint8_t *pData, size_t Size, ReturnArg<String> Text);
		// TTML の時刻表現 (clock-time / offset-time) をミリ秒にする
		static bool ParseTime(std::string_view Str, ReturnArg<int64_t> Time);
	};

} // namespace LibISDB


#endif // ifndef LIBISDB_TTML_CAPTION_H
