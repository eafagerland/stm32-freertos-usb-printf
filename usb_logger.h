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
#ifndef _USB_LOGGER_H_

void usb_logger_init(void);

#define _USB_LOGGER_H_
#endif // _USB_LOGGER_H_