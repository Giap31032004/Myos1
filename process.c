#include "process.h"
#include "uart.h"
#include "queue.h"
#include <stdint.h>

#define SCB_ICSR (*(volatile uint32_t*)0xE000ED04)
#define PENDSVSET_BIT (1UL << 28)

PCB_t *current_pcb = NULL;   
PCB_t *next_pcb = NULL;      

extern void start_first_task(uint32_t *first_sp); 
//extern void switch_context(uint32_t **old_sp_ptr, uint32_t *new_sp);

static queue_t job_queue;
static queue_t ready_queue;
static queue_t device_queue;

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

void process_create(void (*func)(void), uint32_t pid) 
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
    queue_enqueue(&job_queue, p);

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
    while (!queue_is_empty(&job_queue)) {
        PCB_t *p = queue_dequeue(&job_queue);
        if (!p) break;
        p->state = PROC_READY;
        queue_enqueue(&ready_queue, p);
        uart_print("Admitted process ");
        uart_print_dec(p->pid);
        uart_print(" -> READY\r\n");
    }
}

void process_schedule(void) {
    if (queue_is_empty(&ready_queue)) return;

    PCB_t *pnext = queue_dequeue(&ready_queue);
    if (!pnext) return;

    if (current_pcb != NULL && current_pcb->state == PROC_RUNNING) {
        current_pcb->state = PROC_READY;
        queue_enqueue(&ready_queue, current_pcb);
    }

    pnext->state = PROC_RUNNING;
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
