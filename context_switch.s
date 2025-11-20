.syntax unified
.cpu cortex-m3
.thumb

/* ========================================
   KHAI BÁO GLOBAL & EXTERN
   ======================================== */
.global PendSV_Handler
.global start_first_task

/* Các biến toàn cục quản lý Task (từ C code) */
.extern current_pcb
.extern next_pcb

.section .text

/* ========================================
   HÀM: PendSV_Handler
   Mục đích: Context Switching thực tế
   ======================================== */
.type PendSV_Handler, %function
PendSV_Handler:
    /* --- BƯỚC 1: LƯU CONTEXT CŨ --- */
    MRS     r0, psp                 /* r0 = Lấy Process Stack Pointer hiện tại */
    CBZ     r0, load_next_task      /* Nếu PSP = 0 (lần đầu chạy), bỏ qua lưu */

    LDR     r1, =current_pcb        /* r1 = Địa chỉ của biến con trỏ current_pcb */
    LDR     r1, [r1]                /* r1 = Giá trị của current_pcb (địa chỉ struct) */
    CBZ     r1, load_next_task      /* Phòng hờ null pointer */

    /* Lưu các thanh ghi còn lại (R4-R11) vào stack của task đó */
    STMDB   r0!, {r4-r11}           
    
    /* Cập nhật lại đỉnh stack mới vào struct PCB */
    STR     r0, [r1]                /* current_pcb->stack_ptr = r0 mới */

load_next_task:
    /* --- BƯỚC 2: NẠP CONTEXT MỚI --- */
    LDR     r1, =next_pcb           /* r1 = Địa chỉ của biến next_pcb */
    LDR     r1, [r1]                /* r1 = Giá trị next_pcb */
    CBZ     r1, pend_exit           /* Nếu không có task tiếp theo, thoát */

    /* Lấy đỉnh stack từ struct PCB của task mới */
    LDR     r0, [r1]                /* r0 = next_pcb->stack_ptr */

    /* Cập nhật current_pcb = next_pcb */
    LDR     r2, =current_pcb
    STR     r1, [r2]

    /* Khôi phục các thanh ghi (R4-R11) từ stack của task mới */
    LDMIA   r0!, {r4-r11}

    /* Cập nhật thanh ghi PSP của CPU */
    MSR     psp, r0

pend_exit:
    /* --- BƯỚC 3: THOÁT NGẮT --- */
    /* EXC_RETURN: 0xFFFFFFFD => Return to Thread Mode, dùng PSP */
    ORR     lr, lr, #0x04           
    BX      lr                      

/* ========================================
   HÀM: start_first_task
   Mục đích: Chạy task đầu tiên (bootstrapping)
   Input: r0 = Initial Stack Pointer của task đầu tiên
   ======================================== */
.type start_first_task, %function
start_first_task:
    /* Thiết lập PSP ban đầu */
    MSR     psp, r0

    /* Giả lập việc restore context (pop R4-R11 rác ra khỏi stack để cân bằng) */
    MRS     r0, psp
    LDMIA   r0!, {r4-r11}
    MSR     psp, r0

    /* Chuyển sang dùng PSP và chế độ Unprivileged (hoặc Privileged tùy bit 0) */
    MOVS    r0, #2                  /* Bit 1 = 1 (PSP), Bit 0 = 0 (Privileged) */
    MSR     CONTROL, r0
    ISB                             /* Instruction Synchronization Barrier */

    /* Nhảy vào Task */
    LDR     lr, =0xFFFFFFFD         
    BX      lr
    