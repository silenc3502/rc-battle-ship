/*
 * barcode-roi: barcode detection using computuer vision and image processing
 * techniques
 *
 * Copyright (c) 2016 Adrian Rosebrock [PyImageSearch.com]
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

//opencv
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
#include <opencv2/core/ocl.hpp>
//C
#include <stdio.h>
#include <time.h>
#include <unistd.h>
//C++
#include<iostream>
#include<sstream>
#include<vector>
#include<algorithm>
#include<cstdio>

using namespace std;
using namespace cv;

int main ( int argc, char **argv )
{
    int *dbg_res;
    Mat im_gray, im_rgb;
    Mat tmpGrad;
    UMat img_gray;
    UMat img_ocl, blurredImg, threshImg, closedImg;
    UMat abs_gradient, gradX, gradY, gradient;
    UMat img_final, img_output;
    vector<vector<Point> > contours;
    float maxContArea;
    int   maxContIdx;
    VideoCapture cap;
    bool live_camera_input = false;
    bool live_display      = false;
    bool ocl_acc_flag      = false;
    int  frames_to_process = 1;
    char *outputFilename   = NULL;
    
    if(argc < 2) {
      printf ("\nUsage %s: input [1|static_image_filename] opencl_acc [0|1] live_display [1|fileOutput]\n", argv[0]);
      printf ("Live camera input, NO opencl, live display output: %s 1 0 1\n", argv[0]);
      printf ("Live camera input, opencl ON, live display output: %s 1 1 1\n", argv[0]);
      printf ("Static image input, opencl ON, file output: %s sample_barcode.jpg 1 image_det.png\n", argv[0]);
      exit (0);
    }

    if(atoi(argv[1]) == 1) {
      live_camera_input = true;
      if(!cap.open(1)) // open the default camera
        return -1;
      double fps = cap.get(CV_CAP_PROP_FPS); //get the frames per seconds of the video
      cout << "Frame per seconds : " << fps << endl;
      frames_to_process = 30000; // Maximum number of frames to process
    } else {
      live_camera_input = false;
    }

    ocl_acc_flag = atoi(argv[2]);
    if(argc > 3) {
      if(atoi(argv[3]) == 1) live_display = true;
      else outputFilename = argv[3];
    }

    if(live_display) {
      namedWindow( "BarcodeROI", WINDOW_AUTOSIZE );// Create a window for display.
    }

    for (int jj = 0; jj < frames_to_process; jj ++)
    {
      if(live_camera_input) {
        cap >> im_rgb;
      } else {
        im_rgb = imread(argv[1]);
      }
      if(im_rgb.empty()) {
         cout << "Wrong input (" << argv[1] << ")" << endl;
         break;
      }

      ocl::setUseOpenCL(ocl_acc_flag);
      cvtColor(im_rgb,im_gray,CV_RGB2GRAY);

      im_gray.copyTo(img_gray);

      Sobel( img_gray, gradX, CV_16S, 1, 0, -1, 1, 0, BORDER_DEFAULT );
      Sobel( img_gray, gradY, CV_16S, 0, 1, -1, 1, 0, BORDER_DEFAULT );
      subtract(gradX, gradY, gradient);
      convertScaleAbs(gradient, abs_gradient);
      // blur and threshold the image
      //GaussianBlur( abs_gradient, blurredImg, Size(7,7), 0, 0, BORDER_DEFAULT );
      GaussianBlur( abs_gradient, blurredImg, Size(3,3), 0, 0, BORDER_DEFAULT );
      threshold(blurredImg, threshImg, 225, 255, THRESH_BINARY);

      Mat elementKernel = getStructuringElement( MORPH_RECT, Size( 2*10+1, 2*3+1 ), Point(10, 3));
      ocl::setUseOpenCL(false);
      morphologyEx( threshImg, closedImg, MORPH_CLOSE, elementKernel );
      ocl::setUseOpenCL(ocl_acc_flag);

      erode(closedImg, img_final, UMat(), Point(-1, -1), 4);
      dilate(img_final, img_ocl, UMat(), Point(-1, -1), 4);
      /***********************************************************************************/
      ocl::setUseOpenCL(false);
      findContours(img_ocl, contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE);

      maxContArea = 0;
      maxContIdx  = -1;
      for(unsigned int i = 0; i < contours.size(); i++)
      {
        float currContArea = contourArea(contours[i]);
        if(currContArea > maxContArea) {
          maxContArea = currContArea;
          maxContIdx  = i;
        }
      }
      //compute the rotated bounding box of the largest contour
      RotatedRect rect = minAreaRect( Mat(contours[maxContIdx]) );
      Point2f rvertices[4];

      rect.points(rvertices);
      for (int i = 0; i < 4; i++)
        line(im_rgb, rvertices[i], rvertices[(i+1)%4], Scalar(0,255,0), 3);

      /* Put text string to indicate the detection */
      char show_status[160];
      Point2f textPt;
      snprintf(show_status, 160, "barcode detected in green box");
      textPt.x = 2048;
      textPt.y = 2048;
      for (int i = 0; i < 4; i++)
      {
         if (rvertices[i].x < textPt.x)
         {
           textPt.x = rvertices[i].x;
         }
         if (rvertices[i].y < textPt.y)
         {
           textPt.y = rvertices[i].y;
         }
       }
       textPt.y = textPt.y - 20;
       putText(im_rgb, show_status, textPt, FONT_HERSHEY_COMPLEX_SMALL, 1, cvScalar(0, 255,0), 1, CV_AA);

      if(live_display) {
        imshow( "BarcodeROI", im_rgb );   // Show our image inside it.
        if(!live_camera_input) {
          cout << "Wait for input camera stroke" << endl;
          waitKey(0);                     // Wait for keystroke
        } else waitKey(10);
      } else {
        if(outputFilename) {
          imwrite(outputFilename, im_rgb);
        }
        break;
      }
   }
   if(live_camera_input) {
     cap.release();
   }
   return 0;
}

