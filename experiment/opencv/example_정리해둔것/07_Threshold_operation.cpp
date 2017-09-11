/*
double threshold(InputArray src, OutputArray dst, double thresh, double maxval, int type)

Applies a fixed-level threshold to each array element

Parameters:
src – input array (single-channel, 8-bit or 32-bit floating point).
dst – output array of the same size and type as src.
thresh – threshold value.
maxval – maximum value to use with the THRESH_BINARY and THRESH_BINARY_INV thresholding types.
type – thresholding type

THRESH_BINARY
\texttt{dst} (x,y) =  \fork{\texttt{maxval}}{if $\texttt{src}(x,y) > \texttt{thresh}$}{0}{otherwise}

THRESH_BINARY_INV
\texttt{dst} (x,y) =  \fork{0}{if $\texttt{src}(x,y) > \texttt{thresh}$}{\texttt{maxval}}{otherwise}

THRESH_TRUNC
\texttt{dst} (x,y) =  \fork{\texttt{threshold}}{if $\texttt{src}(x,y) > \texttt{thresh}$}{\texttt{src}(x,y)}{otherwise}

THRESH_TOZERO
\texttt{dst} (x,y) =  \fork{\texttt{src}(x,y)}{if $\texttt{src}(x,y) > \texttt{thresh}$}{0}{otherwise}

THRESH_TOZERO_INV
\texttt{dst} (x,y) =  \fork{0}{if $\texttt{src}(x,y) > \texttt{thresh}$}{\texttt{src}(x,y)}{otherwise}
Find an example in OpenCV documentaion.

Steps:
1. Load an image
2. Create a window to display results
3. Create Trackbar to choose type of Threshold
4. Call the function "Threshold_Demo" to perform threshold operation.
*/

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdlib.h>
#include <stdio.h>
 
using namespace cv;
 
int threshold_value = 0;
int threshold_type = 3;;
int const max_value = 255;
int const max_type = 4;
int const max_BINARY_value = 255;
 
Mat src, src_gray, dst;
char* window_name = "Threshold Demo";
 
char* trackbar_type = "Type: \n 0: Binary \n 1: Binary Inverted \n 2: Truncate \n 3: To Zero \n 4: To Zero Inverted";
char* trackbar_value = "Value";
 
void Threshold_Demo( int, void* );
 
int main( int argc, char** argv )
{
  /// Load an image
  src = imread( "result.jpg", 1 );
 
  /// Convert the image to Gray
  cvtColor( src, src_gray, CV_RGB2GRAY );
 
  /// Create a window to display results
  namedWindow( window_name, CV_WINDOW_AUTOSIZE );
 
  /// Create Trackbar to choose type of Threshold
  createTrackbar( trackbar_type,
                  window_name, &threshold_type,
                  max_type, Threshold_Demo );
 
  createTrackbar( trackbar_value,
                  window_name, &threshold_value,
                  max_value, Threshold_Demo );
 
  /// Call the function to initialize
  Threshold_Demo( 0, 0 );
 
  /// Wait until user finishes program
  while(true)
  {
    int c;
    c = waitKey( 20 );
    if( (char)c == 27 )
      { break; }
   }
}
 
 
void Threshold_Demo( int, void* )
{
  /* 0: Binary
     1: Binary Inverted
     2: Threshold Truncated
     3: Threshold to Zero
     4: Threshold to Zero Inverted
   */
 
  threshold( src_gray, dst, threshold_value, max_BINARY_value,threshold_type );
 
  imshow( window_name, dst );
}
