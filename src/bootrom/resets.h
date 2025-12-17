/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RESETS_H
#define _RESETS_H

#include "pico/types.h"

void fake_reset_block_noinline(uint32_t mask);
void fake_unreset_block_wait_noinline(uint32_t mask);
void fake_reset_unreset_block_wait_noinline(uint32_t mask);

#endif
