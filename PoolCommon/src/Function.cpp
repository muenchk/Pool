/*
* Copyright		@Kai Münch		2026
* This is a preexisting file.
*/

#include "Function.h"
#include "BufferOperations.h"
#include "Logging.h"
#include "Utility.h"

namespace CommonLib
{
	namespace Functions
	{
		/// <summary>
		/// runtime registry for Function-classes. factory functions for the classes are registered with their respective classid
		/// </summary>
		std::unordered_map<uint64_t, std::function<std::shared_ptr<BaseFunction>()>> classregistry;

		void RegisterFactory(uint64_t classid, std::function<std::shared_ptr<BaseFunction>()> factory)
		{
			loginfo("Registered callback factory: {}", UtilityBase::GetHex(classid))
				classregistry.insert({ classid, factory });
		}

		std::shared_ptr<BaseFunction> FunctionFactory(uint64_t classid)
		{
			//loginfo("Looking for Class ID: {}", classid);
			if (classregistry.contains(classid) == false)
				logcritical("Missing Class ID: {}", classid);
			auto func = classregistry.at(classid);
			if (func != nullptr)
				return func();
			else
				return nullptr;
		}
		std::shared_ptr<BaseFunction> BaseFunction::Create(Buffer::ArrayBuffer& buffer, LoadResolver* resolver)
		{
			uint64_t type = buffer.Read<uint64_t>();
			auto ptr = FunctionFactory(type);
			ptr->ReadData(buffer, resolver);
			return ptr;
		}

		size_t BaseFunction::GetLength()
		{
			return 8;  // type
		}

		bool BaseFunction::WriteData(Buffer::ArrayBuffer& buffer)
		{
			SAVE_WRITE(buffer, uint64_t, GetType());
			return true;
		}
		unsigned char* BaseFunction::GetData(size_t& size)
		{
			unsigned char* buffer = new unsigned char[GetLength()];
			size_t off = 0;
			Buffer::Write(GetType(), buffer, off);
			size = off;
			return buffer;
		}
	}
}
