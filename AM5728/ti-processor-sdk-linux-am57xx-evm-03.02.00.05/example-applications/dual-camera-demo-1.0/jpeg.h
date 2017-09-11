#ifndef JPEG_H
#define JPEG_H

#ifdef __cplusplus
extern "C" {
#endif

void jpegWrite(unsigned char* yuv, int index, int width, int height);
#ifdef __cplusplus
}
#endif

#endif // JPEG_H
