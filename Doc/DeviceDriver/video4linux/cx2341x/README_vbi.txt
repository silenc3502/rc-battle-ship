Format of embedded V4L2_MPEG_STREAM_VBI_FMT_IVTV VBI data
 V4L2_MPEG_STREAM_VBI_FMT_IVTV VBI 데이터의 임베디드 형식
=========================================================
 
This document describes the V4L2_MPEG_STREAM_VBI_FMT_IVTV format of the VBI data embedded in an MPEG-2 program stream. This format is in part dictated by some hardware limitations of the ivtv driver (the driver for the Conexant cx23415/6 chips), in particular a maximum size for the VBI data. Anything longer is cut off when the MPEG stream is played back through the cx23415.
 
이 문서는 MPEG-2 프로그램 스트림에 내장 된 VBI 데이터의 V4L2_MPEG_STREAM_VBI_FMT_IVTV 형식을 설명합니다. 이 형식의 일부는 ivtv 드라이버 (Conexant cx23415 / 6 칩 용 드라이버)의 하드웨어 제한 사항, 특히 VBI 데이터의 최대 크기에 따라 결정됩니다. MPEG 스트림이 cx23415를 통해 재생되면 더 이상 오래 걸리지 않습니다.
 
 
The advantage of this format is it is very compact and that all VBI data for all lines can be stored while still fitting within the maximum allowed size.
 
이 형식의 장점은 매우 작고 모든 라인에 대한 모든 VBI 데이터가 최대 허용 크기 이내에서 저장 될 수 있다는 것입니다.
 
 
The stream ID of the VBI data is 0xBD. The maximum size of the embedded data is 4 + 43 * 36, which is 4 bytes for a header and 2 * 18 VBI lines with a 1 byte header and a 42 bytes payload each. Anything beyond this limit is cut off by the cx23415/6 firmware. Besides the data for the VBI lines we also need 36 bits for a bitmask determining which lines are captured and 4 bytes for a magic cookie, signifying that this data package contains V4L2_MPEG_STREAM_VBI_FMT_IVTV VBI data. 
If all lines are used, then there is no longer room for the bitmask. To solve this two different magic numbers were introduced:
 
VBI 데이터의 스트림 ID는 0xBD이다. 임베디드된 데이터의 최대 크기는 4 + 43 * 36인데, 이 중, 헤더는 4 바이트이고 + (1 바이트헤더가 있는 , (2 * 18 VBI 라인 (36 바이트) + 페이로드는 42 바이트) ) 이다.
이 한계를 초과하는 것은 cx23415 / 6 펌웨어에 의해 차단됩니다. VBI 라인에 대한 데이터 외에도 캡처 된 라인을 결정하는 비트 마스크에는 36 비트가 필요하고 magic cookie에는 4 바이트가 필요하며, 이 데이터 패키지에는 V4L2_MPEG_STREAM_VBI_FMT_IVTV VBI 데이터가 포함되어 있음을 나타냅니다.
모든 라인이 사용되면, 비트-마스크 를 위한 더 이상의 여유가 없습니다. 이를 해결하기 위해 두 가지 magic number가 소개되었습니다.
 
 
'itv0': After this magic number two unsigned longs follow. Bits 0-17 of the first unsigned long denote which lines of the first field are captured. Bits 18-31 of the first unsigned long and bits 0-3 of the second unsigned long are used for the second field.
 
'itv0': 이 magic number 뒤에 부호없는 long 부호 2 개가옵니다. 첫 번째 부호없는 길이의 비트 0-17은 첫 번째 필드의 어떤 줄이 캡처되었는지 나타냅니다. 첫 번째 부호없는 long의 비트 18-31과 두 번째 unsigned long의 비트 0-3이 두 번째 필드에 사용됩니다.
 
 
'ITV0': This magic number assumes all VBI lines are captured, i.e. it implicitly implies that the bitmasks are 0xffffffff and 0xf.
 
'ITV0': 이 magic number는 모든 VBI 라인이 캡처 된 것으로 가정합니다. 즉, 암시적으로 비트 마스크가 0xffffffff와 0xf임을 의미합니다.
 
 
After these magic cookies (and the 8 byte bitmask in case of cookie 'itv0') the captured VBI lines start:
 
이 magic cookies (그리고 쿠키 'itv0'의 경우 8 바이트 비트 마스크) 후에 캡처 된 VBI 라인들이 시작됩니다 :
 
 
For each line the least significant 4 bits of the first byte contain the data type. 
Possible values are shown in the table below. The payload is in the following 42 bytes.
 
각 행에 대해 첫 번째 바이트의 최하위 4 비트는 데이터 유형을 포함합니다.
가능한 값은 아래 표에 나와 있습니다. 페이로드는 다음 42 바이트에 있습니다.
 
 
Here is the list of possible data types:
 
#define IVTV_SLICED_TYPE_TELETEXT       0x1     // Teletext (uses lines 6-22 for PAL)
#define IVTV_SLICED_TYPE_CC             0x4     // Closed Captions (line 21 NTSC)
#define IVTV_SLICED_TYPE_WSS            0x5     // Wide Screen Signal (line 23 PAL)
#define IVTV_SLICED_TYPE_VPS            0x7     // Video Programming System (PAL) (line 16)
 
Hans Verkuil <hverkuil@xs4all.nl>

