#include<opencv2/opencv.hpp>

//ROI 설정
int setRoi(int x, int y, int width, int height, IplImage* img){
	CvRect rect;

	rect.x = x; rect.y = y;

	rect.width = width; rect.height = height;

	cvSetImageROI(img,rect); //ROI설정부분
	return 0;
}

int main(int* argc, char* argv[])
{
	NamedWindow("test",0); //create window
	IplImage* img = cvLoadImage("경로");
	int add = 300;  //더할값
	setRoi(200,200,100,100,img); //ROI 설정(x,y,width,height,img)
	AddS(img,cvScalar(add),img); //스칼라값을 동일하게받아서 대입
	ResetImageROI(img); //ROI를 원래대로 설정 ( 안하면 ShowImage에서 수정한 부분만 보여준다)
	imshow("test",img); //image를 보여준다
	
	WaitKey(0);

	release(&img); //image Release
	destroyWindow("test"); //close window

	return 0;
}
