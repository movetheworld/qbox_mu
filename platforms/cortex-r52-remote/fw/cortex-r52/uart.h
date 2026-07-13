/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "addr_map.h"

#define UARTDR   (UART_BASE + 0x00UL)
#define UARTFR   (UART_BASE + 0x18UL)
#define UARTCR   (UART_BASE + 0x30UL)

#define UARTFR_TXFF (1U << 5)

static inline void uart_write_reg(uint32_t addr, uint32_t value)
{
    *(volatile uint32_t*)addr = value;
}

static inline uint32_t uart_read_reg(uint32_t addr)
{
    return *(volatile uint32_t*)addr;
}

static inline void uart_init(void)
{
    uart_write_reg(UARTCR, 0x301U);
}

static inline void uart_putc(char c)
{
    while (uart_read_reg(UARTFR) & UARTFR_TXFF) {
    }
    uart_write_reg(UARTDR, (uint32_t)c);
}

static inline void uart_puts(const char* str)
{
    while (*str) {
        uart_putc(*str++);
    }
}

#endif /* UART_H */
