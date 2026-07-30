#ifndef KVM_PROTOCOL_H
#define KVM_PROTOCOL_H

#define KVM_PACKET_SIZE      16U
#define KVM_MAGIC_0          0xA5U
#define KVM_MAGIC_1          0x5AU
#define KVM_PROTOCOL_VERSION 0x01U
#define KVM_END_BYTE         0x0DU
#define KVM_PAYLOAD_SIZE     8U

#define KVM_TYPE_KEYBOARD    0x01U
#define KVM_TYPE_MOUSE       0x02U

#endif