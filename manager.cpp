#include "manager.h"

Manager::Manager()
{
	front = 0;//��ʼ���б�����Ϊ��
	rear = 0;
	for(int i = 0; i < capacity; i++)
	{
		targetPool[i] = NULL;
	}
}

void Manager::setTrackerletList(Trackerlet* list)
{
	trackerletList = list;
}
//���ݴ���trackerlet�ж���ǰ����Ŀ���Ƿ�Ϊָ��Ŀ�꣬ͬʱ��������Ϣ���и���
//�ڸ���Ŀ����ָ��Ŀ�겻һ�µ������£�return false������tracker��������
//Ĭ�������һ���ڵ�Ϊ����Ŀ��
bool Manager::dicision()
{

	return true;
}