#pragma once

#include "Serializable.h"

namespace CommonLib
{
	class Data
	{
	private:
		boost::unordered_map<FormID, std::shared_ptr<ISerializable>> _hashmap;


	public:
		void Init();
	};
}
