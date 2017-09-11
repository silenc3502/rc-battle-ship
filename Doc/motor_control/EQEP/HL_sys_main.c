

/* USER CODE BEGIN (0) */
/* USER CODE END */

/* Include Files */

#include "HL_sys_common.h"

/* USER CODE BEGIN (1) */
#include "HL_eqep.h"
#include "HL_sys_core.h"
#include <stdio.h>
/* USER CODE END */

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

/* USER CODE BEGIN (2) */
#define UNIT_POSITION_X 60U
/* USER CODE END */

void main(void)
{
/* USER CODE BEGIN (3) */

    uint16 deltaT = 0U;
    float velocity = 0U;
    /* EQEP initialization based on GUI Configuration. */
    QEPInit();

    /* Enable Position Counter */
    eqepEnableCounter(eqepREG1);

    /* Enable Unit Timer. */
    eqepEnableUnitTimer(eqepREG1);

    /* Enable capture timer and capture period latch. */
    eqepEnableCapture(eqepREG1);

    while(1)
    {
        /* Status flag is set to indicate that a new value is latched in the QCPRD register. */
        if((eqepREG1->QEPSTS & 0x80U) !=0U)
        {
            /* Elapsed time between unit position events */
            deltaT = eqepREG1->QCPRD;

            /* Calculate Velocity from deltaT and the value of the unit position. */
            /* The value of Unit Position is a sample value and should be changed by the User as per the actual value in the UNIT_POSITION_X macro above. */
            velocity = (float)(UNIT_POSITION_X/deltaT);
            printf("velocity = %f\n", velocity);

            /* Clear the Status flag. */
            eqepREG1->QEPSTS |= 0x80U;
        }
    }
/* USER CODE END */
}

/* USER CODE BEGIN (4) */
/* USER CODE END */
