/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "nvic.h"
#include "uart.h"

void uart_driver_init(void)
{
    uart_init();
    _enable_irq();
}

void __attribute__((interrupt)) invalid_excp(void)
{
    uart_puts("invalid exception happened\r\n");
}

void __attribute__((interrupt)) _handle_irq(void)
{
    uart_puts("IRQ 17 happened\r\n");
    *(volatile unsigned int*)0xc0001000 = 1;
}

void __attribute__((interrupt)) _handle_nmi(void)
{
    uart_puts("NMI happened\r\n");
    *(volatile unsigned int*)0xc0001004 = 1;
}

void __attribute__((interrupt)) _handle_systick(void)
{
    uart_puts("SysTick happened\r\n");
    *(volatile unsigned int*)0xc0001008 = 1;
}

void c_entry(void)
{
    nvic_enable_irq(0);
    nvic_enable_irq(17);

    uart_puts("hello world\r\n");
    uart_puts("Test program is running. Listening for interrupts.\r\n");
    *(volatile unsigned int*)0xc000100c = 1; // start IRQs generation

    while (1) {
        asm volatile("wfi");
    }
}
