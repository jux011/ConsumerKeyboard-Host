#include "usbh_extension.h"

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
bool tuh_hid_get_consumer_page(tuh_hid_report_info_t info[], uint16_t *const consumer_page_start, uint16_t *const consumer_page_end, uint8_t const desc_report[], uint16_t desc_len)
{
  // Report Item 6.2.2.2 USB HID 1.11
  union TU_ATTR_PACKED
  {
    uint8_t byte;
    struct TU_ATTR_PACKED
    {
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

  while (desc_len)
  {
    header.byte = *desc_report++;
    desc_len--;
    bytes_read++;

    uint8_t const tag = header.tag;
    uint8_t const type = header.type;
    uint8_t size = header.size;
    if (size == 3)
    {
      size = 4; // HID 1.11 6.2.2.2 size = 3 is actually 4 bytes
    }

    uint8_t const data8 = (size > 0) ? desc_report[0] : 0;

    switch (type)
    {
    case RI_TYPE_MAIN:
      switch (tag)
      {
      case RI_MAIN_INPUT:
        break;
      case RI_MAIN_OUTPUT:
        break;
      case RI_MAIN_FEATURE:
        break;
      case RI_MAIN_COLLECTION:
        ri_collection_depth++;
        break;

      case RI_MAIN_COLLECTION_END:
        ri_collection_depth--;
        // if (ri_collection_depth == 0) {
        // new report id
        // }
        if (info->usage_page == HID_USAGE_PAGE_CONSUMER)
        {
          // done
          *consumer_page_end = bytes_read;
          return 1;
        }
        else
        {
          tu_memclr(info, sizeof(tuh_hid_report_info_t));
          *consumer_page_start = bytes_read;
        }
        break;

      default:
        break;
      }
      break;

    case RI_TYPE_GLOBAL:
      switch (tag)
      {
      case RI_GLOBAL_USAGE_PAGE:
        // only take in account the "usage page" before REPORT ID
        if (ri_collection_depth == 0)
          memcpy(&info->usage_page, desc_report, size);
        break;

      case RI_GLOBAL_LOGICAL_MIN:
        break;
      case RI_GLOBAL_LOGICAL_MAX:
        break;
      case RI_GLOBAL_PHYSICAL_MIN:
        break;
      case RI_GLOBAL_PHYSICAL_MAX:
        break;

      case RI_GLOBAL_REPORT_ID:
        info->report_id = data8;
        break;

      case RI_GLOBAL_REPORT_SIZE:
        //            ri_report_size = data8;
        break;

      case RI_GLOBAL_REPORT_COUNT:
        //            ri_report_count = data8;
        break;

      case RI_GLOBAL_UNIT_EXPONENT:
        break;
      case RI_GLOBAL_UNIT:
        break;
      case RI_GLOBAL_PUSH:
        break;
      case RI_GLOBAL_POP:
        break;

      default:
        break;
      }
      break;

    case RI_TYPE_LOCAL:
      switch (tag)
      {
      case RI_LOCAL_USAGE:
        // only take in account the "usage" before starting REPORT ID
        if (ri_collection_depth == 0)
          info->usage = data8;
        break;

      case RI_LOCAL_USAGE_MIN:
        break;
      case RI_LOCAL_USAGE_MAX:
        break;
      case RI_LOCAL_DESIGNATOR_INDEX:
        break;
      case RI_LOCAL_DESIGNATOR_MIN:
        break;
      case RI_LOCAL_DESIGNATOR_MAX:
        break;
      case RI_LOCAL_STRING_INDEX:
        break;
      case RI_LOCAL_STRING_MIN:
        break;
      case RI_LOCAL_STRING_MAX:
        break;
      case RI_LOCAL_DELIMITER:
        break;
      default:
        break;
      }
      break;

      // error
    default:
      break;
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
// tuh_hid_get_consumer_report_size
// returns number of bits per key in HID report datafield
// expecting values 1 or 16
//--------------------------------------------------------------------+
uint8_t tuh_hid_get_consumer_report_size(uint8_t const desc_report[], uint16_t const fragment_start, uint16_t const fragment_end)
{
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
  // // this will not occur for any modern keyboard. probably.
  // header.size = 1;
  // header.type = RI_TYPE_GLOBAL;
  // header.tag = RI_GLOBAL_REPORT_SIZE;
  // const uint8_t target_byte = header.byte;
  const uint8_t target_byte = 0x75;

  int i = fragment_start;

  while (i < fragment_end)
  {
    if (desc_report[i] == target_byte)
    {
      return desc_report[i + 1];
    }

    switch (desc_report[i] & 0b11)
    {
    case 3:
      i += 5;
      break; // HID 1.11 6.2.2.2 size = 3 is actually 4 bytes
    case 2:
      i += 3;
      break;
    case 1:
      i += 2;
      break;
    case 0:
      i += 1;
      break;
    default: // wtf
      i = fragment_end;
      break;
    }
  }
  // error
  return 0;
}

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
void tuh_hid_compute_key_bitmap_positions(int computed_bitmap[], uint16_t const target_keys[], const int bitmap_len, uint8_t const desc_report[], uint16_t const fragment_start, uint16_t const fragment_end)
{
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
  // const uint8_t target1_byte = header.byte;
  const uint8_t target1_byte = 0x09;
  // header.size = 2;
  // const uint8_t target2_byte = header.byte;
  const uint8_t target2_byte = 0x0A;

  int i = fragment_start;

  // the first usage seen is 0x09 0x01 -> Consumer Page Usage declaration
  // ignore first usage: adjust starting point -1
  // start counting from 0 instead of 1: adjust starting point -1
  int usages_seen = -2;
  while (i < fragment_end)
  {
    if (desc_report[i] == target1_byte)
    { // size = 1, data 00-FF
      usages_seen++;
      for (int j = 0; j < bitmap_len; j++)
      {
        if (target_keys[j] == desc_report[i + 1])
        {
          computed_bitmap[j] = usages_seen;
          break;
        }
      }
    }
    else if (desc_report[i] == target2_byte)
    { // size = 2, data > FF
      usages_seen++;
      // example: 0x0A, 0x96, 0x01 -> data = 0x0196 (little endian)
      uint16_t data = 0;
      memcpy(&data, desc_report + i + 1, 2);
      for (int j = 0; j < bitmap_len; j++)
      {
        if (target_keys[j] == data)
        {
          computed_bitmap[j] = usages_seen;
          break;
        }
      }
    }

    switch (desc_report[i] & 0b11)
    {
    case 3:
      i += 5;
      break; // HID 1.11 6.2.2.2 size = 3 is actually 4 bytes
    case 2:
      i += 3;
      break;
    case 1:
      i += 2;
      break;
    case 0:
      i += 1;
      break;
    default: // wtf
      i = fragment_end;
      break;
    }
  }
}

//--------------------------------------------------------------------+
// tuh_hid_process_consumer_report_16bit
// processes a consumer report in 16-bit format
// vars:
//   key_report: input, the key report in 16-bit format received from attached keyboard
//   report_len: input, length of report
// return:
//   the keycode corresponding to the report
//--------------------------------------------------------------------+
uint16_t tuh_hid_process_consumer_report_16bit(uint8_t const key_report[], uint16_t const report_len)
{
    if (report_len < sizeof(uint16_t))
    {
      Serial.printf("Error: report length %u is too small for 16-bit consumer report\r\n", report_len);
      return 0;
    }
    uint16_t data = 0;
    // example: 0x96, 0x01 -> data = 0x0196 (little endian)
    memcpy(&data, key_report, sizeof(uint16_t));
    return data;
}

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
                                              int const bitmap_len)
{
  // assuming consumer_keycodes_count < 32
  // which is true for this example. adjust type if more keycodes are needed
  static int stored_report = 0;
  int last_report = stored_report;
  int this_report = 0;
  for (int i = 0; i < bitmap_len; i++)
  {
    if (key_bitmap_positions[i] >= report_len * 8)
    {
      // error
      Serial.printf("Error: consumer keycode position %d out of report data bound %u bits\r\n", key_bitmap_positions[i], report_len * 8);
      return 0;
    }
    if (key_bitmap_positions[i] >= 0)
    {
      int byte_pos = key_bitmap_positions[i] / 8;
      int bit_pos = key_bitmap_positions[i] % 8;
      if (key_report[byte_pos] & (1 << bit_pos))
      {
        this_report |= (1 << i);
      }
      else
      {
        this_report &= ~(1 << i);
      }
    }
  }
  stored_report = this_report;
  for (int i = 0; i < bitmap_len; i++)
  {
    if ((this_report & (1 << i)) && !(last_report & (1 << i)))
    {
      // Serial.printf("Keycode 0x%04x pressed\r\n", this->target_consumer_keys[i]);
      return target_keys[i]; // indicate press with keycode
    }
    else if (!(this_report & (1 << i)) && (last_report & (1 << i)))
    {
      // Serial.printf("Keycode 0x%04x released\r\n", this->target_consumer_keys[i]);
      return 0; // indicate release with 0
    }
  }
  // none of the target_consumer_keys changed state
  // but the function must return an answer
  return 0; // indicate release with 0
}


//--------------------------------------------------------------------+
// ConsumerKeyboard_Host class
// Wrapper class for managing consumer keyboard HID reports
//--------------------------------------------------------------------+

// vars defined in usbh_extension.h

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
ConsumerKeyboard_Host::ConsumerKeyboard_Host(uint16_t const target_keys_list[], int const target_keys_count)
{
  this->consumer_keycodes_count = target_keys_count;
  this->target_consumer_keys = new uint16_t[target_keys_count];
  this->key_bitmap_positions = new int[target_keys_count];
  memcpy(this->target_consumer_keys, target_keys_list, target_keys_count * sizeof(uint16_t));
  for (int i = 0; i < target_keys_count; i++)
  {
    this->key_bitmap_positions[i] = -1; // initialize all positions to -1 (not found)
  }
}

// Destructor - cleanup allocated arrays
ConsumerKeyboard_Host::~ConsumerKeyboard_Host()
{
  if (this->target_consumer_keys != nullptr)
  {
    delete[] this->target_consumer_keys;
    // target_consumer_keys = nullptr;
  }
  if (this->key_bitmap_positions != nullptr)
  {
    delete[] this->key_bitmap_positions;
    // key_bitmap_positions = nullptr;
  }
}

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
int ConsumerKeyboard_Host::process_desc_report(uint8_t const desc_report[], uint16_t const desc_len, uint8_t const instance)
{
  tuh_hid_report_info_t info;
  uint16_t consumer_page_start, consumer_page_end;
  bool found = tuh_hid_get_consumer_page(
      &info, &consumer_page_start, &consumer_page_end,
      desc_report, desc_len);
  if (!found)
  {
    return 1;
  }

  this->tuh_consumer_instance = instance;
  this->tuh_consumer_report_id = info.report_id;
  this->tuh_consumer_report_size = tuh_hid_get_consumer_report_size(desc_report, consumer_page_start, consumer_page_end);

  if (this->tuh_consumer_report_size == 0)
  {
    Serial.printf("Error: consumer report size is 0, probably something wrong !!\r\n");
    return 2;
  }
  else if (this->tuh_consumer_report_size == 1)
  {
    Serial.printf("Consumer key 1bit bitmap\r\n");
    tuh_hid_compute_key_bitmap_positions(
        this->key_bitmap_positions,
        this->target_consumer_keys,
        this->consumer_keycodes_count,
        desc_report,
        consumer_page_start,
        consumer_page_end);
    // // print consumer_map
    // Serial.printf("Consumer key bitmap: \r\n");
    // for (int i = 0; i < this->consumer_keycodes_count; i++) {
    //   Serial.printf("  usage = 0x%04x, bitpos = %d\r\n", this->target_consumer_keys[i], this->key_bitmap_positions[i]);
    // }
  }
  else if (this->tuh_consumer_report_size == 16)
  {
    Serial.printf("Consumer key 16bit datafield\r\n");
  }
  else
  {
    // error
    Serial.printf("Error: consumer report size = %u not computed\r\n", this->tuh_consumer_report_size);
    return 3;
  }
  this->is_valid = true;
  return 0;
}

//--------------------------------------------------------------------+
// public facing
// reset key_bitmap_positions
//--------------------------------------------------------------------+
void ConsumerKeyboard_Host::reset()
{
  if (this->key_bitmap_positions != nullptr)
  {
    delete[] this->key_bitmap_positions;
  }
  this->key_bitmap_positions = new int[this->consumer_keycodes_count];
  this->is_valid = false;
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
uint16_t ConsumerKeyboard_Host::process_consumer_report(uint8_t const key_report[], uint16_t const report_len)
{
  if (!this->is_valid)
  {
    Serial.printf("Error: called to process consumer report before descriptor is parsed\r\n");
    return 0;
  }

  if (this->tuh_consumer_report_size == 1)
  {
    if (this->tuh_consumer_report_id > 0)
    {
      // report with report id: first byte is report id, adjust data pointer and length
      if (report_len < 1)
      {
        Serial.printf("Error: received malformed report with report_len %u, report id %u\r\n", report_len, this->tuh_consumer_report_id);
        return 0;
      }
      else if (key_report[0] != this->tuh_consumer_report_id)
      {
        Serial.printf("Error: received report with report_id %u, expected %u\r\n", key_report[0], this->tuh_consumer_report_id);
        // continue anyways
      }
      return tuh_hid_process_consumer_report_1bit(key_report + 1, report_len - 1, this->key_bitmap_positions, this->target_consumer_keys, this->consumer_keycodes_count);
    }
    else
    {
      return tuh_hid_process_consumer_report_1bit(key_report, report_len, this->key_bitmap_positions, this->target_consumer_keys, this->consumer_keycodes_count);
    }
  }
  else if (this->tuh_consumer_report_size == 16)
  {
    if (this->tuh_consumer_report_id > 0)
    {
      // report with report id: first byte is report id, adjust data pointer and length
      if (report_len < 1)
      {
        Serial.printf("Error: received malformed report with report_len %u, report id %u\r\n", report_len, this->tuh_consumer_report_id);
        return 0;
      }
      else if (key_report[0] != this->tuh_consumer_report_id)
      {
        Serial.printf("Error: received report with report_id %u, expected %u\r\n", key_report[0], this->tuh_consumer_report_id);
        // continue anyways
      }
      return tuh_hid_process_consumer_report_16bit(key_report + 1, report_len - 1);
    }
    else
    {
      return tuh_hid_process_consumer_report_16bit(key_report, report_len);
    }
  }
  else
  {
    // error
    Serial.printf("Error: consumer report size = %u not processed\r\n", this->tuh_consumer_report_size);
    return 0;
  }
}
