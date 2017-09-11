#include<opencv2/opencv.hpp>

using namespace cv;
using namespace std;

int main(){



	Mat srcImage1 = imread ("China.jpg" , IMREAD_GRAYSCALE);
	Mat srcImage2;

	VideoCapture cap(0);

while(1){
	cap >> srcImage2;

	if(srcImage1.empty() || srcImage2.empty())
		printf("can't open image\n");

		return -1;
	

	vector <KeyPoint> keypoints1, keypoints2;
	Mat descriptors1, descriptors2;

	Ptr<KAZE> kazeF = KAZE::create(true);
	kazeF->detectAndCompute(srcImage1, noArray(), keypoints1, descriptors1);
	kazeF->detectAndCompute(srcImage2, noArray(), keypoints2, descriptors2);

	int k = 2;
	vector < vector<DMatch> > matches;

	Ptr<DescriptorMatcher> matcher;
	matcher = DescriptorMatcher::create("FlannBased");
	matcher -> knnMatch(descriptors1, descriptors2, matches, k);
	cout << "matches.size()=" << matches.size() << endl;

	vector<DMatch> goodMatches;
	float nndrRatio = 0.6f;
	for(int i = 0; i < matches.size(); i++)
	{

		if(matches.at(i).size() == 2 &&
			matches.at(i).at(0).distance  <= nndrRatio * matches.at(i).at(1).distance)
		{
		goodMatches.push_back(matches[i][0]);
		}
	}
	
	cout << "goodmatches.size()=" << goodMatches.size() << endl;

	Mat imgMatches;	
	drawMatches(srcImage1, keypoints1, srcImage2, keypoints2, goodMatches,
			imgMatches, Scalar:: all(-1),Scalar::all(-1),
			vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

		if(goodMatches.size() < 4)
			return 0;


	vector<Point2f> obj;
	vector<Point2f> scene;
	for(int i = 0; i < goodMatches.size(); i++)
	{
		obj.push_back(keypoints1[goodMatches[i].queryIdx].pt);
		scene.push_back(keypoints2[goodMatches[i].trainIdx].pt);

	}

	Mat H = findHomography(obj,scene, RANSAC);

	vector<Point2f> objP(4);
	objP[0] = Point2f(0,0);
	objP[1] = Point2f(srcImage1.cols,0);
	objP[2] = Point2f(srcImage1.cols, srcImage1.rows);
	objP[3] = Point2f(0, srcImage1.rows);


	vector<Point2f> sceneP(4);
	perspectiveTransform(objP, sceneP, H);


	for(int i = 0; i < 4; i++)
		sceneP[i] += Point2f(srcImage1.cols, 0);
	for(int i = 0; i < 4; i++)
		line(imgMatches, sceneP[i], sceneP[(i+1) % 4], Scalar(255, 0, 0), 4);

	imshow("imgMatches",imgMatches);
//	imshow("srcImage1", srcImage1);
//	imshow("srcImage2", srcImage2);

	waitKey();
	
	return 0;
	}
}
