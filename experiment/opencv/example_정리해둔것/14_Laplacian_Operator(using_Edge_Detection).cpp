#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdlib.h>
#include <stdio.h>
 
using namespace cv;
 
int main( int argc, char** argv )
{
    Mat src, gray, dst, abs_dst;
    src = imread( "lena.jpg" );
 
    /// Remove noise by blurring with a Gaussian filter
    GaussianBlur( src, src, Size(3,3), 0, 0, BORDER_DEFAULT );
    cvtColor( src, gray, CV_RGB2GRAY );
 
    /// Apply Laplace function
    Laplacian( gray, dst, CV_16S, 3, 1, 0, BORDER_DEFAULT );
    convertScaleAbs( dst, abs_dst );
    imshow( "result", abs_dst );
 
    waitKey(0);
    return 0;
}

/*
Edge detection using Laplacian operator

void Laplacian(InputArray src, OutputArray dst, int ddepth, int ksize=1, double scale=1, double delta=0, int borderType=BORDER_DEFAULT )

Parameters:
src – Source image.
dst – Destination image of the same size and the same number of channels as src .
ddepth – Desired depth of the destination image.
ksize – Aperture size used to compute the second-derivative filters. See getDerivKernels() for details. The size must be positive and odd.
scale – Optional scale factor for the computed Laplacian values. By default, no scaling is applied. See getDerivKernels() for details.
delta – Optional delta value that is added to the results prior to storing them in dst .
borderType – Pixel extrapolation method. See borderInterpolate() for details.

You can find an example in OpenCV Documentation.

Steps:
1. Load image
2. Remove noise by blurring with a Gaussian filter
3. Convert to gray-scale.
4. Apply the Laplacian operator to find the edges.(Laplacian)
5. The resulting image needs to be converted to 8-bit image for display.(convertScaleAbs)
6. Show the result

*/
