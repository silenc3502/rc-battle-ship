                VPBE V4L2 driver design
 	   VPBE V4L2 드라이버 설계
 ==================================================================
 
 File partitioning        	파일 파티셔닝
 -----------------
 V4L2 display device driver   V4L2 디스플레이 장치 드라이버
         drivers/media/platform/davinci/vpbe_display.c
     	drivers/media/platform/davinci/vpbe_display.h
 
 VPBE display controller  	 VPBE 디스플레이 컨트롤러
         drivers/media/platform/davinci/vpbe.c
         drivers/media/platform/davinci/vpbe.h
 
 VPBE venc sub device driver   VPBE venc 하위 장치 드라이버
         drivers/media/platform/davinci/vpbe_venc.c
     	drivers/media/platform/davinci/vpbe_venc.h
         drivers/media/platform/davinci/vpbe_venc_regs.h
 
 VPBE osd driver          	VPBE osd 드라이버
         drivers/media/platform/davinci/vpbe_osd.c
         drivers/media/platform/davinci/vpbe_osd.h
         drivers/media/platform/davinci/vpbe_osd_regs.h
 
 Functional partitioning  	기능적 파티셔닝
 -----------------------
 
 Consists of the following (in the same order as the list under file  partitioning):-
다음으로 구성됩니다 (파일 파티셔닝 목록과 동일한 순서로) :-
 
 1. V4L2 display driver           	디스플레이 드라이버
	Implements creation of video2 and video3 device nodes and provides v4l2 device interface to manage VID0 and VID1 layers.
    	video2 및 video3 장치 노드 생성을 구현하고 VID0 및 VID1 계층을 관리하기 위한 v4l2 장치 인터페이스를 제공합니다.
 
 2. Display controller    	디스플레이 컨트롤러
	Loads up VENC, OSD and external encoders such as ths8200. It provides a set of API calls to V4L2 drivers to set the output/standards in the VENC or external sub devices. It also provides a device object to access the services from OSD subdevice using sub device ops. The connection of external encoders to VENC LCD controller port is done at init time based on default output and standard selection or at run time when application change the output through V4L2 IOCTLs.
  VENC, OSD 및 ths8200과 같은 외부 인코더를 로드합니다. V4L2 드라이버에 대한 일련의 API 호출을 제공하여 VENC 또는 외부 하위 장치의 출력 / 표준을 설정합니다. 또한 하위 장치 작업을 사용하여 OSD 하위 장치에서 서비스에 액세스하는 장치 개체를 제공합니다. 외부 인코더와 VENC LCD 컨트롤러 포트의 연결은 기본 출력 및 표준 선택을 기반으로 하는 초기화시 또는 애플리케이션이 V4L2 IOCTL을 통해 출력을 변경할 때 런타임에 수행됩니다.
 
	When connected to an external encoder, vpbe controller is also responsible for setting up the interface between VENC and external encoders based on board specific settings (specified in board-xxx-evm.c). This allows interfacing external encoders such as ths8200. The setup_if_config() is implemented for this as well as configure_venc() (part of the next patch)  API to set timings in VENC for a specific display resolution. As of this patch series, the interconnection and enabling and setting of the external encoders is not present, and would be a part of the next patch series.
    	외부 인코더에 연결되면 vpbe 컨트롤러는 보드 특정 설정 (board-xxx-evm.c에 지정)에 따라 VENC와 외부 인코더 사이의 인터페이스를 설정해야 합니다. 이를 통해 ths8200과 같은 외부 인코더를 인터페이싱 할 수 있습니다. setup_if_config ()는 특정 디스플레이 해상도에 대해 VENC에서 타이밍을 설정하기 위해 configure_venc () (다음 패치의 일부) API와 함께 구현됩니다. 이 패치 시리즈에서, 외부 인코더의 상호 연결 및 활성화 및 설정은 존재하지 않으며 다음 패치 시리즈의 일부가 됩니다.
 
 3. VENC subdevice module 	하위(서브)장치 모듈
	Responsible for setting outputs provided through internal DACs and also setting timings at LCD controller port when external encoders are connected at the port or LCD panel timings required. When external encoder/LCD panel is connected, the timings for a specific standard/preset is retrieved from the board specific table and the values are used to set the timings in venc using non-standard timing mode.
    	내부 DAC를 통해 제공되는 출력을 설정하고 외부 인코더가 필요한 포트 또는 LCD 패널 타이밍에 연결된 경우 LCD 컨트롤러 포트에서 타이밍을 설정합니다. 외부 인코더 / LCD 패널이 연결되면 특정 표준 / 사전 설정의 타이밍이 보드 특정 테이블에서 검색되고 그 값은 비표준 타이밍 모드를 사용하여 venc의 타이밍을 설정하는 데 사용됩니다.
 
	Support LCD Panel displays using the VENC. For example to support a Logic PD display, it requires setting up the LCD controller port with a set of timings for the resolution supported and setting the dot clock. So we could add the available outputs as a board specific entry (i.e add the "LogicPD" output name to board-xxx-evm.c). A table of timings for various LCDs supported can be maintained in the board specific setup file to support various LCD displays.As of this patch a basic driver is present, and this support for external encoders and displays forms a part of the next patch series.
    	VENC를 사용하여 LCD 패널 디스플레이를 지원하십시오. 예를 들어, Logic PD 디스플레이를 지원하려면 지원되는 해상도에 대한 타이밍 세트와 dot clock 설정으로(점들로 디지털시계를 만드는 듯한 이미지를 떠올리면 된다) LCD 컨트롤러 포트를 설정해야 합니다. 따라서 사용 가능한 출력을 보드 특정 항목으로 추가 할 수 있습니다 (예 : "LogicPD"출력 이름을 board-xxx-evm.c에 추가). 지원되는 다양한 LCD에 대한 타이밍 표는 다양한 LCD 디스플레이를 지원하기 위해 보드 특정 설정 파일에서 유지 관리 할 수 ​​있습니다.이 패치에는 기본 드라이버가 있으며 외부 인코더 및 디스플레이에 대한 지원은 다음 패치 시리즈의 일부를 형성합니다.
 
 4. OSD module
	OSD module implements all OSD layer management and hardware specific features. The VPBE module interacts with the OSD for enabling and disabling appropriate features of the OSD.
 OSD 모듈은 모든 OSD 레이어 관리 및 하드웨어 관련 기능을 구현합니다. VPBE 모듈은 OSD와 상호 작용하여 OSD의 적절한 기능을 활성화 및 비활성화합니다.
 
 Current status:-
현재 상태 :-
 
 A fully functional working version of the V4L2 driver is available. This driver has been tested with NTSC and PAL standards and buffer streaming.
V4L2 드라이버의 모든 기능을 갖춘 작동 버전을 사용할 수 있습니다. 이 드라이버는 NTSC 및 PAL 표준 및 버퍼 스트리밍으로 테스트되었습니다.
 
 Following are TBDs.
 
 vpbe display controller          	 vpbe 디스플레이 컨트롤러
	- Add support for external encoders.
    	외부 인코더에 대한 지원을 추가하십시오.
	- add support for selecting external encoder as default at probe time.
    	프로브 타임에 외부 인코더를 기본값으로 선택하도록 지원합니다.
 
 vpbe venc sub device
	- add timings for supporting ths8200
    	ths8200을 지원하기위한 타이밍 추가
	- add support for LogicPD LCD.
LogicPD LCD에 대한 지원을 추가하십시오.
 
 FB drivers
	- Add support for fbdev drivers.- Ready and part of subsequent patches.
    	fbdev 드라이버에 대한 지원을 추가하십시오. - 후속 패치의 준비 및 일부.
 
 

