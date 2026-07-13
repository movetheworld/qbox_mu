/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GICV3_H
#define GICV3_H

#include <stdint.h>
#include "addr_map.h"

#define GICD_CTLR          (GICD_BASE + 0x0000UL)
#define GICD_IGROUPR       (GICD_BASE + 0x0080UL)
#define GICD_ISENABLER     (GICD_BASE + 0x0100UL)
#define GICD_ISPENDR       (GICD_BASE + 0x0200UL)
#define GICD_IPRIORITYR    (GICD_BASE + 0x0400UL)
#define GICD_IROUTER       (GICD_BASE + 0x6000UL)

#define GICR_CTLR          (GICR_BASE + 0x0000UL)
#define GICR_WAKER         (GICR_BASE + 0x0014UL)

#define GICD_CTLR_ENABLE_G1A (1U << 1)
#define GICD_CTLR_ARE_NS     (1U << 4)
#define GICD_CTLR_ARE        (1U << 5)

static inline void gic_write32(uint32_t addr, uint32_t value)
{
    *(volatile uint32_t*)addr = value;
}

static inline uint32_t gic_read32(uint32_t addr)
{
    return *(volatile uint32_t*)addr;
}

static inline void gic_write8(uint32_t addr, uint8_t value)
{
    *(volatile uint8_t*)addr = value;
}

static inline void gic_route_spi_to_cpu0(uint32_t spi)
{
    /* Route to any connected PE (required for some GICv3 models). */
    gic_write32(GICD_IROUTER + (spi * 8U), 0x80000000U);
    gic_write32(GICD_IROUTER + (spi * 8U) + 4U, 0U);
}

static inline uint32_t icc_sre_read(void)
{
    uint32_t value;
    __asm__ volatile("mrc p15, 0, %0, c12, c12, 5" : "=r"(value));
    return value;
}

static inline void icc_sre_write(uint32_t value)
{
    __asm__ volatile("mcr p15, 0, %0, c12, c12, 5" : : "r"(value));
}

static inline void icc_igrpen1_write(uint32_t value)
{
    __asm__ volatile("mcr p15, 0, %0, c12, c12, 7" : : "r"(value));
}

static inline void icc_pmr_write(uint32_t value)
{
    __asm__ volatile("mcr p15, 0, %0, c4, c6, 0" : : "r"(value));
}

static inline uint32_t icc_iar1_read(void)
{
    uint32_t value;
    __asm__ volatile("mrc p15, 0, %0, c12, c12, 0" : "=r"(value));
    return value;
}

static inline void icc_eoir1_write(uint32_t value)
{
    __asm__ volatile("mcr p15, 0, %0, c12, c12, 1" : : "r"(value));
}

void gic_init(void);
void gic_enable_spi(uint32_t intid);
void gic_enable_cpu_interface(void);
uint32_t gic_acknowledge_irq(void);
void gic_end_of_interrupt(uint32_t intid);

#endif /* GICV3_H */
