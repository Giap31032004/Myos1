#ifndef PROCESS_H
#define PROCESS_H
#include <stdint.h>

#define MAX_PROCESSES 5
#define STACK_SIZE 256

typedef enum {
    PROC_NEW,
    PROC_READY,
    PROC_RUNNING,
    PROC_SUSPENDED,
    PROC_BLOCKED
} process_state_t;

typedef struct PCB {
    /* --- PHẦN CỐT LÕI (Context Switching) --- */
    uint32_t *stack_ptr;       // Con trỏ stack (quan trọng nhất)
    
    /* --- PHẦN ĐỊNH DANH --- */
    uint32_t pid;              // ID tiến trình
    void (*entry)(void);       // Hàm main của task (để debug hoặc reset task)
    
    /* --- PHẦN TRẠNG THÁI & BLOCKING --- */
    process_state_t state;     // READY, RUNNING, BLOCKED...
    uint32_t wake_up_tick;     // [QUAN TRỌNG] Phải là uint32_t để tránh tràn số
                               // Thời điểm (tick hệ thống) mà task sẽ thức dậy

    /* --- PHẦN LẬP LỊCH (SCHEDULING) --- */
    uint8_t base_priority;     // Độ ưu tiên gốc (Cài đặt ban đầu)
    uint8_t current_priority;  // Độ ưu tiên động (Dùng để lập lịch thực tế)
    
    uint8_t time_slice;        // Số tick còn lại trong lượt chạy hiện tại (Round-robin quota)
    
    /* --- PHẦN THỐNG KÊ (Tùy chọn) --- */
    uint32_t total_cpu_runtime; // Tổng số tick task này đã chiếm dụng từ lúc khởi động
                                // Dùng uint32_t để không bị tràn sớm
} PCB_t;

void process_init(void);
void process_create(void (*func)(void), uint32_t pid);
void process_admit_jobs(void);
void process_schedule(void);
void process_set_state(uint32_t pid, process_state_t new_state);
const char* process_state_str(process_state_t state);

#endif
