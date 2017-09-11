/* assertion failed : 
디버그 모드에서 주어진 조건이 만족하지 않으면 강제로 프로그램을 종료시키는 '안전장치' 함수입니다. 
정상적인 값이 아닌데 그냥 계속 실행되다가 도저히 알수없는 어딘가에서 죽는 것보다는, 처음부터 잘못된 값이 들어왔을 때,
인지를 한 그순간 바로 종료시켜 버린다면 문제를 찾기가 훨씬 쉬울 것이기 때문이다.
*/

/* 에러메세지를 분석해보자. */

//OpenCV Error : Assertion failed ((depth == CV_8U || depth == CV_32F) && type == templ.type() && _img.dims() <= 2) in matchTemplate, file /home/kang/kang/opencv_src/opencv/modules/imgproc/src/templmatch.hpp, line 1099
//terminate called after throwing an instance of 'cv::Exception'

/* 이 문구를 파헤쳐 보면, 
1. in matchTemplate 에서 에러가 발생했다. (거의 인자값 잘못 전달받는 에러이다)
2. file /home/kang/kang/opencv_src/opencv/modules/imgproc/src/templmatch.hpp, line 1099 에서 발생한 에러이다.
3. 2번의 파일의 1099 번째 줄의, 인자값
((depth == CV_8U || depth == CV_32F) && type == templ.type() && _img.dims() <= 2) 부분을 확인해본다. */

/* opentest.cpp의
95번째 줄    matchTemplate(frame, tImage1, result, TM_SQDIFF);
여기에서 에러가 발생한 것인데, */

/* templmatch.hpp 의 
1099번째 줄  CV_Assert( (depth == CV_8U || depth == CV_32F) && type == _templ.type() && _img.dims() <= 2 );
은, 아래 함수의 인자값으로 동작하는 함수이다. */

/* matchTemplate()의 형식은 이렇다. */
void cv::matchTemplate	(InputArray image, 
			 InputArray templ,
	 		 OutputArray result,
			 int method,
			 InputArray mask = noArray() 
			 )		

/* 파라미터 정보는 이렇다.
 image : 검색이 실행되는 이미지. 8 비트 또는 32 비트 부동 소수점이어야합니다.
 templ : 검색된 템플릿. 소스 이미지보다 크지 않아야하며 동일한 데이터 유형을 가져야합니다.
 result : 비교 결과의 맵. 단일 채널 32 비트 부동 소수점이어야합니다. 이미지가 W × H이고 templ가 w × h 인 경우 결과는 (W-w + 1) × (H-h + 1)입니다.
 method : 비교 메서드를 지정하는 매개 변수이다. cv :: TemplateMatchModes를 보자.
 mask : 검색된 템플릿의 마스크. 그것은 templ과 동일한 데이터 유형과 크기를 가져야합니다. 기본적으로 설정되지 않습니다. 현재는 TM_SQDIFF 및 TM_CCORR_NORMED 메소드 만 지원됩니다.
*/

/* 템플릿을 겹쳐진 이미지 영역과 비교하는 함수이며, 아래 내용은 해석하여 보자 (오 해석이 있을수 있으므로)
 The function slides through image , compares the overlapped patches of size w×h against templ using the specified method and stores the comparison results in result . Here are the formulae for the available comparison methods ( I denotes image, T template, R result ). The summation is done over template and/or the image patch: x′=0...w−1,y′=0...h−1
 After the function finishes the comparison, the best matches can be found as global minimums (when TM_SQDIFF was used) or maximums (when TM_CCORR or TM_CCOEFF was used) using the minMaxLoc function. In case of a color image, template summation in the numerator and each sum in the denominator is done over all of the channels and separate mean values are used for each channel. That is, the function can take a color template and a color image. The result will still be a single-channel image, which is easier to analyze.
*/

