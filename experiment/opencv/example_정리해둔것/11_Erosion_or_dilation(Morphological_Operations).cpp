#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
 
using namespace cv;
using namespace std;
 
int main( )
{
 
       Mat image,dst;
       image = imread("result.jpg", CV_LOAD_IMAGE_COLOR);  
 
       // Create a structuring element
       int erosion_size = 6;  
       Mat element = getStructuringElement(cv::MORPH_CROSS,
              cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1),
              cv::Point(erosion_size, erosion_size) );
 
       // Apply erosion or dilation on the image
       erode(image,dst,element);  // dilate(image,dst,element);
       
       namedWindow( "Display window", CV_WINDOW_AUTOSIZE );  
       imshow( "Display window", image );                 
 
       namedWindow( "Result window", CV_WINDOW_AUTOSIZE );   
       imshow( "Result window", dst );
 
       waitKey(0);                                         
       return 0;
}

/*
Erosion or dilation (Morphological operations)

void erode(InputArray src, OutputArray dst, InputArray kernel, Point anchor=Point(-1,-1), int iterations=1, int borderType=BORDER_CONSTANT, const Scalar& borderValue=morphologyDefaultBorderValue() )

Parameters:
src – input image; the number of channels can be arbitrary, but the depth should be one of CV_8U, CV_16U, CV_16S, CV_32F` or ``CV_64F.
dst – output image of the same size and type as src.
element – structuring element used for erosion; if element=Mat() , a 3 x 3 rectangular structuring element is used.
anchor – position of the anchor within the element; default value (-1, -1) means that the anchor is at the element center.
iterations – number of times erosion is applied.
borderType – pixel extrapolation method (see borderInterpolate() for details).
borderValue – border value in case of a constant border (see createMorphologyFilter() for details).
void dilate(InputArray src, OutputArray dst, InputArray kernel, Point anchor=Point(-1,-1), int iterations=1, int borderType=BORDER_CONSTANT, const Scalar& borderValue=morphologyDefaultBorderValue() )

Parameters: Same as erode(...)

Steps:
1. Load an image 
2. Create a structuring element
3. Apply erosion or dilation on the image
4. Show result
*/
