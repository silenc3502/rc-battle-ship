#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>
#include <stdlib.h>

using namespace cv;
using namespace std;

void on_low_r_thresh_trackbar(int, void *);
void on_high_r_thresh_trackbar(int, void *);
void on_low_g_thresh_trackbar(int, void *);
void on_high_g_thresh_trackbar(int, void *);
void on_low_b_thresh_trackbar(int, void *);
void on_high_b_thresh_trackbar(int, void *);

int low_r=0, low_g=0, low_b=5;
int high_r=1, high_g=1, high_b=255;

int main()
{
  VideoCapture cap("driving4.mp4");
  Mat frame, dst, smoothed, cdst, HSV;

  namedWindow("Video Capture", WINDOW_NORMAL);
  namedWindow("Object Detection", WINDOW_NORMAL);

    createTrackbar("Low R","Object Detection", &low_r, 255, on_low_r_thresh_trackbar);
    createTrackbar("High R","Object Detection", &high_r, 255, on_high_r_thresh_trackbar);
    createTrackbar("Low G","Object Detection", &low_g, 255, on_low_g_thresh_trackbar);
    createTrackbar("High G","Object Detection", &high_g, 255, on_high_g_thresh_trackbar);
    createTrackbar("Low B","Object Detection", &low_b, 255, on_low_b_thresh_trackbar);
    createTrackbar("High B","Object Detection", &high_b, 255, on_high_b_thresh_trackbar);

  if(!cap.isOpened() )
  {
    cout << "Could not open the input video.." << endl ;
    return -1;
  }

  while(1)
  {
  cap >> frame;
  if(frame.empty()) break;

  cvtColor(frame, dst, CV_BGR2HSV);

  GaussianBlur(dst, smoothed, Size(3, 3), 0);

  Canny(smoothed, cdst, 70, 210);

  inRange(cdst, Scalar(low_b,low_g,low_r), Scalar(high_b,high_g,high_r), HSV);
/*
  vector<Vec4i> lines;
  HoughLinesP(HSV, lines, 1, CV_PI/180, 200, 30, 20 );
  for( size_t i = 0; i < lines.size(); i++ )
  {
    Vec4i l = lines[i];
    line( HSV, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255, 255, 255), 3, CV_AA);
  }
*/
  imshow("Video Capture", frame);
  imshow("Object Detection", HSV);
//  imshow("Object Detection", smoothed);

  if(waitKey(10) >= 0) break;
  }

  cap.release();
  destroyAllWindows(); // It's not necessary !

  return 0;
}

/** @function on_low_r_thresh_trackbar */
void on_low_r_thresh_trackbar(int, void *)
{
    low_r = min(high_r-1, low_r);
    setTrackbarPos("Low R","Object Detection", low_r);
}
//! [low]
//! [high]
/** @function on_high_r_thresh_trackbar */
void on_high_r_thresh_trackbar(int, void *)
{
    high_r = max(high_r, low_r+1);
    setTrackbarPos("High R", "Object Detection", high_r);
}
//![high]
/** @function on_low_g_thresh_trackbar */
void on_low_g_thresh_trackbar(int, void *)
{
    low_g = min(high_g-1, low_g);
    setTrackbarPos("Low G","Object Detection", low_g);
}

/** @function on_high_g_thresh_trackbar */
void on_high_g_thresh_trackbar(int, void *)
{
    high_g = max(high_g, low_g+1);
    setTrackbarPos("High G", "Object Detection", high_g);
}

/** @function on_low_b_thresh_trackbar */
void on_low_b_thresh_trackbar(int, void *)
{
    low_b= min(high_b-1, low_b);
    setTrackbarPos("Low B","Object Detection", low_b);
}

/** @function on_high_b_thresh_trackbar */
void on_high_b_thresh_trackbar(int, void *)
{
    high_b = max(high_b, low_b+1);
    setTrackbarPos("High B", "Object Detection", high_b);
}

