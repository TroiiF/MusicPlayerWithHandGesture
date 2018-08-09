// MultiMediaPre.cpp : 定义控制台应用程序的入口点。
//
/*
	采用RGB->HSV空间，然后找出对应肤色的区域，判断前后两帧的位置差异，对屏幕的上下左右四个区域进行
监控，实现：
			手向上移动 -> 音量增高
			//手向下移动 -> 音量减小
			手向左移动 -> 播放/暂停
			手向右移动 -> 下一曲
	音频播放采用win32自带的api的mciSendString函数进行控制。
	一共有四首歌曲，需提前指定，不支持文件导入和文件夹导入功能（没时间做这么复杂了！）。
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
	// 视频处理参数
	VideoCapture cap(0);
	int height = 480;
	int width = 640;
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, height); //设置摄像头分辨率为1366*768与屏幕保持一致
	cap.set(CV_CAP_PROP_FRAME_WIDTH, width); 
	Mat frame;  // 输入视频帧序列  
	Mat frameHSV;   // hsv空间  
	Mat dst(frame); // 输出图像  
	vector< Mat > contours(500);   // 轮廓  
	vector< Mat > filterContours(50); // 筛选后的轮廓
	vector< Vec4i > hierarchy(500);    // 轮廓的结构数组
	char c;
	int HAND_LOCATION = 0;        // 手位置
	int LAST_HAND_LOCATION = 0;   // 上一帧手位置
	int THIS_MOVE = 0;            // 手移动
	int centerX = 0, centerY = 0; // 框中心点

	// 歌曲参数
	LPCSTR song1 = TEXT("open D:\\songs\\1.mp3 alias song1"); // 命令行 打开第一首歌
	LPCSTR song2 = TEXT("open D:\\songs\\2.mp3 alias song2"); // 命令行 打开第一首歌
	LPCSTR song3 = TEXT("open D:\\songs\\3.mp3 alias song3"); // 命令行 打开第一首歌
	LPCSTR song4 = TEXT("open D:\\songs\\4.mp3 alias song4"); // 命令行 打开第一首歌
	TCHAR cmd[256]; // 后续处理命令行
	TCHAR szVolume[100]; // 音量控制命令行
	TCHAR volume[100]; // 返回的当前音量的数组
	TCHAR return_Info[256];
	int v = 500; // 当前音量的int值
	int n = 1; // 目前播放的歌id
	bool PLAY = false; // 播放？
	
	// 程序开始部分
	mciSendString(song1, NULL, 0, NULL);  // 打开歌曲1
	mciSendString(song2, NULL, 0, NULL);  // 打开歌曲2
	mciSendString(song3, NULL, 0, NULL);  // 打开歌曲3
	mciSendString(song4, NULL, 0, NULL);  // 打开歌曲4
	wsprintf(cmd, "play song%d repeat", n);
	mciSendString(cmd, NULL, 0, NULL); // 播放第一首歌
	PLAY = true;
	while (true) // 循环预览处理结果
	{
		cap >> frame;

		if (frame.empty())
		{
			cout << " < < <  Game over!  > > > ";
			break;
		}
		

		flip(frame, frame, 1); // 翻转图像

		// 中值滤波，去除椒盐噪声  
		medianBlur(frame, frame, 5);
		cvtColor(frame, frameHSV, CV_BGR2HSV); // 转换到HSV空间
		Mat dstTemp1(frame.rows, frame.cols, CV_8UC1);
		Mat dstTemp2(frame.rows, frame.cols, CV_8UC1);
		// 对HSV空间进行量化，得到2值图像，亮的部分为手的形状  
		inRange(frameHSV, Scalar(0, 30, 30), Scalar(40, 170, 256), dstTemp1);
		inRange(frameHSV, Scalar(156, 30, 30), Scalar(180, 170, 256), dstTemp2);
		bitwise_or(dstTemp1, dstTemp2, dst);

		// 形态学操作，去除噪声，并使手的边界更加清晰  
		Mat element = getStructuringElement(MORPH_RECT, Size(3, 3));
		erode(dst, dst, element);
		morphologyEx(dst, dst, MORPH_OPEN, element);
		dilate(dst, dst, element);
		morphologyEx(dst, dst, MORPH_CLOSE, element);
		
		contours.clear();
		hierarchy.clear();
		filterContours.clear();
		// 得到手的轮廓  
		findContours(dst, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
		
		Rect max; // 定义一个矩形框用来包围手部
		max.height = 0;
		max.width = 0;
		for (size_t i = 0; i < contours.size(); i++)
		{
			if (fabs(contourArea(Mat(contours[i]))) > 15000) // 判断手进入区域的阈值
			{
				filterContours.push_back(contours[i]);
				Rect rect = boundingRect(contours[i]);
				if (rect.height > max.height) {
					max = rect; // 全局非最大值抑制
				}
			}
		}
		cout << "numbers = " << filterContours.size() << "   "; // 所有符合条件区域的数量
		
		rectangle(frame, Point(max.x, max.y), Point(max.x + max.width, max.y + max.height), 
								cvScalar(97, 105, 224), 3);  // 画矩形
		centerX = max.x + max.width / 2;
		centerY = max.y + max.height / 2;
		cout << "center in ( " << centerX << ", " << centerY << ")" << "  "; // 标记中心位置

		// 对中心所在位置进行判断，在屏幕的1-9号位置
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

		// 对前后两帧进行判断，得到相对位置，然后设置THIS_MOVE的值
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

		// 基于THIS_MOVE完成相应的动作
		switch (THIS_MOVE)
		{
		case 1:
			wsprintf(cmd, "status song%d volume", n);
			mciSendString(cmd, volume, sizeof(volume), NULL); //v是获取的音量大小值。
			v = atoi(volume);
			cout << "当前音量为" << v ;
			if (v+100 >= 1000) v = 1000; else v += 200;
			wsprintf(szVolume, "setaudio song%d volume to %d", n, v); // 音量增大200
			mciSendString(szVolume, 0, 0, 0); //v是设置的音量值
			cout << "   设置音量为" << v << endl;
			break;
		case 2:
			wsprintf(cmd, "status song%d volume", n);
			mciSendString(cmd, volume, sizeof(volume), NULL); //v是获取的音量大小值。
			v = atoi(volume);
			cout << "当前音量为" << v;
			if (v-100 <= 0) v = 0; else v -= 200;
			wsprintf(szVolume, "setaudio song%d volume to %d", n, v); // 音量减小200
			mciSendString(szVolume, 0, 0, 0); //v是设置的音量值
			cout << "   设置音量为" << v  << endl;
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
			cvScalar(255, 255, 0), 2);  // 画矩形
		rectangle(frame, Point(width/3,0), Point(width*2/3,height),
			cvScalar(255, 255, 0), 2);  // 画矩形
		//rectangle(frame, Point(911,256), Point(1366,512),
		//	cvScalar(255, 255, 0), 2);  // 画矩形
		//rectangle(frame, Point(455,512), Point(911,768),
		//	cvScalar(255, 255, 0), 2);  // 画矩形
		
		
		
		imshow("result", frame); // 显示实时预览图像
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
