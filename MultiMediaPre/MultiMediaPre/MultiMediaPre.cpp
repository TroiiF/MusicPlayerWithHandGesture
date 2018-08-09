// MultiMediaPre.cpp : �������̨Ӧ�ó������ڵ㡣
//
/*
	����RGB->HSV�ռ䣬Ȼ���ҳ���Ӧ��ɫ�������ж�ǰ����֡��λ�ò��죬����Ļ�����������ĸ��������
��أ�ʵ�֣�
			�������ƶ� -> ��������
			//�������ƶ� -> ������С
			�������ƶ� -> ����/��ͣ
			�������ƶ� -> ��һ��
	��Ƶ���Ų���win32�Դ���api��mciSendString�������п��ơ�
	һ�������׸���������ǰָ������֧���ļ�������ļ��е��빦�ܣ�ûʱ������ô�����ˣ�����
*/

#include "stdafx.h"
#include <iostream>  
#include <string>   
#include <opencv2\opencv.hpp>
#include<windows.h>
#include <MMSYSTEM.H>
#include<dsound.h>

#pragma comment(lib, "WINMM.LIB")

using namespace cv;
using namespace std;

int main(int argc, char *argv[])
{
	// ��Ƶ�������
	VideoCapture cap(0);
	int height = 480;
	int width = 640;
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, height); //��������ͷ�ֱ���Ϊ1366*768����Ļ����һ��
	cap.set(CV_CAP_PROP_FRAME_WIDTH, width); 
	Mat frame;  // ������Ƶ֡����  
	Mat frameHSV;   // hsv�ռ�  
	Mat dst(frame); // ���ͼ��  
	vector< Mat > contours(500);   // ����  
	vector< Mat > filterContours(50); // ɸѡ�������
	vector< Vec4i > hierarchy(500);    // �����Ľṹ����
	char c;
	int HAND_LOCATION = 0;        // ��λ��
	int LAST_HAND_LOCATION = 0;   // ��һ֡��λ��
	int THIS_MOVE = 0;            // ���ƶ�
	int centerX = 0, centerY = 0; // �����ĵ�

	// ��������
	LPCSTR song1 = TEXT("open D:\\songs\\1.mp3 alias song1"); // ������ �򿪵�һ�׸�
	LPCSTR song2 = TEXT("open D:\\songs\\2.mp3 alias song2"); // ������ �򿪵�һ�׸�
	LPCSTR song3 = TEXT("open D:\\songs\\3.mp3 alias song3"); // ������ �򿪵�һ�׸�
	LPCSTR song4 = TEXT("open D:\\songs\\4.mp3 alias song4"); // ������ �򿪵�һ�׸�
	TCHAR cmd[256]; // ��������������
	TCHAR szVolume[100]; // ��������������
	TCHAR volume[100]; // ���صĵ�ǰ����������
	TCHAR return_Info[256];
	int v = 500; // ��ǰ������intֵ
	int n = 1; // Ŀǰ���ŵĸ�id
	bool PLAY = false; // ���ţ�
	
	// ����ʼ����
	mciSendString(song1, NULL, 0, NULL);  // �򿪸���1
	mciSendString(song2, NULL, 0, NULL);  // �򿪸���2
	mciSendString(song3, NULL, 0, NULL);  // �򿪸���3
	mciSendString(song4, NULL, 0, NULL);  // �򿪸���4
	wsprintf(cmd, "play song%d repeat", n);
	mciSendString(cmd, NULL, 0, NULL); // ���ŵ�һ�׸�
	PLAY = true;
	while (true) // ѭ��Ԥ��������
	{
		cap >> frame;

		if (frame.empty())
		{
			cout << " < < <  Game over!  > > > ";
			break;
		}
		

		flip(frame, frame, 1); // ��תͼ��

		// ��ֵ�˲���ȥ����������  
		medianBlur(frame, frame, 5);
		cvtColor(frame, frameHSV, CV_BGR2HSV); // ת����HSV�ռ�
		Mat dstTemp1(frame.rows, frame.cols, CV_8UC1);
		Mat dstTemp2(frame.rows, frame.cols, CV_8UC1);
		// ��HSV�ռ�����������õ�2ֵͼ�����Ĳ���Ϊ�ֵ���״  
		inRange(frameHSV, Scalar(0, 30, 30), Scalar(40, 170, 256), dstTemp1);
		inRange(frameHSV, Scalar(156, 30, 30), Scalar(180, 170, 256), dstTemp2);
		bitwise_or(dstTemp1, dstTemp2, dst);

		// ��̬ѧ������ȥ����������ʹ�ֵı߽��������  
		Mat element = getStructuringElement(MORPH_RECT, Size(3, 3));
		erode(dst, dst, element);
		morphologyEx(dst, dst, MORPH_OPEN, element);
		dilate(dst, dst, element);
		morphologyEx(dst, dst, MORPH_CLOSE, element);
		
		contours.clear();
		hierarchy.clear();
		filterContours.clear();
		// �õ��ֵ�����  
		findContours(dst, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
		
		Rect max; // ����һ�����ο�������Χ�ֲ�
		max.height = 0;
		max.width = 0;
		for (size_t i = 0; i < contours.size(); i++)
		{
			if (fabs(contourArea(Mat(contours[i]))) > 15000) // �ж��ֽ����������ֵ
			{
				filterContours.push_back(contours[i]);
				Rect rect = boundingRect(contours[i]);
				if (rect.height > max.height) {
					max = rect; // ȫ�ַ����ֵ����
				}
			}
		}
		cout << "numbers = " << filterContours.size() << "   "; // ���з����������������
		
		rectangle(frame, Point(max.x, max.y), Point(max.x + max.width, max.y + max.height), 
								cvScalar(97, 105, 224), 3);  // ������
		centerX = max.x + max.width / 2;
		centerY = max.y + max.height / 2;
		cout << "center in ( " << centerX << ", " << centerY << ")" << "  "; // �������λ��

		// ����������λ�ý����жϣ�����Ļ��1-9��λ��
		if (centerX < width/3 && centerY < height/3) HAND_LOCATION = 1;
		else if (centerX >= width / 3 && centerX < width * 2 / 3 && centerY <  height / 3) HAND_LOCATION = 2;
		else if (centerX >= width * 2 / 3 && centerY <  height / 3) HAND_LOCATION = 3;
		else if (centerX < width / 3 && centerY <  height*2 / 3 && centerY >= height / 3) HAND_LOCATION = 4;
		else if (centerX >= width / 3 && centerX < width * 2 / 3 && centerY >= height / 3 && centerY < height * 2 / 3) HAND_LOCATION = 5;
		else if (centerX >= width * 2 / 3 && centerY >= height / 3 && centerY < height * 2 / 3) HAND_LOCATION = 6;
		else if (centerX < width / 3 && centerY >= height * 2 / 3) HAND_LOCATION = 7;
		else if (centerX >= width / 3 && centerX < width * 2 / 3 && centerY >= 512) HAND_LOCATION = 8;
		else if (centerX >= width * 2 / 3 && centerY >= height * 2 / 3) HAND_LOCATION = 9;
		cout << HAND_LOCATION << endl;

		// ��ǰ����֡�����жϣ��õ����λ�ã�Ȼ������THIS_MOVE��ֵ
		if (LAST_HAND_LOCATION == 5) {
			if (HAND_LOCATION == 2) {
				cout << "UP!!!!!!!!!!!!" << endl;
				THIS_MOVE = 1;
			}
			else if (HAND_LOCATION == 4) {
				cout << "LEFT!!!!!!!!!!!!" << endl;
				THIS_MOVE = 3;
			}
			else if (HAND_LOCATION == 6) {
				cout << "RIGHT!!!!!!!!!!!!" << endl;
				THIS_MOVE = 4;
			}
			else if (HAND_LOCATION == 8) {
				cout << "DOWN!!!!!!!!!!!!!" << endl;
				THIS_MOVE = 2;
			}
			else {
				THIS_MOVE = 0;
			}
		}
		else THIS_MOVE = 0;

		// ����THIS_MOVE�����Ӧ�Ķ���
		switch (THIS_MOVE)
		{
		case 1:
			wsprintf(cmd, "status song%d volume", n);
			mciSendString(cmd, volume, sizeof(volume), NULL); //v�ǻ�ȡ��������Сֵ��
			v = atoi(volume);
			cout << "��ǰ����Ϊ" << v ;
			if (v+100 >= 1000) v = 1000; else v += 200;
			wsprintf(szVolume, "setaudio song%d volume to %d", n, v); // ��������200
			mciSendString(szVolume, 0, 0, 0); //v�����õ�����ֵ
			cout << "   ��������Ϊ" << v << endl;
			break;
		case 2:
			wsprintf(cmd, "status song%d volume", n);
			mciSendString(cmd, volume, sizeof(volume), NULL); //v�ǻ�ȡ��������Сֵ��
			v = atoi(volume);
			cout << "��ǰ����Ϊ" << v;
			if (v-100 <= 0) v = 0; else v -= 200;
			wsprintf(szVolume, "setaudio song%d volume to %d", n, v); // ������С200
			mciSendString(szVolume, 0, 0, 0); //v�����õ�����ֵ
			cout << "   ��������Ϊ" << v  << endl;
			break;
		case 3:
			
			if (PLAY) {
				wsprintf(cmd, "pause song%d", n);
				mciSendString(cmd, NULL, 0, NULL);
				PLAY = false;
				cout << "PAUSED!" << endl;
			}
			else {
				wsprintf(cmd, "resume song%d", n);
				mciSendString(cmd, NULL, 0, NULL);
				PLAY = true;
				cout << "PLAYIN!" << endl;
			}

			break;
		case 4:
			wsprintf(cmd, "stop song%d", n);
			mciSendString(cmd, NULL, 0, NULL);
			cout << "STOPPED!" << endl;
			PLAY = false;
			n = n % 4 + 1;
			wsprintf(cmd, "play song%d repeat", n);
			mciSendString(cmd, NULL, 0, NULL);
			PLAY = true;
			cout << "NEXT SONG is "<<n<<" !" << endl;

			break;
		default:
			break;
		}

		LAST_HAND_LOCATION = HAND_LOCATION;

		
		rectangle(frame, Point(0,height/3), Point(width,height*2/3),
			cvScalar(255, 255, 0), 2);  // ������
		rectangle(frame, Point(width/3,0), Point(width*2/3,height),
			cvScalar(255, 255, 0), 2);  // ������
		//rectangle(frame, Point(911,256), Point(1366,512),
		//	cvScalar(255, 255, 0), 2);  // ������
		//rectangle(frame, Point(455,512), Point(911,768),
		//	cvScalar(255, 255, 0), 2);  // ������
		
		
		
		imshow("result", frame); // ��ʾʵʱԤ��ͼ��
		dst.release();
		frame.release();
		frameHSV.release();
		dstTemp1.release();
		dstTemp2.release();
		element.release();
		
		c = waitKey(1);
		if (c == 27)
			break;
	}
	cap.release();
	mciSendString(TEXT("close song1"), NULL, 0, NULL);
	mciSendString(TEXT("close song2"), NULL, 0, NULL);
	mciSendString(TEXT("close song3"), NULL, 0, NULL);
	mciSendString(TEXT("close song4"), NULL, 0, NULL);
	return 0;
}
