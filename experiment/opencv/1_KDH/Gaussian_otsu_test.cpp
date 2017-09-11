#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <stdio.h>

using namespace std;
using namespace cv;


/*
void onmouse(int event, int x, int y, int flags, void* param);
void changeQuantisationGrey(Mat &image, int num_bits);

void onmouse(int event, int x, int y, int flags, void* param)
{
//좌클릭할 때 (CV_EVENT_LBUTTONDOWN 부분) , 그 위치에서 픽셀값과 위치를 출력
        if(event == CV_EVENT_LBUTTONDOWN) 
        {
                Mat *img = (Mat*)(param);
                cout <<"Mouse clicked" << (int)(*img).at<uchar>(y,x) << endl;
                cout <<"Mouse clicked" << x << "," << y << endl;
        }
//점찍으려면..
	if(event == cv_EVENT_LBUTTONDOWN){
		//원그리기 : 파라미터 ,  좌표  , 반지름,    빨간점   , 선의굵기
		//           파라미터 는 Mat*으로 강제캐스팅 한 
		circle(*(Mat*)param , point(x,y) , 2 , cv_RGB(255,0,0) , 3 );
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
	Mat gray_image; //그레이체널 Mat 생성을 위한 변수선언
	Mat bitable;
	VideoCapture cap(CAM_ID); //카메라 열기
	if(!cap.isOpened()){ //카메라 열리지 않았다면, 아래 프린트문 실행하고 종료
		printf("Can't open the CAM(%d)\n",CAM_ID);
		return -1;
	}
	//이미지 해상도 얻기 <- cap.size(width,height); 로 못받아오낭..? 찾아봐야함
	double width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	double height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	printf("size = [%f, %f]\n", width, height);


	//윈도우 생성 및 사이즈 변경
	namedWindow("CAM_Window", 0);

//	nameWindow("Second_CAM", 0);

	resizeWindow("CAM_Window", 1080, 720);
	
//	resizeWindow("Second_CAM" , 1080, 720);

//	마우스 이벤트 수행을 위한 콜백함수 사용 부분 : 시스템이 이 함수를 호출해주고(ioctl일까..?)
//							        -> 값을 넣어준다
//	      마우스이벤트받을창이름,부를함수명,onmouse함수의 파라미터중-void* param 으로 넘겨줄값
//	setMouseCallback("CAM_Window",onmouse, &gray_image);


	while(1){


	cap >> frame; //카메라에서 이미지 얻어오기
	

	
	cvtColor(frame, gray_image, CV_BGR2GRAY);  //컬러화면을 흑백(BGR순)으로 바꿔줌
//	changeQuantisationGrey(gray_image, 3);

//	가우시안블러.. 선명한 화질을 뿌옇게 보이게 흐리는 작업
	GaussianBlur(gray_image , bitable ,Size(7,7),0,0);
//	이진화	   입력영상,출력영상,thresh,max_value, 적용할 알고리즘들 
	threshold( bitable , bitable, 128, 255, ADAPTIVE_THRESH_GAUSSIAN_C | CV_THRESH_BINARY | CV_THRESH_OTSU );       

	imshow("CAM_Window", bitable); //이진화까지 적용된 이미지를 윈도우에 표시
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
