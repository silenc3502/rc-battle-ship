infrared remote control support in video4linux drivers
video4linux 드라이버의 적외선 원격 제어 지원
======================================================
 
 
basics
------
Current versions use the linux input layer to support infrared remote controls.  I suggest to download my input layer tools from http://bytesex.org/snapshot/input-<date>.tar.gz
현재 버전에서는 적외선 리모콘을 지원하기 위해 Linux 입력 레이어를 사용합니다. http://bytesex.org/snapshot/input-<date>.tar.gz 에서 입력 도구를 다운로드 하는 것이 좋습니다.
 
Modules you have to load:
로드해야하는 모듈 :
 
saa7134  	statically built in, i.e. just the driver :)
         	정적으로 내장 된 saa7134, 즉 드라이버 만 :)
bttv     	ir-kbd-gpio or ir-kbd-i2c depending on your card.
         	bttv ir-kbd-gpio 또는 ir-kbd-i2c에 따라 다릅니다.
 
ir-kbd-gpio and ir-kbd-i2c don't support all cards lirc supports (yet), mainly for the reason that the code of lirc_i2c and lirc_gpio was very confusing and I decided to basically start over from scratch.
ir-kbd-gpio 및 ir-kbd-i2c는 lirc_i2c 및 lirc_gpio 코드가 매우 혼란스럽고 기본적으로 처음부터 다시 시작하기로 결정한 이유로 lirc가 지원하는 모든 카드 (아직)를 지원하지 않습니다.
Feel free to contact me in case of trouble.  Note that the ir-kbd-*modules work on 2.6.x kernels only through ...
문제가 생기면 저에게 연락하십시오. ir-kbd- * 모듈은 2.6.x 커널에서만 작동합니다 ...
 
 
how it works       	어떻게 작동 하는가?
------------
 
The modules register the remote as keyboard within the linux input layer, i.e. you'll see the keys of the remote as normal key strokes (if CONFIG_INPUT_KEYBOARD is enabled).
모듈은 리모컨을 리눅스 입력레이어 내에서 키보드로 등록합니다. 즉, 리모컨의 키가 일반적인 키-스트로크 (CONFIG_INPUT_KEYBOARD가 활성화 된 경우)로 표시됩니다.
Using the event devices (CONFIG_INPUT_EVDEV) it is possible for applications to access the remote via /dev/input/event<n> devices.
이벤트 장치 (CONFIG_INPUT_EVDEV)를 사용하면 응용 프로그램이 / dev / input / event <n> 장치를 통해 원격에 액세스 할 수 있습니다.
You might have to create the special files using "/sbin/MAKEDEV input".  The input layer tools mentioned above use the event device.
"/ sbin / MAKEDEV input"을 사용하여 특수 파일을 만들어야 할 수도 있습니다. 위에서 언급한 입력 레이어 도구는 이벤트 장치를 사용합니다.
 
The input layer tools are nice for trouble shooting, i.e. to check whenever the input device is really present, which of the devices it is, check whenever pressing keys on the remote actually generates events and the like.  You can also use the kbd utility to change the keymaps (2.6.x kernels only through).
입력 레이어 도구는 문제 해결, 즉 입력 장치가 실제로 존재할 때마다, 어떤 장치인지를 확인하고, 리모컨의 키를 누를 때마다 실제로 이벤트가 생성되는지 확인할 때 유용합니다. kbd 유틸리티를 사용하여 키맵 (2.6.x 커널 만 통과)을 변경할 수도 있습니다.
 
 
using with lircd          	lircd와 함께 사용
================
The cvs version of the lircd daemon supports reading events from the linux input layer (via event device).  The input layer tools tarball comes with a lircd config file.
lircd 데몬의 cvs 버전은 (이벤트 장치를 통해) Linux 입력 레이어에서 이벤트를 읽는 것을 지원합니다. 입력 레이어 툴인 tarball은 lircd 설정 파일과 함께 제공됩니다.
 
using without lircd       	lircd없이 사용하기
===================
 
XFree86 likely can be configured to recognise the remote keys.  Once I simply tried to configure one of the multimedia keyboards as input device, which had the effect that XFree86 recognised some of the keys of my remote control and passed volume up/down key presses as XF86AudioRaiseVolume and XF86AudioLowerVolume key events to the X11 clients.
XFree86은 원격 키를 인식하도록 구성 될 수 있습니다. XFree86이 리모콘의 일부 키를 인식하고 XF86AudioRaiseVolume 및 XF86AudioLowerVolume 키 이벤트를 볼륨 업 / 다운 키 프레스로 X11 클라이언트에 전달한 결과, 멀티미디어 키보드 중 하나를 입력 장치로 구성하기 만 하면 됩니다.
It likely is possible to make that fly with a nice xkb config file,
I know next to nothing about that through.
 
Have fun,
 Gerd
 
--
Gerd Knorr <kraxel@bytesex.org>
 
 

