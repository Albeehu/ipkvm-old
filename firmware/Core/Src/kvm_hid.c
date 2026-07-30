#include "kvm_hid.h"

#include "main.h"
#include "usb_device.h"
#include "usbd_customhid.h"

#include <stdint.h>
#include <string.h>

extern USBD_HandleTypeDef hUsbDeviceFS;

/* USB transmit is asynchronous, so the transmitted buffer must remain valid until the USB transfer finishes. Do not directly transmit a temporary
local buffer.*/

/* buffer,size 16 bytes*/
static uint8_t hid_tx_buffer[16];
static KVM_HID_Status hid_last_status = KVM_HID_STATUS_OK;

KVM_HID_Status KVM_HID_GetLastStatus(void)
{
    return hid_last_status;
}

/* mouse value: -127~127*/
static int16_t clamp_mouse_value(int16_t value){
    if(value > 127){
        return 127;
    }
    if (value < -127){
        return -127;
    }
    return value;
}

/* Send USB HID report*/
static bool KVM_HID_SendReportBlocking(const uint8_t *report, uint16_t length, uint32_t timeout_ms){
    uint32_t start_tick = HAL_GetTick(); /*The number of milliseconds elapsed since the system started.*/

    hid_last_status = KVM_HID_STATUS_OK;

    if ((report == NULL) || (length == 0U) || (length > sizeof(hid_tx_buffer))){
        hid_last_status = KVM_HID_STATUS_INVALID_ARG;
        return false;
    }

    /*Wait until Target has completed USB enumeration.*/
    while (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED){
        /*The USB device must first be configured by the host, 
        and its status must be: USBD_STATE_CONFIGURED. If it has 
        not been configured, wait every 1 ms until timeout -> return false.*/
        if ((HAL_GetTick() - start_tick) >= timeout_ms) {
            hid_last_status = KVM_HID_STATUS_NOT_CONFIGURED_TIMEOUT;
            return false;
        }
        HAL_Delay(1);
    }

    /* hUsbDeviceFS.pClassData is internal data for the USB class.*/
    USBD_CUSTOM_HID_HandleTypeDef *hid =
        (USBD_CUSTOM_HID_HandleTypeDef *)hUsbDeviceFS.pClassDataCmsit[hUsbDeviceFS.classId];

    /*The HID class data failed to initialize before it was initialized.*/
    if (hid == NULL){
        hid_last_status = KVM_HID_STATUS_NO_CLASS_DATA;
        return false;
    }

    /* wait for previous HID transfer.
    If the previous HID report is still being transmitted, the buffer cannot be directly overwritten.*/
    while (hid -> state != CUSTOM_HID_IDLE){
        if ((HAL_GetTick() - start_tick) >= timeout_ms) {
            hid_last_status = KVM_HID_STATUS_PREVIOUS_BUSY_TIMEOUT;
            return false;
        }
        HAL_Delay(1);
    }

    /*Copy the report to the static buffer*/
    memcpy(hid_tx_buffer, report, length);

    /*The actual USB HID report should be sent.
    If the returned report is not USBD_OK, it means the transmission failed.*/
    if (USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, hid_tx_buffer, length) != USBD_OK){
        hid_last_status = KVM_HID_STATUS_SEND_REPORT_FAIL;
        return false;
    }

    /*wait for completion so hid_tx_buffer will not be modifier
    while the USB peripheral is still transmitting it*/
    while (hid -> state != CUSTOM_HID_IDLE) {
        if ((HAL_GetTick() - start_tick) >= timeout_ms) {
            hid_last_status = KVM_HID_STATUS_COMPLETE_TIMEOUT;
            return false;
        }
        HAL_Delay(1);
    }
    return true;
}

bool KVM_HID_SendKeyboard(uint8_t modifiers, const uint8_t keys[6], uint32_t timeout_ms){
    /*Btye 0: Report ID = 1
    Btye 1: modifier bitmap
    Btye 2: Reserved
    Btye 3-8: Up to simultaneous keys*/
    uint8_t report[9] = {
        0x01,
        modifiers,
        0x00,
        0x00, 0x00, 0x00,
        0x00, 0x00, 0x00
    };

    if (keys != NULL){
        memcpy(&report[3], keys, 6);
    }
    return KVM_HID_SendReportBlocking(report, sizeof(report), timeout_ms);
}

bool KVM_HID_SendMouse(uint8_t buttons, int16_t dx, int16_t dy, int16_t wheel, uint32_t timeout_ms){
    dx = clamp_mouse_value(dx);
    dy = clamp_mouse_value(dy);
    wheel = clamp_mouse_value(wheel);

    /*Btye 0: Report ID = 2
    Btye 1: Mouse button bitmap
    Btye 2: Relative X
    Btye 3: Relative Y
    Btye 4: Wheel */
    uint8_t report[5] ={
        0x02,
        buttons, (uint8_t)((int8_t)dx), (uint8_t)((int8_t)dy), (uint8_t)((int8_t)wheel)
    };

    return KVM_HID_SendReportBlocking(report, sizeof(report), timeout_ms);
}

