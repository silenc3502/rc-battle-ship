#include<opencv2/opencv.hpp>

using namespace cv;
using namespace std;

#define DIST_TH 0.4 // threshold for histogram matching

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
	VideoCapture inputVideo(0);
	if(!inputVideo.isOpened())
	{
		cout << "Can not fuck" << endl;
		return 0;
	}

	Size size = Size((int) inputVideo.get(CAP_PROP_FRAME_WIDTH),
		(int) inputVideo.get(CAP_PROP_FRAME_HEIGHT));
	int fps = (int)(inputVideo.get(CAP_PROP_FPS));
	if(fps <= 0) fps = 24;

	Mat dstImage;
	namedWindow("dstImage",0);
	resizeWindow("dstImage", 1080,720);

	setMouseCallback("dstImage", onMouse, (void*)&dstImage);

	int histSize = 8;
	float valueRange[] = {0, 180};
	const float* ranges[] = {valueRange};
	int channels = 0;
	Mat hist, backProject;

	//히스토그램 출력용 선언
        Mat print_histogram;
        Mat histImage(512,512,CV_8U);

	int fourcc = VideoWriter::fourcc('D','I','V','X');
	bool isColor = true;
	VideoWriter outputVideo("trackingRect.avi",fourcc, fps, size, isColor);
	if(!outputVideo.isOpened())
	{
		cout << "Can not open output Video!!!" << endl;
		return 0;
	}

	if(fourcc != -1)
	{
		imshow("dstImage",0);
		waitKey(100);
	}

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
	
	Mat hist1, hist2;

	for(;;)
	{
		inputVideo >> frame;
		if(frame.empty())
			break;
		cvtColor(frame, hsvImage, CV_BGR2HSV);
		frame.copyTo(dstImage);
		hImage.create(hsvImage.size(), CV_8U);

		if(bLButtonDown && 0 < selection.width && 0 < selection.height)
		{
			Mat dstROI = dstImage(selection);
			bitwise_xor(dstROI, Scalar::all(255), dstROI);
		}

		if(trackingMode) // CALC_HIST or TRACKING
		{
			//마스크생성
			int vmin = 50, vmax = 256, smin= 50;
			inRange(hsvImage, Scalar(0,smin, MIN(vmin,vmax)),
				Scalar(180,256, MAX(vmin,vmax)), mask);
			
			int ch[] = {0,0};
			hImage.create(hsvImage.size(), CV_8U);
			mixChannels(&hsvImage, 1, &hImage, 1, ch, 1);

		if(trackingMode == CALC_HIST)
		{
			Mat hImageROI(hImage, selection),maskROI(mask,selection);
			calcHist(&hImageROI, 1,&channels, maskROI,hist,1,&histSize,ranges);
			calcHist(&hImageROI, 1,&channels, maskROI,hist1,1,&histSize,ranges);

			normalize(hist1,hist1,1.0); //for matching
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

	meanShift(backProject, trackWindow, criteria);
//	CamShift(backProject, trackWindow, criteria);
	Point pt1 = Point(trackWindow.x, trackWindow.y);
	Point pt2 = Point(pt1.x + trackWindow.width, pt1.y + trackWindow.height);
	rectangle(dstImage, pt1, pt2, Scalar(0,0,255),2);

	//Validate the result of cvMeanShift
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
}
	imshow("dstImage", dstImage);
	//히스토그램 프린트 부분
        imshow("histImage", histImage);

	outputVideo << dstImage;

	if(waitKey(10) >= 0) break;
        }
        inputVideo.release();
        destroyAllWindows(); // It's not necessary !

	return 0;
}

