#include "Input.h"

namespace CommonLib
{
	size_t InputSingle::GetDataSize()
	{
		size_t sz = ISerializable::GetDataSize()  // form stuff
		            + 4                           // version
		            + 1                           // _returnedInput
			;
		sz += Buffer::_sizeof(_input);

		return sz;
	}
	bool InputSingle::WriteData(Buffer::ArrayBuffer& buffer)
	{
		ISerializable::WriteData(buffer);

		SAVE_WRITE(buffer, uint32_t, version);
		SAVE_WRITE(buffer, bool, _returnedInput);
		SAVE_WRITE(buffer, std::string, _input);
	}
	bool InputSingle::ReadData(Buffer::ArrayBuffer& buffer, LoadResolver* resolver)
	{
		ISerializable::ReadData(buffer, resolver);

		uint32_t vers = buffer.Read<uint32_t>();
		switch (vers) {
		case 0x1:
			_returnedInput = buffer.Read<bool>();
			_input = buffer.Read<std::string>();

			return true;
		default:
			logwarn("unknown version for class InputSingle");
			return false;
		}
	}
	void InputSingle::InitializeEarly(LoadResolver* resolver)
	{

	}
	void InputSingle::InitializeLate(LoadResolver* resolver)
	{

	}
	void InputSingle::Delete()
	{
		Clear();
	}
	bool InputSingle::CanDelete()
	{
		return true;
	}
	void InputSingle::Clear()
	{
		_input = "";
		_returnedInput = false;
	}
	int32_t InputSingle::GetType()
	{
		return FormType::InputSingle;
	}

	std::string InputSingle::GetNextInput()
	{
		if (_returnedInput)
			return "";
		else
			return _input;
	}
	bool InputSingle::HasInputWithIndex(size_t index)
	{
		if (index == 0)
			return true;
		else
			return false;
	}
	std::string InputSingle::GetInputWithIndex(size_t index)
	{
		if (index == 0)
			return _input;
		else
			return "";
	}
	std::string InputSingle::GetLastWrittenInput()
	{
		if (_returnedInput)
			return _input;
		else
			return "";
	}
	size_t InputSingle::GetLastWrittenInputIndex()
	{
		if (_returnedInput)
			return 0;
		else
			return UINT32_MAX;
	}
	bool InputSingle::HasNextInput()
	{
		return !_returnedInput;
	}
	void InputSingle::ResetIndex()
	{
		_returnedInput = false;
	}
	Ref<IInput> InputSingle::Clone()
	{
		InputSingle* input = new InputSingle;
		input->_input = _input;
		input->_returnedInput = _returnedInput;
		return input;
	}

	void InputSingle::SetInput(std::string input)
	{
		_input = input;
		_returnedInput = false;
	}


	
	size_t InputMulti::GetDataSize()
	{
		size_t sz = ISerializable::GetDataSize()  // form stuff
		            + 4                           // version
		            + 8                           // _index
			+ 8 // size of _list
			;
		for (auto& entry : _list)
			sz += Buffer::_sizeof(entry);

		return sz;
	}
	bool InputMulti::WriteData(Buffer::ArrayBuffer& buffer)
	{
		ISerializable::WriteData(buffer);

		SAVE_WRITE(buffer, uint32_t, version);
		SAVE_WRITE(buffer, size_t, _index);
		SAVE_WRITE(buffer, size_t, _list.size());
		for (size_t i = 0; i < _list.size(); i++)
			SAVE_WRITE(buffer, std::string, _list[i]);

		return true;
	}
	bool InputMulti::ReadData(Buffer::ArrayBuffer& buffer, LoadResolver* resolver)
	{
		ISerializable::ReadData(buffer, resolver);

		uint32_t vers = buffer.Read<uint32_t>();
		switch (vers) {
		case 0x1:
			{
				_index = buffer.Read<size_t>();
				size_t sz = buffer.Read<size_t>();
				for (size_t i = 0; i < sz; i++) {
					_list.push_back(buffer.Read<std::string>());
				}
				return true;
			}
			break;
		default:
			logwarn("unknown version for class InputSingle");
			return false;
		}
	}
	void InputMulti::InitializeEarly(LoadResolver* resolver)
	{
	}
	void InputMulti::InitializeLate(LoadResolver* resolver)
	{
	}
	void InputMulti::Delete()
	{
		Clear();
	}
	bool InputMulti::CanDelete()
	{
		return true;
	}
	void InputMulti::Clear()
	{
		_index = 0;
		_list.clear();
		_list.shrink_to_fit();
	}
	int32_t InputMulti::GetType()
	{
		return FormType::InputSingle;
	}

	std::string InputMulti::GetNextInput()
	{
		if (_index < _list.size()) {
			std::string input = _list[_index];
			_index++;
			return input;
		} else {
			return "";
		}
	}
	bool InputMulti::HasInputWithIndex(size_t index)
	{
		return index < _list.size();
	}
	std::string InputMulti::GetInputWithIndex(size_t index)
	{
		if (index < _list.size())
			return _list[index];
		else
			return "";
	}
	std::string InputMulti::GetLastWrittenInput()
	{
		if (_index < _list.size() && _index != 0)
			return _list[_index - 1];
		else
			return "";
	}
	size_t InputMulti::GetLastWrittenInputIndex()
	{
		if (_index == 0)
			return UINT32_MAX;
		else
			return _index;
	}
	bool InputMulti::HasNextInput()
	{
		return _index < _list.size();
	}
	void InputMulti::ResetIndex()
	{
		_index = 0;
	}
	Ref<IInput> InputMulti::Clone()
	{
		InputMulti* input = new InputMulti;
		input->_index = _index;
		input->_list = _list;
		return input;
	}

	size_t InputMulti::AddInput(std::string str)
	{
		_list.push_back(str);
		return _list.size() - 1;
	}

	void InputMulti::EraseInput(std::string str)
	{
		for (auto itr = _list.begin(); itr != _list.end(); itr++)
		{
			if (*itr == str)
				itr == _list.erase(itr);
		}
	}

	void InputMulti::EraseInput(size_t index)
	{
		if (index < _list.size())
		{
			auto itr = _list.begin();
			std::advance(itr, index);
			_list.erase(itr);
		}
	}

	void InputMulti::RemoveLastInput()
	{
		if (_list.empty() == false)
			_list.pop_back();
	}
}
