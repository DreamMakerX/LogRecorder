#include "LogRecorder.h"
#include <io.h>
#include <sstream>
#include <algorithm>
#include <sys/timeb.h>

const size_t _logRecorderMaxBufferLength = 1024;

enum FileOperateErrCode
{
	FOEC_Success = 0,
	FOEC_ContentBadPointer = -10,
	FOEC_InvalidPath,
	FOEC_OpenFileFailed,
	FOEC_BufferOverflow,
	FOEC_IsnotUtf8Encoded,
};

enum LogRsp {
	LogRsp_AlreadyInitialized = -1000,
	LogRsp_NotInitialized,
	LogRsp_AlreadyStarted,
	LogRsp_NotStarted,
	LogRsp_InvalidPointer,
	LogRsp_LogDisable,
};

LogRecorder::LogRecorder(void)
{
	isInit_ = false;
	isKeepWork_ = false;
	logCheckThread_ = nullptr;
	MakeArray(&buffer_, _logRecorderMaxBufferLength);
}
LogRecorder::~LogRecorder()
{
	DeleteArray(&buffer_);
	LogRecorderClose();
}

long LogRecorder::DefaultStart()
{
	//以默认配置启动
	LogRecorderInit(logParam_);
	return LogRecorderStart();
}
long LogRecorder::LogRecorderInit(LogParam& logParam)
{
	if (isInit_) {
		return LogRsp_AlreadyInitialized;
	}
	isInit_ = true;

	logParam_ = logParam;
	MakeLogFileName();

	return 0;
}
long LogRecorder::LogRecorderStart()
{
	if (!isInit_) {
		return LogRsp_NotInitialized;
	}
	if (isKeepWork_) {
		return LogRsp_AlreadyStarted;
	}

	isKeepWork_ = true;
	LPDWORD threadID1 = 0;
	logCheckThread_ = (HANDLE)CreateThread(nullptr, 0, LogFileCheckingThread, (LPVOID)this, 0, threadID1);

	return 0;
}
long LogRecorder::LogRecorderStop()
{
	if (!isInit_)
		return LogRsp_NotInitialized;
	if (!isKeepWork_)
		return LogRsp_NotStarted;

	isKeepWork_ = false;
	CloseSingleThread(logCheckThread_);

	return 0;
}
long LogRecorder::LogRecorderClose()
{
	LogRecorderStop();

	isInit_ = false;
	isKeepWork_ = false;
	logCheckThread_ = nullptr;

	logParam_.InitLogParam();

	return 0;
}

long LogRecorder::PrintMessage(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	vsnprintf_s(buffer_, _logRecorderMaxBufferLength, _TRUNCATE, msg, args);
	va_end(args);

	long rsp = PrintMessageSingel(buffer_);

	return rsp;
}
long LogRecorder::PrintMessageSingel(const char* msg)
{
	if(!isKeepWork_)
	{
		return LogRsp_NotStarted;
	}
	DWORD dwBufLen = strlen(msg) + 50;

	LockGuard lock(mutex_);
	if (logFileName_.empty())
		return FOEC_InvalidPath;
	if (msg == nullptr)
		return FOEC_ContentBadPointer;
	if (dwBufLen > _logRecorderMaxBufferLength)
		return FOEC_BufferOverflow;

	FILE* fp = nullptr;
	fopen_s(&fp, logFileName_.c_str(), "ab+");
	if (fp == nullptr)
		return FOEC_OpenFileFailed;

	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	unsigned short nYear,nMonth,nDay,nHour,nMinute,nSecond,nMinSecond;
	nYear = sysTime.wYear;
	nMonth = sysTime.wMonth;
	nDay = sysTime.wDay;
	nHour = sysTime.wHour;
	nMinute = sysTime.wMinute;
	nSecond = sysTime.wSecond;
	nMinSecond = sysTime.wMilliseconds;

	std::string strMsg = msg;
	char* szDateTime = new char[dwBufLen];
	memset(szDateTime, 0, dwBufLen);
	
	sprintf_s(szDateTime, dwBufLen, "%04d-%02d-%02d %02d:%02d:%02d.%3d: %s\r\n",
		nYear, nMonth, nDay, nHour, nMinute, nSecond, nMinSecond, strMsg.c_str()); 
	fwrite(szDateTime, sizeof(char),  strlen(szDateTime), fp);
	fclose(fp);

	if (szDateTime) { delete[] szDateTime; szDateTime = nullptr; }

	return FOEC_Success;
}
bool LogRecorder::SetLogPath(const std::string& dir)
{
	LockGuard lock(mutex_);
	logParam_.dir_ = dir;

	if (MakeLogFileName() != 0)
		return false;

	return true;
}

long LogRecorder::MakeLogFileName()
{
	LockGuard lock(mutex_);
	std::vector<std::string> flienames;
	std::vector<__time64_t> cTimers;

	GetFiles(logParam_.dir_, logParam_.extension_, flienames, cTimers);
	DWORD vetsize = flienames.size();
	DWORD SecsfileNumber = 0;

	SYSTEMTIME systime;
	GetLocalTime(&systime);
	char SECSsystimechar[0x200] = { 0 };

	sprintf_s(SECSsystimechar, 0x200, "%s_%4d%02d%02d", logParam_.prefix_.c_str(), systime.wYear, systime.wMonth, systime.wDay);
	UINT preLen = strlen(SECSsystimechar);
	if (vetsize > 0)
	{
		for (DWORD s = 0; s < vetsize; s++)
		{
			if (DeleteOldLogFile(flienames[s], cTimers[s]))
				continue;

			size_t i1 = flienames[s].find(SECSsystimechar);
			size_t i3 = flienames[s].find("." + logParam_.extension_);
			if (i3 != std::string::npos && i1 == 0 && preLen < i3)
			{
				std::string sNum;
				sNum = flienames[s].substr(preLen, i3 - preLen);
				DWORD nNum = strtoul(sNum.c_str(), nullptr, 10);
				if (SecsfileNumber < nNum)
					SecsfileNumber = nNum;
			}
		}
	}

	char logName[0x400] = { 0 };
	sprintf_s(logName, 0x400, "%s\\%s%02u.%s", logParam_.dir_.c_str(), SECSsystimechar, SecsfileNumber + 1, logParam_.extension_.c_str());
	logFileName_ = logName;
	CreateDirectoryMultiple(logParam_.dir_);
	return 0;
}
bool LogRecorder::DeleteOldLogFile(std::string fileName, time_t timer)
{
	time_t timep;
	time(&timep);
	double timeDistance = difftime(timep, timer);
	if (timeDistance > (logParam_.keepDays_ * 24 * 60 * 60) && logParam_.keepDays_ != 0) {
		char SECSsystimechar[0x200] = { 0 };
		sprintf_s(SECSsystimechar, 0x200, "%s\\%s", logParam_.dir_.c_str(), fileName.c_str());
		int i = remove(SECSsystimechar);
		return true;
	}

	return false;
}

void LogRecorder::LogFileChecking()
{
	DWORD msgid = 0;
	unsigned long long ullTimer = 0;
	unsigned long long divider = 0;

	if (logParam_.hour_)
		divider = 60 * 60 * 1000;
	else
		divider = 24 * 60 * 60 * 1000;

	ullTimer = SetLogFileTime(divider);
	while (isKeepWork_) {
		Sleep(10);
		if (ullTimer < GetTimeStamp1970()) {				//check for create new log file
			MakeLogFileName();
			ullTimer += divider;
		}
	}
}
ULONGLONG LogRecorder::SetLogFileTime(const ULONGLONG& multiple)
{
	//获取下一次创建文件的时间
	ULONGLONG now = GetTimeStamp1970();

	return now + multiple - (now % multiple);
}
DWORD WINAPI LogRecorder::LogFileCheckingThread(LPVOID lpVoid)
{
	LogRecorder* pLogger = (LogRecorder*)lpVoid;
	pLogger->LogFileChecking();

	return 0;
}

bool LogRecorder::IsDigits(const std::string& s) {
	std::istringstream iss(s);
	double value;

	// 尝试从字符串中提取浮点数
	if (iss >> value) {
		// 检查提取后是否到达字符串末尾
		char c;
		if (iss >> c) {
			// 如果提取后仍有字符，则说明不是有效的浮点数格式
			return false;
		}
		else {
			// 提取成功且到达字符串末尾，说明是有效的浮点数格式
			return true;
		}
	}
	else {
		// 提取失败，说明不是有效的浮点数格式
		return false;
	}
}
void LogRecorder::CloseSingleThread(HANDLE& handle, const DWORD& time)
{
	if (handle != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(handle, time);
		CloseHandle(handle);

		handle = INVALID_HANDLE_VALUE;
	}
}
void LogRecorder::GetFiles(std::string path, std::string extension, std::vector<std::string>& files, std::vector<__time64_t>& cTimers)
{
	//读取文件夹中所有文件名
	//文件句柄
	intptr_t   hFile = 0;

	//文件信息
#ifdef _WIN64
	struct _finddatai64_t fileinfo;
	std::string p;
	if ((hFile = _findfirsti64(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			//如果是目录,迭代之  
			//如果不是,加入列表  
			if ((fileinfo.attrib & _A_SUBDIR))
			{
				;
			}
			else
			{
				std::string szbuf = fileinfo.name;
				size_t isd = szbuf.find("." + extension);
				if (isd > 0) {
					files.push_back(fileinfo.name);
					cTimers.push_back(fileinfo.time_write);
				}
			}
		} while (_findnexti64(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
#else
	struct _finddata_t fileinfo;
	std::string p;
	if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			//如果是目录,迭代之  
			//如果不是,加入列表  
			if ((fileinfo.attrib & _A_SUBDIR))
			{
				;
			}
			else
			{
				std::string szbuf = fileinfo.name;
				size_t isd = szbuf.find("." + extension);
				if (isd > 0) {
					files.push_back(fileinfo.name);
					cTimers.push_back(fileinfo.time_write);
				}
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
#endif
}
bool LogRecorder::CreateDirectoryMultiple(const std::string& folderPath)
{
	//creat dir,multi-level
	std::string strDir;
	int nPos = folderPath.find_first_of("/\\");
	while (nPos != -1)
	{
		strDir = folderPath.substr(0, nPos);
		if (!CreateDirectorySingle(strDir))
			return false;
		nPos = folderPath.find_first_of("/\\", nPos + 1);
	}
	CreateDirectorySingle(folderPath);

	return true;
}
bool LogRecorder::CreateDirectorySingle(const std::string& folderPath)
{
	//creat dir
	BOOL flag = FALSE;
	wchar_t wc_path[0x400] = { 0 };

	DWORD bRes = GetFileAttributesA(folderPath.c_str());
	//若文件夹不存在，创建文件夹
	if (bRes == 0xFFFFFFFF)
		CreateDirectoryA(folderPath.c_str(), nullptr);
	else if (bRes & FILE_ATTRIBUTE_DIRECTORY)	//文件夹存在
		flag = CreateDirectoryA(folderPath.c_str(), nullptr); // flag 为 true 说明创建成功
	else
		return false;

	return true;
}

ULONGLONG LogRecorder::GetTimeStamp1900(bool bLocal)
{
	//获取1900年1月1日至今的秒数
	FILETIME ft;
	SYSTEMTIME st;
	GetSystemTime(&st); // gets current time
	SystemTimeToFileTime(&st, &ft); // converts to file time format
	ULONGLONG tickCount = ((((ULONGLONG)ft.dwHighDateTime) << 32) + ft.dwLowDateTime) / 10000;

	//获取时区
	TIME_ZONE_INFORMATION timeZoneInfo;
	GetTimeZoneInformation(&timeZoneInfo);
	long timeZone = timeZoneInfo.Bias / (-60);

	//返回当地时间或系统时间
	if (bLocal)
		return tickCount + timeZone * 60 * 60;
	else
		return tickCount;
}
ULONGLONG LogRecorder::GetTimeStamp1970(bool bLocal)
{
	//获取1970年1月1日至今的秒数
	struct timeb tb;
	ftime(&tb);

	//获取时区
	TIME_ZONE_INFORMATION timeZoneInfo;
	GetTimeZoneInformation(&timeZoneInfo);
	long timeZone = timeZoneInfo.Bias / (-60);

	//返回当地时间或系统时间
	if (bLocal)
		return tb.time + timeZone * 60 * 60;
	else
		return tb.time;//若需要毫秒数 ==> tb.time*1000 + tb.millitm
}

std::string LogRecorder::GetLowercase(const std::string& s)
{
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}
std::string LogRecorder::GetUppercase(const std::string& s)
{
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);
	return result;
}