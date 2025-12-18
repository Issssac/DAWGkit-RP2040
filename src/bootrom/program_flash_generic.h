/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PROGRAM_FLASH_GENERIC_H
#define _PROGRAM_FLASH_GENERIC_H

#include <stdint.h>
#include <stddef.h>

void f_connect_internal_flash();
void f_flash_init_spi();
void f_flash_put_get(const uint8_t *tx, uint8_t *rx, size_t count, size_t rx_skip);
void f_flash_do_cmd(uint8_t cmd, const uint8_t *tx, uint8_t *rx, size_t count);
void f_flash_exit_xip();
void f_flash_page_program(uint32_t addr, const uint8_t *data);
void f_flash_range_program(uint32_t addr, const uint8_t *data, size_t count);
void f_flash_sector_erase(uint32_t addr);
void f_flash_user_erase(uint32_t addr, uint8_t cmd);
void f_flash_range_erase(uint32_t addr, size_t count, uint32_t block_size, uint8_t block_cmd);
void f_flash_read_data(uint32_t addr, uint8_t *rx, size_t count);
int f_flash_size_log2();
void f_flash_flush_cache();
void f_flash_enter_cmd_xip();
void f_flash_abort();
int f_flash_was_aborted();

#endif // _PROGRAM_FLASH_GENERIC_H_
