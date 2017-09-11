sb1000 is a module network device driver for the General Instrument (also known
as NextLevel) SURFboard1000 internal cable modem board.  This is an ISA card
which is used by a number of cable TV companies to provide cable modem access.
It's a one-way downstream-only cable modem, meaning that your upstream net link
is provided by your regular phone modem.
sb1000은 일반 장비 (NextLevel이라고도 알려짐) SURFboard1000 내부 케이블 모뎀 보드 용 모듈 네트워크 장치 드라이버입니다. 이것은 많은 케이블 TV 회사에서 케이블 모뎀 액세스를 제공하는 데 사용되는 ISA 카드입니다. 이는 단방향 다운 스트림 전용 케이블 모뎀입니다. 즉, 업스트림 네트워크 연결은 일반 전화 모뎀에서 제공됩니다.

This driver was written by Franco Venturi <fventuri@mediaone.net>.  He deserves
a great deal of thanks for this wonderful piece of code!
이 드라이버는 Franco Venturi <fventuri@mediaone.net>에 의해 작성되었습니다. 그는이 멋진 코드에 대해 많은 감사를 감당할 가치가 있습니다!

-----------------------------------------------------------------------------

Support for this device is now a part of the standard Linux kernel.  The
driver source code file is drivers/net/sb1000.c.  In addition to this
you will need:
이 장치에 대한 지원은 이제 표준 Linux 커널의 일부입니다. 드라이버 소스 코드 파일은 drivers / net / sb1000.c입니다. 이 외에도 다음이 필요합니다.

1.) The "cmconfig" program.  This is a utility which supplements "ifconfig"
to configure the cable modem and network interface (usually called "cm0");
and
1.) "cmconfig"프로그램. 이것은 "ifconfig"를 보완하여 케이블 모뎀과 네트워크 인터페이스 (일반적으로 "cm0"라고 함)를 구성하는 유틸리티입니다. 과

2.) Several PPP scripts which live in /etc/ppp to make connecting via your
cable modem easy.
케이블 모뎀을 통해 쉽게 연결할 수 있도록 / etc / ppp에있는 여러 PPP 스크립트.

   These utilities can be obtained from:
이러한 유틸리티는 다음 위치에서 얻을 수 있습니다.

      http://www.jacksonville.net/~fventuri/

   in Franco's original source code distribution .tar.gz file.  Support for
   the sb1000 driver can be found at:
Franco's original 소스 코드 배포 .tar.gz 파일. sb1000 드라이버에 대한 지원은 다음에서 찾을 수 있습니다.

      http://web.archive.org/web/*/http://home.adelphia.net/~siglercm/sb1000.html
      http://web.archive.org/web/*/http://linuxpower.cx/~cable/

   along with these utilities.
   이러한 유틸리티와 함께.

3.) The standard isapnp tools.  These are necessary to configure your SB1000
card at boot time (or afterwards by hand) since it's a PnP card.
3.) 표준 isapnp 도구. SB1000 카드는 부팅시 (또는 이후에 수동으로) PnP 카드이므로 구성해야합니다.

   If you don't have these installed as a standard part of your Linux
   distribution, you can find them at:
Linux 배포판의 표준 부분으로 이들을 설치하지 않은 경우 다음 위치에서 찾을 수 있습니다.
      http://www.roestock.demon.co.uk/isapnptools/

   or check your Linux distribution binary CD or their web site.  For help with
   isapnp, pnpdump, or /etc/isapnp.conf, go to:
또는 Linux 배포 바이너리 CD 또는 해당 웹 사이트를 확인하십시오. isapnp, pnpdump 또는 /etc/isapnp.conf에 대한 도움말을 보려면 다음으로 이동하십시오.

      http://www.roestock.demon.co.uk/isapnptools/isapnpfaq.html

-----------------------------------------------------------------------------

To make the SB1000 card work, follow these steps:
SB1000 카드를 작동 시키려면 다음 단계를 수행하십시오.

1.) Run `make config', or `make menuconfig', or `make xconfig', whichever
you prefer, in the top kernel tree directory to set up your kernel
configuration.  Make sure to say "Y" to "Prompt for development drivers"
and to say "M" to the sb1000 driver.  Also say "Y" or "M" to all the standard
networking questions to get TCP/IP and PPP networking support.
1.)`make config '또는`make menuconfig'또는`make xconfig '를 실행하여 커널 트리 설정 디렉토리에서 커널 설정을한다. "Y"를 "개발 드라이버 확인"으로 말하고 sb1000 드라이버에 "M"이라고 말하십시오. 또한 모든 표준 네트워킹 질문에 "Y"또는 "M"이라고 말하면 TCP / IP 및 PPP 네트워킹 지원을받을 수 있습니다.

2.) *BEFORE* you build the kernel, edit drivers/net/sb1000.c.  Make sure
to redefine the value of READ_DATA_PORT to match the I/O address used
by isapnp to access your PnP cards.  This is the value of READPORT in
/etc/isapnp.conf or given by the output of pnpdump.
2.) * * 커널을 빌드하기 전에 drivers / net / sb1000.c를 편집하십시오. isapnp가 PnP 카드에 액세스하는 데 사용하는 I / O 주소와 일치하도록 READ_DATA_PORT의 값을 다시 정의하십시오. /etc/isapnp.conf의 READPORT 값이거나 pnpdump의 출력 값입니다.

3.) Build and install the kernel and modules as usual.
3.) 평소처럼 커널과 모듈을 빌드하고 설치하십시오.

4.) Boot your new kernel following the usual procedures.
4.) 일반적인 절차에 따라 새 커널을 부팅하십시오.

5.) Set up to configure the new SB1000 PnP card by capturing the output
of "pnpdump" to a file and editing this file to set the correct I/O ports,
IRQ, and DMA settings for all your PnP cards.  Make sure none of the settings
conflict with one another.  Then test this configuration by running the
"isapnp" command with your new config file as the input.  Check for
errors and fix as necessary.  (As an aside, I use I/O ports 0x110 and
0x310 and IRQ 11 for my SB1000 card and these work well for me.  YMMV.)
Then save the finished config file as /etc/isapnp.conf for proper configuration
on subsequent reboots.
5.) "pnpdump"의 출력을 파일에 캡처하고이 파일을 편집하여 모든 PnP 카드에 대해 올바른 I / O 포트, IRQ 및 DMA 설정을 설정하여 새 SB1000 PnP 카드를 구성하도록 설정합니다. 설정이 서로 충돌하지 않는지 확인하십시오. 그런 다음 새 구성 파일을 입력으로 사용하여 "isapnp"명령을 실행하여이 구성을 테스트하십시오. 오류를 확인하고 필요한 경우 수정하십시오. (제쳐두고, 저는 SB1000 카드에 I / O 포트 0x110과 0x310과 IRQ 11을 사용합니다. YMMV.) 그런 다음 재부팅 할 때 적절한 구성을 위해 /etc/isapnp.conf로 완성 된 설정 파일을 저장하십시오. .

6.) Download the original file sb1000-1.1.2.tar.gz from Franco's site or one of
the others referenced above.  As root, unpack it into a temporary directory and
do a `make cmconfig' and then `install -c cmconfig /usr/local/sbin'.  Don't do
`make install' because it expects to find all the utilities built and ready for
installation, not just cmconfig.
6.) 원본 파일 sb1000-1.1.2.tar.gz를 Franco 사이트 또는 위에서 언급 한 다른 사이트에서 다운로드하십시오. root 권한으로 임시 디렉토리에 압축을 풀고`make cmconfig '를 실행하고`-c cmconfig / usr / local / sbin'을 실행하십시오. `make install '을 실행하지 마십시오. 이것은 justcmconfig 대신 모든 유틸리티가 내장되어 설치 가능한 상태가 될 것으로 기대하고 있기 때문입니다.

7.) As root, copy all the files under the ppp/ subdirectory in Franco's
tar file into /etc/ppp, being careful not to overwrite any files that are
already in there.  Then modify ppp@gi-on to set the correct login name,
phone number, and frequency for the cable modem.  Also edit pap-secrets
to specify your login name and password and any site-specific information
you need.
7. root로, Franco의 tar 파일에있는 ppp / subdirectory 아래에있는 모든 파일을 / etc / ppp에 복사하십시오. 거기에 이미있는 파일을 덮어 쓰지 않도록주의하십시오. 그런 다음 ppp @ gi-on을 수정하여 케이블 모뎀의 올바른 로그인 이름, 전화 번호 및 주파수를 설정하십시오. pap-secrets를 편집하여 로그인 이름과 암호 및 필요한 사이트 별 정보를 지정하십시오.

8.) Be sure to modify /etc/ppp/firewall to use ipchains instead of
the older ipfwadm commands from the 2.0.x kernels.  There's a neat utility to
convert ipfwadm commands to ipchains commands:
8.) 2.0.x 커널의 이전 ipfwadm 명령 대신 ipchains를 사용하려면 / etc / ppp / firewall을 수정해야합니다. ipfwadm 명령을 ipchains 명령으로 변환하는 깔끔한 유틸리티가 있습니다.

   http://users.dhp.com/~whisper/ipfwadm2ipchains/

You may also wish to modify the firewall script to implement a different
firewalling scheme.
방화벽 스크립트를 수정하여 다른 방화벽 구성표를 구현할 수도 있습니다.

9.) Start the PPP connection via the script /etc/ppp/ppp@gi-on.  You must be
root to do this.  It's better to use a utility like sudo to execute
frequently used commands like this with root permissions if possible.  If you
connect successfully the cable modem interface will come up and you'll see a
driver message like this at the console:
9.) / etc / ppp / ppp @ gi-on 스크립트를 통해 PPP 연결을 시작하십시오. 이 작업을 수행하려면 루트 사용자 여야합니다. sudo와 같은 유틸리티를 사용하여 가능하면 루트 권한으로 자주 사용하는 명령을 실행하는 것이 좋습니다. 성공적으로 연결하면 케이블 모뎀 인터페이스가 나타나고 콘솔에 다음과 같은 드라이버 메시지가 표시됩니다.

         cm0: sb1000 at (0x110,0x310), csn 1, S/N 0x2a0d16d8, IRQ 11.
         sb1000.c:v1.1.2 6/01/98 (fventuri@mediaone.net)

The "ifconfig" command should show two new interfaces, ppp0 and cm0.
The command "cmconfig cm0" will give you information about the cable modem
interface.
"ifconfig"명령은 ppp0과 cm0이라는 두 개의 새로운 인터페이스를 보여 주어야합니다.
"cmconfig cm0"명령은 케이블 모뎀 인터페이스에 대한 정보를 제공합니다.

10.) Try pinging a site via `ping -c 5 www.yahoo.com', for example.  You should
see packets received.
10.) 예를 들어`ping -c 5 www.yahoo.com '을 통해 사이트에 핑 (ping)을 시도하십시오. 받은 패킷을 확인해야합니다.

11.) If you can't get site names (like www.yahoo.com) to resolve into
IP addresses (like 204.71.200.67), be sure your /etc/resolv.conf file
has no syntax errors and has the right nameserver IP addresses in it.
If this doesn't help, try something like `ping -c 5 204.71.200.67' to
see if the networking is running but the DNS resolution is where the
problem lies.
11.) (예 : www.yahoo.com과 같은) 사이트 이름을 IP 주소 (예 : 204.71.200.67)로 가져올 수없는 경우 /etc/resolv.conf 파일에 구문 오류가없고 올바른 nameserver IP 주소가 포함되어 있습니다. 이 방법이 도움이되지 않는다면`ping -c 5 204.71.200.67 '과 같은 것을 시도해 보면 네트워크가 돌아가고 있는지 DNS 해상도가 문제가있는 곳인지 확인하십시오.

12.) If you still have problems, go to the support web sites mentioned above
and read the information and documentation there.
12.) 여전히 문제가 있다면 위에서 언급 한 지원 웹 사이트로 이동하여 정보와 설명서를 읽으십시오.

-----------------------------------------------------------------------------

Common problems:

1.) Packets go out on the ppp0 interface but don't come back on the cm0
interface.  It looks like I'm connected but I can't even ping any
numerical IP addresses.  (This happens predominantly on Debian systems due
to a default boot-time configuration script.)
1.) 패킷은 ppp0 인터페이스로 나가지만 cm0 인터페이스로 되돌아 가지 않습니다. 내가 연결되어 있지만 숫자 IP 주소를 ping 할 수없는 것 같습니다. (이는 주로 기본 부팅 시간 설정 스크립트로 인해 Debian 시스템에서 발생합니다.)

Solution -- As root `echo 0 > /proc/sys/net/ipv4/conf/cm0/rp_filter' so it
can share the same IP address as the ppp0 interface.  Note that this
command should probably be added to the /etc/ppp/cablemodem script
*right*between* the "/sbin/ifconfig" and "/sbin/cmconfig" commands.
You may need to do this to /proc/sys/net/ipv4/conf/ppp0/rp_filter as well.
If you do this to /proc/sys/net/ipv4/conf/default/rp_filter on each reboot
(in rc.local or some such) then any interfaces can share the same IP
addresses.
해결책 - 루트로서`echo 0> / proc / sys / net / ipv4 / conf / cm0 / rp_filter '명령으로 ppp0 인터페이스와 같은 IP 주소를 공유 할 수 있습니다. 이 명령은 "/ sbin / ifconfig"및 "/ sbin / cmconfig"명령 사이의 / etc / ppp / cablemodem 스크립트 * 오른쪽에 추가해야합니다. 이 작업은 / proc / sys / net / ipv4 / conf / ppp0 / rp_filter에서도 필요합니다. 재부팅 할 때마다 / proc / sys / net / ipv4 / conf / default / rp_filter를 실행하면 (rc.local 또는 그와 같은 경우) 모든 인터페이스가 동일한 IP 주소를 공유 할 수 있습니다.

2.) I get "unresolved symbol" error messages on executing `insmod sb1000.o'.

Solution -- You probably have a non-matching kernel source tree and
/usr/include/linux and /usr/include/asm header files.  Make sure you
install the correct versions of the header files in these two directories.
Then rebuild and reinstall the kernel.
2.)`insmod sb1000.o '실행시 "unresolved symbol"오류 메시지가 나타납니다.

해결책 - 일치하지 않는 커널 소스 트리와 / usr / include / linux 및 / usr / include / asm 헤더 파일이있을 수 있습니다. 이 두 디렉토리에 올바른 버전의 헤더 파일을 설치해야합니다. 그런 다음 커널을 다시 빌드하고 다시 설치하십시오.

3.) When isapnp runs it reports an error, and my SB1000 card isn't working.
isapnp가 실행되면 오류가보고되고 SB1000 카드가 작동하지 않습니다.

Solution -- There's a problem with later versions of isapnp using the "(CHECK)"
option in the lines that allocate the two I/O addresses for the SB1000 card.
This first popped up on RH 6.0.  Delete "(CHECK)" for the SB1000 I/O addresses.
Make sure they don't conflict with any other pieces of hardware first!  Then
rerun isapnp and go from there.
해결책 - SB1000 카드의 두 I / O 주소를 할당하는 줄에 "(CHECK)"옵션을 사용하는 이후 버전의 isapnp에는 문제가 있습니다.
RH 6.0에서 처음 나타났습니다. SB1000 I / O 주소는 "(CHECK)"를 삭제하십시오.
먼저 다른 하드웨어와 충돌하지 않도록하십시오! 그런 다음 isapnp를 다시 실행하고 거기에서 이동하십시오.

4.) I can't execute the /etc/ppp/ppp@gi-on file.
/ etc / ppp / ppp @ gi-on 파일을 실행할 수 없습니다.

Solution -- As root do `chmod ug+x /etc/ppp/ppp@gi-on'.
해결책 - 루트로서`chmod ug + x / etc / ppp / ppp @ gi-on '을하십시오.

5.) The firewall script isn't working (with 2.2.x and higher kernels).
5.) 방화벽 스크립트가 작동하지 않습니다 (2.2.x 이상 커널 사용).

Solution -- Use the ipfwadm2ipchains script referenced above to convert the
/etc/ppp/firewall script from the deprecated ipfwadm commands to ipchains.
해결책 - 위에서 언급 한 ipfwadm2ipchains 스크립트를 사용하여 / etc / ppp / firewall 스크립트를 deprecated ipfwadm 명령에서 ipchains로 변환하십시오.

6.) I'm getting *tons* of firewall deny messages in the /var/kern.log,
/var/messages, and/or /var/syslog files, and they're filling up my /var
partition!!!
6.) 방화벽의 * tons *가 /var/kern.log, / var / messages 및 / 또는 / var / syslog 파일에서 메시지를 거부하고 있으며 / var 파티션을 채우고 있습니다.

Solution -- First, tell your ISP that you're receiving DoS (Denial of Service)
and/or portscanning (UDP connection attempts) attacks!  Look over the deny
messages to figure out what the attack is and where it's coming from.  Next,
edit /etc/ppp/cablemodem and make sure the ",nobroadcast" option is turned on
to the "cmconfig" command (uncomment that line).  If you're not receiving these
denied packets on your broadcast interface (IP address xxx.yyy.zzz.255
typically), then someone is attacking your machine in particular.  Be careful
out there....
Solution - 먼저 DoS (서비스 거부) 및 / 또는 포트 스캔 (UDP 연결 시도) 공격을 받고 있다고 ISP에 알리십시오! 거부 메시지를 살펴보고 공격이 무엇인지 어디서 왔는지 파악하십시오. 그런 다음 / etc / ppp / cablemodem을 편집하여 "nobroadcast"옵션이 "cmconfig"명령 (해당 행의 주석 처리 해제)에 설정되어 있는지 확인하십시오. 브로드 캐스트 인터페이스 (일반적으로 IP 주소 xxx.yyy.zzz.255)에서 이러한 거부 된 패킷을받지 못하면 다른 사람이 귀하의 컴퓨터를 공격하고 있습니다. 밖에서 조심해라 ....

7.) Everything seems to work fine but my computer locks up after a while
(and typically during a lengthy download through the cable modem)!
7.) 모든 것이 잘 작동하는 것처럼 보이지만 잠시 후에 (그리고 일반적으로 케이블 모뎀을 통해 오랜 시간 다운로드 할 때) 내 컴퓨터가 잠 깁니다!

Solution -- You may need to add a short delay in the driver to 'slow down' the
SURFboard because your PC might not be able to keep up with the transfer rate
of the SB1000. To do this, it's probably best to download Franco's
sb1000-1.1.2.tar.gz archive and build and install sb1000.o manually.  You'll
want to edit the 'Makefile' and look for the 'SB1000_DELAY'
define.  Uncomment those 'CFLAGS' lines (and comment out the default ones)
and try setting the delay to something like 60 microseconds with:
'-DSB1000_DELAY=60'.  Then do `make' and as root `make install' and try
it out.  If it still doesn't work or you like playing with the driver, you may
try other numbers.  Remember though that the higher the delay, the slower the
driver (which slows down the rest of the PC too when it is actively
used). Thanks to Ed Daiga for this tip!
해결책 - PC가 SB1000의 전송 속도를 따라 가지 못할 수도 있기 때문에 SURFboard를 'slow down'시키려면 드라이버에 짧은 지 연을 추가해야 할 수도 있습니다. 이를 위해 Franco의 sb1000-1.1.2.tar.gz 아카이브를 다운로드하고 수동으로 sb1000.o를 빌드하고 설치하는 것이 가장 좋습니다. 'Makefile'을 편집하고 'SB1000_DELAY'정의를 찾아야합니다. 'CFLAGS'라인의 주석 처리를 제거하고 (기본 라인을 주석으로 처리하십시오) '-DSB1000_DELAY = 60'을 사용하여 지연을 60 마이크로 초와 같이 설정하십시오. 그런 다음`make '를하고`make install'의 루트로서 그것을 시도해보십시오. 그래도 작동하지 않거나 운전 기사와 놀고 싶으면 다른 번호를 시도해보십시오. 지연이 높을수록 드라이버의 속도가 느려지므로 (실제로 사용되는 경우 PC의 나머지 부분도 느려집니다) 기억하십시오. 이 팁을 주신 Ed Daiga에게 감사드립니다!

-----------------------------------------------------------------------------

Credits:  This README came from Franco Venturi's original README file which is
still supplied with his driver .tar.gz archive.  I and all other sb1000 users
owe Franco a tremendous "Thank you!"  Additional thanks goes to Carl Patten
and Ralph Bonnell who are now managing the Linux SB1000 web site, and to
the SB1000 users who reported and helped debug the common problems listed
above.

크레디트 :이 README 파일은 Franco Venturi의 원본 README 파일에서 가져온 것으로, 드라이버의 .tar.gz 아카이브와 함께 제공됩니다. 나와 다른 모든 sb1000 사용자는 Franco에게 굉장히 감사합니다 "감사합니다!" Linux SB1000 웹 사이트를 관리하고있는 Carl Patten과 Ralph Bonnell과 위에 열거 된 일반적인 문제를보고하고 도움을 준 SB1000 사용자에게 감사드립니다.
					
               Clemmitt Sigler
					csigler@vt.edu