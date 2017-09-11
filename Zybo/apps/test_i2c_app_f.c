#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define I2C_FILE_NAME "/dev/i2c-0"
#define MPU6050_ADDR 0x68
#define ARGS_MSG "Usage : \n./test_i2c_app_f [slave address]"

int main(int argc, char **argv) {
	int fd;
	struct i2c_rdwr_ioctl_data msgset;
	struct i2c_msg msgs[2];
	unsigned char buf[3];
	unsigned char *out = 0;

	printf("Try open file\n");
	if((fd = open(I2C_FILE_NAME, O_RDWR)) < 0) {
		perror("Failed to open file\n");
		return -1;
	}
	printf("Successfully %s File opened!\n\n", I2C_FILE_NAME);

	printf("Try connect slave addr\n");
	if(ioctl(fd, I2C_SLAVE_FORCE, 0x68) < 0) {
		perror("Failed to connect slave addr\n");
		return -1;
	}
	printf("Successfully connect slave addr\n\n");

	printf("Try MPU6050 Enable");
	buf[0] = 0x6B;
	buf[1] = 0x0;
	buf[2] = 0x0;
	if(write(fd, buf, 3) < 3) {
		printf(" 0x%x, 0x%x to 0x%x\n", buf[1], buf[2], buf[0]);
		perror("Failed to send data");
		return -1;
	}
	printf(" Successfully send data to 0x%x, data : 0x%x, 0x%x\n", buf[0], buf[1], buf[2]);

	

	return 0;
}