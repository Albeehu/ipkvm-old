#test find vendor HID, open device, group packet, send ping & keyboard, release all, move mouse, read Ack/status
import time #delay wait
import hid #HID API

TARGET_VID = 0x0483 #Vendor ID, STMicroelectronics's VID
TARGET_PID = 0x5750 #Product ID
TARGET_USAGE_PAGE = 0xFF00 #vendor-defined usage page
TARGET_USAGE = 0x01

KVM_REPORT_ID = 0x10
KVM_PROTOCOL_VER = 0x01

#Custom Messages
KVM_MSG_KEYBOARD = 0x01
KVM_MSG_MOUSE = 0x02
KVM_MSG_RELEASE_ALL = 0x03
KVM_MSG_PING = 0x04 #test device is online
KVM_MSG_STATUS = 0x80 #return status

REPORT_SIZE = 64
PAYLOAD_SIZE = 59

def find_device_info():
    devices = hid.enumerate(TARGET_VID, TARGET_PID)

    for dev in devices:
        if dev.get("usage_page") == TARGET_USAGE_PAGE and dev.get("usage") == TARGET_USAGE:
            return dev
    raise RuntimeError("Can't find vendor HID interface.")

# find HID device and open it
def open_device():
    info = find_device_info()
    # Create an HID device object, dev can dev.write() dev.read() dev.close()...
    dev = hid.device()
    # must need path, so use []
    dev.open_path(info["path"])
    return dev

# b"" is byte
def make_packet(msg_type, seq, payload=b""):
    if len(payload) > PAYLOAD_SIZE:
        raise ValueError("payload too long")

    #[0x00] * (PAYLOAD_SIZE - len(payload) -> HID reports typically have a fixed length.
    #seq & 0xFF -> seq need in 0-255, ex. seq= 300 -> 44
    return bytes([KVM_REPORT_ID, KVM_PROTOCOL_VER, msg_type, seq & 0xFF, len(payload),] + 
                 list(payload) + [0x00] * (PAYLOAD_SIZE - len(payload)))

def read_status(dev, timeout_ms = 1000):
    response = dev.read(REPORT_SIZE, timeout_ms)

    if not response:
        return None

    data = bytes(response)

    if len(data) < 8:
        return None

    if data[0] != KVM_REPORT_ID:
        return None

    if data[1] != KVM_PROTOCOL_VER:
        return None

    if data[2] != KVM_MSG_STATUS:
        return None

    return{
         "sequence": data[3],
         "payload_length": data[4],
         "request_type": data[5],
        "status": data[6],
        "detail": data[7],
        "raw": data,
    }
# packet send to stm32(HID device)
def send_packet(dev, msg_type, seq, payload=b"", wait_status=True):
    packet = make_packet(msg_type, seq, payload)
    written = dev.write(packet) #packet write to stm32

    if written != REPORT_SIZE:
        raise RuntimeError(f"write failed: written={written}")

    if not wait_status:
        return None

    return read_status(dev)

# sned ping to stm32
def ping(dev, seq=1):
    return send_packet(dev, KVM_MSG_PING, seq)

def send_keyboard(dev, modifiers, keys, seq):
    if len(keys) > 6:
        raise ValueError("keys max length is 6")

    padded_keys = list(keys) + [0x00] * (6 - len(keys))

    payload = bytes([modifiers & 0xFF, 0x00, *padded_keys,])
    return send_packet(dev, KVM_MSG_KEYBOARD, seq, payload)

# send release all packet to stm32
def release_all(dev, seq=1):
    return send_packet(dev, KVM_MSG_RELEASE_ALL, seq)

def tap_key(dev, keycode, modifiers=0x00, seq=1, press_ms=50):
    status = send_keyboard(dev, modifiers, [keycode], seq)
    time.sleep(press_ms / 1000)
    release_status = release_all(dev, seq + 1)

    return status, release_status

# Converts an integer to a 2-byte, little-endian, signed format.
# Commonly used for mouse movement measurements, such as X/Y displacement.
def int16le(value):
    return int(value).to_bytes(2, "little", signed=True) #signed=True -> can be -1 , -2...

def move_mouse(dev, dx, dy, wheel=0, button=0x00, seq=1):
    payload = (bytes([button & 0xFF]) + int16le(dx) + int16le(dy) + bytes([wheel & 0xFF, 0x00]))
    return send_packet(dev, KVM_MSG_MOUSE, seq, payload)

def click_left(dev, seq=1):
    down_payload = bytes([0x01]) + int16le(0) + int16le(0) + bytes([0x00, 0x00])
    down_status = send_packet(dev, KVM_MSG_MOUSE, seq, down_payload)
    time.sleep(0.05)
    release_status = release_all(dev, seq + 1)
    return down_status, release_status

if __name__ == "__main__":
    dev = open_device()
    try:
        print("ping", ping(dev, 1))
        print("send a after 3 second, open textedit")
        time.sleep(3)
        print("tap a:", tap_key(dev, 0x04, seq=2))
        print("move mouse:", move_mouse(dev, 50, 0, seq=4))
        print("click left:", click_left(dev, seq=5))
    finally:
        dev.close()
