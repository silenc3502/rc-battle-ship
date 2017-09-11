#include<opencv2/opencv.hpp>

using namespace cv;
using namespace std;

#define DIST_TH //threshold for histogram matching

Rect selection;
bool bLButtonDown = false;
typedef enum {INIT, CALC_HIST, TRACKING} STATUS;
STATUS trackingMode = INIT;

void onMouse(int mevent, int x, int y, int flags, void* param)
{
        static Point origin;
        Mat *pMat = (Mat*)param;
        Mat image = Mat(*pMat);
        if(bLButtonDown)
        {
                selection.x = MIN(x, origin.x);
                selection.y = MIN(y, origin.y); 
                selection.width = selection.x + abs(x - origin.x);
                selection.height = selection.y + abs(y - origin.y);

                selection.x = MAX(selection.x, 0);
                selection.y = MAX(selection.y, 0);
                selection.width = MIN(selection.width, image.cols);
                selection.height = MIN(selection.height, image.rows);
                selection.width -= selection.x;
                selection.height -= selection.y;
        }
        switch(mevent)
        {
                case EVENT_LBUTTONDOWN:
                        origin = Point(x,y);
                        selection = Rect(x,y,0,0);
                        bLButtonDown = true;
                        break;
                case EVENT_LBUTTONUP:
                        bLButtonDown = false;
                        if(selection.width > 0 && selection.height > 0)
                        trackingMode = CALC_HIST;
                        break;
        }
}

int main()
{
	VideoCapture cap(0);

	if(!cap.isOpened())
	{
		cout << "Could not open the input video..!" <<endl;
		return -1;
	}

	Size S = Size((int)cap.get(CAP_PROP_FRAME_WIDTH), (int)cap.get(CAP_PROP_FRAME_HEIGHT));
	int fps = (int)(cap.get(CAP_PROP_FPS));
	if(fps <= 0) fps = 24;

	Mat dstImage;
	namedWindow("dstImage", 0);
	resizeWindow("dstImage", 1080, 720);

	setMouseCallback("dstImage", onMouse, (void*)&dstImage);

        int histSize = 8;
        float valueRange[] = {0, 180};
        const float* ranges[] = {valueRange};
        int channels = 0;
        Mat hist, backProject;

	//히스토그램 출력용 선언
	Mat print_histogram;
	Mat histImage(512,512,CV_8U);
	
/***********************
  <TermCriteria>
  아래의 meanshift 함수의 3번째 인자를 정의한 부분임.
  meanShift의 iteration 회수를 10회 또는 C1_o 와 C1_r의 차가 2pt일 때까지 알고리즘을 구동하라는 의미임

  TermCriteria 는, 반복적인(iterative) 연산을 수행하는 알고리즘들의 종료기준들(termination criteria)을 정의한다. 디폴트생성자(constructor)로 초기화를 한 후, 파라미터들을 이용하여 기능을 재정의(override)하거나, 혹은 생성자가 초기화 할 때 원하는 기능을 정의할 수 있다.

  파라메터 :
  type - 종료기준에 대한 타입을 지정한다.
  maxCount - 최대 반복 횟수 혹은 구성요소(elements)의 제한횟수를 지정한다
  epsilon - 반복 알고리즘이 중지되는 매개변수의 원하는 정확도 또는 변경사항
  criteria - 더 이상 사용하지 않는(deprecated) CvTermCriteria 형식의 종료 기준 지정
***********************/
	TermCriteria criteria = TermCriteria(TermCriteria::COUNT + TermCriteria::EPS, 10, 2);

	Rect trackWindow;
	int delay = 1000 / fps;
	Mat frame, hImage, hsvImage, mask;

	// Kalman Filter
        Point2f ptPredicted;
        Point2f ptEstimated;
        Point2f ptMeasured;

        //state vector x = [x_k, y_k, vx_k, vy_k]^T
        KalmanFilter KF(4,2,0);
        Mat measurement(2,1, CV_32F);

        float dt = 1.0;

        //Transition matrix A describes model parameters at k-1 and k
        const float A[] = {1,0,dt,0,
                           0,1,0,dt,
                           0,0,1,0,
                           0,0,0,1};

        memcpy(KF.transitionMatrix.data, A, sizeof(A));
        cout << "KF.transitionMatrix=" << KF.transitionMatrix << endl;

        //Initialize Kalman parameters
        double Q = 1e-5;
        double R = 0.0001;
        const float H[] = {1,0,0,0,
                           0,1,0,0};
        memcpy(KF.measurementMatrix.data, H, sizeof(H));
        cout << "KF.measurementMatrix=" << KF.measurementMatrix << endl;

        setIdentity(KF.processNoiseCov, Scalar::all(Q));
        KF.processNoiseCov.at<float>(2,2) = 0;
        KF.processNoiseCov.at<float>(3,3) = 0;
        cout << "KF.processNoiseCov=" << KF.processNoiseCov << endl;

	while(1)
	{
		cap >> frame; 

		if(frame.empty()) break;
		cvtColor(frame, hsvImage, CV_BGR2HSV);
		frame.copyTo(dstImage);
		hImage.create(hsvImage.size(), CV_8U);

                if(bLButtonDown && 0<selection.width && 0<selection.height)
                {
                        Mat dstROI = dstImage(selection);
                        bitwise_xor(dstROI, Scalar::all(255), dstROI); 
			TermCriteria criteria = TermCriteria(TermCriteria::COUNT + TermCriteria::EPS, 10, 2);
                }

		/* 전역 구조체(STATUS형) 주소를 담고있는 "trackingMode"에 주소가 담겨있다면.. */
		if(trackingMode)
		{
                        int vmin = 50, vmax = 256, smin= 50;
                        inRange(hsvImage, Scalar(0,smin, MIN(vmin,vmax)),
                                Scalar(180,256, MAX(vmin,vmax)), mask);

                        int ch[] = {0,0};
                        hImage.create(hsvImage.size(), CV_8U);
                        mixChannels(&hsvImage, 1, &hImage, 1, ch, 1);

		        if(trackingMode == CALC_HIST)
		        {
			/*                     struct _IplImage *maskROI - 반드시 NULL 이어야 함 */
		                Mat hImageROI(hImage, selection),maskROI(mask,selection);

/*********************** 
  <calcHist 함수>
  - const Mat* images : 히스토그램을 계산할 이미지(혹은 이미지집합). 
                        이미지는 모두 같은 깊이(depth - CV_8U 또는 CV_32F), 
			같은 size 이어야 한다. 각각은 임의갯수의 채널을 가질 수 있다(채널수가 다를수 있다)
  - int nimages : 소스이미지의 갯수 (영상의 갯수)
  - const int* channels : 히스토그램을 계산할 채널번호를 갖는 정수형배열. 
			  first array channels : 0 ~ images[0].channels()-1, 
			  second array channels : images[0].channels() ~ images[0].channels() 
						  			 + images[1].channels()-1
  - InputArray mask : image[i]와 같은 크기의 8비트 마스크영상. 
		      마스크 사용 안하려면, Mat() 또는 noArray()를 적는다.
  - OutputArray hist :  Mat 또는 SparseMat 자료형의 아웃풋 히스토그램. dims는 히스토그램의 차원을 의미한다! 
			dimensionality는 Positive여야 함, 
			CV_MAX_DIMS보다 크지않아야 한다(CV_MAX_DIMS=32 이하로 구현되어 있다.)
  - const int* histSize : 히스토그램 hist의 각 차원의 bin갯수.
  - const float** ranges : 히스토그램의 각 차원bin의 경계값의 배열의 배열이다. 
		uniform=true일때(등간격 히스토그램) ranges[i]는 2개의 요소[lower,upper]로 구성된 배열임. 
		uniform=false일때(비등간격 히스토그램) ranges[i]는 histSize[i]+1 개의 요소값으로 구성된 배열
  - bool uniform=true : 위의 값(등간격히스토그램인지)true 또는 (비등간격히스토그램인지)false 셋팅해주는 것
  - bool accumulate=false  누적플래그. true 이면 calcHist 함수를 수행할 때 히스토그램을 초기화하지 않고, 이전 값을 계속 누적한다.
***********************/
				//       인풋image nimages   채널    마스크  아웃풋 dims  hist크기  ranges
		                calcHist(&hImageROI,  1,  &channels, maskROI, hist,  1,  &histSize, ranges);

		                normalize(hist, hist, 0, 255, NORM_MINMAX); //for backprojection
				//히스토그램 프린트
				normalize(hist, print_histogram, 0, histImage.rows, NORM_MINMAX, CV_32F);
				/*위에 Mat histImage(512,512,CV_8U); 로 선언되어 있다*/
				histImage = Scalar(255);
				int binW = cvRound((double)histImage.cols/histSize);
				int x1, y1, x2, y2;
				for(int i=0; i<histSize; i++)
				{
					x1 = i * binW;
					y1 = histImage.rows;
					x2 = (i+1) * binW;
					y2 = histImage.rows - cvRound(hist.at<float>(i));
					rectangle(histImage, Point(x1,y1), Point(x2,y2), Scalar(0), -1); //fill black
				}
				cout << "histogram" << print_histogram << endl;
				//히스토그램 프린트 끝

//				normalize(hist, hist1, 1.0); //for matching
		                trackWindow = selection;
		                trackingMode = TRACKING;

				//initialize the staate vector(position and velocity)

			        ptMeasured = Point2f(trackWindow.x + trackWindow.width / 2.0,
                                trackWindow.y + trackWindow.height / 2.0);
			        KF.statePost.at<float>(0,0) = ptMeasured.x;
			        KF.statePost.at<float>(1,0) = ptMeasured.y;
			        KF.statePost.at<float>(2,0) = 0;
			        KF.statePost.at<float>(3,0) = 0;

			        setIdentity(KF.errorCovPost, Scalar::all(1));
		        }

	        Mat prediction = KF.predict(); //predict
	        ptPredicted.x = prediction.at<float> (0,0);
        	ptPredicted.y = prediction.at<float> (1,0);
			
	        //TRACKING:
	        calcBackProject(&hImage, 1, &channels, hist, backProject, ranges);
	        backProject &= mask;

		/*meanShift : 마우스 클릭으로 ROI잡은 영역의 사각형 크기가 고정되어 있음.
			      이것을 보완한 것이 CamShift 함수 */
//	        CamShift(backProject, trackWindow, criteria);
		meanShift(backProject, trackWindow, criteria);
	        Point pt1 = Point(trackWindow.x, trackWindow.y);
	        Point pt2 = Point(pt1.x + trackWindow.width, pt1.y + trackWindow.height);
	        rectangle(dstImage, pt1, pt2, Scalar(0,0,255),2);
		
/*		        //Validate the result of cvMeanShift
       		Mat hImageROI(hImage, trackWindow), maskROI(mask, trackWindow);
	        calcHist(&hImageROI, 1, &channels, maskROI, hist2, 1, &histSize, ranges);

	        normalize(hist2, hist2, 1.0);
	        double dist = cv::compareHist(hist1, hist2, HISTCMP_BHATTACHARYYA);

	        	if(dist < DIST_TH) // A tracking object is detected by meanshift
	        	{
        	        ptMeasured = Point2f(trackWindow.x + trackWindow.width / 2.0,
                	                trackWindow.y + trackWindow.height / 2.0);

		        //measurements : the center point of the track_window
		        measurement.at<float>(0,0) = ptMeasured.x;
	        	measurement.at<float>(1,0) = ptMeasured.y;

		        Mat estimated = KF.correct(measurement); //update

		        ptEstimated.x = estimated.at<float> (0,0);
	        	ptEstimated.y = estimated.at<float> (1,0);

		        trackWindow = Rect(ptEstimated.x - selection.width / 2,
        	        ptMeasured.y - trackWindow.height / 2,
                	selection.width,selection.height);

		        pt1 = Point(ptMeasured.x - trackWindow.width / 2,
        	        ptMeasured.y - selection.height / 2);

		        pt2 = Point(ptMeasured.x + trackWindow.width / 2,
        	        ptMeasured.y + trackWindow.height /2);
	        	rectangle(dstImage, pt1, pt2, Scalar(0,0,255),2);
		        circle(dstImage, ptEstimated, 5, Scalar(255,0,0),2);

		        }
			else  //A tracking object is not detected by meanShift
		        {
        	        trackWindow = Rect(ptPredicted.x - selection.width / 2,
                	        ptPredicted.y - selection.height/2,
                        	selection.width, selection.height);

	                pt1 = Point(ptPredicted.x - trackWindow.width/2,
        	                ptPredicted.y - trackWindow.height/2);
                	pt2 = Point(ptPredicted.x + trackWindow.width/2,
                        	ptPredicted.y + trackWindow.height/2);
	                rectangle(dstImage, pt1, pt2, Scalar(0,255,0),2);
        	        circle(dstImage, ptPredicted, 5, Scalar(0,255,0),2);

        		}
*/
		}

	        imshow("dstImage", dstImage);

		//히스토그램 프린트 부분
		imshow("histImage", histImage);

		if(waitKey(10) >= 0) break;
	}
	cap.release();
    	destroyAllWindows(); // It's not necessary !

    	return 0;
}





















