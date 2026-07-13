/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADDR_MAP_H
#define ADDR_MAP_H

/* Must match conf.lua and addr-map.xlsx */
#define RAM_BASE           0x00000000UL
#define RAM_SIZE           0x00020000UL
#define GICD_BASE          0x2F000000UL
#define GICD_SIZE          0x00010000UL
#define GICR_BASE          0x2F100000UL
#define GICR_SIZE          0x00020000UL
#define UART_BASE          0xC0000000UL
#define UART_SIZE          0x00001000UL
#define IRQ_TEST_BASE      0xC0001000UL
#define IRQ_TEST_SIZE      0x00001000UL

#define GIC_SPI_UART       0
#define GIC_SPI_TEST       1
#define GIC_INTID_UART     (32U + GIC_SPI_UART)
#define GIC_INTID_TEST     (32U + GIC_SPI_TEST)

#define IRQ_TEST_CLEAR     (IRQ_TEST_BASE + 0x0UL)
#define IRQ_TEST_START     (IRQ_TEST_BASE + 0x4UL)

#endif /* ADDR_MAP_H */
