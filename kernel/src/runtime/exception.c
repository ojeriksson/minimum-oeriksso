#include "minemu/irq.h"
#include "minemu/syscall.h"
#include "minemu/platform.h"


 // The UART interrupt handler writes characters into this buffer.
 // The main kernel/shell reads characters out of it.

#define UART_RX_BUFFER_SIZE 32
 
static volatile char uart_rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint32_t uart_rx_head = 0;
static volatile uint32_t uart_rx_tail = 0;



//Handle a UART0 receive interrupt.
// read every available byte from rx_data. Reading rx_data
// removes that byte from the UART hardware buffer.

static void uart0_irq_handler(void)
{
    while ((MINEMU_UART0->status & MINEMU_UART_STATUS_RX_READY) != 0U) {

        char byte = (char)(MINEMU_UART0->rx_data & UINT32_C(0xff));

        uint32_t next =
            (uart_rx_head + 1U) % UART_RX_BUFFER_SIZE;

        
         //Only store the byte if the software buffer has room.
        if (next != uart_rx_tail) {
            uart_rx_buffer[uart_rx_head] = byte;
            uart_rx_head = next;
        }
    }
}


//Try to read one byte from the software receive buffer.
int uart_getc(char *out)
{
    int available = 0;

    minemu_irq_disable();

    if (uart_rx_head != uart_rx_tail) {
        *out = uart_rx_buffer[uart_rx_tail];

        uart_rx_tail =
            (uart_rx_tail + 1U) % UART_RX_BUFFER_SIZE;

        available = 1;
    }

    minemu_irq_enable();

    return available;
}

void minemu_fail_stop(void) {
    for (;;) {
        __asm__ volatile("nop");
    }
}

void minemu_panic(const char *message) {
    (void)message;
    minemu_fail_stop();
}

__attribute__((weak, noreturn)) void minemu_svc_trampoline(void) {
    minemu_fail_stop();
}



__attribute__((weak)) struct minemu_trap_frame *minemu_svc_dispatch(
    struct minemu_trap_frame *frame) {
    (void)frame;
    minemu_fail_stop();
}

__attribute__((weak)) void minemu_undefined_dispatch(struct minemu_trap_frame *frame) {
    (void)frame;
    minemu_fail_stop();
}

__attribute__((weak)) void minemu_abort_dispatch(struct minemu_trap_frame *frame) {
    (void)frame;
    minemu_fail_stop();
}


//Called by the IRQ trampoline after it builds the trap frame.
__attribute__((weak))
struct minemu_trap_frame *
minemu_irq_dispatch(struct minemu_trap_frame *frame)
{
    uint32_t source = (uint32_t)frame->exception_id;

    if (source == MINEMU_IRQ_UART0) {
        uart0_irq_handler();
    }

    if (source != MINEMU_IRQ_NONE) {
        MINEMU_INTERRUPT->eoi = source;
    }

    //No context switching yet, so return the same frame.
    return frame;
}
