#pragma once

#include <Phobos.h>

#include <Dbghelp.h>
#include <Unsorted.h>
#include <MessageListClass.h>
#include <WWMouseClass.h>

#include <chrono>

class Console
{
public:

	enum class ConsoleColor
	{
		Black = 0,
		DarkBlue = 1,
		DarkGreen = 2,
		DarkRed = 4,
		Intensity = 8,

		DarkCyan = DarkBlue | DarkGreen,
		DarkMagenta = DarkBlue | DarkRed,
		DarkYellow = DarkGreen | DarkRed,
		Gray = DarkBlue | DarkGreen | DarkRed,
		DarkGray = Black | Intensity,

		Blue = DarkBlue | Intensity,
		Green = DarkGreen | Intensity,
		Red = DarkRed | Intensity,
		Cyan = Blue | Green,
		Magenta = Blue | Red,
		Yellow = Green | Red,
		White = Red | Green | Blue,
	};

	union ConsoleTextAttribute
	{
		WORD AsWord;
		struct
		{
			ConsoleColor Foreground : 4;
			ConsoleColor Background : 4;
			bool LeadingByte : 1;
			bool TrailingByte : 1;
			bool GridTopHorizontal : 1;
			bool GridLeftVertical : 1;
			bool GridRightVerticle : 1;
			bool ReverseVideo : 1; // Reverse fore/back ground attribute
			bool Underscore : 1;
			bool Unused : 1;
		};
	};
	static ConsoleTextAttribute TextAttribute;
	static HANDLE ConsoleHandle;

	static bool Create();
	static void Release();

	template<size_t Length>
	constexpr static void Write(const char(&str)[Length])
	{
		Write(str, Length - 1); // -1 because there is a '\0' here
	}

	static void SetForeColor(ConsoleColor color);
	static void SetBackColor(ConsoleColor color);
	static void EnableUnderscore(bool enable);
	static void Write(const char* str, int len);
	static void WriteLine(const char* str, int len);
	static void WriteWithVArgs(const char* pFormat, va_list args);
	static void WriteFormat(const char* pFormat, ...);

private:
	static void PatchLog(DWORD dwAddr, void* realFunc, DWORD* pdwRealFunc);
};

class AbstractClass;
class REGISTERS;
class Debug final
{
public:
	enum class Severity : int
	{
		None = 0,
		Verbose = 1,
		Notice = 2,
		Warning = 3,
		Error = 4,
		Fatal = 5
	};

	static FILE* LogFile;
	static bool LogEnabled;

	static std::wstring ApplicationFilePath;
	static std::wstring LogFilePathName;
	static std::wstring LogFileMainName;
	static std::wstring LogFileTempName;
	static std::wstring LogFileMainFormattedName;
	static std::wstring LogFileExt;

	static char DeferredStringBuffer[0x1000];
	static char LogMessageBuffer[0x1000];
	static std::vector<std::string> DeferredLogData;

	enum class ExitCode : int
	{
		Undefined = -1,
		SLFail = 114514
	};

	static FORCEINLINE void TakeMouse()
	{
		WWMouseClass::Instance->ReleaseMouse();
		Imports::ShowCursor.get()(1);
	}

	static FORCEINLINE void ReturnMouse()
	{
		Imports::ShowCursor.get()(0);
		WWMouseClass::Instance->CaptureMouse();
	}

	static NOINLINE void DumpStack(REGISTERS* R, size_t len, int startAt = 0)
	{
		if (!Debug::LogFileActive())
		{
			return;
		}

		Debug::LogUnflushed("Dumping %X bytes of stack\n", len);
		auto const end = len / 4;
		auto const* const mem = R->lea_Stack<DWORD*>(startAt);
		for (auto i = 0u; i < end; ++i)
		{
			const char* suffix = "";
			const uintptr_t ptr = mem[i];
			if (ptr >= 0x401000 && ptr <= 0xB79BE4)
				suffix = "GameMemory!";

			Debug::LogUnflushed("esp+%04X = %08X %s\n", i * 4, mem[i], suffix);
		}

		Debug::Log("====================Done.\n"); // flushes
	}

	template <typename... TArgs>
	static FORCEINLINE void Log(bool enabled, Debug::Severity severity, const char* const pFormat, TArgs&&... args)
	{
		if (enabled)
		{
			Debug::Log(severity, pFormat, std::forward<TArgs>(args)...);
		}
	}

	template <typename... TArgs>
	static FORCEINLINE void Log(bool enabled, const char* const pFormat, TArgs&&... args)
	{
		if (enabled)
		{
			Debug::Log(pFormat, std::forward<TArgs>(args)...);
		}
	}

	template <typename... TArgs>
	static FORCEINLINE void Log(Debug::Severity severity, const char* const pFormat, TArgs&&... args)
	{
		Debug::LogFlushed(severity, pFormat, std::forward<TArgs>(args)...);
	}

	template <typename... TArgs>
	static FORCEINLINE void Log(const char* const pFormat, TArgs&&... args)
	{
		Debug::LogFlushed(pFormat, std::forward<TArgs>(args)...);
	}

	static FORCEINLINE void LogWithVArgs(const char* const pFormat, va_list args)
	{
		if (Debug::LogFileActive())
		{
			Debug::LogWithVArgsUnflushed(pFormat, args);
			Debug::Flush();
		}
	}

	static FORCEINLINE bool LogFileActive()
	{
		return Debug::LogEnabled && Debug::LogFile;
	}

	//This log is not immedietely printed , but buffered until time it need to be finalize(printed)
	static NOINLINE void LogDeferred(const char* pFormat, ...)
	{
		va_list args;
		va_start(args, pFormat);
		vsprintf_s(DeferredStringBuffer, pFormat, args);
		DeferredLogData.emplace_back(DeferredStringBuffer);
		va_end(args);
	}

	static NOINLINE void LogDeferredFinalize()
	{
		for (auto const& Logs : DeferredLogData)
		{
			if (!Logs.empty())
				GameDebugLog::Log("%s", Logs);
		}

		DeferredLogData.clear();
	}

	static NOINLINE void LogAndMessage(const char* pFormat, ...)
	{
		va_list args;
		va_start(args, pFormat);
		vsprintf_s(LogMessageBuffer, pFormat, args);
		Debug::Log("%s", LogMessageBuffer);
		va_end(args);
		wchar_t buffer[0x1000];
		mbstowcs(buffer, LogMessageBuffer, 0x1000);
		MessageListClass::Instance->PrintMessage(buffer);
	}

	static NOINLINE void InitLogFile()
	{
		wchar_t path[MAX_PATH];
		GetCurrentDirectoryW(MAX_PATH, path);
		Debug::ApplicationFilePath = path;
		Debug::LogFilePathName = path;
		Debug::LogFilePathName += L"\\debug";
		CreateDirectoryW(Debug::LogFilePathName.c_str(), nullptr);
	}

	static NOINLINE std::wstring GetCurTime()
	{
		const auto now = std::chrono::system_clock::now();
		const std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
		const std::tm* localTime = std::localtime(&currentTime);

		return std::format(L"{:04}{:02}{:02}-{:02}{:02}{:02}",
			localTime->tm_year + 1900,
			localTime->tm_mon + 1,
			localTime->tm_mday,
			localTime->tm_hour,
			localTime->tm_min,
			localTime->tm_sec);
	}

	static bool made;

	static NOINLINE void PrepareLogFile()
	{
		if (!made)
		{
			Debug::LogFileTempName = Debug::LogFilePathName + Debug::LogFileMainName + Debug::LogFileExt;
			Debug::LogFileMainFormattedName = Debug::LogFilePathName + std::format(L"{}.{}",
			Debug::LogFileMainName, GetCurTime()) + Debug::LogFileExt;

			made = 1;
		}
	}

	static NOINLINE std::wstring PrepareSnapshotDirectory()
	{
		std::wstring buffer = Debug::LogFilePathName + std::format(L"\\snapshot-{}",
				GetCurTime());

		CreateDirectoryW(buffer.c_str(), nullptr);

		return buffer;
	}

	static FORCEINLINE std::wstring FullDump(PMINIDUMP_EXCEPTION_INFORMATION const pException = nullptr)
	{
		return FullDump(Debug::PrepareSnapshotDirectory(), pException);
	}

	static NOINLINE std::wstring FullDump(
		std::wstring destinationFolder,
		PMINIDUMP_EXCEPTION_INFORMATION const pException = nullptr)
	{
		std::wstring filename = std::move(destinationFolder);
		filename += L"\\extcrashdump.dmp";

		HANDLE dumpFile = CreateFileW(filename.c_str(), GENERIC_WRITE,
			0, nullptr, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, nullptr);

		MINIDUMP_TYPE type = static_cast<MINIDUMP_TYPE>(MiniDumpNormal
									   | MiniDumpWithDataSegs
									   | MiniDumpWithIndirectlyReferencedMemory
									   | MiniDumpWithFullMemory
		);

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, type, pException, nullptr, nullptr);
		CloseHandle(dumpFile);

		return filename;
	}

	static NOINLINE void LogFileOpen()
	{
		Debug::PrepareLogFile();
		Debug::LogFileClose(999);

		LogFile = _wfsopen(Debug::LogFileTempName.c_str(), L"w", SH_DENYNO);

		if (!LogFile)
		{
			std::wstring msg = std::format(L"Log file failed to open. Error code = {}", errno);
			MessageBoxW(Game::hWnd.get(), Debug::LogFileTempName.c_str(), msg.c_str(), MB_OK | MB_ICONEXCLAMATION);
			Phobos::Otamaa::ExeTerminated = true;
			ExitProcess(1);
		}
	}

	static NOINLINE void LogFileClose(int tag)
	{
		if (Debug::LogFile)
		{
			fprintf(Debug::LogFile, "Closing log file on request %d", tag);
			fclose(Debug::LogFile);
			CopyFileW(Debug::LogFileTempName.c_str(), Debug::LogFileMainFormattedName.c_str(), FALSE);
			Debug::LogFile = nullptr;
		}
	}

	static NOINLINE void LogFileRemove()
	{
		Debug::LogFileClose(555);
		DeleteFileW(Debug::LogFileTempName.c_str());
	}

	static NOINLINE void FreeMouse()
	{
		Game::StreamerThreadFlush();
		const auto pMouse = MouseClass::Instance();

		if (pMouse)
		{
			const auto pMouseVtable = VTable::Get(pMouse);

			if (pMouseVtable == 0x7E1964)
			{
				pMouse->UpdateCursor(MouseCursorType::Default, false);
			}
		}

		const auto pWWMouse = WWMouseClass::Instance();

		if (pWWMouse)
		{
			const auto pWWMouseVtable = VTable::Get(pWWMouse);

			if (pWWMouseVtable == 0x7F7B2C)
			{
				pWWMouse->ReleaseMouse();
			}
		}

		ShowCursor(TRUE);

		auto const BlackSurface = [](DSurface* pSurface)
			{
				if (pSurface && VTable::Get(pSurface) == DSurface::vtable && pSurface->BufferPtr)
				{
					pSurface->Fill(0);
				}
			};

		BlackSurface(DSurface::Alternate);
		BlackSurface(DSurface::Composite);
		BlackSurface(DSurface::Hidden);
		BlackSurface(DSurface::Temp);
		BlackSurface(DSurface::Primary);
		BlackSurface(DSurface::Sidebar);
		BlackSurface(DSurface::Tile);

		ShowCursor(TRUE);
	}

	static NOINLINE void WriteTimestamp()
	{
		if (LogFile)
		{
			time_t raw;
			time(&raw);

			tm t;
			localtime_s(&t, &raw);

			fprintf(LogFile, "[%02d:%02d:%02d] ", t.tm_hour, t.tm_min, t.tm_sec);
		}
	}

	[[noreturn]] static NOINLINE void ExitGame(unsigned int code = 1u)
	{
		Phobos::ExeTerminate();
		ExitProcess(code);
	}

	static NOINLINE void FatalError(bool Dump = false) /* takes formatted message from Ares::readBuffer */
	{
		static wchar_t Message[0x400];
		wsprintfW(Message,
			L"An internal error has been encountered and the game is unable to continue normally. "
			L"Please notify the mod's creators about this issue, or Contact Otamaa at "
			L"Discord for updates and support.\n"
		);

		Debug::Log("\nFatal Error: \n%s\n", Message);
		Debug::FreeMouse();
		MessageBoxW(Game::hWnd, Message, L"Fatal Error - Yuri's Revenge", MB_OK | MB_ICONERROR);

		if (Dump)
		{
			Debug::FullDump();
		}

		Debug::ExitGame();
	}

	static NOINLINE void FatalError(const char* Message, ...)
	{
		Debug::FreeMouse();

		va_list args;
		va_start(args, Message);
		vsnprintf_s(Phobos::readBuffer, Phobos::readLength - 1, Message, args); /* note that the message will be truncated somewhere after 0x300 chars... */
		va_end(args);

		Debug::FatalError(false);
	}

	[[noreturn]] static NOINLINE void FatalErrorAndExit(ExitCode nExitCode, const char* pFormat, ...)
	{
		va_list args;
		va_start(args, pFormat);
		vsprintf_s(Phobos::readBuffer, pFormat, args);
		va_end(args);
		Debug::FatalError(Phobos::Config::DebugFatalerrorGenerateDump);
		Debug::ExitGame();
	}

	[[noreturn]] static NOINLINE void FatalErrorAndExit(const char* pFormat, ...)
	{
		va_list args;
		va_start(args, pFormat);
		vsprintf_s(Phobos::readBuffer, pFormat, args);
		va_end(args);
		Debug::FatalError(Phobos::Config::DebugFatalerrorGenerateDump);
		Debug::ExitGame();
	}

	static void RegisterParserError()
	{
		if (Phobos::Otamaa::TrackParserErrors)
		{
			Phobos::Otamaa::ParserErrorDetected = true;
		}
	}

	static NOINLINE void DumpObj(void const* data, size_t len)
	{
		if (!Debug::LogFileActive())
		{
			return;
		}

		Debug::LogUnflushed<false>("Dumping %u bytes of object at %p\n", len, data);
		auto const bytes = static_cast<byte const*>(data);

		Debug::LogUnflushed<false>("       |");
		for (auto i = 0u; i < 0x10u; ++i)
		{
			Debug::LogUnflushed<false>(" %02X |", i);
		}
		Debug::LogUnflushed<false>("\n");
		Debug::LogUnflushed<false>("-------|");
		for (auto i = 0u; i < 0x10u; ++i)
		{
			Debug::LogUnflushed<false>("----|", i);
		}
		auto const bytesToPrint = (len + 0x10 - 1) / 0x10 * 0x10;
		for (auto startRow = 0u; startRow < bytesToPrint; startRow += 0x10)
		{
			Debug::LogUnflushed<false>("\n");
			Debug::LogUnflushed<false>(" %05X |", startRow);
			auto const bytesInRow = std::min(len - startRow, 0x10u);
			for (auto i = 0u; i < bytesInRow; ++i)
			{
				Debug::LogUnflushed<false>(" %02X |", bytes[startRow + i]);
			}
			for (auto i = bytesInRow; i < 0x10u; ++i)
			{
				Debug::LogUnflushed<false>(" -- |");
			}
			for (auto i = 0u; i < bytesInRow; ++i)
			{
				auto const& sym = bytes[startRow + i];
				Debug::LogUnflushed<false>("%c", isprint(sym) ? sym : '?');
			}
		}
		Debug::Log("\nEnd of dump.\n"); // flushes
	}

	template <typename T>
	static FORCEINLINE void DumpObj(const T& object)
	{
		DumpObj(&object, sizeof(object));
	}

	static NOINLINE void INIParseFailed(const char* section, const char* flag, const char* value, const char* Message = nullptr)
	{
		if (Phobos::Otamaa::TrackParserErrors)
		{
			const char* LogMessage = (Message == nullptr)
				? "[Phobos] Failed to parse INI file content: [%s]%s=%s\n"
				: "[Phobos] Failed to parse INI file content: [%s]%s=%s (%s)\n"
				;

			Debug::Log(LogMessage, section, flag, value, Message);
			Debug::RegisterParserError();
		}
	}

	static constexpr const char* SeverityString(Debug::Severity const severity)
	{
		switch (severity)
		{
		case Severity::Verbose:
			return "verbose";
		case Severity::Notice:
			return "notice";
		case Severity::Warning:
			return "warning";
		case Severity::Error:
			return "error";
		case Severity::Fatal:
			return "fatal";
		default:
			return "wtf";
		}
	}

	static NOINLINE void LogFlushed(const char* pFormat, ...)
	{
		if (Debug::LogFileActive())
		{
			va_list args;
			va_start(args, pFormat);
			Debug::LogWithVArgsUnflushed(pFormat, args);
			Debug::Flush();
			va_end(args);
		}
	}

	static NOINLINE void LogFlushed(Debug::Severity severity, const char* pFormat, ...)
	{
		if (Debug::LogFileActive())
		{
			if (severity != Severity::None)
			{
				Debug::LogUnflushed<false>(
					"[Developer %s]", SeverityString(severity));
			}

			va_list args;
			va_start(args, pFormat);
			Debug::LogWithVArgsUnflushed(pFormat, args);
			Debug::Flush();
			va_end(args);
		}
	}

	// no flushing, and unchecked
	template<bool check = true>
	static NOINLINE void LogUnflushed(const char* pFormat, ...)
	{
		if constexpr (check)
		{
			if (Debug::LogFileActive())
			{
				va_list args;
				va_start(args, pFormat);
				Debug::LogWithVArgsUnflushed(pFormat, args);
				va_end(args);
			}
		}
		else
		{
			va_list args;
			va_start(args, pFormat);
			Debug::LogWithVArgsUnflushed(pFormat, args);
			va_end(args);
		}
	}

	static NOINLINE void LogWithVArgsUnflushed(const char* pFormat, va_list args)
	{
		Console::WriteWithVArgs(pFormat, args);
		vfprintf(Debug::LogFile, pFormat, args);
	}

	// flush unchecked
	static FORCEINLINE void Flush()
	{
		fflush(Debug::LogFile);
	}
};
