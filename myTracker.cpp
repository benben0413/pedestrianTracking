#include "myTracker.h"
//�����õ�pedArea�洢��tracker�У����ﲻ��ֱ��ʹ�����ðɣ�
//������ʹ��ͬһ�ڴ�ռ䣬������ĸı䣬���������Ӱ�졣��ò��û�����⣬�ȳ��������ٽ����޸İ�

Tracker::Tracker()
{
	//��ɶ�kalman�˲����ĳ�ʼ������������֮���tracklet��Ԥ�����
	KF = cv::KalmanFilter(stateNum, measureNum, 0);
	state = cv::Mat(stateNum, 1, CV_32F);//�˲���״̬����
	processNoise = cv::Mat(stateNum, 1, CV_32F);//�˲�����������
	measurement = cv::Mat::zeros(measureNum, 1, CV_32F);//�˲�����������
	KF.transitionMatrix = *( cv::Mat_<float>(4, 4) << 1,0,1,0,0,1,0,1,0,0,1,0,0,0,0,1 );//ת�ƾ���

	setIdentity(KF.measurementMatrix);//��������
	setIdentity(KF.processNoiseCov, cv::Scalar::all(1e-5));
	setIdentity(KF.measurementNoiseCov, cv::Scalar::all(1e-1));
	setIdentity(KF.errorCovPost, cv::Scalar::all(1));
	randn(KF.statePost, cv::Scalar::all(0), cv::Scalar::all(0.1));

	//�Զ���������ȡģ����г�ʼ������
	extractor = FeatureExtractor();
	extractor.initCache();
	lockedPedArea = NULL;
	trackerletHead = NULL;
}


void Tracker::setLoackedPedArea(LockedArea* result)
{
	lockedPedArea = result;//ֱ��ָ��ͬһ�ڴ�ռ�ͺ��ˣ���ʱ�����������������ܻ�Ҫ���иı�
}
//���¹��̵�˼·����
//һ��ֱ�Ӹ������˼����ο����tracklet��ȡ
//���Ǹ���kalmanԤ��������tracklet��ȡ������֮ǰtracklet���бȽ�
//�������ڿɽ��ܷ�Χ�ڣ�����и��£�������Ϊ����ʧ�ܣ���Ҫ��detector���ͼ������
//����ֵΪisRequset,��ʾ��ǰ���º��Ƿ���Ҫ���м�⣬����ʧ������Ҫ���м�⣬
bool Tracker::update(cv::Mat &sourceImage,bool haveRectBoxing)
{
	if(haveRectBoxing)//��ʾ��ǰ�����ʳ�¯�����˼����ο򣬲���Ҫ����Ԥ����̣��д���ȶ
	{
		if(lockedPedArea == NULL)//��⣬����û�м�⵽���ˡ���Ȼû�м�⵽���˵�����Ȼ���ܹ�ʹ��Ԥ�ⷽ����
			//ԭ��֮ǰ��Ԥ�����Ѿ��������⵱ǰͬ���޷�����Ԥ�����
			return true;
		else//�м�����˿��Ը��ݼ�����˶�tracklet���и��¹��̣�������ζ�tracklet���й���û����ȷ��˼·����ʱ�Ƚ���tracklet���й���
			//��д��һ�����̳�����
		{
			//����lockedPedArea�����µ�tracklet
			int topLeftX = lockedPedArea->topLeftX;
			int topLeftY = lockedPedArea->topLeftY;
			int width = lockedPedArea->width;
			int height = lockedPedArea->height;

			int letWidth = width * 0.4;
			int letHeight = height * 0.18;
			int letTopLeftX = topLeftX + width * 0.3;
			int letTopLeftY = topLeftY + height * 0.25;
			cv::Mat subImage = sourceImage(cv::Rect(letTopLeftX,letTopLeftY,letWidth,letHeight));
			cv::rectangle(sourceImage,cv::Rect(letTopLeftX,letTopLeftY,letWidth,letHeight),cv::Scalar(255,0,0),2);

			blockFeature target;
			extractor.computeFeature(subImage,target);

			Trackerlet* trackerlet = new Trackerlet();
			trackerlet->topLeftX = letTopLeftX;
			trackerlet->topLeftY = letTopLeftY;
			trackerlet->width = letWidth;
			trackerlet->Height = letHeight;
			trackerlet->next = NULL;
			trackerlet->setBlockFeature(target);
			trackerlet->trackerletID = 0;//���ID��ʱû��ʲô���壬����ʱ��������

			if(trackerletHead != NULL)
			{
				delete trackerletHead;
			}
			trackerletHead = trackerlet;
			return false;
		}
	}
	else
	{
		//��ǰtrackerletHead�ǿ�,���Գ��Խ��бȽϣ�������ν���Ԥ���أ���û����ȷ�ĸ�������

		return false;
	}
}