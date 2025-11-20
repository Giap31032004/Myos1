.syntax unified
.cpu cortex-m3
.thumb

/* ========================================
   PHẦN 1: KHAI BÁO GLOBAL & EXTERN
   ======================================== */
.global vector_table
.global Reset_Handler

/* Các symbol được định nghĩa ở file khác hoặc Linker Script */
.extern _estack           /* Đỉnh Stack (từ Linker) */
.extern main              /* Hàm main (từ main.c) */
.extern PendSV_Handler    /* Nằm ở kernel/context_switch.s */
.extern SysTick_Handler   /* Nằm ở drivers/systick.c hoặc kernel */
.extern start_first_task  /* Nằm ở kernel/context_switch.s */

/* ========================================
   PHẦN 2: VECTOR TABLE
   ======================================== */
.section .isr_vector, "a", %progbits
.type vector_table, %object

vector_table:
    .word _estack            /* Top of Stack */
    .word Reset_Handler      /* Reset Handler */
    .word Default_Handler    /* NMI */
    .word Default_Handler    /* HardFault */
    .word Default_Handler    /* MemManage */
    .word Default_Handler    /* BusFault */
    .word Default_Handler    /* UsageFault */
    .word 0,0,0,0            /* Reserved */
    .word Default_Handler    /* SVC */
    .word Default_Handler    /* DebugMon */
    .word 0                  /* Reserved */
    .word PendSV_Handler     /* PendSV - Quan trọng cho OS */
    .word SysTick_Handler    /* SysTick - Quan trọng cho OS */

    /* External Interrupts (Placeholder cho 32 ngắt ngoại vi) */
    .rept 32
        .word Default_Handler
    .endr

/* ========================================
   PHẦN 3: RESET HANDLER
   ======================================== */
.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function

Reset_Handler:
    /* LƯU Ý: Ở đây đang thiếu đoạn copy dữ liệu (xem phân tích bên dưới) 
    */
    
    bl main        /* Nhảy vào hàm main C */
    b .            /* Nếu main trả về (không nên xảy ra), treo tại đây */

/* ========================================
   DEFAULT HANDLER
   ======================================== */
.section .text.Default_Handler
.weak Default_Handler
.type Default_Handler, %function

Default_Handler:
    b .            /* Vòng lặp vô hạn nếu gặp lỗi/ngắt chưa định nghĩa */
    