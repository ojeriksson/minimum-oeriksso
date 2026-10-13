#include <stdint.h>
#include "minemu/boot.h"
#include "minemu/irq.h"
#include "minemu/platform.h"
#include "minemu/runtime.h"
#include "minemu/trace.h"
#include "minemu/trap.h"
#include "user_space.h"

void minemu_kernel_main(const struct minemu_boot_info *boot_info) {
    if ((uintptr_t)boot_info != MINEMU_BOOT_INFO_VADDR ||
        boot_info->magic != MINEMU_BOOT_INFO_MAGIC ||
        boot_info->version != MINEMU_ABI_VERSION ||
        boot_info->size != sizeof(*boot_info) ||
        boot_info->system_rom_base != MINEMU_SYSTEM_ROM_BASE ||
        boot_info->direct_map_vaddr != MINEMU_KERNEL_DIRECT_BASE ||
        boot_info->direct_map_paddr != MINEMU_RAM_BASE ||
        boot_info->direct_map_size != MINEMU_KERNEL_DIRECT_SIZE) {
        minemu_trace_event(UINT32_C(0xb007bad0));
        minemu_fail_stop();
    }

    if (!user_map_system_rom(boot_info)) {
        minemu_panic("cannot map image");
    }
    const struct minemu_module_record *module = user_find_module(boot_info, "minimum-user");
    if (module == NULL || !user_load_module(boot_info, module)) {
        minemu_panic("cannot load minimum-user");
    }

    MINEMU_UART0->control |= MINEMU_UART_CONTROL_RX_IRQ_ENABLE;
    MINEMU_INTERRUPT->enable |= UINT32_C(1) << MINEMU_IRQ_UART0;
    minemu_enter_user(module->entry_vaddr, USER_STACK_TOP);
}