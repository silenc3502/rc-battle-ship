1. Build uio_int_test.c
arm-linux-gnueabihf-gcc uio_int_test.c -lrt -o uio_int_test.out

2. Build gen_ipc_int.c
arm-linux-gnueabihf-gcc gen_ipc_int.c -lrt -o gen_ipc_int.out

3. copy uio_int_test.out & gen_ipc_int.out to filesystem

4. Use k2hk-evm-test.dts and convert into dtb file. Use it to boot using dtb.

5. Run ./uio_int_test.out hyperlink0 10
 ( This will make this application wait for interrupts)

6. Open seperate telnet window and run 
./gen_ipc_int.out 10



