/* Drawing a Filled Polygon
void fillPoly(Mat& img, const Point** pts, const int* npts, int ncontours, const Scalar& color, int lineType=8, int shift=0, Point offset=Point() )

Parameters:
img – Image.
pts – Array of polygons where each polygon is represented as an array of points.
npts – Array of polygon vertex counters.
ncontours – Number of contours that bind the filled region.
color – Polygon color.
lineType – Type of the polygon boundaries. See the line() description.
shift – Number of fractional bits in the vertex coordinates.
offset – Optional offset of all points of the contours.

The function fillPoly fills an area bounded by several polygonal contours. The function can fill complex areas, for example, areas with holes, contours with self-intersections (some of their parts), and so forth.
*/

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

using namespace cv;
 
int main( )
{    
  // Create black empty images
  Mat image = Mat::zeros( 400, 400, CV_8UC3 );
   
  int w=400;
  // Draw a circle 
  /** Create some points */
  Point rook_points[1][20];
  rook_points[0][0] = Point( w/4.0, 7*w/8.0 );
  rook_points[0][1] = Point( 3*w/4.0, 7*w/8.0 );
  rook_points[0][2] = Point( 3*w/4.0, 13*w/16.0 );
  rook_points[0][3] = Point( 11*w/16.0, 13*w/16.0 );
  rook_points[0][4] = Point( 19*w/32.0, 3*w/8.0 );
  rook_points[0][5] = Point( 3*w/4.0, 3*w/8.0 );
  rook_points[0][6] = Point( 3*w/4.0, w/8.0 );
  rook_points[0][7] = Point( 26*w/40.0, w/8.0 );
  rook_points[0][8] = Point( 26*w/40.0, w/4.0 );
  rook_points[0][9] = Point( 22*w/40.0, w/4.0 );
  rook_points[0][10] = Point( 22*w/40.0, w/8.0 );
  rook_points[0][11] = Point( 18*w/40.0, w/8.0 );
  rook_points[0][12] = Point( 18*w/40.0, w/4.0 );
  rook_points[0][13] = Point( 14*w/40.0, w/4.0 );
  rook_points[0][14] = Point( 14*w/40.0, w/8.0 );
  rook_points[0][15] = Point( w/4.0, w/8.0 );
  rook_points[0][16] = Point( w/4.0, 3*w/8.0 );
  rook_points[0][17] = Point( 13*w/32.0, 3*w/8.0 );
  rook_points[0][18] = Point( 5*w/16.0, 13*w/16.0 );
  rook_points[0][19] = Point( w/4.0, 13*w/16.0) ;
 
  const Point* ppt[1] = { rook_points[0] };
  int npt[] = { 20 };
 
  fillPoly( image, ppt, npt, 1, Scalar( 255, 255, 255 ) );
  imshow("Image",image);
 
  waitKey( 0 );
  return(0);
}
