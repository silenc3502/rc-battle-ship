Intel(R) PRO/Wireless 2100 Driver for Linux in support of:
다음을 지원하는 Linux 용 인텔 (R) PRO / 무선 2100 드라이버 :

Intel(R) PRO/Wireless 2100 Network Connection
인텔 (R) PRO / 무선 2100 네트워크 연결

Copyright (C) 2003-2006, Intel Corporation

README.ipw2100

Version: git-1.1.5
Date   : January 25, 2006

Index
-----------------------------------------------
0. IMPORTANT INFORMATION BEFORE USING THIS DRIVER
1. Introduction
2. Release git-1.1.5 Current Features
3. Command Line Parameters
4. Sysfs Helper Files
5. Radio Kill Switch
6. Dynamic Firmware
7. Power Management
8. Support
9. License


0.   IMPORTANT INFORMATION BEFORE USING THIS DRIVER
-----------------------------------------------

Important Notice FOR ALL USERS OR DISTRIBUTORS!!!!
모든 사용자 또는 유통에 중요한 소식 !!!!

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
이러한 이유로 인텔은 무선 WLAN 어댑터 (예 : EEPROM 및 펌웨어)와 함께 바이너리 형식으로 제공되는 소프트웨어를 타사에서 조작 할 수 없습니다. 또한 인텔이 검증하지 않은 권한이없는 당사자 (i.e., 패치, 유틸리티 또는 코드 (오픈 소스 코드 수정 포함))가 조작 한 인텔 무선 LAN 어댑터와 함께 패치, 유틸리티 또는 코드를 사용하는 경우, (i) 귀하는 제품의 규정 준수를 보증 할 책임이 전적으로 귀하에게 있으며, (ii) 수정 된 제품과 관련된 모든 문제에 대한 이론에 대해 책임을지지 않습니다. / 또는 (iii) 인텔은 그러한 수정 된 제품에 대해 제 3 자에게 지원을 제공하거나 제공하도록 지원하지 않아도됩니다.

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
SourceForge에서 다운로드 할 수있는 드라이버는 개발 프로젝트의 일부로 제공됩니다. 현지 규정 요구 사항을 준수하는 것은 개개인 개발자의 책임입니다. 따라서 개발 이외의 목적으로 사용되도록 의도 된 솔루션의 일부로 드라이버를 배포하거나 출하하는 데 관심이있는 경우 인텔 고객 지원부에서 테스트를 마친 드라이버를 구하십시오.

http://www.intel.com/support/wireless/sb/CS-006408.htm

1. Introduction
-----------------------------------------------

This document provides a brief overview of the features supported by the 
IPW2100 driver project.  The main project website, where the latest 
development version of the driver can be found, is:
이 문서는 IPW2100 드라이버 프로젝트가 지원하는 기능에 대한 간략한 개요를 제공합니다. 최신 개발 버전의 드라이버가있는 주요 프로젝트 웹 사이트는 다음과 같습니다.

	http://ipw2100.sourceforge.net

There you can find the not only the latest releases, but also information about
potential fixes and patches, as well as links to the development mailing list
for the driver project.
여기서 최신 릴리스뿐만 아니라 잠재적 인 픽스와 패치에 대한 정보는 물론 드라이버 프로젝트의 개발 메일 링리스트에 대한 링크를 찾을 수 있습니다.

2. Release git-1.1.5 Current Supported Features
-----------------------------------------------
- Managed (BSS) and Ad-Hoc (IBSS)
- WEP (shared key and open)
- Wireless Tools support 
- 802.1x (tested with XSupplicant 1.0.1)

Enabled (but not supported) features:
- Monitor/RFMon mode
- WPA/WPA2

The distinction between officially supported and enabled is a reflection
on the amount of validation and interoperability testing that has been
performed on a given feature.
공식적으로 지원되고 활성화 된 것의 차이는 주어진 기능에 대해 수행 된 유효성 검사 및 상호 운용성 테스트의 양에 대한 반영입니다.

3. Command Line Parameters
-----------------------------------------------

If the driver is built as a module, the following optional parameters are used
by entering them on the command line with the modprobe command using this
syntax:
드라이버가 모듈로 빌드 된 경우 다음 구문을 사용하여 명령 줄에 modprobe 명령을 입력하여 다음 선택적 매개 변수를 사용합니다.

	modprobe ipw2100 [<option>=<VAL1><,VAL2>...]

For example, to disable the radio on driver loading, enter:
예를 들어, 드라이버 로드시 radio를 비활성화하려면 다음을 입력하십시오.

	modprobe ipw2100 disable=1

The ipw2100 driver supports the following module parameters:
ipw2100 드라이버는 다음 모듈 매개 변수를 지원합니다.

Name		Value		Example:
debug		0x0-0xffffffff	debug=1024
mode		0,1,2		mode=1   /* AdHoc */
channel		int		channel=3 /* Only valid in AdHoc or Monitor */
							  / * AdHoc 또는 Monitor에서만 유효합니다 * /
associate	boolean		associate=0 /* Do NOT auto associate */
									/ * 자동 연결 안 함 * /
disable		boolean		disable=1 /* Do not power the HW */
								  / * HW 전원을 켜지 마십시오 * /

4. Sysfs Helper Files
---------------------------     
-----------------------------------------------

There are several ways to control the behavior of the driver.  Many of the 
general capabilities are exposed through the Wireless Tools (iwconfig).  There
are a few capabilities that are exposed through entries in the Linux Sysfs.
드라이버의 동작을 제어하는 몇 가지 방법이 있습니다. 무선 도구 (iwconfig)를 통해 많은 일반 기능이 제공됩니다. Linux Sysfs의 항목을 통해 노출되는 몇 가지 기능이 있습니다.

----- Driver Level ------
For the driver level files, look in /sys/bus/pci/drivers/ipw2100/
드라이버 level 파일은 / sys / bus / pci / drivers / ipw2100 /
  
  debug_level  
	
	This controls the same global as the 'debug' module parameter.  For 
        information on the various debugging levels available, run the 'dvals'
	script found in the driver source directory.
	이것은 'debug'모듈 매개 변수와 동일한 전역을 제어합니다. 사용 가능한 다양한 디버깅 레벨에 대한 정보는 드라이버 소스 디렉토리에있는 'dval'스크립트를 실행하십시오.

	NOTE:  'debug_level' is only enabled if CONFIG_IPW2100_DEBUG is turn on.
    참고 : 'debug_level'은 CONFIG_IPW2100_DEBUG이 켜져있는 경우에만 활성화됩니다.

----- Device Level ------
For the device level files look in
장치 level 파일을 보려면

	/sys/bus/pci/drivers/ipw2100/{PCI-ID}/

For example:
	/sys/bus/pci/drivers/ipw2100/0000:02:01.0

For the device level files, see /sys/bus/pci/drivers/ipw2100:
장치 level 파일은 / sys / bus / pci / drivers / ipw2100 :

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

5. Radio Kill Switch
-----------------------------------------------
Most laptops provide the ability for the user to physically disable the radio.
Some vendors have implemented this as a physical switch that requires no
software to turn the radio off and on.  On other laptops, however, the switch
is controlled through a button being pressed and a software driver then making
calls to turn the radio off and on.  This is referred to as a "software based
RF kill switch"
대부분의 laptops에서는 사용자가 실제로 라디오를 비활성화 할 수있는 기능을 제공합니다.
일부 공급 업체는 이것을 무선 스위치를 끄고 켤 필요가없는 물리적 스위치로 구현했습니다. 그러나 다른 laptops에서는 스위치를 누른 상태에서 단추를 누르고 소프트웨어 드라이버가 라디오를 껐다가 켭니다. 이를 "소프트웨어 기반 RF kill 스위치"라고합니다.

See the Sysfs helper file 'rf_kill' for determining the state of the RF switch
on your system.
시스템의 RF 스위치 상태를 확인하려면 Sysfs helper file 'rf_kill'을 참조하십시오.

6. Dynamic Firmware
-----------------------------------------------
As the firmware is licensed under a restricted use license, it can not be 
included within the kernel sources.  To enable the IPW2100 you will need a 
firmware image to load into the wireless NIC's processors.
펌웨어는 제한된 사용 라이센스 하에서 라이센스되므로 커널 소스에 포함될 수 없습니다. IPW2100을 사용하려면 무선 NIC의 프로세서 에 로드 할 펌웨어 이미지가 필요합니다.

You can obtain these images from <http://ipw2100.sf.net/firmware.php>.
이 이미지는 <http://ipw2100.sf.net/firmware.php>에서 얻을 수 있습니다.

See INSTALL for instructions on installing the firmware.
펌웨어 설치에 대한 지침은 설치 (INSTALL)를 참조하십시오.

7. Power Management
-----------------------------------------------
The IPW2100 supports the configuration of the Power Save Protocol 
through a private wireless extension interface.  The IPW2100 supports 
the following different modes:
IPW2100은 사설 무선 확장 인터페이스를 통해 절전 프로토콜 구성을 지원합니다. IPW2100은 다음과 같은 다른 모드를 지원합니다 :

	off	No power management.  Radio is always on.
	꺼짐 전원 관리가 없습니다. 라디오는 항상 켜져 있습니다.

	on	Automatic power management
		자동 전원 관리

	1-5	Different levels of power management.  The higher the 
		number the greater the power savings, but with an impact to 
		packet latencies. 
1-5 다른 레벨의 전원 관리. 숫자가 높을수록 전력 절감 효과가 크지 만 패킷 대기 시간에 영향을 미칩니다.

Power management works by powering down the radio after a certain 
interval of time has passed where no packets are passed through the 
radio.  Once powered down, the radio remains in that state for a given 
period of time.  For higher power savings, the interval between last 
packet processed to sleep is shorter and the sleep period is longer.
전원 관리는 패킷이 라디오를 통과하지 않는 일정 시간이 지난 후에 라디오의 전원을 꺼서 작동합니다. 전원이 꺼지면 라디오는 일정 시간 동안 그 상태를 유지합니다. 
높은 전력 절약을 위해 절전 상태에 처리 된 마지막 패킷의 간격은 더 짧고, 수면 기간은 더 길다.

When the radio is asleep, the access point sending data to the station 
must buffer packets at the AP until the station wakes up and requests 
any buffered packets.  If you have an AP that does not correctly support 
the PSP protocol you may experience packet loss or very poor performance 
while power management is enabled.  If this is the case, you will need 
to try and find a firmware update for your AP, or disable power 
management (via `iwconfig eth1 power off`)
무선 장치가 잠 들어있을 때 스테이션에 데이터를 보내는 액세스 포인트는 스테이션이 깨어나고 버퍼링 된 패킷을 요청할 때까지 AP에서 패킷을 버퍼해야합니다. PSP 프로토콜을 올바르게 지원하지 않는 AP가있는 경우 전원 관리가 활성화되어있을 때 패킷 손실 또는 성능 저하가 발생할 수 있습니다. 이 경우, AP에 대한 펌웨어 업데이트를 찾거나 전원 관리를 비활성화해야합니다 ( 'iwconfig eth1 power off`를 통해)

To configure the power level on the IPW2100 you use a combination of 
iwconfig and iwpriv.  iwconfig is used to turn power management on, off, 
and set it to auto.
IPW2100의 전원 수준을 구성하려면 iwconfig와 iwpriv를 함께 사용하십시오. iwconfig는 전원 관리를 켜고 끄고 자동으로 설정하는 데 사용됩니다.

	iwconfig eth1 power off    Disables radio power down 
								무선 전원 끄기 비활성화
	
	iwconfig eth1 power on     Enables radio power management to last set level (defaults to AUTO)
				   			   무선 전원 관리를 마지막 설정 수준으로 설정합니다 (기본값은 AUTO). 
	
	iwpriv eth1 set_power 0    Sets power level to AUTO and enables power management if not previously enabled.
								이전에 활성화되지 않은 경우 전원 수준을 AUTO로 설정하고 전원 관리를 활성화합니다.

	iwpriv eth1 set_power 1-5  Set the power level as specified,enabling power management if not previously enabled.
								지정된대로 전원 수준을 설정하고 이전에 활성화하지 않은 경우 전원 관리를 활성화합니다.

You can view the current power level setting via:
	다음을 통해 현재 전원 수준 설정을 볼 수 있습니다.

	iwpriv eth1 get_power

It will return the current period or timeout that is configured as a string
in the form of xxxx/yyyy (z) where xxxx is the timeout interval (amount of
time after packet processing), yyyy is the period to sleep (amount of time to 
wait before powering the radio and querying the access point for buffered
packets), and z is the 'power level'.  If power management is turned off the
xxxx/yyyy will be replaced with 'off' -- the level reported will be the active
level if `iwconfig eth1 power on` is invoked.
xxxx / yyyy (z) 형식으로 문자열로 구성된 현재 기간 또는 시간 초과를 반환합니다. 여기서 xxxx는 시간 초과 간격이고(패킷 처리 후의 시간 량), yyyy는 절전 기간이며(라디오에 전원을 공급하고 버퍼링 된 패킷에 대해 액세스 지점을 쿼리하기 전에 기다리는 시간) z는 'power level'입니다.

8. Support
-----------------------------------------------

For general development information and support,
go to:
일반적인 개발 정보 및 지원을 받으려면 다음으로 이동하십시오.

    http://ipw2100.sf.net/

The ipw2100 1.1.0 driver and firmware can be downloaded from:  
ipw2100 1.1.0 드라이버 및 펌웨어는 다음 위치에서 다운로드 할 수 있습니다.

    http://support.intel.com

For installation support on the ipw2100 1.1.0 driver on Linux kernels 
2.6.8 or greater, email support is available from:  
Linux 커널 2.6.8 이상에서 ipw2100 1.1.0 드라이버에서 설치 지원을 받으려면 전자 email support가 다음에서 제공됩니다.

    http://supportmail.intel.com

9. License
-----------------------------------------------

  Copyright(c) 2003 - 2006 Intel Corporation. All rights reserved.

  This program is free software; you can redistribute it and/or modify it 
  under the terms of the GNU General Public License (version 2) as 
  published by the Free Software Foundation.
  이 프로그램은 자유 소프트웨어입니다. GNU General Public License (버전 2)의 조건에 따라 자유 소프트웨어 재단이 발행 한대로 재배포 및 / 또는 수정할 수 있습니다.

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

  License Contact Information:
  James P. Ketrenos <ipw2100-admin@linux.intel.com>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
  
156/5000
라이센스 연락처 정보 :
   James P. Ketrenos <ipw2100-admin@linux.intel.com>
   Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, 또는 97124-6497