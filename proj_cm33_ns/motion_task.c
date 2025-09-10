/******************************************************************************
 * File Name:   motion_task.c
 *
 * Description: This file contains the task that initializes and configures the
 *              BMI270 Motion Sensor, it starts the detection of the angle of orientation, 
 *              adjusting the LED brightness based on the angle using PWM and displays the
 *              sensor orientation when the button 2 is pressed and stops the detection
 *              whenthe button 1 is pressed.
 *
 *******************************************************************************/


/*******************************************************************************
* Header Files
*******************************************************************************/

#include "motion_task.h"


/******************************************************************************
 * Macros
 ******************************************************************************/

/* I2C Clock frequency in Hz */
#define I2C_CLK_FREQ_HZ                 (400000U)

/*Buffer size for log in UART*/
#define MAX_MSG_SIZE 100

/*******************************************************************************
 * Global Variables
 ********************************************************************************/

/* HAL structure for PWM */
cyhal_pwm_t pwm_led;

/* HAL structure for I2C */
static cyhal_i2c_t kit_i2c;

/* Instance of BMI270 sensor structure */
struct bmi2_dev dev;

/*Initializing the Queue and Semaphore instances*/
QueueHandle_t xQueue;
QueueHandle_t xQueueForLogging;
SemaphoreHandle_t xSemaphore;

/* Sensor status variable for enabling and disabling the sensor detection*/
static bool sensor_state = false;


/*******************************************************************************
 * Function Name: motion_sensor_init
 ********************************************************************************
 * Summary:
 *  Function that configures the I2C master interface and then initializes
 *  the motion sensor.
 *
 * Parameters:
 *  None
 *
 * Return:
 * result
 *
 *******************************************************************************/
static cy_rslt_t motion_sensor_init(void)
{
    cy_rslt_t result;

    /* I2C configuration structure */
    cyhal_i2c_cfg_t kit_i2c_cfg =
    {
            .is_slave = false,
            .address = 0,
            .frequencyhal_hz = I2C_CLK_FREQ_HZ
    };

    /* Initialize the I2C master interface for BMI270 motion sensor */
    result = cyhal_i2c_init(&kit_i2c, (cyhal_gpio_t) CYBSP_I2C_SDA,
            (cyhal_gpio_t) CYBSP_I2C_SCL, NULL);
    if(CY_RSLT_SUCCESS != result)
    {
        printf(" Error : I2C initialization failed !!\r\n");
        CY_ASSERT(0);
    }

    /* Configure the I2C master interface with the desired clock frequency */
    result = cyhal_i2c_configure(&kit_i2c, &kit_i2c_cfg);
    if(CY_RSLT_SUCCESS != result)
    {
        printf(" Error : I2C configuration failed !!\r\n");
        CY_ASSERT(0);
    }

    /* Initialize the BMI270 motion sensor */
    result = mtb_bmi270_init(&dev, &kit_i2c);
    if(CY_RSLT_SUCCESS != result)
    {
        printf(" Error : IMU sensor init failed !!\r\n");
        CY_ASSERT(0);
    }

    result = mtb_bmi270_config(&dev);
    if(CY_RSLT_SUCCESS != result)
    {
        printf(" Error : IMU sensor config failed !!\r\n");
        CY_ASSERT(0);
    }

    return result;
}

/*******************************************************************************
 * Function Name: motion_sensor_update_orientation
 ********************************************************************************
 * Summary:
 *  Function that updates the orientation status to one of the 6 types,
 *  'ORIENTATION_UP, ORIENTATION_DOWN, TOP_EDGE, BOTTOM_EDGE,
 *  LEFT_EDGE, and RIGHT_EDGE'. This functions detects the axis that is most perpendicular
 *  to the ground based on the absolute value of acceleration in that axis.
 *  The sign of the acceleration signifies whether the axis is facing the ground
 *  or the opposite.This function also compute the angle degree of the motion from the tilt 
 *  angle.This function sends the data and current orientation data for log to the queue.
 *
 * Return:
 *  CY_RSLT_SUCCESS upon successful orientation update, else a non-zero value
 *  that indicates the error.
 *
 *******************************************************************************/
static cy_rslt_t motion_sensor_update_orientation(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    struct bmi2_sens_data sensor_data = {0};

    /* Read accelerometer data */
    result = mtb_bmi270_get_sensor_data(&dev, &sensor_data);
    if (CY_RSLT_SUCCESS != result)
    {
        printf("read data failed\r\n");
        return result;
    }

    /* Compute tilt angle */
    float x = (float)sensor_data.acc.x;
    float y = (float)sensor_data.acc.y;
    float z = (float)sensor_data.acc.z;

    float magnitude = sqrtf(x*x + y*y + z*z);

    float angle_deg = acosf(z / magnitude) * (180.0f / M_PI);

    BaseType_t status;

    status = xQueueSendToBack(xQueue,&angle_deg,portMAX_DELAY);

    if(status != pdPASS){
        printf("Cannot able to send to queue!!");
    }

    /* Keep your orientation detection */
    int16_t abs_x = abs(sensor_data.acc.x);
    int16_t abs_y = abs(sensor_data.acc.y);
    int16_t abs_z = abs(sensor_data.acc.z);

    char str[MAX_MSG_SIZE];

    if ((abs_z > abs_x) && (abs_z > abs_y))
    {
        if (sensor_data.acc.z < 0){
            snprintf(str,sizeof(str),"Orientation = ORIENTATION_DOWN\r\n",NULL);
            xQueueSendToBack(xQueueForLogging,str,portMAX_DELAY);
        }
        else{
            snprintf(str,sizeof(str),"Orientation = ORIENTATION_UP\r\n",NULL);
            xQueueSendToBack(xQueueForLogging,str,portMAX_DELAY);
        }

    }
    else if ((abs_y > abs_x) && (abs_y > abs_z))
    {
        if (sensor_data.acc.y > 0){
            snprintf(str,sizeof(str),"Orientation = ORIENTATION_BOTTOM_EDGE\r\n",NULL);
            xQueueSendToBack(xQueueForLogging,str,portMAX_DELAY);
        }
        else{
            snprintf(str,sizeof(str),"Orientation = ORIENTATION_TOP_EDGE\r\n",NULL);
            xQueueSendToBack(xQueueForLogging,str,portMAX_DELAY);
        }
    }
    else
    {
        if (sensor_data.acc.x < 0){
            snprintf(str,sizeof(str),"Orientation = ORIENTATION_RIGHT_EDGE\r\n",NULL);
            xQueueSendToBack(xQueueForLogging,str,portMAX_DELAY);
        }
        else{
            snprintf(str,sizeof(str),"Orientation = ORIENTATION_LEFT_EDGE\r\n",NULL);
            xQueueSendToBack(xQueueForLogging,str,portMAX_DELAY);
        }
    }

    return result;
}

/*******************************************************************************
 * Function Name: Sensor_Task
 ********************************************************************************
 * Summary:
 *  Task that configures the Motion Sensor and sends the sensor data to the queue
 *  and send the current orientation to the logging queue by checking the sensor status 
 *  that the sensor is enabled or disabled with the usage of binary semaphore.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  None
 *
 *******************************************************************************/
static void Sensor_Task(void* pvParameters)
{
    /* Status variable to indicate the result of various operations */
    cy_rslt_t result;

    printf("***************************************************************************\n");
    printf("    PSoC Edge MCU: Interfacing IMU Sensor Through I2C (FreeRTOS)    \n");
    printf("***************************************************************************\r\n");

    /* Initialize BMI270 motion sensor and suspend the task upon failure */
    result = motion_sensor_init();
    if(CY_RSLT_SUCCESS != result)
    {
        printf(" Error : Motion Sensor initialization failed !!\n Check hardware connection\r\n");
        CY_ASSERT(0);
    }
    printf("BMI270 Motion Sensor successfully initialized.\r\n");

    for(;;)
    {
        if(sensor_state == true){
            printf("Sensor Task\r\n");
            xSemaphoreTake(xSemaphore, portMAX_DELAY);
            /* Get current orientation and send the degree to the queue*/
            motion_sensor_update_orientation();
            xSemaphoreGive(xSemaphore);
            vTaskDelay( pdMS_TO_TICKS(50));
        }
        else{
            printf("Sensor Stopped!!\r\n");
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
        
    }
}

/*******************************************************************************
 * Function Name: block_ISR
 ********************************************************************************
 * Summary:
 *  This ISR is called when the button 1 pressed to disable the sensor.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  None
 *
 *******************************************************************************/
static void block_ISR(void *arg, cyhal_gpio_event_t event){
    sensor_state = false;
}


/*******************************************************************************
 * Function Name: LED_Control_Task
 ********************************************************************************
 * Summary:
 *  This task initialize the PWM with the CYBSP_USER_LED1 and start the PWM.It receives
 *  the angle from the xQueue and compute the duty cycle using proper equations to map 
 *  the angle degree with the PWM percentages.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  None
 *
 *******************************************************************************/
static void LED_Control_Task(void *pvParams){

    BaseType_t status;
    float angle;

        status = cyhal_pwm_init(&pwm_led, CYBSP_USER_LED1, NULL);
        CY_ASSERT(CY_RSLT_SUCCESS == status);

        status = cyhal_pwm_start(&pwm_led);
        CY_ASSERT(CY_RSLT_SUCCESS == status);
        char str[MAX_MSG_SIZE];

    for(;;){

        status = xQueueReceive(xQueue,&angle,portMAX_DELAY);

        float duty = (angle/180.0) * 100.0;


        cyhal_pwm_set_duty_cycle(&pwm_led, duty, 1000);

        snprintf(str,sizeof(str),"Angle is : %.2f\r\n",angle);
        xQueueSendToBack(xQueueForLogging,str,portMAX_DELAY);

        snprintf(str,sizeof(str),"LED Brightness Duty Cycle = %.1f %%\r\n",duty);
        xQueueSendToBack(xQueueForLogging,str,portMAX_DELAY);
    }
}

/*******************************************************************************
 * Function Name: UARTLogTask
 ********************************************************************************
 * Summary:
 *  This task receives the log messages like orientation, angle and duty cycle from the queue 
 *  and print messages to the debug UART.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  None
 *
 *******************************************************************************/
static void UARTLogTask(void *pvParameters){

    BaseType_t status;
    char str[MAX_MSG_SIZE];

    for(;;){
        status = xQueueReceive(xQueueForLogging,&str,portMAX_DELAY);

        printf("%s",str);
    }
}

/*******************************************************************************
 * Function Name: btn_pressed
 ********************************************************************************
 * Summary:
 *  This ISR is called when the button 2 pressed to enable the sensor and giving the semaphore to
 *  to the sensor task to start the detection by checking sensor status.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  None
 *
 *******************************************************************************/
static void btn_pressed(void *arg, cyhal_gpio_event_t event){
    BaseType_t flag = pdFALSE;
    sensor_state = true;
    xSemaphoreGiveFromISR(xSemaphore, &flag);
    portYIELD_FROM_ISR(flag);
}

/*******************************************************************************
 * Function Name: create_motion_sensor_task
 ********************************************************************************
 * Summary:
 *  Function that initialize button 1 and 2 and registering the ISR to the perticular button
 *  and creates the motion sensor task,LED_Control_Task and the UARTLogTask.
 *
 * Parameters:
 *  None
 *
 * Return:
 *  CY_RSLT_SUCCESS upon successful creation of the all task, else a non-zero value 
 *  that indicates the error.
 *
 *******************************************************************************/
cy_rslt_t create_motion_sensor_task(void)
{
    BaseType_t status;

    xQueue = xQueueCreate(5,sizeof(float));
    xQueueForLogging = xQueueCreate(5,MAX_MSG_SIZE);
    xSemaphore = xSemaphoreCreateBinary();

    cyhal_gpio_init(CYBSP_USER_BTN2, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    cyhal_gpio_init(CYBSP_USER_BTN1, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);

    
    static cyhal_gpio_callback_data_t btn_interrupt;
    btn_interrupt.callback = btn_pressed;
    btn_interrupt.callback_arg = NULL;

    cyhal_gpio_register_callback(CYBSP_USER_BTN2,  &btn_interrupt);
    cyhal_gpio_enable_event(CYBSP_USER_BTN2, CYHAL_GPIO_IRQ_FALL , 4, true );

   
     static cyhal_gpio_callback_data_t btn_for_stop;
    btn_for_stop.callback = block_ISR;
    btn_for_stop.callback_arg = NULL;

    cyhal_gpio_register_callback(CYBSP_USER_BTN1,  &btn_for_stop);
    cyhal_gpio_enable_event(CYBSP_USER_BTN1, CYHAL_GPIO_IRQ_FALL , 4, true );


     status = xTaskCreate(Sensor_Task, "Motion Sensor Task", 1024,
            NULL, 3, NULL);

     status = xTaskCreate(LED_Control_Task, "LED Task", 1024, NULL, 2, NULL);

     status = xTaskCreate(UARTLogTask, "UART LOG Task", 1024, NULL, 1, NULL);

    return (pdPASS == status) ? CY_RSLT_SUCCESS : (cy_rslt_t) status;
}

/* [] END OF FILE */
