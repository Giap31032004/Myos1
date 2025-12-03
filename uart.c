#include "uart.h"
#include "sync.h"

#define UART0_BASE  0x4000C000
#define UART0_DR (*(volatile unsigned int*)(UART0_BASE + 0x00))
#define UART0_FR (*(volatile uint32_t*)(UART0_BASE + 0x018))
#define UART0_IM (*(volatile uint32_t*)(UART0_BASE + 0x038))
#define UART0_ICR (*(volatile uint32_t*)(UART0_BASE + 0x044))

#define NVIC_EN0 (*(volatile uint32_t*)0xE000E100) 

#define UART_RXFE      (1 << 4) 
#define UART_TXFF      (1 << 5)
#define UART_RXIM      (1 << 4) 

#define RX_BUFFER_SIZE 128
static char rx_bufferr[RX_BUFFER_SIZE];
static int rx_head = 0;
static int rx_tail = 0;

os_sem_t uart_rx_semaphore;

void uart_init(void) {
    sem_init(&uart_rx_semaphore, 0);
    UART0_IM |= UART_RXIM;
    NVIC_EN0 |= (1 << 5); 
}

void uart_putc(char c) {
    while (UART0_FR & UART_TXFF); // chờ đến khi có chỗ trống
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

void UART0_Handler(void) {
    UART0_ICR |= UART_RXIM;
    while((UART0_FR & UART_RXFE) == 0){
        char c = (char)(UART0_DR & 0xFF);
        int next_head = (rx_head + 1) % RX_BUFFER_SIZE;
        if (next_head != rx_tail) {
            rx_bufferr[rx_head] = c;
            rx_head = next_head;
            sem_signal(&uart_rx_semaphore);
        }
    }
}


char uart_getc(void) {
    sem_wait(&uart_rx_semaphore);
    OS_ENTER_CRITICAL();

    char c = rx_bufferr[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;

    OS_EXIT_CRITICAL();

    return c;
}