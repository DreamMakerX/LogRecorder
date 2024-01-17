#include "LogRecorder.h"
#include <iostream>

void PrintMessageDefault()
{
	//��Ĭ������������־
	LogRecorder logRecorder;
	logRecorder.DefaultStart();

	//��ӡ��־
	logRecorder.PrintMessage("%d relevant personnel attended the meeting today", 54);

	//��ͣ��־���
	logRecorder.LogRecorderStop();

	//�ر���־���
	logRecorder.LogRecorderClose();
}
void PrintMessageSetup()
{
	//�Զ�������
	LogParam params;
	params.dir_ = "Meeting";	//��־·��
	params.hour_ = true;		//ÿСʱ����һ����־�ļ�
	params.keepDays_ = 90;		//��־����30��
	params.prefix_ = "Global";	//��־�ļ�ǰ׺
	params.extension_ = "daily";//��־�ļ���׺


	//������־���
	LogRecorder logRecorder;
	logRecorder.LogRecorderInit(params);
	logRecorder.LogRecorderStart();

	//��ӡ��־
	logRecorder.PrintMessage("%d relevant personnel attended the meeting today", 96);

	//��ͣ��־���
	logRecorder.LogRecorderStop();

	//�ر���־���
	logRecorder.LogRecorderClose();
}
void main()
{
	PrintMessageDefault();

	PrintMessageSetup();

	std::cin.get();
}