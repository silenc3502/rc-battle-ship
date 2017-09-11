#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "iostream"
using namespace cv;
using namespace std;
int main( )
{
    Mat image;
    image = imread("result.jpg", 1);  
    namedWindow( "Display window", CV_WINDOW_AUTOSIZE );  
    imshow( "Display window", image );
    Mat gray;
    cvtColor(image, gray, CV_BGR2GRAY);
    Canny(gray, gray, 100, 200, 3);
    /// Find contours   
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    RNG rng(12345);
    findContours( gray, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
    /// Draw contours
    Mat drawing = Mat::zeros( gray.size(), CV_8UC3 );
    for( int i = 0; i< contours.size(); i++ )
    {
        Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
        drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
    }     
 
    imshow( "Result window", drawing );
    waitKey(0);                                         
    return 0;
}

/*
Find Largest contour

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp""
#include "opencv2/imgproc/imgproc.hpp"
#include "iostream"
using namespace cv;
using namespace std;
int main( )
{
    Mat src;
    src = imread("shape.jpg", CV_LOAD_IMAGE_COLOR);
    Mat gray;
    cvtColor(src, gray, CV_BGR2GRAY);
    threshold(gray, gray,200, 255,THRESH_BINARY_INV); //Threshold the gray
    imshow("gray",gray);int largest_area=0;
    int largest_contour_index=0;
    Rect bounding_rect;
    vector<vector<Point>> contours; // Vector for storing contour
    vector<Vec4i> hierarchy;
    findContours( gray, contours, hierarchy,CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );
    // iterate through each contour.
    for( int i = 0; i< contours.size(); i++ )
    {
        //  Find the area of contour
        double a=contourArea( contours[i],false); 
        if(a>largest_area){
            largest_area=a;cout<<i<<" area  "<<a<<endl;
            // Store the index of largest contour
            largest_contour_index=i;               
            // Find the bounding rectangle for biggest contour
            bounding_rect=boundingRect(contours[i]);
        }
    }
    Scalar color( 255,255,255);  // color of the contour in the
    //Draw the contour and rectangle
    drawContours( src, contours,largest_contour_index, color, CV_FILLED,8,hierarchy);
    rectangle(src, bounding_rect,  Scalar(0,255,0),2, 8,0);
    namedWindow( "Display window", CV_WINDOW_AUTOSIZE );
    imshow( "Display window", src );    
    waitKey(0);                                         
    return 0;
} 
*/

/*
Find Contour

void findContours(InputOutputArray image, OutputArrayOfArrays contours, OutputArray hierarchy, int mode, int method, Point offset=Point())

Parameters:
image – Source, an 8-bit single-channel image. Non-zero pixels are treated as 1’s. Zero pixels remain 0’s, so the image is treated as binary . You can use compare() , inRange() , threshold() , adaptiveThreshold() , Canny() , and others to create a binary image out of a grayscale or color one. The function modifies the image while extracting the contours.

contours – Detected contours. Each contour is stored as a vector of points.

hierarchy – Optional output vector, containing information about the image topology. It has as many elements as the number of contours. For each i-th contour contours[i] , the elements hierarchy[i][0] , hiearchy[i][1] , hiearchy[i][2] , and hiearchy[i][3] are set to 0-based indices in contours of the next and previous contours at the same hierarchical level, the first child contour and the parent contour, respectively. If for the contour i there are no next, previous, parent, or nested contours, the corresponding elements of hierarchy[i] will be negative.

mode – Contour retrieval mode (if you use Python see also a note below).
  - CV_RETR_EXTERNAL retrieves only the extreme outer contours. It sets hierarchy[i][2]=hierarchy[i][3]=-1 for all the contours.
  - CV_RETR_LIST retrieves all of the contours without establishing any hierarchical relationships.
  - CV_RETR_CCOMP retrieves all of the contours and organizes them into a two-level hierarchy. At the top level, there are external boundaries of the components. At the second level, there are boundaries of the holes. If there is another contour inside a hole of a connected component, it is still put at the top level.
  - CV_RETR_TREE retrieves all of the contours and reconstructs a full hierarchy of nested contours. This full hierarchy is built and shown in the OpenCV contours.c demo.

method – Contour approximation method (if you use Python see also a note below).
  - CV_CHAIN_APPROX_NONE stores absolutely all the contour points. That is, any 2 subsequent points (x1,y1) and (x2,y2) of the contour will be either horizontal, vertical or diagonal neighbors, that is, max(abs(x1-x2),abs(y2-y1))==1.
  - CV_CHAIN_APPROX_SIMPLE compresses horizontal, vertical, and diagonal segments and leaves only their end points. For example, an up-right rectangular contour is encoded with 4 points.
  - CV_CHAIN_APPROX_TC89_L1,CV_CHAIN_APPROX_TC89_KCOS applies one of the flavors of the Teh-Chin chain approximation algorithm. 

offset – Optional offset by which every contour point is shifted. This is useful if the contours are extracted from the image ROI and then they should be analyzed in the whole image context.

void drawContours(InputOutputArray image, InputArrayOfArrays contours, int contourIdx, const Scalar& color, int thickness=1, int lineType=8, InputArray hierarchy=noArray(), int maxLevel=INT_MAX, Point offset=Point() )

Parameters:
image – Destination image.
contours – All the input contours. Each contour is stored as a point vector.
contourIdx – Parameter indicating a contour to draw. If it is negative, all the contours are drawn.
color – Color of the contours.
thickness – Thickness of lines the contours are drawn with. If it is negative (for example, thickness=CV_FILLED ), the contour interiors are drawn.
lineType – Line connectivity. See line() for details.
hierarchy – Optional information about hierarchy. It is only needed if you want to draw only some of the contours (see maxLevel ).
maxLevel – Maximal level for drawn contours. If it is 0, only the specified contour is drawn. If it is 1, the function draws the contour(s) and all the nested contours. If it is 2, the function draws the contours, all the nested contours, all the nested-to-nested contours, and so on. This parameter is only taken into account when there is hierarchy available.
offset – Optional contour shift parameter. Shift all the drawn contours by the specified \texttt{offset}=(dx,dy) .
contour – Pointer to the first contour.
externalColor – Color of external contours.
holeColor – Color of internal contours (holes)

Steps:
1. Load an image 
2. Create a structuring element
3. Apply erosion or dilation on the image
4. Show result

*/
