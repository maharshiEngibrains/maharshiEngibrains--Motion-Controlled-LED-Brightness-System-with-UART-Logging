/*****************************************************************************
* Project Name        :  Motion-Controlled LED Brightness System with UART Logging
* Type                :  Interrupt based reading of sensor with enable and disable feature
*
*******************************************************************************/

/*****************************************************************************
* File Name        : main.c
*
* Description      : This source file contains the main routine for non-secure
*                    application in the CM33 CPU
*
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/

#include "cyhal.h"
#include "cybsp.h"
#include "motion_task.h"
#include <stdlib.h>
#include "cy_retarget_io.h"
#include "FreeRTOS.h"
#include "task.h"


/******************************************************************************
 * Macros
 ******************************************************************************/

#define CM55_BOOT_WAIT_TIME_USEC (10U)

/*****************************************************************************
 * Function Name: main
 *****************************************************************************
 * Summary:
 * This is the main function for CM33 non-secure application.
 *    1. It initializes the device and board peripherals.
 *    2. It creates the FreeRTOS application task
 *    3. It starts the RTOS task scheduler.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 *****************************************************************************/


int main(void)
{
    cy_rslt_t result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to redirect printf/scanf to the debug UART (for console logging) */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

    /* If retarget-io init failed stop program execution */
    CY_ASSERT(CY_RSLT_SUCCESS == result);

    /* Enable CM55. */ 
    /* Enable CM55. CY_CM55_APP_BOOT_ADDR must be updated if CM55 memory layout is changed. */ 
    Cy_SysEnableCM55(CY_CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_USEC);

     printf("System Started!!!\r\n");

    /* Creating the application task for the project*/
    result = create_motion_sensor_task();
    
    /* Start the RTOS Scheduler */
    vTaskStartScheduler();

    for(;;);
}

