#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define I2C_FILE_NAME "/dev/i2c-0"
#define USAGE_MESSAGE "Usage : \n" \
						"%s r [addr] [register] to read value frome [register] \n" \
						"%s w [addr] [register] [value] to write a value [value] to register [register]\n"

static int set_i2c_register(int file, unsigned char addr, unsigned char reg, unsigned char value) {
	unsigned char outbuf[2];	 // write할 때 사용할 버퍼
	struct i2c_rdwr_ioctl_data msgset; //i2c_msg(간단한 메시지 배열 포인터)와 nmsg(교환될 메시지 수)를 가지고 있는 구조체 msgset
	struct i2c_msg msgs[1];

	msgs[0].addr = addr;
	msgs[0].flags = 0;
	msgs[0].len = sizeof(outbuf);
	msgs[0].buf = outbuf;

	outbuf[0] = reg; // outbuf의 첫 번째 byte는 우리가 쓸 주소를 알리는 용도로 사용한다. (first 7bits)
	outbuf[1] = value; // 두 번째는 쓸 값을 넣는다. 테스트 용이 아니면 일반적으로 더 큰 outbuf를 잡아준다

	/* Transfer the i2c packets to the kernel and verify it worked */
	msgset.msgs = msgs;
	msgset.nmsgs = 1;
	if(ioctl(file, I2C_RDWR, &msgset) < 0) {
		perror("Unable to send data\n");
		return 1;
	}

	return 0;
}

static int get_i2c_register(int file, unsigned char addr, unsigned char reg, unsigned char *val) {
	unsigned char inbuf, outbuf;
	struct i2c_rdwr_ioctl_data msgset;
	struct i2c_msg msgs[2];


	/*
     * In order to read a register, we first do a "dummy write" by writing
     * 0 bytes to the register we want to read from.  This is similar to
     * the packet in set_i2c_register, except it's 1 byte rather than 2.
     */
	outbuf = reg;
	msgs[0].addr = addr;
	msgs[0].flags = 0;
	msgs[0].len = sizeof(outbuf);
	msgs[0].buf = &outbuf;

	/* The data will get returned in this structure */
	msgs[1].addr = addr;
	msgs[1].flags = I2C_M_RD/* | I2C_M_NOSTART*/; // 이 flag로 read 할지 write 할지를 결정한다. 0은 write
	msgs[1].len = sizeof(inbuf);
	msgs[1].buf = &inbuf;

	/* Send the request to the kernel and get the result back */
	msgset.msgs = msgs;
	msgset.nmsgs = 2;
	if(ioctl(file, I2C_RDWR, &msgset) < 0) {
		perror("Unable to send data\n");
		return 1;
	}
	*val = inbuf;

	return 0;
}

int main(int argc, char **argv) {
	int i2c_file;

	// Open a connection to the I2C userpace control file U to D
	if((i2c_file = open(I2C_FILE_NAME, O_RDWR)) < 0) {
		perror("Unable to open i2c control file\n");
		exit(1);
	}

	if(argc > 3 && !strcmp(argv[1], "r")) { // strcmp return 0 if its true
		int addr = strtol(argv[2], NULL, 0); // string to long
		int reg = strtol(argv[2], NULL, 0);
		unsigned char value;
		if(get_i2c_register(i2c_file, addr, reg, &value)) printf("Unable to get register!\n");
		else printf("Register %d: %d (%x)\n", reg, (int)value, (int)value);
	}else if(argc > 4 && !strcmp(argv[1], "w")) {
		int addr = strtol(argv[2], NULL, 0);
		int reg = strtol(argv[3], NULL, 0);
		int value = strtol(argv[4], NULL, 0);
		if(set_i2c_register(i2c_file, addr, reg, value)) printf("Unable to get register!\n");
		else printf("Set register %x: %d (%x)\n", reg, value, value);
	}else fprintf(stderr, USAGE_MESSAGE, argv[0], argv[0]);

	close(i2c_file);

	return 0;
}