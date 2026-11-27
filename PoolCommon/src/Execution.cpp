/*
*  Copyright @Kai Münch, University of Potsdam
* Derived from Test.cpp from TimeFuzz
*/

#include "Execution.h"
#include "Input.h"
#include "Serializable.h"
#include "Processes.h"

#include "Hashing.h"

#define PIPE_SIZE 1048576

#if defined(unix) || defined(__unix__) || defined(__unix)
#	include <poll.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <fcntl.h>
#	include <unistd.h>
#	include <time.h>
#	include <stdlib.h>
#	include <stdio.h>
#	ifndef __USE_TIME64_REDIRECTS
#		ifndef __USE_FILE_OFFSET64
extern int fcntl(int __fd, int __cmd, ...);
#		else
#			ifdef __REDIRECT
extern int __REDIRECT(fcntl, (int __fd, int __cmd, ...), fcntl64);
#			else
#				define fcntl fcntl64
#			endif
#		endif
#		ifdef __USE_LARGEFILE64
extern int fcntl64(int __fd, int __cmd, ...);
#		endif
#	else /* __USE_TIME64_REDIRECTS */
#		ifdef __REDIRECT
extern int __REDIRECT_NTH(fcntl, (int __fd, int __request, ...),
	__fcntl_time64);
extern int __REDIRECT_NTH(fcntl64, (int __fd, int __request, ...),
	__fcntl_time64);
#		else
extern int __fcntl_time64(int __fd, int __request, ...) __THROW;
#			define fcntl64 __fcntl_time64
#			define fcntl __fcntl_time64
#		endif
#	endif

#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#	include <Psapi.h>
#	include <Windows.h>
#endif

namespace CommonLib
{

	Execution::Execution(std::shared_ptr<Functions::BaseFunction> a_callback, uint64_t id) :
		_callback(a_callback)
	{
		_identifier = id;
		Init();
	}

	Execution::~Execution()
	{
		Clear();
	}

	void Execution::Init(std::shared_ptr<Functions::BaseFunction> a_callback, uint64_t id)
	{
		_callback = a_callback;
		_identifier = id;
		_valid = true;
	}

	void Execution::Init()
	{
		StartProfiling;

#if defined(unix) || defined(__unix__) || defined(__unix)
		if (pipe2(red_output, O_NONBLOCK == -1)) {
			_exitreason = ExitReason::InitError;
			return;
		}
		//fcntl64(red_output[0], F_SETFL, O_NONBLOCK);
		//fcntl64(red_output[1], F_SETFL, O_NONBLOCK);
		if (pipe2(red_input, O_NONBLOCK == -1)) {
			close(red_output[0]);
			red_output[0] = -1;
			close(red_output[1]);
			red_output[1] = -1;
			_exitreason = ExitReason::InitError;
			return;
		}
		//fcntl(red_input[0], F_SETPIPE_SZ, PIPE_SIZE_LINUX);
		//fcntl(red_input[1], F_SETPIPE_SZ, PIPE_SIZE_LINUX);
		//fcntl(red_output[0], F_SETPIPE_SZ, PIPE_SIZE_LINUX);
		//fcntl(red_output[1], F_SETPIPE_SZ, PIPE_SIZE_LINUX);
		//fcntl64(red_input[0], F_SETFL, O_NONBLOCK);
		//fcntl64(red_input[1], F_SETFL, O_NONBLOCK);
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
		std::string tmpname = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
		std::string pipe_name_input = "\\\\.\\pipe\\" + std::to_string(intptr_t(this)) + tmpname + "inp";
		std::string pipe_name_output = "\\\\.\\pipe\\" + std::to_string(intptr_t(this)) + tmpname + "out";
		// create pipes for _output

		// set security
		SECURITY_ATTRIBUTES saAttr;
		ZeroMemory(&saAttr, sizeof(saAttr));
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = FALSE;
		saAttr.lpSecurityDescriptor = NULL;
		// create pipes
		red_output[0] = CreateNamedPipeA(
			pipe_name_output.c_str(),
			PIPE_ACCESS_INBOUND,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT,
			1,
			PIPE_SIZE,
			PIPE_SIZE,
			0,
			&saAttr);
		if (red_output[0] == INVALID_HANDLE_VALUE) {
			logcritical("failed to create named pipe {}", GetLastError());
			_exitreason = ExitReason::InitError;
			return;
		}
		ZeroMemory(&saAttr, sizeof(saAttr));
		saAttr.nLength = sizeof(saAttr);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;
		red_output[1] = CreateFileA(pipe_name_output.c_str(), GENERIC_WRITE, 0, &saAttr, OPEN_EXISTING, 0, NULL);
		if (red_output[1] == INVALID_HANDLE_VALUE) {
			logcritical("failed to create event handle {}", GetLastError());
			_exitreason = ExitReason::InitError;
			if (red_output[0] != INVALID_HANDLE_VALUE) {
				CloseHandle(red_output[0]);
				red_output[0] = INVALID_HANDLE_VALUE;
			}
			return;
		}

		BOOL success = ConnectNamedPipe(red_output[0], NULL);
		if (!success && GetLastError() != ERROR_PIPE_CONNECTED) {
			logcritical("cannot connect to named pipe {}", GetLastError());
			_exitreason = ExitReason::InitError;
			if (red_output[0] != INVALID_HANDLE_VALUE) {
				CloseHandle(red_output[0]);
				red_output[0] = INVALID_HANDLE_VALUE;
			}
			if (red_output[1] != INVALID_HANDLE_VALUE) {
				CloseHandle(red_output[1]);
				red_output[1] = INVALID_HANDLE_VALUE;
			}
			return;
		}

		// create pipes for _input
		// set security
		ZeroMemory(&saAttr, sizeof(saAttr));
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;
		// create pipes
		red_input[0] = CreateNamedPipeA(
			pipe_name_input.c_str(),
			PIPE_ACCESS_INBOUND,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,  // | PIPE_NOWAIT,
			1,
			PIPE_SIZE,
			PIPE_SIZE,
			0,
			&saAttr);
		if (red_input[0] == INVALID_HANDLE_VALUE) {
			logcritical("failed to create named pipe {}", GetLastError());
			_exitreason = ExitReason::InitError;
			if (red_output[0] != INVALID_HANDLE_VALUE) {
				CloseHandle(red_output[0]);
				red_output[0] = INVALID_HANDLE_VALUE;
			}
			if (red_output[1] != INVALID_HANDLE_VALUE) {
				CloseHandle(red_output[1]);
				red_output[1] = INVALID_HANDLE_VALUE;
			}
			return;
		}
		ZeroMemory(&saAttr, sizeof(saAttr));
		saAttr.nLength = sizeof(saAttr);
		saAttr.bInheritHandle = FALSE;
		saAttr.lpSecurityDescriptor = NULL;
		red_input[1] = CreateFileA(pipe_name_input.c_str(), GENERIC_WRITE, 0, &saAttr, OPEN_EXISTING, 0, NULL);
		if (red_input[1] == INVALID_HANDLE_VALUE) {
			logcritical("failed to create event handle {}", GetLastError());
			_exitreason = ExitReason::InitError;
			if (red_output[0] != INVALID_HANDLE_VALUE) {
				CloseHandle(red_output[0]);
				red_output[0] = INVALID_HANDLE_VALUE;
			}
			if (red_output[1] != INVALID_HANDLE_VALUE) {
				CloseHandle(red_output[1]);
				red_output[1] = INVALID_HANDLE_VALUE;
			}
			if (red_input[0] != INVALID_HANDLE_VALUE) {
				CloseHandle(red_input[0]);
				red_input[0] = INVALID_HANDLE_VALUE;
			}
			return;
		}

		success = ConnectNamedPipe(red_input[0], NULL);
		if (!success && GetLastError() != ERROR_PIPE_CONNECTED) {
			logcritical("cannot connect to named pipe {}", GetLastError());
			_exitreason = ExitReason::InitError;
			if (red_output[0] != INVALID_HANDLE_VALUE) {
				CloseHandle(red_output[0]);
				red_output[0] = INVALID_HANDLE_VALUE;
			}
			if (red_output[1] != INVALID_HANDLE_VALUE) {
				CloseHandle(red_output[1]);
				red_output[1] = INVALID_HANDLE_VALUE;
			}
			if (red_input[0] != INVALID_HANDLE_VALUE) {
				CloseHandle(red_input[0]);
				red_input[0] = INVALID_HANDLE_VALUE;
			}
			if (red_input[1] != INVALID_HANDLE_VALUE) {
				CloseHandle(red_input[1]);
				red_input[1] = INVALID_HANDLE_VALUE;
			}
			return;
		}

		/*
		if (!CreatePipe(&red_output[0], &red_output[1], &saAttr, PIPE_SIZE)) {
			_exitreason = ExitReason::InitError;
			return;
		}
		// set readhandle for _output to not be inherited
		SetHandleInformation(&red_output[0], HANDLE_FLAG_INHERIT, 0);
		if (!CreatePipe(&red_input[0], &red_input[1], &saAttr, PIPE_SIZE)) {
			_exitreason = ExitReason::InitError;
			return;
		}
		// set writehandle for _input to not be inherited
		SetHandleInformation(&red_input[1], HANDLE_FLAG_INHERIT, 0);*/
#endif
		_pipeinit = true;
		_avail = true;
		logprofile(TimeProfiling, "");
	}

	bool Execution::IsRunning()
	{
		StartProfiling;
		if (!_valid) {
			logcritical("called IsRunning after invalidation");
			return false;
		}
#if defined(unix) || defined(__unix__) || defined(__unix)
		SpinlockA guard(_availFlag);
		bool res = Processes::GetProcessRunning(processid, &exitcode);
		if (res == false)
			_avail = false;
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
		bool res = Processes::GetProcessRunning(pi.hProcess);
#endif
		PROF_3(TimeProfiling, "");
		if (res)
			return true;
		else {
			_endtime = std::chrono::steady_clock::now();
			_exitreason = ExitReason::Natural;
			return false;
		}
	}

#if defined(unix) || defined(__unix__) || defined(__unix)
	bool Execution::SendInput(std::string str, bool waitwrite)
#else
	bool Execution::SendInput(std::string str, bool /*waitwrite*/)
#endif
	{
		if (!_valid) {
			logcritical("called SendInput after invalidation");
			return false;
		}
		int bSuccess = 0;
#if defined(unix) || defined(__unix__) || defined(__unix)
		SpinlockA guard(_availFlag);
		if (_avail == false)
			return false;
		size_t len = strlen(str.c_str());
		//if (IsRunning()) {
		struct pollfd fds;
		int32_t events = 0;
		fds.fd = red_input[1];  // stdin
		fds.events = POLLOUT;
		events = poll(&fds, 1, 0);
		// check it it is still running
		bool res = Processes::GetProcessRunning(processid, &exitcode);
		if (res == false) {
			_avail = false;
			return 0;
		}
		if ((fds.revents & POLLOUT) == POLLOUT) {
			if (waitwrite) {
				size_t totalwritten = 0;
				while (totalwritten != len) {
					size_t written = write(red_input[1], str.c_str() + totalwritten, len);
					if (written == -1) {
						if (errno == EPIPE) {
							_pipeError = true;
							//InValidate();
							return false;
						}
					}
					totalwritten += written;
				}
				return true;
			} else {
				if (size_t written = write(red_input[1], str.c_str(), len); written != len) {
					logcritical("Write to child is missing bytes: Supposed {}, written {}", len, written);
					if (written == -1) {
						if (errno == EPIPE) {
							_pipeError = true;
							//InValidate();
						}
					}
					return false;
				}
				return true;
			}
		} else
			return false;
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
		DWORD dwWritten;
		bSuccess = FALSE;
		const char* cstr = str.c_str();
		bSuccess = WriteFile(red_input[1], cstr, (DWORD)strlen(cstr), &dwWritten, NULL);
		if (bSuccess == false) {
			if (auto err = GetLastError(); err == ERROR_BROKEN_PIPE || err == ERROR_BAD_PIPE || err == ERROR_PIPE_NOT_CONNECTED) {
				_pipeError = true;
				//InValidate();
			}
		}
#endif
		return bSuccess;
	}

	long Execution::Send(const char* data, size_t offset, size_t length)
	{
		if (!_valid) {
			logcritical("called Send after invalidation");
			return false;
		}
		int bSuccess = 0;
#if defined(unix) || defined(__unix__) || defined(__unix)
		SpinlockA guard(_availFlag);
		if (_avail == false) {
			return 0;
		}
		//if (IsRunning()) {
		struct pollfd fds;
		int32_t events = 0;
		fds.fd = red_input[1];  // stdin
		fds.events = POLLOUT;
		events = poll(&fds, 1, 0);
		if ((fds.revents & POLLNVAL) || (fds.revents & POLLERR) || (fds.revents & POLLHUP)) {
			_pipeError = true;
			return -1;
		}
		if ((fds.revents & POLLOUT) == POLLOUT) {
			long written = write(red_input[1], data + offset, length);
			if (written == -1) {
				if (errno == EPIPE) {
					_pipeError = true;
					//InValidate();
				}
			}
			return written;
		} else
			return 0;
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
		DWORD dwWritten;
		bSuccess = FALSE;
		bSuccess = WriteFile(red_input[1], data + offset, (DWORD)length, &dwWritten, NULL);
		if (bSuccess)
			return dwWritten;
		else {
			if (auto err = GetLastError(); err == ERROR_BROKEN_PIPE || err == ERROR_BAD_PIPE || err == ERROR_PIPE_NOT_CONNECTED) {
				_pipeError = true;
				//InValidate();
			}
			return 0;
		}
#endif
	}

	bool Execution::CheckInput()
	{
		StartProfiling;
		if (!_valid) {
			logcritical("called CheckInput after invalidation");
			return false;
		}
		bool ret = false;
#if defined(unix) || defined(__unix__) || defined(__unix)
		SpinlockA guard(_availFlag);
		if (_avail == false)
			return false;
		struct pollfd fds;
		int32_t events = 0;
		fds.fd = red_input[0];  // stdin
		fds.events = POLLIN;
		events = poll(&fds, 1, 0);
		if ((fds.revents & POLLIN) == POLLIN) {
			ret = false;
		} else
			ret = true;
		logdebug("CheckInput : {}", ret);
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
		DWORD bytesAvail = 0;
		BOOL bSuccess = PeekNamedPipe(red_input[0], NULL, 0, NULL, &bytesAvail, NULL);
		if (bytesAvail > 0)
			ret = false;
		else
			ret = true;
		/*DWORD dwRead;
	int32_t sl = (int)strlen(_lastwritten.c_str());
	char* chBuf = new char[sl + 1];
	BOOL bSuccess = FALSE;
	bSuccess = ReadFile(red_input[0], chBuf, sl, &dwRead, NULL);
	if (dwRead > 0) {
		bSuccess = WriteFile(red_input[1], chBuf, dwRead, &dwRead, NULL);
		ret = false;
	}
	else
		ret = true;
	logdebug("CheckInput : {}, succ {}, err {}", ret, bSuccess, GetLastError());*/
#endif
		PROF_3(TimeProfiling, "");
		return ret;
	}

	std::string Execution::ReceiveOutput()
	{
		StartProfiling;
		if (!_valid) {
			logcritical("called ReadOutput after invalidation");
			return "";
		}
		std::string ret;
#if defined(unix) || defined(__unix__) || defined(__unix)
		SpinlockA guard(_availFlag);
		if (_avail == false)
			return "";
		struct pollfd fds;
		int32_t events = 0;
		fds.fd = red_output[0];  // stdin
		fds.events = POLLIN;
		events = poll(&fds, 1, 0);
		while ((fds.revents & POLLIN) == POLLIN) {  // _input to read available
			char buf[512];
			int32_t _read = read(red_output[0], buf, 511);
			if (_read <= 0) {
				if (_read == -1) {
					if (errno == EBADF)  // broken fd, count as pipe error
					{
						_pipeError = true;
						//InValidate();
					}
				}
				break;
			} else {
				// convert to string
				std::string str(buf, _read);
				ret += str;
			}
			fds.revents = 0;
			events = poll(&fds, 1, 0);
		}
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
		DWORD dwRead;
		CHAR chBuf[512];
		BOOL bSuccess = FALSE;
		for (;;) {
			bSuccess = ReadFile(red_output[0], chBuf, 512, &dwRead, NULL);
			if (!bSuccess) {
				if (auto err = GetLastError(); err == ERROR_BROKEN_PIPE || err == ERROR_BAD_PIPE || err == ERROR_PIPE_NOT_CONNECTED) {
					_pipeError = true;
					//InValidate();
					break;
				}
			}
			if (!bSuccess || dwRead == 0)
				break;
			else {
				// convert read to string
				std::string str(chBuf, dwRead);
				ret += str;
			}
		}
#endif
		PROF_3(TimeProfiling, "");
		return ret;
	}

	int64_t Execution::GetMemoryConsumption()
	{
		if (!_valid) {
			logcritical("called GetMemoryConsumption after invalidation");
			return -1;
		}
#if defined(unix) || defined(__unix__) || defined(__unix)
		return Processes::GetProcessMemory(processid);
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
		return Processes::GetProcessMemory(pi.hProcess);
#endif
	}

	bool Execution::KillProcess()
	{
		if (!_valid) {
			logcritical("called KillProcess after invalidation");
			return false;
		}
		loginfo("Execution {}: Killed Execution", _identifier);
		_endtime = std::chrono::steady_clock::now();
#if defined(unix) || defined(__unix__) || defined(__unix)
		return Processes::KillProcess(processid);
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
		return Processes::KillProcess(pi.hProcess);
#endif
	}

	bool Execution::WaitAndKillProcess()
	{
#if defined(unix) || defined(__unix__) || defined(__unix)
		if (processid) {
			if (!Processes::WaitProcess(processid)) {
				//logmessage("Killing process");
				Processes::KillProcess(processid);
				return false;
			}
			return true;
		} else
			return true;
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
		return true;
#endif
	}

	int32_t Execution::GetExitCode()
	{
		if (!_valid) {
			logcritical("called GetExitCode after invalidation");
			return -1;
		}
#if defined(unix) || defined(__unix__) || defined(__unix)
		return exitcode;
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
		return Processes::GetExitCode(pi.hProcess);
#endif
	}

	void Execution::InValidate()
	{
		_valid = false;
#if defined(unix) || defined(__unix__) || defined(__unix)
		processid = 0;
		close(red_input[0]);
		red_input[0] = -1;
		close(red_input[1]);
		red_input[1] = -1;
		close(red_output[0]);
		red_output[0] = -1;
		close(red_output[1]);
		red_output[1] = -1;
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
		if (red_input[0] != INVALID_HANDLE_VALUE)
			CloseHandle(red_input[0]);
		red_input[0] = INVALID_HANDLE_VALUE;
		if (red_input[1] != INVALID_HANDLE_VALUE)
			CloseHandle(red_input[1]);
		red_input[1] = INVALID_HANDLE_VALUE;
		if (red_output[0] != INVALID_HANDLE_VALUE)
			CloseHandle(red_output[0]);
		red_output[0] = INVALID_HANDLE_VALUE;
		if (red_output[1] != INVALID_HANDLE_VALUE)
			CloseHandle(red_output[1]);
		red_output[1] = INVALID_HANDLE_VALUE;
		if (pi.hProcess != INVALID_HANDLE_VALUE)
			CloseHandle(pi.hProcess);
		if (pi.hThread != INVALID_HANDLE_VALUE)
			CloseHandle(pi.hThread);
		pi.hProcess = INVALID_HANDLE_VALUE;
		pi.hThread = INVALID_HANDLE_VALUE;
		pi.dwProcessId = 0;
		pi.dwThreadId = 0;
		pi = {};
		si = {};
#endif
	}

	void Execution::InValidatePreExec()
	{
		_valid = false;
#if defined(unix) || defined(__unix__) || defined(__unix)
		processid = 0;
		close(red_input[0]);
		red_input[0] = -1;
		close(red_input[1]);
		red_input[1] = -1;
		close(red_output[0]);
		red_output[0] = -1;
		close(red_output[1]);
		red_output[1] = -1;
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
		CloseHandle(red_input[0]);
		red_input[0] = INVALID_HANDLE_VALUE;
		CloseHandle(red_input[1]);
		red_input[1] = INVALID_HANDLE_VALUE;
		CloseHandle(red_output[0]);
		red_output[0] = INVALID_HANDLE_VALUE;
		CloseHandle(red_output[1]);
		red_output[1] = INVALID_HANDLE_VALUE;
		pi.hProcess = INVALID_HANDLE_VALUE;
		pi.hThread = INVALID_HANDLE_VALUE;
		pi.dwProcessId = 0;
		pi.dwThreadId = 0;
		pi = {};
		si = {};
#endif
	}

	size_t Execution::GetDataSize()
	{
		static size_t size0x1 = ISerializable::GetDataSize() + 4  // version
		                        + 1                               // _running
		                        + 8                               // _starttime
		                        + 8                               // _endtime
		                        + 8                               // _lasttime
		                        + 1                               // storeoutput
		                        + 8                               // _identifier
		                        + 8                               // _exitreason
		                        + 1                               // valid
		                        + 1                               // has _callback
		                        + 1                               // _skipOracle
		                        + 8                               // formid input
			;


		auto size = size0x1;

		size += Buffer::ArrayBuffer::GetContainerSize(_reactiontime);
		size += Buffer::_sizeof(_cmdArgs);
		size += Buffer::_sizeof(_scriptArgs);
		if (_storeoutput) {
			size += Buffer::_sizeof(_output);
		} else {
			std::string str = std::string("");
			size += Buffer::_sizeof(str);
		}
		if (_callback)
			size += _callback->GetLength() + 8 /*extra for writing to buffer*/;

		return size;
	}

	bool Execution::WriteData(Buffer::ArrayBuffer& buffer)
	{
		SAVE_WRITE_EXPR(ISerializable::WriteData(buffer));

		SAVE_WRITE(buffer, uint32_t, version)
		SAVE_WRITE(buffer, bool, _running);
		SAVE_WRITE(buffer, std::chrono::steady_clock::time_point, _starttime);
		SAVE_WRITE(buffer, std::chrono::steady_clock::time_point, _endtime);
		SAVE_WRITE(buffer, std::chrono::steady_clock::time_point, _lasttime);

		SAVE_WRITE_LIST(buffer, uint64_t, _reactiontime);
		SAVE_WRITE(buffer, std::string, _cmdArgs);
		SAVE_WRITE(buffer, std::string, _scriptArgs);
		SAVE_WRITE(buffer, bool, _storeoutput);
		if (_storeoutput) {
			SAVE_WRITE(buffer, std::string, _output);
		} else {
			SAVE_WRITE(buffer, std::string, std::string(""));
		}

		SAVE_WRITE(buffer, uint64_t, _identifier);
		SAVE_WRITE(buffer, uint64_t, _exitreason);
		SAVE_WRITE(buffer, bool, _valid);
		SAVE_WRITE(buffer, bool, _callback.operator bool());
		if (_callback) {
			size_t sz = 0;
			auto buf = _callback->GetData(sz);
			SAVE_WRITE_BUFFER(buffer, buf, sz);
		}
		SAVE_WRITE(buffer, bool, _skipOracle);

		if (_input) {
			SAVE_WRITE(buffer, FormID, _input->GetFormID());
		} else {
			SAVE_WRITE(buffer, FormID, 0);
		}

		__hash = Hashing::hash(this);
		return true;
	}

	bool Execution::ReadData(Buffer::ArrayBuffer& buffer, LoadResolver* resolver)
	{
		ISerializable::ReadData(buffer, resolver);

		if (_loadData)
			delete _loadData;
		_loadData = new LoadData();

		int32_t vers = buffer.Read<uint32_t>();
		switch (vers) {
		case 0x1:
			{
				try {
					_running = buffer.Read<bool>();
					_starttime = buffer.Read<std::chrono::steady_clock::time_point>();
					_endtime = buffer.Read<std::chrono::steady_clock::time_point>();
					_lasttime = buffer.Read<std::chrono::steady_clock::time_point>();

					_reactiontime = buffer.ReadList<uint64_t>();
					_cmdArgs = buffer.Read<std::string>();
					_scriptArgs = buffer.Read<std::string>();
					_storeoutput = buffer.Read<bool>();
					_output = buffer.Read<std::string>();
					_identifier = buffer.Read<uint64_t>();
					_exitreason = buffer.Read<uint64_t>();
					_valid = buffer.Read<bool>();
					bool hascall = buffer.Read<bool>();
					if (hascall) {
						int64_t len = 0;
						unsigned char* buf = buffer.Read(&len);
						Buffer::ArrayBuffer abuf(buf, 0, len);
						_callback = Functions::BaseFunction::Create(abuf, resolver);
					}
					_skipOracle = buffer.Read<bool>();
					_cmdArgs = "";
				} catch (std::exception& e) {
					logcritical("Exception in read method: {}", e.what());
				}
				return true;
			}
		default:
			return false;
		}
	}

	void Execution::Delete()
	{
		Clear();
	}

	void Execution::Clear()
	{
		ISerializable::ClearForm();
		_running = false;
		//_executed = 0;
		//_lastwritten = "";
		_reactiontime.clear();
		_cmdArgs.clear();
		_cmdArgs.shrink_to_fit();
		_scriptArgs.clear();
		_scriptArgs.shrink_to_fit();
		_output.clear();
		_output.shrink_to_fit();
		_exitreason = ExitReason::Running;
		_skipOracle = false;
		//_skipExclusionCheck = false;
		_valid = true;
		_pipeinit = false;
		_input.reset();
		if (_valid)
			InValidate();
		_callback.reset();
	}

	void Execution::FreeMemory()
	{
		if (TryLock()) {
			if (HasFlag(ISerializable::SerializableFlags::DoNotFree) == false) {
				_output.clear();
				_output.shrink_to_fit();
				_scriptArgs.clear();
				_scriptArgs.shrink_to_fit();
				_cmdArgs.clear();
				_cmdArgs.shrink_to_fit();
			}
			Unlock();
		}
	}

	bool Execution::Freed()
	{
		if (_cmdArgs.empty() && _scriptArgs.empty() && _output.empty())
			return true;
		return false;
	}

	size_t Execution::MemorySize()
	{
		return sizeof(Execution) + _output.size() + _cmdArgs.size() + _scriptArgs.size() + _reactiontime.size() * sizeof(uint64_t) * 2;
	}

	void Execution::DeepCopy(Ref<Execution> other)
	{
		other->_valid = _valid;
		other->_exitreason = _exitreason;
		other->_identifier = _identifier;
		other->_storeoutput = _storeoutput;
		other->_output = _output;
		other->_cmdArgs = _cmdArgs;
		other->_reactiontime = _reactiontime;
		other->_endtime = _endtime;
		other->_starttime = _starttime;
		other->_running = _running;
	}

	bool Execution::HasChanged()
	{
		return ISerializable::HasChanged() || __hash != Hashing::hash(this);
	}

	void Execution::RegisterFactories()
	{
		if (!_registeredFactories) {
			_registeredFactories = !_registeredFactories;
		}
	}
}

namespace Hashing
{
	size_t hash(CommonLib::Execution* exec)
	{
		if (!exec)
			return 0;
		size_t hs = 0;

		hs = std::hash<bool>{}(exec->_running);
		hs = hs ^ (std::hash<uint64_t>{}(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::time_point_cast<std::chrono::nanoseconds>(exec->_starttime).time_since_epoch()).count()) << 1);
		hs = hs ^ (std::hash<uint64_t>{}(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::time_point_cast<std::chrono::nanoseconds>(exec->_endtime).time_since_epoch()).count()) << 1);
		if (auto ptr = exec->_input; ptr)
			hs = hs ^ (std::hash<CommonLib::FormID>{}(ptr->GetFormID()) << 1);
		else
			hs = hs ^ (std::hash<CommonLib::FormID>{}(0) << 1);

		hs = hs ^ (std::hash<uint64_t>{}(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::time_point_cast<std::chrono::nanoseconds>(exec->_lasttime).time_since_epoch()).count()) << 1);
		hs = hs ^ (Hashing::hash(exec->_reactiontime) << 1);
		hs = hs ^ (std::hash<std::string>{}(exec->_output) << 1);
		hs = hs ^ (std::hash<bool>{}(exec->_storeoutput) << 1);
		hs = hs ^ (std::hash<uint64_t>{}(exec->_identifier) << 1);
		hs = hs ^ (std::hash<uint64_t>{}(exec->_exitreason) << 1);
		hs = hs ^ (std::hash<bool>{}(exec->_skipOracle) << 1);
		hs = hs ^ (std::hash<bool>{}(exec->_valid) << 1);
		return hs;
	}
}
