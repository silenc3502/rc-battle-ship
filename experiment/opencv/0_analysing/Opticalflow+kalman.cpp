#include<opencv2/opencv.hpp>
#include<iostream>
#include<vector>
#include<stdio.h>
#include<opencv2/imgproc.hpp>

using namespace cv;
using namespace std;

int main(int argc, const char** argv)
{
	int CAM_ID = 0;
	VideoCapture cap(CAM_ID);
	namedWindow("CAM_Window",0);

	Mat flow, frame;

	UMat flowUmat, prevgray;

	for(;;)
	{
		bool Is = cap.grab();
		if(Is == false){
		cout << "Camera Capture Fail" << endl;
		break;
		}
	else{

	Mat img;
	Mat original;

	cap.retrieve(img, CV_CAP_OPENNI_BGR_IMAGE);
	resize(img,img,Size(640,480));

	img.copyTo(original);

	cvtColor(img, img, COLOR_BGR2GRAY);


	if(prevgray.empty() == false){

	//calcOpticalFlowFarneback(prevgray,img,flowUmat, 0.4,1,12,2,8,1.2,0);
	calcOpticalFlowFarneback(prevgray,img,flowUmat, 0.1,0.1,10,1,3,1.2,0);
//출력할결과,이전프레임영상,flowUMat, 0.4,1,인식감지하는범위?, 1 움직이는거리(라인이 따라가는 거리), 1.2, 다음프레임 감지범위,0 원본

	flowUmat.copyTo(flow);


	for(int y = 0; y< original.rows; y += 2){

	for (int x = 0; x < original.cols; x += 2)
	{
              // get the flow from y, x position * 10 for better visibility
              const Point2f flowatxy = flow.at<Point2f>(y, x) * 1;
                     // draw line at flow direction

/*
        GaussianBlur(img, original, Size(7,7),0,0);
        threshold( img, original, 128, 255, ADAPTIVE_THRESH_GAUSSIAN_C | CV_THRESH_BINARY | CV_THRESH_OTSU);

*/

       line(original, Point(x, y), Point(cvRound(x + flowatxy.x), cvRound(y + flowatxy.y)), Scalar(0,150,0));
                                                         // draw initial point
 //      circle(original, Point(x, y), 1, Scalar(0, 0, 0), -1);

      }

     }
   
                                 // draw the results
    
    imshow("CAM_Window", original);
    
                                 // fill previous image again
    img.copyTo(prevgray);

   }
   else {

                                 // fill previous image in case prevgray.empty() == true
    img.copyTo(prevgray);
   
   }
   

   int key1 = waitKey(20);

  }
 }
}

