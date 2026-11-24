/*
* Copyright		@Kai Münch		2026
* This is a preexisting file.
*/

#include "Flags.h"


namespace PoolCommon
{
	void Flags::SetFlag(uint64_t flag)
	{
		_flags.insert(flag);
		if (flag > 191)
			_flagsAlloc4 |= (uint64_t)1 << (flag - 192);
		if (flag > 127)
			_flagsAlloc3 |= (uint64_t)1 << (flag - 128);
		if (flag > 63)
			_flagsAlloc2 |= (uint64_t)1 << (flag - 64);
		else
			_flagsAlloc |= ((uint64_t)1 << flag);
	}

	void Flags::UnsetFlag(uint64_t flag)
	{
		auto itr = _flags.find(flag);
		if (itr != _flags.end()) {
			_flags.erase(itr);
			if (!_flags.contains(flag)) {
				if (flag > 191)
					_flagsAlloc4 = _flagsAlloc4 & (0xFFFFFFFFFFFFFFFF xor ((uint64_t)1 << (flag - 192)));
				if (flag > 127)
					_flagsAlloc3 = _flagsAlloc3 & (0xFFFFFFFFFFFFFFFF xor ((uint64_t)1 << (flag - 128)));
				if (flag > 63)
					_flagsAlloc2 = _flagsAlloc2 & (0xFFFFFFFFFFFFFFFF xor ((uint64_t)1 << (flag - 64)));
				else
					_flagsAlloc = _flagsAlloc & (0xFFFFFFFFFFFFFFFF xor ((uint64_t)1 << flag));
			}
		}
	}

	bool Flags::HasFlag(uint64_t flag)
	{
		if (flag > 191)
			return (_flagsAlloc4 & ((uint64_t)1 << (flag - 192))) > 0;
		if (flag > 127)
			return (_flagsAlloc3 & ((uint64_t)1 << (flag - 128))) > 0;
		if (flag > 63)
			return (_flagsAlloc2 & ((uint64_t)1 << (flag - 64))) > 0;
		else
			return (_flagsAlloc & (((uint64_t)1 << flag))) > 0;
	}

	void Flags::Clear()
	{
		_flagsAlloc = 0;
		_flagsAlloc2 = 0;
		_flagsAlloc3 = 0;
		_flagsAlloc4 = 0;
		_flags.clear();
	}

	size_t Flags::GetDataSize()
	{
		return 4                     // version
		       + 8                   //_flagsAlloc
		       + 8                   //_flagsAlloc2
		       + 8                   //_flagsAlloc3
		       + 8                   //_flagsAlloc4
		       + 8                   // size of multiset
		       + _flags.size() * 8;  // elements of multiset
	}

	bool Flags::WriteData(Buffer::ArrayBuffer& buffer)
	{
		SAVE_WRITE(buffer, uint32_t, version);
		SAVE_WRITE(buffer, uint64_t, _flagsAlloc);
		SAVE_WRITE(buffer, uint64_t, _flagsAlloc2);
		SAVE_WRITE(buffer, uint64_t, _flagsAlloc3);
		SAVE_WRITE(buffer, uint64_t, _flagsAlloc4);
		SAVE_WRITE(buffer, size_t, _flags.size());
		for (auto flag : _flags) {
			SAVE_WRITE(buffer, uint64_t, flag);
		}

		return true;
	}

	bool Flags::ReadData(Buffer::ArrayBuffer& buffer)
	{
		size_t size = 0;
		try {
			uint32_t vers = buffer.Read<uint32_t>();
			switch (vers) {
			case 0x1:
				_flagsAlloc = buffer.Read<uint64_t>();
				_flagsAlloc2 = buffer.Read<uint64_t>();
				_flagsAlloc3 = buffer.Read<uint64_t>();
				_flagsAlloc4 = buffer.Read<uint64_t>();
				size = buffer.Read<size_t>();
				for (int i = 0; i < (int)size; i++)
					_flags.insert(buffer.Read<uint64_t>());
				break;
			default:
				throw new std::exception("Wrong class version");
				break;
			}
		} catch (std::exception& e) {
			logcritical("Caught error while reading savefile: {}", e.what());
			return false;
		}
		return true;
	}

	void FlagsUnique::SetFlag(uint64_t flag)
	{
		_flags.insert(flag);
		if (flag > 191)
			_flagsAlloc4 |= (uint64_t)1 << (flag - 192);
		if (flag > 127)
			_flagsAlloc3 |= (uint64_t)1 << (flag - 128);
		if (flag > 63)
			_flagsAlloc2 |= (uint64_t)1 << (flag - 64);
		else
			_flagsAlloc |= ((uint64_t)1 << flag);
	}

	void FlagsUnique::UnsetFlag(uint64_t flag)
	{
		auto itr = _flags.find(flag);
		if (itr != _flags.end()) {
			_flags.erase(itr);
			if (flag > 191)
				_flagsAlloc4 = _flagsAlloc4 & (0xFFFFFFFFFFFFFFFF xor ((uint64_t)1 << (flag - 192)));
			if (flag > 127)
				_flagsAlloc3 = _flagsAlloc3 & (0xFFFFFFFFFFFFFFFF xor ((uint64_t)1 << (flag - 128)));
			if (flag > 63)
				_flagsAlloc2 = _flagsAlloc2 & (0xFFFFFFFFFFFFFFFF xor ((uint64_t)1 << (flag - 64)));
			else
				_flagsAlloc = _flagsAlloc & (0xFFFFFFFFFFFFFFFF xor ((uint64_t)1 << flag));
		}
	}

	bool FlagsUnique::HasFlag(uint64_t flag)
	{
		if (flag > 191)
			return _flagsAlloc4 & (uint64_t)1 << (flag - 192);
		if (flag > 127)
			return _flagsAlloc3 & (uint64_t)1 << (flag - 128);
		if (flag > 63)
			return _flagsAlloc2 & (uint64_t)1 << (flag - 64);
		else
			return _flagsAlloc & ((uint64_t)1 << flag);
	}

	void FlagsUnique::Clear()
	{
		_flagsAlloc = 0;
		_flagsAlloc2 = 0;
		_flagsAlloc3 = 0;
		_flagsAlloc4 = 0;
		_flags.clear();
	}

	size_t FlagsUnique::GetDataSize()
	{
		return 4                     // version
		       + 8                   //_flagsAlloc
		       + 8                   //_flagsAlloc2
		       + 8                   //_flagsAlloc3
		       + 8                   //_flagsAlloc4
		       + 8                   // size of multiset
		       + _flags.size() * 8;  // elements of multiset
	}

	bool FlagsUnique::WriteData(Buffer::ArrayBuffer& buffer)
	{
		SAVE_WRITE(buffer, uint32_t, version);
		SAVE_WRITE(buffer, uint64_t, _flagsAlloc);
		SAVE_WRITE(buffer, uint64_t, _flagsAlloc2);
		SAVE_WRITE(buffer, uint64_t, _flagsAlloc3);
		SAVE_WRITE(buffer, uint64_t, _flagsAlloc4);
		SAVE_WRITE(buffer, size_t, _flags.size());
		for (auto flag : _flags) {
			SAVE_WRITE(buffer, uint64_t, flag);
		}

		return true;
	}

	bool FlagsUnique::ReadData(Buffer::ArrayBuffer& buffer)
	{
		size_t size = 0;
		try {
			uint32_t vers = buffer.Read<uint32_t>();
			switch (vers) {
			case 0x1:
				_flagsAlloc = buffer.Read<uint64_t>();
				_flagsAlloc2 = buffer.Read<uint64_t>();
				_flagsAlloc3 = buffer.Read<uint64_t>();
				_flagsAlloc4 = buffer.Read<uint64_t>();
				size = buffer.Read<size_t>();
				for (int i = 0; i < (int)size; i++)
					_flags.insert(buffer.Read<uint64_t>());
				break;
			default:
				throw new std::exception("Wrong class version");
				break;
			}
		} catch (std::exception& e) {
			logcritical("Caught error while reading savefile: {}", e.what());
			return false;
		}
		return true;
	}

	FlagsBool::FlagsBool(size_t length)
	{
		_flags.resize(length);
		_length = length;
		std::fill(std::begin(_flags), std::end(_flags), false);
	}

	FlagsBool::~FlagsBool()
	{
	}

	void FlagsBool::Clone(FlagsBool& flags)
	{
		flags.SetLength(_length);
		std::copy(std::begin(_flags), std::end(_flags), std::begin(flags._flags));
	}

	void FlagsBool::Clone(std::vector<bool>& flags)
	{
		flags.resize(_length);
		std::copy(std::begin(_flags), std::end(_flags), std::begin(flags));
	}

	FlagsBool FlagsBool::Clone()
	{
		FlagsBool flags(_length);
		std::copy(std::begin(_flags), std::end(_flags), std::begin(flags._flags));
		return flags;
	}

	uint64_t FlagsBool::GetLength()
	{
		return _length;
	}

	void FlagsBool::SetLength(uint64_t length)
	{
		if (_length == length)
			return;
		else {
			_flags.resize(length);
			_length = length;
		}
	}

	std::vector<bool>::reference FlagsBool::operator[](uint64_t flag)
	{
		return _flags[flag];
	}

	std::vector<bool>::const_reference FlagsBool::operator[](uint64_t flag) const
	{
		return _flags[flag];
	}

	void FlagsBool::SetFlag(uint64_t flag)
	{
		_flags[flag] = true;
	}

	void FlagsBool::UnsetFlag(uint64_t flag)
	{
		_flags[flag] = false;
	}

	bool FlagsBool::HasFlag(uint64_t flag)
	{
		return _flags[flag];
	}

	void FlagsBool::Clear()
	{
		for (int i = 0; i < (int)_flags.size(); i++)
			_flags[i] = false;
	}

	size_t FlagsBool::GetDataSize()
	{
		return 4               // version
		       + 8             // length
		       + _length * 1;  // elements of multiset
	}

	bool FlagsBool::WriteData(Buffer::ArrayBuffer& buffer)
	{
		SAVE_WRITE(buffer, uint32_t, version);
		SAVE_WRITE(buffer, uint64_t, _length);
		for (int i = 0; i < (int)_length; i++) {
			SAVE_WRITE(buffer, bool, _flags[i]);
		}

		return true;
	}

	bool FlagsBool::ReadData(Buffer::ArrayBuffer& buffer)
	{
		size_t size = 0;
		try {
			uint32_t vers = buffer.Read<uint32_t>();
			switch (vers) {
			case 0x1:
				_length = buffer.Read<uint64_t>();
				_flags.resize(_length);
				for (int i = 0; i < (int)size; i++)
					_flags[i] = buffer.Read<bool>();
				break;
			default:
				throw new std::exception("Wrong class version");
				break;
			}
		} catch (std::exception& e) {
			logcritical("Caught error while reading savefile: {}", e.what());
			return false;
		}
		return true;
	}
}
