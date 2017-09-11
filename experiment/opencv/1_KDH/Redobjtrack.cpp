#include <iostream>  
#include <opencv2/core/mat.hpp>  
#include <opencv2/imgcodecs.hpp>  
#include <opencv2/imgproc.hpp>  
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
 
 
using namespace cv;
using namespace std;
 
 
Mat objectHistogram;
Mat globalHistogram;
 
void getObjectHistogram(Mat &frame, Rect object_region) 
{
    const int channels[] = { 0, 1 };
    const int histSize[] = { 64, 64 };
    float range[] = { 0, 256 };
    const float *ranges[] = { range, range };
 
    // Histogram in object region
//오브젝트 영역의 히스토그램
    Mat objectROI = frame(object_region);
    calcHist(&objectROI, 1, channels, noArray(), objectHistogram, 2, histSize, ranges, true, false);
    
    
    // A priori color distribution with cumulative histogram
//누적 히스토그램을 사용한 선명한 색상 분포
    calcHist(&frame, 1, channels, noArray(), globalHistogram, 2, histSize, ranges, true, true);
    
    
    // Boosting: Divide conditional probabilities in object area by a priori probabilities of colors

//Boosting : 오브젝트 영역의 조건부 확률을 이전의 색상 확률로 나눕니다.

    for (int y = 0; y < objectHistogram.rows; y++) {
        for (int x = 0; x < objectHistogram.cols; x++) {
            objectHistogram.at<float>(y, x) /= globalHistogram.at<float>(y, x);
        }
    }
    normalize(objectHistogram, objectHistogram, 0, 255, NORM_MINMAX);
}
void backProjection(const Mat &frame, const Mat &histogram, Mat &bp) {
    const int channels[] = { 0, 1 };
    float range[] = { 0, 256 };
    const float *ranges[] = { range, range };
    calcBackProject(&frame, 1, channels, objectHistogram, bp, ranges);
}
 
 
 
int main(  )
{
    VideoCapture cap(0);
    
    //웹캡에서 캡처되는 이미지 크기를 320x240으로 지정  
    cap.set(CAP_PROP_FRAME_WIDTH,320);  
    cap.set(CAP_PROP_FRAME_HEIGHT,240);
    
    if ( !cap.isOpened() )
    {
        cout << "웹캠을 열 수 없습니다." << endl;
        return -1;
    }
 
    
    namedWindow("찾을 색범위 설정", CV_WINDOW_AUTOSIZE); 
 
    //트랙바에서 사용되는 변수 초기화 
     int LowH = 170;
     int HighH = 179;
 
      int LowS = 50; 
     int HighS = 255;
 
      int LowV = 0;
     int HighV = 255;
 
    
    //트랙바 생성 
     cvCreateTrackbar("LowH", "찾을 색범위 설정", &LowH, 179); //Hue (0 - 179)
     cvCreateTrackbar("HighH", "찾을 색범위 설정", &HighH, 179);
 
      cvCreateTrackbar("LowS", "찾을 색범위 설정", &LowS, 255); //Saturation (0 - 255)
     cvCreateTrackbar("HighS", "찾을 색범위 설정", &HighS, 255);
 
      cvCreateTrackbar("LowV", "찾을 색범위 설정", &LowV, 255); //Value (0 - 255)
     cvCreateTrackbar("HighV", "찾을 색범위 설정", &HighV, 255);
 
 
    Rect trackingWindow(0, 0, 30, 30);
  
    int i = 0;
    
 
 
    Rect prev_rect;
    Mat bp;
    
    
      while (true)
     {
        i++;
        
        //웹캠에서 캡처되는 속도 출력  
        int fps = cap.get(CAP_PROP_FPS);  
        cout<<fps<<" "<<i<<endl;  
    
        Mat img_input, img_hsv, img_binary;
        
 
        //카메라로부터 이미지를 가져옴 
        bool ret = cap.read(img_input); 
 
        if (!ret) 
        {
            cout << "카메라로부터 이미지를 가져올 수 없습니다." << endl;
            break;
        }
 
     
        //HSV로 변환
        cvtColor(img_input, img_hsv, COLOR_BGR2HSV); 
        
  
  
            //지정한 HSV 범위를 이용하여 영상을 이진화
        inRange(img_hsv, Scalar(LowH, LowS, LowV), Scalar(HighH, HighS, HighV), img_binary); 
            
 
        //morphological opening 작은 점들을 제거 
        erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
        dilate( img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
 
 
        //morphological closing 영역의 구멍 메우기 
        dilate( img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
        erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
                  
 
        
        if ( i < 100 )
        {
 
            //라벨링 
            Mat img_labels,stats, centroids;  
            int numOfLables = connectedComponentsWithStats(img_binary, img_labels,   
                                                    stats, centroids, 8,CV_32S);  
 
 
            //영역박스 그리기
            int max = -1, idx=0; 
            for (int j = 1; j < numOfLables; j++) {
                int area = stats.at<int>(j, CC_STAT_AREA);
                if ( max < area ) 
                {
                    max = area;
                    idx = j;
                }
            }
                
                
            int left = stats.at<int>(idx, CC_STAT_LEFT);  
            int top  = stats.at<int>(idx, CC_STAT_TOP);  
            int width = stats.at<int>(idx, CC_STAT_WIDTH);  
            int height  = stats.at<int>(idx, CC_STAT_HEIGHT);  
 
 
            rectangle( img_input, Point(left,top), Point(left+width,top+height),  
                        Scalar(0,0,255),1 );  
            
            Rect object_region( left, top, width, height);
            
            getObjectHistogram(img_hsv, object_region);
            prev_rect = object_region;
        }
        else{                      
 
            backProjection(img_hsv, objectHistogram, bp);
            bitwise_and(bp, img_binary, bp);
            RotatedRect rect = CamShift(bp, prev_rect, cvTermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 20, 1));
 
            ellipse(img_input, rect, CV_RGB(255,0,0), 3, CV_AA);
        
        
        }
        
        imshow("이진화 영상", img_binary); 
        imshow("원본 영상", img_input); 
 
 
        //ESC키 누르면 프로그램 종료
        if (waitKey(1) == 27) 
            break; 
    }
 
    
       return 0;
}
