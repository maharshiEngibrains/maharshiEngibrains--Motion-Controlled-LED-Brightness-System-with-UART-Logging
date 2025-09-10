/******************************************************************************
* File Name:   motion_task.h
*
* Description: This file is the public interface of motion_task.c. This file 
*              also contains the BMI270 motion sensor configuration parameters.
*
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/

#include "mtb_bmi270.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cybsp.h"
#include "cyhal.h"
#include "cyhal_i2c.h"
#include "cyhal_system.h"
#include "cy_result.h"
#include "cy_retarget_io.h"
#include <stdbool.h>
#include <stdlib.h>
#include <math.h> 
#include "semphr.h"


/*******************************************************************************
* Function Prototypes
********************************************************************************/
cy_rslt_t create_motion_sensor_task(void);

/* [] END OF FILE */
