/*來宣告 KVM HID 相關 API 意思是如果還沒有定義 KVM_HID_H
就定義 KVM_HID_H並包含下面的內容*/
#ifndef KVM_HID_H
#define KVM_HID_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    KVM_HID_STATUS_OK = 0U,
    KVM_HID_STATUS_INVALID_ARG = 1U,
    KVM_HID_STATUS_NOT_CONFIGURED_TIMEOUT = 2U,
    KVM_HID_STATUS_NO_CLASS_DATA = 3U,
    KVM_HID_STATUS_PREVIOUS_BUSY_TIMEOUT = 4U,
    KVM_HID_STATUS_SEND_REPORT_FAIL = 5U,
    KVM_HID_STATUS_COMPLETE_TIMEOUT = 6U,
} KVM_HID_Status;

/*modifiers 代表修飾鍵 -> ctrl、Shift、Alt、GUI / Windows / Command 
keys[6]代表最多同時按下的 6 個普通鍵
等待送出完成的 timeout，單位是毫秒*/
bool KVM_HID_SendKeyboard(uint8_t modifiers, const uint8_t keys[6], uint32_t timeout_ms);

/*buttons 滑鼠按鍵狀態，例如左鍵、右鍵、中鍵，通常也是 bit mask
滑鼠相對位移量：
dx > 0往右 dx < 0往左 dy > 0上 dy < 0下
int...有號整數*/
bool KVM_HID_SendMouse(uint8_t buttons, int16_t dx, int16_t dy, int16_t wheel, uint32_t timeout_ms);

KVM_HID_Status KVM_HID_GetLastStatus(void);

#endif
