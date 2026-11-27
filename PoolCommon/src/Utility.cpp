/*
* Copyright		@Kai Münch		2026
* This is a preexisting file.
*/

#include "Utility.h"
#include "Logging.h"

#include <set>
#include <random>


namespace CommonLib
{
	//-------------------Random-------------------------

	/// <summary>
	/// random number generator for processing probabilities
	/// </summary>
	/// <param name=""></param>
	/// <returns></returns>
	static std::mt19937 rand((unsigned int)(std::chrono::system_clock::now().time_since_epoch().count()));

	static std::uniform_int_distribution<signed> rand100(1, 100);

	int UtilityBase::RandomInt(int min, int max)
	{
		return std::uniform_int_distribution<signed>(min, max)(rand);
	}

	float UtilityBase::RandomFloat(float min, float max)
	{
		return std::uniform_real_distribution<float>(min, max)(rand);
	}

	bool UtilityBase::RandomBool()
	{
		return std::uniform_int_distribution<int>(0, 1)(rand);
	}

	float UtilityBase::ConvertUnitsToFeet(float units)
	{
		return (float)((double)units / 21.3333333333333333333333333);
	}
	float UtilityBase::ConvertUnitsToMeter(float units)
	{
		return (float)((double)units / 69.99125119);
	}

	bool Compare(std::string& lhs, std::string& rhs)
	{
		return lhs.find(rhs) != std::string::npos&& lhs.length() == rhs.length();
	}

	bool Compare(std::string& lhs, std::string rhs)
	{
		return lhs.find(rhs) != std::string::npos && lhs.length() == rhs.length();
	}

	struct window
	{
		std::string sliding;

		int32_t size = 0;
		int32_t maxsize = 0;

		char AddChar(char c)
		{
			char ret = 0;
			if (size == maxsize) {
				ret = sliding[0];
				sliding = sliding.substr(1, sliding.length() - 1);
				sliding += c;
			} else {
				sliding += c;
				size++;
			}
			return ret;
		}
		void Reset()
		{
			sliding = "";
			size = 0;
		}
		std::string GetWindow()
		{
			return sliding;
		}
		int32_t Compare(std::string& other)
		{
			return sliding.compare(other);
		}
	};

	std::vector<std::string> UtilityBase::SplitString(std::string str, char delimiter, bool removeEmpty, bool escape, char escapesymbol, bool allowdisableescape, char disablechar)
	{
		std::vector<std::string> splits;
		if (escape) {
			std::string tmp;
			bool escaped = false;
			char last = 0;
			for (char c : str) {
				if (escaped == false) {
					if (c == delimiter) {
						splits.push_back(tmp);
						tmp = "";
					} else if (c == escapesymbol && (allowdisableescape == false || allowdisableescape && last != disablechar)) {
						escaped = !escaped;
						tmp += c;
					} else
						tmp += c;
				} else {
					if (c == escapesymbol && (allowdisableescape == false || allowdisableescape && last != disablechar)) {
						escaped = !escaped;
						tmp += c;
					} else
						tmp += c;
				}
				last = c;
			}
			if (tmp.empty() == false || last == delimiter && escaped == false)
				splits.push_back(tmp);
		} else {
			size_t pos = str.find(delimiter);
			while (pos != std::string::npos) {
				splits.push_back(str.substr(0, pos));
				str.erase(0, pos + 1);
				pos = str.find(delimiter);
			}
			splits.push_back(str);
		}
		if (removeEmpty) {
			auto itr = splits.begin();
			while (itr != splits.end()) {
				if (*itr == "") {
					itr = splits.erase(itr);
					continue;
				}
				itr++;
			}
		}
		return splits;
	}

	std::vector<std::string> UtilityBase::SplitString(std::string str, std::string delimiter, bool removeEmpty, bool escape, char escapesymbol)
	{
		std::vector<std::string> splits;
		if (escape) {
			bool escaped = false;
			std::string tmp;
			window slide;
			slide.maxsize = (int)delimiter.size();
			for (char c : str) {
				//logdebug("Char: {}\t Window: {}\t Wsize: {}\t Wmaxsize: {}\t tmp: {}", c, slide.GetWindow(), slide.size, slide.maxsize, tmp);
				if (escape == true && escaped == true) {
					// escaped sequence.
					// just add char to string since we don't use window during escaped sequences
					tmp += c;
					if (c == escapesymbol) {
						escaped = !escaped;
					}
				} else {
					char x = slide.AddChar(c);
					if (x != 0)
						tmp += x;
					if (escape == true && c == escapesymbol) {
						// we are at the beginning of an escaped sequence
						escaped = !escaped;
						// add window to string since we don't use window during escaped sequence
						tmp += slide.GetWindow();
						slide.Reset();
					} else {
						if (slide.Compare(delimiter) == 0) {
							// delimiter matches sliding window
							splits.push_back(tmp);
							tmp = "";
							slide.Reset();
						}
					}
				}
			}
			// assemble last complete string part
			tmp += slide.GetWindow();
			// add as last split
			splits.push_back(tmp);
		}
		else {
			size_t pos = str.find(delimiter);
			while (pos != std::string::npos) {
				splits.push_back(str.substr(0, pos));
				str.erase(0, pos + delimiter.length());
				pos = str.find(delimiter);
			}
			splits.push_back(str);
		}
		if (removeEmpty) {
			auto itr = splits.begin();
			while (itr != splits.end()) {
				if (*itr == "") {
					itr = splits.erase(itr);
					continue;
				}
				itr++;
			}
		}
		return splits;
	}

	std::string& UtilityBase::RemoveWhiteSpaces(std::string& str, char escape, bool removetab, bool allowdisableescape, char disablechar)
	{
		bool escaped = false;
		auto itr = str.begin();
		char last = 0;
		while (itr != str.end()) {
			if (*itr == escape && (allowdisableescape == false || allowdisableescape && last != disablechar))
				escaped = !escaped;
			last = *itr;
			if (!escaped) {
				//char c = *_itr;
				if (*itr == ' ' || (removetab && *itr == '\t')) {
					itr = str.erase(itr);
					continue;
				}
			}
			itr++;
		}
		return str;
	}

	std::string& UtilityBase::RemoveSymbols(std::string& str, char symbol, bool enableescape, char escape)
	{
		bool escaped = false;
		auto itr = str.begin();
		while (itr != str.end()) {
			if (enableescape && *itr == escape)
				escaped = !escaped;
			if (!escaped) {
				char c = *itr;
				if (*itr == symbol) {
					itr = str.erase(itr);
					continue;
				}
			}
			itr++;
		}
		return str;
	}

	std::string& UtilityBase::RemoveSymbols(std::string& str, char symbol, char disablechar)
	{
		auto itr = str.begin();
		char last = 0;
		while (itr != str.end()) {
			if (*itr == symbol && last != disablechar) {
				itr = str.erase(itr);
				continue;
			}
			last = *itr;
			itr++;
		}
		return str;
	}

	std::vector<std::pair<char, int32_t>> UtilityBase::GetSymbols(std::string str)
	{
		std::vector<std::pair<char, int32_t>> result;
		for (char c : str) {
			for (size_t i = 0; i < result.size(); i++) {
				if (result[i].first == c) {
					result[i] = { c, result[i].second + 1 };
					continue;
				}
			}
			result.push_back({ c, 1 });
		}
		return result;
	}

	std::vector<int> UtilityBase::ParseIntArray(std::string line)
	{
		std::vector<int> ret;
		size_t pos = line.find(';');
		while (pos != std::string::npos) {
			try {
				ret.push_back(std::stoi(line.substr(0, pos)));
			} catch (std::out_of_range&) {
				return std::vector<int>();
			} catch (std::invalid_argument&) {
				return std::vector<int>();
			}
			line.erase(0, pos + 1);
			pos = line.find(';');
		}
		if (line.length() != 0) {
			try {
				ret.push_back(std::stoi(line));
			} catch (std::out_of_range&) {
				return std::vector<int>();
			} catch (std::invalid_argument&) {
				return std::vector<int>();
			}
		}
		return ret;
	}

}
