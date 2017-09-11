The cx23416 can produce (and the cx23415 can also read) raw YUV output. The format of a YUV frame is specific to this chip and is called HM12. 'HM' stands for 'Hauppauge Macroblock', which is a misnomer as 'Conexant Macroblock' would
be more accurate.
 
cx23416은 raw YUV 출력을 생성 할 수 있습니다 (또한 cx23415도 읽을 수 있음). YUV 프레임의 형식은이 칩에 고유하며 HM12라고합니다. 'HM'은 'Hauppauge Macroblock'의 약자로, 'Conexant Macroblock'이 더 정확하기 때문에 잘못된 이름입니다.
 
 
The format is YUV 4:2:0 which uses 1 Y byte per pixel and 1 U and V byte per four pixels.
 
포맷은 픽셀 당 1Y 바이트와 4 픽셀 당 1U 및 V 바이트를 사용하는 YUV 4 : 2 : 0입니다.
 
 
The data is encoded as two macroblock planes, the first containing the Y values, the second containing UV macroblocks.
 
데이터는 두 개의 매크로 블록 평면으로 인코딩됩니다. 첫 번째는 Y 값을 포함하고 두 번째 매크로는 UV 매크로 블록을 포함합니다.
 
 
The Y plane is divided into blocks of 16x16 pixels from left to right and from top to bottom. Each block is transmitted in turn, line-by-line.
 
Y 평면은 왼쪽에서 오른쪽으로, 위에서 아래로 16x16 픽셀의 블록으로 나뉩니다. 각 블록은 라인 단위로 차례로 전송됩니다.
 
 
So the first 16 bytes are the first line of the top-left block, the second 16 bytes are the second line of the top-left block, etc. After transmitting this block the first line of the block on the right to the
first block is transmitted, etc.
 
따라서 첫 번째 16 바이트는 상단 왼쪽 블록의 첫 번째 라인이고, 두 번째 16 바이트는 상단 왼쪽 블록의 두 번째 라인입니다.이 블록을 전송 한 후 오른쪽 블록의 첫 번째 라인부터 첫 번째 블록 전송된다.
 
 
The UV plane is divided into blocks of 16x8 UV values going from left to right, top to bottom. Each block is transmitted in turn, line-by-line.
 
UV 평면은 왼쪽에서 오른쪽, 위에서 아래로가는 16x8 UV 값의 블록으로 나뉩니다. 각 블록은 라인 단위로 차례로 전송됩니다.
 
 
So the first 16 bytes are the first line of the top-left block and contain 8 UV value pairs (16 bytes in total). The second 16 bytes are the second line of 8 UV pairs of the top-left block, etc. After transmitting this block the first line of the block on the right to the first block is transmitted, etc.
 
따라서 처음 16 바이트는 왼쪽 위 블록의 첫 번째 라인이며 8 개의 UV 값 쌍 (총 16 바이트)을 포함합니다. 두 번째 16 바이트는 왼쪽 상단 블록의 8 쌍의 UV 쌍 중 두 번째 줄입니다.이 블록을 전송 한 후 첫 번째 블록 오른쪽의 첫 번째 블록이 전송됩니다.
 
 
The code below is given as an example on how to convert HM12 to separate Y, U and V planes. This code assumes frames of 720x576 (PAL) pixels.
 
아래의 코드는 HM12를 Y, U 및 V 평면으로 분리하는 방법을 보여주는 예입니다. 이 코드는 720x576 (PAL) 픽셀의 프레임을 가정합니다.
 
 
The width of a frame is always 720 pixels, regardless of the actual specified width.
 
프레임의 너비는 실제 지정된 너비와 관계없이 항상 720 픽셀입니다.
 
 
If the height is not a multiple of 32 lines, then the captured video is missing macroblocks at the end and is unusable. So the height must be a multiple of 32.
 
높이가 32 라인의 배수가 아니면 캡쳐 된 비디오 끝에 매크로-블록이 없으므로 사용할 수 없습니다. 따라서 높이는 32의 배수 여야합니다.
 
--------------------------------------------------------------------------
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
static unsigned char frame[576*720*3/2];
static unsigned char framey[576*720];
static unsigned char frameu[576*720 / 4];
static unsigned char framev[576*720 / 4];
 
static void de_macro_y(unsigned char* dst, unsigned char *src, int dstride, int w, int h)
{
    unsigned int y, x, i;
 
    // descramble Y plane
    // dstride = 720 = w
    // The Y plane is divided into blocks of 16x16 pixels
    // Each block in transmitted in turn, line-by-line.
    for (y = 0; y < h; y += 16) {
	for (x = 0; x < w; x += 16) {
	    for (i = 0; i < 16; i++) {
		memcpy(dst + x + (y + i) * dstride, src, 16);
		src += 16;
	    }
	}
    }
}
 
static void de_macro_uv(unsigned char *dstu, unsigned char *dstv, unsigned char *src, int dstride, int w, int h)
{
    unsigned int y, x, i;
 
    // descramble U/V plane
    // dstride = 720 / 2 = w
    // The U/V values are interlaced (UVUV...).
    // Again, the UV plane is divided into blocks of 16x16 UV values.
    // Each block in transmitted in turn, line-by-line.
    for (y = 0; y < h; y += 16) {
	for (x = 0; x < w; x += 8) {
	    for (i = 0; i < 16; i++) {
		int idx = x + (y + i) * dstride;
 
		dstu[idx+0] = src[0];  dstv[idx+0] = src[1];
		dstu[idx+1] = src[2];  dstv[idx+1] = src[3];
		dstu[idx+2] = src[4];  dstv[idx+2] = src[5];
		dstu[idx+3] = src[6];  dstv[idx+3] = src[7];
		dstu[idx+4] = src[8];  dstv[idx+4] = src[9];
		dstu[idx+5] = src[10]; dstv[idx+5] = src[11];
		dstu[idx+6] = src[12]; dstv[idx+6] = src[13];
		dstu[idx+7] = src[14]; dstv[idx+7] = src[15];
		src += 16;
	    }
	}
    }
}
 
/*************************************************************************/
int main(int argc, char **argv)
{
    FILE *fin;
    int i;
 
    if (argc == 1) fin = stdin;
    else fin = fopen(argv[1], "r");
 
    if (fin == NULL) {
	fprintf(stderr, "cannot open input\n");
	exit(-1);
    }
    while (fread(frame, sizeof(frame), 1, fin) == 1) {
	de_macro_y(framey, frame, 720, 720, 576);
	de_macro_uv(frameu, framev, frame + 720 * 576, 720 / 2, 720 / 2, 576 / 2);
	fwrite(framey, sizeof(framey), 1, stdout);
	fwrite(framev, sizeof(framev), 1, stdout);
	fwrite(frameu, sizeof(frameu), 1, stdout);
    }
    fclose(fin);
    return 0;
}
 
--------------------------------------------------------------------------

