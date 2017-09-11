#include <iostream>
#include <fstream>
using namespace std;
#include <opencv2/opencv.hpp>
using namespace cv;


int ham(int** &tImg, char* fname, int &tWidth, int &tHeight, float& tAverage){
	Mat tInput = imread("china.jpg");
	Mat input;
	cvtColor(tInput,input,CV_RGB2GRAY);
	tHeight = input.rows;
	tWidth = input.cols;
	tImg = new int* [tHeight];
	tAverage = 0;
	for(int i = 0 ; i < tHeight ; i++)
		tImg[i] = new int[tWidth];
	for(int i = 0 ; i < tHeight ; i++)
		for(int j = 0 ; j < tWidth ; j++){

			tImg[i][j] = (int)input.at<uchar>(i,j);
			tAverage += tImg[i][j];
		}
	tAverage = tAverage / (tHeight* tWidth);
	//nMask = 0;
	//for(int i = 0 ; i <m ; i++)
	//	for(int j = 0 ; j <n ; j++)
	//		nMask += mask[i][j];
	//nMask = (nMask==0)?1:nMask;
	//
	namedWindow("TEMP");
	imshow("TEMP",input);
	return 0;
}
int main(void){
//	Mat input = imread("china.jpg");
	int CAM_ID = 0;
	Mat img;
	Mat out;
 	VideoCapture cap(CAM_ID);

//	cvtColor(img,img,CV_RGB2GRAY);
	
//	Mat img_new = img.clone(); // FINAL IMAGE

    	int tWidth = 0; // TEMPLATE WIDTH
	int tHeight = 0; // TEMPLATE HEIGHT
	float tAverage = 0; // Average Gray Level of Template
	int** tImg = NULL;  // Template Image
	ham(tImg, "china.jpg", tWidth, tHeight, tAverage);
	//---------------------------------//
	cout << "iWIDTH : " << img.cols<<endl;
	cout << "iHEIGHT : " << img.rows<<endl;
	
	cout << "tWIDTH : " << tWidth<<endl;
	cout << "tHEIGHT : " << tHeight<<endl;
	cout << "tAverage : " << tAverage<<endl;
	
//	namedWindow("BEFORE");
//	imshow("BEFORE",img);
	
	//------------------------------------------
	// TEMPLATE MATCHING
	float iAverage = 0; // Average Gray Level of Image
	float threshold = 0.800; //Threshold
	float cor = 0;
	int foundX= 0, foundY = 0;
		
		/// COMPUTE
	for (int i = 0 ; i < img.rows ; i++){	
		if( (img.rows - i) < tHeight ) break;
		for(int j = 0 ; j < img.cols ; j ++){		
			if( (img.cols - j) < tWidth ) break;
			
			/// FIND AVERAGE GRAY LEVEL OF SECTION IMAGE
			for(int k = 0; k < tHeight; k++)
				for(int l = 0; l < tWidth ; l++){
					iAverage += (int)img.at<uchar>(i + k, j + l);
				}
			iAverage = iAverage / (tHeight * tWidth);
			/// COMPUTE COR
			float numerator = 0; //tu so
			float denominator = 0; // mau so
			float tCor = 0; // numerator / denominator
			float sumxix = 0; // sum xi - x
			float sumyiy = 0; // sum yi - y //denominator = sqrt(sumxix * sumyiy)

			for(int k = 0; k < tHeight; k++)
				for(int l = 0; l < tWidth ; l++){
					int xi = tImg[k][l];
					int yi = out.at<uchar>(i + k, j + l);
					numerator += (xi - tAverage)*(yi - iAverage);
					sumxix += (xi - tAverage)*(xi - tAverage);
					sumyiy += (yi - iAverage)*(yi - iAverage);
				}
			denominator = sqrt(sumxix*sumyiy);
			tCor = numerator / denominator;
			if(tCor >= threshold && tCor >= cor){
				cor = tCor;
				foundX = i;
				foundY = j;
				cout << "FOUND : "<< cor <<endl;
			}
		}
	}
		///FOUND?
	for (int i = 0 ; i < img.rows && cor != 0; i++)
		for(int j = 0 ; j < img.cols ; j ++){		
			if( i == foundX || j == foundY || i== foundX + tHeight || j == foundY + tWidth)
				out.at<uchar>(i,j) =255;
		}
	
//-------------------------------------------
	namedWindow("CAM_window",0);
	imshow("CAM_window",out);
	waitKey(0);

}

