/* 
void cvtColor(InputArray src, OutputArray dst, int code, int dstCn=0 )

Parameters:
src – input image: 8-bit unsigned, 16-bit unsigned ( CV_16UC... ), or single-precision floating-point.
dst – output image of the same size and depth as src.
code – color space conversion code (see the description below).
dstCn – number of channels in the destination image; if the parameter is 0, the number of the channels is derived automatically from src and code .

Note that the default color format in OpenCV is often referred to as RGB but it is actually BGR (the bytes are reversed).

The conventional ranges for R, G, and B channel values are:
- 0 to 255 for CV_8U images
- 0 to 65535 for CV_16U images
- 0 to 1 for CV_32F images

Steps:
1. Load an image 
2. Convert to gray scale
3. Show result
*/

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "iostream"
 
using namespace cv;
using namespace std;
 
int main( )
{
 
       Mat image;
       image = imread("lena.jpg", CV_LOAD_IMAGE_COLOR);  
 
       if(! image.data )                             
       {
              cout <<  "Could not open or find the image" << std::endl ;
              return -1;
       }
 
       // Create a new matrix to hold the gray image
       Mat gray;
 
       // convert RGB image to gray
       cvtColor(image, gray, CV_BGR2GRAY);
 
       namedWindow( "Display window", CV_WINDOW_AUTOSIZE );  
       imshow( "Display window", image );                 
 
       namedWindow( "Result window", CV_WINDOW_AUTOSIZE );   
       imshow( "Result window", gray );
 
       waitKey(0);                                          
       return 0;
}
