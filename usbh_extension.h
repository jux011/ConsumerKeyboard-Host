#ifndef USBH_EXTENSION_H
#define USBH_EXTENSION_H

#include <cstdint>
#include "Adafruit_TinyUSB.h"

// Parse report descriptor to find consumer page information
bool tuh_hid_get_consumer_page(tuh_hid_report_info_t info[],
                               uint16_t *const consumer_page_start,
                               uint16_t *const consumer_page_end,
                               uint8_t const desc_report[],
                               uint16_t desc_len);

// Get the report size (in bits) for consumer keys from report descriptor
uint8_t tuh_hid_get_consumer_report_size(uint8_t const desc_report[],
                                         uint16_t const fragment_start,
                                         uint16_t const fragment_end);

// Compute bitmap of consumer key positions in report data
void tuh_hid_compute_key_bitmap_positions(int computed_bitmap[],
                                          uint16_t const target_keys[],
                                          int const bitmap_len,
                                          uint8_t const desc_report[],
                                          uint16_t const fragment_start,
                                          uint16_t const fragment_end);

// Process a consumer report and return the corresponding keycode
uint16_t tuh_hid_process_consumer_report_16bit(uint8_t const report[], uint16_t const report_len);

// Process a 1 bit consumer report
uint16_t tuh_hid_process_consumer_report_1bit(uint8_t const report[], uint16_t const report_len,
                                              int const computed_bitmap[],
                                              uint16_t const target_keys[],
                                              int const bitmap_len);

//--------------------------------------------------------------------+
// ConsumerKeyboard_Host class
// Wrapper class for managing consumer keyboard HID reports
//--------------------------------------------------------------------+

class ConsumerKeyboard_Host
{
public:
    // Disallow default constructor
    ConsumerKeyboard_Host() = delete;

    // Constructor with target keys list
    ConsumerKeyboard_Host(uint16_t const target_keys_list[], int const target_keys_count);

    // Destructor - cleanup allocated arrays
    ~ConsumerKeyboard_Host();

    // Compute and cache consumer key positions from descriptor
    int process_desc_report(uint8_t const desc_report[], uint16_t const desc_len, uint8_t const instance);

    // Cleanup and reset state
    void reset();

    // Get the consumer control interface instance number
    uint8_t get_tuh_consumer_instance()
    {
        return this->tuh_consumer_instance;
    }

    // Process a consumer report and return the corresponding keycode
    uint16_t process_consumer_report(uint8_t const key_report[], uint16_t const report_len);

    bool is_valid = false;

    // list of consumer keys to listen for
    uint16_t *target_consumer_keys = nullptr;

    // count of target consumer keys to listen for
    int consumer_keycodes_count = 0;

    // bitmap of positions of consumer keys in report data of attached keyboard
    // index starting from 0, -1 if key not found in report
    int *key_bitmap_positions = nullptr;

    uint8_t tuh_consumer_instance = 0;

    uint8_t tuh_consumer_report_id = 0;

    uint16_t tuh_consumer_report_size = 0;
};

#endif // USBH_EXTENSION_H
