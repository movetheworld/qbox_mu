/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>

#define UART_BASE 0xc0000000UL

#define UARTDR   (UART_BASE + 0x00)
#define UARTFR   (UART_BASE + 0x18)
#define UARTCR   (UART_BASE + 0x30)

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
    /* Enable TX/RX on the PL011 (UARTEN | TXE | RXE). */
    uart_write_reg(UARTCR, 0x301);
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
