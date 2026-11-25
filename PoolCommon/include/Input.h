#pragma once

#include <deque>

#include "Serializable.h"

namespace CommonLib
{
	class IInput
	{
	private:
		static const uint32_t version = 0x1;
	public:
		struct InputFlags : public ISerializable
		{
			enum InputFlag : EnumType
			{
				Fragments = 0x1
			};
		};

		virtual bool HasFragments() {
			return _flags.HasFlag(InputFlags::Fragments);
		}

		virtual bool SetFragments() {
			_flags.SetFlag(InputFlags::Fragments);
		}

		/// <summary>
		/// Returns the next input / fragment to send to the application
		/// </summary>
		/// <returns></returns>
		virtual std::string GetNextInput() = 0;
		/// <summary>
		/// Returns whether there is an input with the given index
		/// </summary>
		/// <param name="index"></param>
		/// <returns></returns>
		virtual bool HasInputWithIndex(size_t index) = 0;
		/// <summary>
		/// Returns the input fragement at the given position
		/// </summary>
		/// <param name="index"></param>
		/// <returns></returns>
		virtual std::string GetInputWithIndex(size_t index) = 0;
		/// <summary>
		/// Returns the input that was last written
		/// </summary>
		/// <returns></returns>
		virtual std::string GetLastWrittenInput() = 0;
		/// <summary>
		/// Returns the index of the last written input (UINT32_MAX) if none was written
		/// </summary>
		/// <returns></returns>
		virtual size_t GetLastWrittenInputIndex() = 0;
		/// <summary>
		/// Returns whether there are any inputs left to write to the application
		/// </summary>
		/// <returns></returns>
		virtual bool HasNextInput() = 0;
		/// <summary>
		/// Resets the index of the next input (starts over from the beginning)
		/// </summary>
		virtual void ResetIndex() = 0;
		/// <summary>
		/// The timeout after which the target application is terminated after the last input has been sent to the application, if set to 0ns the programm will not be terminated before the global timeout
		/// </summary>
		/// <returns></returns>
		virtual std::chrono::nanoseconds TimeoutTerminateAfterLastInput() { return 0ns; }
		/// <summary>
		/// Returns an exact copy of this instance
		/// </summary>
		/// <returns></returns>
		virtual Ref<IInput> Clone() = 0;

	protected:

	};

	class InputSingle : public IInput
	{
	protected:
		std::string _input = "";
		bool _returnedInput = false;

	public:
		virtual std::string GetNextInput();
		virtual bool HasInputWithIndex(size_t index);
		virtual std::string GetInputWithIndex(size_t index);
		virtual std::string GetLastWrittenInput();
		virtual size_t GetLastWrittenInputIndex();
		virtual bool HasNextInput();
		virtual void ResetIndex();
		virtual Ref<IInput> Clone();

		/// <summary>
		/// Sets the input
		/// </summary>
		/// <param name="input"></param>
		virtual void SetInput(std::string input);
	};

	class InputMulti : public IInput
	{
	protected:
		size_t _index = 0;
		std::deque<std::string> _list = {};
	public:
		virtual std::string GetNextInput();
		virtual bool HasInputWithIndex(size_t index);
		virtual std::string GetInputWithIndex(size_t index);
		virtual std::string GetLastWrittenInput();
		virtual size_t GetLastWrittenInputIndex();
		virtual bool HasNextInput();
		virtual void ResetIndex();
		virtual Ref<IInput> Clone();

		/// <summary>
		/// Adds a new input to the list and returns the index in the list
		/// </summary>
		/// <param name=""></param>
		/// <returns></returns>
		virtual size_t AddInput(std::string str);
		/// <summary>
		/// Erases all occurences of the given input from the list
		/// </summary>
		/// <param name="str"></param>
		virtual void EraseInput(std::string str);
		/// <summary>
		/// Erases the input at the given position
		/// </summary>
		/// <param name="index"></param>
		virtual void EraseInput(size_t index);
		/// <summary>
		/// Removes the last input in the list
		/// </summary>
		virtual void RemoveLastInput();
	};
}
