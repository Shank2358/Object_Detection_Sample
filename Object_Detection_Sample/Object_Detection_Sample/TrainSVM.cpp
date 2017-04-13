/* --------------------------------------------------------
* author��livezingy
*
* BLOG��http://www.livezingy.com
*
* Development Environment��
*      Visual Studio V2013
*      opencv3.1
*      Tesseract3.04
*
* Version��
*      V1.0    20170408

--------------------------------------------------------- */
#include "TrainSVM.h"
#include "util.h"
#include "lbp.hpp"

using namespace cv;
using namespace cv::ml;
using namespace std;
int numDes;

extern string descriptorType;
extern string svmClassiferPath;
extern string svmDetectPath;

TrainSVM::TrainSVM() 
{
}

TrainSVM::~TrainSVM() 
{  
}

void TrainSVM::trainDetector(void)
{
	cv::Ptr<cv::ml::TrainData>& trainData = getDetectorSet();

	Ptr<cv::ml::SVM> svm = cv::ml::SVM::create();

	svm->setType(cv::ml::SVM::C_SVC);
	svm->setKernel(cv::ml::SVM::KernelTypes::LINEAR);
	svm->setTermCriteria(cvTermCriteria(CV_TERMCRIT_ITER, 40000, 0.00001));
	svm->trainAuto(trainData, 10, cv::ml::SVM::getDefaultGrid(cv::ml::SVM::C),
		cv::ml::SVM::getDefaultGrid(cv::ml::SVM::GAMMA), cv::ml::SVM::getDefaultGrid(cv::ml::SVM::P),
		cv::ml::SVM::getDefaultGrid(cv::ml::SVM::NU), cv::ml::SVM::getDefaultGrid(cv::ml::SVM::COEF),
		cv::ml::SVM::getDefaultGrid(cv::ml::SVM::DEGREE), true);

	
	//��ȡ֧��������������Ĭ����CV_32F
	Mat supportVector = svm->getSupportVectors();//

	//��ȡalpha��rho
	Mat alpha;//ÿ��֧��������Ӧ�Ĳ�����(�������ճ���)��Ĭ��alpha��float64��
	Mat svIndex;//֧���������ڵ�����
	float rho = svm->getDecisionFunction(0, alpha, svIndex);

	//ת������:����һ��Ҫע�⣬��Ҫת��Ϊ32��
	Mat alpha2;
	alpha.convertTo(alpha2, CV_32FC1);

	//������������������
	Mat result(1, numDes, CV_32FC1);
	result = alpha2*supportVector;

	//����-1������Ϊʲô�����-1��
	//ע����Ϊsvm.predictʹ�õ���alpha*sv*another-rho�����Ϊ���Ļ�����Ϊ������������HOG�ļ�⺯���У�ʹ��rho+alpha*sv*another(anotherΪ-1)
	for (int i = 0; i < numDes; ++i)
		result.at<float>(0, i) *= -1;

	//�����������浽�ļ�������HOGʶ��
	//��������������б����Ĳ���(��)��HOG����ֱ��ʹ�øò�������ʶ��
	FILE *fp = fopen(svmDetectPath.c_str(), "wb");
	for (int i = 0; i< numDes; i++)
	{
		fprintf(fp, "%f \n", result.at<float>(0, i));
	}
	fprintf(fp, "%f", rho);

	fclose(fp);

	fprintf(stdout, ">> Your SVM Detector was saved to %s\n", svmDetectPath);
}

void TrainSVM::trainClassifier(void)
{
	cv::Ptr<cv::ml::TrainData>& trainData = getClassiferSet();

	Ptr<cv::ml::SVM> svm = cv::ml::SVM::create();

	svm->setType(cv::ml::SVM::C_SVC);
	svm->setKernel(cv::ml::SVM::KernelTypes::RBF);
	svm->setTermCriteria(cvTermCriteria(CV_TERMCRIT_ITER, 40000, 0.00001));
	svm->trainAuto(trainData, 10, cv::ml::SVM::getDefaultGrid(cv::ml::SVM::C),
		cv::ml::SVM::getDefaultGrid(cv::ml::SVM::GAMMA), cv::ml::SVM::getDefaultGrid(cv::ml::SVM::P),
		cv::ml::SVM::getDefaultGrid(cv::ml::SVM::NU), cv::ml::SVM::getDefaultGrid(cv::ml::SVM::COEF),
		cv::ml::SVM::getDefaultGrid(cv::ml::SVM::DEGREE), true);

	fprintf(stdout, ">> Saving model file...\n");

	svm->save(svmClassiferPath);

	fprintf(stdout, ">> Your SVM classifer was saved to %s\n", svmClassiferPath);
}


cv::Ptr<cv::ml::TrainData> TrainSVM::getDetectorSet(void)
{
	Mat classes;
	cv::Mat samples;
	vector<string> imgPathTrain;

	vector<string> tmpImgTrain;

	hog = new HOGDescriptor(cvSize(198, 214), cvSize(38, 54), cvSize(8, 8), cvSize(19, 27), 3);

	cv::Mat feature;
	
	char buffer[260] = { 0 };

	sprintf(buffer, "etc/TrainDetector/has");
	imgPathTrain = utils::getFiles(buffer);
	vector<int> objPresentTrain(imgPathTrain.size(), 1);

	sprintf(buffer, "etc/TrainDetector/no");
	tmpImgTrain = utils::getFiles(buffer);

	for (auto file : tmpImgTrain)
	{
		imgPathTrain.push_back(file);
		objPresentTrain.push_back(0);
	}

	for (auto f : imgPathTrain)
	{
		auto image = cv::imread(f, 0);
		if (!image.data) {
			fprintf(stdout, ">> Invalid image: %s  ignore.\n", f.c_str());
			continue;
		}

		getHOGFeatures(image, feature);
		samples.push_back(feature);
	}

	numDes = samples.cols;

	samples.convertTo(samples, CV_32FC1);

	auto train_data = cv::ml::TrainData::create(samples, cv::ml::SampleTypes::ROW_SAMPLE,
		Mat(objPresentTrain));

	return train_data;
}

cv::Ptr<cv::ml::TrainData> TrainSVM::getClassiferSet(void)
{
	Mat classes;
	cv::Mat samples;
	vector<string> imgPathTrain;

	vector<string> tmpImgTrain;

	char buffer[260] = { 0 };

	sprintf(buffer, "etc/TrainClassifier/Big");
	imgPathTrain = utils::getFiles(buffer);
	vector<int> objPresentTrain(imgPathTrain.size(), 1);

	sprintf(buffer, "etc/TrainClassifier/small");
	tmpImgTrain = utils::getFiles(buffer);

	for (auto file : tmpImgTrain)
	{
		imgPathTrain.push_back(file);
		objPresentTrain.push_back(2);
	}

	sprintf(buffer, "etc/TrainClassifier/no");
	tmpImgTrain = utils::getFiles(buffer);

	for (auto file : tmpImgTrain)
	{
		imgPathTrain.push_back(file);
		objPresentTrain.push_back(0);
	}

	for (auto f : imgPathTrain)
	{
		auto image = cv::imread(f, 0);
		if (!image.data) {
			fprintf(stdout, ">> Invalid image: %s  ignore.\n", f.c_str());
			continue;
		}

		if (descriptorType == "LBP")
		{
			cv::Mat feature;
			getLBPFeatures(image, feature);
			samples.push_back(feature);
		}
		else
		{
			image = image.reshape(1, 1);
			samples.push_back(image);
		}
	}

	numDes = samples.cols;

	samples.convertTo(samples, CV_32FC1);

	auto train_data = cv::ml::TrainData::create(samples, cv::ml::SampleTypes::ROW_SAMPLE,
		Mat(objPresentTrain));

	return train_data;
}

void TrainSVM::getHOGFeatures(const cv::Mat& image, cv::Mat& features)
{
	std::vector<float> descriptor;
	
	//compute descripter
	hog->compute(image, descriptor, Size(8, 8));
	
	cv::Mat tmp(1, Mat(descriptor).cols, CV_32FC1); //< used for transposition if needed

	//hog����õ�������cols=1,rows=ά��
	transpose(Mat(descriptor), tmp);

	tmp.copyTo(features);
}

void TrainSVM::getLBPFeatures(const cv::Mat& image, cv::Mat& features)
{
	Mat lbpimage;
	lbpimage = libfacerec::olbp(image);

	//spatial_histogram�������ص�������Ϊ��������������ΪCV_32FC1
	Mat lbp_hist = libfacerec::spatial_histogram(lbpimage, 32, 4, 4);

	features = lbp_hist;
}