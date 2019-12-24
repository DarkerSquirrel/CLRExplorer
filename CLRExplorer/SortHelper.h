#pragma once

struct SortHelper final abstract {
	static bool SortStrings(const CString& s1, const CString& s2, bool ascending);
	static bool SortStrings(const std::string& s1, const std::string& s2, bool ascending);
	static bool SortBoolean(bool a, bool b, bool asc);

	template<typename Number>
	static bool SortNumbers(const Number& n1, const Number& n2, bool ascending) {
		return ascending ? n2 > n1 : n2 < n1;
	}
};


