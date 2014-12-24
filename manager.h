
//ʵ�ֶ�trackerlet�Ĺ��������ڱ�Ҫ��ʱ����ɸ���Ŀ����ڵ�����
//�ܹ���Ŀ���ٴγ���ʱ�����ٶ�λ
#pragma once
#include "tracker.h"
#include "discriminator.h"

//������������trackerʹ�õ���ͬһ���ֱ������б�Ҫ�ֳ�������ô��
//��Ȼ�ǲ��ֿܷ��ģ����ǲ��ֿ��ܹ��ܺõ����ô�����ܣ��������Խ�weights����ˢ�µ���ǰdiscriminator��
class Manager
{
private:
	Trackerlet* trackerletList;//���ɼ��㴫�ݹ�����trackerlet�б�

	Trackerlet* targetPool[6];//Ŀ��أ������ʶ��Ŀ�����ʷ��Ϣ��������ܣ����ʱ��x��Ŀ����أ�ͬʱ�޳�oldest
	Trackerlet* correctTarget;
	static const int capacity = 6;//Ŀ���������Ϣ
	int front;
	int rear;
	static const int interval = 30;//��̬������Ŀ����ص�ʱ����
	int acc;//���ڶ�������´��������ۼӣ�
	Discriminator discriminator;//�ֱ���������trackerlet���в���ֵ����

	//���ض�Ŀ����д洢�����ڶ��ض�Ŀ������ж����̣�ͬʱ���ڵ��������ܹ�������ʷ��Ϣ
	//��Ŀ���ٴ�ʶ�𣬽���������һ����Ϣ�ǲ��ܹ���Ŀ���ж������ģ����ڵ������󻷾����ϴ��Ǵ��ڽ϶಻ͬ��
public:
	Manager();

	void setTrackerletList(Trackerlet* list);
	void updateWeights(double weights[]);//�Էֱ�������Ȩ�ؽ��и���
	bool dicision();//���ݴ���trackerlet�ж���ǰ����Ŀ���Ƿ�Ϊָ��Ŀ�꣬ͬʱ��������Ϣ���и���
	//�ڸ���Ŀ����ָ��Ŀ�겻һ�µ������£�return false������tracker��������
	Trackerlet* haveTarget();//����listѰ���Ƿ���ڸ���Ŀ��

	Trackerlet* correct();//������ɾ��ߣ��϶���ǰtracker����Ŀ����ָ��Ŀ�겻ͬ�������������Ҳ���ǽ���ǰָ��tracklet������tracker

	void updatePool(Trackerlet* target);//��Ŀ��ؽ��и���
};