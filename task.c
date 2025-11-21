#include "task.h"
#include "uart.h"
#include "process.h" // Thêm cái này để dùng os_delay
#include "sync.h"    // Thêm cái này để dùng mutex
#include <stdint.h>

/* Biến toàn cục */
volatile int current_temperature = 25; 
volatile int system_uptime = 0;
extern os_mutex_t app_mutex;

/* TASK 1: SENSOR */
void task_sensor_update(void) {
    int direction = 1; 
    while (1) {
        os_delay(10); 

        mutex_lock(&app_mutex);
        if (direction == 1) {
            current_temperature += 5;
            if (current_temperature >= 55) direction = -1; 
        } else {
            current_temperature -= 5;
            if (current_temperature <= 20) direction = 1;
        }
        mutex_unlock(&app_mutex);
    }
}

/* TASK 2: DISPLAY */
void task_display(void) {
    int last_temp = -999; 

    while (1) {
        // SỬA: Đưa lock vào trong vòng lặp
        mutex_lock(&app_mutex);
        
        if (current_temperature != last_temp) {
            uart_print("----------------------\r\n");
            uart_print("| Temp: "); 
            uart_print_dec(current_temperature); 
            uart_print(" C         |\r\n");
            uart_print("----------------------\r\n");
            last_temp = current_temperature; 
        }
        
        // SỬA: Unlock ngay sau khi in xong
        mutex_unlock(&app_mutex);
        
        os_delay(20); // Tăng delay lên chút cho dễ nhìn
    }
}

/* TASK 3: ALARM */
void task_alarm(void) {
    int alarm_active = 0; 

    while (1) {
        os_delay(10);
        
        mutex_lock(&app_mutex);
        if (current_temperature > 40) {
            if (alarm_active == 0) {
                uart_print("\r\n!!! [ALARM] WARNING: OVERHEAT !!!\r\n");
                alarm_active = 1; 
            }
        } 
        else {
            if (alarm_active == 1) {
                uart_print("\r\n[ALARM] Temperature Normal.\r\n");
                alarm_active = 0; 
            }
        }
        mutex_unlock(&app_mutex);
    }
}