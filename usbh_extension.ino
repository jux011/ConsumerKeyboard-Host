
//--------------------------------------------------------------------+
// tuh_hid_get_consumer_page
// adapted from tuh_hid_parse_report_descriptor()
// Arduino\libraries\Adafruit_TinyUSB_Library\src\class\hid\hid_host.c
//--------------------------------------------------------------------+

bool tuh_hid_get_consumer_page(tuh_hid_report_info_t* const info, uint16_t* const consumer_page_start, uint16_t* const consumer_page_end, uint8_t const* desc_report, uint16_t desc_len) {
  // Report Item 6.2.2.2 USB HID 1.11
  union TU_ATTR_PACKED {
    uint8_t byte;
    struct TU_ATTR_PACKED {
      uint8_t size : 2;
      uint8_t type : 2;
      uint8_t tag : 4;
    };
  } header;

  tu_memclr(info, sizeof(tuh_hid_report_info_t));

  // current parsed report count & size from descriptor
  // uint8_t ri_report_count = 0;
  // uint8_t ri_report_size = 0;

  uint8_t ri_collection_depth = 0;
  uint16_t bytes_read = 0;
  *consumer_page_start = 0;
  *consumer_page_end = 0;

  while (desc_len) {
    header.byte = *desc_report++;
    desc_len--;
    bytes_read++;

    uint8_t const tag = header.tag;
    uint8_t const type = header.type;
    uint8_t size = header.size;
    if (size == 3) {
      size = 4;  // HID 1.11 6.2.2.2 3 is 4 bytes
    }

    uint8_t const data8 = (size > 0) ? desc_report[0] : 0;

    switch (type) {
      case RI_TYPE_MAIN:
        switch (tag) {
          case RI_MAIN_INPUT: break;
          case RI_MAIN_OUTPUT: break;
          case RI_MAIN_FEATURE: break;
          case RI_MAIN_COLLECTION:
            ri_collection_depth++;
            break;

          case RI_MAIN_COLLECTION_END:
            ri_collection_depth--;
            // if (ri_collection_depth == 0) {
            // new report id
            // }
            if (info->usage_page == HID_USAGE_PAGE_CONSUMER) {
              // done
              *consumer_page_end = bytes_read;
              return 1;
            } else {
              tu_memclr(info, sizeof(tuh_hid_report_info_t));
              *consumer_page_start = bytes_read;
            }
            break;

          default: break;
        }
        break;

      case RI_TYPE_GLOBAL:
        switch (tag) {
          case RI_GLOBAL_USAGE_PAGE:
            // only take in account the "usage page" before REPORT ID
            if (ri_collection_depth == 0) memcpy(&info->usage_page, desc_report, size);
            break;

          case RI_GLOBAL_LOGICAL_MIN: break;
          case RI_GLOBAL_LOGICAL_MAX: break;
          case RI_GLOBAL_PHYSICAL_MIN: break;
          case RI_GLOBAL_PHYSICAL_MAX: break;

          case RI_GLOBAL_REPORT_ID:
            info->report_id = data8;
            break;

          case RI_GLOBAL_REPORT_SIZE:
            //            ri_report_size = data8;
            break;

          case RI_GLOBAL_REPORT_COUNT:
            //            ri_report_count = data8;
            break;

          case RI_GLOBAL_UNIT_EXPONENT: break;
          case RI_GLOBAL_UNIT: break;
          case RI_GLOBAL_PUSH: break;
          case RI_GLOBAL_POP: break;

          default: break;
        }
        break;

      case RI_TYPE_LOCAL:
        switch (tag) {
          case RI_LOCAL_USAGE:
            // only take in account the "usage" before starting REPORT ID
            if (ri_collection_depth == 0) info->usage = data8;
            break;

          case RI_LOCAL_USAGE_MIN: break;
          case RI_LOCAL_USAGE_MAX: break;
          case RI_LOCAL_DESIGNATOR_INDEX: break;
          case RI_LOCAL_DESIGNATOR_MIN: break;
          case RI_LOCAL_DESIGNATOR_MAX: break;
          case RI_LOCAL_STRING_INDEX: break;
          case RI_LOCAL_STRING_MIN: break;
          case RI_LOCAL_STRING_MAX: break;
          case RI_LOCAL_DELIMITER: break;
          default: break;
        }
        break;

        // error
      default: break;
    }

    desc_report += size;
    desc_len -= size;
    bytes_read += size;
  }

  // consumer page not found
  // tu_memclr(info, sizeof(tuh_hid_report_info_t)); // redundant
  tu_varclr(consumer_page_start);
  tu_varclr(consumer_page_end);
  return 0;
}

//--------------------------------------------------------------------+
// get_consumer_report_size
// Number of bits per field in HID report data
//--------------------------------------------------------------------+

uint8_t get_consumer_report_size(uint8_t const* desc_report, uint16_t fragment_start, uint16_t fragment_end) {
  // // Report Item 6.2.2.2 USB HID 1.11
  // union TU_ATTR_PACKED {
  //   uint8_t byte;
  //   struct TU_ATTR_PACKED {
  //     uint8_t size : 2;
  //     uint8_t type : 2;
  //     uint8_t tag : 4;
  //   };
  // } header;

  // // breaks if consumer_report_size > 255
  // // this will not occur for HID. probably.
  // header.size = 1;
  // header.type = RI_TYPE_GLOBAL;
  // header.tag = RI_GLOBAL_REPORT_SIZE;
  // const uint8_t target_byte = header.byte;
  const uint8_t target_byte = 0x75;

  int i = fragment_start;

  while (i < fragment_end) {
    if (desc_report[i] == target_byte) {
      return desc_report[i + 1];
    }

    switch (desc_report[i] & 0b11) {
      case 3: i += 5; break;  // HID 1.11 6.2.2.2 3 is 4 bytes
      case 2: i += 3; break;
      case 1: i += 2; break;
      case 0: i += 1; break;
      default:  //wtf
        i = fragment_end;
        break;
    }
  }
  // error
  return 0;
}

//--------------------------------------------------------------------+
// init_consumer_bitmap
// set all starting values
//--------------------------------------------------------------------+

void init_tuh_consumer_settings() {
  for (int i = 0; i < CONSUMER_KEYCODES_COUNT; i++) {
    consumer_map[i].position = -1;
  }
  consumer_report_size = 0;
  tuh_consumer_instance = 0;
}

//--------------------------------------------------------------------+
// get_consumer_report_bitmap
// populate consumer_bitmap[i] with the position of key[i] in field in in HID report data
//--------------------------------------------------------------------+

void get_consumer_report_bitmap(bitpos_map* consumer_bitmap, const int bitmap_len, uint8_t const* desc_report, uint16_t fragment_start, uint16_t fragment_end) {
  // // Report Item 6.2.2.2 USB HID 1.11
  // union TU_ATTR_PACKED {
  //   uint8_t byte;
  //   struct TU_ATTR_PACKED {
  //     uint8_t size : 2;
  //     uint8_t type : 2;
  //     uint8_t tag : 4;
  //   };
  // } header;

  // header.size = 1;
  // header.type = RI_TYPE_LOCAL;
  // header.tag = RI_LOCAL_USAGE;
  const uint8_t target1_byte = 0x09;
  // const uint8_t target1_byte = header.byte;
  // header.size = 2;
  const uint8_t target2_byte = 0x0A;
  // const uint8_t target2_byte = header.byte;

  int i = fragment_start;

  // the first usage seen is 0x09 0x01 -> Consumer Page Usage declaration
  // ignore first usage: adjust starting point -1
  // start counting from 0 instead of 1: adjust starting point -1
  int usages_seen = -2;
  while (i < fragment_end) {
    if (desc_report[i] == target1_byte) {  // size = 1, data 00-FF
      usages_seen++;
      for (int j = 0; j < bitmap_len; j++) {
        if (consumer_bitmap[j].keycode == desc_report[i + 1]) {
          consumer_bitmap[j].position = usages_seen;
          break;
        }
      }
    } else if (desc_report[i] == target2_byte) {  // size = 2, data > FF
      usages_seen++;
      // 2 byte example: 0x0A, 0x96, 0x01 -> data = 0x0196 (little endian)
      uint16_t data = 0;
      memcpy(&data, desc_report + i + 1, 2);
      for (int j = 0; j < bitmap_len; j++) {
        if (consumer_bitmap[j].keycode == data) {
          consumer_bitmap[j].position = usages_seen;
          break;
        }
      }
    }

    switch (desc_report[i] & 0b11) {
      case 3: i += 5; break;  // HID 1.11 6.2.2.2 3 is 4 bytes
      case 2: i += 3; break;
      case 1: i += 2; break;
      case 0: i += 1; break;
      default:  //wtf
        i = fragment_end;
        break;
    }
  }
}
