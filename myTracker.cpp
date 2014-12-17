#include "myTracker.h"
//�����õ�pedArea�洢��tracker�У����ﲻ��ֱ��ʹ�����ðɣ�
//������ʹ��ͬһ�ڴ�ռ䣬������ĸı䣬���������Ӱ�졣��ò��û�����⣬�ȳ��������ٽ����޸İ�

#define HAVE_BORDER 1
//ʹ�ñ߽�Ԥ�⣬���Ƕ��䲻���и�ֵ���Խ�����Ԥ��Ч������������ʱ��׷����ԭ���Ƚ�����������ͨ����ܴ����
Tracker::Tracker()
{
	//��ɶ�kalman�˲����ĳ�ʼ������������֮���tracklet��Ԥ�����

#if HAVE_BORDER == 1
	stateNum = 8;
	measureNum = 4;
#else
	stateNum = 4;
	measureNum = 2;
#endif

	KF = cv::KalmanFilter(stateNum, measureNum, 0);
	state = cv::Mat(stateNum, 1, CV_32F);//�˲���״̬����
	processNoise = cv::Mat(stateNum, 1, CV_32F);//�˲�����������
	measurement = cv::Mat::zeros(measureNum, 1, CV_32F);//�˲�����������

#if HAVE_BORDER == 1
	KF.transitionMatrix = *( Mat_<float>(8, 8) << 
		1,0,1,0,0,0,0,0,
		0,1,0,1,0,0,0,0,
		0,0,1,0,0,0,0,0,
		0,0,0,1,0,0,0,0,
		0,0,0,0,1,0,1,0,
		0,0,0,0,0,1,0,1,
		0,0,0,0,0,0,1,0,
		0,0,0,0,0,0,0,1);//ת�ƾ���
#else
	KF.transitionMatrix = *(Mat_<float>(4,4) << 1,0,1,0,0,1,0,1,0,0,1,0,0,0,0,1);
#endif

	setIdentity(KF.measurementMatrix);//��������
	setIdentity(KF.processNoiseCov, cv::Scalar::all(1e-5));
	setIdentity(KF.measurementNoiseCov, cv::Scalar::all(1e-1));
	setIdentity(KF.errorCovPost, cv::Scalar::all(1));
	//KF.statePost = *(Mat_<float>(4,1) << 320,240,1,1);
	randn(KF.statePost, Scalar::all(0), Scalar::all(0.1));

	//�Զ���������ȡģ����г�ʼ������
	extractor = FeatureExtractor();
	extractor.initCache();
	lockedPedArea = NULL;//����ͷ�ڵ�
	distratorList = NULL;//����ͷ�ڵ�
	targetTrackerlet = NULL;//��ǰָ���

	//Ȩ�س�ʼ������
	for(int i = 0; i < 8; ++i)
	{
		weights[i] = 1.0 / 8;
	}

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
//Ӧ�ö��Һ��б�Ҫ���߿���Ϣ���ǽ�����ֱ��ʹ��֮ǰ�ı߿���Ϣ�����ԣ�
//�������Ͻǵ��Ԥ�⻹����һ��Ч����
//��дupdate���������Ȩ���޸Ĳ���
bool Tracker::update(cv::Mat &sourceImage)
{
	if(targetTrackerlet == 0)//��ǰtargetTrackerletΪ��
	{
		if(lockedPedArea != NULL)//������Ҫ�ٶ�����Ƶ�ĳ�ʼ�׶����ҽ���Ŀ�����˳��ֲ��ɼ�⣬��һ�޶�����
		{
			Trackerlet* trackerlet = new Trackerlet();
			extractTracklet(sourceImage,lockedPedArea,trackerlet);
			targetTrackerlet = trackerlet;//������Ϊ֮ǰû�д���traget���������ֱ�Ӹ�ֵ��û������

			//����ǰ����ֱֵ����ԭͼ�Ͻ��л��ƣ���ɫ��ʾ����ֵ
			circle(sourceImage,cv::Point(trackerlet->topLeftX,trackerlet->topLeftY),5,CV_RGB(255,0,0),3);

			//���ݵ�ǰ���ֵ��kalman���������������predict������ӣ��������Ԥ������û�в����ĵģ���Ϊ���ڼ��ֵ
			Mat prediction = KF.predict();
			measurement.at<float>(0) = (float)trackerlet->topLeftX;
			measurement.at<float>(1) = (float)trackerlet->topLeftY;
			KF.correct(measurement);//���õ�ǰ����ֵ�����˲�����������
			
			//����ֱ���϶����õ�������ȷ����Ȼ�Ǵ�������ģ�
			return false;
		}
		else
		{
			return true;//��ʾ��ǰ�����ڼ����ο���Ҫ��һ���ļ����̣�
		}
	}
	//���ﻹ��Ҫ����һ�£�˼·һ��Ҫ����
	else//��ǰtarget����ֵ�����Խ��бȽϼ�Ԥ�����
	{
		bool haveNewTarget = false;
		Trackerlet* newTargetTrackerlet = NULL;//���ڴ洢�µõ�tracklet�������Ǽ��õ�����Ԥ��õ�

		//���ȶ�lockedPedArea���б����жϣ����������һ���Ƿ���targetTrackerlet���������û�з��֣�����ϻ�������
		LockedArea* current = lockedPedArea;//�����ж�
		while(current != NULL)
		{
			Trackerlet* trackerlet = new Trackerlet();
			extractTracklet(sourceImage,current,trackerlet);
			double distinguishValue = this->distinguish(targetTrackerlet->featureSet,trackerlet->featureSet);
			std::cout<<"����ֵΪ��"<<distinguishValue<<std::endl;
			if(distinguishValue < 0.3)//�϶�ΪĿ������
			{
				newTargetTrackerlet = trackerlet;
				current = current->next;
				break;
			}
			else//����distrator�б�
			{
				insertDistrator(trackerlet);
				current = current->next;
			}
		}
		//��ʣ��tracklet����distrator
		while(current != NULL)
		{
			Trackerlet* trackerlet = new Trackerlet();
			extractTracklet(sourceImage,current,trackerlet);
			current = current->next;
		}

	
	}

	return false;
}
bool Tracker::update(cv::Mat &sourceImage,bool haveRectBoxing)
{
	//��ʾ��ǰ�����ʳ�¯�����˼����ο򣬲���Ҫ����Ԥ����̣��д���ȶ
	if(haveRectBoxing && lockedPedArea != NULL)
	{

		Trackerlet* trackerlet = new Trackerlet();
		extractTracklet(sourceImage,lockedPedArea,trackerlet);
		circle(sourceImage,cv::Point(trackerlet->topLeftX,trackerlet->topLeftY),5,CV_RGB(255,0,0),3);

		//���ݵ�ǰ���ֵ��kalman���������������predict������ӣ��������Ԥ������û�в����ĵģ���Ϊ���ڼ��ֵ
		Mat prediction = KF.predict();

		measurement.at<float>(0) = (float)trackerlet->topLeftX;
		measurement.at<float>(1) = (float)trackerlet->topLeftY;

		KF.correct(measurement);//���õ�ǰ����ֵ�����˲�����������

		targetTrackerlet = trackerlet;//����ֱ���滻Ҳ�ǲ���ȷ�ģ��滻��Ҫ��Ȩ�ؼ������֮��Ž���
		//����ֱ���϶����õ�������ȷ����Ȼ�Ǵ�������ģ�
		return false;

	}
	else
	{
		//��ǰtrackerletHead�ǿ�,���Գ��Խ��бȽϣ�������ν���Ԥ���أ�
		//����и��ٹ���
		Mat prediction = KF.predict();//�����˲����Ե�ǰ���tracklet���ν���Ԥ��
		float *data = prediction.ptr<float>(0);
		int predictX = data[0];
		int predictY = data[1];
		std::cout<<predictX<<" "<<predictY<<" "<<std::endl;

		if (targetTrackerlet == NULL)
			return true;
		else
		{
			//���ｫ���ݵ�ǰ�õ�tracklet��֮ǰtracklet����Ԥ��ƥ�䣬������ƶȿ�������������ɾ�����������������
			//������൫Ҫ˼·����
			int letWidth = targetTrackerlet->width;
			int letHeight = targetTrackerlet->height;
			cv::Mat subImage = sourceImage(cv::Rect(predictX,predictY,letWidth,letHeight));
			blockFeature target;
			extractor.computeFeature(subImage,target);

			//����ǰ�õ�blockfeature��֮ǰ�洢���ݽ��бȽ�
			double distinguish = this->distinguish(targetTrackerlet->featureSet,target);
			std::cout<<"����ֵΪ��"<<distinguish<<std::endl;
			if(distinguish > 0.35)
				return true;
			else
			{
				//��tracklet���и��¹��̣���Ȩ�ص����������
				//trackerletHead->setBlockFeature(target);
				//trackerletHead->topLeftX = predictX;
				//trackerletHead->topLeftY = predictY;
				//measurement.at<float>(0) = (float)predictX;
				//measurement.at<float>(1) = (float)predictY;
				//KF.correct(measurement);����ֱ����Ԥ��ֵ������������Ȼ�ǲ������
				circle(sourceImage,cv::Point(predictX,predictY),5,CV_RGB(0,255,0),3);
				cv::rectangle(sourceImage,cv::Rect(predictX,predictY,letWidth,letHeight),cv::Scalar(255,0,0),2);
				return false;
			}
		}
	}
}

//�������һ���������ֵ����ݣ�������ȡ��Ϊһ����������ʵ��
//���ݾ��ο���ȡtacklet
void Tracker::extractTracklet(cv::Mat &sourceImage,LockedArea* lockedPedArea,Trackerlet* trackerlet)
{
	int topLeftX = lockedPedArea->topLeftX;
    int topLeftY = lockedPedArea->topLeftY;
	int width = lockedPedArea->width;
	int height = lockedPedArea->height;
	cv::Rect rect(topLeftX,topLeftY,width,height);

	int letWidth = rect.width * 0.4;
	int letHeight = rect.height * 0.18;
	int letTopLeftX = rect.x + rect.width * 0.3;
	int letTopLeftY = rect.y + rect.height * 0.25;
	cv::Mat subImage = sourceImage(cv::Rect(letTopLeftX,letTopLeftY,letWidth,letHeight));

	blockFeature target;
	extractor.computeFeature(subImage,target);

	trackerlet->trackerletID = 0;//���ID��ʱû��ʲô���壬����ʱ��������
	trackerlet->topLeftX = letTopLeftX;
	trackerlet->topLeftY = letTopLeftY;
	trackerlet->width = letWidth;
	trackerlet->height = letHeight;
	trackerlet->setBlockFeature(target);
	trackerlet->next = NULL;

	//�Ծ��ο���б궨
	cv::rectangle(sourceImage,cv::Rect(letTopLeftX,letTopLeftY,letWidth,letHeight),cv::Scalar(255,0,0),2);
}
void Tracker::insertDistrator(Trackerlet* tracklet)
{

}


//���㵱ǰ������Ŀ������֮��Ĳ�ֵ
double Tracker::distinguish(blockFeature& target, blockFeature& current)
{
	cv::MatND targetLBP = cv::Mat(target.cs_lbpFeature);
	cv::MatND currentLBP = cv::Mat(current.cs_lbpFeature);
	cv::MatND targetCanny = cv::Mat(target.cannyFeature);
	cv::MatND currentCanny = cv::Mat(current.cannyFeature);

	double hueDistance = compareHist(target.hueHist,current.hueHist,CV_COMP_BHATTACHARYYA);
	double satDistance = compareHist(target.satHist,current.satHist,CV_COMP_BHATTACHARYYA);
	double valDistance = compareHist(target.valHist,current.valHist,CV_COMP_BHATTACHARYYA);
	double lbpDistance = compareHist(targetLBP,currentLBP,CV_COMP_BHATTACHARYYA);
	double cannyDistance = compareHist(targetCanny,currentCanny,CV_COMP_BHATTACHARYYA);
	double horDerDistance = compareHist(target.horDerHist,current.horDerHist,CV_COMP_BHATTACHARYYA);
	double verDerDistance = compareHist(target.verDerHist,current.verDerHist,CV_COMP_BHATTACHARYYA);

	cv::MatND targetEHD = cv::Mat(5,1,CV_32F);
	cv::MatND currentEHD = cv::Mat(5,1,CV_32F);
	for(int i = 0; i < 5; i++)
	{
		float* targetPtr = targetEHD.ptr<float>(i);
		float* currentPtr = currentEHD.ptr<float>(i);
		targetPtr[0] = target.EHD[i];
		currentPtr[0] = current.EHD[i];
	}
	double EHDDistance = compareHist(targetEHD,currentEHD,CV_COMP_BHATTACHARYYA);
	//��ɾ��������̣�

	//���㵱ǰͼ�����Ŀ��ͼ���Ĳ���ֵ
	double dissimilarity = weights[0] * hueDistance + weights[0] * satDistance + weights[0] * valDistance + 
		weights[0] * lbpDistance + weights[0] * cannyDistance + weights[0] * horDerDistance + 
		weights[0] * verDerDistance + weights[0] * EHDDistance;

	std::cout<<"dissimilarity is :"<<dissimilarity<<std::endl;
	return dissimilarity;
}
//���ݵ�ǰcurrent����ȷĿ�꣩��preTarget����ǰ�洢�ģ���distrator�����������ݣ�
void Tracker::featureWeighting(blockFeature& current)
{
	//���������и����Ĺ�ʽ�ֱ����������Ͼ���
	cv::MatND targetEHD = cv::Mat(5,1,CV_32F);
	cv::MatND currentEHD = cv::Mat(5,1,CV_32F);

	//���current����������distrator��feature���Ͼ����ֵ
	double meanhueDistance = 0,meansatDistance = 0,meanvalDistance = 0;
	double meanlbpDistance = 0,meancannyDistance = 0;
	double meanhorDerDistance = 0,meanverDerDistance = 0,meanEHDDistance = 0;

	cv::MatND currentLBP = cv::Mat(current.cs_lbpFeature);
	cv::MatND currentCanny = cv::Mat(current.cannyFeature);

	int count = 0;
	Trackerlet *distratorPtr = distratorList;
	while(distratorPtr != NULL)
	{
		cv::MatND targetLBP = cv::Mat(distratorPtr->featureSet.cs_lbpFeature);
		cv::MatND targetCanny = cv::Mat(distratorPtr->featureSet.cannyFeature);

		meanhueDistance = meanhueDistance + compareHist(distratorPtr->featureSet.hueHist,current.hueHist,CV_COMP_BHATTACHARYYA);
		meansatDistance = meansatDistance + compareHist(distratorPtr->featureSet.satHist,current.satHist,CV_COMP_BHATTACHARYYA);
		meanvalDistance = meanvalDistance + compareHist(distratorPtr->featureSet.valHist,current.valHist,CV_COMP_BHATTACHARYYA);
		meanlbpDistance = meanlbpDistance + compareHist(targetLBP,currentLBP,CV_COMP_BHATTACHARYYA);
		meancannyDistance = meancannyDistance + compareHist(targetCanny,currentCanny,CV_COMP_BHATTACHARYYA);
		meanhorDerDistance = meanhorDerDistance + compareHist(distratorPtr->featureSet.horDerHist,current.horDerHist,CV_COMP_BHATTACHARYYA);
		meanverDerDistance = meanverDerDistance + compareHist(distratorPtr->featureSet.verDerHist,current.verDerHist,CV_COMP_BHATTACHARYYA);
		for(int i = 0; i < 5; i++)
		{
			float* targetPtr = targetEHD.ptr<float>(i);
			float* currentPtr = currentEHD.ptr<float>(i);
			targetPtr[0] = distratorPtr->featureSet.EHD[i];
			currentPtr[0] = current.EHD[i];
		}
		meanEHDDistance = meanEHDDistance + compareHist(targetEHD,currentEHD,CV_COMP_BHATTACHARYYA);
	}
	if(count != 0)
	{
		meanhueDistance = meanhueDistance / count;
		meansatDistance = meansatDistance / count;
		meanvalDistance = meanvalDistance / count;
		meanlbpDistance = meanlbpDistance / count;
		meancannyDistance = meancannyDistance / count;
		meanhorDerDistance = meanhorDerDistance / count;
		meanverDerDistance = meanverDerDistance / count;
		meanEHDDistance = meanEHDDistance / count;
	}
	
	//���current��preTarget��feature���Ͼ���ļ���
	double hueDistance = 0,satDistance = 0,valDistance = 0;
	double lbpDistance = 0,cannyDistance = 0;
	double horDerDistance = 0,verDerDistance = 0,EHDDistance = 0;

	cv::MatND targetLBP = cv::Mat(targetTrackerlet->featureSet.cs_lbpFeature);
	cv::MatND targetCanny = cv::Mat(targetTrackerlet->featureSet.cannyFeature);

	hueDistance = compareHist(targetTrackerlet->featureSet.hueHist,current.hueHist,CV_COMP_BHATTACHARYYA);
	satDistance = compareHist(targetTrackerlet->featureSet.satHist,current.satHist,CV_COMP_BHATTACHARYYA);
	valDistance = compareHist(targetTrackerlet->featureSet.valHist,current.valHist,CV_COMP_BHATTACHARYYA);
	lbpDistance = compareHist(targetLBP,currentLBP,CV_COMP_BHATTACHARYYA);
	cannyDistance = compareHist(targetCanny,currentCanny,CV_COMP_BHATTACHARYYA);
	horDerDistance = compareHist(targetTrackerlet->featureSet.horDerHist,current.horDerHist,CV_COMP_BHATTACHARYYA);
	verDerDistance = compareHist(targetTrackerlet->featureSet.verDerHist,current.verDerHist,CV_COMP_BHATTACHARYYA);

	for(int i = 0; i < 5; i++)
	{
		float* targetPtr = targetEHD.ptr<float>(i);
		float* currentPtr = currentEHD.ptr<float>(i);
		targetPtr[0] = targetTrackerlet->featureSet.EHD[i];
		currentPtr[0] = current.EHD[i];
	}
	EHDDistance = compareHist(targetEHD,currentEHD,CV_COMP_BHATTACHARYYA);
	
	//��ɶ�feature�ĸ�Ȩ�ص������̣����������һ�ַ����������Ƿ�����õķ����أ����д���һ����ȷ��
	weights[0] = weights[0] + (meanhueDistance - hueDistance);
	weights[1] = weights[1] + (meansatDistance - satDistance);
	weights[2] = weights[2] + (meanvalDistance - valDistance);
	weights[3] = weights[3] + (meanlbpDistance - lbpDistance);
	weights[4] = weights[4] + (meancannyDistance - cannyDistance);
	weights[5] = weights[5] + (meanhorDerDistance - horDerDistance);
	weights[6] = weights[6] + (meanverDerDistance - verDerDistance);
	weights[7] = weights[7] + (meanEHDDistance - EHDDistance);

	//��һ������
	double sum = 0;
	for(int i = 0; i < 8; ++i)
	{
		sum = sum + weights[i];
	}
	for(int i = 0; i < 8; ++i)
	{
		weights[i] = weights[i] / sum;
	}
	//���Ȩ�ص��������ǻ���֪��Ч�����
}