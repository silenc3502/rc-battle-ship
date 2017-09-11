#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
using namespace cv;
 
int main( )
{    
  // Create black empty images
  Mat image1 = Mat::zeros( 400, 400, CV_8UC3 );
  Mat image2 = Mat::zeros( 400, 400, CV_8UC3 );
  Mat image3 = Mat::zeros( 400, 400, CV_8UC3 );
  Mat image4 = Mat::zeros( 400, 400, CV_8UC3 );
  Mat image5 = Mat::zeros( 400, 400, CV_8UC3 );
   
  // Draw a line 
  line( image1, Point( 15, 20 ), Point( 70, 50), Scalar( 110, 220, 0 ),  2, 8 );
  imshow("Image1",image1);
 
  // Draw a circle 
  circle( image2, Point( 200, 200 ), 32.0, Scalar( 0, 0, 255 ), 1, 8 );
  imshow("Image2",image2);

  // Draw a ellipse 
  ellipse( image3, Point( 200, 200 ), Size( 100.0, 160.0 ), 45, 0, 360, Scalar( 255, 0, 0 ), 1, 8 );
  ellipse( image3, Point( 200, 200 ), Size( 100.0, 160.0 ), 135, 0, 360, Scalar( 255, 0, 0 ), 10, 8 );
  ellipse( image3, Point( 200, 200 ), Size( 150.0, 50.0 ), 135, 0, 360, Scalar( 0, 255, 0 ), 1, 8 );
  imshow("Image3",image3);

  // Draw a rectangle ( 5th argument is not -ve)
  rectangle( image4, Point( 15, 20 ), Point( 70, 50), Scalar( 0, 55, 255 ), +1, 4 );
  imshow("Image4",image4);
  // Draw a filled rectangle ( 5th argument is -ve)
  rectangle( image5, Point( 115, 120 ), Point( 170, 150), Scalar( 100, 155, 25 ), -1, 8 );
  imshow("Image5",image5);

  waitKey( 0 );
  return(0);
}

/* 
Drawing a line

void line(Mat& img, Point pt1, Point pt2, const Scalar& color, int thickness=1, int lineType=8, int shift=0)

 Parameters:
img – Image.
pt1 – First point of the line segment.
pt2 – Second point of the line segment.
color – Line color.
thickness – Line thickness.
lineType – Type of the line:
8 (or omitted) - 8-connected line.
4 - 4-connected line.
CV_AA - antialiased line.
shift – Number of fractional bits in the point coordinates.

===

Drawing a Circle

void circle(Mat& img, Point center, int radius, const Scalar& color, int thickness=1, int lineType=8, int shift=0)

Parameters:
img – Image where the circle is drawn.
center – Center of the circle.
radius – Radius of the circle.
color – Circle color.
thickness – Thickness of the circle outline, if positive. Negative thickness means that a filled circle is to be drawn.
lineType – Type of the circle boundary. See the line() description.
shift – Number of fractional bits in the coordinates of the center and in the radius value.

The function circle draws a simple or filled circle with a given center and radius.

===

Drawing an Ellipse

void ellipse(Mat& img, Point center, Size axes, double angle, double startAngle, double endAngle, const Scalar& color, int thickness=1, int lineType=8, int shift=0)

Parameters:
img – Image.
center – Center of the ellipse.
axes – Length of the ellipse axes.
angle – Ellipse rotation angle in degrees.
startAngle – Starting angle of the elliptic arc in degrees.
endAngle – Ending angle of the elliptic arc in degrees.
box – Alternative ellipse representation via RotatedRect or CvBox2D. This means that the function draws an ellipse inscribed in the rotated rectangle.
color – Ellipse color.
thickness – Thickness of the ellipse arc outline, if positive. Otherwise, this indicates that a filled ellipse sector is to be drawn.
lineType – Type of the ellipse boundary. See the line() description.
shift – Number of fractional bits in the coordinates of the center and values of axes.

The functions ellipse with less parameters draw an ellipse outline, a filled ellipse, an elliptic arc, or a filled ellipse sector. A piecewise-linear curve is used to approximate the elliptic arc boundary.

If you use the first variant of the function and want to draw the whole ellipse, not an arc, pass startAngle=0 and endAngle=360.

===

Drawing a Rectangle

void rectangle(Mat& img, Point pt1, Point pt2, const Scalar& color, int thickness=1, int lineType=8, int shift=0)

Parameters:
img – Image.
pt1 – Vertex of the rectangle.
pt2 – Vertex of the rectangle opposite to pt1 .
rec – Alternative specification of the drawn rectangle.
color – Rectangle color or brightness (grayscale image).
thickness – Thickness of lines that make up the rectangle. Negative values, like CV_FILLED , mean that the function has to draw a filled rectangle.
lineType – Type of the line. See the line() description.
shift – Number of fractional bits in the point coordinates.

*/
