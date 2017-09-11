#include <iostream>
#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

#define PI 3.141592

using namespace cv;
using namespace std;

int main()
{
  VideoCapture cap("driving6.ogv");
  Mat frame, dst, res;

  if(!cap.isOpened())
  {
    cout << "Could not open the input video.." << endl ;
    return -1;
  }
// frame 의 가로, 세로 정보를 가져온다
    double Width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
    double Height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

// framesize 설정 - videowriter 인자값으로 넘겨주기 위해 이러한 형태로 만듬
    Size framesize(static_cast<int>(Width), static_cast<int>(Height));

// initializing videowrite class
    VideoWriter oVideoWriter("video.avi" , CV_FOURCC('P','I','M','1'), 20, framesize, true);

  while(1)
  {
    cap.read(frame);
    if(frame.empty())
    {
        cerr << "ERROR! blank frame grabbed\n" ;
        break;
    }
// 그레이스케일로 변환
    cvtColor(frame, dst, COLOR_BGR2GRAY);

// 가우시안 블러링 적용 - 커널사이즈 5,5
    GaussianBlur(dst, dst, Size(5, 5), 0);

// Canny 엣지 적용
    Canny(dst, dst, 50, 200);

// 빈 Mat 만듬 (8비트 1채널)
  Mat dstImage(dst.rows, dst.cols, CV_8UC1, Scalar(0,0,0));


//pts1[4] 꼭지점4개짜리 다각형 만들기 {왼쪽바닥점, 왼쪽윗점, 오른쪽윗점, 오른쪽바닥점}
//두번째,세번째 값은, 후에 x 좌표를-> 소실점-50, 소실점+50 해주도록 한다
  Point pts1[6] = {Point(0, dstImage.rows-100), Point(100, dstImage.rows/2+70), Point(dstImage.cols/2-40, 140), Point(dstImage.cols/2+40, 140),  Point(dstImage.cols-100, dstImage.rows/2+70), Point(dstImage.cols, dstImage.rows-100)};
//pts2[3] 꼭지점3개짜리 더미 다각형(가짜)
  Point pts2[3] = {Point(0,0), Point(0, 1), Point(1, 0)};

//fillPoly 를 사용하기 위한 상수 배열 선언
  const Point *polygon[2] = {pts1, pts2};

//fillPoly 를 사용하기 위한 꼭지점갯수 배열 선언
  int npts[2] = {6,3};

//fillPoly(입력영상, 마스크영상(배열명), 꼭지점갯수(배열명), n개의 다각형 만들어라, 스칼라값 )
  fillPoly(dstImage, polygon, npts, 2, Scalar(255, 255, 255));

//bitwise_and : 두 영상 비교하여 1일경우만 거르기 - (미리만들어놓은 Mat) res 에 저장하기
  bitwise_and(dst, dstImage, res);

  vector<Vec4i> lines;

// 입력영상, rho, theta, 180, 선의최소길이(최소길이 이상의 선만 검출), 선위의 점들사이 최대>거리(점 사이의 거리가 이 값보다 크면, 나와는 다른선으로 간주한다)
  HoughLinesP(res, lines, 1, CV_PI/180, 30, 30, 30 );

// 허프선 변환을 통해 그려지는 선의 양끝점 프린트하기
  Vec4i params;
  int x1, y1, x2, y2, slope_degree;
  for(int k = 0; k < lines.size(); k++ ){
    params = lines[k];
    x1 = params[0];
    y1 = params[1];
    x2 = params[2];
    y2 = params[3];
    slope_degree = atan2(y1-y2, x1-x2)*180 / PI;

    printf("lines[%2d] = P1(%4d, %4d) = P2(%4d, %4d) , slope_degree:%3d \n", k, x1, y1, x2, y2, slope_degree);
  }

  for( size_t i = 0; i < lines.size(); i++ )
  {
    Vec4i l = lines[i];
    line( frame, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
  }

   imshow("res", res);
   imshow("frame", frame);

   // 녹화
   oVideoWriter.write(frame);

   if(waitKey(10)>= 0) break;
   }

   cap.release();
   destroyAllWindows();

   return 0;
}

