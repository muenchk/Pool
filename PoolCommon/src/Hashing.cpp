#include "Hashing.h"

namespace Hashing
{
	size_t hash(std::multiset<EnumType> const& st)
	{
		auto itr = st.begin();
		std::size_t hs = 0;
		if (itr != st.end()) {
			hs = std::hash<EnumType>{}(*itr);
			itr++;
		}
		while (itr != st.end()) {
			hs = hs ^ (std::hash<EnumType>{}(*itr) << 1);
			itr++;
		}
		return hs;
	}

	size_t hash(std::list<uint64_t> const& st)
	{
		auto itr = st.begin();
		std::size_t hs = 0;
		if (itr != st.end()) {
			hs = std::hash<uint64_t>{}(*itr);
			itr++;
		}
		while (itr != st.end()) {
			hs = hs ^ (std::hash<uint64_t>{}(*itr) << 1);
			itr++;
		}
		return hs;
	}
}
