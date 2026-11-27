/*
* Copyright		@Kai Münch		2026
* This is a preexisting file.
*/

#include <atomic>
#include <queue>
#include <exception>

#include "Data.h"
#include "TaskController.h"
#include "Threading.h"
#include "BufferOperations.h"
#include "Logging.h"
#include "Settings.h"

namespace CommonLib
{

	TaskController* TaskController::GetSingleton()
	{
		static TaskController controller;
		return std::addressof(controller);
	}

	/*
void TaskController::AddTask(std::shared_ptr<Functions::BaseFunction> a_task)
{
	if (a_task) {
			if (_optimizeFuncExec && a_task->GetFunctionType() == Functions::FunctionType::Light) {
				std::unique_lock<std::mutex> guard(_lock);
				_tasks_light.push_back(a_task);
				_condition_light.notify_one();
			} else if (_optimizeFuncExec && a_task->GetFunctionType() == Functions::FunctionType::Medium) {
				std::unique_lock<std::mutex> guard(_lock);
				_tasks_medium.push_back(a_task);
			} else {
				std::unique_lock<std::mutex> guard(_lock);
				_tasks.push_back(a_task);
			}
			_condition.notify_one();
		} else {
			logcritical("Tried to add nullptr as task to taskcontroller");
		}
}*/

	void TaskController::Start(std::shared_ptr<Settings> settings, int32_t numthreads)
	{
		_settings = settings;
		if (numthreads == 0)
			throw std::runtime_error("Cannot start a TaskController with 0 threads.");
		_numThreads = numthreads;
		if (_numThreads < 1)
			_numThreads = 1;
		if (_numThreads == 1) {
			_numLightThreads = 0;
			_numMediumThreads = 0;
			_numHeavyThreads = 0;
			_numAllThreads = 1;
			_controlEnableFine = false;
			_status.push_back(ThreadStatus::Initializing);
			_statusTime.push_back(std::chrono::steady_clock::now());
			_statusTask.push_back("None");
			_threads.emplace_back(std::thread(&TaskController::InternalLoop_SingleThread, this, 0));
		} else {
			_controlEnableFine = true;
			_controlEnableLight = true;
			_controlEnableMedium = false;
			_controlEnableHeavy = true;
			_numLightThreads = 1;
			_numMediumThreads = 0;
			_numHeavyThreads = _numThreads - 1;
			_numAllThreads = 0;
			for (int32_t i = 0; i < numthreads; i++) {
				_status.push_back(ThreadStatus::Initializing);
				_statusTime.push_back(std::chrono::steady_clock::now());
				_statusTask.push_back("None");
				if (i == 0)
					_threads.emplace_back(std::thread(&TaskController::InternalLoop_LightExclusive, this, i));
				else
					_threads.emplace_back(std::thread(&TaskController::InternalLoop_Heavy, this, i));
			}
		}
	}

	void TaskController::Start(std::shared_ptr<Settings> settings, int32_t numLightThreads, int32_t numMediumThreads, int32_t numHeavyThreads, int32_t numAllThreads)
	{
		_settings = settings;
		if (numHeavyThreads == 0)
			throw std::runtime_error("Cannot start a TaskController with 0 heavy threads.");
		_numLightThreads = numLightThreads;
		_numMediumThreads = numMediumThreads;
		_numHeavyThreads = numHeavyThreads;
		_numAllThreads = numAllThreads;
		if (_numAllThreads == 0)
			_numThreads = numLightThreads + numMediumThreads + numHeavyThreads;
		else {
			_numThreads = numAllThreads;
			_numLightThreads = 0;
			_numMediumThreads = 0;
			_numHeavyThreads = 0;
		}
		if (_numThreads < 1 || _numThreads == 1) {
			_numThreads = 1;
			_numAllThreads = 1;
			_numLightThreads = 0;
			_numMediumThreads = 0;
			_numHeavyThreads = 0;
		}
		int32_t i = 0;
		if (_numThreads == 1 || _numAllThreads > 0) {
			_numLightThreads = 0;
			_numMediumThreads = 0;
			_numHeavyThreads = 0;
			_controlEnableFine = false;
			_enableCustomAllocators = true;
			for (int32_t c = 0; c < _numAllThreads; c++, i++) {
				_status.push_back(ThreadStatus::Initializing);
				_statusTime.push_back(std::chrono::steady_clock::now());
				_statusTask.push_back("None");
				_threads.emplace_back(std::thread(&TaskController::InternalLoop_SingleThread, this, i));
			}
		} else {
			_controlEnableFine = true;
			if (_numLightThreads > 0)
				_controlEnableLight = true;
			for (int32_t c = 0; c < _numLightThreads; c++, i++) {
				_status.push_back(ThreadStatus::Initializing);
				_statusTime.push_back(std::chrono::steady_clock::now());
				_statusTask.push_back("None");
				_threads.emplace_back(std::thread(&TaskController::InternalLoop_LightExclusive, this, i));
			}
			if (_numMediumThreads > 0)
				_controlEnableMedium = true;
			for (int32_t c = 0; c < _numMediumThreads; c++, i++) {
				_status.push_back(ThreadStatus::Initializing);
				_statusTime.push_back(std::chrono::steady_clock::now());
				_statusTask.push_back("None");
				_threads.emplace_back(std::thread(&TaskController::InternalLoop_Medium, this, i));
			}
			if (_numHeavyThreads > 0)
				_controlEnableHeavy = true;
			for (int32_t c = 0; c < _numHeavyThreads; c++, i++) {
				_status.push_back(ThreadStatus::Initializing);
				_statusTime.push_back(std::chrono::steady_clock::now());
				_statusTask.push_back("None");
				_threads.emplace_back(std::thread(&TaskController::InternalLoop_Heavy, this, i));
			}
		}
	}

	int32_t TaskController::GetHeavyThreadCount()
	{
		return _numHeavyThreads + _numAllThreads;
	}

	int32_t TaskController::GetMediumThreadCount()
	{
		return _numMediumThreads + _numAllThreads;
	}

	int32_t TaskController::GetLightThreadCount()
	{
		// all of the below can execute light threads and will do so first if some are available
		return (_numLightThreads + _numAllThreads);
	}

	void TaskController::Stop(bool completeall)
	{
		{
			std::unique_lock<std::mutex> guard(_lock);
			_terminate = true;
			_wait = completeall;
		}
		_condition.notify_all();
		_condition_light.notify_all();
		for (std::thread& t : _threads)
			t.join();
		_threads.clear();
		while (_tasks.empty() == false) {
			_tasks.front()->Dispose();
			_tasks.pop_front();
		}
		while (_tasks_light.empty() == false) {
			_tasks_light.front()->Dispose();
			_tasks_light.pop_front();
		}
		while (_tasks_medium.empty() == false) {
			_tasks_medium.front()->Dispose();
			_tasks_medium.pop_front();
		}
	}

	TaskController::~TaskController()
	{
		if (_terminate == false) {
			// we are exiting unexpectedly
			// set terminate and detach the threads so we do not have to wait on them
			_terminate = true;
			for (auto& thread : _threads)
				thread.detach();
			while (_tasks.empty() == false) {
				_tasks.front()->Dispose();
				_tasks.pop_front();
			}
			while (_tasks_light.empty() == false) {
				_tasks_light.front()->Dispose();
				_tasks_light.pop_front();
			}
			while (_tasks_medium.empty() == false) {
				_tasks_medium.front()->Dispose();
				_tasks_medium.pop_front();
			}
		}
	}

	bool TaskController::Busy()
	{
		bool running = true;
		for (int32_t i = 0; i < (int32_t)_status.size(); i++)
			running &= _status[i] == ThreadStatus::Running;
		return !_tasks.empty() || running;
	}

	void TaskController::InternalLoop_LightExclusive(int32_t number)
	{
		_status[number] = ThreadStatus::Waiting;
		while (true) {
			std::shared_ptr<Functions::BaseFunction> del;
			{
				std::unique_lock<std::mutex> guard(_lockLight);
				// while freeze is [true], this will never return, if freeze is [false] it only returns when [tasks is non-empty], when [terinated and not waiting], or when [terminating and tasks is empty]
				_condition_light.wait(guard, [this] { return _freeze == false && (!_tasks_light.empty() || _terminate && _wait == false || _terminate && _tasks_light.empty()); });
				if (_terminate && _wait == false || _terminate && _tasks_light.empty())
					return;
				del = _tasks_light.front();
				_tasks_light.pop_front();
			}
			if (del) {
				_status[number] = ThreadStatus::Running;
				_statusTime[number] = std::chrono::steady_clock::now();
				_statusTask[number] = del->GetName();
				del->Run();
				del->Dispose();
				_completedjobs++;
			}
			_status[number] = ThreadStatus::Waiting;
		}
	}

	void TaskController::InternalLoop_Medium(int32_t number)
	{
		_status[number] = ThreadStatus::Waiting;
		while (true) {
			std::shared_ptr<Functions::BaseFunction> del;
			{
				std::unique_lock<std::mutex> guard(_lockMedium);
				// while freeze is [true], this will never return, if freeze is [false] it only returns when [tasks is non-empty], when [terinated and not waiting], or when [terminating and tasks is empty]
				_condition_medium.wait(guard, [this] { return _freeze == false && (!_tasks_medium.empty() || _terminate && _wait == false || _terminate && _tasks_medium.empty()); });
				if (_terminate && _wait == false || _terminate && _tasks_medium.empty())
					return;
				if (!_tasks_medium.empty()) {
					del = _tasks_medium.front();
					_tasks_medium.pop_front();
				}
			}
			if (del) {
				_status[number] = ThreadStatus::Running;
				_statusTime[number] = std::chrono::steady_clock::now();
				_statusTask[number] = del->GetName();
				del->Run();
				del->Dispose();
				_completedjobs++;
			}
			_status[number] = ThreadStatus::Waiting;
		}
	}

	void TaskController::InternalLoop_Heavy(int32_t number)
	{
		_status[number] = ThreadStatus::Waiting;
		while (true) {
			std::shared_ptr<Functions::BaseFunction> del;
			{
				std::unique_lock<std::mutex> guard(_lock);
				// while freeze is [true], this will never return, if freeze is [false] it only returns when [tasks is non-empty], when [terinated and not waiting], or when [terminating and tasks is empty]
				_condition.wait(guard, [this] { return _freeze == false && (!_tasks.empty() || _terminate && _wait == false || _terminate && _tasks.empty()); });
				if (_terminate && _wait == false || _terminate && _tasks.empty())
					return;
				if (!_tasks.empty()) {
					del = _tasks.front();
					_tasks.pop_front();
				}
			}
			if (del) {
				_status[number] = ThreadStatus::Running;
				_statusTime[number] = std::chrono::steady_clock::now();
				_statusTask[number] = del->GetName();
				del->Run();
				del->Dispose();
				_completedjobs++;
			}
			_status[number] = ThreadStatus::Waiting;
		}
	}

	void TaskController::InternalLoop_SingleThread(int32_t number)
	{
		_status[number] = ThreadStatus::Waiting;
		while (true) {
			std::shared_ptr<Functions::BaseFunction> del;
			{
				std::unique_lock<std::mutex> guard(_lock);
				// while freeze is [true], this will never return, if freeze is [false] it only returns when [tasks is non-empty], when [terinated and not waiting], or when [terminating and tasks is empty]
				_condition.wait_for(guard, std::chrono::milliseconds(100), [this] { return _freeze == false && (!_tasks_light.empty() || !_tasks_medium.empty() || !_tasks.empty() || _terminate && _wait == false || _terminate && _tasks_light.empty() && _tasks_medium.empty() && _tasks.empty()); });
				if (_terminate && _wait == false || _terminate && _tasks_light.empty() && _tasks_medium.empty() && _tasks.empty())
					return;
				if (_freeze)
					continue;
				if (!_tasks_light.empty()) {
					del = _tasks_light.front();
					_tasks_light.pop_front();
				} else if (!_tasks_medium.empty()) {
					del = _tasks_medium.front();
					_tasks_medium.pop_front();
				} else if (!_tasks.empty()) {
					del = _tasks.front();
					_tasks.pop_front();
				}
			}
			if (del) {
				_status[number] = ThreadStatus::Running;
				_statusTime[number] = std::chrono::steady_clock::now();
				_statusTask[number] = del->GetName();
#ifndef NDEBUG
				{
					std::unique_lock<std::mutex> finLock(_finishedTasksLock);
					int32_t type = (int32_t)del->GetType();
					std::string name = std::string(del->GetName() /*typeid(T).name()) + " | " + std::string((char*)&type* 4*/);
					auto itr = _finishedTasks.find(name);
					if (itr != _finishedTasks.end())
						_finishedTasks.insert_or_assign(name, itr->second + 1);
					else
						_finishedTasks.insert_or_assign(name, 1);
				}
#endif
				del->Run();
				del->Dispose();
				_completedjobs++;
			}
			_status[number] = ThreadStatus::Waiting;
		}
	}

	uint64_t TaskController::GetCompletedJobs()
	{
		return _completedjobs;
	}

	int32_t TaskController::GetWaitingJobs()
	{
		return (int32_t)_tasks.size() + (int32_t)_tasks_medium.size() + (int32_t)_tasks_light.size();
	}

	int32_t TaskController::GetWaitingLightJobs()
	{
		return (int32_t)_tasks_light.size();
	}

	int32_t TaskController::GetWaitingMediumJobs()
	{
		return (int32_t)_tasks_medium.size();
	}

	int32_t TaskController::GetWaitingHeavyJobs()
	{
		return (int32_t)_tasks.size();
	}

	size_t TaskController::GetDataSize()
	{
		static size_t sz = ISerializable::GetDataSize() + 4  // version
		                   + 1                               // terminate
		                   + 1                               // wait
		                   + 1                               // active
		                   + 4                               // numthreads
		                   + 4                               // _numLightThreads
		                   + 4                               // _numMediumThreads
		                   + 4                               // _numHeavyThreads
		                   + 4                               // _numAllThreads
		                   + 1                               // _controlEnableFine
		                   + 1                               // _controlEnableLight
		                   + 1                               // _controlEnableMedium
		                   + 1                               // _controlEnableHeavy
		                   + 8                               // size of waiting tasks
		                   + 8                               // size of waiting tasks_medium
		                   + 8                               // size of waiting tasks_light
		                   + 8;                              // completedjobs

		for (auto task : _tasks) {
			if (task != nullptr) {
				sz += task->GetLength();
			}
		}
		for (auto task : _tasks_medium) {
			if (task != nullptr) {
				sz += task->GetLength();
			}
		}
		for (auto task : _tasks_light) {
			if (task != nullptr) {
				sz += task->GetLength();
			}
		}
		sz += 8;
		for (auto [str, val] : _executedTasks) {
			sz += Buffer::CalcStringLength(str) + 8;
		}
		return sz;
	}

	bool TaskController::WriteData(Buffer::ArrayBuffer& buffer)
	{
		SAVE_WRITE_EXPR(ISerializable::WriteData(buffer));

		SAVE_WRITE(buffer, uint32_t, version);
		SAVE_WRITE(buffer, bool, _terminate);
		SAVE_WRITE(buffer, bool, _wait);
		SAVE_WRITE(buffer, bool, _threads.size() > 0);
		SAVE_WRITE(buffer, int32_t, _numThreads);
		SAVE_WRITE(buffer, int32_t, _numLightThreads);
		SAVE_WRITE(buffer, int32_t, _numMediumThreads);
		SAVE_WRITE(buffer, int32_t, _numHeavyThreads);
		SAVE_WRITE(buffer, int32_t, _numAllThreads);
		SAVE_WRITE(buffer, bool, _controlEnableFine);
		SAVE_WRITE(buffer, bool, _controlEnableLight);
		SAVE_WRITE(buffer, bool, _controlEnableMedium);
		SAVE_WRITE(buffer, bool, _controlEnableHeavy);
		SAVE_WRITE(buffer, size_t, _tasks.size());

		for (auto task : _tasks) {
			SAVE_WRITE_EXPR(task->WriteData(buffer));
		}
		SAVE_WRITE(buffer, size_t, _tasks_medium.size());
		for (auto task : _tasks_medium) {
			SAVE_WRITE_EXPR(task->WriteData(buffer));
		}
		SAVE_WRITE(buffer, size_t, _tasks_light.size());
		for (auto task : _tasks_light) {
			SAVE_WRITE_EXPR(task->WriteData(buffer));
		}
		SAVE_WRITE(buffer, uint64_t, _completedjobs.load());

		SAVE_WRITE(buffer, size_t, _executedTasks.size());
		for (auto [str, val] : _executedTasks) {
			SAVE_WRITE(buffer, std::string, str);
			SAVE_WRITE(buffer, int64_t, val);
		}
		return true;
	}

	bool TaskController::ReadData(Buffer::ArrayBuffer& buffer, LoadResolver* resolver)
	{
		SetChanged();
		ClearTasks();

		ISerializable::ReadData(buffer, resolver);

		int32_t version = buffer.Read<uint32_t>();
		switch (version) {
		case 0x1:
			{
				_terminate = buffer.Read<bool>();
				_wait = buffer.Read<bool>();
				bool active = buffer.Read<bool>();
				_numThreads = buffer.Read<int32_t>();
				_numLightThreads = buffer.Read<int32_t>();
				_numMediumThreads = buffer.Read<int32_t>();
				_numHeavyThreads = buffer.Read<int32_t>();
				_numAllThreads = buffer.Read<int32_t>();
				_controlEnableFine = buffer.Read<bool>();
				_controlEnableLight = buffer.Read<bool>();
				_controlEnableMedium = buffer.Read<bool>();
				_controlEnableHeavy = buffer.Read<bool>();
				size_t num = buffer.Read<size_t>();
				for (int32_t i = 0; i < (int32_t)num; i++) {
					std::shared_ptr<Functions::BaseFunction> func = Functions::BaseFunction::Create(buffer, resolver);
					if (!CmdArgs::_clearTasks && resolver->finalsave)
						_tasks.push_back(func);
					else
						func->Dispose();
				}
				num = buffer.Read<size_t>();
				for (int32_t i = 0; i < (int32_t)num; i++) {
					std::shared_ptr<Functions::BaseFunction> func = Functions::BaseFunction::Create(buffer, resolver);
					if (!CmdArgs::_clearTasks && resolver->finalsave)
						_tasks_medium.push_back(func);
					else
						func->Dispose();
				}
				num = buffer.Read<size_t>();
				for (int32_t i = 0; i < (int32_t)num; i++) {
					std::shared_ptr<Functions::BaseFunction> func = Functions::BaseFunction::Create(buffer, resolver);
					if (!CmdArgs::_clearTasks && resolver->finalsave)
						_tasks_light.push_back(func);
					else
						func->Dispose();
				}
				_completedjobs = buffer.Read<uint64_t>();
				if (active)
					//Start(_numthreads);
					active = false;
				num = buffer.Read<size_t>();
				for (size_t i = 0; i < num; i++) {
					std::string str = buffer.Read<std::string>();
					int64_t val = buffer.Read<int64_t>();
					_executedTasks.insert_or_assign(str, val);
				}
				return true;
			}
			break;
		default:
			return false;
		}
	}

	void TaskController::InitializeEarly(LoadResolver* /*resolver*/)
	{
	}

	void TaskController::InitializeLate(LoadResolver* /*resolver*/)
	{
	}

	void TaskController::Delete()
	{
		Clear();
	}

	void TaskController::Clear()
	{
		ISerializable::ClearForm();
		_terminate = true;
		_wait = false;
		_condition.notify_all();
		_condition_light.notify_all();
		for (std::thread& t : _threads)
			t.detach();
		_threads.clear();
		while (_tasks.empty() == false) {
			if (_tasks.front())
				_tasks.front()->Dispose();
			_tasks.pop_front();
		}
		while (_tasks_light.empty() == false) {
			if (_tasks_light.front())
				_tasks_light.front()->Dispose();
			_tasks_light.pop_front();
		}
		while (_tasks_medium.empty() == false) {
			if (_tasks_medium.front())
				_tasks_medium.front()->Dispose();
			_tasks_medium.pop_front();
		}
	}

	void TaskController::RegisterFactories()
	{
		if (!_registeredFactories) {
			_registeredFactories = !_registeredFactories;
		}
	}

	size_t TaskController::MemorySize()
	{
		return sizeof(TaskController);
	}

	void TaskController::ClearChanged()
	{
		ISerializable::ClearChanged();
		SetChanged();
	}

	void TaskController::RequestFreeze()
	{
		loginfo("Requesting freezing of execution...");
		// set freezing to true
		_freeze = true;
	}

	void TaskController::Freeze()
	{
		loginfo("Freezing execution...");
		// set freezing to true
		_freeze = true;
		// wait for all threads to freeze
		bool frozen = false;
		while (frozen == false && _threads.size() > 0) {
			frozen = true;
			for (int32_t i = 0; i < (int32_t)_status.size(); i++)
				frozen &= _status[i] == ThreadStatus::Waiting;
		}
		//_lock.lock();
		//_lockLight.lock();
		//_lockMedium.lock();
		loginfo("Frozen execution.");
	}

	bool TaskController::IsFrozen()
	{
		return _freeze;
	}

	void TaskController::Thaw()
	{
		loginfo("Thawing execution...");
		_freeze = false;
		//_lock.unlock();
		//_lockLight.unlock();
		//_lockMedium.unlock();
		_condition.notify_all();
		_condition_light.notify_all();
		_condition_medium.notify_all();
		loginfo("Resumed execution.");
	}

	void TaskController::ClearTasks()
	{
		bool locks = false;
		if (!IsFrozen()) {
			// was not frozen so get lock first
			_lock.lock();
			_lockLight.lock();
			_lockMedium.lock();
			locks = true;
		}
		while (_tasks.empty() == false) {
			_tasks.front()->Dispose();
			_tasks.pop_front();
		}
		while (_tasks_light.empty() == false) {
			_tasks_light.front()->Dispose();
			_tasks_light.pop_front();
		}
		while (_tasks_medium.empty() == false) {
			_tasks_medium.front()->Dispose();
			_tasks_medium.pop_front();
		}
		if (locks) {
			_lock.unlock();
			_lockLight.unlock();
			_lockMedium.unlock();
		}
	}

	void TaskController::GetThreadStatus(std::vector<ThreadStatus>& status, std::vector<const char*>& names, std::vector<std::string>& time)
	{
		if (status.size() < _status.size()) {
			status.resize(_status.size());
			names.resize(_status.size());
			time.resize(_status.size());
		}
		auto now = std::chrono::steady_clock::now();
		for (int32_t i = 0; i < (int32_t)status.size(); i++) {
			status[i] = _status[i];
			names[i] = _statusTask[i];
			time[i] = Logging::FormatTime(std::chrono::duration_cast<std::chrono::microseconds>(now - _statusTime[i]).count());
		}
	}

	std::pair<bool, uint64_t> TaskController::IsActive()
	{
		uint64_t diff = _completedjobs.load() - lasttasks;
		lasttasks = _completedjobs.load();
		if (diff != 0) {
			//logmessage("task: completes");
			return { true, diff };
		}
		for (auto stat : _status) {
			if (stat != ThreadStatus::Waiting) {
				//logmessage("task: running");
				return { true, diff };
			}
		}
		if (_tasks_light.size() > 0 || _tasks_medium.size() > 0 || _tasks.size() > 0) {
			//logmessage("task: waiting");
			return { true, diff };
		}
		if (lasttasks != _completedjobs.load()) {
			lasttasks = _completedjobs.load();
			//logmessage("task: completex");
			return { true, diff };
		}
		return { false, diff };
	}

	int64_t TaskController::Working()
	{
		int64_t sum = 0;
		for (auto stat : _status) {
			if (stat != ThreadStatus::Waiting) {
				sum++;
			}
		}
		return sum;
	}
}
