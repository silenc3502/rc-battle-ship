#include <string.h>
#include <stdio.h>

#include "HL_sys_common.h"
#include "HL_sys_core.h"
#include "HL_sci.h"
#include "HL_gio.h"
#include "HL_i2c.h"
#include "HL_rti.h"

#define UART            sciREG1
#define MPU6050_ADDR    0x68

void sciDisplayText(sciBASE_t *sci, uint8 *text, uint32 len);
void mpu6050_enable(void);
void mpu6050_acc_config(void);

volatile char g_acc_xyz[6];
volatile int g_acc_flag;

void wait(uint32 t)
{
    t--;
}

int main(void)
{
    volatile int i;
    char txt_buf[256];
    unsigned int buf_len;
    signed short acc_x, acc_y, acc_z;
    double real_acc_x, real_acc_y, real_acc_z;

    gioInit();
    sciInit();

    wait(10000000);

    i2cInit();

    wait(10000000);

    mpu6050_enable();
    sprintf(txt_buf, "MPU6050 Enabled\n\r\0");
    buf_len = strlen(txt_buf);
    sciDisplayText(sciREG1, (uint8 *)txt_buf, buf_len);
    wait(200);

    mpu6050_acc_config();
    sprintf(txt_buf, "MPU6050 Accelerometer Configured\n\r\0");
    buf_len = strlen(txt_buf);
    sciDisplayText(sciREG1, (uint8 *)txt_buf, buf_len);
    wait(200);

    rtiInit();
    rtiEnableNotification(rtiREG1, rtiNOTIFICATION_COMPARE2);
    _enable_IRQ_interrupt_();
    rtiStartCounter(rtiREG1, rtiCOUNTER_BLOCK1);

    sprintf(txt_buf, "RTI Enabled\n\r\0");
    buf_len = strlen(txt_buf);
    sciDisplayText(sciREG1, (uint8 *)txt_buf, buf_len);

    for(;;)
    {
        if(g_acc_flag)
        {
            acc_x = acc_y = acc_z = 0;
            real_acc_x = real_acc_y = real_acc_z = 0.0;

            acc_x = g_acc_xyz[0];
            acc_x = acc_x << 8;
            acc_x |= g_acc_xyz[1];
            real_acc_x = ((double)acc_x) / 2048.0;

            acc_y = g_acc_xyz[2];
            acc_y = acc_y << 8;
            acc_y |= g_acc_xyz[3];
            real_acc_y = ((double)acc_y) / 2048.0;

            acc_z = g_acc_xyz[4];
            acc_z = acc_z << 8;
            acc_z |= g_acc_xyz[5];
            real_acc_z = ((double)acc_z) / 2048.0;

            sprintf(txt_buf, "acc_x = %2.5lf\tacc_y=%2.5lf\tacc_z=%2.5lf\n\r\0", real_acc_x, real_acc_y, real_acc_z);

            buf_len = strlen(txt_buf);
            sciDisplayText(sciREG1, (uint8 *)txt_buf, buf_len);

            g_acc_flag = 0;
        }
    }

    return 0;
}

void sciDisplayText(sciBASE_t *sci, uint8 *text, uint32 len)
{
    while(len--)
    {
        while((UART->FLR & 0x4) == 4)
            ;
        sciSendByte(UART, *text++);
    }
}

void mpu6050_enable(void)
{
    volatile unsigned int cnt = 2;
    unsigned char data[2] = {0x00U, 0x00U};
    unsigned char slave_word_address = 0x6bU;

    i2cSetSlaveAdd(i2cREG2, MPU6050_ADDR);
    i2cSetDirection(i2cREG2, I2C_TRANSMITTER);
    i2cSetCount(i2cREG2, cnt + 1);
    i2cSetMode(i2cREG2, I2C_MASTER);
    i2cSetStop(i2cREG2);
    i2cSetStart(i2cREG2);
    i2cSendByte(i2cREG2, slave_word_address);
    i2cSend(i2cREG2, cnt, data);

    while(i2cIsBusBusy(i2cREG2) == true)
        ;

    while(i2cIsStopDetected(i2cREG2) == 0)
        ;

    i2cClearSCD(i2cREG2);

    wait(1000000);
}

void mpu6050_acc_config(void)
{
    volatile unsigned int cnt = 1;
    unsigned char data[1] = {0x18U};
    unsigned char slave_word_address = 0x1cU;

    i2cSetSlaveAdd(i2cREG2, MPU6050_ADDR);
    i2cSetDirection(i2cREG2, I2C_TRANSMITTER);
    i2cSetCount(i2cREG2, cnt + 1);
    i2cSetMode(i2cREG2, I2C_MASTER);
    i2cSetStop(i2cREG2);
    i2cSetStart(i2cREG2);
    i2cSendByte(i2cREG2, slave_word_address);
    i2cSend(i2cREG2, cnt, data);

    while(i2cIsBusBusy(i2cREG2) == true)
        ;

    while(i2cIsStopDetected(i2cREG2) == 0)
        ;

    i2cClearSCD(i2cREG2);

    wait(1000000);
}

void rtiNotification(rtiBASE_t *rtiREG, uint32 notification)
{
    unsigned char slave_word_address = 0x3B;

    i2cSetSlaveAdd(i2cREG2, MPU6050_ADDR);
    i2cSetDirection(i2cREG2, I2C_TRANSMITTER);
    i2cSetCount(i2cREG2, 1);
    i2cSetMode(i2cREG2, I2C_MASTER);
    i2cSetStop(i2cREG2);
    i2cSetStart(i2cREG2);
    i2cSendByte(i2cREG2, slave_word_address);

    while(i2cIsBusBusy(i2cREG2) == true)
        ;

    while(i2cIsStopDetected(i2cREG2) == 0)
        ;

    i2cClearSCD(i2cREG2);

    i2cSetDirection(i2cREG2, I2C_RECEIVER);
    i2cSetCount(i2cREG2, 6);
    i2cSetMode(i2cREG2, I2C_MASTER);
    i2cSetStart(i2cREG2);

    i2cReceive(i2cREG2, 6, (unsigned char *)g_acc_xyz);
    i2cSetStop(i2cREG2);

    while(i2cIsBusBusy(i2cREG2) == true)
        ;

    while(i2cIsStopDetected(i2cREG2) == 0)
        ;

    i2cClearSCD(i2cREG2);

    g_acc_flag = 1;
}
