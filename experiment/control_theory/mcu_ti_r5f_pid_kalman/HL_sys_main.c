#include "HL_sys_common.h"
#include "HL_system.h"
#include "HL_etpwm.h"
#include "HL_sci.h"
#include "HL_i2c.h"
#include "kalman.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// FreeRTOS 스케쥴러
#include "FreeRTOS.h"
#include "os_task.h"

#define UART			sciREG1
#define DATA_COUNT		14
#define MPU6050_ADDRESS		0x68 // MPU6050
#define RA_COUNT		1
#define MAX			38U
#define PI			3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679
#define n 2
#define m 1
#define r 1

volatile char RX_Data_Master[DATA_COUNT] = { 0 };
volatile int delay;
uint32 receiveData = 0;

unsigned int ePWM4A = 0, ePWM6B = 0, ePWM3B = 0, ePWM1B = 0;
unsigned char RA = 0x3B;
unsigned char Device_Init_Address = 0x6B;
unsigned char Device_Init_Option = 0x00;
unsigned char Gyro_Config_Address = 0x1B;
unsigned char Gyro_Init_Option = 0x18;
unsigned char Accel_Config_Address = 0x1C;
unsigned char Accel_Init_Option = 0x18;

float u;
float y;

float s_00;
float inn_00;
float K_00;
float K_10;
float AP_00;
float AP_01;
float AP_10;
float AP_11;
float APAT_00;
float APAT_01;
float APAT_10;
float APAT_11;

float buf[100];
float real_value[100];
float roll_output[200];
float pitch_output[200];
signed short temp = 0;
double accelX = 0.0, accelY = 0.0, accelZ = 0.0;
double gyroX = 0.0, gyroY = 0.0, gyroZ = 0.0;
double clk;
float dt = 0.00098147;
float roll_gyro_input;
float pitch_gyro_input;
float roll_accel_input;
float pitch_accel_input;
float roll_kalman_output = 0.0;
float pitch_kalman_output = 0.0;
kalman roll_kalman_state;
kalman pitch_kalman_state;

xTaskHandle xTask1Handle, xTask2Handle, xTask3Handle;

void kalman_init(kalman *self, float dt, float Sz_00, float Sw_00, float Sw_11);
float kalman_update(kalman *self,  float gyroscope_rate,  float accelerometer_angle);


void sciDisplayText(sciBASE_t *sci, uint8 *text,uint32 length){
	while(length--)    {
		while ((sci->FLR & 0x4) == 4);
		sciSendByte(sci,*text++);
	};
}

void catchCommand(sciBASE_t *sci) {
	while ((sci->FLR & 0x4) == 4);
	receiveData = sciReceiveByte(sci);
}

void sensorConnect() {
	// MPU-6050 장치 활성화
	i2cSetSlaveAdd(i2cREG1, MPU6050_ADDRESS);	// slave 주소
	i2cSetDirection(i2cREG1, I2C_TRANSMITTER);	// master mode
	i2cSetCount(i2cREG1, 3);			// data 양 = 3 개 전송
	i2cSetMode(i2cREG1, I2C_MASTER);		// Cortex-R5F MCU 가 Master
	i2cSetStop(i2cREG1);
	i2cSetStart(i2cREG1);
	i2cSendByte(i2cREG1, Device_Init_Address);
	i2cSendByte(i2cREG1, Device_Init_Option);
	i2cSendByte(i2cREG1, Device_Init_Option);
	while(i2cIsBusBusy(i2cREG1) == true);
	while(i2cIsStopDetected(i2cREG1) == 0);
	i2cClearSCD(i2cREG1);

	// Device 구동을 기다리는 딜레이(Datasheet 의 신호선을 보고 최적화 필요)
	for (delay = 0; delay < 10000000; delay++);

	// 자이로 센서 설정
	i2cSetSlaveAdd(i2cREG1, MPU6050_ADDRESS);	// slave 주소
	i2cSetDirection(i2cREG1, I2C_TRANSMITTER);	// master mode
	i2cSetCount(i2cREG1, 2);
	i2cSetMode(i2cREG1, I2C_MASTER);
	i2cSetStop(i2cREG1);
	i2cSetStart(i2cREG1);
	i2cSendByte(i2cREG1, Gyro_Config_Address);
	i2cSendByte(i2cREG1, Gyro_Init_Option);
	while(i2cIsBusBusy(i2cREG1) == true);
	while(i2cIsStopDetected(i2cREG1) == 0);
	i2cClearSCD(i2cREG1);

	// 가속도 센서 설정
	i2cSetSlaveAdd(i2cREG1, MPU6050_ADDRESS);	// slave 주소
	i2cSetDirection(i2cREG1, I2C_TRANSMITTER);	// master mode
	i2cSetCount(i2cREG1, 2);
	i2cSetMode(i2cREG1, I2C_MASTER);
	i2cSetStop(i2cREG1);
	i2cSetStart(i2cREG1);
	i2cSendByte(i2cREG1, Accel_Config_Address);
	i2cSendByte(i2cREG1, Accel_Init_Option);
	while(i2cIsBusBusy(i2cREG1) == true);
	while(i2cIsStopDetected(i2cREG1) == 0);
	i2cClearSCD(i2cREG1);
}

void vTask1(void *pvParameters) {
	while (1) {
		/* RA Register 설정 */
		i2cSetSlaveAdd(i2cREG1, MPU6050_ADDRESS);
		i2cSetDirection(i2cREG1, I2C_TRANSMITTER);
		i2cSetCount(i2cREG1, RA_COUNT);
		i2cSetMode(i2cREG1, I2C_MASTER);
		i2cSetStop(i2cREG1);
		i2cSetStart(i2cREG1);
		i2cSendByte(i2cREG1, RA);
		while(i2cIsBusBusy(i2cREG1) == true);
		while(i2cIsStopDetected(i2cREG1) == 0);
		i2cClearSCD(i2cREG1);

		/* Data 수신 */
		i2cSetSlaveAdd(i2cREG1, MPU6050_ADDRESS);
		i2cSetDirection(i2cREG1, I2C_RECEIVER);
		i2cSetCount(i2cREG1, DATA_COUNT);
		i2cSetMode(i2cREG1, I2C_MASTER);
		i2cSetStop(i2cREG1);
		i2cSetStart(i2cREG1);
		i2cReceive(i2cREG1, DATA_COUNT, (unsigned char *)RX_Data_Master);
		while(i2cIsBusBusy(i2cREG1) == true);
		while(i2cIsStopDetected(i2cREG1) == 0);
		i2cClearSCD(i2cREG1);

		/* 터미널 출력 */
		temp = RX_Data_Master[0];
		temp = temp << 8;
		temp |= RX_Data_Master[1];
		accelX = (double)temp / 2048.0;

		temp = RX_Data_Master[2];
		temp = temp << 8;
		temp |= RX_Data_Master[3];
		accelY = (double)temp / 2048.0;

		temp = RX_Data_Master[4];
		temp = temp << 8;
		temp |= RX_Data_Master[5];
		accelZ = (double)temp / 2048.0;

		temp = RX_Data_Master[8];
		temp = temp << 8;
		temp |= RX_Data_Master[9];
		gyroX = (double)temp / 16.4;

		temp = RX_Data_Master[10];
		temp = temp << 8;
		temp |= RX_Data_Master[11];
		gyroY = (double)temp / 16.4;

		temp = RX_Data_Master[12];
		temp = temp << 8;
		temp |= RX_Data_Master[13];
		gyroZ = (double)temp / 16.4;
	}
}

void vTask2(void *pvParameters) {
	while(1) {
		for (delay = 0; delay<100000; delay++);
		if (ePWM6B >= MAX*2) ePWM6B = 0;
		else ePWM6B++;
		etpwmSetCmpB(etpwmREG6, ePWM6B);


		if (ePWM1B >= MAX) {
			for (delay = 0; delay<250000; delay++);
			ePWM1B-- ;
		}
		else {
			for (delay = 0; delay<250000; delay++);
			ePWM1B++;
		}

		// 터미널을 통해 들어오는 신호 감지
		if ((UART->FLR & 0x200) != 0) {
			catchCommand(UART);
			if (receiveData == 52) {
				if (ePWM1B >= 2) ePWM1B = ePWM1B - 2;
				else ePWM1B = 0;
			}
			if (receiveData == 54) {
				if (ePWM1B < (MAX*2)-2)	ePWM1B = ePWM1B + 2;
				else ePWM1B = MAX*2;
			}
			if (receiveData == 53) {
				ePWM1B = 0;
			}
		}
		etpwmSetCmpB(etpwmREG1, ePWM1B);
		UART->FLR &= 0xFDFF;
	}
}


void vTask3(void *pvParameters) {
	while(1) {
		for (delay = 0; delay < 250000; delay++);
		dt += 0.00098147;

		roll_gyro_input = gyroX;
		pitch_gyro_input = gyroY;
		roll_accel_input = atan(accelY/sqrt(accelX*accelX + accelZ*accelZ))*180/PI;
		pitch_accel_input = atan(accelX/sqrt(accelY*accelY + accelZ*accelZ))*180/PI;

		roll_kalman_output = kalman_update(&roll_kalman_state, roll_gyro_input, roll_accel_input);
		sprintf(roll_output, "roll_gyro(x)=%7.3f deg/sec\troll_accel(x)=%7.3f deg\troll_kalman_output(x)=%10.4f deg",
				roll_gyro_input, roll_accel_input, roll_kalman_output);
		sciDisplayText(UART, (uint8 *)roll_output, strlen(roll_output));
		sciSendByte(UART, 10);
		sciSendByte(UART, 13);

		pitch_kalman_output = kalman_update(&pitch_kalman_state, pitch_gyro_input, pitch_accel_input);
		sprintf(pitch_output, "pitch_gyro(x)=%7.3f deg/sec\tpitch_accel(x)=%7.3f deg\tpitch_kalman_output(x)=%10.4f deg",
				pitch_gyro_input, pitch_accel_input, pitch_kalman_output);
		sciDisplayText(UART, (uint8 *)pitch_output, strlen(pitch_output));
		sciSendByte(UART, 10);
		sciSendByte(UART, 13);
		sciSendByte(UART, 10);
		sciSendByte(UART, 13);

	}
}

void main(void)
{
	sciInit();
	i2cInit();
	etpwmInit();
	sensorConnect();
	kalman_init(&roll_kalman_state, dt, 1.000, 0.001, 0.003);
	kalman_init(&pitch_kalman_state, dt, 1.000, 0.001, 0.003);

	// Task 1 생성
	xTaskCreate(vTask1, "sensor", 256, NULL, 1, &xTask1Handle);

	// Task 2 생성
	xTaskCreate(vTask2, "pwm", 256, NULL, 1, xTask2Handle);

	// Task 3 생성
	xTaskCreate(vTask3, "kalman", 512, NULL, 1, xTask3Handle);

	// Scheduler 구동!
	vTaskStartScheduler();

	while(1);
}

void kalman_init(kalman *self, float dt, float Sz_00, float Sw_00, float Sw_11)
{
	self->x_00 = 0.0;
	self->x_10 = 0.0;

	self->P_00 = 3.0;
	self->P_01 = 0.0;
	self->P_10 = 0.0;
	self->P_11 = 3.0;

	self->A_00 = 1.0;
	self->A_01 = -dt;
	self->A_10 = 0.0;
	self->A_11 = 1.0;

	self->B_00 = dt;

	self->Sz_00 = Sz_00;

	self->Sw_00 = Sw_00;
	self->Sw_11 = Sw_11;
}

float kalman_update(kalman *self,  float gyro_rate,  float accel_angle)
{
	u = gyro_rate;
	y = accel_angle;

	self->x_00 = (self->A_01 * self->x_10) + (self->B_00 * u);

	inn_00 = y - self->x_00;

	s_00 = self->P_00 + self->Sz_00;

	AP_00 = self->P_00 + self->A_01 * self->P_10;
	AP_01 = self->P_01 + self->A_01 * self->P_11;
	AP_10 = self->P_10;
	AP_11 = self->P_11;

	K_00 = AP_00 / s_00;
	K_10 = AP_10 / s_00;

	self->x_00 += K_00 * inn_00;
	self->x_10 += K_10 * inn_00;

	APAT_00 = AP_00 + (AP_01 * self->A_01);
	APAT_01 = AP_01;
	APAT_10 = AP_10 + (AP_11 * self->A_01);
	APAT_11 = AP_11;

	self->P_00 = APAT_00 + self->Sw_00;
	self->P_01 = APAT_01;
	self->P_10 = APAT_10;
	self->P_11 = APAT_11 + self->Sw_11;

	return self->x_00;
}
