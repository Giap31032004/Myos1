#include "uart.h"
#include "systick.h"
#include "process.h"

/* --- CẤU HÌNH HỆ THỐNG --- */
#define SYSTEM_CLOCK      80000000
#define SYSTICK_RATE      8000000   // 10 tick/giây (đảo task nhanh hơn để thấy phản ứng)

/* Biến toàn cục "Giả lập phần cứng" (Shared Resource) */
/* LƯU Ý: Trong OS thực tế, truy cập biến này cần Mutex để tránh tranh chấp! */
volatile int current_temperature = 25; 
volatile int system_uptime = 0;

/* Hàm Delay giả lập (như đã bàn trước đó) */
void delay(volatile unsigned int count) {
    while (count--) {
        __asm("nop");
    }
}

/* * TASK 1: SENSOR NODE
 * Nhiệm vụ: Giả lập đọc cảm biến và cập nhật nhiệt độ sau mỗi khoảng thời gian.
 */
void task_sensor_update(void) {
    int direction = 1; // 1 là tăng, -1 là giảm
    
    while (1) {
        // 1. Giả lập thời gian đọc cảm biến
        delay(1000000000); 

        // 2. Cập nhật nhiệt độ
        if (direction == 1) {
            current_temperature += 5;
            if (current_temperature >= 50) direction = -1; // Nóng quá thì giảm
        } else {
            current_temperature -= 5;
            if (current_temperature <= 20) direction = 1;  // Lạnh quá thì tăng
        }
        
        // Debug log nhẹ
        // uart_print("[SENSOR] Reading sensor data...\r\n");
    }
}

/* * TASK 2: DISPLAY NODE
 * Nhiệm vụ: Hiển thị thông số ra màn hình console cho người dùng.
 */
void task_display(void) {
    while (1) {
        // 1. Refresh rate của màn hình (chậm hơn sensor)
        delay(2000000000);

        // 2. In giao diện
        uart_print("----------------------\r\n");
        uart_print("| SYSTEM MONITOR     |\r\n");
        uart_print("| Temp: "); 
        uart_print_dec(current_temperature); 
        uart_print(" C         |\r\n");
        uart_print("----------------------\r\n");
    }
}

/* * TASK 3: ALARM SYSTEM
 * Nhiệm vụ: Giám sát liên tục, phản ứng NGAY LẬP TỨC nếu quá nhiệt.
 */
void task_alarm(void) {
    while (1) {
        // Task này kiểm tra rất nhanh
        delay(500000000);

        // Logic kiểm tra an toàn
        if (current_temperature > 40) {
            uart_print("!!! [ALARM] WARNING: OVERHEAT DETECTED !!!\r\n");
            uart_print("!!! [ALARM] ACTIVATING COOLING FAN...  !!!\r\n");
        }
    }
}

/* --- MAIN --- */
void main(void) {
    uart_init();
    
    uart_print("\033[2J"); // Lệnh xóa màn hình terminal (nếu hỗ trợ)
    uart_print("MyOS IoT System Booting...\r\n");
    delay(50000000); // Chờ khởi động

    process_init();

    /* Tạo các task với chức năng cụ thể */
    process_create(task_sensor_update, 0); // PID 0
    process_create(task_display, 1);       // PID 1
    process_create(task_alarm, 2);         // PID 2

    process_admit_jobs();

    /* Khởi động nhịp tim hệ thống */
    systick_init(SYSTICK_RATE);

    while (1) {
        // Idle task: Có thể dùng để tính toán uptime hoặc ngủ tiết kiệm điện
        // Ở đây ta để trống để nhường CPU cho các task kia
    }
}