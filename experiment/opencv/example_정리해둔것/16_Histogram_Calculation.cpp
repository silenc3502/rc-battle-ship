#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
 
using namespace std;
using namespace cv;
 
int main(int, char**)
{
    Mat gray=imread("result.jpg",0);
    namedWindow( "Gray", 1 );    imshow( "Gray", gray );
 
    // Initialize parameters
    int histSize = 256;    // bin size
    float range[] = { 0, 255 };
    const float *ranges[] = { range };
 
    // Calculate histogram
    MatND hist;
    calcHist( &gray, 1, 0, Mat(), hist, 1, &histSize, ranges, true, false );
     
    // Show the calculated histogram in command window
    double total;
    total = gray.rows * gray.cols;
    for( int h = 0; h < histSize; h++ )
         {
            float binVal = hist.at<float>(h);
            cout<<" "<<binVal;
         }
 
    // Plot the histogram
    int hist_w = 512; int hist_h = 400;
    int bin_w = cvRound( (double) hist_w/histSize );
 
    Mat histImage( hist_h, hist_w, CV_8UC1, Scalar( 0,0,0) );
    normalize(hist, hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );
     
    for( int i = 1; i < histSize; i++ )
    {
      line( histImage, Point( bin_w*(i-1), hist_h - cvRound(hist.at<float>(i-1)) ) ,
                       Point( bin_w*(i), hist_h - cvRound(hist.at<float>(i)) ),
                       Scalar( 255, 0, 0), 2, 8, 0  );
    }
 
    namedWindow( "Result", 1 );    imshow( "Result", histImage );
 
    waitKey(0);    
    return 0;
}

/*
reference 
1 : http://docs.opencv.org/2.4/modules/imgproc/doc/histograms.html?highlight=calchist#calchist
2 : http://docs.opencv.org/2.4/doc/tutorials/imgproc/histograms/histogram_calculation/histogram_calculation.html#histogram-calculation
*/

/*
Histogram Calculation

void calcHist(const Mat* images, int nimages, const int* channels, InputArray mask, OutputArray hist, int dims, const int* histSize, const float** ranges, bool uniform=true, bool accumulate=false )
Calculates a histogram of a set of arrays.

Parameters:
images – Source arrays. They all should have the same depth, CV_8U or CV_32F , and the same size. Each of them can have an arbitrary number of channels.
nimages – Number of source images.
channels – List of the dims channels used to compute the histogram. The first array channels are numerated from 0 to images[0].channels()-1 , the second array channels are counted from images[0].channels() to images[0].channels() + images[1].channels()-1, and so on.
mask – Optional mask. If the matrix is not empty, it must be an 8-bit array of the same size as images[i] . The non-zero mask elements mark the array elements counted in the histogram.
hist – Output histogram, which is a dense or sparse dims -dimensional array.
dims – Histogram dimensionality that must be positive and not greater than CV_MAX_DIMS (equal to 32 in the current OpenCV version).
histSize – Array of histogram sizes in each dimension.

ranges – Array of the dims arrays of the histogram bin boundaries in each dimension. When the histogram is uniform ( uniform =true), then for each dimension i it is enough to specify the lower (inclusive) boundary  L_0 of the 0-th histogram bin and the upper (exclusive) boundary U_{\texttt{histSize}[i]-1} for the last histogram bin histSize[i]-1 . That is, in case of a uniform histogram each of ranges[i] is an array of 2 elements. When the histogram is not uniform ( uniform=false ), then each of ranges[i] contains histSize[i]+1 elements: 

L_0, U_0=L_1, U_1=L_2, ..., U_{\texttt{histSize[i]}-2}=L_{\texttt{histSize[i]}-1}, U_{\texttt{histSize[i]}-1} . 

The array elements, that are not between  
L_0 and  U_{\texttt{histSize[i]}-1} , are not counted in the histogram.

uniform – Flag indicating whether the histogram is uniform or not (see above).
accumulate – Accumulation flag. If it is set, the histogram is not cleared in the beginning when it is allocated. This feature enables you to compute a single histogram from several sets of arrays, or to update the histogram in time.

void normalize(InputArray src, OutputArray dst, double alpha=1, double beta=0, int norm_type=NORM_L2, int dtype=-1, InputArray mask=noArray() )
                           (or)
void normalize(const SparseMat& src, SparseMat& dst, double alpha, int normType)

Normalizes the norm or value range of an array.

 Parameters:
src – input array.
dst – output array of the same size as src .
alpha – norm value to normalize to or the lower range boundary in case of the range normalization.
beta – upper range boundary in case of the range normalization; it is not used for the norm normalization.
normType – normalization type (NORM_MINMAX, NORM_INF, NORM_L1, or NORM_L2).
dtype – when negative, the output array has the same type as src; otherwise, it has the same number of channels as src and the depth =CV_MAT_DEPTH(dtype).
mask – optional operation mask.

The functions normalize scale and shift the input array elements so that
                         
(where p=Inf, 1 or 2) when normType=NORM_INF, NORM_L1, or NORM_L2, respectively; or so that
                        \min _I  \texttt{dst} (I)= \texttt{alpha} , \, \, \max _I  \texttt{dst} (I)= \texttt{beta}

when normType=NORM_MINMAX (for dense arrays only). The optional mask specifies a sub-array to be normalized. This means that the norm or min-n-max are calculated over the sub-array, and then this sub-array is modified to be normalized.

*/
