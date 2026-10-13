#include <stdint.h>

#include "minemu/mmu.h"
#include "minemu/platform.h"
#include "minemu/trap.h"
#include "minemu/user_abi.h"
#include "user_space.h"

#define CPSR_MODE_MASK UINT32_C(0x1f)
#define CPSR_MODE_USR  UINT32_C(0x10)
#define CPSR_THUMB     UINT32_C(0x20)
#define A32_SVC_ZERO   UINT32_C(0xef000000)

int uart_getc(char *out);

static int32_t uart_read(int fd, uint32_t address, uint32_t count) {
    if ((fd != MINEMU_FD_UART_IN && fd != MINEMU_FD_UART_DIAG) ||
        (int32_t)count < 0 ||
        !user_range_ok(address, count, MINEMU_PTE_WRITABLE)) {
        return MINEMU_IOCTL_ERROR;
    }

    char *destination = (char *)(uintptr_t)address;
    uint32_t copied = 0;
    if (fd == MINEMU_FD_UART_IN) {
        while (copied < count && uart_getc(&destination[copied])) {
            ++copied;
        }
    } else {
        while (copied < count && (MINEMU_UART1->status & MINEMU_UART_STATUS_RX_READY)) {
            destination[copied++] = (char)MINEMU_UART1->rx_data;
        }
    }
    return (int32_t)copied;
}

static int32_t uart_write(int fd, uint32_t address, uint32_t count) {
    volatile struct minemu_uart_regs *uart;
    if (fd == MINEMU_FD_UART_OUT) {
        uart = MINEMU_UART0;
    } else if (fd == MINEMU_FD_UART_DIAG) {
        uart = MINEMU_UART1;
    } else {
        return MINEMU_IOCTL_ERROR;
    }
    if ((int32_t)count < 0 || !user_range_ok(address, count, MINEMU_PTE_READABLE)) {
        return MINEMU_IOCTL_ERROR;
    }

    const char *source = (const char *)(uintptr_t)address;
    for (uint32_t i = 0; i < count; ++i) {
        while ((uart->status & MINEMU_UART_STATUS_TX_READY) == 0) {
        }
        uart->tx_data = (uint32_t)(uint8_t)source[i];
    }
    return (int32_t)count;
}

static int32_t ioctl_dispatch(int fd, uint32_t op, uint32_t arg0, uint32_t arg1) {
    switch (op) {
    case MINEMU_IOCTL_UART_READ:
        return uart_read(fd, arg0, arg1);
    case MINEMU_IOCTL_UART_WRITE:
        return uart_write(fd, arg0, arg1);
    case MINEMU_IOCTL_RNG_SEED:
        if (fd != MINEMU_FD_RNG) return MINEMU_IOCTL_ERROR;
        MINEMU_RNG->seed = arg0;
        return 0;
    case MINEMU_IOCTL_RNG_NEXT:
    case MINEMU_IOCTL_RNG_STATE:
        if (fd != MINEMU_FD_RNG ||
            !user_range_ok(arg0, sizeof(uint32_t), MINEMU_PTE_WRITABLE)) {
            return MINEMU_IOCTL_ERROR;
        }
        
        {
            uint32_t value = op == MINEMU_IOCTL_RNG_NEXT ? MINEMU_RNG->data : MINEMU_RNG->state;
            uint8_t *out = (uint8_t *)(uintptr_t)arg0;
            for (uint32_t i = 0; i < sizeof(value); ++i) {
                out[i] = (uint8_t)(value >> (i * 8));
            }
        }
        return 0;
    case MINEMU_IOCTL_TRACE_EVENT:
        if (fd != MINEMU_FD_TRACE) return MINEMU_IOCTL_ERROR;
        MINEMU_TRACE->event = arg0;
        return 0;
    default:
        return MINEMU_IOCTL_ERROR;
    }
}

struct minemu_trap_frame *minemu_svc_dispatch(struct minemu_trap_frame *frame) {
    if (frame->exception_id != MINEMU_EXCEPTION_SVC ||
        (frame->spsr & CPSR_MODE_MASK) != CPSR_MODE_USR ||
        (frame->spsr & CPSR_THUMB) != 0 ||
        !user_range_ok(frame->fault_pc, sizeof(uint32_t), MINEMU_PTE_READABLE) ||
        *(const uint32_t *)(uintptr_t)frame->fault_pc != A32_SVC_ZERO ||
        frame->r[7] != MINEMU_SYSCALL_IOCTL) {
        frame->r[0] = (uint32_t)MINEMU_IOCTL_ERROR;
        return frame;
    }
    frame->r[0] = (uint32_t)ioctl_dispatch((int)frame->r[0], frame->r[1],
                                           frame->r[2], frame->r[3]);
    return frame;
}