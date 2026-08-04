/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_custom_hid_if.c
  * @version        : v1.0_Cube
  * @brief          : USB Device Custom HID interface file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_custom_hid_if.h"

/* USER CODE BEGIN INCLUDE */
#include "kvm_protocol.h"
#include "kvm_hid.h"
#include <stdint.h>
#include <string.h>
/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device.
  * @{
  */

/** @addtogroup USBD_CUSTOM_HID
  * @{
  */

/** @defgroup USBD_CUSTOM_HID_Private_TypesDefinitions USBD_CUSTOM_HID_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Defines USBD_CUSTOM_HID_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */
#define KVM_QUEUE_SIZE 8U /* Maximum storage of 8 packages */
/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Macros USBD_CUSTOM_HID_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Variables USBD_CUSTOM_HID_Private_Variables
  * @brief Private variables.
  * @{
  */

/** Usb HID report descriptor. */
__ALIGN_BEGIN static uint8_t CUSTOM_HID_TargetReportDesc_FS[USBD_CUSTOM_HID_TARGET_REPORT_DESC_SIZE] __ALIGN_END =
{
  /* USER CODE BEGIN 0 */
  /* Keyboard: Report ID 1 */
  0x05, 0x01,       /* Usage Page (Generic Desktop) */
  0x09, 0x06,       /* Usage (Keyboard) */
  0xA1, 0x01,       /* Collection (Application) */
  0x85, 0x01,       /*   Report ID (1) */

  0x05, 0x07,       /*   Usage Page (Keyboard/Keypad) */
  0x19, 0xE0,       /*   Usage Minimum (Left Control) */
  0x29, 0xE7,       /*   Usage Maximum (Right GUI) */
  0x15, 0x00,       /*   Logical Minimum (0) */
  0x25, 0x01,       /*   Logical Maximum (1) */
  0x75, 0x01,       /*   Report Size (1) */
  0x95, 0x08,       /*   Report Count (8) */
  0x81, 0x02,       /*   Input (Data, Variable, Absolute) */

  0x95, 0x01,       /*   Report Count (1) */
  0x75, 0x08,       /*   Report Size (8) */
  0x81, 0x01,       /*   Input (Constant) */

  0x95, 0x06,       /*   Report Count (6 keys) */
  0x75, 0x08,       /*   Report Size (8) */
  0x15, 0x00,       /*   Logical Minimum (0) */
  0x25, 0x65,       /*   Logical Maximum (101) */
  0x05, 0x07,       /*   Usage Page (Keyboard/Keypad) */
  0x19, 0x00,       /*   Usage Minimum (0) */
  0x29, 0x65,       /*   Usage Maximum (101) */
  0x81, 0x00,       /*   Input (Data, Array) */
  0xC0,             /* End Collection */

  /* Mouse: Report ID 2 */
  0x05, 0x01,       /* Usage Page (Generic Desktop) */
  0x09, 0x02,       /* Usage (Mouse) */
  0xA1, 0x01,       /* Collection (Application) */
  0x85, 0x02,       /*   Report ID (2) */

  0x09, 0x01,       /*   Usage (Pointer) */
  0xA1, 0x00,       /*   Collection (Physical) */
  0x05, 0x09,       /*     Usage Page (Button) */
  0x19, 0x01,       /*     Usage Minimum (Button 1) */
  0x29, 0x05,       /*     Usage Maximum (Button 5) */
  0x15, 0x00,       /*     Logical Minimum (0) */
  0x25, 0x01,       /*     Logical Maximum (1) */
  0x95, 0x05,       /*     Report Count (5) */
  0x75, 0x01,       /*     Report Size (1) */
  0x81, 0x02,       /*     Input (Data, Variable, Absolute) */

  0x95, 0x01,       /*     Report Count (1) */
  0x75, 0x03,       /*     Report Size (3) */
  0x81, 0x01,       /*     Input (Constant) */

  0x05, 0x01,       /*     Usage Page (Generic Desktop) */
  0x09, 0x30,       /*     Usage (X) */
  0x09, 0x31,       /*     Usage (Y) */
  0x09, 0x38,       /*     Usage (Wheel) */
  0x15, 0x81,       /*     Logical Minimum (-127) */
  0x25, 0x7F,       /*     Logical Maximum (127) */
  0x75, 0x08,       /*     Report Size (8) */
  0x95, 0x03,       /*     Report Count (3) */
  0x81, 0x06,       /*     Input (Data, Variable, Relative) */
  0xC0,             /*   End Physical Collection */
  0xC0,              /* End Application Collection */
};
__ALIGN_BEGIN static uint8_t CUSTOM_HID_VendorReportDesc_FS[USBD_CUSTOM_HID_VENDOR_REPORT_DESC_SIZE] __ALIGN_END = {
  0x06, 0x00, 0xFF, /* Usage Page Vendor 0xFF00 */
  0x09, 0x01,
  0xA1, 0x01,
  0x85, 0x10,       /* Report ID 0x10 */

  0x15, 0x00,
  0x26, 0xFF, 0x00,
  0x75, 0x08,
  0x95, 0x3F,
  0x09, 0x01,
  0x91, 0x02,       /* Output: 63 bytes */

  0x95, 0x3F,
  0x09, 0x02,
  0x81, 0x02,       /* Input: 63 bytes, optional ACK/status */

  0xC0
};
uint8_t *USBD_CUSTOM_HID_GetTargetReportDesc(uint16_t *length){
  *length = sizeof(CUSTOM_HID_TargetReportDesc_FS);
  return CUSTOM_HID_TargetReportDesc_FS;
}
uint8_t *USBD_CUSTOM_HID_GetVendorReportDesc(uint16_t *length){
  *length = sizeof(CUSTOM_HID_VendorReportDesc_FS);
  return CUSTOM_HID_VendorReportDesc_FS;
}
  
  /* USER CODE END 0 */


/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Exported_Variables USBD_CUSTOM_HID_Exported_Variables
  * @brief Public variables.
  * @{
  */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */
/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_FunctionPrototypes USBD_CUSTOM_HID_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CUSTOM_HID_Init_FS(void);
static int8_t CUSTOM_HID_DeInit_FS(void);
static int8_t CUSTOM_HID_OutEvent_FS(uint8_t *report_buffer);

/**
  * @}
  */

USBD_CUSTOM_HID_ItfTypeDef USBD_CustomHID_fops_FS =
{
  CUSTOM_HID_TargetReportDesc_FS,
  CUSTOM_HID_Init_FS,
  CUSTOM_HID_DeInit_FS,
  CUSTOM_HID_OutEvent_FS
};

/** @defgroup USBD_CUSTOM_HID_Private_Functions USBD_CUSTOM_HID_Private_Functions
  * @brief Private functions.
  * @{
  */

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_Init_FS(void)
{
  /* USER CODE BEGIN 4 */
  return (USBD_OK);
  /* USER CODE END 4 */
}

/**
  * @brief  DeInitializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_DeInit_FS(void)
{
  /* USER CODE BEGIN 5 */
  return (USBD_OK);
  /* USER CODE END 5 */
}

static void KVM_SendKeyboardToTarget(const uint8_t *payload){
  uint8_t keys[6] = {0};
  /* copy payload 2-7 to key 0-5 */
  memcpy(keys, &payload[2], 6);
  (void)KVM_HID_SendKeyboard(payload[0], keys, 10);
}

static int16_t KVM_ReadInt16LE(const uint8_t *data){
  return (int16_t)((uint8_t)data[0] | (uint8_t)data[1] << 8);
}
static void KVM_SendMouseToTarget(const uint8_t *payload){
  uint8_t buttons = payload[0];
  int16_t dx = KVM_ReadInt16LE(&payload[1]);
  int16_t dy = KVM_ReadInt16LE(&payload[3]);
  int8_t wheel = (int8_t)payload[5];

  (void)KVM_HID_SendMouse(buttons, dx, dy, wheel, 10);
}

static void KVM_ReleaseAll(void){
  uint8_t keys[6] = {0};

  (void) KVM_HID_SendKeyboard(0x00, keys, 10);
  (void)KVM_HID_SendMouse(0x00, 0, 0, 0, 10);
}

/**
  * @brief  Manage the CUSTOM HID class events
  * @param  event_idx: Event index
  * @param  state: Event state
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static void KVM_ParseHostReport(uint8_t *report_buffer){
  if (report_buffer == NULL) {
    return;
  }
  if (report_buffer[0] != KVM_USB_OUT_REPORT_ID) {
    return;
  }
  if (report_buffer[1] != KVM_PROTOCOL_VERSION) {
    return;
  }
  uint8_t message_type = report_buffer[2];
  uint8_t payload_length = report_buffer[4];
  const uint8_t *payload = &report_buffer[5];

  if (payload_length > KVM_USB_OUT_PAYLOAD_SIZE) {
    return;
  }
  if (message_type == KVM_TYPE_KEYBOARD) {
    if (payload_length == 8U){
      KVM_SendKeyboardToTarget(payload);
    }
  }
  else if(message_type == KVM_TYPE_MOUSE){
    if (payload_length == 7U) {
      KVM_SendMouseToTarget(payload);
    }
  }
  else if (message_type == KVM_TYPE_RELEASE_ALL) {
    KVM_ReleaseAll();
  }
  else if (message_type == KVM_TYPE_PING) {
  
  }
}
/* Each packet size = KVM_USB_OUT_REPORT_SIZE = 64 bytes

kvm_write_index = Where to write to the next packet

kvm_read_index = Where to read from the next packet*/
typedef struct{
  uint8_t data[KVM_USB_OUT_REPORT_SIZE];
} KvmPacket;
static KvmPacket kvm_queue[KVM_QUEUE_SIZE];
static volatile uint8_t kvm_write_index = 0;
static volatile uint8_t kvm_read_index = 0;

static void KVM_QueueHostReport(uint8_t *report_buffer){
  if (report_buffer == NULL){
    return;
  }

  uint8_t next = (uint8_t)((kvm_write_index + 1U) % KVM_QUEUE_SIZE);
  /* If the next cell is not read_index, it means the queue is not full. 
  If it is full, the packet will be dropped and will not block the USB callback.*/
  if (next != kvm_read_index){
    memcpy(kvm_queue[kvm_write_index].data, report_buffer, KVM_USB_OUT_REPORT_SIZE);
    /* A memory barrier ensures that data is copied completely before updating kvm_write_index.*/
    __DMB();
    kvm_write_index = next;
  }
}

/* As long as there is data in the queue, retrieve a 64-byte packet, 
move the `read_index` to the next position, and pass this packet to `KVM_ParseHostReport()` for parsing.*/
void KVM_ProcessUsbPackets(void){
  /* kvm_read_index != kvm_write_index -> queue have data*/
  while (kvm_read_index != kvm_write_index){
    /* Establish a temporary 64-byte buffer.*/
    uint8_t packet[KVM_USB_OUT_REPORT_SIZE];
    /* copy to local packet*/
    memcpy(packet, kvm_queue[kvm_read_index].data, KVM_USB_OUT_REPORT_SIZE);
    kvm_read_index = (uint8_t)((kvm_read_index + 1U) % KVM_QUEUE_SIZE);
    /* check report ID, protocol version. Determine if Keyboard / Mouse / ReleaseAll / Ping; 
    Call KVM_HID_SendKeyboard(); Call KVM_HID_SendMouse()*/
    KVM_ParseHostReport(packet);
  }
}

static int8_t CUSTOM_HID_OutEvent_FS(uint8_t *report_buffer)
{
  /* USER CODE BEGIN 6 */
  /* Upon receiving the report_buffer, do not parse it; simply add it to the queue and immediately prepare to 
  receive the next USB OUT report.*/
  KVM_QueueHostReport(report_buffer);
  /*
   * report_buffer[0] = Report ID
   * report_buffer[1] = Protocol version
   * report_buffer[2] = Message type
   * report_buffer[3] = Sequence number
   * report_buffer[4] = Payload length
   * report_buffer[5]~[63] = Payload
   */
  /* Start next USB packet transfer once data processing is completed */
  if (USBD_CUSTOM_HID_ReceivePacket(&hUsbDeviceFS) != (uint8_t)USBD_OK)
  {
    return -1;
  }

  return (USBD_OK);
  /* USER CODE END 6 */
}

/* USER CODE BEGIN 7 */
/**
  * @brief  Send the report to the Host
  * @param  report: The report to be sent
  * @param  len: The report length
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
/*
static int8_t USBD_CUSTOM_HID_SendReport_FS(uint8_t *report, uint16_t len)
{
  return USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, report, len);
}
*/
/* USER CODE END 7 */

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
