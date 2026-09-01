#ifndef USBH_EXTENSION_H
#define USBH_EXTENSION_H

#ifndef ADAFRUIT_TINYUSB_H_
#error "This file requires Adafruit_TinyUSB.h to be included first"
#endif

//--------------------------------------------------------------------+
// Static global variables for consumer page parsing and init
//--------------------------------------------------------------------+

// list of consumer keys to listen for
static uint16_t* target_consumer_keys = nullptr;

// bitmap of positions of consumer keys in report data of attached keyboard
// index starting from 0, -1 if key not found in report
static int* key_bitmap_positions = nullptr;

// len(target_consumer_keys)
static int CONSUMER_KEYCODES_COUNT;

// consumer report settings
static uint8_t tuh_consumer_report_size;
static uint8_t tuh_consumer_instance;
static uint8_t tuh_consumer_report_id;

//--------------------------------------------------------------------+
// Public function declarations
//--------------------------------------------------------------------+

// Initialize consumer settings (clears state)
void tuh_init_consumer_settings();

// Initialize consumer settings with target keys list
void tuh_init_consumer_settings(const uint16_t target_keys_list[], const int target_keys_count);

// Parse report descriptor to find consumer page information
bool tuh_hid_get_consumer_page(tuh_hid_report_info_t info[], 
                               uint16_t* const consumer_page_start, 
                               uint16_t* const consumer_page_end, 
                               uint8_t const desc_report[], 
                               uint16_t desc_len);

// Compute and store consumer page values from report descriptor
bool tuh_compute_consumer_page_values(uint8_t const desc_report[], 
                                      uint16_t consumer_page_start, 
                                      uint16_t consumer_page_end,
                                      uint8_t instance, 
                                      uint8_t report_id);

// Process a consumer report and return the corresponding keycode
uint16_t tuh_process_consumer_report(uint8_t const report[], uint16_t report_len);

// Get the consumer control interface instance number
uint8_t get_tuh_consumer_instance();

//--------------------------------------------------------------------+
// Private function declarations
//--------------------------------------------------------------------+

// Get the report size (in bits) for consumer keys from report descriptor
uint8_t get_consumer_report_size(uint8_t const desc_report[], 
                                 uint16_t fragment_start, 
                                 uint16_t fragment_end);

// Compute bitmap of consumer key positions in report data
void compute_consumer_report_bitmap(int computed_bitmap[], 
                                    uint16_t const target_keys[], 
                                    const int bitmap_len, 
                                    uint8_t const desc_report[], 
                                    uint16_t fragment_start, 
                                    uint16_t fragment_end);

// Convert a bitmap report to a single keycode
uint16_t convert_bitmap_report_to_keycode(uint8_t const datafield[], uint16_t datafield_len);

#endif  // USBH_EXTENSION_H
