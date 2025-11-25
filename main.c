#include "uart.h"
#include "systick.h"
#include "process.h"
#include "task.h"
#include "sync.h" 
#include "ipc.h"
#include <stdint.h>


/* --- CẤU HÌNH HỆ THỐNG --- */
#define SYSTEM_CLOCK      80000000
#define SYSTICK_RATE      2000000   // 10 tick/giây (đảo task nhanh hơn để thấy phản ứng)

os_msg_queue_t temp_queue; // Hàng đợi tin nhắn cho nhiệt độ
os_mutex_t app_mutex; // chiếc khóa chung cho cả hệ thống

void delay(volatile unsigned int count) {
    while (count--) {
        __asm("nop");
    }
}

/* --- MAIN --- */
void main(void) {
    uart_init();
    process_init();

    msg_queue_init(&temp_queue);
    mutex_init(&app_mutex);
    
    uart_print("\033[2J"); // Lệnh xóa màn hình terminal (nếu hỗ trợ)
    uart_print("MyOS IoT System Booting...\r\n");
    delay(5000000); // Chờ khởi động

    process_init();
    mutex_init(&app_mutex);

    /* Tạo các task với chức năng cụ thể */
    process_create(task_sensor_update, 0, 4); // PID 0
    process_create(task_display, 1, 2);       // PID 1
    process_create(task_alarm, 2, 3);         // PID 2

    process_admit_jobs();

    /* Khởi động nhịp tim hệ thống */
    systick_init(SYSTICK_RATE);

    while (1) {
        // Idle task: Có thể dùng để tính toán uptime hoặc ngủ tiết kiệm điện
        // Ở đây ta để trống để nhường CPU cho các task kia
    }
}