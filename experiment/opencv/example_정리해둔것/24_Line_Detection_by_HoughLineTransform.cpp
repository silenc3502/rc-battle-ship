#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
 
using namespace cv;
using namespace std;
 
int main()
{
    Mat src = imread("building.jpg", 0);
 
    Mat dst, cdst;
    Canny(src, dst, 50, 200, 3); 
    cvtColor(dst, cdst, CV_GRAY2BGR); 
 
    vector<Vec2f> lines;
    // detect lines
    HoughLines(dst, lines, 1, CV_PI/180, 150, 0, 0 );
 
    // draw lines
    for( size_t i = 0; i < lines.size(); i++ )
    {
        float rho = lines[i][0], theta = lines[i][1];
        Point pt1, pt2;
        double a = cos(theta), b = sin(theta);
        double x0 = a*rho, y0 = b*rho;
        pt1.x = cvRound(x0 + 1000*(-b));
        pt1.y = cvRound(y0 + 1000*(a));
        pt2.x = cvRound(x0 - 1000*(-b));
        pt2.y = cvRound(y0 - 1000*(a));
        line( cdst, pt1, pt2, Scalar(0,0,255), 3, CV_AA);
    }
 
    imshow("source", src);
    imshow("detected lines", cdst);
 
    waitKey();
    return 0;
}

/*

Line Detection by Hough Line Transform

void HoughLines(InputArray image, OutputArray lines, double rho, double theta, int threshold, double srn=0, double stn=0 )

Parameters:
image – 8-bit, single-channel binary source image (use edge detectors)
        8비트 단일채널 이진입력영상 image에서 직선을 검출한다
lines – Output vector of lines. Each line is represented by a two-element vector  (\rho, \theta) .  \rho is the distance from the coordinate origin  (0,0) (top-left corner of the image).  \theta is the line rotation angle in radians ( 0 \sim \textrm{vertical line}, \pi/2 \sim \textrm{horizontal line} ).
        CV_32FC2 자료형의 행렬로 검출된 직선의 (rho, theta) 가 저장된다. 
        rho : 영상왼쪽위(top-left) 즉 원점으로 하는 거리,   theta : 회전각도
rho – Distance resolution of the accumulator in pixels.
      원점으로부터 거리의 간격
theta – Angle resolution of the accumulator in radians.
        x축과의 각도(라디안간격)
threshold – Accumulator threshold parameter. Only those lines are returned that get enough votes (>threshold ).
            직선을 검출하기 위한 어큐물레이터(Accumulator)의 임계값.
srn – For the multi-scale Hough transform, it is a divisor for the distance resolution rho . The coarse accumulator distance resolution is rho and the accurate accumulator resolution is rho/srn . If both srn=0 and stn=0 , the classical Hough transform is used. Otherwise, both these parameters should be positive.
      다중 스케일 Hough변환에서 rho/srn 의 어큐물레이터(Accumulator)의 간격
stn – For the multi-scale Hough transform, it is a divisor for the distance resolution theta.
A good example for Hough Line Transform is provided in OpenCV Documentation.
      다중 스케일 Hough변환에서 theta/stn의 어큐물레이터 간격
      만약, srm=0, stn=0 이면 표준 Hough변환이 사용되고, 
      그렇지 않으면 coarse-fine 의 다중스케일방식으로 직선을 검출한다
      처음에는 rho, theta를 이용하여 거친(coarse) 스케일로 직선을 검출하고, 더욱 자세한(fine) 스케일에서 rho/srn, theta/stn 의 정밀도로 계산한다.

Steps:
1. Load image and convert to gray-scale.
2. Apply the Hough Transform to find the lines.(HoughLines)
3. Draw the detected lines.(line)
4. Show the result

*/
