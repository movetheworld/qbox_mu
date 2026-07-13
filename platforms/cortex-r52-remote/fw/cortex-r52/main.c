/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "addr_map.h"
#include "gicv3.h"
#include "uart.h"

void invalid_excp(void) { uart_puts("invalid exception happened\r\n"); }

void handle_irq(void)
{
    uint32_t intid = gic_acknowledge_irq();

    if (intid == GIC_INTID_TEST) {
        uart_puts("SPI 33 happened\r\n");
        *(volatile unsigned int*)IRQ_TEST_CLEAR = 1U;
    } else if (intid == GIC_INTID_UART) {
        uart_puts("UART SPI happened\r\n");
    } else {
        uart_puts("unexpected IRQ\r\n");
    }

    gic_end_of_interrupt(intid);
}

void uart_driver_init(void)
{
    uart_init();
    gic_init();
}

void c_entry(void)
{
    unsigned int i = 1;
    while (i);
    gic_enable_spi(GIC_INTID_TEST);
    gic_enable_cpu_interface();
    __asm__ volatile("cpsie i");

    uart_puts("hello world\r\n");
    uart_puts("Test program is running. Listening for interrupts.\r\n");
    *(volatile unsigned int*)IRQ_TEST_START = 1U;

    /* Poll GICD until the injected SPI is visible. */
    for (volatile unsigned int i = 0U; i < 0x8000000U; i++) {
        if (gic_read32(GICD_ISPENDR + 4U) & (1U << 1)) {
            uart_puts("SPI 33 happened\r\n");
            *(volatile unsigned int*)IRQ_TEST_CLEAR = 1U;
            break;
        }
    }

    unsigned int test = 0;
    unsigned int testb = 0;
    while (1) {
        test = *(volatile unsigned int*)0x80000000;
        testb = *(volatile unsigned int*)0x80000004;
        test++;
        testb++;
        uart_puts("hello world\r\n");
        *(volatile unsigned int*)0x80000000 = test;
        *(volatile unsigned int*)0x80000004 = testb;
    }
}
