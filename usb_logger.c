/********************************************************************************************
 *  Filename: usb_logger.h
 *  Author: Erik Fagerland
 *  Created On: 01/06/2024
 * 
 *  # STM32 FreeRTOS USB Logger
 *  
 *  Enables printf() to write on the USB port of your STM32 board.
 *  When using printf() it will re-route the data to be queued into the freeRTOS task.
 *  Queue is handled in USB Logger Task, writing the data out on the USB port.
 *  When the queue is empty the task will be suspended, and resumed again when
 *  items are in the queue.
 *  
 *  If you need to use printf() using floats, enable in "MCU Settings".
 *  
 *  ## How to use:
 *      1. Import usb_logger.c and usb_logger.h into your project.
 *      2. In CubeMX go to "Pinout and Configuration" -> "Connectivity" -> Enable "USB_OTG_FS" and set "Mode"
 *         to "Device_Only".
 *      3. In CubeMX go to "Pinout and Configuration" -> "Middleware and Software Packs" -> Set "USB_DEVICE" to
 *         "Communication Device Class (Virtual Port Com)".
 *      4. Make sure "Clock Configuration" is correct for use with USB port.
 *      5. Enable FreeRTOS in "Pinout and Configuration" -> "Middleware and Software Packs".
 *      6. Generate Code.
 *      7. Include "usb_logger.h" and call usb_logger_init() during system init.
 *      8. printf() can now be used to write to serial port.
 * 
 *******************************************************************************************/
#include "usb_logger.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

// Private defines
#define USB_TASK_PRIORITY           (1U)
#define USB_TASK_STACK_SIZE         (1000UL)
#define USB_LOG_QUEUE_MAX           (5U)
#define USB_TX_BUFFER_MAX_SIZE      (64UL)
#define USB_TIMEOUT_MS              (5000UL)
#define USB_TIMEOUT_RETRY_INTERVAL  (5000UL)

// Private types
typedef struct
{
    uint8_t tx_buffer[USB_TX_BUFFER_MAX_SIZE];
    uint16_t tx_len;
} USB_Task_DataStruct;

// Private Function Prototypes
static void usb_logger_task(void *pvParameters);
static void on_usb_timeout(TimerHandle_t xTimer);

//Private variables
static TaskHandle_t usb_task_handle = NULL;
static QueueHandle_t usb_queue_handle = NULL;
static BaseType_t is_usb_timeout = pdFALSE;

/********************************************************************************************
 *  Initilizes the USB Logging Task
 *******************************************************************************************/
void usb_logger_init(void)
{
    usb_queue_handle = xQueueCreate(USB_LOG_QUEUE_MAX, sizeof(USB_Task_DataStruct));
    xTaskCreate(usb_logger_task,
                "USB Logger",
                USB_TASK_STACK_SIZE,
                NULL,
                USB_TASK_PRIORITY,
                &usb_task_handle);
}

/********************************************************************************************
 *  Re-Route "printf()" to queue string in USB Logger Task
 *******************************************************************************************/
int _write(int file, char *ptr, int len)
{
    (void)file; // unused

    // Truncate to max buffer size
    if (len > USB_TX_BUFFER_MAX_SIZE)
        len = USB_TX_BUFFER_MAX_SIZE;

    USB_Task_DataStruct data;
    memcpy(data.tx_buffer, ptr, len);
    data.tx_len = len;

    BaseType_t status = xQueueSend(usb_queue_handle, &data, 0);

    if (status == pdPASS)
    {
        vTaskResume(usb_task_handle);
        return len;
    }
    else
        return USBD_FAIL;
}

/********************************************************************************************
 *  Task for writing data on USB port.
 *  When task is resumed it will check the queue for data and print
 *  on USB port if available. Task is suspended again after write operation.
 *******************************************************************************************/
static void usb_logger_task(void *pvParameters)
{
    (void)pvParameters; // unused
    TimerHandle_t timeout_timer = xTimerCreate("Timeout",
                                               USB_TIMEOUT_MS,
                                               pdTRUE,
                                               (void*)0,
                                               on_usb_timeout);

    for (;;)
    {
        uint8_t rc = USBD_OK;
        USB_Task_DataStruct queued_data;

        // Get data from queue
        BaseType_t queue_status = xQueueReceive(usb_queue_handle, &queued_data, 0);

        // Transmit data if available
        if (queue_status == pdPASS && is_usb_timeout == pdFALSE)
        {
            xTimerStart(timeout_timer, 0);
            do 
            {
                rc = CDC_Transmit_FS(queued_data.tx_buffer, queued_data.tx_len);
            } while (rc == USBD_BUSY && is_usb_timeout == pdFALSE);
        }

        // If a timeout occured on USB write it will wait and try again after retry value
        if (is_usb_timeout == pdTRUE)
        {
            vTaskDelay(USB_TIMEOUT_RETRY_INTERVAL / portTICK_PERIOD_MS);
            is_usb_timeout = pdFALSE;
            xTimerReset(timeout_timer, 0);
        }
        else
        {
            BaseType_t queue_spaces = uxQueueSpacesAvailable(usb_queue_handle);
            if (queue_spaces == USB_LOG_QUEUE_MAX) // Suspend task if queue is empty
                vTaskSuspend(usb_task_handle);
        }
    }
}

/********************************************************************************************
 *  Callback for USB Timeout Timer
 *******************************************************************************************/
static void on_usb_timeout(TimerHandle_t xTimer)
{
    (void)xTimer; // unused
    is_usb_timeout = pdTRUE;
}