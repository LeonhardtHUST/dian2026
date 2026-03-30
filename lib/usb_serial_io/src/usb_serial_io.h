#ifndef USB_SERIAL_IO_H
#define USB_SERIAL_IO_H

#include <stdarg.h>

static const char *DEFAULT_TAG = "USB_APP";
#define BUF_SIZE 128

extern const char *endl;

extern void usb_io_set_buffer_size(const uint8_t n);
extern uint8_t usb_io_get_buffer_size(void);
extern int usb_print(const char* str, const uint32_t ms_to_wait)
extern void usb_print_multi(const uint8_t num, const char* str, ...);

#endif // USB_SERIAL_IO_H