#include "manager.h"


Manager::Manager()
{
	front = 0;//��ʼ���б�����Ϊ��
	rear = 0;
	for(int i = 0; i < capacity; i++)
	{
		targetPool[i] = NULL;
	}
	discriminator = Discriminator();//����Ĭ�Ϲ��캯��
	acc = 0;
}

void Manager::setTrackerletList(Trackerlet* list)
{
	trackerletList = list;
}

void Manager::updateWeights(double weights[])
{
	discriminator.setWeights(weights);
}

//���ݴ���trackerlet�ж���ǰ����Ŀ���Ƿ�Ϊָ��Ŀ�꣬ͬʱ��������Ϣ���и���
//�ڸ���Ŀ����ָ��Ŀ�겻һ�µ������£�return false������tracker��������
//Ĭ�������һ���ڵ�Ϊ����Ŀ��
//����ֵ��ʾ��ͷ����Ƿ�Ϊ���ٽڵ㡣�� true���� false
bool Manager::dicision()
{
	//��������������ж�ͷ����Ƿ�Ϊָ��Ŀ�ꣻ����Ŀ���

	//�жϵ�ǰtrackerletListͷ����Ƿ�Ϊ���ٽڵ�
	if(trackerletList == NULL)
		return true;//�����������������£���Ȼ�޷������������̣���Ȼ����true������֪����û�д���ô�ģ��е����
	
	if(front == rear)//��ǰĿ���Ϊ�գ�ֱ�Ӽ�������ͷ���
	{
		trackerletList->occupied++;
		targetPool[front++] = trackerletList;//��ͷ����������
		return true;
	}
	else//Ŀ��طǿյ������£���Ҫ��ͷ�������ж���
	{
		discriminator.clearDistance();
		//���Ƚ�ͷ���blockfeature������ʱ�����discriminator
		discriminator.setCurrentFeature(trackerletList->featureSet);

		//��ͷ���������Ŀ����д洢���ݽ��бȽ�
		int traversal = rear;
		while(traversal != front)
		{   
			blockFeature distratorFeature = targetPool[traversal]->featureSet;
			discriminator.computeDistanceHold(distratorFeature);
			traversal = (traversal + 1) % capacity;
		}
		double dissimilarity = discriminator.distinguish();//�õ���ǰtarget��Ŀ���������Ԫ�صĲ���ֵ
		if(dissimilarity > 1.0)//��ʾͷ��㲻�Ǹ���Ŀ��,��Ҫ��listʣ�����ݽ��б�����Ѱ���Ƿ���ڸ���Ŀ��
		{
			correctTarget = haveTarget();//�õ�����trackerlet
			if(correctTarget != NULL)
			{
				updatePool(correctTarget);
			}
			return false;
		}
		else
		{
			//�϶���ǰͷ�������Ϊָ������Ŀ�꣬�ڱ�Ҫ�������¶�Ŀ��ؽ��и���
			updatePool(trackerletList);
			return true;
		}
	}
}

//�ж�trackerlet����ʣ���������Ƿ����֮ǰָ������Ŀ�꣬���򷵻�ָ��ָ�룬���򷵻�NULL
Trackerlet* Manager::haveTarget()
{
	Trackerlet* current = trackerletList->next;
	while(current != NULL)
	{
		discriminator.clearDistance();
		//���Ƚ�ͷ���blockfeature������ʱ�����discriminator
		discriminator.setCurrentFeature(trackerletList->featureSet);

		//��ͷ���������Ŀ����д洢���ݽ��бȽ�
		int traversal = rear;
		while(traversal != front)
		{   
			blockFeature distratorFeature = targetPool[traversal]->featureSet;
			discriminator.computeDistanceHold(distratorFeature);
			traversal = (traversal + 1) % capacity;
		}
		double dissimilarity = discriminator.distinguish();//�õ���ǰtarget��Ŀ���������Ԫ�صĲ���ֵ
		if(dissimilarity < 1.0)
		{
			return current;
		}
		else
		{
			current = current->next;
		}
	}
	return NULL;
}

//������Ϊֹ��˼�����ݻ���������̵�˼�룬����
//��Ŀ��ؽ��и��£���ȷtarget��ǰ���£�����֮ǰ��insertDistrator��������Ҫ��ʱ���������ƿ�
//�������Ϊ��ǰȷ��target
/***********�ɸĽ������һ��**********/
void Manager::updatePool(Trackerlet* target)
{
	//������Ҫ�趨���¼��
	//ע��front ָ���ͷ��һ��Ԫ�أ�rearָ���βԪ��
	acc++;
	if(acc > interval)
	{
		acc = 0;//���ۼ�������������������ﻹ�������¿������ģ��ǽ�����oldestɾ���������趨һЩ�ж����ݣ�
		//Ϊ�˱�֤Ŀ������ݵĶ����ԣ�����Ϊ�ӽ���һ������ɾ�������������Ļ���ʹ�������ȽϷ��㣬�Ͼ��漰����
		//����ɾ��������֮���ٽ��п��ǰɣ�mark...
		target->occupied++;
		if((front + 1)%capacity == rear)//����
		{	
			//����֮ǰ��Ҫ����βԪ�س���	
			Trackerlet* tmp = targetPool[rear];
			tmp->occupied--;
			if(tmp->occupied == 0)
			{
				delete tmp;//�б�Ҫ��Ԫ��ɾ��
				tmp = NULL;
			}
			rear = (rear + 1)%capacity;

			targetPool[front] = target;
			front = (front + 1)%capacity;
		
			//����ͱ�����Ҫ����һЩ����Ϊ�����Ǵ洢��ָ����Ϣ�����Է���Ľ��и�ֵ����
		}
		else//��δ������ֱ�ӽ��в������
		{
			targetPool[front] = target;
			front = (front + 1)%capacity;
		}
	}
}

Trackerlet* Manager::correct()
{
	return correctTarget;
}