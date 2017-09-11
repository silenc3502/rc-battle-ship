


/* USER CODE BEGIN (0) */
/* USER CODE END */

/* Include Files */

#include "HL_sys_common.h"
#include "HL_system.h"
#include "HL_etpwm.h"
#include "HL_sci.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define TSIZE1  10
#define TSIZE2  5
#define TSIZE3  4

uint8 TEXT1[TSIZE1] = {'H', 'e', 'l', 'l', 'o', ' ', 'T', 'I', ' ', 'C', 'o', 'r', 't', 'e', 'x', '-', 'R', '5', 'F', ' ', 'M', 'C', 'U'};

unsigned int ePWM1B = 90;

void sciDisplayText(sciBASE_t *sci, uint32 *text, uint32 length);
void wait(uint32 time);

#define UART sciREG1

uint32 receiveData = 0;
uint32 temp = 0;
float flat = 1;
int ten = 10;
int squre = 3;


int main(void)
{
    sciInit();      /* SCI/SCI-Lin 초기화, 짝수 패리티, Stop Bits : 2 */
    etpwmInit();


    while(1)
    {
        etpwmStartTBCLK();

        squre = 3;      // 10 의 제곱
        receiveData = 0;
        while(1)
        {
            temp = sciReceiveByte(UART);
            sciDisplayText(UART, &temp, TSIZE1);    /* Text 전송 */

            temp = temp-48;
            if(temp > 9 || temp < 0)
                break;
            receiveData += temp * pow(ten, squre);
            printf("receiveData = %d\n", receiveData);
            squre--;
        }

        if(receiveData > 0){
            ePWM1B = receiveData;
            etpwmSetCmpB(etpwmREG1, ePWM1B);
            printf("11 ePWM1B = %d\n", ePWM1B);
            wait(4000);
        }else{
            printf("-------\n");
        }


        wait(4000);
    }

}

void sciDisplayText(sciBASE_t *sci, uint32 *text, uint32 length)
{
    while(length--)
    {
        while((UART->FLR & 0x4) == 4)
            ;   /* wait until busy */
        sciSendByte(UART, *text++);     /* send out text */
    }
}

void wait(uint32 time)
{
    time--;
}
/* USER CODE BEGIN (4) */
/* USER CODE END */
