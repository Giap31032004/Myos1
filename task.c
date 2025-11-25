#include "task.h"
#include "uart.h"
#include "process.h" 
#include "sync.h"    
#include "ipc.h"
#include <stdint.h>

/* Biến toàn cục */
volatile int current_temperature = 25; 
volatile int system_uptime = 0;
extern os_mutex_t app_mutex;
extern os_msg_queue_t temp_queue;

/* TASK 1: SENSOR */
void task_sensor_update(void) {
    int local_temp = 25; 
    int direction = 1; 
    while (1) {
        os_delay(10); 
        
        if(direction == 1){
            local_temp += 5;
            if(local_temp >= 55) direction = -1;
        } else {
            local_temp -= 5;
            if(local_temp <= 20) direction = 1;
        }

        msg_queue_send(&temp_queue, local_temp);
    }
}

/* TASK 2: DISPLAY */
void task_display(void) {
    int received_temp; 

    while (1) {
        // 1. Nhận dữ liệu từ hàng đợi IPC
        received_temp = msg_queue_receive(&temp_queue);
        
        // 2. Cập nhật biến toàn cục (để Task Alarm dùng nếu cần)
        current_temperature = received_temp;

        // 3. In ra màn hình giá trị VỪA NHẬN ĐƯỢC
        mutex_lock(&app_mutex);
            uart_print("----------------------\r\n");
            uart_print("| Temp: "); 
            
            // SỬA QUAN TRỌNG: In received_temp (số mới) thay vì current_temperature (số cũ)
            uart_print_dec(received_temp); 
            
            uart_print(" C         |\r\n");
            uart_print("----------------------\r\n");
        mutex_unlock(&app_mutex);
    }
}

/* TASK 3: ALARM */
void task_alarm(void) {
    int alarm_active = 0; 

    while (1) {
        os_delay(5);
        
        mutex_lock(&app_mutex);
        if (current_temperature > 40) {
            if (alarm_active == 0) {
                uart_print("\r\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
                uart_print("\r\n!!! [ALARM] WARNING: OVERHEAT !!!\r\n");
                uart_print("\r\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
                alarm_active = 1; 
            }
        } 
        else {
            if (alarm_active == 1) {
                uart_print("\r\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
                uart_print("\r\n[ALARM] Temperature Normal.\r\n");                
                uart_print("\r\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
                alarm_active = 0; 
            }
        }
        mutex_unlock(&app_mutex);
    }
}