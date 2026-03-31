#include "utils.h"

int max_list(const uint8_t num, const int n, ...) {
    if (num == 0) return 0;
    int maximum = n;
    va_list args;
    va_start(args, n);
    for (uint8_t i = 1; i < num; i++) {
        int val = va_arg(args, int);
        if (val > maximum) {
            maximum = val;
        }
    }
    va_end(args);
    return maximum;
}