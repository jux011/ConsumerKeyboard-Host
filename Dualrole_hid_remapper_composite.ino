/*********************************************************************
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 Copyright (c) 2019 Ha Thach for Adafruit Industries
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/* This example demonstrates use of both device and host, where
 * - Device run on native usb controller (roothub port0)
 * - Host depending on MCUs run on either:
 *   - rp2040: bit-banging 2 GPIOs with the help of Pico-PIO-USB library (roothub port1)
 *
 * Requirements:
 * - For rp2040:
 *   - [Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB) library
 *   - 2 consecutive GPIOs: D+ is defined by PIN_USB_HOST_DP, D- = D+ +1
 *   - Provide VBus (5v) and GND for peripheral
 *   - CPU Speed must be either 120 or 240 Mhz. Selected via "Menu -> CPU Speed"
 */

/* Example sketch receive keyboard report from host interface (from e.g consumer keyboard)
 * and remap it to another key and send it via device interface (to PC). For simplicity,
 * this example only toggle shift key to the report, effectively remap:
 * - all character key <-> upper case
 * - number <-> its symbol (with shift)
 */

#define PRINT_SERIAL_DELAY 2000  // milliseconds

#include "pin_setup_helper.h"

// USBHost is defined in usbh_helper.h
#include "usbh_helper.h"

#include "usbh_extension.h"

// Report ID
enum {
  RID_KEYBOARD = 1,
  RID_MOUSE,
  RID_CONSUMER_CONTROL,  // Media, volume etc ..
};

// HID report descriptor using TinyUSB's template
uint8_t const desc_hid_report[] = {
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD)),
  TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(RID_MOUSE)),
  TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(RID_CONSUMER_CONTROL))
};

// list of consumer keys to listen for
struct bitpos_map consumer_map[6] = {
  { HID_USAGE_CONSUMER_SCAN_PREVIOUS_TRACK, -1 },
  { HID_USAGE_CONSUMER_PLAY_PAUSE, -1 },
  { HID_USAGE_CONSUMER_SCAN_NEXT_TRACK, -1 },
  { HID_USAGE_CONSUMER_MUTE, -1 },
  { HID_USAGE_CONSUMER_VOLUME_DECREMENT, -1 },
  { HID_USAGE_CONSUMER_VOLUME_INCREMENT, -1 },
};
static const int CONSUMER_KEYCODES_COUNT = sizeof(consumer_map) / sizeof(bitpos_map);

uint8_t consumer_report_size;
uint8_t tuh_consumer_instance;

// USB HID object
Adafruit_USBD_HID usb_hid;

//--------------------------------------------------------------------+
// For RP2040 use both core0 for device stack, core1 for host stack
//--------------------------------------------------------------------+

//------------- Core0 -------------//
void setup() {
  Serial.begin(115200);
  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.setStringDescriptor("TinyUSB HID Composite\n");
  usb_hid.begin();

  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }

#if defined(PRINT_SERIAL_DELAY) && PRINT_SERIAL_DELAY
  // wait for native usb
  for (int i = 0; i < PRINT_SERIAL_DELAY; i += 10) {
    if (Serial) { break; }
    delay(10);
  }
#endif

  Serial.println("TinyUSB Host Composite HID Remap Example\n");
}

void loop() {
  // nothing to do
}

//------------- Core1 -------------//
void setup1() {
#if defined(PRINT_SERIAL_DELAY) && PRINT_SERIAL_DELAY
  // wait for native usb
  for (int i = 0; i < PRINT_SERIAL_DELAY; i += 10) {
    if (Serial) { break; }
    delay(10);
  }
#endif
  // configure pio-usb: defined in usbh_helper.h
  rp2040_configure_pio_usb();

  init_tuh_consumer_settings();

  // run host stack on controller (rhport) 1
  // Note: For rp2040 pico-pio-usb, calling USBHost.begin() on core1 will have most of the
  // host bit-banging processing works done in core1 to free up core0 for other works
  USBHost.begin(1);
}

void loop1() {
  USBHost.task();
}

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+
extern "C" {

  // Invoked when device with hid interface is mounted
  // Report descriptor is also available for use.
  // tuh_hid_parse_report_descriptor() can be used to parse common/simple enough
  // descriptor. Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE,
  // it will be skipped therefore report_desc = NULL, desc_len = 0
  void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    Serial.printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);
    Serial.printf("VID = %04x, PID = %04x\r\n", vid, pid);

    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
      Serial.printf("HID Keyboard\r\n");
      if (!tuh_hid_receive_report(dev_addr, instance)) {
        Serial.printf("Error: cannot request to receive report\r\n");
      }
      return;
    }

    tuh_hid_report_info_t info;
    uint16_t consumer_page_start, consumer_page_end;
    bool found = tuh_hid_get_consumer_page(&info, &consumer_page_start, &consumer_page_end, desc_report, desc_len);
    if (!found) {
      return;
    }
    Serial.printf("HID Consumer Control\r\n");
    tuh_consumer_instance = instance;
    consumer_report_size = get_consumer_report_size(desc_report, consumer_page_start, consumer_page_end);
    if (consumer_report_size == 0) {
      Serial.printf("Error: consumer report size is 0, probably something wrong !!\r\n");
    } else if (consumer_report_size == 1) {
      get_consumer_report_bitmap(consumer_map, CONSUMER_KEYCODES_COUNT, desc_report, consumer_page_start, consumer_page_end);
      // print consumer_map
      Serial.printf("Consumer key bitmap: \r\n");
      for (int i = 0; i < CONSUMER_KEYCODES_COUNT; i++) {
        Serial.printf("  usage = 0x%04x, bitpos = %d\r\n", consumer_map[i].keycode, consumer_map[i].position);
      }
    } else if (consumer_report_size == 16) {
      Serial.printf("Consumer key 16bit datafield\r\n");
    } else {
      // error
      Serial.printf("Error: consumer report size = %u not supported in this example !!\r\n", consumer_report_size);
      return;
    }

    if (!tuh_hid_receive_report(dev_addr, instance)) {
      Serial.printf("Error: cannot request to receive report\r\n");
    }
    return;
  }

  // Invoked when device with hid interface is un-mounted
  void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    Serial.printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
    if (instance == tuh_consumer_instance) {
      init_tuh_consumer_settings();
    }
  }

  void remap_key(hid_keyboard_report_t const *original_report, hid_keyboard_report_t *remapped_report) {
    memcpy(remapped_report, original_report, sizeof(hid_keyboard_report_t));
    // do nothing lmao
  }

  // Invoked when received report from device via interrupt endpoint
  void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    if (instance == 0) {  // boot keyboard
      // Serial.printf("Received report from instance %d, len = %u\r\n", instance, len);
      if (len != 8) {
        Serial.printf("report len = %u NOT 8, probably something wrong !!\r\n", len);
      } else {
        hid_keyboard_report_t remapped_report;
        remap_key((hid_keyboard_report_t const *)report, &remapped_report);

        // send remapped report to PC
        // NOTE: for better performance you should save/queue remapped report instead of
        // blocking wait for usb_hid ready here
        while (!usb_hid.ready()) {
          yield();
        }

        usb_hid.sendReport(RID_KEYBOARD, &remapped_report, sizeof(hid_keyboard_report_t));
      }

      // continue to request to receive report
      if (!tuh_hid_receive_report(dev_addr, instance)) {
        Serial.printf("Error: cannot request to receive report\r\n");
      }
    } else if (instance == tuh_consumer_instance) {
      // Serial.printf("Received report from consumer control instance %d, len = %u\r\n", instance, len);
      // don't remap key
      if (consumer_report_size == 16) {
        // 16 bit datafield, just forward the report
        while (!usb_hid.ready()) {
          yield();
        }
        uint8_t report_to_send[2] = { report[1], report[2] };
        usb_hid.sendReport(RID_CONSUMER_CONTROL, report_to_send, sizeof(report_to_send));
        // continue to request to receive report
        if (!tuh_hid_receive_report(dev_addr, instance)) {
          Serial.printf("Error: cannot request to receive report\r\n");
        }
      } else if (consumer_report_size == 1) {
        // // bitmap, translate it to 16bit keycode
        // uint16_t remapped_report = 0;




        // while (!usb_hid.ready()) {
        //   yield();
        // }
        // usb_hid.sendReport(RID_CONSUMER_CONTROL, &remapped_report, sizeof(remapped_report));
        // // continue to request to receive report
        // if (!tuh_hid_receive_report(dev_addr, instance)) {
        //   Serial.printf("Error: cannot request to receive report\r\n");
        // }
        return;
      }
    } else {
      Serial.printf("Received report from unknown instance %d, len = %u\r\n", instance, len);
    }
  }
}
