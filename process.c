#include "process.h"
#include "uart.h"
#include "queue.h"
#include <stdint.h>

#define SCB_ICSR (*(volatile uint32_t*)0xE000ED04)
#define PENDSVSET_BIT (1UL << 28)

volatile uint32_t tick_count = 0; // Biến đếm tick hệ thống

PCB_t *current_pcb = NULL;   
PCB_t *next_pcb = NULL;      

extern void start_first_task(uint32_t *first_sp); 
//extern void switch_context(uint32_t **old_sp_ptr, uint32_t *new_sp);

queue_t job_queue;
queue_t ready_queue;
queue_t device_queue;

static uint32_t stacks[MAX_PROCESSES][STACK_SIZE];
static PCB_t pcb_table[MAX_PROCESSES];

static int total_processes = 0; 

const char* process_state_str(process_state_t state) {
    switch (state) {
        case PROC_NEW:        return "NEW";
        case PROC_READY:      return "READY";
        case PROC_RUNNING:    return "RUNNING";
        case PROC_SUSPENDED:  return "SUSPENDED";
        case PROC_BLOCKED:    return "BLOCKED";
        default:              return "UNKNOWN";
    }
}

void process_init(void) {
    uart_print("Process system initialized.\r\n");

    queue_init(&job_queue);
    queue_init(&ready_queue);
    queue_init(&device_queue);

    total_processes = 0;
    current_pcb = NULL;
    next_pcb = NULL;
}

void process_create(void (*func)(void), uint32_t pid, uint8_t priority) 
{
    if (pid >= MAX_PROCESSES) return;

    PCB_t *p = &pcb_table[pid];
    p->pid = pid;
    p->entry = func;
    p->state = PROC_NEW;

    uint32_t *sp = &stacks[pid][STACK_SIZE];

    *(--sp) = 0x01000000UL;        /* xPSR: Thumb bit set */
    *(--sp) = (uint32_t)func;      /* PC -> entry function */
    *(--sp) = 0xFFFFFFFDUL;        /* LR -> EXC_RETURN (return to Thread mode, use PSP) */
    *(--sp) = 0;                   /* R12 */
    *(--sp) = 0;                   /* R3 */
    *(--sp) = 0;                   /* R2 */
    *(--sp) = 0;                   /* R1 */
    *(--sp) = 0;                   /* R0 */

    for (int i = 0; i < 8; ++i) {
        *(--sp) = 0; /* R11 .. R4 placeholders */
    }

    p->stack_ptr = sp;

    p->state = PROC_NEW;

    p->dynamic_priority = priority;
    p->static_priority = priority;
    p->time_slice = 5;
    p->total_cpu_runtime = 0;
    p->wake_up_tick = 0;

    OS_ENTER_CRITICAL();
    queue_enqueue(&job_queue, p);
    OS_EXIT_CRITICAL();

    uart_print("Created process ");
    uart_print_dec(pid);
    uart_print(" -> state: ");
    uart_print(process_state_str(p->state));
    uart_print("\r\n");

    total_processes++;
}

// void process_set_state(uint32_t pid, process_state_t new_state) {
//     if (pid >= (uint32_t)MAX_PROCESSES) return;
//     pcb_table[pid].state = new_state;
// }

void process_admit_jobs(void) {
    OS_ENTER_CRITICAL();
    while (!queue_is_empty(&job_queue)) {
        PCB_t *p = queue_dequeue(&job_queue);
        if (!p) break;
        p->state = PROC_READY;
        queue_enqueue(&ready_queue, p);
        uart_print("Admitted process ");
        uart_print_dec(p->pid);
        uart_print(" -> READY\r\n");
    }
    OS_EXIT_CRITICAL();
}

void process_schedule(void) {
    OS_ENTER_CRITICAL();

    // 1. Kiểm tra nếu không có process READY
    if (queue_is_empty(&ready_queue)) {
    OS_EXIT_CRITICAL();
    return;}

    // 2. Lấy process tiếp theo từ hàng đợi READY
    PCB_t *pnext = queue_dequeue(&ready_queue);
    if (!pnext) return;

    // 3. Xử lý task hiện tại
    if (current_pcb != NULL) {
        // SỬA 5: Logic quan trọng cho Blocking!
        // Chỉ enqueue lại nếu nó vẫn đang RUNNING (tức là hết giờ time-slice).
        // Nếu nó gọi os_delay, state đã là BLOCKED -> KHÔNG enqueue lại.
        if (current_pcb->state == PROC_RUNNING) {
            current_pcb->state = PROC_READY;
            queue_enqueue(&ready_queue, current_pcb);
        }
    }

    pnext->state = PROC_RUNNING;
    OS_EXIT_CRITICAL();

    uart_print("Switching to process ");
    uart_print_dec(pnext->pid);
    uart_print(" (");
    uart_print(process_state_str(pnext->state));
    uart_print(")\r\n");

    /* Nếu chưa có process nào chạy*/
    if (current_pcb == NULL) {
        current_pcb = pnext;
        /*set PSP và 'exception return' vào task đầu */
        start_first_task(current_pcb->stack_ptr);
    } else {
        next_pcb = pnext;
        SCB_ICSR |= PENDSVSET_BIT;
    }
}

/* */
void os_delay(uint32_t ticks) {
    // 1. Tính thời điểm báo thức
    // Nếu ticks = 100, hiện tại là 500 -> wake_up_tick = 600
    current_pcb->wake_up_tick = tick_count + ticks;

    // 2. Chuyển trạng thái sang BLOCKED
    current_pcb->state = PROC_BLOCKED;

    // gọi scheduler để tìm task mới 
    process_schedule();
    // 3. Nhường CPU ngay lập tức!
    // Không chờ hết time-slice, ta kích hoạt PendSV để đổi task ngay
    // SCB_ICSR |= PENDSVSET_BIT; (Bạn đã define macro này rồi)
    //*(volatile uint32_t*)0xE000ED04 |= (1UL << 28); 
}

void process_timer_tick(void) {
    tick_count++; // Tăng giờ hệ thống

    /* Quét mảng để tìm Task ngủ dậy */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        PCB_t *p = &pcb_table[i];
        
        // Chỉ kiểm tra task nào đang BLOCKED và có PID hợp lệ (đã tạo)
        if (p->state == PROC_BLOCKED) {
            if (p->wake_up_tick <= tick_count) {
                // Dậy thôi!
                p->state = PROC_READY;
                p->wake_up_tick = 0; // Reset
                
                // Đưa lại vào hàng đợi READY
                queue_enqueue(&ready_queue, p);
                
                // (Optional) Debug log
                // uart_print("Task woken up: "); uart_print_dec(p->pid); uart_print("\r\n");
            }
        }
    }
}