.syntax unified
.cpu cortex-m3
.thumb

/* ========================================
   PHẦN 1: KHAI BÁO GLOBAL & EXTERN
   ======================================== */
.global vector_table
.global Reset_Handler

.extern _estack
.extern _sidata   /* Đầu dữ liệu trong Flash */
.extern _sdata    /* Đầu dữ liệu trong RAM */
.extern _edata    /* Cuối dữ liệu trong RAM */
.extern _sbss     /* Đầu vùng BSS */
.extern _ebss     /* Cuối vùng BSS */

.extern main
.extern PendSV_Handler
.extern SysTick_Handler
.extern start_first_task

/* ========================================
   PHẦN 2: VECTOR TABLE
   ======================================== */
.section .isr_vector, "a", %progbits
.type vector_table, %object

vector_table:
    .word _estack
    .word Reset_Handler
    .word Default_Handler    /* NMI */
    .word Default_Handler    /* HardFault */
    .word Default_Handler    /* MemManage */
    .word Default_Handler    /* BusFault */
    .word Default_Handler    /* UsageFault */
    .word 0,0,0,0            /* Reserved */
    .word Default_Handler    /* SVC */
    .word Default_Handler    /* DebugMon */
    .word 0                  /* Reserved */
    .word PendSV_Handler     /* PendSV */
    .word SysTick_Handler    /* SysTick */
    
    .rept 32
        .word Default_Handler
    .endr

/* ========================================
   PHẦN 3: RESET HANDLER (Đã thêm đoạn Copy Data)
   ======================================== */
.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function

Reset_Handler:
    /* 1. Copy .data từ Flash sang RAM */
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata
    movs r3, #0
    b loop_copy_data

copy_data:
    ldr r4, [r2, r3]
    str r4, [r0, r3]
    adds r3, r3, #4

loop_copy_data:
    adds r4, r0, r3
    cmp r4, r1
    bcc copy_data

    /* 2. Xóa .bss về 0 */
    ldr r2, =_sbss
    ldr r4, =_ebss
    movs r3, #0
    b loop_zero_bss

zero_bss:
    str r3, [r2]
    adds r2, r2, #4

loop_zero_bss:
    cmp r2, r4
    bcc zero_bss

    /* 3. Vào Main */
    bl main
    b .

/* Default Handler */
.section .text.Default_Handler
.weak Default_Handler
.type Default_Handler, %function
Default_Handler:
    b .
    