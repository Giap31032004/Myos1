#include "uart.h"

#define UART0_BASE  0x4000C000
#define UART0_DR    (*(volatile unsigned int*)(UART0_BASE + 0x00))
#define UART0_FR    (*(volatile unsigned int*)(UART0_BASE + 0x18))
#define TXFF        (1 << 5)

void uart_init(void) {
    // QEMU lm3s6965evb khởi tạo UART0 sẵn
}

void uart_putc(char c) {
    while (UART0_FR & TXFF); // chờ đến khi có chỗ trống
    UART0_DR = c;
}

void uart_print(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

void uart_print_dec(uint32_t val) {
    char buf[10];
    int i = 0;

    if (val == 0) {
        uart_putc('0');
        return;
    }

    while (val > 0 && i < sizeof(buf)) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }

    while (i > 0) {
        uart_putc(buf[--i]);
    }
}
