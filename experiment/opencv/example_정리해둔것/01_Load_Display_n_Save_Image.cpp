#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
 
using namespace cv;
using namespace std;
 
int main( )
{
 
       Mat image;
 
       // LOAD image
       image = imread("image1.jpg", CV_LOAD_IMAGE_COLOR);   // Read the file "image.jpg".
              //This file "image.jpg" should be in the project folder.
              //Else provide full address : "D:/images/image.jpg"
 
       if(! image.data )  // Check for invalid input
       {
              cout <<  "Could not open or find the image" << std::endl ;
              return -1;
       }
 
       //DISPLAY image
       namedWindow( "window", CV_WINDOW_AUTOSIZE ); // Create a window for display.
       imshow( "window", image ); // Show our image inside it.
 
       //SAVE image
       imwrite("result.jpg",image);// it will store the image in name "result.jpg"
 
       waitKey(0);                       // Wait for a keystroke in the window
       return 0;
}

/*
Load, Display and Save an image

Mat imread(const string& filename, intflags=1 )

Parameters:	
filename – Name of file to be loaded.
flags –Flags specifying the color type of a loaded image:
CV_LOAD_IMAGE_ANYDEPTH - return 16-bit/32-bit image when the input has the corresponding depth, otherwise convert it to 8-bit.
CV_LOAD_IMAGE_COLOR(>0) - If set, always convert image to the color one
CV_LOAD_IMAGE_GRAYSCALE (0)- If set, always convert image to the grayscale one
CV_LOAD_IMAGE_UNCHANGED (<0) loads the image as is (including the alpha channel if present)

bool imwrite(const string& filename, InputArray img, const vector<int>&params=vector<int>() )

Parameters:	
filename – Name of the file.
image – Image to be saved.
params – Format-specific save parameters encoded as pairs paramId_1, paramValue_1, paramId_2, paramValue_2, ... . The following parameters are currently supported:
For JPEG, it can be a quality (CV_IMWRITE_JPEG_QUALITY ) from 0 to 100 (the higher is the better). Default value is 95.
For PNG, it can be the compression level (CV_IMWRITE_PNG_COMPRESSION) from 0 to 9. A higher value means a smaller size and longer compression time. Default value is 3.
For PPM, PGM, or PBM, it can be a binary format flag (CV_IMWRITE_PXM_BINARY ), 0 or 1. Default value is 1.

void imshow(const string& winname, InputArray mat)

Parameters:
winname – Name of the window.
image – Image to be shown.
Steps:
Load image using imread().
Display image using namedWindow() and imshow().
Save the image using imwrite().
Wait for keyboard button press using waitKey().

*/
