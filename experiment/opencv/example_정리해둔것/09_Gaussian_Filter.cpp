#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
 
using namespace std;
using namespace cv;
 
int main( int argc, char** argv )
 {
   
     Mat src = imread( "result.jpg", 1 );
     Mat dst;
 
     //Apply median filter
     GaussianBlur( src, dst, Size( 5, 5 ), 0, 0 );
     imshow("source", src);
     imshow("result", dst);  
 
     waitKey(0);
     return 0;
 }

/*
Applying Gaussian Filter

void GaussianBlur(InputArray src, OutputArray dst, Size ksize, double sigmaX, double sigmaY=0, int borderType=BORDER_DEFAULT )

Parameters:
src – input image; the image can have any number of channels, which are processed independently, but the depth should be CV_8U, CV_16U, CV_16S, CV_32F or CV_64F.
dst – output image of the same size and type as src.
ksize – Gaussian kernel size. ksize.width and ksize.height can differ but they both must be positive and odd. Or, they can be zero’s and then they are computed from sigma* .
sigmaX – Gaussian kernel standard deviation in X direction.
sigmaY – Gaussian kernel standard deviation in Y direction; if sigmaY is zero, it is set to be equal to sigmaX, if both sigmas are zeros, they are computed from ksize.width and ksize.height , respectively (see getGaussianKernel() for details); to fully control the result regardless of possible future modifications of all this semantics, it is recommended to specify all of ksize, sigmaX, and sigmaY.
borderType – pixel extrapolation method (see borderInterpolate() for details).

You can find a nice example in OpenCV Documentation.  

*/

