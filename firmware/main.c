#include <string.h>
#include <fx2lib.h>
#include <fx2regs.h>
#include <fx2ints.h>
#include <fx2usb.h>
#include <fx2delay.h>
#include <fx2i2c.h>
#include <fx2eeprom.h>
#include <usbmicrosoft.h>
#include "glasgow.h"
#include "version.h"

// bcdDevice is a 16-bit number where the high byte indicates the API revision and the low byte
// indicates the hardware revision. If the firmware is not flashed (only the FX2 header is present)
// then the high byte is zero (as configured by `glasgow factory`). The low byte can be zero on
// legacy devices with old or no firmware where the hardware revision is present only in
// the Glasgow configuration block. Loading new firmware ensures it is present in the FX2 header.

usb_desc_device_c usb_device = {
  .bLength              = sizeof(struct usb_desc_device),
  .bDescriptorType      = USB_DESC_DEVICE,
  .bcdUSB               = 0x0200,
  .bDeviceClass         = USB_DEV_CLASS_PER_INTERFACE,
  .bDeviceSubClass      = USB_DEV_SUBCLASS_PER_INTERFACE,
  .bDeviceProtocol      = USB_DEV_PROTOCOL_PER_INTERFACE,
  .bMaxPacketSize0      = 64,
  .idVendor             = VID_QIHW,
  .idProduct            = PID_GLASGOW,
  .bcdDevice            = 0, // filled in descriptors_init()
  .iManufacturer        = 1,
  .iProduct             = 2,
  .iSerialNumber        = 3,
  .bNumConfigurations   = 2,
};

usb_desc_device_qualifier_c usb_device_qualifier = {
  .bLength              = sizeof(struct usb_desc_device_qualifier),
  .bDescriptorType      = USB_DESC_DEVICE_QUALIFIER,
  .bcdUSB               = 0x0200,
  .bDeviceClass         = USB_DEV_CLASS_PER_INTERFACE,
  .bDeviceSubClass      = USB_DEV_SUBCLASS_PER_INTERFACE,
  .bDeviceProtocol      = USB_DEV_PROTOCOL_PER_INTERFACE,
  .bMaxPacketSize0      = 8,
  .bNumConfigurations   = 0,
};

#define USB_INTERFACE(bInterfaceNumber_, bAlternateSetting_, bNumEndpoints_)              \
  {                                                                                       \
    .bLength              = sizeof(struct usb_desc_interface),                            \
    .bDescriptorType      = USB_DESC_INTERFACE,                                           \
    .bInterfaceNumber     = bInterfaceNumber_,                                            \
    .bAlternateSetting    = bAlternateSetting_,                                           \
    .bNumEndpoints        = bNumEndpoints_,                                               \
    .bInterfaceClass      = USB_IFACE_CLASS_VENDOR,                                       \
    .bInterfaceSubClass   = USB_IFACE_SUBCLASS_VENDOR,                                    \
    .bInterfaceProtocol   = USB_IFACE_PROTOCOL_VENDOR,                                    \
    .iInterface           = 0,                                                            \
  }

usb_desc_interface_c usb_interface_0_disabled =
  USB_INTERFACE(/*bInterfaceNumber=*/0, /*bAlternateSetting=*/0, /*bNumEndpoints=*/0);
usb_desc_interface_c usb_interface_0_enabled =
  USB_INTERFACE(/*bInterfaceNumber=*/0, /*bAlternateSetting=*/1, /*bNumEndpoints=*/1);
usb_desc_interface_c usb_interface_1_disabled =
  USB_INTERFACE(/*bInterfaceNumber=*/1, /*bAlternateSetting=*/0, /*bNumEndpoints=*/0);
usb_desc_interface_c usb_interface_1_enabled =
  USB_INTERFACE(/*bInterfaceNumber=*/1, /*bAlternateSetting=*/1, /*bNumEndpoints=*/1);
usb_desc_interface_c usb_interface_2_disabled =
  USB_INTERFACE(/*bInterfaceNumber=*/2, /*bAlternateSetting=*/0, /*bNumEndpoints=*/0);
usb_desc_interface_c usb_interface_2_enabled =
  USB_INTERFACE(/*bInterfaceNumber=*/2, /*bAlternateSetting=*/1, /*bNumEndpoints=*/1);
usb_desc_interface_c usb_interface_3_disabled =
  USB_INTERFACE(/*bInterfaceNumber=*/3, /*bAlternateSetting=*/0, /*bNumEndpoints=*/0);
usb_desc_interface_c usb_interface_3_enabled =
  USB_INTERFACE(/*bInterfaceNumber=*/3, /*bAlternateSetting=*/1, /*bNumEndpoints=*/1);

#define USB_BULK_ENDPOINT(bEndpointAddress_)                                              \
  {                                                                                       \
    .bLength              = sizeof(struct usb_desc_endpoint),                             \
    .bDescriptorType      = USB_DESC_ENDPOINT,                                            \
    .bEndpointAddress     = bEndpointAddress_,                                            \
    .bmAttributes         = USB_XFER_BULK,                                                \
    .wMaxPacketSize       = 512,                                                          \
    .bInterval            = 0,                                                            \
  }

usb_desc_endpoint_c usb_endpoint_2_out =
  USB_BULK_ENDPOINT(/*bEndpointAddress=*/2|USB_DIR_OUT);
usb_desc_endpoint_c usb_endpoint_4_out =
  USB_BULK_ENDPOINT(/*bEndpointAddress=*/4|USB_DIR_OUT);
usb_desc_endpoint_c usb_endpoint_6_in =
  USB_BULK_ENDPOINT(/*bEndpointAddress=*/6|USB_DIR_IN );
usb_desc_endpoint_c usb_endpoint_8_in =
  USB_BULK_ENDPOINT(/*bEndpointAddress=*/8|USB_DIR_IN );

usb_configuration_c usb_config_4_pipes = {
  {
    .bLength              = sizeof(struct usb_desc_configuration),
    .bDescriptorType      = USB_DESC_CONFIGURATION,
    .bNumInterfaces       = 4,
    .bConfigurationValue  = 1,
    .iConfiguration       = 0,
    .bmAttributes         = USB_ATTR_RESERVED_1,
    .bMaxPower            = 250,
  },
  {
    { .interface  = &usb_interface_0_disabled },
    { .interface  = &usb_interface_0_enabled  },
    { .endpoint   = &usb_endpoint_2_out       },
    { .interface  = &usb_interface_1_disabled },
    { .interface  = &usb_interface_1_enabled  },
    { .endpoint   = &usb_endpoint_4_out       },
    { .interface  = &usb_interface_2_disabled },
    { .interface  = &usb_interface_2_enabled  },
    { .endpoint   = &usb_endpoint_6_in        },
    { .interface  = &usb_interface_3_disabled },
    { .interface  = &usb_interface_3_enabled  },
    { .endpoint   = &usb_endpoint_8_in        },
    { 0 }
  }
};

usb_configuration_c usb_config_2_pipes = {
  {
    .bLength              = sizeof(struct usb_desc_configuration),
    .bDescriptorType      = USB_DESC_CONFIGURATION,
    .bNumInterfaces       = 2,
    .bConfigurationValue  = 2,
    .iConfiguration       = 0,
    .bmAttributes         = USB_ATTR_RESERVED_1,
    .bMaxPower            = 250,
  },
  {
    { .interface  = &usb_interface_0_disabled },
    { .interface  = &usb_interface_0_enabled  },
    { .endpoint   = &usb_endpoint_2_out       },
    { .interface  = &usb_interface_1_disabled },
    { .interface  = &usb_interface_1_enabled  },
    { .endpoint   = &usb_endpoint_6_in        },
    { 0 }
  }
};

// check for "earlier than 3.5", but version macros shipped in 3.6
#if !defined(__SDCC_VERSION_MAJOR)
__code const struct usb_configuration *__code const usb_configs[] = {
#else
usb_configuration_set_c usb_configs[] = {
#endif
  &usb_config_4_pipes,
  &usb_config_2_pipes,
};

// This replaces the beginning of "Glasgow Interface Explorer" in the string table below if
// the "modified from original design" flag is set in the configuration.
#define MODIFIED_DESIGN_PRODUCT_STRING "Another"

usb_ascii_string_c usb_strings[] = {
  [0] = "whitequark research\0\0\0", // CONFIG_SIZE_MANUFACTURER characters long
  [1] = "Glasgow Interface Explorer (git " GIT_REVISION ")",
  [2] = "XX-XXXXXXXXXXXXXXXX",
};

usb_descriptor_set_c usb_descriptor_set = {
  .device           = &usb_device,
  .device_qualifier = &usb_device_qualifier,
  .config_count     = ARRAYSIZE(usb_configs),
  .configs          = usb_configs,
  .string_count     = ARRAYSIZE(usb_strings),
  .strings          = usb_strings,
};

usb_desc_microsoft_v10_c usb_microsoft = {
  .bLength          = sizeof(struct usb_desc_microsoft_v10),
  .bDescriptorType  = USB_DESC_STRING,
  .qwSignature      = USB_DESC_MICROSOFT_V10_SIGNATURE,
  .bMS_VendorCode   = 0xC0,
};

usb_desc_ms_ext_compat_id_c usb_ms_ext_compat_id = {
  .dwLength         = sizeof(struct usb_desc_ms_ext_compat_id) +
                      sizeof(struct usb_desc_ms_compat_function),
  .bcdVersion       = 0x0100,
  .wIndex           = USB_DESC_MS_EXTENDED_COMPAT_ID,
  .bCount           = 1,
  .functions        = {
    {
      .bFirstInterfaceNumber  = 0,
      .bReserved1             = 1,
      .compatibleID           = "WINUSB",
    },
  }
};

usb_desc_ms_ext_property_c usb_ms_ext_properties = {
  .dwLength         = sizeof(struct usb_desc_ms_ext_property),
  .bcdVersion       = 0x0100,
  .wIndex           = USB_DESC_MS_EXTENDED_PROPERTIES,
  .wCount           = 0,
};

void handle_usb_get_descriptor(enum usb_descriptor type, uint8_t index) {
  if(type == USB_DESC_STRING && index == 0xEE) {
    xmemcpy(scratch, (__xdata void *)&usb_microsoft, usb_microsoft.bLength);
    SETUP_EP0_IN_DESC(scratch);
  } else {
    usb_serve_descriptor(&usb_descriptor_set, type, index);
  }
}

static void config_init() {
  unsigned char load_cmd;
  if(!eeprom_read(I2C_ADDR_FX2_MEM, 0, &load_cmd, sizeof(load_cmd), /*double_byte=*/true))
    goto fail;
  if(load_cmd == 0xff) {
    goto fail;
  } else if(load_cmd == 0xc2) {
    // A C2 load, used on devices with firmware, automatically loads configuration.
    return;
  } else if(load_cmd == 0xc0) {
    // A C0 load, used on factory-programmed devices without firmware, does not, so
    // load it explicitly.
    if(!eeprom_read(I2C_ADDR_FX2_MEM, 8 + 4, (__xdata void *)&glasgow_config,
                    sizeof(glasgow_config), /*double_byte=*/true))
      goto fail;
    return;
  }

fail:
  // Configuration block is corrupted or missing, load default configuration so that
  // we don't hang or present nonsensical descriptors.
  glasgow_config.revision = GLASGOW_REV_NA;
  xmemcpy((__xdata void *)glasgow_config.serial, (__xdata void *)"9999999999999999",
          sizeof(glasgow_config.serial));
  glasgow_config.bitstream_size = 0;
}

static __xdata char *usb_string_at_index(uint8_t index) {
  return (__xdata char *)usb_strings[index - 1];
}

// Populate descriptors from device configuration, if any.
static void descriptors_init() {
  __xdata struct usb_desc_device *desc_device = (__xdata struct usb_desc_device *)usb_device;
  __xdata char *desc_string;

  // Set revision from configuration if any, or pretend to be an unflashed device if it's missing.
  if(glasgow_config.revision != GLASGOW_REV_NA) {
    desc_device->bcdDevice = (CUR_API_LEVEL << 8) | glasgow_config.revision;
  } else {
    desc_device->idVendor  = VID_CYPRESS;
    desc_device->idProduct = PID_FX2;
  }

  // Set manufacturer from configuration if it's set. Most devices will have this field zeroed,
  // leaving the manufacturer string at the default value.
  if (glasgow_config.manufacturer[0] != '\0') {
    desc_string = usb_string_at_index(usb_device.iManufacturer);
    xmemcpy(&desc_string[0], (__xdata void *)glasgow_config.manufacturer,
            sizeof(glasgow_config.manufacturer));
  }

  // Set product based on configuration flags.
  if (glasgow_config.flags & CONFIG_FLAG_MODIFIED_DESIGN) {
    desc_string = usb_string_at_index(usb_device.iProduct);
    xmemcpy(&desc_string[0], (__xdata void *)MODIFIED_DESIGN_PRODUCT_STRING,
            sizeof(MODIFIED_DESIGN_PRODUCT_STRING) - 1); // without trailing \0
  }

  // Set serial number from configuration. Serial number must be always valid, and the firmware
  // fixes up the serial number in `config_init()` if the configuration is corrupted or missing.
  desc_string = usb_string_at_index(usb_device.iSerialNumber);
  desc_string[0] = 'A' + (glasgow_config.revision >> 4) - 1;
  desc_string[1] = '0' + (glasgow_config.revision & 0xF);
  xmemcpy(&desc_string[3], (__xdata void *)glasgow_config.serial,
          sizeof(glasgow_config.serial));
}

enum {
  // Only used by old checkouts of software, can be removed.
  USB_REQ_API_LEVEL    = 0x0F,
  // Glasgow API requests
  USB_REQ_EEPROM       = 0x10,
  USB_REQ_FPGA_CFG     = 0x11,
  USB_REQ_STATUS       = 0x12,
  USB_REQ_REGISTER     = 0x13,
  USB_REQ_IO_VOLT      = 0x14,
  USB_REQ_SENSE_VOLT   = 0x15,
  USB_REQ_ALERT_VOLT   = 0x16,
  USB_REQ_POLL_ALERT   = 0x17,
  USB_REQ_BITSTREAM_ID = 0x18,
  USB_REQ_IOBUF_ENABLE = 0x19,
  USB_REQ_LIMIT_VOLT   = 0x1A,
  USB_REQ_PULL         = 0x1B,
  USB_REQ_TEST_LEDS    = 0x1C,
  // Cypress requests
  USB_REQ_CYPRESS_EEPROM_DB = 0xA9,
  // libfx2 requests
  USB_REQ_LIBFX2_PAGE_SIZE  = 0xB0,
  // Microsoft requests
  USB_REQ_GET_MS_DESCRIPTOR = 0xC0,
};

// Test mode functions
__bit test_leds = false;

enum {
  // Status bits
  ST_ERROR    = 1<<0,
  ST_FPGA_RDY = 1<<1,
  ST_ALERT    = 1<<2,
};

// We use a self-clearing error latch. That is, when an error condition occurs,
// we light up the ERR LED, and set ST_ERROR bit in the status register.
// When the status register is next read, the ST_ERROR bit is cleared and the LED
// is turned off.
//
// The reason for this design is that stalling an OUT transfer results in
// an USB timeout, and we want to indicate error conditions faster.
static uint8_t status;

static void update_err_led() {
  if(!test_leds) {
    if(status & (ST_ERROR | ST_ALERT))
      IO_LED_ERR = 1;
    else
      IO_LED_ERR = 0;
  }
}

static void latch_status_bit(uint8_t bit) {
  status |= bit;
  update_err_led();
}

static bool reset_status_bit(uint8_t bit) {
  if(status & bit) {
    status &= ~bit;
    update_err_led();
    return true;
  }
  return false;
}

// We perform lengthy operations in the main loop to avoid hogging the interrupt.
// This flag is used for synchronization between the main loop and the ISR;
// to allow new SETUP requests to arrive while the previous one is still being
// handled (with all data received), the flag should be reset as soon as
// the entire SETUP request is parsed.
static volatile __bit pending_setup;

void handle_usb_setup(__xdata struct usb_req_setup *req) {
  req;
  if(pending_setup) {
    STALL_EP0();
  } else {
    pending_setup = true;
  }
}

uint8_t usb_alt_setting[4];

bool handle_usb_set_configuration(uint8_t config_value) {
  switch(config_value) {
    case 0: break;
    case 1: fifo_configure(/*two_ep=*/false); break;
    case 2: fifo_configure(/*two_ep=*/true);  break;
    default: return false;
  }

  usb_config_value = config_value;
  usb_alt_setting[0] = 0;
  usb_alt_setting[1] = 0;
  usb_alt_setting[2] = 0;
  usb_alt_setting[3] = 0;

  fpga_pipe_rst(/*set=*/0xf, /*clr=*/0);

  usb_reset_data_toggles(&usb_descriptor_set, /*interface=*/0xff, /*alt_setting=*/0xff);
  return true;
}

bool handle_usb_set_interface(uint8_t interface, uint8_t alt_setting) {
  bool two_ep;
  uint8_t ep_mask;

  switch(usb_config_value) {
    case 1: two_ep = false; ep_mask = 1 <<      interface;  break;
    case 2: two_ep = true;  ep_mask = 1 << (2 * interface); break;
    default: return false;
  }

  if (!fpga_pipe_rst(/*set=*/ep_mask, /*clr=*/0))
    return false;

  fifo_reset(two_ep, ep_mask);

  if(alt_setting == 1) {
    if (!fpga_pipe_rst(/*set=*/0, /*clr=*/ep_mask))
      return false;
  }

  usb_alt_setting[interface] = alt_setting;

  usb_reset_data_toggles(&usb_descriptor_set, interface, alt_setting);
  return true;
}

void handle_usb_get_interface(uint8_t interface) {
  EP0BUF[0] = usb_alt_setting[interface];
  SETUP_EP0_BUF(1);
}

// This monotonically increasing number ensures that we upload bitstream chunks
// strictly in order.
uint16_t bitstream_idx;

void handle_pending_usb_setup() {
  __xdata struct usb_req_setup *req = (__xdata struct usb_req_setup *)SETUPDAT;
  register bool req_dir_in = (req->bmRequestType & USB_DIR_IN);

  if(req->bmRequestType != (USB_RECIP_DEVICE|USB_TYPE_VENDOR|USB_DIR_IN) &&
     req->bmRequestType != (USB_RECIP_DEVICE|USB_TYPE_VENDOR|USB_DIR_OUT)) {
    goto stall_ep0_return;
  }

  // EEPROM read/write requests
  if(!req_dir_in &&
     req->bRequest == USB_REQ_LIBFX2_PAGE_SIZE) {
    pending_setup = false;

    // We have built-in knowledge of correct page sizes, ignore any supplied value.
    ACK_EP0();
    return;
  }

  if((req->bRequest == USB_REQ_CYPRESS_EEPROM_DB ||
      req->bRequest == USB_REQ_EEPROM)) {
    uint8_t  arg_chip = 0;
    uint16_t arg_addr = req->wValue;
    uint16_t arg_len  = req->wLength;
    uint8_t  page_size = 0;
    uint8_t  timeout   = 255; // 5 ms
    if(req->bRequest == USB_REQ_CYPRESS_EEPROM_DB) {
      arg_chip = I2C_ADDR_FX2_MEM;
    } else /* req->bRequest == USB_REQ_EEPROM */ {
      switch(req->wIndex) {
        case 0:
          arg_chip  = I2C_ADDR_FX2_MEM;
          page_size = 6; // 64 bytes
          break;
        case 1:
          arg_chip  = I2C_ADDR_ICE_MEM;
          page_size = 8; // 256 bytes
          break;
        case 2:
          // Same chip, different I2C address for the top half.
          arg_chip  = I2C_ADDR_ICE_MEM + 1;
          page_size = 8;
          break;
        case 3:
          // The HX8K bitstream is slightly (less than 4 KB) larger than the capacity of ICE_MEM,
          // so we stuff the very tail end of the bitstream back into FX2_MEM. It's necessary to
          // make sure the writes don't wrap, or we can overwrite the configuration info.
          if(arg_addr <= 0x1000 && arg_len <= 0x1000 && (arg_addr + arg_len) <= 0x1000) {
            arg_chip  = I2C_ADDR_FX2_MEM;
            page_size = 6; // 64 bytes
            arg_addr += 0x7000;
          }
      }
    }
    pending_setup = false;

    if(!arg_chip) {
      goto stall_ep0_return;
    }

    while(arg_len > 0) {
      uint8_t chunk_len = arg_len < 64 ? arg_len : 64;

      if(req_dir_in) {
        while(EP0CS & _BUSY);
        if(!eeprom_read(arg_chip, arg_addr, EP0BUF, chunk_len, /*double_byte=*/true)) {
          goto stall_ep0_return;
        }
        SETUP_EP0_BUF(chunk_len);
      } else {
        SETUP_EP0_BUF(0);
        while(EP0CS & _BUSY);
        if(!eeprom_write(arg_chip, arg_addr, EP0BUF, chunk_len, /*double_byte=*/true,
                         page_size, timeout)) {
          goto stall_ep0_return;
        }
      }

      arg_len  -= chunk_len;
      arg_addr += chunk_len;
    }

    return;
  }

  // FPGA register read/write requests
  if(req->bRequest == USB_REQ_REGISTER) {
    uint8_t  arg_addr = req->wValue;
    uint16_t arg_len  = req->wLength;
    pending_setup = false;

    if(fpga_reg_select(arg_addr)) {
      if(req_dir_in) {
        while(EP0CS & _BUSY);
        if(fpga_reg_read(EP0BUF, arg_len)) {
          SETUP_EP0_BUF(arg_len);
          return;
        }
      } else {
        SETUP_EP0_BUF(0);
        while(EP0CS & _BUSY);
        fpga_reg_write(EP0BUF, arg_len);
        return;
      }
    }

    goto stall_ep0_return;
  }

  // Device status request
  if(req_dir_in &&
     req->bRequest == USB_REQ_STATUS &&
     req->wLength == 1) {
    pending_setup = false;

    while(EP0CS & _BUSY);
    EP0BUF[0] = status |
      (fpga_is_ready() ? ST_FPGA_RDY : 0);
    SETUP_EP0_BUF(1);

    reset_status_bit(ST_ERROR);

    return;
  }

  // Bitstream download request
  if(!req_dir_in &&
     req->bRequest == USB_REQ_FPGA_CFG &&
     (req->wIndex == 0 || req->wIndex == bitstream_idx + 1)) {
    uint16_t arg_idx = req->wIndex;
    uint16_t arg_len = req->wLength;
    pending_setup = false;

    if(arg_idx == 0) {
      memset(glasgow_config.bitstream_id, 0, CONFIG_SIZE_BITSTREAM_ID);
      fpga_reset();
    }

    while(arg_len > 0) {
      uint8_t chunk_len = arg_len < 64 ? arg_len : 64;

      SETUP_EP0_BUF(0);
      while(EP0CS & _BUSY);
      fpga_load(EP0BUF, chunk_len);

      arg_len -= chunk_len;
    }

    bitstream_idx = arg_idx;
    return;
  }

  // Bitstream ID get/set request
  if(req->bRequest == USB_REQ_BITSTREAM_ID &&
     req->wLength == CONFIG_SIZE_BITSTREAM_ID) {
    pending_setup = false;

    if(req_dir_in) {
      while(EP0CS & _BUSY);
      xmemcpy(EP0BUF, glasgow_config.bitstream_id, CONFIG_SIZE_BITSTREAM_ID);
      SETUP_EP0_BUF(CONFIG_SIZE_BITSTREAM_ID);
    } else {
      if(fpga_start()) {
        SETUP_EP0_BUF(0);
        while(EP0CS & _BUSY);
        xmemcpy(glasgow_config.bitstream_id, EP0BUF, CONFIG_SIZE_BITSTREAM_ID);
      } else {
        goto stall_ep0_return;
      }
    }

    return;
  }

  // I/O voltage get/set request
  if(req->bRequest == USB_REQ_IO_VOLT &&
     req->wLength == 2) {
    uint8_t  arg_mask = req->wIndex;
    pending_setup = false;

    if(req_dir_in) {
      while(EP0CS & _BUSY);
      if(!iobuf_get_voltage(arg_mask, (__xdata uint16_t *)EP0BUF)) {
        goto stall_ep0_return;
      } else {
        SETUP_EP0_BUF(2);
      }
    } else {
      SETUP_EP0_BUF(2);
      while(EP0CS & _BUSY);
      if(!iobuf_set_voltage(arg_mask, (__xdata uint16_t *)EP0BUF)) {
        latch_status_bit(ST_ERROR);
      }
    }

    return;
  }

  // Voltage sense request
  if(req_dir_in &&
     req->bRequest == USB_REQ_SENSE_VOLT &&
     req->wLength == 2) {
    uint8_t  arg_mask = req->wIndex;
    pending_setup = false;
    bool result;

    while(EP0CS & _BUSY);

    if(glasgow_config.revision >= GLASGOW_REV_C2)
      result = iobuf_measure_voltage_ina233(arg_mask, (__xdata uint16_t *)EP0BUF);
    else
      result = iobuf_measure_voltage_adc081c(arg_mask, (__xdata uint16_t *)EP0BUF);

    if(!result) {
      goto stall_ep0_return;
    } else {
      SETUP_EP0_BUF(2);
    }

    return;
  }

  // Voltage alert get/set request
  if(req->bRequest == USB_REQ_ALERT_VOLT &&
     req->wLength == 4) {
    uint8_t  arg_mask = req->wIndex;
    pending_setup = false;
    bool result;

    if(req_dir_in) {
      while(EP0CS & _BUSY);

      if(glasgow_config.revision >= GLASGOW_REV_C2)
        result = iobuf_get_alert_ina233(arg_mask, (__xdata uint16_t *)EP0BUF, (__xdata uint16_t *)EP0BUF + 1);
      else
        result = iobuf_get_alert_adc081c(arg_mask, (__xdata uint16_t *)EP0BUF, (__xdata uint16_t *)EP0BUF + 1);

      if(!result) {
        goto stall_ep0_return;
      } else {
        SETUP_EP0_BUF(4);
      }
    } else {
      SETUP_EP0_BUF(4);
      while(EP0CS & _BUSY);

      if(glasgow_config.revision >= GLASGOW_REV_C2)
        result = iobuf_set_alert_ina233(arg_mask, (__xdata uint16_t *)EP0BUF, (__xdata uint16_t *)EP0BUF + 1);
      else
        result = iobuf_set_alert_adc081c(arg_mask, (__xdata uint16_t *)EP0BUF, (__xdata uint16_t *)EP0BUF + 1);

      if(!result) {
        latch_status_bit(ST_ERROR);
      }
    }

    return;
  }

  // Alert poll request
  if(req_dir_in &&
     req->bRequest == USB_REQ_POLL_ALERT &&
     req->wLength == 1) {
    pending_setup = false;
    bool result = true;

    while(EP0CS & _BUSY);

    // Read out the alert status and also clear the alert status (or cache)
    if(glasgow_config.revision >= GLASGOW_REV_C2)
      iobuf_read_alert_cache_ina233(EP0BUF, /*clear=*/true);
    else
      result = iobuf_poll_alert_adc081c(EP0BUF, /*clear=*/true);

    if(!result) {
      goto stall_ep0_return;
    } else {
      SETUP_EP0_BUF(1);
      // Clear the ERR led since we cleared the alert status above
      reset_status_bit(ST_ALERT);
    }

    return;
  }

  // I/O buffer enable request
  if(!req_dir_in &&
     req->bRequest == USB_REQ_IOBUF_ENABLE &&
     req->wLength == 0) {
    bool arg_enable = req->wValue;
    pending_setup = false;

    iobuf_enable(arg_enable);
    ACK_EP0();

    return;
  }

  // I/O voltage limit get/set request
  if(req->bRequest == USB_REQ_LIMIT_VOLT &&
     req->wLength == 2) {
    uint8_t  arg_mask = req->wIndex;
    pending_setup = false;

    if(req_dir_in) {
      while(EP0CS & _BUSY);
      if(!iobuf_get_voltage_limit(arg_mask, (__xdata uint16_t *)EP0BUF)) {
        goto stall_ep0_return;
      } else {
        SETUP_EP0_BUF(2);
      }
    } else {
      SETUP_EP0_BUF(2);
      while(EP0CS & _BUSY);
      if(!iobuf_set_voltage_limit(arg_mask, (__xdata uint16_t *)EP0BUF)) {
        latch_status_bit(ST_ERROR);
      } else {
        if(!eeprom_write(I2C_ADDR_FX2_MEM,
                         8 + 4 + __builtin_offsetof(struct glasgow_config, voltage_limit),
                         (__xdata void *)&glasgow_config.voltage_limit,
                         sizeof(glasgow_config.voltage_limit),
                         /*double_byte=*/true, /*page_size=*/8, /*timeout=*/255)) {
          latch_status_bit(ST_ERROR);
        }
      }
    }

    return;
  }

  // Pull resistor get/set request
  if(req->bRequest == USB_REQ_PULL &&
     req->wLength == 2) {
    uint8_t  arg_selector = req->wIndex;
    pending_setup = false;

    if(req_dir_in){
      while(EP0CS & _BUSY);
      if(glasgow_config.revision < GLASGOW_REV_C0 ||
         !iobuf_get_pull(arg_selector,
                         (__xdata uint8_t *)EP0BUF + 0,
                         (__xdata uint8_t *)EP0BUF + 1)) {
        goto stall_ep0_return;
      } else {
        SETUP_EP0_BUF(2);
      }
    } else {
      SETUP_EP0_BUF(2);
      while(EP0CS & _BUSY);
      if(glasgow_config.revision < GLASGOW_REV_C0 ||
         !iobuf_set_pull(arg_selector,
                         *((__xdata uint8_t *)EP0BUF + 0),
                         *((__xdata uint8_t *)EP0BUF + 1))) {
        latch_status_bit(ST_ERROR);
      }
    }

    return;
  }

  // LED test mode request
  if(!req_dir_in &&
     req->bRequest == USB_REQ_TEST_LEDS &&
     req->wLength == 0) {
    uint8_t arg_states = req->wIndex;
    pending_setup = false;

    // Exit LED testing mode by resetting the device.
    test_leds = true;
    IOD &=             ~(0xf  << PIND_LED_FX2);
    IOD |= (arg_states & 0xf) << PIND_LED_FX2;
    ACK_EP0();

    return;
  }

  // Only used by old checkouts of software, can be removed.
  if(req_dir_in &&
     req->bRequest == USB_REQ_API_LEVEL &&
     req->wLength == 1) {
    pending_setup = false;

    while(EP0CS & _BUSY);
    EP0BUF[0] = CUR_API_LEVEL;
    SETUP_EP0_BUF(1);
    return;
  }

  // Microsoft descriptor requests
  if(req_dir_in &&
     req->bRequest == USB_REQ_GET_MS_DESCRIPTOR &&
     req->wIndex == USB_DESC_MS_EXTENDED_COMPAT_ID) {
    pending_setup = false;

    xmemcpy(scratch, (__xdata void *)&usb_ms_ext_compat_id, usb_ms_ext_compat_id.dwLength);
    SETUP_EP0_IN_DESC(scratch);
    return;
  }
  if(req_dir_in &&
     req->bRequest == USB_REQ_GET_MS_DESCRIPTOR &&
     req->wIndex == USB_DESC_MS_EXTENDED_PROPERTIES) {
    pending_setup = false;

    xmemcpy(scratch, (__xdata void *)&usb_ms_ext_properties, usb_ms_ext_properties.dwLength);
    SETUP_EP0_IN_DESC(scratch);
    return;
  }

  // Factor out the stall exit to reduce code size.
stall_ep0_return:
  pending_setup = false;
  STALL_EP0();
}

// Directly use the irq enable register EX0 to notify about a pending alert to avoid using
// a separate variable which could get out of sync.
// Define it to armed_alert to document this usage pattern
#define armed_alert EX0

void isr_IE0() __interrupt(_INT_IE0) {
  // INT_IE0 is level triggered, the ~ALERT line is continuously pulled low by the ADC
  // So disable this irq unil we have fully handled it, otherwise it permanently triggers
  armed_alert = false;
}

void handle_pending_alert() {
  __xdata uint8_t mask;
  __xdata uint16_t millivolts = 0;

  // switch on the ERR led
  latch_status_bit(ST_ALERT);

  if(glasgow_config.revision >= GLASGOW_REV_C2) {
    iobuf_poll_alert_ina233(&mask);
    // the ~ALERT line was not yet cleared by this call
  } else {
    iobuf_poll_alert_adc081c(&mask, /*clear=*/false);
    // the ~ALERT line was cleared by this call
  }

  // TODO: handle i2c comms errors of above calls

  // permanently switch off the voltage regulators of the ports we got a alert on
  iobuf_set_voltage(mask, &millivolts);

  if(glasgow_config.revision >= GLASGOW_REV_C2) {
    // only clear the ~ALERT line after the port vio has been disabled
    // this prevents re-enabling the port voltage for a short time
    // since on revC2 ~ALERT already disables the respective Vreg on a hw level
    iobuf_clear_alert_ina233(mask);
  }

  // the ADC that pulled the ~ALERT line should have released it by now
  // so we can re-enable the interrupt to catch the next alert
  armed_alert = true;
}

void isr_TF2() __interrupt(_INT_TF2) {
  if (!test_leds)
    IO_LED_ACT = 0;
  TR2 = false;
  TF2 = false;
}

static void isr_EPn() __interrupt {
  if (!test_leds)
    IO_LED_ACT = 1;
  // Just let it run, at the maximum reload value we get a pulse width of around 16ms.
  TR2 = true;
  // Clear all EPn IRQs, since we don't really need this IRQ to be fine-grained.
  CLEAR_USB_IRQ();
  EPIRQ = _EPI_EP0IN|_EPI_EP0OUT|_EPI_EP2|_EPI_EP4|_EPI_EP6|_EPI_EP8;
}

void isr_EP0IN()  __interrupt __naked { __asm ljmp _isr_EPn __endasm; }
void isr_EP0OUT() __interrupt __naked { __asm ljmp _isr_EPn __endasm; }
void isr_EP2()    __interrupt __naked { __asm ljmp _isr_EPn __endasm; }
void isr_EP4()    __interrupt __naked { __asm ljmp _isr_EPn __endasm; }
void isr_EP6()    __interrupt __naked { __asm ljmp _isr_EPn __endasm; }
void isr_EP8()    __interrupt __naked { __asm ljmp _isr_EPn __endasm; }

int main() {
  // Run at 48 MHz, drive CLKOUT.
  CPUCS = _CLKOE|_CLKSPD1;

  // All of our I2C devices can run at 400 kHz.
  I2CTL = _400KHZ;

  // Initialize subsystems.
  config_init();
  descriptors_init();
  iobuf_init_dac_ldo();

  if(glasgow_config.revision >= GLASGOW_REV_C2) {
    if (!iobuf_init_adc_ina233())
      latch_status_bit(ST_ERROR);
  }
  else
    iobuf_init_adc_adc081c();

  fpga_init();
  fifo_init();

  // Disable EP1IN/OUT
  SYNCDELAY;
  EP1INCFG = 0;
  SYNCDELAY;
  EP1OUTCFG = 0;

  // Set up LEDs.
  OED |= (1<<PIND_LED_FX2)|(1<<PIND_LED_ACT)|(1<<PIND_LED_ERR);
  IO_LED_FX2 = 1;
  IOD &=                 ~((1<<PIND_LED_ACT)|(1<<PIND_LED_ERR));

  // Use timer 2 in 16-bit timer mode for ACT LED.
  T2CON = _CPRL2;
  ET2 = true;

  // Set up endpoint interrupts for ACT LED.
  EPIE |= _EPI_EP0IN|_EPI_EP0OUT|_EPI_EP2|_EPI_EP4|_EPI_EP6|_EPI_EP8;

  // Set up interrupt for ADC ALERT, see documentation at the armed_alert definition for details
  armed_alert = true;

  // If there's a bitstream flashed, load it.
  if(glasgow_config.bitstream_size > 0) {
    uint32_t length = glasgow_config.bitstream_size;
    uint8_t  chip = I2C_ADDR_ICE_MEM;
    uint16_t addr = 0;

    // Loading the bitstream over I2C can take up to five seconds.
    IO_LED_ACT = 1;

    fpga_reset();
    while(length > 0) {
      uint8_t chunk_len = 0x80;
      if(length < chunk_len)
        chunk_len = length;

      if(!eeprom_read(chip, addr, (uint8_t *)&scratch, chunk_len, /*double_byte=*/true)) {
        latch_status_bit(ST_ERROR);
        break;
      }
      fpga_load((__xdata uint8_t *)scratch, chunk_len);

      length -= chunk_len;
      addr   += chunk_len;
      if(addr == 0) {
        // Advance to the next logical chip in case of address wraparound.
        chip += 1;
        if(chip == I2C_ADDR_ICE_MEM + 2) {
          // See explanation in USB_REQ_EEPROM.
          chip  = I2C_ADDR_FX2_MEM;
          addr += 0x7000;
        }
      }
    }
    if(length == 0) {
      if(!fpga_start())
        latch_status_bit(ST_ERROR);
    }

    IO_LED_ACT = 0;
  }

  // Finally, enumerate.
  usb_init(/*reconnect=*/true);

  while(1) {
    // Handle pending events.
    if(pending_setup)
      handle_pending_usb_setup();
    if(!armed_alert)
      handle_pending_alert();

    // There are few things more frustrating than having your debug tools fail you.
    // Power-only USB cables are regretfully common. If the device finds itself without
    // an address it should indicate this unusual condition, though in a gentle way
    // because there are legitimate reasons for this to happen (PC in suspend, Glasgow
    // used 'offline', etc).
    if(!test_leds) {
      if(FNADDR == 0) {
        // If no address is assigned, slowly breathe. (Or, during enumeration, abruptly
        // blink. That's okay though.)
        switch (USBFRAMEH >> 1) {
          case 0b00: IO_LED_FX2 = 1; break;
          case 0b10: IO_LED_FX2 = 0; break;
          case 0b01:
          case 0b11: IO_LED_FX2 ^= 1; break;
        }
      } else {
        // Got plugged in, light up permanently.
        IO_LED_FX2 = 1;
      }
    }
  }
}
