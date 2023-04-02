#pragma once

struct Helpers abstract final {
	static std::wstring ExtractResource(UINT id, PCWSTR type, PCWSTR filename);
	template<typename T>
	static CString FormatWithCommas(T const& value) {
		if (value == 0)
			return L"0";
		auto str = std::format(L"{}", value);
		static NUMBERFMT fmt;
		static WCHAR sep[2];
		if (fmt.lpThousandSep == nullptr) {
			::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, sep, 2);
			fmt.lpDecimalSep = (PWSTR)L".";
			fmt.Grouping = 3;
			fmt.lpThousandSep = sep;
		}
		CString text;
		int chars = ::GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0, str.c_str(), &fmt, text.GetBufferSetLength(20), 20);
		return text.Left(chars);
	}
};

