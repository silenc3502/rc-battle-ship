[README.cx88] 파일
 
cx8800 release notes	cx8800 릴리즈 노트
====================
 
This is a v4l2 device driver for the cx2388x chip.
이것은 cx2388x 칩용 v4l2 장치 드라이버입니다.
 
 
current status	현재 상태
==============
 
video
	- Basically works.
	- For now, only capture and read(). Overlay isn't supported.
	- 기본적으로 작동합니다.
	- 당분간은 capture와 read () 만하십시오. 오버레이는 지원되지 않습니다.
 
 
audio
	- The chip specs for the on-chip TV sound decoder are next to useless :-/
	- Neverless the builtin TV sound decoder starts working now, at least for some standards.
	  FOR ANY REPORTS ON THIS PLEASE MENTION THE TV NORM YOU ARE USING.
	- Most tuner chips do provide mono sound, which may or may not be useable depending on the board design.  With the Hauppauge cards it works, so there is mono sound available as fallback.
	- audio data dma (i.e. recording without loopback cable to the sound card) is supported via cx88-alsa.
	- 온-칩 TV 사운드 디코더를 위한 칩 사양은 쓸모가 없다. :-/
	- 내장된 TV 사운드 디코더가 적어도 일부 표준에서는 작동하기 시작한 적은 절대로 없습니다. 이 보고서에 대해 당신이 사용하고있는 TV에 대해 언급하십시오.
	- 대부분의 튜너 칩은 모노 사운드를 제공합니다. 모노 사운드는 보드 디자인에 따라 사용 가능하거나 사용하지 못할 수 있습니다. Hauppauge카드로 작동하므로 fallback으로 사용할 수 있는 모노 사운드가 있다. 
	- 오디오 데이터 dma (즉, 사운드 카드에 루프백 케이블없이 녹음)는 cx88-alsa를 통해 지원됩니다.
 
 
vbi
	- Code present. Works for NTSC closed caption. PAL and other TV norms may or may not work.
	- 코드가 있습니다. NTSC 자막 작업. PAL 및 기타 TV 표준이 작동하거나 작동하지 않을 수 있습니다.
 
 
 
how to add support for new cards
새 카드에 대한 지원을 추가하는 방법
================================
 
The driver needs some config info for the TV cards.  This stuff is in cx88-cards.c.  If the driver doesn't work well you likely need a new entry for your card in that file.  Check the kernel log (using dmesg) to see whenever the driver knows your card or not.  There is a line like this one:
드라이버는 TV 카드에 대한 몇 가지 구성 정보가 필요합니다. 이 내용은 cx88-cards.c에 있습니다. 드라이버가 잘 작동하지 않으면 해당 파일에 카드에 대한 새로운 항목이 필요할 수 있습니다. dmesg를 사용하여 커널 로그를 확인하여 드라이버가 자신의 카드를 아는지 여부를 확인하십시오. 다음과 같은 행이 있습니다.
 
	cx8800[0]: 
		subsystem: 0070:3400, 
		board: Hauppauge WinTV \34xxx models [card=1,autodetected]
 
If your card is listed as "board: UNKNOWN/GENERIC" it is unknown to the driver.  What to do then?
귀하의 카드가 "board : UNKNOWN / GENERIC"으로 표시되면 드라이버가 알 수 없습니다. 그 때 무엇을 해야 합니까? 
 
 (1) Try upgrading to the latest snapshot, maybe it has been added meanwhile.
 (1) 최신 스냅 샷으로 업그레이드하십시오. 그 사이에 스냅 샷이 추가되었을 수도 있습니다.
 
 (2) You can try to create a new entry yourself, have a look at cx88-cards.c.  If that worked, mail me your changes as unified diff ("diff -u").
?(2) cx88-cards.c에서 새로운 항목을 직접 만들 수 있습니다. 문제가 해결되면 변경 사항을 통합 된 diff ( "diff -u")로 메일로 보내주십시오.
 
 (3) Or you can mail me the config information.  I need at least the following informations to add the card:
?(3) 또는 설정 정보를 메일로 보낼 수 있습니다. 카드를 추가하려면 최소한 다음 정보가 필요합니다.
 
     * the PCI Subsystem ID ("0070:3400" from the line above, "lspci -v" output is fine too).
     * the tuner type used by the card.  You can try to find one by trial-and-error using the tuner=<n> insmod option.  If you
       know which one the card has you can also have a look at the list in CARDLIST.tuner
   ??* PCI 서브 시스템 ID (위의 "0070 : 3400", "lspci -v"출력도 괜찮음).
?????* 카드가 사용하는 튜너 유형. 튜너 = <n> insmod 옵션을 사용하여 시행 착오를 통해 시도해 볼 수 있습니다. 만약 카드 중 어느 카드인지 알면 CARDLIST.tuner의 목록을 볼 수도 있습니다.
 
 
Have fun,
(하나도 즐겁지 아니하다..;; :ㅣ)
 
  Gerd
 
--
Gerd Knorr <kraxel@bytesex.org> [SuSE Labs]
 
 
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 
 
[README.saa7134] 파일
 
What is it?
===========
 
This is a v4l2/oss device driver for saa7130/33/34/35 based capture / TV boards.  See http://www.semiconductors.philips.com/pip/saa7134hl for a description.
이것은 saa7130 / 33 / 34 / 35 기반 캡처 / TV 보드 용 v4l2 / oss 장치 드라이버입니다. 자세한 내용은 http://www.semiconductors.philips.com/pip/saa7134hl을 참조하십시오.
 
 
Status
======
 
Almost everything is working.  video, sound, tuner, radio, mpeg ts, ...
거의 모든 것이 효과적입니다. 비디오, 사운드, 튜너, 라디오, MPEG TS, ...
 
As with bttv, card-specific tweaks are needed.  Check CARDLIST for a list of known TV cards and saa7134-cards.c for the drivers card configuration info.
bttv와 마찬가지로 카드별 조정이 필요합니다. CARDLIST에서 알려진 TV 카드 목록을 확인하고 드라이버 카드 구성 정보에 대해 saa7134-cards.c를 확인하십시오.
 
 
Build
=====
 
Pick up videodev + v4l2 patches from http://bytesex.org/patches/.
http://bytesex.org/patches/에서 videodev + v4l2 패치를 가져옵니다.
 
Configure, build, install + boot the new kernel.  You'll need at least these config options:
새 커널을 구성, 빌드 및 인스톨 + 부팅 합니다. 적어도 다음과 같은 설정 옵션이 필요합니다.
 
	CONFIG_I2C=m
	CONFIG_VIDEO_DEV=m
 
Type "make" to build the driver now.  "make install" installs the driver.  "modprobe saa7134" should load it.  Depending on the card you might have to pass card=<nr> as insmod option, check CARDLIST for valid choices.
이제 "make"를 입력하여 드라이버를 빌드하십시오. "make install"은 드라이버를 설치합니다. "modprobe saa7134"가 로드해야 합니다. 카드에 따라 insmod 옵션으로 card = <nr>을 전달해야 할 수도 있습니다. 유효한 선택 사항은 CARDLIST를 확인하십시오.
 
 
Changes / Fixes
===============
 
Please mail me unified diffs ("diff -u") with your changes, and don't forget to tell me what it changes / which problem it fixes / whatever it is good for ...
변경 사항과 함께 통일 된 diffs ( "diff -u")를 보내 주시고 변경 사항 / 어떤 문제가 수정되었는지 / 어떤 것이 좋을 지 알려주시기 바랍니다.
 
 
Known Problems
==============
 
* The tuner for the flyvideos isn't detected automatically and the default might not work for you depending on which version you have.  There is a tuner= insmod option to override the driver's default.
* flyvideos 용 튜너는 자동으로 감지되지 않으며 갖고있는 버전에 따라 기본값이 작동하지 않을 수 있습니다.
??드라이버의 기본값을 무시하려면 tuner = insmod 옵션이 필요합니다.
 
Card Variations:
================
 
Cards can use either of these two crystals (xtal):
카드는 이 두 결정 (xtal) 중 하나를 사용할 수 있습니다.
 - 32.11 MHz -> .audio_clock=0x187de7
 - 24.576MHz -> .audio_clock=0x200000
(xtal * .audio_clock = 51539600)
 
Some details about 30/34/35:
30/34/35에 대한 세부 정보 :
 
   saa7130 - low-price chip, doesn't have mute, that is why all those cards should have .mute field defined in their tuner structure.
   saa7130 - 저렴한 가격의 칩, 음소거가 없기 때문에 모든 카드의 튜너 구조에 .mute 필드가 정의되어 있어야합니다.
 
 - saa7134 - usual chip
   saa7134 - 일반적인 칩
 
 - saa7133/35 - saa7135 is probably a marketing decision, since all those chips identifies itself as 33 on pci.
   saa7133 / 35 - saa7135는 아마도 마케팅 결정 일 것입니다. 왜냐하면 모든 칩은 pci로 33을 식별하기 때문입니다.
 
 
Credits
=======
 
andrew.stevens@philips.com + werner.leeb@philips.com for providing saa7134 hardware specs and sample board.
saa7134 하드웨어 사양 및 샘플 보드를 제공하려면 andrew.stevens@philips.com + werner.leeb@philips.com.
 
 
Have fun,
 
  Gerd
 
--
Gerd Knorr <kraxel@bytesex.org> [SuSE Labs]
 
 
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 
 
[README.ivtv] 파일
 
 
ivtv release notes
ivtv 릴리즈 노트
==================
 
This is a v4l2 device driver for the Conexant cx23415/6 MPEG encoder/decoder.
이것은 Conexant cx23415 / 6 MPEG 인코더 / 디코더 용 v4l2 장치 드라이버입니다.
 
The cx23415 can do both encoding and decoding, the cx23416 can only do MPEG encoding. Currently the only card featuring full decoding support is the Hauppauge PVR-350.
cx23415는 인코딩과 디코딩을 모두 할 수 있으며 cx23416은 MPEG 인코딩만 할 수 있습니다. 현재 전체 디코딩을 지원하는 유일한 카드는 Hauppauge PVR-350입니다.
 
NOTE: this driver requires the latest encoder firmware (version 2.06.039, size 376836 bytes). Get the firmware from here:
참고 :이 드라이버는 최신 인코더 펌웨어 (버전 2.06.039, 크기 376836 바이트)가 필요합니다. 여기에서 펌웨어를 가져 오십시오
 
http://dl.ivtvdriver.org/ivtv/firmware/
 
NOTE: 'normal' TV applications do not work with this driver, you need an application that can handle MPEG input such as mplayer, xine, MythTV, etc.
참고 : '일반'TV 응용 프로그램은이 드라이버에서 작동하지 않으므로 mplayer, xine, MythTV 등과 같은 MPEG 입력을 처리 할 수있는 응용 프로그램이 필요합니다.
 
The primary goal of the IVTV project is to provide a "clean room" Linux Open Source driver implementation for video capture cards based on the iCompression iTVC15 or Conexant CX23415/CX23416 MPEG Codec.
IVTV 프로젝트의 주요 목표는 iCompression iTVC15 또는 Conexant CX23415 / CX23416 MPEG 코덱을 기반으로 한 비디오 캡처 카드 용 "클린 룸"Linux 오픈 소스 드라이버 구현을 제공하는 것입니다.
 
 
Features:
 
 * Hardware mpeg2 capture of broadcast video (and sound) via the tuner or S-Video/Composite and audio line-in.
 * Hardware mpeg2 capture of FM radio where hardware support exists
 * Supports NTSC, PAL, SECAM with stereo sound
 * Supports SAP and bilingual transmissions.
 * Supports raw VBI (closed captions and teletext).
 * Supports sliced VBI (closed captions and teletext) and is able to insert this into the captured MPEG stream.
 * Supports raw YUV and PCM input.
 
?* 튜너 또는 S-Video/Composite 및 오디오 라인 입력을 통해 방송 비디오 (및 사운드)를 MPEG2로 캡처합니다.
?* 하드웨어 지원이 있는 FM 라디오의 하드웨어 mpeg2 캡처
?* NTSC, PAL, SECAM 및 스테레오 사운드 지원
?* SAP 및 이중 언어 전송을 지원합니다.
?* 원시 VBI (자막 및 텔레 텍스트)를 지원합니다.
?* 슬라이스 VBI (자막 및 텔레 텍스트)를 지원하고 캡쳐 된 MPEG stream을 삽입할 수 있습니다.
?* 원시 YUV 및 PCM 입력을 지원합니다.
 
 
Additional features for the PVR-350 (CX23415 based):
PVR-350 (CX23415 기반)의 추가 기능
 
 * Provides hardware mpeg2 playback
 * Provides comprehensive OSD (On Screen Display: ie. graphics overlaying the video signal)
 * Provides a framebuffer (allowing X applications to appear on the video device)
 * Supports raw YUV output.
 
?* 하드웨어 MPEG2 재생 제공
?* 포괄적 인 OSD 제공 (온 스크린 디스플레이 : 즉, 비디오 신호를 오버레이하는 그래픽)
?* 프레임 버퍼 제공 (X 응용 프로그램을 비디오 장치에 표시 할 수있게 함)
?* 원시 YUV 출력을 지원합니다.
 
IMPORTANT: In case of problems first read this page:
!! 중요 : 문제가 발생한 경우 이 페이지를 먼저 읽으십시오.
	   http://www.ivtvdriver.org/index.php/Troubleshooting
 
See also:
 
Homepage + Wiki
http://www.ivtvdriver.org
 
IRC
irc://irc.freenode.net/ivtv-dev
 
----------------------------------------------------------
 
Devices
=======
 
A maximum of 12 ivtv boards are allowed at the moment.
현재 최대 12 개의 ivtv 보드가 허용됩니다.
 
Cards that don't have a video output capability (i.e. non PVR350 cards) lack the vbi8, vbi16, video16 and video48 devices. They also do not support the framebuffer device /dev/fbx for OSD.
비디오 출력 기능이없는 카드 (예 : 비 PVR350 카드)에는 vbi8, vbi16, video16 및 video48 장치가 없습니다. 또한 OSD 용 프레임 버퍼 장치 / dev / fbx도 지원하지 않습니다.
 
The radio0 device may or may not be present, depending on whether the card has a radio tuner or not.
radio0 장치는 카드에 라디오 튜너가 있는지 여부에 따라 표시 될 수도 있고 표시되지 않을 수도 있습니다.
 
Here is a list of the base v4l devices:   다음은 기본 v4l 장치 목록입니다
crw-rw----    1 root     video     81,   0 Jun 19 22:22 /dev/video0
crw-rw----    1 root     video     81,  16 Jun 19 22:22 /dev/video16
crw-rw----    1 root     video     81,  24 Jun 19 22:22 /dev/video24
crw-rw----    1 root     video     81,  32 Jun 19 22:22 /dev/video32
crw-rw----    1 root     video     81,  48 Jun 19 22:22 /dev/video48
crw-rw----    1 root     video     81,  64 Jun 19 22:22 /dev/radio0
crw-rw----    1 root     video     81, 224 Jun 19 22:22 /dev/vbi0
crw-rw----    1 root     video     81, 228 Jun 19 22:22 /dev/vbi8
crw-rw----    1 root     video     81, 232 Jun 19 22:22 /dev/vbi16
 
 
Base devices
============
 
For every extra card you have the numbers increased by one. For example,
/dev/video0 is listed as the 'base' encoding capture device so we have:
모든 추가 카드에 대해 번호가 하나씩 증가합니다. 예를 들어,
/ dev / video0은 '기본'인코딩 캡쳐 장치로 나열되어 있으므로 다음과 같습니다.
 
 /dev/video0  is the encoding capture device for the first card (card 0)
		첫 번째 카드 (카드 0)의 인코딩 캡처 장치입니다.
 /dev/video1  is the encoding capture device for the second card (card 1)
		두 번째 카드 (카드 1)의 인코딩 캡처 장치입니다.
 /dev/video2  is the encoding capture device for the third card (card 2)
		세 번째 카드 (카드 2)의 인코딩 캡처 장치입니다.
 
Note that if the first card doesn't have a feature (eg no decoder, so no video16, the second card will still use video17. The simple rule is 'add the card number to the base device number'. If you have other capture cards (e.g. WinTV PCI) that are detected first, then you have to tell the ivtv module about it so that it will start counting at 1 (or 2, or whatever). Otherwise the device numbers can get confusing. The ivtv 'ivtv_first_minor' module option can be used for that.
첫 번째 카드에 기능이 없는 경우 (예 : 디코더가 없으므로 비디오 16이없는 경우 두 번째 카드는 여전히 비디오 17을 사용합니다. 간단한 규칙은 '기본 장치 번호에 카드 번호 추가'입니다.) 다른 캡처가 있는 경우 카드 (예 : WinTV PCI)가 감지되면 ivtv 모듈에 알려서 1 (또는 2 등)에서 계산을 시작해야합니다. 그렇지 않으면 장치 번호가 혼란 스러울 수 있습니다. ivtv 'ivtv_first_minor' 모듈 옵션을 사용할 수 있습니다.
 
 
/dev/video0
The encoding capture device(s). 	인코딩 캡쳐 장치.
Read-only.			읽기 전용.
 
Reading from this device gets you the MPEG1/2 program stream.
이 장치에서 읽으면 MPEG1 / 2 프로그램 스트림을 얻을 수 있습니다.
 
Example:		예:
cat /dev/video0 > my.mpg (you need to hit ctrl-c to exit)
			(종료하려면 ctrl-c를 누르십시오)
 
 
/dev/video16
The decoder output device(s)	디코더 출력 장치 (들)
Write-only. 			쓰기 전용. 
Only present if the MPEG decoder (i.e. CX23415) exists.
MPEG 디코더 (예 : CX23415)가있는 경우에만 표시됩니다.
 
An mpeg2 stream sent to this device will appear on the selected video display, audio will appear on the line-out/audio out.  It is only available for cards that support video out. 
이 장치로 보낸 mpeg2 스트림이 선택한 비디오 디스플레이에 나타나고 오디오가 라인 출력 / 오디오 출력에 나타납니다. 비디오 출력을 지원하는 카드에서만 사용할 수 있습니다. 
 
Example:		예:
cat my.mpg >/dev/video16
 
 
/dev/video24
The raw audio capture device(s).	raw 오디오 캡처 장치.
Read-only			읽기 전용.
 
The raw audio PCM stereo stream from the currently selected tuner or audio line-in.  Reading from this device results in a raw (signed 16 bit Little Endian, 48000 Hz, stereo pcm) capture.
This device only captures audio. This should be replaced by an ALSA device in the future.
Note that there is no corresponding raw audio output device, this is not supported in the decoder firmware.
현재 선택된 튜너 또는 오디오 라인 입력의 raw 오디오 PCM 스테레오 스트림입니다. 이 장치에서 읽으면 raw (부호가있는 16 비트 Little Endian, 48000 Hz, 스테레오 pcm) 캡처가 됩니다.
이 장치는 오디오 만 캡처합니다. 이것은 앞으로 ALSA 장치로 대체되어야 합니다.
해당 raw 오디오 출력 장치가 없다는 점에 유의하십시오. 이 장치는 디코더 펌웨어에서 지원되지 않습니다.
 
 
/dev/video32
The raw video capture device(s)	raw 비디오 캡처 장치 (들)
Read-only			읽기 전용
 
The raw YUV video output from the current video input. The YUV format is non-standard (V4L2_PIX_FMT_HM12).
현재 비디오 입력에서 생성 된 원시 YUV 비디오 출력. YUV 형식은 비표준 형식입니다 (V4L2_PIX_FMT_HM12)
 
Note that the YUV and PCM streams are not synchronized, so they are of limited use.
YUV와 PCM 스트림은 동기화되지 않으므로 제한적으로 사용됩니다.
 
 
/dev/video48
The raw video display device(s)	raw 비디오 디스플레이 장치 (들)
Write-only. 			쓰기 전용
Only present if the MPEG decoder (i.e. CX23415) exists.
MPEG 디코더 (예 : CX23415)가있는 경우에만 표시됩니다.
 
Writes a YUV stream to the decoder of the card.
YUV 스트림을 카드의 디코더에 씁니다.
 
 
/dev/radio0
The radio tuner device(s)		라디오 튜너 장치 (들)
Cannot be read or written.		읽거나 쓸 수 없습니다.
 
Used to enable the radio tuner and tune to a frequency. You cannot read or write audio streams with this device.  Once you use this device to tune the radio, use /dev/video24 to read the raw pcm stream
or /dev/video0 to get an mpeg2 stream with black video.
라디오 튜너를 활성화하고 주파수를 튜닝하는 데 사용됩니다. 이 장치로는 오디오 스트림을 읽거나 쓸 수 없습니다. 일단이 장치를 사용하여 라디오를 튜닝하면 / dev / video24를 사용하여 raw pcm 스트림을 읽습니다
또는 / dev / video0을 사용하여 검은 색 비디오가 있는 mpeg2 스트림을 가져옵니다.
 
 
/dev/vbi0
The 'vertical blank interval' (Teletext, CC, WSS etc) capture device(s) 
'수직 공백 간격'(텔레 텍스트, CC, WSS 등) 캡처 장치 
Read-only			읽기 전용
 
Captures the raw (or sliced) video data sent during the Vertical Blank Interval. This data is used to encode teletext, closed captions, VPS, widescreen signalling, electronic program guide information, and other services.
수직 블랭크 간격(Vertical Blank Interval) 동안 전송된 raw (또는 슬라이스) 비디오 데이터를 캡처합니다. 이 데이터는 teletext, closed captions, VPS, 와이드 스크린 신호, 전자 프로그램 가이드 정보 및 기타 서비스를 인코딩하는 데 사용됩니다.
 
 
/dev/vbi8
Processed vbi feedback device(s)	처리 된 vbi 피드백 장치
Read-only. 			읽기 전용
Only present if the MPEG decoder (i.e. CX23415) exists.
MPEG 디코더 (예 : CX23415)가있는 경우에만 표시됩니다.
 
The sliced VBI data embedded in an MPEG stream is reproduced on this device. So while playing back a recording on /dev/video16, you can read the embedded VBI data from /dev/vbi8.
MPEG 스트림에 내장된 sliced VBI 데이터가 이 장치에서 재생됩니다. 따라서 / dev / video16에서 레코딩을 재생하는 동안 / dev / vbi8에서 포함 된 VBI 데이터를 읽을 수 있습니다.
 
/dev/vbi16
The vbi 'display' device(s)		vbi '디스플레이'장치
Write-only. 			쓰기 전용
Only present if the MPEG decoder (i.e. CX23415) exists.
MPEG 디코더 (예 : CX23415)가있는 경우에만 표시됩니다.
 
Can be used to send sliced VBI data to the video-out connector.
슬라이스 된 VBI 데이터를 비디오 출력 커넥터로 보내는 데 사용할 수 있습니다.
 
---------------------------------
 
Hans Verkuil <hverkuil@xs4all.nl>
 
 
 
 
 

