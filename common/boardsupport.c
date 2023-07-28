#include <stdio.h>
#include <stdint.h>
#include "boardsupport.h"

#define UART_BASE_REG       0x88001000
#define UART_RX_REG         (UART_BASE_REG + 0x0)
#define UART_TX_REG         (UART_BASE_REG + 0x0)
#define UART_DLL_REG        (UART_BASE_REG + 0x0)
#define UART_DLM_REG        (UART_BASE_REG + 0x4)
#define UART_LCR_REG	    (UART_BASE_REG + 0xC)
#define UART_LCR_REG_DLAB   0x00000080
#define UART_LCR_REG_WLS_8B 0x00000003
#define UART_LSR_REG	    (UART_BASE_REG + 0x14)
#define UART_LSR_REG_THRE   0x00000020
#define UART_LSR_REG_DR     0x00000001
#define UART_BAUDRATE       115200
#define UART_CLK_FREQ       100000000

uint64_t begin_time, end_time, user_time;
uint64_t start_instruction_count, end_instruction_count, user_instruction_count;
uint64_t scaled_ipc;

static int output_char(char c, FILE *file)
{
    (void) file;
    //Ensure space in buffer
    while (!((*(volatile int32_t *)UART_LSR_REG) & UART_LSR_REG_THRE));

    *(volatile int32_t *)UART_TX_REG = (int32_t) c;

    return 0;
}

static int input_char(FILE *file)
{
    (void) file;
    //Wait for character
    while (!((*(volatile int32_t *)UART_LSR_REG) & UART_LSR_REG_DR));

    return *((volatile int32_t *)UART_RX_REG);
}

// Adapted from XilinxProcessorIPLib/drivers/uartns550/src/xuartns550_l.c
static void set_uart_baudrate(int32_t clk_freq, int32_t baudrate)
{
    /*
     * Determine what the divisor should be to get the specified baud
     * rater based upon the input clock frequency and a baud clock prescaler
     * of 16
     */
    int32_t divisor = ((clk_freq + ((baudrate * 16UL)/2)) / (baudrate * 16UL));

    /*
     * Get the least significant and most significant bytes of the divisor
     * so they can be written to 2 byte registers
     */
    int32_t divisor_lsb = divisor & 0xFF;
    int32_t divisor_msb = (divisor >> 8) & 0xFF;

    /*
     * Get the line control register contents and set the divisor latch
     * access bit so the baud rate can be set
     */
    int32_t lcr_register = *((volatile int32_t *)UART_LCR_REG);
    *(volatile int32_t *)UART_LCR_REG = lcr_register | UART_LCR_REG_DLAB;

    /*
     * Set the baud divisors to set rate, the initial write of 0xFF is to
     * keep the divisor from being 0 which is not recommended as per the
     * NS16550D spec sheet
     */
    *(volatile int32_t *)UART_DLL_REG = 0xFF;
    *(volatile int32_t *)UART_DLM_REG = divisor_msb;
    *(volatile int32_t *)UART_DLL_REG = divisor_lsb;

    /*
     * Clear the divisor latch access bit, DLAB to allow nornal
     * operation and write to the line control register
     */
    *(volatile int32_t *)UART_LCR_REG = lcr_register;
}


static FILE __stdio = FDEV_SETUP_STREAM(output_char, input_char, NULL, _FDEV_SETUP_RW);

FILE *const stdin  = &__stdio;
FILE *const stdout = &__stdio;
FILE *const stderr = &__stdio;

uint64_t read_cycle()
{
    uint64_t result;
    uint32_t lower;
    uint32_t upper1;
    uint32_t upper2;

    asm volatile (
            "repeat_cycle_%=: csrr %0, cycleh;\n"
            "        csrr %1, cycle;\n"
            "        csrr %2, cycleh;\n"
            "        bne %0, %2, repeat_cycle_%=;\n"
            : "=r" (upper1),"=r" (lower),"=r" (upper2)
            :
            :
            );

    result = (uint64_t) upper1 << 32 | lower;
    return result;
}

uint64_t read_inst()
{
    uint64_t result;
    uint32_t lower;
    uint32_t upper1;
    uint32_t upper2;

    asm volatile (
            "repeat_inst_%=: csrr %0, instreth;\n"
            "        csrr %1, instret;\n"
            "        csrr %2, instreth;\n"
            "        bne %0, %2, repeat_inst_%=;\n"
            : "=r" (upper1),"=r" (lower),"=r" (upper2)
            :
            :
            );

    result = (uint64_t) upper1 << 32 | lower;
    return result;
}

void __attribute__ ((constructor)) __attribute__ ((noinline)) __attribute__ ((externally_visible)) initialise_board ()
{
    set_uart_baudrate(UART_CLK_FREQ, UART_BAUDRATE);
    *(volatile int32_t *)UART_LCR_REG = UART_LCR_REG_WLS_8B;
    asm volatile ("li a0, 0" : : : "memory");
}

void __attribute__ ((noinline)) __attribute__ ((externally_visible)) start_trigger ()
{
    begin_time = read_cycle();
    start_instruction_count = read_inst();
    asm volatile ("addi x0, x0, 0xC" : : : "memory");
}

void __attribute__ ((noinline)) __attribute__ ((externally_visible)) stop_trigger ()
{
    asm volatile ("addi x0, x0, 0xD" : : : "memory");
    end_time = read_cycle();
    end_instruction_count = read_inst();
    user_time = end_time - begin_time;
    user_instruction_count = end_instruction_count - start_instruction_count;

    printf("Begin time  : %lld\r\n", begin_time);
    printf("End time    : %lld\r\n", end_time);
    printf("User time   : %lld\r\n", user_time);
    printf("Begin inst  : %lld\r\n", start_instruction_count);
    printf("End inst    : %lld\r\n", end_instruction_count);
    printf("User inst   : %lld\r\n", user_instruction_count);
}

void __attribute__ ((__noreturn__)) _exit (int status)
{
    if (status == 0)
        printf("Result: CORRECT\r\n");
    else
        printf("Result: FAILED\r\n");

    asm volatile ("addi x0, x0, 0xA" : : : "memory");
    while(1);
}
