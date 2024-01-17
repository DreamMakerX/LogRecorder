#include "LogRecorder.h"
#include <iostream>

void PrintMessageDefault()
{
	//以默认配置启动日志
	LogRecorder logRecorder;
	logRecorder.DefaultStart();

	//打印日志
	logRecorder.PrintMessage("%d relevant personnel attended the meeting today", 54);

	//暂停日志输出
	logRecorder.LogRecorderStop();

	//关闭日志输出
	logRecorder.LogRecorderClose();
}
void PrintMessageSetup()
{
	//自定义配置
	LogParam params;
	params.dir_ = "Meeting";	//日志路径
	params.hour_ = true;		//每小时创建一个日志文件
	params.keepDays_ = 90;		//日志保存30天
	params.prefix_ = "Global";	//日志文件前缀
	params.extension_ = "daily";//日志文件后缀


	//启动日志输出
	LogRecorder logRecorder;
	logRecorder.LogRecorderInit(params);
	logRecorder.LogRecorderStart();

	//打印日志
	logRecorder.PrintMessage("%d relevant personnel attended the meeting today", 96);

	//暂停日志输出
	logRecorder.LogRecorderStop();

	//关闭日志输出
	logRecorder.LogRecorderClose();
}
void main()
{
	PrintMessageDefault();

	PrintMessageSetup();

	std::cin.get();
}