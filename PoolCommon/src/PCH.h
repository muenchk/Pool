#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define IMGUI_DEFINE_MATH_OPERATORS

#include <array>
#include <numbers>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <string_view>
#include <string>

#pragma warning(push)

#include <codecvt>

#include <boost/container/string.hpp>
#include <boost/container/set.hpp>
#include <boost/container/vector.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>

#include <fmt/format.h>

using namespace std::literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

#include "Logging.h"

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif
#pragma warning(pop)

#define DLLEXPORT __declspec(dllexport)


namespace stl
{
	template <class CharT>
	using basic_zstring = std::basic_string_view<CharT>;

	using zstring = basic_zstring<char>;
	using zwstring = basic_zstring<wchar_t>;
}


#include "Version.h"
#include "Plugin.h"
