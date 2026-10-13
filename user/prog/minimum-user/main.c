#include <stdint.h>

#include "minemu/user_abi.h"

#define MSH_LINE_MAX 20U
#define MSH_BUFFER_MAX (MSH_LINE_MAX + 5U)

static void uart_putc(char byte) {
    (void)ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE,
                (void *)&byte, 1U);
}

static void uart_puts(const char *string) {
    uint32_t length = 0;

    while (string[length] != '\0') {
        ++length;
    }

    (void)ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE,
                (void *)string, length);
}

static int is_echo(const char *command) {
    return command[0] == 'e' &&
           command[1] == 'c' &&
           command[2] == 'h' &&
           command[3] == 'o' &&
           (command[4] == '\0' || command[4] == ' ');
}

static void msh_execute(char *line, uint32_t length) {
    char *command = line;

    while (*command == ' ') {
        ++command;
    }

    if (*command == '\0') {
        return;
    }

    if (length > MSH_LINE_MAX && !is_echo(command)) {
        return;
    }

    if (is_echo(command)) {
        char *text = command + 4;

        while (*text == ' ') {
            ++text;
        }

        char *out = text;
        int have_text = 0;
        int pending_space = 0;

        for (char *in = text; *in != '\0'; ++in) {
            if (*in == ' ') {
                pending_space = have_text;
            } else {
                if (pending_space) {
                    *out++ = ' ';
                    pending_space = 0;
                }

                *out++ = *in;
                have_text = 1;
            }
        }

        *out = '\0';

        uart_puts(text);
        uart_putc('\n');
        return;
    }

    char *end = command;

    while (*end != '\0' && *end != ' ') {
        ++end;
    }

    *end = '\0';

    uart_puts("command not found: ");
    uart_puts(command);
    uart_putc('\n');
}

void minemu_user_main(void) {
    static char prompt[] = "msh> ";
    char line[MSH_BUFFER_MAX + 1U];
    uint32_t logical_length = 0;

    uart_puts(prompt);

    for (;;) {
        char byte;

        if (ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ,
                  (void *)&byte, 1U) != 1) {
            continue;
        }

        if (byte == '\b' || (uint8_t)byte == UINT8_C(0x7f)) {
            if (logical_length > 0) {
                --logical_length;
            }
            continue;
        }

        if (byte == '\n') {
            if (logical_length <= MSH_BUFFER_MAX) {
                line[logical_length] = '\0';
                msh_execute(line, logical_length);
            }

            logical_length = 0;
            uart_puts(prompt);
            continue;
        }

        if (logical_length == 0 && byte == ' ') {
            continue;
        }

        if (logical_length == MSH_BUFFER_MAX && byte == ' ') {
            continue;
        }

        if (logical_length < MSH_BUFFER_MAX) {
            line[logical_length] = byte;
        }

        ++logical_length;
    }
}