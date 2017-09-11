/*------------------------------------------------------------------------------------------*\

This file contains material supporting chapter 6 of the cookbook:

Computer Vision Programming using the OpenCV Library.

by Robert Laganiere, Packt Publishing, 2011.



This program is free software; permission is hereby granted to use, copy, modify,

and distribute this source code, or portions thereof, for any purpose, without fee,

subject to the restriction that the copyright notice may not be removed

or altered from any source or altered source distribution.

The software is released on an as-is basis and without any warranties of any kind.

In particular, the software is not guaranteed to be fault-tolerant or free from failure.

The author disclaims all warranties with regard to this software, any use,

and any consequent failure, is purely the responsibility of the user.



Copyright (C) 2010-2011 Robert Laganiere, www.laganiere.name

\*------------------------------------------------------------------------------------------*/



#include <iostream>

#include <iomanip>

#include <opencv2/core/core.hpp>

#include <opencv2/imgproc/imgproc.hpp>

#include <opencv2/highgui/highgui.hpp>

#include "laplacianZC.h"



int main()

{

	// 입력 영상 읽기

	cv::Mat image = cv::imread("robert/boldt.jpg", 0);

	if (!image.data)

		return 0;



	// 영상 띄워 보기

	cv::namedWindow("Original Image");

	cv::imshow("Original Image", image);



	// 소벨 X 미분 계산

	cv::Mat sobelX;

	cv::Sobel(image, sobelX, CV_8U, 1, 0, 3, 0.4, 128);



	// 영상 띄워 보기

	cv::namedWindow("Sobel X Image");

	cv::imshow("Sobel X Image", sobelX);



	// 소벨 Y 미분 계산

	cv::Mat sobelY;

	cv::Sobel(image, sobelY, CV_8U, 0, 1, 3, 0.4, 128);



	// 영상 띄워 보기

	cv::namedWindow("Sobel Y Image");

	cv::imshow("Sobel Y Image", sobelY);



	// 소벨 놈 계산

	cv::Sobel(image, sobelX, CV_16S, 1, 0);

	cv::Sobel(image, sobelY, CV_16S, 0, 1);

	cv::Mat sobel;

	// L1 놈 계산

	sobel = abs(sobelX) + abs(sobelY);



	double sobmin, sobmax;

	cv::minMaxLoc(sobel, &sobmin, &sobmax);

	std::cout << "sobel value range: " << sobmin << "  " << sobmax << std::endl;



	// 화소값을 창에 출력

	for (int i = 0; i<12; i++) {

		for (int j = 0; j<12; j++)

			std::cout << std::setw(5) << static_cast<int>(sobel.at<short>(i + 135, j + 362)) << " ";

		std::cout << std::endl;

	}

	std::cout << std::endl;

	std::cout << std::endl;

	std::cout << std::endl;



	// 8-비트 영상으로 변환

	// sobelImage = -alpha*sobel + 255

	cv::Mat sobelImage;

	sobel.convertTo(sobelImage, CV_8U, -255. / sobmax, 255);



	// 영상 띄워 보기

	cv::namedWindow("Sobel Image");

	cv::imshow("Sobel Image", sobelImage);



	// 소벨 놈에 대한 경계값 적용(낮은 경계값)

	cv::Mat sobelThresholded;

	cv::threshold(sobelImage, sobelThresholded, 225, 255, cv::THRESH_BINARY);



	// 영상 띄워 보기

	cv::namedWindow("Binary Sobel Image (low)");

	cv::imshow("Binary Sobel Image (low)", sobelThresholded);



	// 소벨 놈에 대한 경계값 적용(높은 경계값)

	cv::threshold(sobelImage, sobelThresholded, 190, 255, cv::THRESH_BINARY);



	// 영상 띄워 보기

	cv::namedWindow("Binary Sobel Image (high)");

	cv::imshow("Binary Sobel Image (high)", sobelThresholded);



	// 3x3 라플라시안 계산

	cv::Mat laplace;

	cv::Laplacian(image, laplace, CV_8U, 1, 1, 128);



	// 영상 띄워 보기

	cv::namedWindow("Laplacian Image");

	cv::imshow("Laplacian Image", laplace);



	// Print window pixel values

	for (int i = 0; i<12; i++) {

		for (int j = 0; j<12; j++)

			std::cout << std::setw(5) << static_cast<int>(laplace.at<uchar>(i + 135, j + 362)) - 128 << " ";

		std::cout << std::endl;

	}

	std::cout << std::endl;

	std::cout << std::endl;

	std::cout << std::endl;



	// 7x7 라플라시안 계산

	cv::Laplacian(image, laplace, CV_8U, 7, 0.01, 128);



	// 영상 띄워 보기

	cv::namedWindow("Laplacian Image");

	cv::imshow("Laplacian Image", laplace);



	// 화소값을 창에 출력

	for (int i = 0; i<12; i++) {

		for (int j = 0; j<12; j++)

			std::cout << std::setw(5) << static_cast<int>(laplace.at<uchar>(i + 135, j + 362)) - 128 << " ";

		std::cout << std::endl;

	}



	// 작은 창 추출

	cv::Mat window(image, cv::Rect(362, 135, 12, 12));

	cv::namedWindow("Image window");

	cv::imshow("Image window", window);

	cv::imwrite("window.bmp", window);



	// LaplacianZC 클래스를 이용한 라플라시안 계산

	LaplacianZC laplacian;

	laplacian.setAperture(7);

	cv::Mat flap = laplacian.computeLaplacian(image);

	double lapmin, lapmax;

	cv::minMaxLoc(flap, &lapmin, &lapmax);

	std::cout << "Laplacian value range=[" << lapmin << "," << lapmax << "]\n";

	laplace = laplacian.getLaplacianImage();

	cv::namedWindow("Laplacian Image (7x7)");

	cv::imshow("Laplacian Image (7x7)", laplace);



	// 라플라시안 값 출력

	std::cout << std::endl;

	for (int i = 0; i<12; i++) {

		for (int j = 0; j<12; j++)

			std::cout << std::setw(5) << static_cast<int>(flap.at<float>(i + 135, j + 362) / 100) << " ";

		std::cout << std::endl;

	}

	std::cout << std::endl;



	// 영교차점 계산 및 띄워 보기

	cv::Mat zeros;

	zeros = laplacian.getZeroCrossings(lapmax);

	cv::namedWindow("Zero-crossings");

	cv::imshow("Zero-crossings", zeros);



	// 영교차점 계산 및 띄워 보기(소벨 버전)

	zeros = laplacian.getZeroCrossings();

	zeros = laplacian.getZeroCrossingsWithSobel(50);

	cv::namedWindow("Zero-crossings (2)");

	cv::imshow("Zero-crossings (2)", zeros);



	// 화소값을 창에 출력

	for (int i = 0; i<12; i++) {

		for (int j = 0; j<12; j++)

			std::cout << std::setw(2) << static_cast<int>(zeros.at<uchar>(i + 135, j + 362)) << " ";

		std::cout << std::endl;

	}



	// 영상을 창에 띄워 보기

	cv::rectangle(image, cv::Point(362, 135), cv::Point(374, 147), cv::Scalar(255, 255, 255));

	cv::namedWindow("Original Image with window");

	cv::imshow("Original Image with window", image);



	cv::waitKey();

	return 0;

}



출처: http://visionprogrammer.tistory.com/15 [합격통지서]
