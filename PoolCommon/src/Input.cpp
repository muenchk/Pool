#include "Input.h"

namespace CommonLib
{

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
	std::string vGetLastWrittenInput()
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
