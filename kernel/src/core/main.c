#include "minemu/boot.h"
#include "minemu/trap.h"
#include "minemu/trace.h"
#include "minemu/platform.h"
#include "minemu/irq.h"

int uart_getc(char *out);

#define MSH_LINE_MAX 20

static void uart_putc(char byte)
{
    while ((MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY) == 0U) {
    }

    MINEMU_UART0->tx_data = (uint32_t)byte;
}

static void uart_puts (const char *string) {
    while (*string != '\0') {
        uart_putc(*string);
        string++;
    }
}

static void msh_execute(char *line)
{
    char *command = line;

    while (*command == ' ') {
        command++;
    }

    if (*command == '\0') {
        return;
    }

    if (command[0] == 'e' &&
        command[1] == 'c' &&
        command[2] == 'h' &&
        command[3] == 'o' &&
        (command[4] == '\0' || command[4] == ' ')) {

        char *text = command + 4;

        while (*text == ' ') {
            text++;
        }

        uart_puts(text);
        uart_putc('\n');
        return;
    }

    char *end = command;

    while (*end != '\0' && *end != ' ') {
        end++;
    }

    char saved = *end;
    *end = '\0';

    uart_puts("command not found: ");
    uart_puts(command);
    uart_putc('\n');

    *end = saved;
}


static void msh_run(void)
{
    char line[MSH_LINE_MAX + 1];
    unsigned int logical_length = 0;

    uart_puts("msh> ");

    for (;;) {
        char byte;

        if (!uart_getc(&byte)) {
            continue;
        }

        if (byte == 0x08 || byte == 0x7f) {
            if (logical_length > 0) {
                logical_length--;
            }

            continue;
        }

        if (byte == '\n') {
            if (logical_length <= MSH_LINE_MAX) {
                line[logical_length] = '\0';
                msh_execute(line);
            }

            logical_length = 0;
            uart_puts("msh> ");
            continue;
        }

        if (logical_length < MSH_LINE_MAX) {
            line[logical_length] = byte;
        }

        logical_length++;
    }
}


void minemu_kernel_main(const struct minemu_boot_info *boot_info) {
    if ((uintptr_t)boot_info != MINEMU_BOOT_INFO_VADDR ||
        boot_info->magic != MINEMU_BOOT_INFO_MAGIC ||
        boot_info->version != MINEMU_ABI_VERSION ||
        boot_info->size != sizeof(*boot_info) ||
        boot_info->system_rom_base != UINT32_C(0x08000000) ||
        boot_info->direct_map_vaddr != UINT32_C(0xc0000000) ||
        boot_info->direct_map_paddr != UINT32_C(0x40000000) ||
        boot_info->direct_map_size != UINT32_C(0x04000000)) {
        minemu_trace_event(UINT32_C(0xb007bad0));
        minemu_fail_stop();
    }
    
    uart_puts("hello world\n");
    
    MINEMU_UART0->control |= MINEMU_UART_CONTROL_RX_IRQ_ENABLE;

    MINEMU_INTERRUPT->enable |= UINT32_C(1) << MINEMU_IRQ_UART0;

    minemu_irq_enable();

    msh_run();
   
}
