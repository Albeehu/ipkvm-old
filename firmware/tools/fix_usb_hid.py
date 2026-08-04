from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]


def fix_usbd_conf() -> None:
    path = ROOT / "USB_DEVICE" / "Target" / "usbd_conf.h"
    text = path.read_text()

    values = {
    "USBD_MAX_NUM_INTERFACES": "2U",
    "USBD_CUSTOMHID_OUTREPORT_BUF_SIZE": "64U",
    "USBD_CUSTOM_HID_TARGET_REPORT_DESC_SIZE": "101U",
    "USBD_CUSTOM_HID_VENDOR_REPORT_DESC_SIZE": "29U",
    "USBD_CUSTOM_HID_REPORT_DESC_SIZE": "USBD_CUSTOM_HID_TARGET_REPORT_DESC_SIZE",
    "CUSTOM_HID_EPIN_SIZE": "16U",
    "CUSTOM_HID_VENDOR_EPIN_ADDR": "0x82U",
    "CUSTOM_HID_VENDOR_EPIN_SIZE": "64U",
    "CUSTOM_HID_EPOUT_SIZE": "64U",
    }

    for name, value in values.items():
        line = f"#define {name}     {value}"
        pattern = rf"^#define\s+{name}\s+\S+.*$"

        if re.search(pattern, text, flags=re.MULTILINE):
            text = re.sub(pattern, line, text, flags=re.MULTILINE)
        else:
            text = re.sub(
                r"^#define\s+CUSTOM_HID_FS_BINTERVAL\s+\S+.*$",
                f"{line}\n/*---------- -----------*/\n#define CUSTOM_HID_FS_BINTERVAL     0x5U",
                text,
                count=1,
                flags=re.MULTILINE,
            )

    path.write_text(text)


def fix_custom_hid_descriptor() -> None:
    path = ROOT / "USB_DEVICE" / "App" / "usbd_custom_hid_if.c"
    text = path.read_text()

    # CubeMX leaves its default END_COLLECTION byte after the USER CODE block.
    # The custom descriptor already has the required End Collection items.
    text = re.sub(
        r"(\n\s*/\* USER CODE END 0 \*/\s*)\n\s*0xC0\s*/\*[^\n]*END_COLLECTION[^\n]*\*/",
        r"\1",
        text,
        count=1,
    )

    path.write_text(text)


def main() -> None:
    fix_usbd_conf()
    fix_custom_hid_descriptor()
    print("Fixed USB HID config and descriptor")


if __name__ == "__main__":
    main()
