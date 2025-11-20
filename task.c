#include "task.h"
#include "uart.h"
#include <stdint.h>

/* Biến toàn cục */
volatile int current_temperature = 25; 
volatile int system_uptime = 0;

/* ------------------------------------------------
   TASK 1: SENSOR (Cập nhật 1 giây/lần)
   ------------------------------------------------ */
void task_sensor_update(void) {
    int direction = 1; 
    while (1) {
        os_delay(10); // 1 giây (với 10 tick/s)

        if (direction == 1) {
            current_temperature += 5;
            if (current_temperature >= 55) direction = -1; // Tăng max lên 55 cho dễ test
        } else {
            current_temperature -= 5;
            if (current_temperature <= 20) direction = 1;
        }
    }
}

/* ------------------------------------------------
   TASK 2: DISPLAY (Cập nhật 5 giây/lần - RẤT CHẬM)
   ------------------------------------------------ */
void task_display(void) {
    int last_temp = -999; // Lưu nhiệt độ cũ

    while (1) {
        // Chỉ in lại nếu nhiệt độ đã thay đổi
        if (current_temperature != last_temp) {
            uart_print("----------------------\r\n");
            uart_print("| Temp: "); 
            uart_print_dec(current_temperature); 
            uart_print(" C         |\r\n");
            uart_print("----------------------\r\n");
            
            last_temp = current_temperature; // Cập nhật
        }

        os_delay(10); // Kiểm tra mỗi 1 giây
    }
}

/* ------------------------------------------------
   TASK 3: ALARM (Kiểm tra nhanh nhưng báo thông minh)
   ------------------------------------------------ */
void task_alarm(void) {
    int alarm_active = 0; // Cờ trạng thái để tránh spam

    while (1) {
        // Kiểm tra 0.5 giây/lần
        os_delay(5);

        if (current_temperature > 40) {
            // Chỉ in cảnh báo MỘT LẦN khi mới phát hiện
            if (alarm_active == 0) {
                uart_print("\r\n\r\n"); // Xuống dòng tạo khoảng cách
                uart_print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
                uart_print("!!! [ALARM] WARNING: OVERHEAT !!!\r\n");
                uart_print("!!! [ALARM] COOLING FAN ON    !!!\r\n");
                uart_print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
                alarm_active = 1; // Đánh dấu đã báo rồi -> Không in nữa
            }
        } 
        else {
            // Khi nhiệt độ giảm xuống an toàn
            if (alarm_active == 1) {
                uart_print("\r\n[ALARM] Temperature Normal. Fan OFF.\r\n");
                alarm_active = 0; // Reset cờ
            }
        }
    }
}