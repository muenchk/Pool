/*
* Copyright		@Kai Münch		2026
* This is a preexisting file.
*/

#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace PoolCommon
{
	/// <summary>
	/// Provides generic functions
	/// </summary>
	class UtilityBase
	{
	public:

#pragma region Comparison

		template <typename T, typename U>
		bool EqualsSharedPtr(const std::weak_ptr<T>& t, const std::weak_ptr<U>& u)
		{
			return !t.owner_before(u) && !u.owner_before(t);
		}

/* template <typename T, typename U>
	inline bool EqualsSharedPtr(const std::weak_ptr<T>& t, const std::shared_ptr<U>& u)
	{
		return !t.owner_before(u) && !u.owner_before(t);
	}*/
#pragma endregion

		/// <summary>
		/// Returns a random integer between [min] and [max]
		/// </summary>
		/// <param name="min"></param>
		/// <param name="max"></param>
		/// <returns></returns>
		static int RandomInt(int min, int max);
		/// <summary>
		/// Returns a random floating point number between [min] and [max]
		/// </summary>
		/// <param name="min"></param>
		/// <param name="max"></param>
		/// <returns></returns>
		static float RandomFloat(float min, float max);
		/// <summary>
		/// Returns a random boolean value
		/// </summary>
		/// <returns></returns>
		static bool RandomBool();

		/// <summary>
		/// Converts game units to feet (imperial)
		/// </summary>
		/// <param name="units"></param>
		/// <returns></returns>
		static float ConvertUnitsToFeet(float units);
		/// <summary>
		/// Converts game units to meters (metric)
		/// </summary>
		/// <param name="units"></param>
		/// <returns></returns>
		static float ConvertUnitsToMeter(float units);

		/// <summary>
		/// Returns a string showing [val] as Hexadecimal number
		/// </summary>
		/// <param name="val"></param>
		/// <returns></returns>
		static std::string GetHex(long val)
		{
			std::stringstream ss;
			ss << std::hex << val;
			return ss.str();
		}
		/// <summary>
		/// Returns a string showing [val] as Hexadecimal number
		/// </summary>
		/// <param name="val"></param>
		/// <returns></returns>
		static std::string GetHex(uint64_t val)
		{
			std::stringstream ss;
			ss << std::hex << val;
			return ss.str();
		}
		/// <summary>
		/// Returns a string showing [val] as Hexadecimal number
		/// </summary>
		/// <param name="val"></param>
		/// <returns></returns>
		static std::string GetHex(uint32_t val)
		{
			std::stringstream ss;
			ss << std::hex << val;
			return ss.str();
		}
		/// <summary>
		/// Returns a string showing [val] as Hexadecimal number
		/// </summary>
		/// <param name="val"></param>
		/// <returns></returns>
		static std::string GetHex(int val)
		{
			std::stringstream ss;
			ss << std::hex << val;
			return ss.str();
		}
		/// <summary>
		/// Returns a string showing [val] as Hexadecimal number with padding
		/// </summary>
		/// <param name="val"></param>
		/// <returns></returns>
		static std::string GetHexFill(uint32_t val)
		{
			std::stringstream ss;
			ss << std::setw(16) << std::hex << std::setfill('0') << val;
			return ss.str();
		}
		/// <summary>
		/// Returns a string showing [val] as Hexadecimal number with padding
		/// </summary>
		/// <param name="val"></param>
		/// <returns></returns>
		static std::string GetHexFill(uint64_t val)
		{
			std::stringstream ss;
			ss << std::setw(16) << std::hex << std::setfill('0') << val;
			return ss.str();
		}

		/// <summary>
		/// Converts all symbols in a string into lower case.
		/// </summary>
		/// <param name="s"></param>
		/// <returns></returns>
		static std::string ToLower(std::string s)
		{
			std::transform(s.begin(), s.end(), s.begin(),
				[](unsigned char c) { return (unsigned char)std::tolower(c); }  // correct
			);
			return s;
		}

		/// <summary>
		/// Allocates a zeroed array of the given type
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <param name="length"></param>
		/// <returns></returns>
		template <class T>
		static T* AllocateZeroedArray(size_t length)
		{
			T* val = new T[length];
			for (size_t i = 0; i < length; i++)
				val[i] = 0;
			return val;
		}
		/// <summary>
		/// Allocates a given zeroed value
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <returns></returns>
		template <class T>
		static T* AllocateZeroed()
		{
			T* val = new T;
			*val = 0;
			return val;
		}

		/// <summary>
		/// returns the path to the currently executing dll
		/// </summary>
		/// <returns></returns>
		static std::wstring GetCurrentDLLPath()
		{
			HMODULE hModule = NULL;

			if (GetModuleHandleExW(
					GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
					(LPCWSTR)&GetCurrentDLLPath,
					&hModule)) {
				wchar_t buffer[MAX_PATH];

				if (GetModuleFileNameW(hModule, buffer, MAX_PATH) != 0) {
					return std::wstring(buffer);
				}
			}

			return L"";
		}

		/// <summary>
		/// Returns the name of the currently executing dll
		/// </summary>
		/// <returns></returns>
		static std::string GetCurrentDLLName() 
		{
			std::filesystem::path path = GetCurrentDLLPath();
			return path.filename().string();
		}

		
		static std::string executable_name()
		{
#if defined(PLATFORM_POSIX) || defined(__linux__)  //check defines for your setup

			std::string sp;
			std::ifstream stream(std::filesystem::path("/proc/self/comm"), std::ios_base::in);
			stream >> sp;
			return sp;

			//char result[ 100 ];
			//ssize_t count = readlink( "/proc/self/comm", result, 100 );
			//return std::string( result, (count > 0) ? count : 0 );

#elif defined(_WIN32)

			char buf[MAX_PATH];
			GetModuleFileNameA(nullptr, buf, MAX_PATH);
			return buf;

#else

			static_assert(false, "unrecognized platform");

#endif
		}

		/// <summary>
		/// Splits a string at a delimiter, optionally removes empty results, and optionally respects escaped sequences
		/// </summary>
		/// <param name="str">input to split</param>
		/// <param name="delimiter">delimiter at which to split</param>
		/// <param name="removeEmpty">whether to remove empty strings</param>
		/// <param name="escape">whether to respect escaped sequences</param>
		/// <param name="escapesymbol">symbol that marks escaped sequences, sequences need to be surrounded by this symbol</param>
		/// <returns></returns>
		static std::vector<std::string> SplitString(std::string str, char delimiter, bool removeEmpty, bool escape = false, char escapesymbol = '\"', bool allowdisableescape = false, char disblechar = '\\');

		/// <summary>
		/// Splits a string at a delimiter, optionally removes empty results, and optionally respects escaped sequences
		/// </summary>
		/// <param name="str">input to split</param>
		/// <param name="delimiter">delimiter at which to split</param>
		/// <param name="removeEmpty">whether to remove empty strings</param>
		/// <param name="escape">whether to respect escaped sequences</param>
		/// <param name="escapesymbol">symbol that marks escaped sequences, sequences need to be surrounded by this symbol</param>
		/// <returns></returns>
		static std::vector<std::string> SplitString(std::string str, std::string delimiter, bool removeEmpty, bool escape = false, char escapesymbol = '\"');

		/// <summary>
		/// Builds a string from a range and a delimiter
		/// </summary>
		/// <param name="vec"></param>
		/// <param name="concatString"></param>
		/// <returns></returns>
		template <typename _Rng>
			requires std::ranges::input_range<_Rng>
		static std::string BuildString(_Rng vec, std::string concatString = " ")
		{
			std::string result = "";
			auto itr = vec.begin();
			if (itr != vec.end()) {
				result = *itr;
				itr++;
			}
			while (itr != vec.end()) {
				result += concatString + *itr;
				itr++;
			}
			return result;
		}

		/// <summary>
		/// Evaluates whether the char at position [position] is escaped
		/// </summary>
		/// <param name="str">input to search</param>
		/// <param name="escapesymbol">symbol that marks escaped sequences, sequences need to be surrounded by this symbol</param>
		/// <param name="position">the position to check</param>
		/// <returns></returns>
		static bool IsEscaped(std::string str, int32_t position, char escapesymbol = '\"')
		{
			bool escaped = false;
			int32_t count = 0;
			for (char c : str) {
				//logdebug("Char: {}\t Window: {}\t Wsize: {}\t Wmaxsize: {}\t tmp: {}", c, slide.GetWindow(), slide.size, slide.maxsize, tmp);
				if (escaped == true) {
					// escaped sequence.
					if (c == escapesymbol) {
						escaped = !escaped;
					}
				} else {
					if (c == escapesymbol) {
						// we are at the beginning of an escaped sequence
						escaped = !escaped;
					}
				}
				if (count == position)
					return escaped;
				count++;
			}
			return false;
		}

		/// <summary>
		/// Removes whitespace from an input in-place, optionally respects escaped sequences
		/// </summary>
		static std::string& RemoveWhiteSpaces(std::string& str, char escape, bool removetab = false, bool allowdisableescape = false, char disablechar = '\\');

		/// <summary>
		/// Removes Symbols from an input in-place, optionally respects escaped sequences
		/// </summary>
		static std::string& RemoveSymbols(std::string& str, char symbol, bool enableescape = false, char escape = '\"');

		/// <summary>
		/// Removes Symbols from an input in-place, optionally doesn't remove escaped characters
		/// </summary>
		static std::string& RemoveSymbols(std::string& str, char symbol, char disablechar);

		/// <summary>
		/// Counts the occurences of the given symbol, respecting escaped sequences
		/// </summary>
		static int32_t CountSymbols(std::string str, char symbol, char escaped1, char escaped2)
		{
			int32_t count = 0;
			bool escape = false;
			for (char& c : str) {
				// escaped strings are marked by, e.g., " or ' and everything between two of them should not be counted
				// we need to check for both since one string could be marked with ", while another one could use '
				// this means that something thats invalid in python "usdzfgf' is counted as valid.
				if (c == escaped1 || c == escaped2)
					escape = !escape;
				if (escape == false)
					if (c == symbol)
						count++;
			}
			return count;
		}

		static std::vector<std::pair<char, int32_t>> GetSymbols(std::string str);

		
		// From Crashandler

		[[nodiscard]] static auto ConvertToWideString(std::string_view a_in) noexcept
			-> std::optional<std::wstring>
		{
			const auto len = mbstowcs(nullptr, a_in.data(), a_in.length());
			if (len == 0) {
				return std::nullopt;
			}

			std::wstring out(len, '\0');
			mbstowcs(out.data(), a_in.data(), a_in.length());

			return out;
		}

		[[nodiscard]] static auto ConvertToString(std::wstring_view a_in) noexcept
			-> std::optional<std::string>
		{
			const wchar_t* input = a_in.data();

			// Count required buffer size (plus one for null-terminator).
			size_t size = (wcslen(input) + 1) * sizeof(wchar_t);
			std::string out(size, '\0');

			size_t convertedSize;
			//wcstombs_s(&convertedSize, out.data(), size, input, size);
			convertedSize = wcstombs(out.data(), input, size);

			if (strcmp(out.c_str(), ""))
				return std::nullopt;

			return out;
		}

#pragma region Parsing

		/// <summary>
		/// Parses a string into a vector of int (array of int)
		/// </summary>
		/// <param name="line">string to parse</param>
		/// <returns>vector of int (array of int)</returns>
		static std::vector<int> ParseIntArray(std::string line);

#pragma endregion
	};
}
