#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>

using namespace cv;
using namespace std;

int main(int, char**)
{
    VideoCapture cap(0); // open the default camera
    if(!cap.isOpened())  // check if we succeeded
        return -1;

    namedWindow("Video",1);

// frame 의 가로, 세로 정보를 가져온다
    double Width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
    double Height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

// framesize 설정 - videowriter 인자값으로 넘겨주기 위해 이러한 형태로 만듬
    Size framesize(static_cast<int>(Width), static_cast<int>(Height));

// initializing videowrite class
    VideoWriter oVideoWriter("video.avi" , CV_FOURCC('P','I','M','1'), 20, framesize, true);

    while(1)
    {
        Mat frame;
        cap >> frame;         // get a new frame from camera
        imshow("Video", frame);

	// 녹화           (녹화할 프레임)
 	oVideoWriter.write(frame);

    if(waitKey(10)>= 0) break;
    }

    cap.release();
    destroyAllWindows();

    return 0;
}

/*
cv::VideoWriter::VideoWriter (const String & 	filename,   만들파일이름
					int  	fourcc,     녹화형태(코덱설정)
					double 	fps,        녹화할 영상의 fps설정
					Size 	frameSize,  미리 정의해둔 프레임사이즈
					bool 	isColor = true  컬러형 녹화할것인지..
			      )		

This is an overloaded member function, provided for convenience. It differs from the above function only in what argument(s) it accepts.

Parameters

- filename   Name of the output video file.
- fourcc     4-character code of codec used to compress the frames. 
 	     For example, VideoWriter::fourcc('P','I','M','1') is a MPEG-1 codec, VideoWriter::fourcc('M','J','P','G') is a motion-jpeg codec etc. List of codes can be obtained at Video Codecs by FOURCC page. FFMPEG backend with MP4 container natively uses other values as fourcc code: see ObjectType, so you may receive a warning message from OpenCV about fourcc code conversion.
- fps	     Framerate of the created video stream.
- frameSize  Size of the video frames.
- isColor    If it is not zero, the encoder will expect and encode color frames, otherwise it will work with grayscale frames (the flag is currently supported on Windows only).

Tips:

With some backends fourcc=-1 pops up the codec selection dialog from the system.
To save image sequence use a proper filename (eg. img_%02d.jpg) and fourcc=0 OR fps=0. Use uncompressed image format (eg. img_%02d.BMP) to save raw frames.
Most codecs are lossy. If you want lossless video file you need to use a lossless codecs (eg. FFMPEG FFV1, Huffman HFYU, Lagarith LAGS, etc...)
If FFMPEG is enabled, using codec=0; fps=0; you can create an uncompressed (raw) video file.
*/
