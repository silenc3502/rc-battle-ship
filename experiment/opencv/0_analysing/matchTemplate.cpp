#include <opencv2/opencv.hpp>
using namespace cv;
using namespace std;
int main()
{
    Mat srcImage = imread("china_sample4.jpg", 0);
    if( srcImage.empty() )
        return -1;
    Mat tImage1 = imread("China.png", 0);
 
    Mat dstImage1, dstImage2, dstImage3, dstImage4;
    cvtColor(srcImage, dstImage1, COLOR_GRAY2BGR );
    cvtColor(srcImage, dstImage2, COLOR_GRAY2BGR );
    cvtColor(srcImage, dstImage3, COLOR_GRAY2BGR );
    cvtColor(srcImage, dstImage4, COLOR_GRAY2BGR );
 
     double minVal, maxVal;
    Point minLoc, maxLoc;
    Mat result, fin_result;


    // 제곱차 매칭 방법(TM_SQDIFF)
    matchTemplate(srcImage, tImage1, result, TM_SQDIFF);
    minMaxLoc(result, &minVal, NULL, &minLoc, NULL);
    rectangle(dstImage1,minLoc,
        Point(minLoc.x+ tImage1.cols,minLoc.y+tImage1.rows), Scalar(255, 0, 0), 2); 

 
    // 정규화된 제곱차 매칭 방법(TM_SQDIFF_NORMED)
    matchTemplate(srcImage, tImage1, result, TM_SQDIFF_NORMED);
    minMaxLoc(result, &minVal, NULL, &minLoc, NULL);
    rectangle(dstImage2, minLoc,
        Point(minLoc.x+ tImage1.cols,minLoc.y+tImage1.rows), Scalar(0, 255, 0), 2); 
 

    // 정규화된 상관관계 방법(TM_CCORR_NORMED)
    matchTemplate(srcImage, tImage1, result, TM_CCORR_NORMED);
    minMaxLoc(result, NULL, &maxVal, NULL, &maxLoc);
    rectangle(dstImage3, maxLoc,
        Point(maxLoc.x+ tImage1.cols,maxLoc.y+tImage1.rows), Scalar(0, 0, 255), 2); 

 
    // 정규화된 상관계수 방법(TM_CCOEFF_NORMED)
    matchTemplate(srcImage, tImage1, result, TM_CCOEFF_NORMED);
    minMaxLoc(result, NULL, &maxVal, NULL, &maxLoc);
    rectangle(dstImage4, maxLoc,
        Point(maxLoc.x+ tImage1.cols,maxLoc.y+tImage1.rows), Scalar(255, 0, 255), 2); 

    imshow("dstImage1", dstImage1);
    imshow("dstImage2", dstImage2);
    imshow("dstImage3", dstImage3);
    imshow("dstImage4", dstImage4);

    waitKey();
    return 0;
}
