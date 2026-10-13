#ifndef USER_SPACE_H
#define USER_SPACE_H

#include <stdbool.h>
#include <stdint.h>

#include "minemu/boot.h"


#define USER_SPACE_BASE   UINT32_C(0x00400000)
#define USER_STACK_BOTTOM UINT32_C(0x007ff000)
#define USER_STACK_TOP    UINT32_C(0x00800000)
#define USER_SPACE_END    USER_STACK_TOP


bool user_map_system_rom(const struct minemu_boot_info *boot_info);


const struct minemu_module_record *user_find_module(
    const struct minemu_boot_info *boot_info,
    const char *name);


bool user_load_module(
    const struct minemu_boot_info *boot_info,
    const struct minemu_module_record *module);


bool user_range_ok(uint32_t vaddr, uint32_t length, uint32_t required);


void minemu_enter_user(uint32_t entry_vaddr, uint32_t stack_top)
    __attribute__((noreturn));

#endif