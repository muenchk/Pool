#pragma once

#include <set>
#include <list>

namespace Hashing
{
	size_t hash(std::multiset<uint64_t> const& st);
	size_t hash(std::list<uint64_t> const& st);
}
