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
 @file   TTMLCaption.cpp
 @brief  ARIB-TTML 字幕
 @author cngu-dtv
*/


#include "../LibISDBPrivate.hpp"
#include "TTMLCaption.hpp"
#include "MMTString.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


namespace
{

struct ElementInfo {
	std::string_view Name;
	bool Exclude;
	int64_t Begin;
	int64_t End;
};

struct AttributeInfo {
	std::string_view Name;
	std::string_view Value;
};


std::string_view GetLocalName(std::string_view Name)
{
	const size_t Pos = Name.find(':');
	if (Pos == std::string_view::npos)
		return Name;
	return Name.substr(Pos + 1);
}


bool IsXMLSpace(char c)
{
	return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
}


void AppendUTF8(std::string &Str, uint32_t Code)
{
	if (Code < 0x80) {
		Str += static_cast<char>(Code);
	} else if (Code < 0x800) {
		Str += static_cast<char>(0xC0 | (Code >> 6));
		Str += static_cast<char>(0x80 | (Code & 0x3F));
	} else if (Code < 0x10000) {
		Str += static_cast<char>(0xE0 | (Code >> 12));
		Str += static_cast<char>(0x80 | ((Code >> 6) & 0x3F));
		Str += static_cast<char>(0x80 | (Code & 0x3F));
	} else if (Code < 0x110000) {
		Str += static_cast<char>(0xF0 | (Code >> 18));
		Str += static_cast<char>(0x80 | ((Code >> 12) & 0x3F));
		Str += static_cast<char>(0x80 | ((Code >> 6) & 0x3F));
		Str += static_cast<char>(0x80 | (Code & 0x3F));
	}
}


// 実体参照を 1 つ解釈する。Pos は '&' の位置で、解釈できた場合は ';' の次に進める
bool DecodeEntity(std::string_view Text, size_t &Pos, std::string &Out)
{
	const size_t End = Text.find(';', Pos);
	if ((End == std::string_view::npos) || (End - Pos > 10))
		return false;

	const std::string_view Name = Text.substr(Pos + 1, End - Pos - 1);
	uint32_t Code;

	if (Name == "lt") {
		Code = '<';
	} else if (Name == "gt") {
		Code = '>';
	} else if (Name == "amp") {
		Code = '&';
	} else if (Name == "quot") {
		Code = '"';
	} else if (Name == "apos") {
		Code = '\'';
	} else if ((Name.size() >= 2) && (Name[0] == '#')) {
		const bool Hex = (Name[1] == 'x') || (Name[1] == 'X');
		const std::string_view Digits = Name.substr(Hex ? 2 : 1);
		if (Digits.empty())
			return false;
		Code = 0;
		for (char c : Digits) {
			uint32_t Value;
			if ((c >= '0') && (c <= '9'))
				Value = c - '0';
			else if (Hex && (c >= 'a') && (c <= 'f'))
				Value = c - 'a' + 10;
			else if (Hex && (c >= 'A') && (c <= 'F'))
				Value = c - 'A' + 10;
			else
				return false;
			Code = Code * (Hex ? 16 : 10) + Value;
			if (Code >= 0x110000)
				return false;
		}
	} else {
		return false;
	}

	AppendUTF8(Out, Code);
	Pos = End + 1;

	return true;
}


// 空白は 1 つの空白にまとめる (xml:space="default")。'\r' は改行 (br 要素) の印
void AppendText(std::string &Out, std::string_view Text, bool DecodeEntities)
{
	for (size_t Pos = 0; Pos < Text.size();) {
		const char c = Text[Pos];

		if (IsXMLSpace(c)) {
			if (!Out.empty() && (Out.back() != ' ') && (Out.back() != '\r'))
				Out += ' ';
			Pos++;
		} else if ((c == '&') && DecodeEntities) {
			if (!DecodeEntity(Text, Pos, Out)) {
				Out += c;
				Pos++;
			}
		} else {
			Out += c;
			Pos++;
		}
	}
}


void TrimText(std::string &Text)
{
	std::string Result;
	Result.reserve(Text.size());

	for (size_t i = 0; i < Text.size(); i++) {
		const char c = Text[i];
		if (c == ' ') {
			if (Result.empty() || (Result.back() == '\r'))
				continue;
			if ((i + 1 == Text.size()) || (Text[i + 1] == '\r'))
				continue;
		}
		Result += c;
	}

	while (!Result.empty() && (Result.back() == '\r'))
		Result.pop_back();

	Text = std::move(Result);
}


// タグの終わりの '>' の位置 (属性値の中の '>' は除く)
size_t FindTagEnd(std::string_view Doc, size_t Pos)
{
	char Quote = '\0';

	for (; Pos < Doc.size(); Pos++) {
		const char c = Doc[Pos];
		if (Quote != '\0') {
			if (c == Quote)
				Quote = '\0';
		} else if ((c == '"') || (c == '\'')) {
			Quote = c;
		} else if (c == '>') {
			return Pos;
		}
	}

	return std::string_view::npos;
}


bool ParseTag(std::string_view Tag, std::string_view *pName, std::vector<AttributeInfo> *pAttributes)
{
	pAttributes->clear();

	size_t Pos = 0;
	while ((Pos < Tag.size()) && !IsXMLSpace(Tag[Pos]))
		Pos++;
	*pName = Tag.substr(0, Pos);
	if (pName->empty())
		return false;

	for (;;) {
		while ((Pos < Tag.size()) && IsXMLSpace(Tag[Pos]))
			Pos++;
		if (Pos >= Tag.size())
			break;

		const size_t NameStart = Pos;
		while ((Pos < Tag.size()) && (Tag[Pos] != '=') && !IsXMLSpace(Tag[Pos]))
			Pos++;
		const std::string_view Name = Tag.substr(NameStart, Pos - NameStart);
		while ((Pos < Tag.size()) && IsXMLSpace(Tag[Pos]))
			Pos++;
		if ((Pos >= Tag.size()) || (Tag[Pos] != '='))
			return false;
		Pos++;
		while ((Pos < Tag.size()) && IsXMLSpace(Tag[Pos]))
			Pos++;
		if ((Pos >= Tag.size()) || ((Tag[Pos] != '"') && (Tag[Pos] != '\'')))
			return false;
		const char Quote = Tag[Pos++];
		const size_t ValueEnd = Tag.find(Quote, Pos);
		if (ValueEnd == std::string_view::npos)
			return false;

		AttributeInfo &Attribute = pAttributes->emplace_back();
		Attribute.Name = Name;
		Attribute.Value = Tag.substr(Pos, ValueEnd - Pos);
		Pos = ValueEnd + 1;
	}

	return true;
}


const AttributeInfo * FindAttribute(const std::vector<AttributeInfo> &List, std::string_view Name)
{
	for (const AttributeInfo &e : List) {
		if (e.Name == Name)
			return &e;
	}
	return nullptr;
}


bool ParseNumber(std::string_view Str, size_t &Pos, int64_t *pValue)
{
	const size_t Start = Pos;
	int64_t Value = 0;

	while ((Pos < Str.size()) && (Str[Pos] >= '0') && (Str[Pos] <= '9')) {
		Value = Value * 10 + (Str[Pos] - '0');
		if (Value > 0x7FFFFFFF)
			return false;
		Pos++;
	}

	*pValue = Value;

	return Pos > Start;
}


// 小数部をミリ秒にする (4 桁目以降は切り捨て)
int64_t ParseFraction(std::string_view Str, size_t &Pos)
{
	int64_t Value = 0;
	int Digits = 0;

	while ((Pos < Str.size()) && (Str[Pos] >= '0') && (Str[Pos] <= '9')) {
		if (Digits < 3) {
			Value = Value * 10 + (Str[Pos] - '0');
			Digits++;
		}
		Pos++;
	}
	for (; Digits < 3; Digits++)
		Value *= 10;

	return Value;
}

}




bool TTMLCaptionDecoder::Decode(const uint8_t *pData, size_t Size, ReturnArg<ParagraphList> List)
{
	if (!List)
		return false;

	List->clear();

	if (pData == nullptr)
		return false;

	std::string_view Doc(reinterpret_cast<const char *>(pData), Size);
	if ((Doc.size() >= 3) && (Doc.substr(0, 3) == "\xEF\xBB\xBF"))
		Doc.remove_prefix(3);

	std::vector<ElementInfo> Stack;
	std::vector<AttributeInfo> Attributes;
	bool HasRoot = false;
	bool InParagraph = false;
	size_t ParagraphDepth = 0;
	ParagraphInfo Paragraph;
	std::string Text;

	const auto EndParagraph =
		[&]() {
			InParagraph = false;
			TrimText(Text);
			if (!Text.empty()) {
				// '\r' は DecodeMMTString で LIBISDB_NEWLINE になる
				DecodeMMTString(reinterpret_cast<const uint8_t *>(Text.data()), Text.size(), &Paragraph.Text);
				if (!Paragraph.Text.empty())
					List->push_back(std::move(Paragraph));
			}
		};

	size_t Pos = 0;

	while (Pos < Doc.size()) {
		if (Doc[Pos] != '<') {
			size_t End = Doc.find('<', Pos);
			if (End == std::string_view::npos)
				End = Doc.size();
			if (InParagraph && !Stack.empty() && !Stack.back().Exclude)
				AppendText(Text, Doc.substr(Pos, End - Pos), true);
			Pos = End;
			continue;
		}

		if (Doc.substr(Pos, 4) == "<!--") {
			const size_t End = Doc.find("-->", Pos + 4);
			if (End == std::string_view::npos)
				return false;
			Pos = End + 3;
			continue;
		}
		if (Doc.substr(Pos, 9) == "<![CDATA[") {
			const size_t End = Doc.find("]]>", Pos + 9);
			if (End == std::string_view::npos)
				return false;
			if (InParagraph && !Stack.empty() && !Stack.back().Exclude)
				AppendText(Text, Doc.substr(Pos + 9, End - (Pos + 9)), false);
			Pos = End + 3;
			continue;
		}
		if (Doc.substr(Pos, 2) == "<?") {
			const size_t End = Doc.find("?>", Pos + 2);
			if (End == std::string_view::npos)
				return false;
			Pos = End + 2;
			continue;
		}
		if (Doc.substr(Pos, 2) == "<!") {
			const size_t End = Doc.find('>', Pos + 2);
			if (End == std::string_view::npos)
				return false;
			Pos = End + 1;
			continue;
		}

		const size_t TagEnd = FindTagEnd(Doc, Pos + 1);
		if (TagEnd == std::string_view::npos)
			return false;
		std::string_view Tag = Doc.substr(Pos + 1, TagEnd - (Pos + 1));
		Pos = TagEnd + 1;

		if (!Tag.empty() && (Tag[0] == '/')) {
			Tag.remove_prefix(1);
			while (!Tag.empty() && IsXMLSpace(Tag.back()))
				Tag.remove_suffix(1);
			for (size_t i = Stack.size(); i > 0; i--) {
				if (Stack[i - 1].Name == Tag) {
					if (InParagraph && (i - 1 <= ParagraphDepth))
						EndParagraph();
					Stack.resize(i - 1);
					break;
				}
			}
			continue;
		}

		const bool EmptyElement = !Tag.empty() && (Tag.back() == '/');
		if (EmptyElement)
			Tag.remove_suffix(1);

		std::string_view Name;
		if (!ParseTag(Tag, &Name, &Attributes))
			return false;

		const std::string_view LocalName = GetLocalName(Name);
		const ElementInfo *pParent = Stack.empty() ? nullptr : &Stack.back();
		ElementInfo Element;

		Element.Name = Name;
		Element.Exclude =
			((pParent != nullptr) && pParent->Exclude)
			|| (LocalName == "head")
			|| (FindAttribute(Attributes, "arib-tt:ruby") != nullptr);

		// 時刻は親要素の開始時刻からの相対値 (ARIB TR-B39 第1部 第3編 8.5.9.1)
		const int64_t ParentBegin = (pParent != nullptr) ? pParent->Begin : 0;
		const int64_t ParentEnd = (pParent != nullptr) ? pParent->End : TIME_INVALID;
		int64_t Time;
		Element.Begin = ParentBegin;
		if (const AttributeInfo *pAttr = FindAttribute(Attributes, "begin");
				(pAttr != nullptr) && ParseTime(pAttr->Value, &Time))
			Element.Begin = ParentBegin + Time;
		Element.End = TIME_INVALID;
		if (const AttributeInfo *pAttr = FindAttribute(Attributes, "end");
				(pAttr != nullptr) && ParseTime(pAttr->Value, &Time))
			Element.End = ParentBegin + Time;
		else if (const AttributeInfo *pDur = FindAttribute(Attributes, "dur");
				(pDur != nullptr) && ParseTime(pDur->Value, &Time))
			Element.End = Element.Begin + Time;
		if ((ParentEnd != TIME_INVALID) && ((Element.End == TIME_INVALID) || (Element.End > ParentEnd)))
			Element.End = ParentEnd;

		if (LocalName == "tt")
			HasRoot = true;

		if (!Element.Exclude) {
			if ((LocalName == "p") && !InParagraph) {
				InParagraph = true;
				ParagraphDepth = Stack.size();
				Paragraph = ParagraphInfo();
				Paragraph.Begin = Element.Begin;
				Paragraph.End = Element.End;
				Text.clear();
			}
			if (InParagraph) {
				if (Paragraph.Region.empty()) {
					if (const AttributeInfo *pAttr = FindAttribute(Attributes, "region"); pAttr != nullptr)
						DecodeMMTString(reinterpret_cast<const uint8_t *>(pAttr->Value.data()), pAttr->Value.size(), &Paragraph.Region);
				}
				if (LocalName == "br") {
					while (!Text.empty() && (Text.back() == ' '))
						Text.pop_back();
					Text += '\r';
				}
			}
		}

		if (EmptyElement) {
			if (InParagraph && (Stack.size() == ParagraphDepth))
				EndParagraph();
		} else {
			Stack.push_back(Element);
		}
	}

	if (InParagraph)
		EndParagraph();

	return HasRoot;
}


bool TTMLCaptionDecoder::DecodeText(const uint8_t *pData, size_t Size, ReturnArg<String> Text)
{
	if (!Text)
		return false;

	Text->clear();

	ParagraphList List;
	if (!Decode(pData, Size, &List))
		return false;

	for (const ParagraphInfo &Paragraph : List) {
		if (!Text->empty())
			*Text += LIBISDB_STR(LIBISDB_NEWLINE);
		*Text += Paragraph.Text;
	}

	return true;
}


bool TTMLCaptionDecoder::ParseTime(std::string_view Str, ReturnArg<int64_t> Time)
{
	if (!Time)
		return false;

	while (!Str.empty() && IsXMLSpace(Str.front()))
		Str.remove_prefix(1);
	while (!Str.empty() && IsXMLSpace(Str.back()))
		Str.remove_suffix(1);

	size_t Pos = 0;
	int64_t Value;

	if (Str.find(':') != std::string_view::npos) {
		// clock-time : hours ":" minutes ":" seconds ("." fraction)?
		int64_t Hours, Minutes, Seconds;
		if (!ParseNumber(Str, Pos, &Hours) || (Pos >= Str.size()) || (Str[Pos++] != ':')
				|| !ParseNumber(Str, Pos, &Minutes) || (Pos >= Str.size()) || (Str[Pos++] != ':')
				|| !ParseNumber(Str, Pos, &Seconds))
			return false;
		Value = ((Hours * 60 + Minutes) * 60 + Seconds) * 1000;
		if ((Pos < Str.size()) && (Str[Pos] == '.')) {
			Pos++;
			Value += ParseFraction(Str, Pos);
		}
		if (Pos != Str.size())
			return false;
	} else {
		// offset-time : time-count fraction? metric
		int64_t Count;
		if (!ParseNumber(Str, Pos, &Count))
			return false;
		int64_t Fraction = 0;
		if ((Pos < Str.size()) && (Str[Pos] == '.')) {
			Pos++;
			Fraction = ParseFraction(Str, Pos);
		}
		const std::string_view Metric = Str.substr(Pos);
		if (Metric == "h")
			Value = Count * 3600000 + Fraction * 3600;
		else if (Metric == "m")
			Value = Count * 60000 + Fraction * 60;
		else if (Metric == "s")
			Value = Count * 1000 + Fraction;
		else if (Metric == "ms")
			Value = Count;
		else
			return false;
	}

	*Time = Value;

	return true;
}


} // namespace LibISDB
