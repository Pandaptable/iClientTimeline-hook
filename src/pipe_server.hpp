#pragma once

#include <Windows.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <mutex>
#include <print>
#include <string>
#include <thread>

using json = nlohmann::json;

#ifndef NDEBUG
#define LOG(...) std::println(__VA_ARGS__)
#else
#define LOG(...) ((void)0)
#endif

class PipeServer
{
	static constexpr DWORD kBufSize = 65536;

public:
	explicit PipeServer(std::string name = "\\\\.\\pipe\\SteamTimeline")
		: m_name(std::move(name)), m_stopEvent(CreateEventA(nullptr, TRUE, FALSE, nullptr)), m_brokenEvent(CreateEventA(nullptr, TRUE, FALSE, nullptr))
	{
	}

	~PipeServer()
	{
		Stop();
		CloseHandle(m_stopEvent);
		CloseHandle(m_brokenEvent);
	}

	PipeServer(const PipeServer &) = delete;
	PipeServer &operator=(const PipeServer &) = delete;

	void Start()
	{
		m_thread = std::thread(&PipeServer::Worker, this);
	}

	void Stop()
	{
		SetEvent(m_stopEvent);
		if (m_thread.joinable())
			m_thread.join();
		ResetEvent(m_stopEvent);
	}

	void Write(const std::string &type, json data = json::object())
	{
		std::string line;
		try {
			line = json{{"type", type}, {"data", data}}.dump(-1, ' ', false, json::error_handler_t::replace) + "\n";
		} catch (...) {
			return;
		}

		std::lock_guard lock(m_mutex);
		if (m_pipe == INVALID_HANDLE_VALUE)
			return;

		OVERLAPPED ov{};
		ov.hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
		
		DWORD written = 0;
		if (!WriteFile(m_pipe, line.data(), static_cast<DWORD>(line.size()), &written, &ov))
		{
			if (GetLastError() == ERROR_IO_PENDING)
			{
				if (!GetOverlappedResult(m_pipe, &ov, &written, TRUE))
				{
					LOG("[Pipe] WriteFile async failed ({}), client disconnected\n", GetLastError());
					SetEvent(m_brokenEvent);
				}
			}
			else
			{
				LOG("[Pipe] WriteFile failed ({}), client disconnected\n", GetLastError());
				SetEvent(m_brokenEvent);
			}
		}
		CloseHandle(ov.hEvent);
	}

private:
	void Worker()
	{
		while (true)
		{
			HANDLE h = CreateNamedPipeA(
				m_name.c_str(),
				PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
				PIPE_TYPE_BYTE | PIPE_WAIT,
				PIPE_UNLIMITED_INSTANCES,
				kBufSize, kBufSize,
				0, nullptr);

			if (h == INVALID_HANDLE_VALUE)
			{
				LOG("[Pipe] CreateNamedPipe failed ({})\n", GetLastError());
				if (WaitForSingleObject(m_stopEvent, 1000) == WAIT_OBJECT_0)
					break;
				continue;
			}

			LOG("[Pipe] Waiting for client on {}\n", m_name);

			OVERLAPPED ov{};
			ov.hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);

			BOOL res = ConnectNamedPipe(h, &ov);
			bool connected = false;
			DWORD err = GetLastError();

			if (res)
			{
				connected = true;
			}
			else if (err == ERROR_PIPE_CONNECTED)
			{
				connected = true;
			}
			else if (err == ERROR_IO_PENDING)
			{
				HANDLE waits[2] = {ov.hEvent, m_stopEvent};
				connected = (WaitForMultipleObjects(2, waits, FALSE, INFINITE) == WAIT_OBJECT_0);
			}

			CloseHandle(ov.hEvent);

			if (!connected)
			{
				CancelIo(h);
				DisconnectNamedPipe(h);
				CloseHandle(h);
				break;
			}

			LOG("[Pipe] Client connected\n");
			{
				std::lock_guard lock(m_mutex);
				m_pipe = h;
			}
			ResetEvent(m_brokenEvent);

			HANDLE waits[2] = {m_stopEvent, m_brokenEvent};
			bool stopping = (WaitForMultipleObjects(2, waits, FALSE, INFINITE) == WAIT_OBJECT_0);

			{
				std::lock_guard lock(m_mutex);
				m_pipe = INVALID_HANDLE_VALUE;
			}
			DisconnectNamedPipe(h);
			CloseHandle(h);
			LOG("[Pipe] Client disconnected\n");

			if (stopping)
				break;
		}
	}

	std::string m_name;
	HANDLE m_pipe{INVALID_HANDLE_VALUE};
	HANDLE m_stopEvent;
	HANDLE m_brokenEvent;
	std::mutex m_mutex;
	std::thread m_thread;
};
