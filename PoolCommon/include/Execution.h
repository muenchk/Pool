/*
*  Copyright @Kai Münch, University of Potsdam
* Derived from Test.h from TimeFuzz
*/

#pragma once

#include "Function.h"


namespace PoolCommon
{
	class Execution;
}

namespace Hashing
{
	size_t hash(PoolCommon::Execution* node);
}

namespace PoolCommon
{
	class ExecInput
	{

	};

	class Execution
	{
	private:
		void Init();

	public:
		struct ExitReason
		{
			enum ExitReasons
			{
				/// <summary>
				/// Process hasn't exited
				/// </summary>
				Running = 0,
				/// <summary>
				/// Process has exited naturally after finishing computation
				/// </summary>
				Natural = 1 << 0,
				/// <summary>
				/// Last input was written and worked and there wasn't a result
				/// </summary>
				LastInput = 1 << 1,
				/// <summary>
				/// Process has been terminated, for some reason
				/// </summary>
				Terminated = 1 << 2,
				/// <summary>
				/// Process has been terminated due to exceeding a timeout
				/// </summary>
				Timeout = 1 << 3,
				/// <summary>
				/// Process has been terminated due to exceeding timeout fragment
				/// </summary>
				FragmentTimeout = 1 << 4,
				/// <summary>
				/// Process has been terminated due to using up too much memory
				/// </summary>
				Memory = 1 << 5,
				/// <summary>
				/// Process has been terminated due to a pipe error
				/// </summary>
				Pipe = 1 << 6,
				/// <summary>
				/// An error occured when starting the test -> discard result
				/// </summary>
				InitError = 1 << 10,
				/// <summary>
				/// Not really an exit reason as this is decided by the oracle
				/// </summary>
				Repeat = 1 << 11,
			};
		};

		Execution(std::shared_ptr<Functions::BaseFunction> a_callback, uint64_t id);
		Execution() {}
		~Execution();

		void Init(std::shared_ptr<Functions::BaseFunction> a_callback, uint64_t id);

		/// <summary>
		/// Prepares the instance for execution
		/// </summary>
		void PrepareForExecution() { Init(); }

		/// <summary>
		/// whether the execution is currently running
		/// </summary>
		bool _running = false;
		/// <summary>
		/// the time of the tests start
		/// </summary>
		std::chrono::steady_clock::time_point _starttime;
		/// <summary>
		/// the time of the tests end
		/// </summary>
		std::chrono::steady_clock::time_point _endtime;

		/// <summary>
		/// _runtime variable that defines when an input is considered timeouted
		/// </summary>
		std::chrono::steady_clock::time_point _timeouttime;

		
		/// <summary>
		/// reaction time for each fragment [on whole test this contains only one element]
		/// </summary>
		std::list<uint64_t> _reactiontime;
		/// <summary>
		/// called after the test has been finished
		/// </summary>
		std::shared_ptr<Functions::BaseFunction> _callback;
		/// <summary>
		/// command line args for the test
		/// </summary>
		String _cmdArgs;
		/// <summary>
		/// stdin arguments for scripts
		/// </summary>
		String _scriptArgs;

		/// <summary>
		/// output of the PUT
		/// </summary>
		String _output;
		/// <summary>
		/// whether to store the put output
		/// </summary>
		bool _storeoutput;

		/// <summary>
		/// unique exec identifier
		/// </summary>
		uint64_t _identifier;

		/// <summary>
		/// reason the process has exited
		/// </summary>
		uint64_t _exitreason = ExitReason::Running;

		// process stuff
#if defined(unix) || defined(__unix__) || defined(__unix)
		pid_t processid;
		int32_t red_input[2];
		int32_t red_output[2];
		int32_t exitcode = -1;
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		HANDLE red_input[2];
		HANDLE red_output[2];
#endif

		/// <summary>
		/// this is a flag indicating that this test was already run and no oracle is necessary
		/// </summary>
		bool _skipOracle = false;

		/// <summary>
		/// whether there was an error in the pipe
		/// </summary>
		bool _pipeError = false;

		/// <summary>
		/// Returns whether the test is still running
		/// </summary>
		/// <returns></returns>
		bool IsRunning();

		/// <summary>
		/// Writes input to the stdint of the PUT
		/// </summary>
		/// <param name=""></param>
		bool SendInput(std::string, bool waitwrite);
		/// <summary>
		/// Writes data to the stdint of the PUT
		/// </summary>
		/// <param name="data">the data to write</param>
		/// <param name="offset">the offset to start at</param>
		/// <param name="length">the number of bytes to write</param>
		/// <returns></returns>
		long Send(const char* data, size_t offset, size_t length);
		/// <summary>
		/// Checks whether the PUT has reacted to the last input, by reading from and rewriting to pipe.
		/// </summary>
		/// <returns>returns whether input has been handled</returns>
		bool CheckInput();

		/// <summary>
		/// reads the output of the PUT
		/// </summary>
		/// <returns></returns>
		std::string ReceiveOutput();

		/// <summary>
		/// returns the memory used by the process
		/// </summary>
		/// <returns></returns>
		int64_t GetMemoryConsumption();
		/// <summary>
		/// terminates the process
		/// </summary>
		/// <returns></returns>
		bool KillProcess();

		bool WaitAndKillProcess();

		/// <summary>
		/// returns the exitcode
		/// </summary>
		int32_t GetExitCode();

		/// <summary>
		/// invalidates the test [functions cannot be called anymore]
		/// </summary>
		void InValidate();
		/// <summary>
		/// invalidates the test [functions cannot be called anymore]
		/// </summary>
		void InValidatePreExec();

		/// <summary>
		/// returns whether the test is valid and still needs to be run
		/// </summary>
		/// <returns></returns>
		bool IsValid() { return _valid; }

		/// <summary>
		/// returns wether the test is valid and the pipes are initialized
		/// </summary>
		/// <returns></returns>
		bool IsInitialized() { return _valid && _pipeinit; }

		bool PipeError() { return _pipeError; }

		void DeepCopy(std::shared_ptr<Execution> other);

		const int32_t classversion = 0x1;

		
private:
		inline static bool _registeredFactories = false;
		bool _valid = true;
		bool _pipeinit = false;
		bool _avail = false;
		std::atomic_flag _availFlag = ATOMIC_FLAG_INIT;

		size_t __hash = 0;
		friend size_t Hashing::hash(Execution* node);

		size_t GetStaticSize(int32_t version);
		size_t GetDynamicSize();
		virtual bool WriteData(std::ostream* buffer, size_t& offset, size_t length);
		virtual bool ReadData(std::istream* buffer, size_t& offset, size_t length, LoadResolver* resolver);
		int32_t GetType() override
		{
			return FormType::Test;
		}
		static int32_t GetTypeStatic()
		{
			return FormType::Test;
		}
		void Delete() override;
		void Clear() override;
		static void RegisterFactories();

		void FreeMemory() override;
		/// <summary>
		/// returns whether the memmory of this form has been freed
		/// </summary>
		/// <returns></returns>
		bool Freed() override;
		size_t MemorySize() override;

		virtual bool HasChanged() override;
	};
}

