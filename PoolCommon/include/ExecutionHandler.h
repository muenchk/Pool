/*
* Copyright		@Kai Münch		2026
* This is a preexisting file.
*/

#pragma once

#if defined(unix) || defined(__unix__) || defined(__unix)
#	include <unistd.h>
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#	include <Windows.h>
#	include <processthreadsapi.h>
#endif
#include <functional>
#include <string>
#include <memory>
#include <mutex>
#include <thread>
#include <deque>
#include <condition_variable>
#include <list>
#include <semaphore>
#include <queue>

#include "Execution.h"
#include "Serializable.h"
#include "Function.h"

class TaskController;

namespace CommonLib
{

	struct stop_token
	{
	private:
		bool stop = false;

	public:
		void request_stop()
		{
			stop = true;
		}

		bool stop_requested()
		{
			return stop;
		}
	};

	enum class ExecHandlerStatus
	{
		None,
		Sleeping,
		MainLoop,
		StartingTests,
		Exitted,
		Waiting,
		HandlingTests,
		KillingProcessMemory,
		WriteFragment,
		KillingProcessTimeout,
		StoppingTest
	};

	struct ExecutionHandlerSettings
	{

	};

	class ExecutionHandler : public ISerializable
	{
	private:
		/// <summary>
		/// settings of the current session
		/// </summary>
		std::shared_ptr<ExecutionHandlerSettings> _settings;
		/// <summary>
		/// taskcontroller used to execute callbacks
		/// </summary>
		std::shared_ptr<TaskController> _threadpool;
		/// <summary>
		/// maximum amount of concurrently active Executions
		/// </summary>
		int32_t _maxConcurrentExecutions = 1;
		/// <summary>
		/// currently active Executions
		/// </summary>
		std::atomic<int32_t> _currentExecutions = 0;
		/// <summary>
		/// Maximum amount of Executions that can be fully initialized for execution at the same time
		/// </summary>
		int32_t _populationSize = 50;
		/// <summary>
		/// lock for the internal loop and Executions queue synchronisation
		/// </summary>
		std::mutex _lockqueue;
		/// <summary>
		/// condition variable for waiting in the InternalQueue
		/// </summary>
		std::condition_variable _waitforjob;
		/// <summary>
		/// queue for Executions waiting to be initialized for execution
		/// </summary>
		std::deque<std::shared_ptr<Execution>> _waitingExecutions;
		/// <summary>
		/// queue for Executions waiting to be executed
		/// </summary>
		std::deque<std::shared_ptr<Execution>> _waitingExecutionsExec;
		/// <summary>
		/// list holding currently active Executions
		/// </summary>
		std::list<std::shared_ptr<Execution>> _runningExecutions;
		/// <summary>
		///  atomic flag for access to _runningExecutions
		/// </summary>
		std::atomic_flag _runningExecutionsFlag = ATOMIC_FLAG_INIT;
		/// <summary>
		/// internal vector holding temp references to actively handled Executions
		/// </summary>
		std::vector<std::shared_ptr<Execution>> _handleExecutions;
		/// <summary>
		/// Execution currently starting
		/// </summary>
		std::queue<std::shared_ptr<Execution>> _startingExecutions;
		std::mutex _startingLock;
		std::condition_variable _startingLockCond;
		/// <summary>
		/// list holding stopped Executions while handler is frozen
		/// </summary>
		std::list<std::shared_ptr<Execution>> _stoppingExecutions;
		/// <summary>
		/// synchronises access to all top-level functions, including start, stop, wait
		/// </summary>
		std::mutex _toplevelsync;
		/// <summary>
		/// condition used to wait on handler exiting
		/// </summary>
		std::condition_variable _waitforhandler;
		/// <summary>
		/// whether the execution handler is currently running
		/// </summary>
		bool _active = false;
		/// <summary>
		/// next Execution identifier
		/// </summary>
		uint64_t _nextid = 1;
		/// <summary>
		/// whether the instace has been cleared for deletion
		/// </summary>
		bool _cleared = false;
		/// <summary>
		/// freezes execution
		/// </summary>
		bool _freeze = false;
		/// <summary>
		/// tells the freeze strategy to wait for Execution completion
		/// </summary>
		bool _freeze_waitforexecutioncompletion = false;
		/// <summary>
		/// response to freezing
		/// </summary>
		bool _frozen = false;
		/// <summary>
		/// response to freezing
		/// </summary>
		bool _frozenStarter = false;
		/// <summary>
		/// condition used to work with frozen ExecutionStarter
		/// </summary>
		std::condition_variable _freezecond;
		/// <summary>
		/// locks access to _stoppingExecutions to provide a consistent evironment during freeze
		/// </summary>
		std::mutex _freezelock;

		/// <summary>
		/// time handler waits between cycles in nanoseconds
		/// </summary>
		std::chrono::nanoseconds _waittime = std::chrono::nanoseconds(100000000);
		/// <summary>
		/// int-value for waittime
		/// </summary>
		int64_t _waittimeL = 100000000;

		/// <summary>
		/// last time the InternalLoop ran the main loop
		/// </summary>
		std::chrono::steady_clock::time_point _lastExec;

		/// <summary>
		/// whether to enable fragment execution
		/// </summary>
		bool _enableFragments = false;

		/// <summary>
		/// whether to stop the execution handler
		/// </summary>
		bool _stopHandler = false;

		/// <summary>
		/// whether to finish Executions before stopping the handler
		/// </summary>
		bool _finishexecutions = false;

		/// <summary>
		/// indicates that the handler has been stale, forces recomputation of Execution timeouts
		/// </summary>
		bool _stale = false;

		/// <summary>
		/// status of the internal thread;
		/// </summary>
		ExecHandlerStatus _tstatus;

#if defined(unix) || defined(__unix__) || defined(__unix)

#	include <unistd.h>

		std::deque<pid_t> _waitQueue;
		std::condition_variable _waitCond;

		std::mutex _waitLock;
		std::mutex _waitQueueLock;
#endif

		std::thread _thread;

		std::shared_ptr<stop_token> _threadStopToken;

		std::thread _threadTS;

		std::shared_ptr<stop_token> _threadTSStopToken;

		std::thread _threadExecution;

		std::shared_ptr<stop_token> _threadExecutionStopToken;

		void InternalLoop(std::shared_ptr<stop_token> stoken);

		void ExecutionStarter(std::shared_ptr<stop_token> stoken);

		void ExecutionWaiter(std::shared_ptr<stop_token> stoken);

		bool StartExecution(std::shared_ptr<Execution> exec);

		void StopExecution(std::shared_ptr<Execution> exec);

		/// <summary>
		/// class version
		/// </summary>
		const uint32_t version = 0x1;

		void InitExecutionsLockFree();

		struct LoadData
		{
			std::vector<FormID> ids;
		};

		LoadData* _loadData = nullptr;

	public:
		ExecutionHandler();
		~ExecutionHandler();

		void Init(std::shared_ptr<TaskController> threadpool, int32_t maxConcurrentExecutions);

		static ExecutionHandler* GetSingleton();

		/// <summary>
		/// enables execution of Execution fragments
		/// </summary>
		/// <param name="enable"></param>
		void SetEnableFragments(bool enable = true);

		/// <summary>
		/// Changes the maximum number of Executions run concurrently
		/// </summary>
		/// <param name="maxConcurrentExecutions"></param>
		void SetMaxConcurrentExecutions(int32_t maxConcurrentExecutions);

		/// <summary>
		/// starts the handler
		/// </summary>
		void StartHandler();
		/// <summary>
		/// resets the internal thread
		/// </summary>
		void ReinitHandler();
		/// <summary>
		/// starts the handler without invalidating existing Executions
		/// </summary>
		void StartHandlerAsIs();
		/// <summary>
		/// stops the handler after all Executions have been completed.
		/// Controll will not return until the handler stops
		/// </summary>
		void StopHandlerAfterExecutionsFinishAndWait();
		/// <summary>
		/// tells the handler to terminate [kills all running Executions]
		/// </summary>
		void StopHandler();
		/// <summary>
		/// waits for the handler to actually terminate
		/// </summary>
		void WaitOnHandler();

		/// <summary>
		/// returns whether the internal thread is stale, i.e. the last main loop execution is older than [dur]
		/// </summary>
		/// <param name="dur"></param>
		/// <returns></returns>
		bool IsStale(std::chrono::milliseconds dur);

		/// <summary>
		/// Returns the number of currently waiting Executions
		/// </summary>
		/// <returns></returns>
		size_t WaitingTasks() { return _waitingExecutions.size() + _waitingExecutionsExec.size() + _runningExecutions.size() + _stoppingExecutions.size() + _startingExecutions.size(); }

		/// <summary>
		/// sets the period of the Execution engine
		/// </summary>
		/// <param name="period"></param>
		void SetPeriod(std::chrono::nanoseconds period);

		/// <summary>
		/// Adds a new Execution to the queue [passes of Execution handling to external application/script]
		/// </summary>
		/// <param name="input">The input to run the Execution on</param>
		/// <param name="callback">called after the Execution has been finished</param>
		/// <param name="bypass">Bypass queue and start as soon as possible</param>
		bool AddTest(std::shared_ptr<IInput> input, std::shared_ptr<Functions::BaseFunction> callback, bool bypass = false, bool replay = false);

		void InitExecutions();

		/// <summary>
		/// Freezes execution
		/// </summary>
		void Freeze(bool waitfortestcompletion);
		/// <summary>
		/// Resumes execution
		/// </summary>
		void Thaw();
		/// <summary>
		/// returns whether the executionhandler is currently frozen
		/// </summary>
		/// <returns></returns>
		bool IsFrozen();

		/// <summary>
		/// Clears all active and waiting Executions
		/// </summary>
		void ClearExecutions();

		/// <summary>
		/// Returns the number of waiting Executions
		/// </summary>
		/// <returns></returns>
		int32_t GetWaitingExecutions();
		/// <summary>
		/// Returns the number of waiting Executions
		/// </summary>
		/// <returns></returns>
		int32_t GetInitializedExecutions();
		/// <summary>
		/// Returns the number of running Executions
		/// </summary>
		/// <returns></returns>
		int32_t GetRunningExecutions();
		/// <summary>
		/// Returns the number of stopping Executions
		/// </summary>
		/// <returns></returns>
		int32_t GetStoppingExecutions();

		/// <summary>
		/// returns the status if the internal thread
		/// </summary>
		/// <returns></returns>
		ExecHandlerStatus GetThreadStatus();

		size_t GetDataSize() override;
		bool WriteData(Buffer::ArrayBuffer& buffer) override;
		bool ReadData(std::istream* buffer, size_t& offset, size_t length, LoadResolver* resolver) override;
		/// <summary>
		/// Early initialiazation pass [is executed once all forms have been loaded
		/// </summary>
		/// <param name="resolver"></param>
		void InitializeEarly(LoadResolver* resolver) override;
		/// <summary>
		/// late initialization pass [is executed once all Initialize Early has been called for all forms]
		/// </summary>
		/// <param name="resolver"></param>
		void InitializeLate(LoadResolver* resolver) override;
		static int32_t GetTypeStatic()
		{
			return FormType::ExecutionHandler;
		}
		int32_t GetType() override
		{
			return FormType::ExecutionHandler;
		}
		inline static bool _registeredFactories = false;
		static void RegisterFactories();

		void Delete(Data*) override;
		/// <summary>
		/// clears all internals and releases shared pointers
		/// DO NOT CALL BEFORE WAITING ON HANDLER!
		/// </summary>
		void Clear() override;
		void ClearChanged() override;
	};
}
