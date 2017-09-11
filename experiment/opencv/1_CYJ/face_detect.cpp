#include <opencv2/opencv.hpp>
//#include "opencv.hpp"
#include <opencv2/highgui.hpp>
#include <stdio.h>

using namespace std;
using namespace cv;

int main(int argc, char *argv[])
{
	int CAM_ID = 0;
	VideoCapture cap(CAM_ID);
	//      printf("cap = 0x%x\n", cap);
	//      printf("cap.isOpened() = %d\n", cap.isOpened());
	if(!cap.isOpened())
	{
		cerr << "Can't Open Camera" << endl;
		return -1;
	}

	namedWindow("Face", 1);

	//cascadeclassifier 클래스 

	CascadeClassifier face_classifier;

	//얼굴 인식 xml 로딩 

	face_classifier.load("/home/koitt02/opencv_src/opencv/data/haarcascades/haarcascade_frontalface_default.xml");


	while(1){
		bool frame_valid = true;

		Mat frame_original;
		Mat frame;


		try{
			//카메라로부터 이미지 얻어오기 
			cap >> frame_original;
		}catch(Exception& e){
			cerr << "Execption occurred." << endl;
			frame_valid = false;
		}

	if(frame_valid){
		try{
			Mat grayframe;

			//gray scale로 변환 
			cvtColor(frame_original, grayframe, CV_BGR2GRAY);
			//histogram 얻기 
			face_classifier.load("/home/koitt02/opencv_src/opencv/data/haarcascades/haarcascade_frontalface_default.xml");

			while(1){
				bool frame_valid = true;

				Mat frame_original;
				Mat frame;

				try{
					//카메라로부터 이미지 얻어오기 
					cap >> frame_original;
				}catch(Exception& e){
					cerr << "Execption occurred." << endl;
					frame_valid = false;
				}

				if(frame_valid){
					try{
						Mat grayframe;

						//gray scale로 변환 
						cvtColor(frame_original, grayframe, CV_BGR2GRAY);
						//histogram 얻기 
						equalizeHist(grayframe, grayframe);

						//이미지 표시용 변수 
						vector<Rect> faces;

						//얼굴의 위치와 영역을 탐색한다. 
						face_classifier.detectMultiScale(grayframe, faces,
										1.1,
										3,
										0,      //CV_HAAR_FIND_BIGGEST_OBJECT | CV_HAAR_SCALE_IMAGE, 
										Size(30, 30));

						for(int i=0;i<faces.size();i++){
							Point lb(faces[i].x + faces[i].width,
							faces[i].y + faces[i].height);
							Point tr(faces[i].x, faces[i].y);

						//rectangle(Mat& img, Point pt1, Point pt2, const Scalar& color, int thickness=1, int lineType=8, int shift=0) 
						//img 적용할 이미지 
						//pt1 그릴 상자의 꼭지점 
						//pt2 pt1의 반대편 꼭지점 
						//color 상자의 색상 
						//thickness 상자의 라인들의 두께 음수 또는 CV_FILLED를 주면 상자를 채운다. 
						//lineType 라인의 모양 line()함수 확인하기 
						//shift ?? Number of fractional bits in the point coordinates. 
						//포인트 좌표의 분수 비트의 수?? 
						rectangle(frame_original, lb, tr, Scalar(0, 255, 0), 3, 4, 0); 
					}

					//윈도우에 이미지 그리기 
					imshow("Face", frame_original);
				}catch(Exception& e){
					cerr << "Exception occurred. face" << endl;
				}
				//키 입력 대기 
				if(waitKey(30) == 0)
					break;
				}
			}
			return 0;
		}

		//g++ -o face face_detect.cpp `pkg-config opencv --libs` 컴파일코드
	}
}
