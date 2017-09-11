#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <stdio.h>

using namespace std;
using namespace cv;
//using namespace cv::xfeatures2d;

/*
void onmouse(int event, int x, int y, int flags, void* param);
void changeQuantisationGrey(Mat &image, int num_bits);

void onmouse(int event, int x, int y, int flags, void* param)
{
//좌클릭할때 그위치에서 픽셀값과 위치를 출력
        if(event == CV_EVENT_LBUTTONDOWN)
        {
                Mat *img = (Mat*)(param);
                cout <<"Mouse clicked" << (int)(*img).at<uchar>(y,x) << endl;
                cout <<"Mouse clicked" << x << "," << y << endl;
        }
}
//256단계의 gray 이미지를 2^num_bits 단계로 바꾸는 함수
void changeQuantisationGrey(Mat &image, int num_bits)
{
        CV_Assert((image.type() == CV_8UC1) && (num_bits >= 1) && (num_bits <= 8));

        uchar mask = 0xFF << (8 - num_bits);

        for (int row = 0; row < image.rows; row++)
        {
                for(int col = 0; col < image.cols; col++)
                {
                        image.at<uchar>(row, col) = image.at<uchar>(row,col) & mask;
                }
        }
}
*/

int main(void)
{
	int CAM_ID = 0;
	Mat frame;
	Mat gray_image; //그레이체널 Mat 생성
	Mat bitable;  //이진화할 Mat 함수
	Mat Detect;
	
	VideoCapture cap(CAM_ID); //카메라 생성
	Mat tImage1 = imread("A.png");

/* Face Detect

CascadeClassifier face_cascade;
face_cascade.load( "/home/kang/kang/opencv_src/opencv/data/haarcascades/haarcascade_frontalface_alt2.xml" );

*/


	if(!cap.isOpened()){ //카메라 열렸는지 확인
		printf("Can't open the CAM(%d)\n",CAM_ID);
		return -1;
	}
	//이미지 해상도 얻기
	double width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	double height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	printf("size = [%f, %f]\n", width, height);


	//윈도우 생성 및 사이즈 변경
	namedWindow("CAM_Window", 0);

//	nameWindow("Second_CAM", 0);

	resizeWindow("CAM_Window", 1080, 720);
	
//	resizeWindow("Second_CAM" , 1080, 720);

//	setMouseCallback("CAM_Window",onmouse, &gray_image);



	while(1){


	cap >> frame; //카메라에서 이미지 얻어오기
	
	cvtColor(frame, gray_image, CV_BGR2GRAY);  //컬러화면을 흑백으로 바꿔줌
	cvtColor(gray_image, bitable, COLOR_GRAY2BGR);

	double minVal, maxVal;
	Point minLoc, maxLoc;
	Mat result;

	matchTemplate(frame, tImage1, result, TM_CCOEFF_NORMED);
	minMaxLoc(result , &minVal, NULL, &minLoc, NULL);
	rectangle(bitable, minLoc,
	Point(minLoc.x + tImage1.cols, minLoc.y + tImage1.rows), Scalar(255, 0, 255),2);
//	changeQuantisationGrey(gray_image, 3);

//	Canny(gray_image , bitable, 50, 100);

/* 2진화 출력

	GaussianBlur(gray_image , bitable ,Size(7,7),0,0);
	threshold( bitable , bitable, 128, 255, ADAPTIVE_THRESH_GAUSSIAN_C | CV_THRESH_BINARY | CV_THRESH_OTSU ); 

*/
/* 
    -face Detect hamsu

	vector<Rect> faces;

	face_cascade.detectMultiScale( bitable, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(140, 140) );
*/



/*	for( int i = 0; i < faces.size(); i++ )
   	 {
        Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
        ellipse( Detect, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0 );

}   ellipse detect
*/

/* Detect Rectangle
	for (unsigned i = 0; i<faces.size(); i++){
				rectangle(bitable,faces[i], Scalar(255, 0, 0), 2, 1);
	}
*/

	imshow("CAM_Window", bitable); //얻어온 이미지를 윈도우에 표시
//	imshow("Second_CAM", gray_image);
        char ch = waitKey(1);
		if(ch == 27)
		{
			cap.release();   //OpenCV는 종료를 release로 하더이다.
			break;
		}
	//destroyWindow("CAM_Window");
	}
	return 0;

	
}
