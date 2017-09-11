Intel(R) PRO/Wireless 2915ABG Driver for Linux in support of:

Intel(R) PRO/Wireless 2200BG Network Connection
Intel(R) PRO/Wireless 2915ABG Network Connection

Note: The Intel(R) PRO/Wireless 2915ABG Driver for Linux and Intel(R)
PRO/Wireless 2200BG Driver for Linux is a unified driver that works on
both hardware adapters listed above. In this document the Intel(R)
PRO/Wireless 2915ABG Driver for Linux will be used to reference the
unified driver.
참고 : Linux 용 인텔 (R) PRO / 무선 2915ABG 드라이버 및 Linux 용 인텔 (R) PRO / 무선 2200BG 드라이버는 위에 나열된 하드웨어 어댑터 모두에서 작동하는 통합 드라이버입니다. 이 문서에서는 Linux 용 인텔 (R) PRO / 무선 2915ABG 드라이버가 통합 드라이버를 참조하는 데 사용됩니다.

Copyright (C) 2004-2006, Intel Corporation

README.ipw2200

Version: 1.1.2
Date   : March 30, 2006


Index
-----------------------------------------------
0.   IMPORTANT INFORMATION BEFORE USING THIS DRIVER
1.   Introduction
1.1. Overview of features
1.2. Module parameters
1.3. Wireless Extension Private Methods
1.4. Sysfs Helper Files
1.5. Supported channels
2.   Ad-Hoc Networking
3.   Interacting with Wireless Tools
3.1. iwconfig mode
3.2. iwconfig sens
4.   About the Version Numbers
5.   Firmware installation
6.   Support
7.   License


0.   IMPORTANT INFORMATION BEFORE USING THIS DRIVER
-----------------------------------------------

Important Notice FOR ALL USERS OR DISTRIBUTORS!!!! 
모든 사용자 또는 대리점을위한 중요 공지 !!!!

Intel wireless LAN adapters are engineered, manufactured, tested, and
quality checked to ensure that they meet all necessary local and
governmental regulatory agency requirements for the regions that they
are designated and/or marked to ship into. Since wireless LANs are
generally unlicensed devices that share spectrum with radars,
satellites, and other licensed and unlicensed devices, it is sometimes
necessary to dynamically detect, avoid, and limit usage to avoid
interference with these devices. In many instances Intel is required to
provide test data to prove regional and local compliance to regional and
governmental regulations before certification or approval to use the
product is granted. Intel's wireless LAN's EEPROM, firmware, and
software driver are designed to carefully control parameters that affect
radio operation and to ensure electromagnetic compliance (EMC). These
parameters include, without limitation, RF power, spectrum usage,
channel scanning, and human exposure. 
인텔 무선 LAN 어댑터는 설계 및 제조, 테스트 및 품질 검사를 거쳐 해당 지역 및 정부 규제 기관의 요구 사항을 충족 시키거나 배송하도록 지정되었습니다. 무선 LAN은 일반적으로 레이더, 위성 및 기타 허가 및 비인가 장치와 스펙트럼을 공유하는 비인가 장치이기 때문에 이러한 장치의 간섭을 피하기 위해 사용을 동적으로 감지, 회피 및 제한해야 할 때가 있습니다. 대부분의 경우 인텔은 제품 사용에 대한 인증 또는 승인이 부여되기 전에 지역 및 정부 규정을 지역 및 지역 규정 준수를 입증하기위한 테스트 데이터를 제공해야합니다. 인텔 무선 LAN의 EEPROM, 펌웨어 및 소프트웨어 드라이버는 무선 작동에 영향을 미치는 매개 변수를 신중하게 제어하고 EMC (전자기 적합성)를 보장하도록 설계되었습니다. 이러한 매개 변수에는 RF 전력, 스펙트럼 사용, 채널 스캐닝 및 사람의 노출이 포함 되나 이에 국한되지 않습니다.

For these reasons Intel cannot permit any manipulation by third parties
of the software provided in binary format with the wireless WLAN
adapters (e.g., the EEPROM and firmware). Furthermore, if you use any
patches, utilities, or code with the Intel wireless LAN adapters that
have been manipulated by an unauthorized party (i.e., patches,
utilities, or code (including open source code modifications) which have
not been validated by Intel), (i) you will be solely responsible for
ensuring the regulatory compliance of the products, (ii) Intel will bear
no liability, under any theory of liability for any issues associated
with the modified products, including without limitation, claims under
the warranty and/or issues arising from regulatory non-compliance, and
(iii) Intel will not provide or be required to assist in providing
support to any third parties for such modified products.  
이러한 이유로 인텔은 무선 WLAN 어댑터 (i.e. , EEPROM 및 펌웨어)와 함께 바이너리 형식으로 제공되는 소프트웨어를 타사에서 조작 할 수 없습니다. 또한 인텔이 검증하지 않은 패치, 유틸리티 또는 코드 (오픈 소스 코드 수정 포함)에 의해 조작 된 인텔 무선 LAN 어댑터와 함께 패치, 유틸리티 또는 코드를 사용하는 경우, (i) 귀하는 제품의 규정 준수를 보증 할 책임이 전적으로 귀하에게 있으며, (ii) 수정 된 제품과 관련된 모든 문제에 대한 이론에 대해 책임을지지 않습니다. / 또는 (iii) 인텔은 그러한 수정 된 제품에 대해 제 3 자에게 지원을 제공하지 않거나 지원하도록 요구하지 않을 것입니다.

Note: Many regulatory agencies consider Wireless LAN adapters to be
modules, and accordingly, condition system-level regulatory approval
upon receipt and review of test data documenting that the antennas and
system configuration do not cause the EMC and radio operation to be
non-compliant.
참고 : 많은 규제 기관에서는 무선 LAN 어댑터를 모듈로 간주하므로 안테나 및 시스템 구성으로 인해 EMC 및 무선 통신이 규격을 준수하지 않는 것으로 간주되는 테스트 데이터를 수신 및 검토 할 때 시스템 수준의 규제 승인을 받아야합니다.

The drivers available for download from SourceForge are provided as a 
part of a development project.  Conformance to local regulatory 
requirements is the responsibility of the individual developer.  As 
such, if you are interested in deploying or shipping a driver as part of 
solution intended to be used for purposes other than development, please 
obtain a tested driver from Intel Customer Support at:
SourceForge에서 다운로드 할 수있는 드라이버는 개발 프로젝트의 일부로 제공됩니다. 현지 규정 요구 사항을 준수하는 것은 개개 개발자의 책임입니다. 따라서 개발 이외의 목적으로 사용되도록 의도 된 솔루션의 일부로 드라이버를 배포하거나 출하하는 데 관심이있는 경우 인텔 고객 지원부에서 테스트를 마친 드라이버를 구하십시오.

http://support.intel.com


1.   Introduction
-----------------------------------------------
The following sections attempt to provide a brief introduction to using 
the Intel(R) PRO/Wireless 2915ABG Driver for Linux.
다음 절에서는 Linux 용 Intel (R) PRO / 무선 2915ABG 드라이버 사용에 대한 간단한 소개를 제공하려고합니다.

This document is not meant to be a comprehensive manual on 
understanding or using wireless technologies, but should be sufficient 
to get you moving without wires on Linux.
이 문서는 무선 기술을 이해하거나 사용하는 것에 대한 포괄적 인 안내서가 아니며 Linux에서 유선없이 이동하는 데 충분해야합니다.

For information on building and installing the driver, see the INSTALL file.
드라이버 빌드 및 설치에 대한 자세한 내용은 INSTALL 파일을 참조하십시오.

1.1. Overview of Features
-----------------------------------------------
The current release (1.1.2) supports the following features:
현재 릴리스 (1.1.2)는 다음 기능을 지원합니다.

+ BSS mode (Infrastructure, Managed)
+ IBSS mode (Ad-Hoc)
+ WEP (OPEN and SHARED KEY mode)
+ 802.1x EAP via wpa_supplicant and xsupplicant
+ Wireless Extension support 
+ Full B and G rate support (2200 and 2915)
+ Full A rate support (2915 only)
+ Transmit power control
+ S state support (ACPI suspend/resume)

The following features are currently enabled, but not officially
supported:
다음 기능이 현재 사용 가능하지만 공식적으로 지원되지 않습니다.

+ WPA
+ long/short preamble support
+ Monitor mode (aka RFMon)

The distinction between officially supported and enabled is a reflection 
on the amount of validation and interoperability testing that has been
performed on a given feature. 
공식적으로 지원되고 활성화 된 것의 차이는 주어진 기능에 대해 수행 된 유효성 검사 및 상호 운용성 테스트의 양에 대한 반영입니다.


1.2. Command Line Parameters
-----------------------------------------------

Like many modules used in the Linux kernel, the Intel(R) PRO/Wireless
2915ABG Driver for Linux allows configuration options to be provided 
as module parameters.  The most common way to specify a module parameter 
is via the command line.  
Linux 용 인텔 (R) PRO / 무선 2915ABG 드라이버는 Linux 커널에서 사용되는 많은 모듈과 마찬가지로 구성 옵션을 모듈 매개 변수로 제공 할 수 있습니다. 모듈 매개 변수를 지정하는 가장 일반적인 방법은 명령 행을 사용하는 것입니다.

The general form is:
일반적인 형식은 다음과 같습니다.

% modprobe ipw2200 parameter=value

Where the supported parameter are:
지원되는 매개 변수는 다음과 같습니다.

  associate
	Set to 0 to disable the auto scan-and-associate functionality of the
	driver.  If disabled, the driver will not attempt to scan 
	for and associate to a network until it has been configured with 
	one or more properties for the target network, for example configuring 
	the network SSID.  Default is 0 (do not auto-associate)
	드라이버의 자동 스캔 및 연관 기능을 사용하지 않으려면 0으로 설정하십시오. 비활성화 된 경우 드라이버는 대상 네트워크에 대해 하나 이상의 속성 (예 : 네트워크 SSID 구성)이 구성 될 때까지 드라이버를 검색하여 네트워크에 연결하려고 시도하지 않습니다. 기본값은 0입니다 (자동 연결하지 않음).

	Example: % modprobe ipw2200 associate=0

  auto_create
	Set to 0 to disable the auto creation of an Ad-Hoc network 
	matching the channel and network name parameters provided.  
	Default is 1.
	제공된 채널 및 네트워크 이름 매개 변수와 일치하는 Ad-Hoc 네트워크 자동 생성을 비활성화하려면 0으로 설정하십시오. 기본값은 1입니다.

  channel
	channel number for association.  The normal method for setting
        the channel would be to use the standard wireless tools
        (i.e. `iwconfig eth1 channel 10`), but it is useful sometimes
	to set this while debugging.  Channel 0 means 'ANY'
	연결을위한 채널 번호. 채널을 설정하는 일반적인 방법은 표준 무선 도구 (i.e. `iwconfig eth1 channel 10`)를 사용하는 것이지만 디버깅 중에 이것을 설정하는 것이 유용 할 때가 있습니다. 채널 0은 'ANY'

  debug
	If using a debug build, this is used to control the amount of debug
	info is logged.  See the 'dvals' and 'load' script for more info on
	how to use this (the dvals and load scripts are provided as part 
	of the ipw2200 development snapshot releases available from the 
	SourceForge project at http://ipw2200.sf.net)
  	디버그 빌드를 사용하는 경우 디버깅 정보의 양을 제어하는 데 사용됩니다. 이것을 사용하는 방법에 대한 자세한 정보는 'dvals'및 'load'스크립트를 참조하십시오 (dvals 및로드 스크립트는 http://ipw2200.sf.net의 SourceForge 프로젝트에서 사용 가능한 ipw2200 개발 스냅 샷 릴리즈의 일부로 제공됩니다)

  led
	Can be used to turn on experimental LED code.
	실험용 LED 코드를 켜는 데 사용할 수 있습니다.

	0 = Off, 1 = On.  Default is 1.

  mode
	Can be used to set the default mode of the adapter. 
	어댑터의 기본 모드를 설정하는 데 사용할 수 있습니다.

	0 = Managed, 1 = Ad-Hoc, 2 = Monitor


1.3. Wireless Extension Private Methods
-----------------------------------------------

As an interface designed to handle generic hardware, there are certain 
capabilities not exposed through the normal Wireless Tool interface.  As 
such, a provision is provided for a driver to declare custom, or 
private, methods.  The Intel(R) PRO/Wireless 2915ABG Driver for Linux 
defines several of these to configure various settings.
일반 하드웨어를 처리하도록 설계된 인터페이스로 일반 무선 툴 인터페이스를 통해 노출되지 않는 특정 기능이 있습니다. 따라서 드라이버가 맞춤형 또는 개인용 메서드를 선언 할 수있는 조항이 제공됩니다. Linux 용 인텔 (R) PRO / 무선 2915ABG 드라이버는 다양한 설정을 구성하기 위해 이들 중 몇 가지를 정의합니다.

The general form of using the private wireless methods is:
개인 무선 방법을 사용하는 일반적인 형태는 다음과 같습니다.

	% iwpriv $IFNAME method parameters

Where $IFNAME is the interface name the device is registered with 
(typically eth1, customized via one of the various network interface
name managers, such as ifrename)
여기서 $ IFNAME은 장치가 등록 된 인터페이스 이름입니다 (일반적으로 eth1, 다양한 네트워크 인터페이스 이름 관리자 중 하나 (예 : ifrename)를 통해 사용자 정의 됨)

The supported private methods are:
지원되는 개인 메소드는 다음과 같습니다.

  get_mode
	Can be used to report out which IEEE mode the driver is 
	configured to support.  Example:
	드라이버가 지원하도록 구성된 IEEE 모드를보고하는 데 사용할 수 있습니다. 예:

	% iwpriv eth1 get_mode
	eth1	get_mode:802.11bg (6)

  set_mode
	Can be used to configure which IEEE mode the driver will 
	support.  
드라이버가 지원할 IEEE 모드를 구성하는 데 사용할 수 있습니다.

	Usage:
	% iwpriv eth1 set_mode {mode}
	Where {mode} is a number in the range 1-7:
	1	802.11a (2915 only)
	2	802.11b
	3	802.11ab (2915 only)
	4	802.11g 
	5	802.11ag (2915 only)
	6	802.11bg
	7	802.11abg (2915 only)

  get_preamble
	Can be used to report configuration of preamble length.
	preamble 길이 구성을보고하는 데 사용할 수 있습니다.
  
  set_preamble
	Can be used to set the configuration of preamble length:
	preamble 길이를 설정하는 데 사용할 수 있습니다.

	Usage:
	% iwpriv eth1 set_preamble {mode}
	Where {mode} is one of:
	1	Long preamble only
	0	Auto (long or short based on connection)
	

1.4. Sysfs Helper Files:
-----------------------------------------------

The Linux kernel provides a pseudo file system that can be used to 
access various components of the operating system.  The Intel(R)
PRO/Wireless 2915ABG Driver for Linux exposes several configuration
parameters through this mechanism.
Linux 커널은 운영 체제의 다양한 구성 요소에 액세스하는 데 사용할 수있는 가상 파일 시스템을 제공합니다.
Linux 용 인텔 (R) PRO / Wireless 2915ABG 드라이버는이 메커니즘은 일부 구성 매개 변수를 제공합니다.

An entry in the sysfs can support reading and/or writing.  You can 
typically query the contents of a sysfs entry through the use of cat, 
and can set the contents via echo.  For example:
sysfs의 항목은 읽기 및 / 또는 쓰기를 지원할 수 있습니다. 일반적으로 cat을 사용하여 sysfs 항목의 내용을 쿼리 할 수 있으며 echo를 통해 내용을 설정할 수 있습니다. 

예 :
% cat /sys/bus/pci/drivers/ipw2200/debug_level

Will report the current debug level of the driver's logging subsystem 
(only available if CONFIG_IPW2200_DEBUG was configured when the driver
was built).
드라이버 로깅 서브 시스템의 현재 디버그 레벨을보고합니다 (드라이버가 빌드되었을 때 CONFIG_IPW2200_DEBUG가 구성된 경우에만 사용 가능).

You can set the debug level via:
다음을 통해 디버그 level를 설정할 수 있습니다.

% echo $VALUE > /sys/bus/pci/drivers/ipw2200/debug_level

Where $VALUE would be a number in the case of this sysfs entry.  The 
input to sysfs files does not have to be a number.  For example, the 
firmware loader used by hotplug utilizes sysfs entries for transferring 
the firmware image from user space into the driver.
여기서 $ VALUE는이 sysfs 항목의 경우 숫자입니다. sysfs 파일에 대한 입력은 숫자 일 필요는 없습니다. 예를 들어 hotplug에서 사용하는 펌웨어 로더는 sysfs 항목을 사용하여 펌웨어 이미지를 사용자 공간에서 드라이버로 전송합니다.

The Intel(R) PRO/Wireless 2915ABG Driver for Linux exposes sysfs entries 
at two levels -- driver level, which apply to all instances of the driver 
(in the event that there are more than one device installed) and device 
level, which applies only to the single specific instance.
Linux 용 인텔 (R) PRO / Wireless 2915ABG 드라이버는 드라이버의 모든 인스턴스 (여러 장치가 설치되어있는 경우)에 적용되는 드라이버 level과 특정 인스턴스에 적용되는 장치 level의 두 가지 level 로 sysfs 항목을 공개합니다.

1.4.1 Driver Level Sysfs Helper Files
-----------------------------------------------

For the driver level files, look in /sys/bus/pci/drivers/ipw2200/
드라이버 레벨 파일은 / sys / bus / pci / drivers / ipw2200 /

  debug_level  
	
	This controls the same global as the 'debug' module parameter
	이것은 'debug'모듈 매개 변수와 동일한 전역을 제어합니다


1.4.2 Device Level Sysfs Helper Files
-----------------------------------------------

For the device level files, look in
	장치 level 파일의 경우

	/sys/bus/pci/drivers/ipw2200/{PCI-ID}/

For example:
	/sys/bus/pci/drivers/ipw2200/0000:02:01.0

For the device level files, see /sys/bus/pci/drivers/ipw2200:
장치 레벨 파일에 대해서는 / sys / bus / pci / drivers / ipw2200 :

  rf_kill
	read - 
	0 = RF kill not enabled (radio on)
		RF 킬 사용 불가

	1 = SW based RF kill active (radio off)
		SW 기반 RF 소거 활성화

	2 = HW based RF kill active (radio off)
		HW 기반 RF 소거 활성화

	3 = Both HW and SW RF kill active (radio off)
		HW와 SW RF 모두 활성

	write -
	0 = If SW based RF kill active, turn the radio back on
		SW 기반 RF가 활성화되지 않으면 라디오를 다시 켭니다.

	1 = If radio is on, activate SW based RF kill
		라디오가 켜져 있으면 SW 기반 RF 차단을 활성화합니다.

	NOTE: If you enable the SW based RF kill and then toggle the HW
  	based RF kill from ON -> OFF -> ON, the radio will NOT come back on
	참고 : SW 기반 RF 킬을 활성화 한 다음 HW 기반 RF 킬을 ON-> OFF-> ON에서 toggle하면 라디오가 다시 켜지지 않습니다

  ucode 
	read-only access to the ucode version number
	ucode 버전 번호에 대한 읽기 전용 액세스

  led
	read -
	0 = LED code disabled
	1 = LED code enabled
	write -
	0 = Disable LED code
	1 = Enable LED code

	NOTE: The LED code has been reported to hang some systems when 
	running ifconfig and is therefore disabled by default.
	참고 : LED 코드는 ifconfig를 실행할 때 일부 시스템이 멈추는 것 으로 보고 되었으므로 기본적으로 비활성화되어 있습니다.

1.5. Supported channels
-----------------------------------------------

Upon loading the Intel(R) PRO/Wireless 2915ABG Driver for Linux, a
message stating the detected geography code and the number of 802.11
channels supported by the card will be displayed in the log.
Linux 용 Intel (R) PRO / 무선 2915ABG 드라이버를 로드 하면 감지 된  geography 코드와 카드에서 지원하는 802.11 채널 수를 나타내는 메시지가 로그에 표시됩니다.

The geography code corresponds to a regulatory domain as shown in the
table below.
geography 코드는 아래 표에 표시된 규제 도메인에 해당합니다.

					  Supported channels
Code	Geography			802.11bg	802.11a

---	Restricted			11 	 	 0
ZZF	Custom US/Canada		11	 	 8
ZZD	Rest of World			13	 	 0
ZZA	Custom USA & Europe & High	11		13
ZZB	Custom NA & Europe    		11		13
ZZC	Custom Japan			11	 	 4
ZZM	Custom 				11	 	 0
ZZE	Europe				13		19
ZZJ	Custom Japan			14	 	 4
ZZR	Rest of World			14	 	 0
ZZH	High Band			13	 	 4
ZZG	Custom Europe			13	 	 4
ZZK	Europe 				13		24
ZZL	Europe				11		13


2.   Ad-Hoc Networking
-----------------------------------------------

When using a device in an Ad-Hoc network, it is useful to understand the 
sequence and requirements for the driver to be able to create, join, or 
merge networks.
Ad-Hoc 네트워크에서 장치를 사용할 때 드라이버가 네트워크를 만들거나 가입하거나 병합 할 수있는 순서와 요구 사항을 이해하는 것이 유용합니다.

The following attempts to provide enough information so that you can 
have a consistent experience while using the driver as a member of an 
Ad-Hoc network.
Ad-Hoc 네트워크의 구성원으로 드라이버를 사용하는 동안 일관된 경험을 가질 수 있도록 충분한 정보를 제공하려는 다음과 같은 시도가 있습니다.

2.1. Joining an Ad-Hoc Network
-----------------------------------------------

The easiest way to get onto an Ad-Hoc network is to join one that 
already exists.
Ad-Hoc 네트워크에 접속하는 가장 쉬운 방법은 이미 존재하는 네트워크에 가입시키는 것입니다.

2.2. Creating an Ad-Hoc Network
-----------------------------------------------

An Ad-Hoc networks is created using the syntax of the Wireless tool.
Ad-Hoc 네트워크는 무선 도구의 구문을 사용하여 만들어집니다.

For Example:
iwconfig eth1 mode ad-hoc essid testing channel 2
iwconfig eth1 모드 ad-hoc essid 테스트 채널 2

2.3. Merging Ad-Hoc Networks
-----------------------------------------------


3.  Interaction with Wireless Tools
-----------------------------------------------

3.1 iwconfig mode
-----------------------------------------------

When configuring the mode of the adapter, all run-time configured parameters
are reset to the value used when the module was loaded.  This includes
channels, rates, ESSID, etc.
어댑터 모드를 구성 할 때 모든 런타임 구성 매개 변수는 모듈로드시 사용 된 값으로 재설정됩니다. 여기에는 channels, rates, ESSID 등이 포함됩니다.

3.2 iwconfig sens
-----------------------------------------------

The 'iwconfig ethX sens XX' command will not set the signal sensitivity
threshold, as described in iwconfig documentation, but rather the number
of consecutive missed beacons that will trigger handover, i.e. roaming
to another access point. At the same time, it will set the disassociation
threshold to 3 times the given value.
'iwconfig ethX sens XX'명령은 iwconfig 설명서에 설명 된대로 신호 감도 임계 값을 설정하지 않고, 핸드 오버를 트리거하는 연속 누락 비컨 수, i.e.가 다른 액세스 포인트로 로밍하는 횟수를 설정합니다. 동시에 해체 기준치를 주어진 값의 3 배로 설정합니다.

4.   About the Version Numbers
-----------------------------------------------

Due to the nature of open source development projects, there are 
frequently changes being incorporated that have not gone through 
a complete validation process.  These changes are incorporated into 
development snapshot releases.
오픈 소스 개발 프로젝트의 특성상 완전한 검증 과정을 거치지 않은 많은 변화가 포함되어 있습니다.
이러한 변경은 개발 스냅 샷 릴리스에 포함되어 있습니다.

Releases are numbered with a three level scheme: 
릴리스에는 3 가지 레벨 구성표가 있습니다.

	major.minor.development

Any version where the 'development' portion is 0 (for example
1.0.0, 1.1.0, etc.) indicates a stable version that will be made 
available for kernel inclusion.
'development'부분이 0 인 모든 버전 (예 : 1.0.0, 1.1.0 등)은 커널 포함에 사용할 수있는 stable 버전을 나타냅니다.


Any version where the 'development' portion is not a 0 (for
example 1.0.1, 1.1.5, etc.) indicates a development version that is
being made available for testing and cutting edge users.  The stability 
and functionality of the development releases are not know.  We make
efforts to try and keep all snapshots reasonably stable, but due to the
frequency of their release, and the desire to get those releases 
available as quickly as possible, unknown anomalies should be expected.
'개발'부분이 0이 아닌 버전 (예를 들어, 1.0.1,1.1.5 등)는 테스트 사용자와 최첨단의 사용자가 사용할 수있는 개발 버전을 나타냅니다.
개발 릴리스의 안정성과 기능은 알 수 없습니다. 우리는 모든 스냅 샷을 비교적 안정적으로 유지하려고 노력하지만, 릴리스 빈도와 가능한 빨리 릴리스를 사용할 수 있기를 원하기 때문에 알 수없는 예외가 예상됩니다.

The major version number will be incremented when significant changes
are made to the driver.  Currently, there are no major changes planned.
주 버전 번호는 드라이버를 크게 변경하면 증가합니다. 현재 주요 변경 계획은 없습니다.

5.  Firmware installation
----------------------------------------------

The driver requires a firmware image, download it and extract the
files under /lib/firmware (or wherever your hotplug's firmware.agent
will look for firmware files)
드라이버는 펌웨어 이미지를 필요로하며 다운로드하고 / lib / firmware (또는 hotplug의 firmware.agent가 펌웨어 파일을 찾을 곳)의 파일을 추출합니다.

The firmware can be downloaded from the following URL:
펌웨어는 다음 URL에서 다운로드 할 수 있습니다.

    http://ipw2200.sf.net/


6.  Support
-----------------------------------------------

For direct support of the 1.0.0 version, you can contact 
http://supportmail.intel.com, or you can use the open source project
support.
1.0.0 버전을 직접 지원하려면 http://supportmail.intel.com에 문의하거나 오픈 소스 프로젝트 지원을 사용할 수 있습니다.

For general information and support, go to:
일반 정보 및 지원을 받으려면 다음으로 이동하십시오.

    http://ipw2200.sf.net/


7.  License
-----------------------------------------------

  Copyright(c) 2003 - 2006 Intel Corporation. All rights reserved.

  This program is free software; you can redistribute it and/or modify it 
  under the terms of the GNU General Public License version 2 as 
  published by the Free Software Foundation.
  이 프로그램은 자유 소프트웨어입니다. 자유 소프트웨어 재단이 발표 한 GNU General Public License 버전 2의 조건에 따라 재배포 및 / 또는 수정할 수 있습니다.

  This program is distributed in the hope that it will be useful, but WITHOUT 
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
  more details.
  이 프로그램은 유용 할 것이라는 희망으로 배포되었지만 어떠한 보증도하지 않습니다. 상품성 또는 특정 목적에의 적합성에 대한 묵시적인 보증조차하지 않습니다. 자세한 내용은 GNU General Public License를 참조하십시오.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59 
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  이 프로그램과 함께 GNU 일반 공중 사용 허가서 사본을 받아야합니다. 그렇지 않은 경우 Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA에 문의하십시오.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
  전체 GNU 일반 공중 사용 허가서는이 배포판에 LICENSE라는 파일에 포함되어 있습니다.

  Contact Information:
  James P. Ketrenos <ipw2100-admin@linux.intel.com>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
  연락처 정보 :
   James P. Ketrenos <ipw2100-admin@linux.intel.com>
   Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, 또는 97124-6497