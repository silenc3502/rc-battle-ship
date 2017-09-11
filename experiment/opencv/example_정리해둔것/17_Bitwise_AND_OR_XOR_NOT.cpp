#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
 
using namespace cv;
using namespace std;
 
int main(  )
{
    Mat drawing1 = Mat::zeros( Size(400,200), CV_8UC1 );
    Mat drawing2 = Mat::zeros( Size(400,200), CV_8UC1 );
 
    drawing1(Range(0,drawing1.rows),Range(0,drawing1.cols/2))=255; imshow("drawing1",drawing1);
    drawing2(Range(100,150),Range(150,350))=255; imshow("drawing2",drawing2);
 
    Mat res;
    bitwise_and(drawing1,drawing2,res);     imshow("AND",res);
    bitwise_or(drawing1,drawing2,res);      imshow("OR",res);
    bitwise_xor(drawing1,drawing2,res);     imshow("XOR",res);
    bitwise_not(drawing1,res);              imshow("NOT",res);
 
 
    waitKey(0);
    return(0);
}

/*
Bitwise AND, OR, XOR and NOT

void bitwise_and(InputArray src1, InputArray src2, OutputArray dst, InputArray mask=noArray())
Calculates the per-element bit-wise conjunction of two arrays or an array and a scalar.

Parameters:
src1 – first input array or a scalar.
src2 – second input array or a scalar.
src – single input array.
value – scalar value.
dst – output array that has the same size and type as the input arrays.
mask – optional operation mask, 8-bit single channel array, that specifies elements of the output array to be changed.

The function calculates the per-element bit-wise logical conjunction for:
- Two arrays when src1 and src2 have the same size:
   \texttt{dst} (I) =  \texttt{src1} (I)  \wedge \texttt{src2} (I) \quad \texttt{if mask} (I) \ne0
- An array and a scalar when src2 is constructed from Scalar or has the same number of elements as src1.channels():
   \texttt{dst} (I) =  \texttt{src1} (I)  \wedge \texttt{src2} \quad \texttt{if mask} (I) \ne0
- A scalar and an array when src1 is constructed from Scalar or has the same number of elements as src2.channels():
   \texttt{dst} (I) =  \texttt{src1}  \wedge \texttt{src2} (I) \quad \texttt{if mask} (I) \ne0

void bitwise_not(InputArray src, OutputArray dst, InputArray mask=noArray())
Inverts every bit of an array.

void bitwise_or(InputArray src1, InputArray src2, OutputArray dst, InputArray mask=noArray())
Calculates the per-element bit-wise disjunction of two arrays or an array and a scalar.

void bitwise_xor(InputArray src1, InputArray src2, OutputArray dst, InputArray mask=noArray())
Calculates the per-element bit-wise “exclusive or” operation on two arrays or an array and a scalar.
*/
