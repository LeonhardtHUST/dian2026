#ifndef USB_SERIAL_IO_H
#define USB_SERIAL_IO_H

#include <stdarg.h>
#include <stdint.h>

typedef struct {
    int length;
    void* data;
} USB_RX_Data;

extern const char *endl;

extern void usb_io_init(const uint8_t n);
extern uint8_t usb_io_get_buffer_size(void);
extern int usb_print(const uint32_t ms_to_wait, const char* str);
extern int usb_print_multi(const uint32_t ms_to_wait, const uint8_t num, const char* str, ...);
extern USB_RX_Data app_usb_read(const uint32_t ms_to_wait);
extern void usb_rx_data_destruct(USB_RX_Data* rx_data);

#endif // USB_SERIAL_IO_H
