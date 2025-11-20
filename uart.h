#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);
void uart_putc(char c);
void uart_print(const char *s);
void uart_print_dec(uint32_t val);

#endif
