#pragma warning(disable: 4819)

#include <iostream>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#define MIN_AREA_PIXELS 300
#define LINE_SPACING_PIXELS 30

using namespace std;
using namespace cv;

class CLine
{
  public:
    Point center;
    double rate;
    double y_inter;
    int left;
    int width;
    CLine(double rate, Point center, int left, int width)
    {
	this->rate = rate;
	this->center = center;
	this->left = left;
	this->width = width;
	y_inter = -rate*center.x + center.y;
    }
    ~CLine(){
    }
    double dist_to_point(Point p){
	double x1 = p.x;
	double y1 = p.y;
	double a = rate;
	double b = -1.0;
	double c = -rate*center.x + center.y;
	double d = fabs(a*x1 + b*y1 + c) / sqrt(a*a + b*b);
	return d;
    }
};

/* 구분된 영역의 edge를 이용해 해당 영역의 기울기를 찾아 하나의 직선을 생성하는 함수 */
int extractLine(CLine* lines, int num_labels, Mat src_img, Mat labelled_img, Mat stats, Mat centroids)
{
  int line_index = 0;

//Labelled image blob을 모두 체크 (i=0 인 경우는 배경이므로 제외)
  for(int i = 1; i<num_labels; i++)
  {

  //blob area 가 너무작으면 무시
    if(stats.at<int>(i, CC_STAT_AREA) < MIN_AREA_PIXELS) continue;

  //해당 Label 영역의 픽셀은 255, 나머지는 0으로 만든 새로운 이미지 Mat 생성
    Mat blob = Mat(src_img);
    cvtColor(blob, blob, COLOR_BGR2GRAY);

    for(int row = 0; row < blob.rows; row++)
    {
	int* label = labelled_img.ptr<int>(row);
	uchar* pixel = blob.ptr<uchar>(row);

	for(int col = 0; col < blob.cols; col++)
	{
	  if(label[col] == i) pixel[col] = 0;
	  else pixel[col] = 255;
	}
    }

  //해당 영역만 edge 추출
    Mat blob_edge;
    Canny(blob, blob_edge, 30, 90, 3);
    vector<Vec4i> blob_lines;

  //Houghtransform 후 line 조각 추출, line을 하나도 추출못했을 경우 이후를 무시하고 다음 blob으로 넘어간다
    HoughLinesP(blob_edge, blob_lines, 1, CV_PI/180, 30, 30, 3);

    if(blob_lines.size() == 0) continue;
  //Transform 후 추출된 line조각 중에 가장 긴 line 을 찾는다
    double longDistance = 0;
    int longDistanceIndex = 0;

    for(int j=0; j<blob_lines.size(); j++)
    {
      Vec4i L = blob_lines[i];
      double distance = (L[2] - L[0]) * (L[2] - L[0]) + (L[3] - L[1]) * (L[3] - L[1]);

      if(distance > longDistance && distance > LINE_SPACING_PIXELS * 2)
      {
	longDistance = distance;
	longDistanceIndex = j;
      }
    }
  //가장 긴 line조각의 기울기를 이용해 이미지 blob의 center를 기준으로 새로운 line 생성
    Vec4i L = blob_lines[longDistanceIndex];
    double rate = ((double)(L[3]-L[1])) / ((double)(L[2]-L[0])+0.5); //분모가 0이 되는 경우(무한대 기울기)를 막기 위해 분모에 0.5를 더해준다
    int unique_line_flag = 1;
  //이미 생성한 line과 중복되는(너무 가까운)line은 새로 생성하지 않는다
    for(int k=0; k<line_index; k++){
      if(lines[k].dist_to_point(Point(L[0],L[1])) < LINE_SPACING_PIXELS)
	unique_line_flag = 0;
    }

  //새로운 line 인 경우 생성하여 lines 메모리 영역에 저장
    if(unique_line_flag)
    {
      double centerx = centroids.at<double>(i, 0);
      double centery = centroids.at<double>(i, 1);
      int left = stats.at<int>(i, CC_STAT_LEFT);
      int width = stats.at<int>(i, CC_STAT_WIDTH); 
      lines[line_index] = CLine(rate, Point((int)centerx, (int)centery), left, width);
      line_index++;
    }
  }
//생성한 line 개수를 반환
  return line_index;
}

/* 직선들의 정보를 화면에 출력하는 함수 */
void displayLineinfo(Mat src_img, CLine* lines, int num_lines, Scalar linecolor, Scalar captioncolor)
{
  for(int i=0; i<num_lines; i++)
  {
    double x1, x2, y1, y2;
    x1 = lines[i].left;
    x2 = lines[i].left + lines[i].width;
    y1 = lines[i].rate * x1 + lines[i].y_inter;
    y2 = lines[i].rate * x2 + lines[i].y_inter;

    line(src_img, Point((int)x1, (int)y1), Point((int)x2, (int)y2), linecolor, 1, LINE_AA);
    char text[15];
    double tanval = -atan2(lines[i].rate, 1.0) / (double)CV_PI;

    if(tanval < 0) tanval = tanval + 1.0;
    sprintf(text, "%.2lfpi", tanval);
    putText(src_img, text, lines[i].center, 2, 0.5, captioncolor);
  }
}

int main()
{
  Mat src, src_hsv;
  Mat green;
  Mat green_edge;
  Mat red_lower;
  Mat red_upper;
  Mat red;

  VideoCapture video(0);
  if(!video.isOpened()){
    cout << "cannot read video \n";
    return -1;
  }
  namedWindow("Input");

/*
Hue값은 0~359이나 8bit 저장을 위해 반으로 나누어(0~179) 사용한다
Saturation 과 Value 값은 0~100 이나, 8bit 저장을 위해 2.55를 곱해(0~255) 사용한다
*/
// 녹색 분리를 위한 min max
  Scalar hsv_green_min = cvScalar(60, 55, 50);
  Scalar hsv_green_max = cvScalar(95,255,180);

//빨강 분리를 위한 min mas
  Scalar hsv_red_min_upper = cvScalar(150,200,60);
  Scalar hsv_red_max_upper = cvScalar(179,255,180);
  Scalar hsv_red_min_lower = cvScalar(0,200,60);
  Scalar hsv_red_max_lower = cvScalar(10,255,180);

//영상이 이어지는 동안 무한루프  
  while(video.isOpened())
  {
//영상에서 이미지 캡쳐
    video >> src;
    if(src.empty()) break;

//노이즈를 감안한 영상 blurring
    GaussianBlur(src, src, Size(3,3), 1.5, 1.5);

//색 구성을 BGR->HSV로 변환
    cvtColor(src, src_hsv, CV_BGR2HSV);

//녹색을 분리해 green에 저장
    inRange(src_hsv, hsv_green_min, hsv_green_max, green);

//빨강을 분리해 red에 저장
    inRange(src_hsv, hsv_red_min_lower, hsv_red_max_lower, red_lower);
    inRange(src_hsv, hsv_red_min_upper, hsv_red_max_upper, red_upper);
    addWeighted(red_lower, 1.0, red_upper, 1.0, 0.0, red);

//녹색, 빨강에 대해 각각 OPEN과 CLOSE를 수행하여 불완전하게 검출된 인접영역이 있다면 하나로 합한다
    morphologyEx(green, green, MORPH_OPEN, getStructuringElement(0, Size(3,3), Point(0,0)));
    morphologyEx(green, green, MORPH_CLOSE, getStructuringElement(0, Size(3,3), Point(0,0)));
    morphologyEx(red, red, MORPH_OPEN, getStructuringElement(0, Size(3,3), Point(0,0)));
    morphologyEx(red, red, MORPH_CLOSE, getStructuringElement(0, Size(3,3), Point(0,0)));

/* 빨강 영역 처리 */
//검출된 영역에서 큰 덩어리들을 Labeling
    Mat img_labels, stats, centroids;
    int numOfLabels_red = connectedComponentsWithStats(red, img_labels, stats, centroids, 8, CV_32S);

//Labelled 영역에서 직선을 추출해 저장하기 위한 메모리 할당 (최대 label 개수만큼)
    CLine* lines = (CLine *)malloc(sizeof(CLine)*numOfLabels_red);

//Labelled 영역에서 직선 추출
    int red_lines = extractLine(lines, numOfLabels_red, src, img_labels, stats, centroids);

//추출된 직선을 화면에 그리고, 그 옆에 기울기를 출력
    displayLineinfo(src, lines, red_lines, Scalar(0,0,255), Scalar(200,200,255));

//메모리 해제
    free(lines);

/* 녹색 영역 처리 */
    int numOfLabels_green = connectedComponentsWithStats(green, img_labels, stats, centroids, 8, CV_32S);
    lines = (CLine *)malloc(sizeof(CLine)*numOfLabels_red);
    int green_lines = extractLine(lines, numOfLabels_red, src, img_labels, stats, centroids);
    displayLineinfo(src, lines, green_lines, Scalar(0,255,0), Scalar(200,255,200));
    free(lines);

    imshow("Input", src);

    if(waitKey(10)>= 0) break;
  }

  cvDestroyAllWindows();
  return 0;
}

























