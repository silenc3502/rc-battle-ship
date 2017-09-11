# rc-battle-ship
RM57L843(Cortex-R5F), TI AM5728 &amp; Xilinx FPGA Based RC Battleship which one has Water Rocket Precision Striking System

Now, I have to add the summary doc when I have enough time.
It has very big scale too hard to understand means of dir and file.

## Summary of Directory

<p>1. ti-processor-sdk-linux-am57xx-evm-03.02.00.05</p>
This is for TI AM5728.
Main is the Linux Kernel.
So have to make Linux Device Driver(version 4.x).
We use it for Image Processing and main controller to communicate with mobile phone that use Wi-Fi(WL1837).

<p>2. Cortex-R5F</p>
This is for TI RM57L843(Cortex-R5F MCU).
We use it to control BLDC Motor with ePWM, control Encoder with eQEP, measurement various sensor with ADC, design OC(Open Collector Circuit) with GPIO to triggering, and so on.

<p>3. Doc</p>
It has very many Documentation what we do and how.
And there are many summary of the theory.

<p>4. Zybo</p>
This is for Xilinx FPGA Zynq Zybo Board.
Same as No. 1(have to make Device Driver) - (version 4.x).

<p>5. experiment</p>
There are many experiment data to implement Battle Ship.
