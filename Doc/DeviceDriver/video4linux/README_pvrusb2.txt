$Id$
Mike Isely <isely@pobox.com>
 
                   		pvrusb2 driver
    	           		pvrusb2 드라이버
 
Background:  배경설명
 
  This driver is intended for the "Hauppauge WinTV PVR USB 2.0", which is a USB 2.0 hosted TV Tuner.  This driver is a work in progress.
Its history started with the reverse-engineering effort by Björn Danielsson <pvrusb2@dax.nu> whose web page can be found here:
이 드라이버는 USB 2.0 호스트 TV 튜너 인 "Hauppauge WinTV PVR USB 2.0"을위한 것입니다. 이 드라이버는 진행중인 작업입니다.
  Björn Danielsson <pvrusb2@dax.nu>의 리버스 엔지니어링 노력으로 시작된 이 웹페이지는 여기에서 찾을 수 있습니다.
 
	http://pvrusb2.dax.nu/
 
From there Aurelien Alleaume <slts@free.fr> began an effort to create a video4linux compatible driver.  I began with Aurelien's last known snapshot and evolved the driver to the state it is in here.
거기에서 Aurelien Alleaume <slts@free.fr>은 video4linux 호환 드라이버를 만들기 시작했습니다. 나는 Aurelien의 마지막 스냅샷 으로 시작하여 드라이버가 여기에 있는 상태로 진화했습니다.
 
More information on this driver can be found at:
이 드라이버에 대한 자세한 내용은 다음에서 찾을 수 있습니다.
 
    http://www.isely.net/pvrusb2.html
 
 
This driver has a strong separation of layers.  They are very roughly:
 이 드라이버는 레이어를 강하게 구분합니다. 그들은 매우 대략적입니다 :
 
1a. Low level wire-protocol implementation with the device.
  	낮은 수준의 와이어 프로토콜 구현.
 
1b. I2C adaptor implementation and corresponding I2C client drivers implemented elsewhere in V4L.
I2C 어댑터 구현 및 해당 I2C 클라이언트 드라이버는 V4L의 다른 곳에서 구현됩니다.
 
1c. High level hardware driver implementation which coordinates all activities that ensure correct operation of the device.
  	장치의 올바른 작동을 보장하는 모든 활동을 조정하는 고급 하드웨어 드라이버 구현.
 
2.  A "context" layer which manages instancing of driver, setup, tear-down, arbitration, and interaction with high level interfaces appropriately as devices are hotplugged in the system.
시스템에서 장치가 hotplugged 될 때 드라이버, 셋업, tear-down, arbitration 및 고수준 인터페이스와의 상호작용을 적절하게 관리하는 "컨텍스트"계층.
 
3.  High level interfaces which glue the driver to various published Linux APIs (V4L, sysfs, maybe DVB in the future).
다양한 공개 된 Linux API (V4L, sysfs, 아마도 향후 DVB)에 드라이버를 연결하는 고급 인터페이스.
 
The most important shearing layer is between the top 2 layers.  A lot of work went into the driver to ensure that any kind of conceivable API can be laid on top of the core driver.  (Yes, the driver internally leverages V4L to do its work but that really has nothing to do with the API published by the driver to the outside world.)  The architecture allows for different APIs to simultaneously access the driver.  I have a strong sense of fairness about APIs and also feel that it is a good design principle to keep implementation and interface isolated from each other.  Thus while right now the V4L high level interface is the most complete, the sysfs high level interface will work equally well for similar functions, and there's no reason I see right now why it shouldn't be possible to produce a DVB high level interface that can sit right alongside V4L.
가장 중요한 shearing계층은 상위 2개 층 사이에 있습니다. 어떤 종류의 상상할 수 있는 API라도 핵심 드라이버 위에 놓을 수 있도록 많은 노력이 드라이버에 포함되었습니다. (예, 드라이버는 내부적으로 V4L을 활용하여 작업을 수행하지만 실제로 드라이버는 외부 세계에 공개 된 API와 아무런 관련이 없습니다.) 아키텍처를 사용하면 서로 다른 API가 드라이버에 동시에 액세스 할 수 있습니다. 나는 API에 대한 공정성에 대해 강한 인식을 가지고 있으며, 구현과 인터페이스를 서로 고립시켜 두는 것이 좋은 설계 원리라고 생각한다. 따라서 현재 V4L 고수준 인터페이스가 가장 완벽하지만 sysfs 고수준 인터페이스는 비슷한 기능을 위해 똑같이 잘 작동 할 것입니다. 왜 DVB 고수준 인터페이스를 생성 할 수 없는지 지금 당장 볼 이유가 없습니다 V4L과 나란히 앉을 수 있습니다.
 
  NOTE: Complete documentation on the pvrusb2 driver is contained in the html files within the doc directory; these are exactly the same as what is on the web site at the time.  Browse those files (especially the FAQ) before asking questions.
참고 : pvrusb2 드라이버의 전체 설명서는 doc 디렉토리의 html 파일에 있습니다. 이것들은 당시의 웹 사이트에 있는 것과 정확히 동일합니다. 질문하기 전에 해당 파일 (특히 FAQ)을 찾아보십시오.
 
Building
 
To build these modules essentially amounts to just running "Make", but you need the kernel source tree nearby and you will likely also want to set a few controlling environment variables first in order to link things up with that source tree.  Please see the Makefile here for comments that explain how to do that.
이 모듈을 빌드하려면 근본적으로 "Make"를 실행하는데 그치지만, 근처에 커널소스트리가 있어야 하며 몇 가지 제어 환경 변수를 먼저 설정하여 해당 소스트리와 연결할 수 있습니다. 그 방법을 설명하는 코멘트는 여기에있는 Makefile을보십시오.
 
 
Source file list / functional overview:          	소스 파일 목록 / 기능 개요 :
 
(Note: The term "module" used below generally refers to loosely defined functional units within the pvrusb2 driver and bears no relation to the Linux kernel's concept of a loadable module.)
 (참고 : 아래에서 사용되는 "모듈"이라는 용어는 일반적으로 pvrusb2 드라이버에서 느슨하게 정의 된 기능 단위를 의미하며 Linux 커널의 로드 가능한 모듈 개념과는 관련이 없습니다.)
 
pvrusb2-audio.[ch]
- This is glue logic that resides between this driver and the msp3400.ko I2C client driver (which is found elsewhere in V4L).
이것은 이 드라이버와 msp3400.ko ​​I2C 클라이언트 드라이버 (V4L의 다른 곳에서 찾을 수 있음) 사이에있는 glue logic 입니다.
(Glue logic (글루로직): 커스텀로직회로, 사용자 요구에 맞게 커스터마이징 한 회로)
 
pvrusb2-context.[ch] - This module implements the context for an instance of the driver.  Everything else eventually ties back to or is otherwise instanced within the data structures implemented  here.  Hotplugging is ultimately coordinated here.  All high level interfaces tie into the driver through this module.  This module helps arbitrate each interface's access to the actual driver core, and is designed to allow concurrent access through multiple instances of multiple interfaces (thus you can for example change the tuner's frequency through sysfs while simultaneously streaming video through V4L out to an instance of mplayer).
이 모듈은 드라이버 인스턴스에 대한 컨텍스트를 구현합니다. 다른 모든 것은 결국 여기에 구현된 데이터 구조로 돌아가거나 그렇지 않으면 인스턴스화됩니다. Hotplugging(새로운 장치를 연결했을 때 바로 인식되어 사용할 수 있도록 해 주는 기능) 는 궁극적으로 여기서 조정됩니다. 모든 고급 인터페이스는 이 모듈을 통해 드라이버에 연결됩니다. 이 모듈은 실제 드라이버 코어에 대한 각 인터페이스의 액세스를 중재하는 데 도움이되며 여러 인터페이스의 여러 인스턴스를 통해 동시 액세스를 허용하도록 설계되었습니다 (예를 들어 sysfs를 통해 튜너의 빈도를 변경하는 동시에 V4L을 통해 비디오를 mplayer 인스턴스로 스트리밍 할 수 있음) ).
 
pvrusb2-debug.h - This header defines a printk() wrapper and a mask of debugging bit definitions for the various kinds of debug messages that can be enabled within the driver.
이 헤더는 드라이버 내에서 활성화 할 수 있는 다양한 종류의 디버그 메시지에 대한 printk() wrapper 및 디버깅bit 정의 마스크를 정의합니다.
 
pvrusb2-debugifc.[ch] - This module implements a crude command line oriented debug interface into the driver.  Aside from being part of the process for implementing manual firmware extraction (see the pvrusb2 web site mentioned earlier), probably I'm the only one who has ever used this.  It is mainly a debugging aid.
이 모듈은 드라이버에 익숙한 명령 행 지향 디버그 인터페이스를 구현합니다. 수동 펌웨어 추출을 구현하는 프로세스 (앞에서 언급 한 pvrusb2 웹 사이트 참조)의 일부분을 제외하고, 아마 나는 이것을 사용한 적이 있는 유일한 사람 일 것입니다. 주로 디버깅을 돕는 도구입니다.
pvrusb2-eeprom.[ch] - This is glue logic that resides between this driver the tveeprom.ko module, which is itself implemented elsewhere in V4L.
이것은 이 드라이버와 tveeprom.ko 모듈 사이에 있는 glue logic(글루로직: 사용자요구에 맞춰서 만든 커스터마이징 로직 회로) 입니다.이 모듈은 V4L의 다른 곳에서 구현됩니다.
 
pvrusb2-encoder.[ch] - This module implements all protocol needed to interact with the Conexant mpeg2 encoder chip within the pvrusb2 device.  It is a crude echo of corresponding logic in ivtv, however the design goals (strict isolation) and physical layer (proxy through USB instead of PCI) are enough different that this implementation had to be completely different.
이 모듈은 pvrusb2 장치 내에서 Conexant mpeg2 인코더 칩과 상호 작용하는 데 필요한 모든 프로토콜을 구현합니다. ivtv에서 해당 논리의 조잡한 반향이지만 설계 목표 (엄격한 격리)와 물리적 계층 (PCI 대신 USB를 통한 프록시)은이 구현이 완전히 달라야 한다는 점에서 충분히 다릅니다.
 
pvrusb2-hdw-internal.h - This header defines the core data structure in the driver used to track ALL internal state related to control of the hardware.  Nobody outside of the core hardware-handling modules should have any business using this header.  All external access to the driver should be through one of the high level interfaces (e.g. V4L, sysfs, etc), and in fact even those high level interfaces are restricted to the API defined in pvrusb2-hdw.h and NOT this header.
이 헤더는 하드웨어 제어와 관련된 모든 내부 상태를 추적하는 데 사용되는 드라이버의 코어 데이터 구조를 정의합니다. 핵심 하드웨어 처리 모듈 외부의 어느 누구도 이 머리글을 사용하는 비즈니스를 가져서는 안됩니다. 드라이버에 대한 모든 외부 액세스는 상위 레벨 인터페이스 (예 : V4L, sysfs 등) 중 하나를 통해 이루어져야 하며 실제로 이러한 고급 인터페이스는 pvrusb2-hdw.h에 정의 된 API로 제한되며 이 헤더가 아닙니다.
 
pvrusb2-hdw.h - This header defines the full internal API for controlling the hardware.  High level interfaces (e.g. V4L, sysfs) will work through here.
이 헤더는 하드웨어를 제어하기 위한 전체 내부 API를 정의합니다. 상위 레벨 인터페이스 (예 : V4L, sysfs)가 여기에서 작동합니다.
 
pvrusb2-hdw.c - This module implements all the various bits of logic that handle overall control of a specific pvrusb2 device. (Policy, instantiation, and arbitration of pvrusb2 devices fall within the jurisdiction of pvrusb-context not here).
이 모듈은 특정 pvrusb2 장치의 전반적인 제어를 처리하는 다양한 논리 비트를 구현합니다. pvrusb2 장치의 정책, 인스턴스화 및 중재는 여기에 없는 pvrusb- 컨텍스트의 관할권 내에 있습니다.
 
pvrusb2-i2c-chips-*.c - These modules implement the glue logic to tie together and configure various I2C modules as they attach to the I2C bus.  There are two versions of this file.  The "v4l2" version is intended to be used in-tree alongside V4L, where we implement just the logic that makes sense for a pure V4L environment.  The "all" version is intended for use outside of V4L, where we might encounter other possibly "challenging" modules from ivtv or older kernel snapshots (or even the support modules in the standalone snapshot).
이 모듈은 서로 묶어서 I2C 버스에 부착 할 때 다양한 I2C 모듈을 구성하도록 글루 로직 (글루로직이 뭔지 모르면 검색해서 다시 볼것..)을 구현합니다. 이 파일에는 두 가지 버전이 있습니다. "v4l2"버전은 순수 V4L 환경에 적합한 로직을 구현하는 V4L과 함께 트리 내에서 사용하기 위한 버전입니다. "all"버전은 V4L 외부에서 사용하기 위한 것이며 ivtv 또는 이전 커널 스냅 샷 (또는 독립형 스냅 샷의 지원 모듈)에서 "도전적인"모듈을 만날 수도 있습니다.
 
pvrusb2-i2c-cmd-v4l1.[ch] - This module implements generic V4L1 compatible commands to the I2C modules.  It is here where state changes inside the pvrusb2 driver are translated into V4L1 commands that are in turn send to the various I2C modules.
이 모듈은 I2C 모듈에 대한 일반적인 V4L1 호환 명령을 구현합니다. 여기서 pvrusb2 드라이버의 상태 변경이 V4L1 명령으로 변환되어 다양한 I2C 모듈로 전송됩니다.
 
pvrusb2-i2c-cmd-v4l2.[ch] - This module implements generic V4L2 compatible commands to the I2C modules.  It is here where state changes inside the pvrusb2 driver are translated into V4L2 commands that are in turn send to the various I2C modules.
이 모듈은 I2C 모듈에 대한 일반적인 V4L2 호환 명령을 구현합니다. 여기서 pvrusb2 드라이버의 상태 변경이 V4L2 명령으로 변환되어 다양한 I2C 모듈로 전송됩니다.
 
pvrusb2-i2c-core.[ch] - This module provides an implementation of a kernel-friendly I2C adaptor driver, through which other external I2C client drivers (e.g. msp3400, tuner, lirc) may connect and operate corresponding chips within the pvrusb2 device.  It is through here that other V4L modules can reach into this driver to operate specific pieces (and those modules are in turn driven by glue logic which is coordinated by pvrusb2-hdw, doled out by pvrusb2-context, and then ultimately made available to users through one of the high level interfaces).
이 모듈은 다른 외부 I2C 클라이언트 드라이버 (예 : msp3400, 튜너, lirc)가 pvrusb2 장치 내에서 해당 칩을 연결하고 작동시킬 수 있는 커널친화적 인 I2C 어댑터 드라이버 구현을 제공합니다. 여기에서 다른 V4L 모듈이 이 드라이버에 연결되어 특정 조각을 조작 할 수 있습니다. (그리고 이러한 모듈은 pvrusb2-hdw에 의해 조정되고 pvrusb2- 컨텍스트에 의해 조정 된 후 최종적으로 사용자가 사용할 수 있게하는 글루 로직에 의해 구동됩니다 높은 수준의 인터페이스 중 하나를 통해).
 
pvrusb2-io.[ch] - This module implements a very low level ring of transfer buffers, required in order to stream data from the device.  This module is *very* low level.  It only operates the buffers and makes no attempt to define any policy or mechanism for how such buffers might be used.
이 모듈은 장치에서 데이터를 스트리밍하기 위해 필요한 매우 낮은 수준의 전송 버퍼 링을 구현합니다. 이 모듈은 매우 낮음 * 수준입니다. 버퍼 만 조작하고 그러한 버퍼를 사용하는 방법에 대한 정책이나 메커니즘을 정의하지 않습니다.
 
pvrusb2-ioread.[ch] - This module layers on top of pvrusb2-io.[ch] to provide a streaming API usable by a read() system call style of I/O.  Right now this is the only layer on top of pvrusb2-io.[ch], however the underlying architecture here was intended to allow for other styles of I/O to be implemented with additional modules, like mmap()'ed buffers or something even more exotic.
이 모듈은 pvrusb2-io. [ch] 위에 레이어되어 I / O의 read () 시스템 호출 스타일에서 사용할 수 있는 스트리밍 API를 제공합니다. 지금 이것은 pvrusb2-io. [ch] 의 유일한 레이어입니다. 여기에 기본 아키텍처는 다른 스타일의 I / O를 mmap ()의 버퍼 같은 것 같은 추가 모듈로 구현할 수 있게 하기 위한 것입니다. 훨씬 더 이색.
 
pvrusb2-main.c - This is the top level of the driver.  Module level and USB core entry points are here.  This is our "main".
이것은 드라이버의 최상위 레벨입니다. 모듈 레벨 및 USB 코어 엔트리 포인트는 여기에 있습니다. 이것이 우리의 "기본"입니다.
 
pvrusb2-sysfs.[ch] - This is the high level interface which ties the pvrusb2 driver into sysfs.  Through this interface you can do everything with the driver except actually stream data.
이것은 pvrusb2 드라이버를 sysfs에 묶는 고급 인터페이스입니다. 이 인터페이스를 통해 실제로는 스트림 데이터를 제외하고 드라이버로 모든 것을 할 수 있습니다.
 
pvrusb2-tuner.[ch] - This is glue logic that resides between this driver and the tuner.ko I2C client driver (which is found elsewhere in V4L).
이것은이 드라이버와 tuner.ko I2C 클라이언트 드라이버 (V4L의 다른 곳에서 찾을 수 있음) 사이에 있는 글루 로직입니다.
 
pvrusb2-util.h - This header defines some common macros used throughout the driver.  These macros are not really specific to the driver, but they had to go somewhere.
이 헤더는 드라이버 전체에서 사용되는 몇 가지 공통 매크로를 정의합니다. 이 매크로는 드라이버에만 국한된 것은 아니지만 어딘가에 있어야 했습니다.
 
pvrusb2-v4l2.[ch] - This is the high level interface which ties the pvrusb2 driver into video4linux.  It is through here that V4L applications can open and operate the driver in the usual V4L ways.  Note that **ALL** V4L functionality is published only through here and nowhere else.
pvrusb2 드라이버를 video4linux에 연결하는 상위 인터페이스입니다. 여기서 V4L 애플리케이션은 일반적인 V4L 방식으로 드라이버를 열고 조작 할 수 있습니다. ** ALL ** V4L 기능은 여기와 다른 곳을 통해서만 게시됩니다.
 
pvrusb2-video-*.[ch] - This is glue logic that resides between this driver and the saa711x.ko I2C client driver (which is found elsewhere in V4L).  Note that saa711x.ko used to be known as saa7115.ko in ivtv.  There are two versions of this; one is selected depending on the particular saa711[5x].ko that is found.
이것은 이 드라이버와 sa611x.ko I2C 클라이언트 드라이버 (V4L의 다른 곳에서 찾을 수 있음) 사이에있는 글루 로직입니다. saa711x.ko는 ivtv에서 saa7115.ko로 알려져 있었습니다. 이 두 가지 버전이 있습니다. 하나는 발견 된 특정 saa711 [5x] .ko에 따라 선택됩니다.
 
pvrusb2.h - This header contains compile time tunable parameters (and at the moment the driver has very little that needs to be tuned).
이 헤더에는 컴파일 타임 tunable 매개 변수가 포함됩니다 (드라이버가 튜닝 할 필요가 거의 없습니다).
 
 
  -Mike Isely
  isely@pobox.com
 
 
 

