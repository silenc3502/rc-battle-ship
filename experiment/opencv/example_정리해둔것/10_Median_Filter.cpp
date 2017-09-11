#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
 
using namespace std;
using namespace cv;
 
int main( int argc, char** argv )
 {
 
     Mat src = imread( "result.jpg", 1 );
     Mat dst;
 
     //Apply median filter
     medianBlur ( src, dst, 15 );
     imshow("source", src);
     imshow("result", dst);  
 
     waitKey(0);
     return 0;
 }

/*
Applying Median Filter

void medianBlur(InputArray src, OutputArray dst, int ksize)

Parameters:
src – input 1-, 3-, or 4-channel image; when ksize is 3 or 5, the image depth should be CV_8U, CV_16U, or CV_32F, for larger aperture sizes, it can only be CV_8U.
dst – destination array of the same size and type as src.
ksize – aperture linear size; it must be odd and greater than 1, for example: 3, 5, 7 ...

You can find a nice example in OpenCV Documentation.  

*/
