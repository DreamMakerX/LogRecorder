#pragma once
#include <afx.h>
#include <vector>
#include <string>

class RecursiveMutex {
public:
	RecursiveMutex() {
		InitializeCriticalSection(&section_);
	}
	~RecursiveMutex() {
		DeleteCriticalSection(&section_);
	}
	PCRITICAL_SECTION cs() {
		return &section_;
	}
private:
	CRITICAL_SECTION section_;
};

class LockGuard {
public:
	LockGuard(RecursiveMutex& mutex) : mutex_(mutex) {
		EnterCriticalSection(mutex_.cs());
	}
	~LockGuard() {
		LeaveCriticalSection(mutex_.cs());
	}
private:
	RecursiveMutex& mutex_;
};

typedef struct LogParam {
	LogParam(void) { InitLogParam(); }
	void InitLogParam() {
		hour_ = false;
		prefix_ = "";
		keepDays_ = 7;
		extension_ = "log";
		dir_ = "LogRecorder";
	}
	bool			hour_;				//true ==> 每小时创建日志文件;false ==> 每天创建日志文件
	std::string		dir_;				//日志路径
	std::string		prefix_;			//日志名称前缀
	DWORD			keepDays_;			//日志保存时间（天）
	std::string		extension_;			//日志名称后缀
}LogParam;

class LogRecorder
{
public:
	LogRecorder(void);
	~LogRecorder();

private:
	bool					isInit_;				//判定初始化的标识符
	bool					isKeepWork_;			//判定校验启动的标识符

	RecursiveMutex			mutex_;					//日志文件互斥量
	char*					buffer_;				//日志缓冲区
	LogParam				logParam_;				//日志参数
	std::string				logFileName_;			//日志名称（完整路径）
	HANDLE					logCheckThread_;		//日志校验线程句柄

public:	
	long DefaultStart();
	long LogRecorderInit(LogParam& logParam);
	long LogRecorderStart();
	long LogRecorderStop();
	long LogRecorderClose();

	long PrintMessage(const char* msg, ...);
	bool SetLogPath(const std::string& dir);

private:
	long MakeLogFileName();
	bool DeleteOldLogFile(std::string fileName, time_t timer);
	long PrintMessageSingel(const char* msg);

	void LogFileChecking();
	ULONGLONG SetLogFileTime(const ULONGLONG& multiple);
	static DWORD WINAPI LogFileCheckingThread(LPVOID lpVoid);

	bool IsDigits(const std::string& s);
	void CloseSingleThread(HANDLE& handle, const DWORD& time = 3000);
	void GetFiles(std::string path, std::string extension, std::vector<std::string>& files, std::vector<__time64_t>& cTimers);
	bool CreateDirectoryMultiple(const std::string& folderPath);
	bool CreateDirectorySingle(const std::string& folderPath);

	ULONGLONG GetTimeStamp1900(bool bLocal = true);
	ULONGLONG GetTimeStamp1970(bool bLocal = true);

	std::string GetLowercase(const std::string& s);
	std::string GetUppercase(const std::string& s);

	template<class T>
	T DataTypeConvert(const std::string& value)
	{
		if (IsDigits(value))
		{
			return static_cast<T>(std::stod(value));
		}
		else
		{
			if (!GetLowercase(value).compare("true"))
			{
				return static_cast<T>(std::stod("1"));
			}
			else
			{
				return static_cast<T>(std::stod("0"));
			}
		}
	}
	template<class T>
	T DataTypeConvert(const char* value)
	{
		return DataTypeConvert<T>((std::string)value);
	}
	template<class T>
	std::string DataTypeConvert(const T& value)
	{
		std::stringstream ss;
		ss << value;
		return ss.str();
	}

	template<class T>
	void MakeArray(T** ptr, const size_t& count)
	{
		if (count == 0)
		{
			*ptr = nullptr;
		}
		else
		{
			*ptr = new T[count];
		}
	}

	template<class T>
	void DeleteArray(T** ptr)
	{
		if (*ptr != nullptr)
		{
			delete[] * ptr;
			*ptr = nullptr;
		}
	}
};

