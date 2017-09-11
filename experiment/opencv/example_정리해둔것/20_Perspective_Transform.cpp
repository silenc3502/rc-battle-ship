#include<opencv2/opencv.hpp>
 
using namespace cv;
 
int main( )
{
    // Input Quadilateral or Image plane coordinates
    Point2f inputQuad[4]; 
    // Output Quadilateral or World plane coordinates
    Point2f outputQuad[4];
         
    // Lambda Matrix
    Mat lambda( 2, 4, CV_32FC1 );
    //Input and Output Image;
    Mat input, output;
     
    //Load the image
    input = imread( "lena.jpg", 1 );
    // Set the lambda matrix the same type and size as input
    lambda = Mat::zeros( input.rows, input.cols, input.type() );
 
    // The 4 points that select quadilateral on the input , from top-left in clockwise order
    // These four pts are the sides of the rect box used as input 
    inputQuad[0] = Point2f( -30,-60 );
    inputQuad[1] = Point2f( input.cols+50,-50);
    inputQuad[2] = Point2f( input.cols+100,input.rows+50);
    inputQuad[3] = Point2f( -50,input.rows+50  );  
    // The 4 points where the mapping is to be done , from top-left in clockwise order
    outputQuad[0] = Point2f( 0,0 );
    outputQuad[1] = Point2f( input.cols-1,0);
    outputQuad[2] = Point2f( input.cols-1,input.rows-1);
    outputQuad[3] = Point2f( 0,input.rows-1  );
 
    // Get the Perspective Transform Matrix i.e. lambda 
    lambda = getPerspectiveTransform( inputQuad, outputQuad );
    // Apply the Perspective Transform just found to the src image
    warpPerspective(input,output,lambda,output.size() );
 
    //Display input and output
    imshow("Input",input);
    imshow("Output",output);
 
    waitKey(0);
    return 0;
}

/*
Perspective Transform

Mat getPerspectiveTransform(InputArray src, InputArray dst)
- Calculates a perspective transform from four pairs of the corresponding points.

void warpPerspective(InputArray src, OutputArray dst, InputArray M, Size dsize, int flags=INTER_LINEAR, int borderMode=BORDER_CONSTANT, const Scalar& borderValue=Scalar())
- Applies a perspective transformation to an image.

Parameters:
src – input image.
dst – output image that has the size dsize and the same type as src .
M – 3\times 3 transformation matrix.
dsize – size of the output image.
flags – combination of interpolation methods (INTER_LINEAR or INTER_NEAREST) and the optional flag WARP_INVERSE_MAP, that sets M as the inverse transformation ( \texttt{dst}\rightarrow\texttt{src} ).
borderMode – pixel extrapolation method (BORDER_CONSTANT or BORDER_REPLICATE).
borderValue – value used in case of a constant border; by default, it equals 0.
*/
