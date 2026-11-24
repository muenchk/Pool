/*
* Copyright		@Kai Münch		2026
* This is a preexisting file.
*/

#pragma once

#include "BufferOperations.h"

namespace CommonLib
{
	class Flags
	{
		static const uint32_t version = 0x1;

		uint64_t _flagsAlloc = 0;
		uint64_t _flagsAlloc2 = 0;
		uint64_t _flagsAlloc3 = 0;
		uint64_t _flagsAlloc4 = 0;
		std::multiset<uint64_t> _flags;

	public:
		void SetFlag(uint64_t flag);

		void UnsetFlag(uint64_t flag);

		bool HasFlag(uint64_t flag);

		void Clear();

		size_t GetDataSize();

		bool WriteData(Buffer::ArrayBuffer& buffer);

		bool ReadData(Buffer::ArrayBuffer& buffer);
		
		size_t Hash();
	};
	class FlagsUnique
	{
		static const uint32_t version = 0x1;

		uint64_t _flagsAlloc = 0;
		uint64_t _flagsAlloc2 = 0;
		uint64_t _flagsAlloc3 = 0;
		uint64_t _flagsAlloc4 = 0;
		std::set<uint64_t> _flags;

	public:
		void SetFlag(uint64_t flag);

		void UnsetFlag(uint64_t flag);

		bool HasFlag(uint64_t flag);

		void Clear();

		size_t GetDataSize();

		bool WriteData(Buffer::ArrayBuffer& buffer);

		bool ReadData(Buffer::ArrayBuffer& buffer);

		size_t Hash();
	};
	class FlagsBool
	{
		static const uint32_t version = 0x1;

		uint64_t _length = 0;
		std::vector<bool> _flags;

	public:
		FlagsBool() = default;
		FlagsBool(const FlagsBool&) = delete;
		FlagsBool(FlagsBool&&) = default;

		FlagsBool(size_t length);

		~FlagsBool();

		void Clone(FlagsBool& flags);

		void Clone(std::vector<bool>& flags);

		FlagsBool Clone();

		uint64_t GetLength();

		void SetLength(uint64_t length);

		std::vector<bool>::reference operator[](uint64_t flag);

		std::vector<bool>::const_reference operator[](uint64_t flag) const;

		void SetFlag(uint64_t flag);

		void UnsetFlag(uint64_t flag);

		bool HasFlag(uint64_t flag);

		void Clear();

		size_t GetDataSize();

		bool WriteData(Buffer::ArrayBuffer& buffer);

		bool ReadData(Buffer::ArrayBuffer& buffer);

		size_t Hash();
	};
}
