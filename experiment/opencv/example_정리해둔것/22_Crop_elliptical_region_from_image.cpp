#include <opencv2/highgui/highgui.hpp>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
 
 
using namespace cv;
using namespace std;
 
int main()
{
    Mat im1 = imread("NorthKorea1.jpg");
    if (im1.empty()) 
    {
        cout << "Cannot load image!" << endl;
        return -1;
    }
 cvtColor(im1, im1, CV_BGR2GRAY);
    imshow("im1", im1);  // Origina image
  
 Mat im2(im1.rows, im1.cols, CV_8UC1, Scalar(0,0,0));
 ellipse( im2, Point( 120, 130 ), Size( 50.0, 60.0 ), 0, 0, 360, Scalar( 255, 255, 255), -1, 8 );
 imshow("im2",im2);  // mask
  
 Mat res;
 bitwise_and(im1,im2,res);     
 imshow("AND",res);  // resultant image
 
    waitKey(0);
 return 0;
}

/*
Crop elliptical region from image

We know that we can represent image as a matrix, and matrix can not be elliptical. So when you want to crop a elliptical area from an image, then you have to extract the elliptical region and remove rest of the information outside the ellipse. This can be done by AND operation of the image with an elliptical mask.

Steps:
1. Read the image and convert to gray scale.
2. Create the elliptical mask. ( use Negative argument to fill the ellipse)
ellipse( im2, Point( 120, 130 ), Size( 50.0, 60.0 ), 0, 0, 360, Scalar( 255, 255, 255), -1, 8 );
3. Use AND operation.
4. Show the result.
*/
