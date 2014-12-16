#include "myTracker.h"
//�����õ�pedArea�洢��tracker�У����ﲻ��ֱ��ʹ�����ðɣ�
//������ʹ��ͬһ�ڴ�ռ䣬������ĸı䣬���������Ӱ�졣��ò��û�����⣬�ȳ��������ٽ����޸İ�

Tracker::Tracker()
{
	//��ɶ�kalman�˲����ĳ�ʼ������������֮���tracklet��Ԥ�����
	stateNum = 4;
	measureNum = 2;

	KF = cv::KalmanFilter(stateNum, measureNum, 0);
	state = cv::Mat(stateNum, 1, CV_32F);//�˲���״̬����
	processNoise = cv::Mat(stateNum, 1, CV_32F);//�˲�����������
	measurement = cv::Mat::zeros(measureNum, 1, CV_32F);//�˲�����������
	//KF.transitionMatrix = *( Mat_<float>(8, 8) << 1,0,1,0,0,0,0,0,0,1,0,1,0,0,0,0,
	//	0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,
	//	0,0,0,0,1,0,1,0,0,0,0,0,0,1,0,1,
	//	0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1);//ת�ƾ���
	KF.transitionMatrix = *(Mat_<float>(4,4) << 1,0,1,0,0,1,0,1,0,0,1,0,0,0,0,1);

	setIdentity(KF.measurementMatrix);//��������
	setIdentity(KF.processNoiseCov, cv::Scalar::all(1e-5));
	setIdentity(KF.measurementNoiseCov, cv::Scalar::all(1e-1));
	setIdentity(KF.errorCovPost, cv::Scalar::all(1));
	//KF.statePost = *(Mat_<float>(4,1) << 320,240,1,1);
	randn(KF.statePost, Scalar::all(0), Scalar::all(0.1));

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

			//���ݵ�ǰ���ֵ��kalman��������
			Mat prediction = KF.predict();
			float *data = prediction.ptr<float>(0);
			double predictX = data[0];
			double predictY = data[1];
			//double predictW = data[2];
			//double predictH = data[3];
			circle(sourceImage,cv::Point(predictX,predictY),5,CV_RGB(0,255,0),3);

			measurement.at<float>(0) = (float)letTopLeftX;
			measurement.at<float>(1) = (float)letTopLeftY;
			//measurement.at<float>(2) = (float)letWidth;
			//measurement.at<float>(3) = (float)letHeight;
			KF.correct(measurement);//���õ�ǰ����ֵ�����˲�����������

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
		//��ǰtrackerletHead�ǿ�,���Գ��Խ��бȽϣ�������ν���Ԥ���أ���û����ȷ�ĸ�������,���ｫ����kalman�˲�����Ԥ��
		//����и��ٹ���
		Mat prediction = KF.predict();//�����˲����Ե�ǰ���tracklet���ν���Ԥ��
		float *data = prediction.ptr<float>(0);
		int predictX = data[0];
		int predictY = data[1];
		//int predictW = data[2];
		//int predictH = data[3];
		std::cout<<predictX<<" "<<predictY<<" "<<std::endl;//<<predictW<<" "<<predictH<<std::endl;
		circle(sourceImage,cv::Point(predictX,predictY),5,CV_RGB(0,255,0),3);
		//cv::Mat subImage = sourceImage(cv::Rect(predictX,predictY,predictW,predictH));
		//cv::rectangle(sourceImage,cv::Rect(predictX,predictY,predictW,predictH),cv::Scalar(255,0,0),2);

		return true;
	}
}