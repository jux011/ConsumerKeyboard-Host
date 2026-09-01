#ifndef USBH_EXTENSION_H
#define USBH_EXTENSION_H

#ifndef ADAFRUIT_TINYUSB_H_
#error "This file requires Adafruit_TinyUSB.h to be included first"
#endif

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

//--------------------------------------------------------------------+
// ConsumerKeyboard_Host class
// Wrapper class for managing consumer keyboard HID reports
//--------------------------------------------------------------------+

class ConsumerKeyboard_Host {
public:
    // Disallow default constructor
    ConsumerKeyboard_Host() = delete;
    
    // Constructor with target keys list
    ConsumerKeyboard_Host(const uint16_t target_keys_list[], const int target_keys_count);
    
    // Destructor - cleanup allocated arrays
    ~ConsumerKeyboard_Host();
    
    // Initialize with report descriptor
    int begin(uint8_t const desc_report[], uint16_t desc_len);
    
    // Cleanup and reset state
    void end();
    
    // Get the consumer control interface instance number
    uint8_t get_tuh_consumer_instance() {
        return tuh_consumer_instance;
    }
    
    // Compute and cache consumer key positions from descriptor
    bool compute_consumer_keys_map();
    
    // Process a consumer report and return the corresponding keycode
    uint16_t process_consumer_report(uint8_t const key_report[], uint16_t report_len);

private:
    // list of consumer keys to listen for
    uint16_t* target_consumer_keys = nullptr;

    // bitmap of positions of consumer keys in report data of attached keyboard
    // index starting from 0, -1 if key not found in report
    int* key_bitmap_positions = nullptr;

    uint8_t tuh_consumer_instance;
};

#endif  // USBH_EXTENSION_H
