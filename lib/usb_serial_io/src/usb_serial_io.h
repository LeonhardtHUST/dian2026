#ifndef USB_SERIAL_IO_H
#define USB_SERIAL_IO_H

#include <stdarg.h>
#include <stdint.h>

typedef struct {
    int length;
    void* data;
} usbio_rx_data_t;

extern const char *endl;

extern void usbio_init(const uint8_t n);
extern uint8_t usbio_get_buffer_size(void);
extern int usbio_print(const uint32_t ms_to_wait, const char* str);
extern int usbio_print_multi(const uint32_t ms_to_wait, const uint8_t num, const char* str, ...);
extern usbio_rx_data_t usbio_read(const uint32_t ms_to_wait);
extern void usbio_rx_data_destruct(usbio_rx_data_t* rx_data);

#endif // USB_SERIAL_IO_H
