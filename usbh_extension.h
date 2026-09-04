#ifndef USBH_EXTENSION_H
#define USBH_EXTENSION_H

#include <cstdint>
#include "Adafruit_TinyUSB.h"

//--------------------------------------------------------------------+
// tuh_hid_get_consumer_page
// adapted from tuh_hid_parse_report_descriptor()
// Arduino\libraries\Adafruit_TinyUSB_Library\src\class\hid\hid_host.c
// parses the report descriptor to find the consumer page and gets its report id and size
// vars:
//   info: output, report info struct pointer to fill in
//   consumer_page_start: output, pointer to uint16_t to fill in the starting index of consumer page in desc_report
//   consumer_page_end: output, pointer to uint16_t to fill in the ending index of consumer page in desc_report
//   desc_report: input, the report descriptor to parse
//   desc_len: input, length of desc_report
// return:
//   true if consumer page found and output values are set, false if consumer page not found and output values are cleared
//--------------------------------------------------------------------+
bool tuh_hid_get_consumer_page(tuh_hid_report_info_t info[],
                               uint16_t *const consumer_page_start,
                               uint16_t *const consumer_page_end,
                               uint8_t const desc_report[],
                               uint16_t desc_len);

//--------------------------------------------------------------------+
// tuh_hid_get_consumer_report_size
// returns number of bits per key in HID report datafield
// expecting values 1 or 16
//--------------------------------------------------------------------+
uint8_t tuh_hid_get_consumer_report_size(uint8_t const desc_report[],
                                         uint16_t const fragment_start,
                                         uint16_t const fragment_end);

//--------------------------------------------------------------------+
// tuh_hid_compute_key_bitmap_positions
// populates computed_bitmap[i] with the position of key[i] in field in HID report data
// vars:
//   computed_bitmap: output, bitmap of positions of consumer keys in report data of attached keyboard, index starting from 0, -1 if key not found in report
//   target_keys: input, list of consumer keycodes to listen for
//   bitmap_len: input, length of computed_bitmap and target_keys
//   desc_report: input, the report descriptor to parse
//   fragment_start: input, the starting index of consumer page in desc_report
//   fragment_end: input, the ending index of consumer page in desc_report
//--------------------------------------------------------------------+
void tuh_hid_compute_key_bitmap_positions(int computed_bitmap[],
                                          uint16_t const target_keys[],
                                          int const bitmap_len,
                                          uint8_t const desc_report[],
                                          uint16_t const fragment_start,
                                          uint16_t const fragment_end);

//--------------------------------------------------------------------+
// tuh_hid_process_consumer_report_16bit
// processes a consumer report in 16-bit format
// vars:
//   key_report: input, the key report in 16-bit format received from attached keyboard
//   report_len: input, length of report
// return:
//   the keycode corresponding to the report
//--------------------------------------------------------------------+
uint16_t tuh_hid_process_consumer_report_16bit(uint8_t const key_report[], uint16_t const report_len);

//--------------------------------------------------------------------+
// tuh_hid_process_consumer_report_1bit
// processes a consumer report in 1-bit format
// vars:
//   key_report: input, the key report in 1-bit format received from attached keyboard
//   report_len: input, length of report
//   key_bitmap_positions: input, bitmap of positions of consumer keys in report data
//   target_keys: input, list of consumer keycodes to listen for
//   bitmap_len: input, length of key_bitmap_positions and target_keys
// return:
//   the keycode corresponding to the report
//--------------------------------------------------------------------+
uint16_t tuh_hid_process_consumer_report_1bit(uint8_t const key_report[], uint16_t const report_len,
                                              int const key_bitmap_positions[],
                                              uint16_t const target_keys[],
                                              int const bitmap_len);


//--------------------------------------------------------------------+
// ConsumerKeyboard_Host class
// Wrapper class for managing consumer keyboard HID reports
//--------------------------------------------------------------------+

class ConsumerKeyboard_Host
{
public:
    bool is_valid = false;

    // list of consumer keys to listen for
    uint16_t *target_consumer_keys = nullptr;

    // count of target consumer keys to listen for
    int consumer_keycodes_count = 0;

    // bitmap of positions of consumer keys in report data of attached keyboard
    // index starting from 0, -1 if key not found in report
    int *key_bitmap_positions = nullptr;

    // instance number of the consumer control interface
    uint8_t tuh_consumer_instance = 0;

    // report ID of the consumer control interface
    uint8_t tuh_consumer_report_id = 0;

    // size of the consumer report format in bits
    uint16_t tuh_consumer_report_size = 0;

    // Disallow default constructor
    ConsumerKeyboard_Host() = delete;

    //--------------------------------------------------------------------+
    // public facing
    // ConsumerKeyboard_Host constructor
    // sets all starting values and sets target_consumer_keys list
    // vars:
    //   target_keys_list: input, list of consumer keycodes to listen for
    //   target_keys_count: input, length of target_keys_list
    // class variables updated:
    //   target_consumer_keys, key_bitmap_positions, consumer_keycodes_count
    //--------------------------------------------------------------------+
    ConsumerKeyboard_Host(uint16_t const target_keys_list[], int const target_keys_count);

    // Destructor - cleanup allocated arrays
    ~ConsumerKeyboard_Host();

    //--------------------------------------------------------------------+
    // public facing
    // process_desc_report
    // parses desc_report and initializes all settings related to consumer page
    //   vars:
    //   desc_report: input, the report descriptor to parse
    //   desc_len: input, length of desc_report
    //   instance: input, the instance number of the consumer control interface
    // return:
    //   0 if consumer page found and output values are set
    //   1 if consumer page not found in this instance
    //   >1 if there is an error in parsing the report descriptor
    // class variables updated:
    //   key_bitmap_positions, is_valid,
    //   tuh_consumer_report_size, tuh_consumer_instance, tuh_consumer_report_id
    //--------------------------------------------------------------------+
    int process_desc_report(uint8_t const desc_report[], uint16_t const desc_len, uint8_t const instance);

    //--------------------------------------------------------------------+
    // public facing
    // reset key_bitmap_positions
    //--------------------------------------------------------------------+
    void reset();

    //--------------------------------------------------------------------+
    // public facing
    // Get the consumer control interface instance number
    //--------------------------------------------------------------------+
    uint8_t get_tuh_consumer_instance()
    {
        if (!this->is_valid)
        {
            Serial.printf("Error: called to give consumer instance number before descriptor is parsed\r\n");
        }
        return this->tuh_consumer_instance;
    }

    //--------------------------------------------------------------------+
    // public facing
    // process_consumer_report
    // process a consumer report and return keycode for a changed keypress
    // vars:
    //   key_report: input, the consumer report received from attached keyboard
    //   report_len: input, length of report
    // return:
    //   the keycode corresponding to the report
    //--------------------------------------------------------------------+
    uint16_t process_consumer_report(uint8_t const key_report[], uint16_t const report_len);
};

#endif // USBH_EXTENSION_H
